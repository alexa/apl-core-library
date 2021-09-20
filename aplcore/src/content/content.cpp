/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include "apl/content/settings.h"
#include "apl/content/importrequest.h"
#include "apl/content/content.h"
#include "apl/content/jsondata.h"
#include "apl/content/metrics.h"
#include "apl/command/arraycommand.h"
#include "apl/utils/log.h"
#include "apl/utils/session.h"

namespace apl {

static const bool DEBUG_CONTENT = false;

const char* DOCUMENT_IMPORT = "import";
const char* DOCUMENT_MAIN_TEMPLATE = "mainTemplate";
const char* DOCUMENT_ENVIRONMENT = "environment";
const char* DOCUMENT_LANGUAGE = "lang";
const char* DOCUMENT_LAYOUT_DIRECTION = "layoutDirection";

ContentPtr
Content::create(JsonData&& document) {
    return create(std::move(document), makeDefaultSession());
}

ContentPtr
Content::create(JsonData&& document, const SessionPtr& session) {
    if (!document) {
        CONSOLE_S(session).log("Document parse error offset=%u: %s.", document.offset(), document.error());
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

    return std::make_shared<Content>(session, ptr, it->value);
}

Content::Content(SessionPtr session,
                 PackagePtr mainPackagePtr,
                 const rapidjson::Value& mainTemplate)
        : mSession(std::move(session)),
          mMainPackage(std::move(mainPackagePtr)),
          mState(LOADING),
          mMainTemplate(mainTemplate)
{
    addImportList(*mMainPackage);
    addExtensions(*mMainPackage);

    // Extract the array of main template parameters
    mMainParameters = ParameterArray::parameterNames(mainTemplate);

    // Extract the array of environment parameters
    const rapidjson::Value & json = mMainPackage->json();
    auto it = json.FindMember(DOCUMENT_ENVIRONMENT);
    if (it != json.MemberEnd())
        mEnvironmentParameters = ParameterArray::parameterNames(it->value);

    // The ordered list of parameter names starts with "main" and ends with "environmental".
    // Duplicate entries are dropped from the mAllParameters list.
    for (const auto& m : mMainParameters)
        if (mPendingParameters.emplace(m).second)
            mAllParameters.emplace_back(m);

    for (const auto& m : mEnvironmentParameters)
        if (mPendingParameters.emplace(m).second)
            mAllParameters.emplace_back(m);

    updateStatus();
}

PackagePtr
Content::getPackage(const std::string& name) const {
    if (name == Path::MAIN)
        return mMainPackage;

    auto it = std::find_if(mLoaded.begin(), mLoaded.end(), [&name](const std::pair<ImportRef, PackagePtr>& ref) {
        return ref.first.name() == name;
    });

    return it != mLoaded.end() ? it->second : nullptr;
}

std::set<ImportRequest>
Content::getRequestedPackages() {
    mPending.insert(mRequested.begin(), mRequested.end());
    auto result = mRequested;
    mRequested.clear();
    return result;
}

void
Content::addPackage(const ImportRequest& request, JsonData&& raw) {
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

    // We expect packages to be objects, erase from the requested set
    if (!raw.get().IsObject()) {
        CONSOLE_S(mSession).log("Package %s (%s) is not a JSON object",
                                request.reference().name().c_str(),
                                request.reference().version().c_str());
        mState = ERROR;
        return;
    }
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
        LOGF(LogLevel::kError, "Package %s (%s) could not be moved to the loaded list.",
             request.reference().name().c_str(),
             request.reference().version().c_str());
        mState = ERROR;
        return;
    }

    mLoaded.emplace(request.reference(), ptr);
    addExtensions(*ptr);
    // Process the import list for this package
    addImportList(*ptr);
    updateStatus();
}

void Content::addData(const std::string& name, JsonData&& raw) {
    if (mState != LOADING)
        return;

    if (!mPendingParameters.erase(name)) {
        CONSOLE_S(mSession).log("Data parameter '%s' does not exist or is already assigned",
                                name.c_str());
        return;
    }

    // If the data is invalid, set the error state
    if (!raw) {
        CONSOLE_S(mSession).log("Data '%s' parse error offset=%u: %s",
                                name.c_str(), raw.offset(), raw.error());
        mState = ERROR;
        return;
    }

    mParameterValues.emplace(name, std::move(raw));
    updateStatus();
}

bool
Content::getMainProperties(Properties& out) const {
    if (!isReady())
        return false;

    ParameterArray params(mMainTemplate);
    for (const auto& m : params)
        out.emplace(m.name, mParameterValues.at(m.name).get());

    if (DEBUG_CONTENT) {
        LOG(LogLevel::kDebug) << "Main Properties:";
        for (const auto& m : out)
            LOG(LogLevel::kDebug) << " " << m.first << ": " << m.second.toDebugString();
    }

    return true;
}

void
Content::addImportList(Package& package) {
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
Content::addImport(Package& package, const rapidjson::Value& value) {
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
Content::addExtensions(Package& package) {

    const auto features = arrayifyProperty(package.json(), "extension", "extensions");
    for (auto it = features.begin(); it != features.end(); it++) {
        std::string uri;
        std::string name;

        if (it->IsObject()) {  // Check for a "uri" property and a "name" property
            auto iter = it->FindMember("uri");
            if (iter != it->MemberEnd() && iter->value.IsString())
                uri = iter->value.GetString();

            iter = it->FindMember("name");
            if (iter != it->MemberEnd() && iter->value.IsString())
                name = iter->value.GetString();
        }

        // The properties are required
        if (uri.empty() || name.empty()) {
            CONSOLE_S(mSession).log("Illegal extension request in package '%s'", package.name().c_str());
            continue;
        }

        auto eit = std::find_if(mExtensionRequests.begin(), mExtensionRequests.end(),
                [&name](const std::pair<std::string, std::string>& element) {return element.first == name;});
        if (eit != mExtensionRequests.end()) {
            if (eit->second == uri)  // The same NAME->URI mapping is ignored
                continue;

            CONSOLE_S(mSession).log("The extension name='%s' is referencing different URI values", name.c_str());
            mState = ERROR;
            return;
        } else {
            mExtensionRequests.emplace_back(std::pair<std::string, std::string>(name, uri));
        }
    }
}

void
Content::updateStatus() {
    if (mState == LOADING && mPendingParameters.empty() && mRequested.empty() && mPending.empty()) {
        // Content is ready if the dependency list is successfully ordered, otherwise there is an error.
        if (orderDependencyList()) {
            mState = READY;
        } else {
            mState = ERROR;
        }
    }
}

/**
 * Loads Extension settings from imported Packages.  This process uses the ordered dependency list.
 * Should multiple packages provide settings for the same named Extension, or packages reference
 * the same Extension by multiple names, existing settings are overwritten and new settings augmented.
 */
void
Content::loadExtensionSettings() {

    // Settings reader per package
    auto sMap = std::map<std::string, SettingsPtr>();
    // uri -> user assigned settings
    auto tmpMap = std::map<std::string, ObjectMapPtr>();

    // find settings for all requested extensions in packages
    for (auto& extensionRequest : mExtensionRequests) {
        auto name = extensionRequest.first;
        auto uri = extensionRequest.second;

        // create a map to hold the settings for the extension
        auto it = tmpMap.find(uri);
        ObjectMapPtr esMap;
        if (it == tmpMap.end()) {
            esMap = std::make_shared<ObjectMap>();
            tmpMap.emplace(uri, esMap);
        } else {
            esMap = it->second;
        }

        // get the settings for this extension from each package
        for (auto pkg: mOrderedDependencies) {

            auto sItr = sMap.find(pkg->name());
            SettingsPtr settings;

            if (sItr == sMap.end()) {
                // create a Settings for this package
                const rapidjson::Value& settingsValue = Settings::findSettings(*pkg);
                if (settingsValue.IsNull())
                    continue; // no Settings in this package
                settings = std::make_shared<Settings>(Settings(settingsValue));
                sMap.emplace(pkg->name(), settings);
                LOG_IF(DEBUG_CONTENT) << "created settings for pkg: " << pkg->name();
            } else {
                settings = sItr->second;
            }

            // get the named settings for this extension
            auto val = settings->getValue(name);
            if (!val.isMap())
                continue; // no settings for this extension

            // override / augment existing settings
            for (auto v: val.getMap())
                (*esMap)[v.first] = v.second;
            LOG_IF(DEBUG_CONTENT) << "extension:" << name << " pkg:" << pkg << " inserting: " << val;
        }

    }

    // initialize the settings cache
    mExtensionSettings = std::make_shared<ObjectMap>();
    // store settings Object by extension uri
    for (auto tm : tmpMap) {
        auto obj = (!tm.second->empty()) ? Object(tm.second) : Object::NULL_OBJECT();
        mExtensionSettings->emplace(tm.first, obj);
        LOG_IF(DEBUG_CONTENT) << "extension result: " << obj.toDebugString();
    }
}


Object
Content::getBackground(const Metrics& metrics, const RootConfig& config) const {
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
    auto context = Context::createBackgroundEvaluationContext(metrics, config, theme);
    auto object = evaluate(*context, backgroundIter->value);
    return asFill(*context, object);
}

/**
 * "lang": "VALUE",             // Backward Compatibility
 * "layoutDirection": "VALUE",  // Backward Compatibility
 * "environment": {
 *   "parameters": [ "payload" ],
 *   "lang": "en-US",
 *   "layoutDirection": "${payload.foo}"
 *  }
 */
Content::Environment
Content::getEnvironment(const RootConfig& config) const
{
    // Use the RootConfig to initialize the default values
    auto language = config.getProperty(RootProperty::kLang).asString();
    auto layoutDirection = static_cast<LayoutDirection>(config.getProperty(kLayoutDirection).asInt());

    // For backward compatibility with the older 1.7 specification, allow the user to specify
    // "lang" at the document level
    const auto& json = mMainPackage->json();
    auto langIter = json.FindMember(DOCUMENT_LANGUAGE);
    if (langIter != json.MemberEnd() && langIter->value.IsString())
        language = langIter->value.GetString();

    // For backward compatibility with the older 1.7 specification, allow the user to specify
    // "layoutDirection" at the document level
    auto ldIter = json.FindMember(DOCUMENT_LAYOUT_DIRECTION);
    if (ldIter != json.MemberEnd() && ldIter->value.IsString()) {
        auto s = ldIter->value.GetString();
        auto ld = static_cast<LayoutDirection>(sLayoutDirectionMap.get(s, -1));
        if (ld == static_cast<LayoutDirection>(-1)) {
            CONSOLE_S(mSession)
                << "Document 'layoutDirection' property is invalid.  Falling back to system defaults";
        }
        else if (ld != kLayoutDirectionInherit) {
            // Only overwrite the layout direction if it is LTR or RTL.  If it is "inherit", use the RootConfig value
            layoutDirection = ld;
        }
    }

    // If the document has defined a "environment" section, we parse that
    auto envIter = json.FindMember(DOCUMENT_ENVIRONMENT);
    if (envIter != json.MemberEnd() && envIter->value.IsObject()) {
        auto context = Context::createTypeEvaluationContext(config);
        for (const auto& m : mEnvironmentParameters)
            context->putUserWriteable(m, mParameterValues.at(m).get());

        // Update the language, if it is defined
        language = propertyAsString(*context, envIter->value, DOCUMENT_LANGUAGE, language);

        // Extract the layout direction as a string.
        auto ld = propertyAsString(*context, envIter->value, DOCUMENT_LAYOUT_DIRECTION);
        if (!ld.empty()) {
            auto value = sLayoutDirectionMap.get(ld, -1);
            if (value != -1 && value != LayoutDirection::kLayoutDirectionInherit)
                layoutDirection = static_cast<LayoutDirection>(value);
        }
    }

    return {language, layoutDirection};
}

const SettingsPtr
Content::getDocumentSettings() const {
    const rapidjson::Value& settingsValue = Settings::findSettings(*mMainPackage);
    auto settings = std::make_shared<Settings>(Settings(settingsValue));
    return settings;
}

std::set<std::string>
Content::getExtensionRequests() const {
    std::set<std::string> result;
    for (const auto& m : mExtensionRequests)
        result.emplace(m.second);
    return result;
}

Object
Content::getExtensionSettings(const std::string& uri) {
    if (!isReady()) {
        CONSOLE_S(mSession).log("Settings for extension name='%s' cannot be returned.  The document is not Ready.",
                                uri.c_str());
        return Object::NULL_OBJECT();
    }

    if (!mExtensionSettings) {
        loadExtensionSettings();
    }

    const std::map<std::string, Object>::const_iterator& es = mExtensionSettings->find(uri);
    if (es != mExtensionSettings->end()) {
        LOG_IF(DEBUG_CONTENT) << "getExtensionSettings " << uri << ":" << es->second.toDebugString()
                              << " mapaddr:" << &es->second;
        return es->second;
    }
    return Object::NULL_OBJECT();
}


/**
 * Create a deterministic order for all packages.
 */
bool
Content::orderDependencyList() {
    std::set<PackagePtr> inProgress;
    bool isOrdered = addToDependencyList(mOrderedDependencies, inProgress, mMainPackage);
    if (!isOrdered)
        CONSOLE_S(mSession).log("Failure to order packages");
    return isOrdered;
}


/**
 * Traverse the dependencies of a package and create a deterministic order.
 */
bool
Content::addToDependencyList(std::vector<PackagePtr>& ordered,
                             std::set<PackagePtr>& inProgress,
                             const PackagePtr& package) {
    LOG_IF(DEBUG_CONTENT) << "addToDependencyList " << package << " dependency count="
                          << package->getDependencies().size();

    inProgress.insert(package);  // For dependency loop detection

    // Start with the package dependencies
    for (const auto& ref : package->getDependencies()) {
        LOG_IF(DEBUG_CONTENT) << "checking child " << ref.toString();

        // Convert the reference into a loaded PackagePtr
        const auto& pkg = mLoaded.find(ref);
        if (pkg == mLoaded.end()) {
            LOGF(LogLevel::kError, "Missing package '%s' in the loaded set", ref.name().c_str());
            return false;
        }

        const PackagePtr& child = pkg->second;

        // Check if it is already in the dependency list (someone else included it first)
        auto it = std::find(ordered.begin(), ordered.end(), child);
        if (it != ordered.end()) {
            LOG_IF(DEBUG_CONTENT) << "child package " << ref.toString() << " already in dependency list";
            continue;
        }

        // Check for a circular dependency
        if (inProgress.count(child)) {
            CONSOLE_S(mSession).log("Circular package dependency '%s'", ref.name().c_str());
            return false;
        }

        if (!addToDependencyList(ordered, inProgress, child)) {
            LOG_IF(DEBUG_CONTENT) << "returning false with child package " << child->name();
            return false;
        }
    }

    LOG_IF(DEBUG_CONTENT) << "Pushing package " << package << " onto ordered list";
    ordered.push_back(package);
    inProgress.erase(package);
    return true;
}


} // namespace apl
