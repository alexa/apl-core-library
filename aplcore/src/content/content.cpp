/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <algorithm>

#include "apl/engine/arrayify.h"
#include "apl/engine/parameterarray.h"
#include "apl/content/package.h"
#include "apl/engine/propdef.h"
#include "apl/content/importrequest.h"
#include "apl/content/content.h"
#include "apl/content/jsondata.h"
#include "apl/content/metrics.h"
#include "apl/command/arraycommand.h"
#include "apl/utils/log.h"
#include "apl/utils/session.h"

namespace apl {

static const bool DEBUG_CONTENT = false;

const char *DOCUMENT_IMPORT = "import";
const char *DOCUMENT_MAIN_TEMPLATE = "mainTemplate";

ContentPtr
Content::create(JsonData&& document)
{
    return create(std::move(document), makeDefaultSession());
}

ContentPtr
Content::create(JsonData&& document, const SessionPtr& session)
{
    if (!document) {
        CONSOLE_S(session).log("Document parse error offset=%u: %s", document.offset(), document.error());
        return nullptr;
    }

    auto ptr = Package::create(session, Path::MAIN, std::move(document));
    if (!ptr)
        return nullptr;

    const rapidjson::Value& json = ptr->json();
    auto it = json.FindMember(DOCUMENT_MAIN_TEMPLATE);
    if (it == json.MemberEnd()) {
        CONSOLE_S(session) << "Document does not contain a mainTemplate property";
        return nullptr;
    }

    ParameterArray params(it->value);
    std::vector<std::string> parameterNames;
    for (const auto& v : params)
        parameterNames.push_back(v.name);

    return std::make_shared<Content>(session, ptr, it->value, std::move(parameterNames));
}

Content::Content(const SessionPtr &session,
                 const PackagePtr& mainPackagePtr,
                 const rapidjson::Value& mainTemplate,
                 std::vector<std::string>&& parameterNames)
    : mSession(session),
      mMainPackage(mainPackagePtr),
      mMainParameters(std::move(parameterNames)),
      mState(LOADING),
      mMainTemplate(mainTemplate)
{
    addImportList(*mMainPackage);
    updateStatus();
}

PackagePtr
Content::getPackage(const std::string& name) const
{
    if (name == Path::MAIN)
        return mMainPackage;

    auto it = std::find_if(mLoaded.begin(), mLoaded.end(), [&name](const std::pair<ImportRef, PackagePtr>& ref) {
        return ref.first.name() == name;
    });

    return it != mLoaded.end() ? it->second : nullptr;
}

std::set<ImportRequest>
Content::getRequestedPackages()
{
    mPending.insert(mRequested.begin(), mRequested.end());
    auto result = mRequested;
    mRequested.clear();
    return result;
}

void
Content::addPackage(const ImportRequest& request, JsonData&& raw)
{
    if (mState != LOADING)
        return;

    // If the package data is invalid, set the error state
    if (!raw) {
        CONSOLE_S(mSession).log("Package %s (%s) parse error offset=%u: %s",
             request.reference().name().c_str(),
             request.reference().version().c_str(),
             raw.offset(), raw.error());
        mState = ERROR;
        return;
    }

    // We expect packages to be objects
    if (!raw.get().IsObject()) {
        CONSOLE_S(mSession).log("Package %s (%s) is not a JSON object",
            request.reference().name().c_str(),
            request.reference().version().c_str());
        mState = ERROR;
        return;
    }

    // Erase from the requested set
    for (auto it = mRequested.begin(); it != mRequested.end();) {
        if (it->reference() == request.reference())
            it = mRequested.erase(it);
        else
            it++;
    }

    // Erase from the pending set
    for (auto it = mPending.begin(); it != mPending.end();) {
        if (it->reference() == request.reference())
            it = mPending.erase(it);
        else
            it++;
    }

    // Insert into the mLoaded list.  Note that json has been moved
    auto ptr = Package::create(mSession, request.reference().toString(), std::move(raw));
    if (!ptr) {
        LOGF(LogLevel::ERROR, "Package %s (%s) could not be moved to the loaded list.",
                request.reference().name().c_str(),
                request.reference().version().c_str());
        mState = ERROR;
        return;
    }

    mLoaded.emplace(request.reference(), ptr);
    // Process the import list for this package
    addImportList(*ptr);
    updateStatus();
}

void Content::addData(const std::string& name, JsonData&& raw) {
    if (mState != LOADING)
        return;

    // If the data is invalid, set the error state
    if (!raw) {
        CONSOLE_S(mSession).log("Data '%s' parse error offset=%u: %s",
             name.c_str(), raw.offset(), raw.error());
        mState = ERROR;
        return;
    }

    auto it = std::find(mMainParameters.begin(), mMainParameters.end(), name);
    if (it == mMainParameters.end()) {
        CONSOLE_S(mSession).log("Data parameter '%s' does not exist in the document", name.c_str());
        mState = ERROR;
        return;
    }

    if (mParameterValues.count(name)) {
        CONSOLE_S(mSession).log("Can't reuse data parameters '%s'", name.c_str());
        mState = ERROR;
        return;
    }

    mParameterValues.emplace(name, std::move(raw));
    updateStatus();
}

bool
Content::getMainProperties(Properties& out) const
{
    if (!isReady())
        return false;

    ParameterArray params(mMainTemplate);
    for (const auto& m : params)
        out.emplace(m.name, mParameterValues.at(m.name).get());

    if (DEBUG_CONTENT) {
        LOG(LogLevel::DEBUG) << "Main Properties:";
        for (const auto& m : out)
            LOG(LogLevel::DEBUG) << " " << m.first << ": " << m.second;
    }

    return true;
}

void
Content::addImportList(Package& package)
{
    LOG_IF(DEBUG_CONTENT) << "addImportList " << &package;

    const rapidjson::Value& value = package.json();

    auto it = value.FindMember(DOCUMENT_IMPORT);
    if (it != value.MemberEnd()) {
        if (!it->value.IsArray()) {
            CONSOLE_S(mSession).log("%s: Document import property should be an array", package.name().c_str());
            mState = ERROR;
            return;
        }
        for (const auto& v : it->value.GetArray())
            addImport(package, v);
    }
}

void
Content::addImport(Package& package, const rapidjson::Value& value)
{
    LOG_IF(DEBUG_CONTENT) << "addImport " << &package;

    if (!value.IsObject()) {
        CONSOLE_S(mSession).log("Invalid import record in document");
        mState = ERROR;
        return;
    }

    ImportRequest request(value);
    if (!request.isValid()) {
        CONSOLE_S(mSession).log("Malformed import record");
        mState = ERROR;
        return;
    }

    package.addDependency(request.reference());

    if (mRequested.find(request) == mRequested.end() &&
        mPending.find(request) == mPending.end() &&
        mLoaded.find(request.reference()) == mLoaded.end()) {
        // It is a new request
        mRequested.insert(std::move(request));
    }
}

void
Content::updateStatus()
{
    if (mState == LOADING && mParameterValues.size() == mMainParameters.size() &&
        mRequested.size() == 0 && mPending.size() == 0) {
        mState = READY;
    }
}

Object
Content::getBackground(const Metrics& metrics, const RootConfig& config) const
{
    const auto& json = mMainPackage->json();
    auto backgroundIter = json.FindMember("background");
    if (backgroundIter == json.MemberEnd())
        return Color();  // Transparent

    // If the theme is set in the document it will override the system theme
    std::string theme = metrics.getTheme();
    auto themeIter = json.FindMember("theme");
    if (themeIter != json.MemberEnd() && themeIter->value.IsString())
        theme = themeIter->value.GetString();

    // Create a working context and evaluate any data-binding expression
    // This is a restricted context because we don't load any resources or styles
    auto context = Context::create(metrics, config, theme);
    auto object = evaluate(context, backgroundIter->value);

    // Try to create a gradient
    auto gradient = Gradient::create(context, object);
    if (gradient.isGradient())
        return gradient;

    // Return this as a color
    return object.asColor(mSession);
}

} // namespace apl
