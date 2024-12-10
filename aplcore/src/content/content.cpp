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
#include "apl/content/packageresolver.h"
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

const char* DOCUMENT_MAIN_TEMPLATE = "mainTemplate";
const char* DOCUMENT_ENVIRONMENT = "environment";
const char* DOCUMENT_LANGUAGE = "lang";
const char* DOCUMENT_LAYOUT_DIRECTION = "layoutDirection";

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
Content::load(SuccessCallback&& onSuccess, FailureCallback&& onFailure)
{
    if (mState == ERROR) {
        onFailure();
        return;
    }

    // If we're already loaded, then invoke the success callback right away.
    if (!isWaiting()) {
        onSuccess();
        return;
    }

    auto weakSelf = std::weak_ptr<Content>(shared_from_this());
    mPackageResolver->load(
        mCurrentPendingImports,
        [weakSelf, onSuccess](std::vector<PackagePtr>&& ordered) {
            auto self = weakSelf.lock();
            if (!self) return;

            self->mOrderedDependencies = std::move(ordered);
            self->updateStatus();
            onSuccess();
        },
        [weakSelf, onFailure](const ImportRef& request, const std::string& errorMessage, int errorCode) {
            auto self = weakSelf.lock();
            if (!self) return;

            self->mState = ERROR;
            CONSOLE(self->mSession) << "Content could not load requested packages.";
            onFailure();
        },
        [weakSelf](const Package& package) {
            auto self = weakSelf.lock();
            if (!self) return;

            self->addExtensions(package);
        });
}

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
    mState = LOADING;

    // Update the package manager in our package resolver. It may be new.
    mPackageResolver->setPackageManager(mConfig.getPackageManager() == nullptr ? mContentPackageManager : mConfig.getPackageManager());

    addExtensions(*mMainPackage);
    preloadPackages();
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

    if (mConfig.getPackageManager() == nullptr) {
        mContentPackageManager = std::make_shared<ContentPackageManager>();
        mPackageResolver = PackageResolver::create(mContentPackageManager, mSession);
    } else {
        mPackageResolver = PackageResolver::create(mConfig.getPackageManager(), mSession);
    }
    // First chance where we can extract settings. Set up the session.
    auto diagnosticLabel = getDocumentSettings()->getValue("-diagnosticLabel").asString();
    mSession->setLogIdPrefix(diagnosticLabel);
    LOG(LogLevel::kInfo).session(mSession) << "Initializing experience using " << std::string(sCoreRepositoryVersion);

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

    addExtensions(*mMainPackage);
    preloadPackages();
}

void Content::preloadPackages() {
    if (mState == ERROR) {
        return;
    }

    if (mContentPackageManager != nullptr) {
        mContentPackageManager->mRequested.clear();
    }

    mCurrentPendingImports = nullptr;

    auto pendingImports = std::make_shared<PendingImportPackage>(mEvaluationContext, mSession, mMainPackage, mOrderedDependencies); 
    
    if (pendingImports->isReady()) {
        mOrderedDependencies = std::move(pendingImports->moveOrderedDependencies());
        updateStatus();
    } else if (pendingImports->isError()) {
        mState = ERROR;
        CONSOLE(mSession) << "Content could not load requested packages.";
    } else {
        mCurrentPendingImports = pendingImports;
        // Force a load if we're using mContentPackageManager to ensure 
        // backwards compatible behaviour.
        if (mContentPackageManager != nullptr) {
            load([] {}, [] {}); 
        }
    }
}

PackagePtr
Content::getPackage(const std::string& name) const
{
    if (name == Path::MAIN)
        return mMainPackage;

    auto it = std::find_if(mOrderedDependencies.begin(), mOrderedDependencies.end(), [&name](const PackagePtr& ref) {
        return ref->name() == name;
    });

    return it != mOrderedDependencies.end() ? *it : nullptr;
}

std::set<ImportRequest>
Content::getRequestedPackages()
{
    // This should now be done via a injected package manager, but we need to 
    // enable backwards compatible behaviour if a package manager is not 
    // supplied by the runtime. 
    if (mContentPackageManager != nullptr) {
        auto requested = mContentPackageManager->mRequested;
        mContentPackageManager->mRequested.clear();
        return requested;
    } else {
        return {};
    }
}

bool
Content::isWaiting() const
{
    return !isError() && mCurrentPendingImports && !mCurrentPendingImports->isReady();
}

void
Content::addPackage(const ImportRequest& request, JsonData&& raw)
{
    // This should now be done via a injected package manager, but we need to 
    // enable backwards compatible behaviour if a package manager is not 
    // supplied by the runtime. 
    if (mContentPackageManager != nullptr) {
        mPackageResolver->onPackageLoaded(request, std::move(raw));
    }
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
    for (const auto& m : mOrderedDependencies) {
        if (m->name() != Path::MAIN) {
            result.push_back(m->name());
        }
    }

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
Content::addExtensions(const Package& package)
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
    if (mPendingParameters.empty() && !isWaiting()) {
        mState = READY;
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

bool
Content::reactiveConditionalInflation() const
{
    return getDocumentSettings()->reactiveConditionalInflation(mConfig);
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
    if (isError()) {
        CONSOLE(mSession).log("Settings for extension name='%s' cannot be returned.  The document has an error.",
                                uri.c_str());
        return Object::NULL_OBJECT();
    } else if (isWaiting()) {
        CONSOLE(mSession).log("Settings for extension name='%s' cannot be returned.  The document is waiting for packages.",
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

} // namespace apl
