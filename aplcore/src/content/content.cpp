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

#include "apl/content/content.h"

#include <algorithm>

#include "apl/buildTimeConstants.h"
#include "apl/command/arraycommand.h"
#include "apl/component/hostcomponent.h"
#include "apl/content/extensionrequest.h"
#include "apl/content/importrequest.h"
#include "apl/content/jsondata.h"
#include "apl/content/metrics.h"
#include "apl/content/package.h"
#include "apl/content/settings.h"
#include "apl/embed/embedrequest.h"
#include "apl/engine/arrayify.h"
#include "apl/engine/parameterarray.h"
#include "apl/engine/propdef.h"
#include "apl/utils/identifier.h"
#include "apl/utils/log.h"
#include "apl/utils/session.h"

namespace apl {

static const bool DEBUG_CONTENT = false;

const char* DOCUMENT_IMPORT = "import";
const char* DOCUMENT_MAIN_TEMPLATE = "mainTemplate";
const char* DOCUMENT_ENVIRONMENT = "environment";
const char* DOCUMENT_LANGUAGE = "lang";
const char* DOCUMENT_LAYOUT_DIRECTION = "layoutDirection";
const char* PACKAGE_TYPE = "type";
const char* PACKAGE_TYPE_PACKAGE = "package";
const char* PACKAGE_TYPE_ONEOF = "oneOf";
const char* PACKAGE_TYPE_ALLOF = "allOf";
const char* PACKAGE_OTHERWISE = "otherwise";
const char* PACKAGE_ITEMS = "items";

ContentPtr
Content::create(JsonData&& document)
{
    return create(std::move(document), makeDefaultSession());
}

ContentPtr
Content::create(JsonData&& document, const SessionPtr& session)
{
    return create(std::move(document), session, Metrics(), RootConfig(), false);
}


ContentPtr
Content::create(JsonData&& document, const SessionPtr& session,
                const Metrics& metrics, const RootConfig& config)
{
    return create(std::move(document), session, metrics, config, true);
}

ContentPtr
Content::create(JsonData&& document, const SessionPtr& session, const Metrics& metrics,
                const RootConfig& config, bool supportsEvaluation)
{
    if (!document) {
        CONSOLE(session).log("Document parse error offset=%u: %s.", document.offset(), document.error());
        return nullptr;
    }

    auto ptr = Package::create(session, Path::MAIN, std::move(document));
    if (!ptr)
        return nullptr;

    const rapidjson::Value& json = ptr->json();
    auto it = json.FindMember(DOCUMENT_MAIN_TEMPLATE);
    if (it == json.MemberEnd()) {
        CONSOLE(session) << "Document does not contain a mainTemplate property";
        return nullptr;
    }

    auto result = std::make_shared<Content>(session, ptr, it->value, metrics, config);
    result->init(supportsEvaluation);
    return result;
}

Content::Content(const SessionPtr& session,
                 const PackagePtr& mainPackagePtr,
                 const rapidjson::Value& mainTemplate,
                 const Metrics& metrics,
                 const RootConfig& rootConfig)
        : mSession(session),
          mMainPackage(mainPackagePtr),
          mState(LOADING),
          mMainTemplate(mainTemplate),
          mMetrics(metrics),
          mConfig(rootConfig)
{}

void
Content::refresh(const Metrics& metrics, const RootConfig& config)
{
    LOG_IF(DEBUG_CONTENT).session(mSession) << "Refreshing evaluation context.";

    // We refresh imports and settings only, for now.
    mMetrics = metrics;
    mConfig = config;
    mEvaluationContext = Context::createContentEvaluationContext(
        mMetrics,
        mConfig,
        mMainPackage->version(),
        extractTheme(mMetrics),
        getSession());

    mExtensionRequests.clear();
    mMainPackage->mDependencies.clear();

    for (const auto& pkg : mLoaded) {
        pkg.second->mDependencies.clear();
        mStashed.emplace(pkg);
    }

    mLoaded.clear();
    mPending.clear();
    mRequested.clear();
    mOrderedDependencies.clear();

    mState = LOADING;

    addImportList(*mMainPackage);
    addExtensions(*mMainPackage);

    updateStatus();
}

void
Content::refresh(const EmbedRequest& request, const DocumentConfigPtr& documentConfig) {
    auto parentHost = HostComponent::cast(request.mOriginComponent.lock());
    if (!parentHost) return;

    parentHost->refreshContent(shared_from_this(), documentConfig);
}

void
Content::init(bool supportsEvaluation)
{
    if (supportsEvaluation)
        mEvaluationContext = Context::createContentEvaluationContext(
            mMetrics, mConfig, mMainPackage->version(), extractTheme(mMetrics), getSession());

    // First chance where we can extract settings. Set up the session.
    auto diagnosticLabel = getDocumentSettings()->getValue("-diagnosticLabel").asString();
    mSession->setLogIdPrefix(diagnosticLabel);
    LOG(LogLevel::kInfo).session(mSession) << "Initializing experience using " << std::string(sCoreRepositoryVersion);

    addImportList(*mMainPackage);
    addExtensions(*mMainPackage);

    // Extract the array of main template parameters
    mMainParameters = ParameterArray::parameterNames(mMainTemplate);

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
Content::getPackage(const std::string& name) const
{
    if (name == Path::MAIN)
        return mMainPackage;

    auto it = std::find_if(mLoaded.begin(), mLoaded.end(), [&name](const std::pair<ImportRef, PackagePtr>& ref) {
        return ref.first.toString() == name;
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
Content::loadPackage(const ImportRef& ref, const PackagePtr& package)
{
    LOG_IF(DEBUG_CONTENT).session(mSession) << "Adding package: " << &package;
    mLoaded.emplace(ref, package);
    addExtensions(*package);
    addImportList(*package);
}

void
Content::addPackage(const ImportRequest& request, JsonData&& raw)
{
    if (mState != LOADING)
        return;

    // If the package data is invalid, set the error state
    if (!raw) {
        CONSOLE(mSession).log("Package %s (%s) parse error offset=%u: %s",
                                request.reference().name().c_str(),
                                request.reference().version().c_str(),
                                raw.offset(), raw.error());
        mState = ERROR;
        return;
    }

    // We expect packages to be objects, erase from the requested set
    if (!raw.get().IsObject()) {
        CONSOLE(mSession).log("Package %s (%s) is not a JSON object",
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
        LOG(LogLevel::kError).session(mSession) << "Package " << request.reference().name()
            << " (" << request.reference().version() << ") could not be moved to the loaded list.";
        mState = ERROR;
        return;
    }

    loadPackage(request.reference(), ptr);
    updateStatus();
}

void Content::addData(const std::string& name, JsonData&& raw)
{
    if (!allowAdd(name))
        return;

    // If the data is invalid, set the error state
    if (!raw) {
        CONSOLE(mSession).log("Data '%s' parse error offset=%u: %s",
                                name.c_str(), raw.offset(), raw.error());
        mState = ERROR;
        return;
    }

    mParameterValues.emplace(name, raw.moveToObject());
    updateStatus();
}

void
Content::addObjectData(const std::string& name, const Object& data)
{
    if (!allowAdd(name))
        return;

    mParameterValues.emplace(name, data);
    updateStatus();
}

bool
Content::allowAdd(const std::string& name)
{
    if (mState != LOADING)
        return false;

    if (!mPendingParameters.erase(name)) {
        CONSOLE(mSession).log("Data parameter '%s' does not exist or is already assigned",
                              name.c_str());
        return false;
    }

    return true;
}

std::vector<std::string>
Content::getLoadedPackageNames() const
{
    std::vector<std::string> result;
    for (const auto& m : mLoaded)
        result.push_back(m.second->name());
    return result;
}

bool
Content::getMainProperties(Properties& out) const
{
    if (!isReady())
        return false;

    ParameterArray params(mMainTemplate);
    for (const auto& m : params)
        out.emplace(m.name, mParameterValues.at(m.name));

    if (DEBUG_CONTENT) {
        LOG(LogLevel::kDebug).session(mSession) << "Main Properties:";
        for (const auto& m : out)
            LOG(LogLevel::kDebug).session(mSession) << " " << m.first << ": " << m.second.toDebugString();
    }

    return true;
}

void
Content::addImportList(Package& package)
{
    LOG_IF(DEBUG_CONTENT).session(mSession) << "addImportList " << &package;

    const rapidjson::Value& value = package.json();

    auto it = value.FindMember(DOCUMENT_IMPORT);
    if (it != value.MemberEnd()) {
        if (!it->value.IsArray()) {
            CONSOLE(mSession).log("%s: Document import property should be an array", package.name().c_str());
            mState = ERROR;
            return;
        }
        for (const auto& v : it->value.GetArray())
            addImport(package, v);
    }
}

bool
Content::addImport(Package& package,
                   const rapidjson::Value& value,
                   const std::string& name,
                   const std::string& version,
                   const std::set<std::string>& loadAfter)
{
    LOG_IF(DEBUG_CONTENT).session(mSession) << "addImport " << &package;

    if (mState == ERROR) return false;

    if (!value.IsObject()) {
        CONSOLE(mSession).log("Invalid import record in document");
        mState = ERROR;
        return false;
    }

    // Check for conditionality, only if context available.
    if (mEvaluationContext) {
        auto it_when = value.FindMember("when");
        if (it_when != value.MemberEnd()) {
            auto evaluatedWhen = evaluate(*mEvaluationContext, it_when->value.GetString());
            if (!evaluatedWhen.asBoolean()) return false;
        }
    }

    auto typeIt = value.FindMember(PACKAGE_TYPE);
    std::string type = PACKAGE_TYPE_PACKAGE;
    if (typeIt != value.MemberEnd() && typeIt->value.IsString()) {
        type = typeIt->value.GetString();
    }

    if (type == PACKAGE_TYPE_ONEOF) {
        auto sIt = value.FindMember(PACKAGE_ITEMS);
        if (sIt != value.MemberEnd() && sIt->value.IsArray()) {
            // Expansion. Can use common name/version/loadAfter.
            auto commonNameAndVersion = ImportRequest::extractNameAndVersion(value, mEvaluationContext);
            auto commonLoadAfter = ImportRequest::extractLoadAfter(value, mEvaluationContext);
            for (const auto& s : sIt->value.GetArray()) {
                if (addImport(package, s,
                              commonNameAndVersion.first.empty() ? name : commonNameAndVersion.first,
                              commonNameAndVersion.second.empty() ? version : commonNameAndVersion.second,
                              commonLoadAfter.empty() ? loadAfter : commonLoadAfter))
                    return true;
            }
        } else {
            CONSOLE(mSession).log("%s: Missing items field for the oneOf import", package.name().c_str());
            mState = ERROR;
            return false;
        }

        // If no imports were matched - use otherwise
        auto otherwiseIt = value.FindMember(PACKAGE_OTHERWISE);
        if (otherwiseIt != value.MemberEnd() && otherwiseIt->value.IsArray()) {
            // Expansion. Can use common name/version/loadAfter.
            auto commonNameAndVersion = ImportRequest::extractNameAndVersion(value, mEvaluationContext);
            auto commonLoadAfter = ImportRequest::extractLoadAfter(value, mEvaluationContext);
            for (const auto& s : otherwiseIt->value.GetArray()) {
                if (!addImport(package, s, commonNameAndVersion.first, commonNameAndVersion.second, commonLoadAfter)) {
                    CONSOLE(mSession).log("%s: Otherwise imports failed", package.name().c_str());
                    mState = ERROR;
                    return false;
                }
            }
        }

        // Nothing was done, which is kinda fine
        return true;
    } else if (type == PACKAGE_TYPE_ALLOF) {
        auto sIt = value.FindMember(PACKAGE_ITEMS);
        if (sIt != value.MemberEnd() && sIt->value.IsArray()) {
            auto commonNameAndVersion = ImportRequest::extractNameAndVersion(value, mEvaluationContext);
            auto commonLoadAfter = ImportRequest::extractLoadAfter(value, mEvaluationContext);
            for (const auto& s : sIt->value.GetArray()) {
                addImport(package, s,
                          commonNameAndVersion.first.empty() ? name : commonNameAndVersion.first,
                          commonNameAndVersion.second.empty() ? version : commonNameAndVersion.second,
                          commonLoadAfter.empty() ? loadAfter : commonLoadAfter);
            }
        } else {
            CONSOLE(mSession).log("%s: Missing items field for the allOf import", package.name().c_str());
            mState = ERROR;
            return false;
        }

        return true;
    }

    ImportRequest request = ImportRequest::create(value, mEvaluationContext, name, version, loadAfter);
    if (!request.isValid()) {
        CONSOLE(mSession).log("Malformed package import record");
        mState = ERROR;
        return false;
    }

    package.addDependency(request.reference());

    if (mRequested.find(request) == mRequested.end() &&
        mPending.find(request) == mPending.end() &&
        mLoaded.find(request.reference()) == mLoaded.end()) {

        // Reuse if was already resolved
        auto stashed = mStashed.find(request.reference());
        if (stashed != mStashed.end()) {
            loadPackage(request.reference(), stashed->second);
            return true;
        }

        // It is a new request
        mRequested.insert(std::move(request));
    }

    return true;
}

void
Content::addExtensions(Package& package)
{
    const auto features = arrayifyProperty(package.json(), "extension", "extensions");
    for (const auto& feature : features) {
        std::string uri;
        std::string name;
        bool required = false;

        if (feature.IsObject()) {
            auto iter = feature.FindMember("uri");
            if (iter != feature.MemberEnd() && iter->value.IsString())
                uri = iter->value.GetString();

            iter = feature.FindMember("name");
            if (iter != feature.MemberEnd() && iter->value.IsString())
                name = iter->value.GetString();

            iter = feature.FindMember("required");
            if (iter != feature.MemberEnd() && iter->value.IsBool())
                required = iter->value.GetBool();
        }

        // The properties are required
        if (uri.empty() || name.empty()) {
            CONSOLE(mSession).log("Illegal extension request in package '%s'", package.name().c_str());
            continue;
        }

        auto eit = std::find_if(mExtensionRequests.begin(), mExtensionRequests.end(),
                [&name](const ExtensionRequest& request) {return request.name == name;});
        if (eit != mExtensionRequests.end()) {
            if (eit->uri == uri) { // The same NAME->URI mapping is ignored unless required
                eit->required |= required;
                continue;
            }

            CONSOLE(mSession).log("The extension name='%s' is referencing different URI values", name.c_str());
            mState = ERROR;
            return;
        } else {
            mExtensionRequests.emplace_back(ExtensionRequest{name, uri, required});
        }
    }
}

void
Content::updateStatus()
{
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
Content::loadExtensionSettings()
{
    // Settings reader per package
    auto sMap = std::map<std::string, SettingsPtr>();
    // uri -> user assigned settings
    auto tmpMap = std::map<std::string, ObjectMapPtr>();

    // find settings for all requested extensions in packages
    for (auto& extensionRequest : mExtensionRequests) {
        auto name = extensionRequest.name;
        auto uri = extensionRequest.uri;

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
                LOG_IF(DEBUG_CONTENT).session(mSession) << "created settings for pkg: " << pkg->name();
            } else {
                settings = sItr->second;
            }

            // get the named settings for this extension
            auto val = settings->getValue(name);
            if (!val.isMap())
                continue; // no settings for this extension

            // override / augment existing settings
            for (const auto& v: val.getMap())
                (*esMap)[v.first] = v.second;
            LOG_IF(DEBUG_CONTENT).session(mSession) << "extension:" << name
                                                    << " pkg:" << pkg
                                                    << " inserting: " << val;
        }

    }

    // initialize the settings cache
    mExtensionSettings = std::make_shared<ObjectMap>();
    // store settings Object by extension uri
    for (const auto& tm : tmpMap) {
        auto obj = (!tm.second->empty()) ? Object(tm.second) : Object::NULL_OBJECT();
        mExtensionSettings->emplace(tm.first, obj);
        LOG_IF(DEBUG_CONTENT).session(mSession) << "extension result: " << obj.toDebugString();
    }
}

std::string
Content::extractTheme(const Metrics& metrics) const
{
    // If the theme is set in the document it will override the system theme
    const auto& json = mMainPackage->json();
    std::string theme = metrics.getTheme();
    auto themeIter = json.FindMember("theme");
    if (themeIter != json.MemberEnd() && themeIter->value.IsString())
        theme = themeIter->value.GetString();
    return theme;
}

Object
Content::extractBackground(const Context& evaluationContext) const
{
    const auto& json = mMainPackage->json();
    auto backgroundIter = json.FindMember("background");
    if (backgroundIter == json.MemberEnd())
        return Color();  // Transparent

    auto object = evaluate(evaluationContext, backgroundIter->value);
    return asFill(evaluationContext, object);
}

Object
Content::getBackground(const Metrics& metrics, const RootConfig& config) const
{
    // Create a working context and evaluate any data-binding expression
    // This is a restricted context because we don't load any resources or styles
    auto context = Context::createContentEvaluationContext(
        metrics,
        config,
        mMainPackage->version(),
        extractTheme(metrics),
        getSession());

    return extractBackground(*context);
}

Object
Content::getBackground() const
{
    if (mEvaluationContext) {
        return extractBackground(*mEvaluationContext);
    } else {
        LOG(LogLevel::kError).session(mSession)
            << "Using extractBackground() with deprecated Content constructors. No evaluation context available.";
        return Color();
    }
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
            CONSOLE(mSession)
                << "Document 'layoutDirection' property is invalid.  Falling back to system defaults";
        }
        else if (ld != kLayoutDirectionInherit) {
            // Only overwrite the layout direction if it is LTR or RTL.  If it is "inherit", use the RootConfig value
            layoutDirection = ld;
        }
    }

    // If the document has defined an "environment" section, we parse that
    auto envIter = json.FindMember(DOCUMENT_ENVIRONMENT);
    if (envIter != json.MemberEnd() && envIter->value.IsObject()) {
        auto context = Context::createTypeEvaluationContext(config, mMainPackage->version(), getSession());
        for (const auto& m : mEnvironmentParameters) {
            if (!isValidIdentifier(m))
                CONSOLE(context) << "Unable to add environment parameter '" << m
                                 << "' to context. Invalid identifier.";
            else
                context->putUserWriteable(m, mParameterValues.at(m));
        }

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
Content::getDocumentSettings() const
{
    const rapidjson::Value& settingsValue = Settings::findSettings(*mMainPackage);
    auto settings = std::make_shared<Settings>(Settings(settingsValue));
    return settings;
}

std::set<std::string>
Content::getExtensionRequests() const
{
    std::set<std::string> result;
    for (const auto& m : mExtensionRequests)
        result.emplace(m.uri);
    return result;
}

const std::vector<ExtensionRequest>&
Content::getExtensionRequestsV2() const
{
    return mExtensionRequests;
}

Object
Content::getExtensionSettings(const std::string& uri)
{
    if (!isReady()) {
        CONSOLE(mSession).log("Settings for extension name='%s' cannot be returned.  The document is not Ready.",
                                uri.c_str());
        return Object::NULL_OBJECT();
    }

    if (!mExtensionSettings) {
        loadExtensionSettings();
    }

    const std::map<std::string, Object>::const_iterator& es = mExtensionSettings->find(uri);
    if (es != mExtensionSettings->end()) {
        LOG_IF(DEBUG_CONTENT).session(mSession) << "getExtensionSettings " << uri
                                                << ":" << es->second.toDebugString()
                                                << " mapaddr:" << &es->second;
        if (mEvaluationContext)
            return evaluateNested(*mEvaluationContext, es->second);
        else
            return es->second;
    }
    return Object::NULL_OBJECT();
}


/**
 * Create a deterministic order for all packages.
 */
bool
Content::orderDependencyList()
{
    std::set<PackagePtr> inProgress;
    bool isOrdered = addToDependencyList(mOrderedDependencies, inProgress, mMainPackage);
    if (!isOrdered)
        CONSOLE(mSession).log("Failure to order packages");
    mStashed.clear();
    return isOrdered;
}


/**
 * Traverse the dependencies of a package and create a deterministic order.
 */
bool
Content::addToDependencyList(std::vector<PackagePtr>& ordered,
                             std::set<PackagePtr>& inProgress,
                             const PackagePtr& package)
{
    LOG_IF(DEBUG_CONTENT).session(mSession) << "addToDependencyList " << package
                                            << " dependency count=" << package->getDependencies().size();

    inProgress.insert(package);  // For dependency loop detection

    auto pds = package->getDependencies();
    auto depQueue = std::queue<ImportRef>();
    for (const auto& pd : pds) depQueue.emplace(pd);

    std::set<std::string> available;
    std::set<std::pair<std::string, std::string>> delayed;
    size_t circularCounter = 0;
    while (!depQueue.empty()) {
        auto ref = depQueue.front();
        depQueue.pop();

        auto needDeps = false;

        // Check to see if package has load dependency and it was already included
        for (const auto& dep : ref.loadAfter()) {
            if (!ref.loadAfter().empty() && !available.count(dep)) {
                // Check if we have anything else to load
                if (depQueue.empty()) {
                    CONSOLE(mSession).log("Required loadAfter package not available %s for %s",
                                          dep.c_str(), ref.name().c_str());
                    return false;
                }

                // Check if we have reverse dep
                if (delayed.count(std::make_pair(dep, ref.name()))) {
                    CONSOLE(mSession).log("Circular package loadAfter dependency between %s and %s",
                                          ref.name().c_str(), dep.c_str());
                    return false;
                }

                delayed.emplace(ref.name(), dep);
                depQueue.emplace(ref);
                needDeps = true;
            }
        }

        if (circularCounter > depQueue.size()) {
            CONSOLE(mSession).log("Circular package loadAfter dependency chain");
            return false;
        }

        circularCounter++;

        if (needDeps) continue;

        // Reset counter
        circularCounter = 0;

        LOG_IF(DEBUG_CONTENT).session(mSession) << "checking child " << ref.toString();

        // Convert the reference into a loaded PackagePtr
        const auto& pkg = mLoaded.find(ref);
        if (pkg == mLoaded.end()) {
            LOG(LogLevel::kError).session(mSession) << "Missing package '" << ref.name()
                                                    << "' in the loaded set";
            return false;
        }

        const PackagePtr& child = pkg->second;

        // Check if it is already in the dependency list (someone else included it first)
        auto it = std::find(ordered.begin(), ordered.end(), child);
        if (it != ordered.end()) {
            LOG_IF(DEBUG_CONTENT).session(mSession) << "child package " << ref.toString()
                                                    << " already in dependency list";
            continue;
        }

        // Check for a circular dependency
        if (inProgress.count(child)) {
            CONSOLE(mSession).log("Circular package dependency '%s'", ref.name().c_str());
            return false;
        }

        if (!addToDependencyList(ordered, inProgress, child)) {
            LOG_IF(DEBUG_CONTENT).session(mSession) << "returning false with child package " << child->name();
            return false;
        }
        available.emplace(ref.name());
    }

    LOG_IF(DEBUG_CONTENT).session(mSession) << "Pushing package " << package
                                            << " onto ordered list";
    ordered.push_back(package);
    inProgress.erase(package);
    return true;
}


} // namespace apl
