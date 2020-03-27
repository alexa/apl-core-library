/**
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include "apl/engine/extensionmanager.h"
#include "apl/engine/rootcontext.h"
#include "apl/engine/resources.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/rootcontextdata.h"
#include "apl/time/timemanager.h"
#include "apl/graphic/graphic.h"
#include "apl/livedata/livedataobject.h"

namespace apl {

const char *ELAPSED_TIME = "elapsedTime";
const char *LOCAL_TIME = "localTime";
const char *UTC_TIME = "utcTime";

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

    mCore = std::make_shared<RootContextData>(metrics, config, theme,
                                              content->getDocument()->version(), session,
                                              content->mExtensionRequests);
    mContext = Context::create(metrics, mCore);

    mContext->putSystemWriteable(ELAPSED_TIME, mTimeManager->currentTime());

    mUTCTime = config.getUTCTime();
    mLocalTimeAdjustment = config.getLocalTimeAdjustment();
    mContext->putSystemWriteable(UTC_TIME, mUTCTime);
    mContext->putSystemWriteable(LOCAL_TIME, mUTCTime + mLocalTimeAdjustment);

    // Insert one LiveArrayObject or LiveMapObject into the top-level context for each defined LiveObject
    for (const auto& m : config.getLiveObjectMap())
        LiveDataObject::create(m.second, mContext, m.first);
}

RootContext::~RootContext()
{
    assert(mCore);
    clearDirty();
    mCore->terminate();
}

void
RootContext::clearPending() const
{
    assert(mCore);

    // Flush any dynamic data changes
    mCore->dataManager().flushDirty();

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

    // This should never be reached.
    LOG(LogLevel::ERROR) << "No events available";
    std::exit(EXIT_FAILURE);
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
    ContextPtr ctx = Context::create(payloadContext());
    auto event = createDocumentEventProperties(handler);
    ctx->putConstant("event", event);
    return ctx;
}


ContextPtr
RootContext::createKeyboardDocumentContext(const std::string& handler, const ObjectMapPtr& keyboard)
{
    ContextPtr ctx = Context::create(payloadContext());
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

ActionPtr
RootContext::invokeExtensionEventHandler(const std::string& uri, const std::string& name, const ObjectMap& data,
                                         bool fastMode)
{
    auto handler = mCore->extensionManager().findHandler(ExtensionEventHandler{uri, name});
    if (handler.isNull())
        return nullptr;

    // Create a document-level context and copy the provided data in
    ContextPtr ctx = createDocumentContext(name);
    for (const auto& m : data)
        ctx->putConstant(m.first, m.second);

    return mContext->sequencer().executeCommands(handler, ctx, nullptr, fastMode);
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

ContextPtr
RootContext::payloadContext() const
{
    // We could cache the payload context, but it is infrequently used. Instead we search upwards from the
    // top components context until we find the context right before the top-level context.

    auto context = mCore->mTop->getContext();
    if (context == nullptr || context == mContext)
        return mContext;

    while (context->parent() != mContext)
        context = context->parent();

    return context;
}

void
RootContext::updateTime(apl_time_t elapsedTime)
{
    auto lastTime = mTimeManager->currentTime();
    mTimeManager->updateTime(elapsedTime);
    mContext->systemUpdateAndRecalculate(ELAPSED_TIME, mTimeManager->currentTime(), true); // Read back in case it gets changed

    // Update the local time by how much time passed on the "elapsed" timer
    mUTCTime += mTimeManager->currentTime() - lastTime;
    mContext->systemUpdateAndRecalculate(UTC_TIME, mUTCTime, true);
    mContext->systemUpdateAndRecalculate(LOCAL_TIME, mUTCTime + mLocalTimeAdjustment, true);
}

void
RootContext::updateTime(apl_time_t elapsedTime, apl_time_t utcTime)
{
    mTimeManager->updateTime(elapsedTime);
    mContext->systemUpdateAndRecalculate(ELAPSED_TIME, mTimeManager->currentTime(), true); // Read back in case it gets changed

    mUTCTime = utcTime;
    mContext->systemUpdateAndRecalculate(UTC_TIME, mUTCTime, true);
    mContext->systemUpdateAndRecalculate(LOCAL_TIME, mUTCTime + mLocalTimeAdjustment, true);
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

    std::vector<PackagePtr> ordered = mContent->ordered();

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
        const rapidjson::Value& settingsValue = Settings::findSettings(*mContent->getDocument());
        if (!settingsValue.IsNull())
            mCore->mSettings.read(settingsValue);
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

    // Identify all registered event handlers in all ordered documents
    auto& em = mCore->extensionManager();
    for (const auto& handler : em.qualifiedHandlerMap()) {
        for (const auto& child : ordered) {
            const auto& json = child->json();
            auto h = json.FindMember(handler.first.c_str());
            if (h != json.MemberEnd()) {
                auto oldHandler = em.findHandler(handler.second);
                if (!oldHandler.isNull())
                    CONSOLE_CTP(mContext) << "Overwriting existing command handler " << handler.first;
                em.addEventHandler(handler.second, asCommand(mContext, evaluate(mContext, h->value)));
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
        auto type = child->type();
        if (type.compare("APML") == 0) CONSOLE_CTP(mContext)
                    << child->name() << ": Stop using the APML document format!";
        else if (type.compare("APL") != 0) {
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
