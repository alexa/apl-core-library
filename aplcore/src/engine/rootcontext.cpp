/**
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <rapidjson/stringbuffer.h>
#include <apl/command/documentcommand.h>


#include "apl/engine/builder.h"
#include "apl/engine/styles.h"
#include "apl/action/scrolltoaction.h"
#include "apl/content/content.h"
#include "apl/utils/log.h"
#include "apl/content/metrics.h"
#include "apl/engine/rootcontext.h"
#include "apl/engine/resources.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/rootcontextdata.h"
#include "apl/time/timemanager.h"
#include "apl/graphic/graphic.h"

namespace apl {

const char *ELAPSED_TIME = "elapsedTime";
const char *LOCAL_TIME = "localTime";
const char *UTC_TIME = "utcTime";

static const bool DEBUG_ROOT_CONTEXT = false;

RootContextPtr
RootContext::create(const Metrics& metrics, const ContentPtr& content)
{
    return create(metrics, content, RootConfig(), nullptr);
}

RootContextPtr
RootContext::create(const Metrics& metrics, const ContentPtr& content, const RootConfig& config)
{
    return create(metrics, content, config, nullptr);
}

RootContextPtr
RootContext::create(const Metrics& metrics, const ContentPtr& content,
                    const RootConfig& config, std::function<void(const RootContextPtr&)> callback)
{
    if (!content->isReady()) {
        LOG(LogLevel::ERROR) << "Attempting to create root context with illegal content";
        return nullptr;
    }

    auto root = std::make_shared<RootContext>(metrics, content, config);
    if (callback)
        callback(root);
    if (!root->setup())
        return nullptr;

    return root;
}

RootContext::RootContext(const Metrics& metrics, const ContentPtr& content, const RootConfig& config)
    : mContent(content),
      mTimeManager(config.getTimeManager())
{
    std::string theme = metrics.getTheme();
    const auto& json = content->getDocument()->json();
    auto themeIter = json.FindMember("theme");
    if (themeIter != json.MemberEnd() && themeIter->value.IsString())
        theme = themeIter->value.GetString();

    auto session = config.getSession();
    if (!session)
        session = content->getSession();

    mCore = std::make_shared<RootContextData>(metrics, config, theme, content->getDocument()->version(), session);
    mContext = Context::create(metrics, mCore);

    mContext->putSystemWriteable(ELAPSED_TIME, mTimeManager->currentTime());

    mLocalTime = config.getLocalTime();
    mContext->putSystemWriteable(LOCAL_TIME, mLocalTime);
    mContext->putSystemWriteable(UTC_TIME, mLocalTime - config.getLocalTimeAdjustment());
}

RootContext::~RootContext()
{
    assert(mCore);
    mCore->terminate();
}

void
RootContext::clearPending() const
{
    assert(mCore);
    // Make sure any pending events have executed
    mTimeManager->runPending();

    // TODO: Reaching directly in to mTop is ugly.  See if there is a better approach
    // If we need a layout pass, do it now - it will update the dirty events
    if (mCore->mTop != nullptr && mCore->mTop->needsLayout())
        mCore->mTop->layout(mCore->width, mCore->height, true);
}

bool
RootContext::hasEvent() const
{
    assert(mCore);
    clearPending();
    return !mCore->events.empty();
}

Event
RootContext::popEvent()
{
    assert(mCore);
    clearPending();

    if (!mCore->events.empty()) {
        Event event = mCore->events.front();
        mCore->events.pop();
        return event;
    }

    LOG(LogLevel::ERROR) << "No events available";
    assert(false);
}

bool
RootContext::isDirty() const
{
    assert(mCore);
    clearPending();
    return mCore->dirty.size() > 0;
}

const std::set<ComponentPtr>&
RootContext::getDirty()
{
    assert(mCore);
    clearPending();
    return mCore->dirty;
}

void
RootContext::clearDirty()
{
    assert(mCore);
    for (auto component : mCore->dirty)
        component->clearDirty();

    mCore->dirty.clear();
}


std::shared_ptr<ObjectMap>
RootContext::createDocumentEventProperties(const std::string& handler) const {
    auto source = std::make_shared<ObjectMap>();
    source->emplace("source", "Document");
    source->emplace("type", "Document");
    source->emplace("handler", handler);
    source->emplace("id", Object::NULL_OBJECT());
    source->emplace("uid", Object::NULL_OBJECT());
    source->emplace("value", Object::NULL_OBJECT());
    auto event = std::make_shared<ObjectMap>();
    event->emplace("source", source);
    return event;
}

ContextPtr
RootContext::createDocumentContext(const std::string& handler)
{
    ContextPtr ctx = Context::create(mContext);
    auto event = createDocumentEventProperties(handler);
    ctx->putConstant("event", event);
    return ctx;
}


ContextPtr
RootContext::createKeyboardDocumentContext(const std::string& handler, const ObjectMapPtr& keyboard)
{
    ContextPtr ctx = Context::create(mContext);
    auto event = createDocumentEventProperties(handler);
    event->emplace("keyboard", keyboard);
    ctx->putConstant("event", event);
    return ctx;
}

ActionPtr
RootContext::executeCommands(const apl::Object& commands, bool fastMode)
{
    ContextPtr ctx = createDocumentContext("External");
    return mContext->sequencer().executeCommands(commands, ctx, nullptr, fastMode);
}

void
RootContext::cancelExecution()
{
    assert(mCore);
    mCore->sequencer().reset();
}


ComponentPtr
RootContext::topComponent()
{
    return mCore->mTop;
}

void
RootContext::updateTime(apl_time_t currentTime)
{
    updateTime(currentTime, currentTime - mTimeManager->currentTime() + mLocalTime);
}

void
RootContext::updateTime(apl_time_t currentTime, apl_time_t localTime)
{
    mTimeManager->updateTime(currentTime);
    mContext->systemUpdateAndRecalculate(ELAPSED_TIME, mTimeManager->currentTime(), true); // Read back in case it gets changed

    mLocalTime = localTime;
    mContext->systemUpdateAndRecalculate(LOCAL_TIME, mLocalTime, true);
    mContext->systemUpdateAndRecalculate(UTC_TIME, localTime - mCore->rootConfig().getLocalTimeAdjustment(), true);
}

void
RootContext::scrollToRectInComponent(const ComponentPtr& component, const Rect &bounds,
                                     CommandScrollAlign align) {
    ScrollToAction::make(mTimeManager, align, bounds, mContext, component);
}


apl_time_t
RootContext::nextTime()
{
    return mTimeManager->nextTimeout();
}

apl_time_t
RootContext::currentTime()
{
    return mTimeManager->currentTime();
}

bool
RootContext::screenLock()
{
    assert(mCore);
    clearPending();
    return mCore->screenLock();
}

const Settings&
RootContext::settings() {
    return mCore->mSettings;
}

bool
RootContext::setup()
{
    // Generate an ordered dependency list
    std::vector<PackagePtr> ordered;
    std::set<PackagePtr> inProgress;
    if (!addToDependencyList(ordered, inProgress, mContent->getDocument()))
        return false;

    // check type field of each package
    auto enforceTypeField = mCore->rootConfig().getEnforceTypeField();
    if(!verifyTypeField(ordered, enforceTypeField)) {
        return false;
    }

    auto supportedVersions = mCore->rootConfig().getEnforcedAPLVersion();
    if(!verifyAPLVersionCompatibility(ordered, supportedVersions)) {
        return false;
    }

    bool trackProvenance = mCore->rootConfig().getTrackProvenance();

    // Read settings
    {
        const auto& json = mContent->getDocument()->json();
        auto settingsIter = json.FindMember("settings");

        // NOTE: Backward compatibility for some APL 1.0 users where a runtime allowed "features" instead of "settings"
        if (settingsIter == json.MemberEnd())
            settingsIter = json.FindMember("features");

        if (settingsIter != json.MemberEnd() && settingsIter->value.IsObject())
            mCore->mSettings.read(settingsIter->value);
    }

    // Resource processing:
    for (const auto& child : ordered) {
        const auto& json = child->json();
        const auto path = Path(trackProvenance ? child->name() : std::string());

        auto resIter = json.FindMember("resources");
        if (resIter != json.MemberEnd() && resIter->value.IsArray())
            addOrderedResources(*mContext, resIter->value, path.addArray("resources"));
    }

    // Style processing
    for (const auto& child : ordered) {
        const auto& json = child->json();
        const auto path = Path(trackProvenance ? child->name() : std::string());

        auto styleIter = json.FindMember("styles");
        if (styleIter != json.MemberEnd() && styleIter->value.IsObject())
            mCore->styles().addStyleDefinitions(mCore->session(), &styleIter->value, path.addObject("styles"));
    }

    // Layout processing
    for (const auto& child : ordered) {
        const auto& json = child->json();
        const auto path = Path(trackProvenance ? child->name() : std::string()).addObject("layouts");

        auto layoutIter = json.FindMember("layouts");
        if (layoutIter != json.MemberEnd() && layoutIter->value.IsObject()) {
            for (const auto& kv : layoutIter->value.GetObject()) {
                const auto& name = kv.name.GetString();
                mCore->mLayouts[name] = { &kv.value, path.addObject(name) };
            }
        }
    }

    // Command processing
    for (const auto& child : ordered) {
        const auto& json = child->json();
        const auto path = Path(trackProvenance ? child->name() : std::string()).addObject("commands");

        auto commandIter = json.FindMember("commands");
        if (commandIter != json.MemberEnd() && commandIter->value.IsObject()) {
            for (const auto& kv : commandIter->value.GetObject()) {
                const auto& name = kv.name.GetString();
                mCore->mCommands[name] = { &kv.value, path.addObject(name) };
            }
        }
    }

    // Graphics processing
    for (const auto& child : ordered) {
        const auto& json = child->json();
        const auto path = Path(trackProvenance ? child->name() : std::string()).addObject("graphics");

        auto graphicsIter = json.FindMember("graphics");
        if (graphicsIter != json.MemberEnd() && graphicsIter->value.IsObject()) {
            for (const auto& kv : graphicsIter->value.GetObject()) {
                const auto& name = kv.name.GetString();
                mCore->mGraphics[name] = { &kv.value, path.addObject(name)};
            }
        }
    }

    // Inflate the top component
    Properties properties;

    mContent->getMainProperties(properties);
    mCore->mTop = Builder::inflate(mContext, properties, mContent->getMainTemplate());

    if (!mCore->mTop)
        return false;

    mCore->mTop->layout(mCore->width, mCore->height, false);

    // Execute the "onMount" document command
    auto cmd = DocumentCommand::create(kPropertyOnMount, "Mount", shared_from_this());
    mContext->sequencer().execute(cmd, false);

    // A bunch of commands may be queued up at the start time.  Clear those out.
    clearPending();

    // Those commands may have set the dirty flags.  Clear them.
    clearDirty();

    return true;
}

bool
RootContext::addToDependencyList(std::vector<PackagePtr>& ordered,
                                 std::set<PackagePtr>& inProgress,
                                 const PackagePtr& package)
{
    LOG_IF(DEBUG_ROOT_CONTEXT) << "addToDependencyList" << package << " dependency count="
                             << package->getDependencies().size();

    inProgress.insert(package);  // For dependency loop detection

    // Start with the package dependencies
    for (const auto& ref : package->getDependencies()) {
        LOG_IF(DEBUG_ROOT_CONTEXT) << "checking child " << ref.toString();

        // Convert the reference into a loaded PackagePtr
        const auto& pkg = mContent->loaded().find(ref);
        if (pkg == mContent->loaded().end()) {
            LOGF(LogLevel::ERROR, "Missing package '%s' in the loaded set", ref.name().c_str());
            return false;
        }

        const PackagePtr& child = pkg->second;

        // Check if it is already in the dependency list (someone else included it first)
        auto it = std::find(ordered.begin(), ordered.end(), child);
        if (it != ordered.end()) {
            LOG_IF(DEBUG_ROOT_CONTEXT) << "child package " << ref.toString() << " already in dependency list";
            continue;
        }

        // Check for a circular dependency
        if (inProgress.count(child)) {
            CONSOLE_CTP(mContext).log("Circular package dependency '%s'", ref.name().c_str());
            return false;
        }

        if (!addToDependencyList(ordered, inProgress, child))
            return false;
    }

    LOG_IF(DEBUG_ROOT_CONTEXT) << "Pushing package " << package << " onto ordered list";
    ordered.push_back(package);
    inProgress.erase(package);
    return true;
}

bool
RootContext::verifyAPLVersionCompatibility(const std::vector<PackagePtr>& ordered,
                                           const APLVersion& compatibilityVersion)
{
    for(const auto& child : ordered) {
        if(!compatibilityVersion.isValid(child->version())) {
            CONSOLE_CTP(mContext) << child->name() << " has invalid version: " << child->version();
            return false;
        }
    }
    return true;
}

bool
RootContext::verifyTypeField(const std::vector<std::shared_ptr<Package>>& ordered,
                             bool enforce)
{
    for(auto& child : ordered) {
        auto type = child->type().c_str();
        if (std::strcmp(type, "APML") == 0)
            CONSOLE_CTP(mContext)<< child->name() << ": Stop using the APML document format!";
        else if (std::strcmp(type, "APL") != 0) {
            CONSOLE_CTP(mContext) << child->name() << ": Document type field should be \"APL\"!";
            if(enforce) {
                return false;
            }
        }
    }
    return true;
}


streamer&
operator<<(streamer& os, const RootContext& root)
{
    os << "RootContext: " << root.context();
    return os;
}


void
RootContext::updateCursorPosition(Point cursorPosition) {

    assert(mCore);
    mCore->hoverManager().setCursorPosition(cursorPosition);

}

bool
RootContext::handleKeyboard(KeyHandlerType type, const Keyboard &keyboard) {

    assert(mCore);
    auto &km = mCore->keyboardManager();
    auto &fm = mCore->focusManager();
    return km.handleKeyboard(type, fm.getFocus(), keyboard, shared_from_this());
}

const SessionPtr&
RootContext::getSession() const
{
    return mCore->session();
}

} // namespace apl
