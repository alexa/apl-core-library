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

#include "rapidjson/stringbuffer.h"

#include "apl/engine/evaluate.h"
#include "apl/engine/styles.h"
#include "apl/engine/propdef.h"
#include "apl/action/scrolltoaction.h"
#include "apl/command/arraycommand.h"
#include "apl/command/configchangecommand.h"
#include "apl/command/documentcommand.h"
#include "apl/component/yogaproperties.h"
#include "apl/content/content.h"
#include "apl/content/metrics.h"
#include "apl/content/rootconfig.h"
#include "apl/content/configurationchange.h"
#include "apl/datasource/datasource.h"
#include "apl/datasource/datasourceprovider.h"
#include "apl/engine/builder.h"
#include "apl/extension/extensionmanager.h"
#include "apl/engine/rootcontext.h"
#include "apl/engine/resources.h"
#include "apl/engine/rootcontextdata.h"
#include "apl/graphic/graphic.h"
#include "apl/livedata/livedataobject.h"
#include "apl/livedata/livedataobjectwatcher.h"
#include "apl/time/timemanager.h"
#include "apl/utils/log.h"
#include "apl/utils/tracing.h"

namespace apl {

static const char *ELAPSED_TIME = "elapsedTime";
static const char *LOCAL_TIME = "localTime";
static const char *UTC_TIME = "utcTime";
static const char *ON_MOUNT_HANDLER_NAME = "Mount";

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
        LOG(LogLevel::kError) << "Attempting to create root context with illegal content";
        return nullptr;
    }

    auto root = std::make_shared<RootContext>(metrics, content, config);
    if (callback)
        callback(root);
    if (!root->setup(nullptr))
        return nullptr;

#ifdef ALEXAEXTENSIONS
    // Bind to the extension mediator
    // TODO ExtensionMediator is an experimental class facilitating message passing to and from extensions.
    // TODO The mediator class will be replaced by direct messaging between extensions and ExtensionManager
    auto extensionMediator = config.getExtensionMediator();
    if (extensionMediator) {
        extensionMediator->bindContext(root);
    }
#endif

    return root;
}

RootContext::RootContext(const Metrics& metrics, const ContentPtr& content, const RootConfig& config)
    : mContent(content),
      mTimeManager(config.getTimeManager()) {
    init(metrics, config, false);
}

RootContext::~RootContext() {
    assert(mCore);
    mCore->sequencer().terminateSequencer(ConfigChangeCommand::SEQUENCER);
    clearDirty();
    mCore->dirtyVisualContext.clear();
    mTimeManager->terminate();
    mCore->terminate();
}

void
RootContext::configurationChange(const ConfigurationChange& change)
{
    // If we're in the middle of a configuration change, drop it
    mCore->sequencer().terminateSequencer(ConfigChangeCommand::SEQUENCER);

    mActiveConfigurationChanges.mergeConfigurationChange(change);
    if (mActiveConfigurationChanges.empty())
        return;

    auto cmd = ConfigChangeCommand::create(shared_from_this(),
                                           mActiveConfigurationChanges.asEventProperties(mCore->mConfig,
                                                                                         mCore->mMetrics));
    mContext->sequencer().executeOnSequencer(cmd, ConfigChangeCommand::SEQUENCER);
}

void
RootContext::reinflate()
{
    // The basic algorithm is to simply re-build core and re-inflate the component hierarchy.
    // TODO: Re-use parts of the hierarchy and to maintain state during reinflation.

    // Release any "onConfigChange" action
    mCore->sequencer().terminateSequencer(ConfigChangeCommand::SEQUENCER);

    auto metrics = mActiveConfigurationChanges.mergeMetrics(mCore->mMetrics);
    auto config = mActiveConfigurationChanges.mergeRootConfig(mCore->mConfig);

    // Update the configuration with the current UTC time and time adjustment
    config.utcTime(mUTCTime);
    config.localTimeAdjustment(mLocalTimeAdjustment);

    // Stop any execution on the old core
    auto oldTop = mCore->halt();
    // Ensure that nothing is pending.
    assert(mTimeManager->size() == 0);

    // The initialization routine replaces mCore with a new core
    init(metrics, config, true);
    setup(oldTop);  // Pass the old top component

    // If there was a previous top component, release it and its children to free memory
    if (oldTop)
        oldTop->release();

    // Clear the old active configuration; it is reset on a reinflation
    mActiveConfigurationChanges.clear();
}

void
RootContext::resize()
{
    // Release any "onConfigChange" action
    mCore->sequencer().terminateSequencer(ConfigChangeCommand::SEQUENCER);
    mCore->layoutManager().configChange(mActiveConfigurationChanges);
    // Note: we do not clear the configuration changes - there may be a reinflate() coming in the future.
}

static LayoutDirection
getLayoutDirection(const rapidjson::Value& json, const RootConfig& config) {
    LayoutDirection layoutDirection = static_cast<LayoutDirection>(config.getProperty(RootProperty::kLayoutDirection).asInt());
    auto layoutDirectionIter = json.FindMember("layoutDirection");
    if (layoutDirectionIter != json.MemberEnd() && layoutDirectionIter->value.IsString()) {
        auto s = layoutDirectionIter->value.GetString();
        layoutDirection = static_cast<LayoutDirection>(sLayoutDirectionMap.get(s, kLayoutDirectionLTR));
        if (sLayoutDirectionMap.find(s) == sLayoutDirectionMap.endBtoA()) {
            LOG(LogLevel::kWarn) << "Document 'layoutDirection' property is invalid. Falling back to 'LTR' instead of : " << s;
        }
    }
    if (layoutDirection == kLayoutDirectionInherit) {
        LOG(LogLevel::kWarn) << "Document 'layoutDirection' can not be 'Inherit', falling back to 'LTR' instead";
        layoutDirection = kLayoutDirectionLTR;
    }
    return layoutDirection;
}

void
RootContext::init(const Metrics& metrics, const RootConfig& config, bool reinflation)
{
    APL_TRACE_BLOCK("RootContext:init");
    std::string theme = metrics.getTheme();
    const auto& json = mContent->getDocument()->json();
    auto themeIter = json.FindMember("theme");
    if (themeIter != json.MemberEnd() && themeIter->value.IsString())
        theme = themeIter->value.GetString();

    std::string lang = config.getProperty(RootProperty::kLang).asString();
    auto langIter = json.FindMember("lang");
    if (langIter != json.MemberEnd() && langIter->value.IsString())
        lang = langIter->value.GetString();

    auto layoutDirection = getLayoutDirection(json, config);

    auto session = config.getSession();
    if (!session)
        session = mContent->getSession();

    mCore = std::make_shared<RootContextData>(metrics,
                                              config,
                                              RuntimeState(theme,
                                                           mContent->getDocument()->version(),
                                                           reinflation),
                                              mContent->getDocumentSettings(),
                                              session,
                                              mContent->mExtensionRequests);
    mCore->lang(lang)
          .layoutDirection(layoutDirection);
    mContext = Context::createRootEvaluationContext(metrics, mCore);

    mContext->putSystemWriteable(ELAPSED_TIME, mTimeManager->currentTime());

    mUTCTime = config.getUTCTime();
    mLocalTimeAdjustment = config.getLocalTimeAdjustment();
    mContext->putSystemWriteable(UTC_TIME, mUTCTime);
    mContext->putSystemWriteable(LOCAL_TIME, mUTCTime + mLocalTimeAdjustment);

    // Insert one LiveArrayObject or LiveMapObject into the top-level context for each defined LiveObject
    for (const auto& m : config.getLiveObjectMap()) {
        auto ldo = LiveDataObject::create(m.second, mContext, m.first);
        auto watchers = config.getLiveDataWatchers(m.first);
        for (auto& watcher : watchers) {
            if (watcher)
                watcher->registerObjectWatcher(ldo);
        }
    }
}

void
RootContext::clearPending() const
{
    clearPendingInternal(false);
}

void
RootContext::clearPendingInternal(bool first) const
{
    assert(mCore);

    APL_TRACE_BLOCK("RootContext:clearPending");
    // Flush any dynamic data changes
    mCore->dataManager().flushDirty();

    // Make sure any pending events have executed
    mTimeManager->runPending();

    // If we need a layout pass, do it now - it will update the dirty events
    if (mCore->layoutManager().needsLayout())
        mCore->layoutManager().layout(true, first);

    mCore->mediaManager().processMediaRequests(mContext);

    // Run any onMount handlers for something that may have been attached at runtime
    // We execute those on the sequencer to avoid messing stuff up. Will work much more similarly to previous behavior,
    // but will not interrupt something that may have been scheduled just before.
    auto& onMounts = mCore->pendingOnMounts();
    if (!onMounts.empty()) {
        const auto& tm = getRootConfig().getTimeManager();
        std::vector<ActionPtr> parallelCommands;
        for (auto& pendingOnMount : onMounts) {
            if (auto comp = pendingOnMount.lock()) {
                auto commands = comp->getCalculated(kPropertyOnMount);
                auto ctx = comp->createDefaultEventContext(ON_MOUNT_HANDLER_NAME);
                parallelCommands.emplace_back(
                        ArrayCommand::create(
                                ctx,
                                commands,
                                comp,
                                Properties(),
                                "")->execute(tm, false));
            }
        }
        onMounts.clear();

        auto mountAction = Action::makeAll(tm, parallelCommands);
        mCore->sequencer().attachToSequencer(mountAction, "__MOUNT_SEQUENCER");
    }
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
    LOG(LogLevel::kError) << "No events available";
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
    APL_TRACE_BLOCK("RootContext:clearDirty");
    for (auto& component : mCore->dirty)
        component->clearDirty();

    mCore->dirty.clear();
}


bool
RootContext::isVisualContextDirty() const
{
    assert(mCore);
    return !mCore->dirtyVisualContext.empty();
}

void
RootContext::clearVisualContextDirty()
{
    assert(mCore);
    mCore->dirtyVisualContext.clear();
}

rapidjson::Value
RootContext::serializeVisualContext(rapidjson::Document::AllocatorType& allocator)
{
    clearVisualContextDirty();
    return topComponent()->serializeVisualContext(allocator);
}

bool
RootContext::isDataSourceContextDirty() const
{
    assert(mCore);
    return !mCore->dirtyDatasourceContext.empty();
}

void
RootContext::clearDataSourceContextDirty()
{
    assert(mCore);
    mCore->dirtyDatasourceContext.clear();
}

rapidjson::Value
RootContext::serializeDataSourceContext(rapidjson::Document::AllocatorType& allocator)
{
    clearDataSourceContextDirty();

    rapidjson::Value outArray(rapidjson::kArrayType);

    for (const auto& tracker : mCore->dataManager().trackers()) {
        if (auto sourceConnection = tracker->getDataSourceConnection()) {
            rapidjson::Value datasource(rapidjson::kObjectType);
            sourceConnection->serialize(datasource, allocator);

            outArray.PushBack(datasource.Move(), allocator);
        }
    }
    return outArray;
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
RootContext::createDocumentContext(const std::string& handler, const ObjectMap& optional)
{
    ContextPtr ctx = Context::createFromParent(payloadContext());
    auto event = createDocumentEventProperties(handler);
    for (const auto& m : optional)
        event->emplace(m.first, m.second);
    ctx->putConstant("event", event);
    return ctx;
}


ContextPtr
RootContext::createKeyboardDocumentContext(const std::string& handler, const ObjectMapPtr& keyboard)
{
    ContextPtr ctx = Context::createFromParent(payloadContext());
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
    ContextPtr ctx = createDocumentContext(name, data);
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
    if (!mCore || !mCore->mTop)
        return mContext;

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

    mCore->pointerManager().handleTimeUpdate(elapsedTime);
}

void
RootContext::updateTime(apl_time_t elapsedTime, apl_time_t utcTime)
{
    mTimeManager->updateTime(elapsedTime);
    mContext->systemUpdateAndRecalculate(ELAPSED_TIME, mTimeManager->currentTime(), true); // Read back in case it gets changed

    mUTCTime = utcTime;
    mContext->systemUpdateAndRecalculate(UTC_TIME, mUTCTime, true);
    mContext->systemUpdateAndRecalculate(LOCAL_TIME, mUTCTime + mLocalTimeAdjustment, true);

    mCore->pointerManager().handleTimeUpdate(elapsedTime);
}

void
RootContext::scrollToRectInComponent(const ComponentPtr& component, const Rect &bounds,
                                     CommandScrollAlign align) {
    ScrollToAction::make(mTimeManager, align, bounds, mContext, std::static_pointer_cast<CoreComponent>(component));
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

/**
 * @deprecated Use Content->getDocumentSettings()
 */
const Settings&
RootContext::settings() {
    return *(mCore->mSettings);
}

const RootConfig&
RootContext::rootConfig() {
    return mCore->rootConfig();
}

bool
RootContext::setup(const CoreComponentPtr& top)
{
    APL_TRACE_BLOCK("RootContext:setup");
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
    // Deprecated, get settings from Content->getDocumentSettings()
    {
        APL_TRACE_BEGIN("RootContext:readSettings");
        mCore->mSettings->read(mCore->rootConfig());
        APL_TRACE_END("RootContext:readSettings");
    }

    // Resource processing:
    APL_TRACE_BEGIN("RootContext:processResources");
    for (const auto& child : ordered) {
        const auto& json = child->json();
        const auto path = Path(trackProvenance ? child->name() : std::string());
        addNamedResourcesBlock(*mContext, json, path, "resources");
    }
    APL_TRACE_END("RootContext:processResources");

    // Style processing
    APL_TRACE_BEGIN("RootContext:processStyles");
    for (const auto& child : ordered) {
        const auto& json = child->json();
        const auto path = Path(trackProvenance ? child->name() : std::string());

        auto styleIter = json.FindMember("styles");
        if (styleIter != json.MemberEnd() && styleIter->value.IsObject())
            mCore->styles()->addStyleDefinitions(mCore->session(), &styleIter->value, path.addObject("styles"));
    }
    APL_TRACE_END("RootContext:processStyles");

    // Layout processing
    APL_TRACE_BEGIN("RootContext:processLayouts");
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
    APL_TRACE_END("RootContext:processLayouts");

    // Command processing
    APL_TRACE_BEGIN("RootContext:processCommands");
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
    APL_TRACE_END("RootContext:processCommands");

    // Graphics processing
    APL_TRACE_BEGIN("RootContext:processGraphics");
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
    APL_TRACE_END("RootContext:processGraphics");

    // Identify all registered event handlers in all ordered documents
    APL_TRACE_BEGIN("RootContext:processExtensionHandlers");
    auto& em = mCore->extensionManager();
    for (const auto& handler : em.qualifiedHandlerMap()) {
        for (const auto& child : ordered) {
            const auto& json = child->json();
            auto h = json.FindMember(handler.first.c_str());
            if (h != json.MemberEnd()) {
                auto oldHandler = em.findHandler(handler.second);
                if (!oldHandler.isNull())
                    CONSOLE_CTP(mContext) << "Overwriting existing command handler " << handler.first;
                em.addEventHandler(handler.second, asCommand(*mContext, evaluate(*mContext, h->value)));
            }
        }
    }
    APL_TRACE_END("RootContext:processExtensionHandlers");

    // Inflate the top component
    Properties properties;

    APL_TRACE_BEGIN("RootContext:retrieveProperties");
    mContent->getMainProperties(properties);
    APL_TRACE_END("RootContext:retrieveProperties");

    mCore->mTop = Builder(top).inflate(mContext, properties, mContent->getMainTemplate());

    if (!mCore->mTop)
        return false;

    mCore->mTop->markGlobalToLocalTransformStale();
    mCore->layoutManager().firstLayout();

    // Execute the "onMount" document command
    APL_TRACE_BEGIN("RootContext:executeOnMount");
    auto cmd = DocumentCommand::create(kPropertyOnMount, ON_MOUNT_HANDLER_NAME, shared_from_this());
    mContext->sequencer().execute(cmd, false);
    // Clear any pending mounts as we just executed those
    mCore->pendingOnMounts().clear();
    APL_TRACE_END("RootContext:executeOnMount");

    // A bunch of commands may be queued up at the start time.  Clear those out.
    clearPendingInternal(true);

    // Those commands may have set the dirty flags.  Clear them.
    clearDirty();

    // Commands or layout may have marked visual context dirty.  Clear visual context.
    mCore->dirtyVisualContext.clear();

    // Process and schedule tick handlers.
    processTickHandlers();

    return true;
}

void
RootContext::scheduleTickHandler(const Object& handler, double delay) {
    auto weak_ptr = std::weak_ptr<RootContext>(shared_from_this());

    // Lambda capture takes care of handler here as it's a copy.
    mTimeManager->setTimeout([weak_ptr, handler, delay]() {
        auto self = weak_ptr.lock();
        if (!self)
            return;

        auto ctx = self->createDocumentContext("Tick");
        if (propertyAsBoolean(*ctx, handler, "when", true)) {
            auto commands = Object(arrayifyProperty(*ctx, handler, "commands"));
            if (!commands.empty())
                self->context().sequencer().executeCommands(commands, ctx, nullptr, true);
        }

        self->scheduleTickHandler(handler, delay);

    }, delay);
}

void
RootContext::processTickHandlers() {
    auto& json = content()->getDocument()->json();
    auto it = json.FindMember("handleTick");
    if (it == json.MemberEnd())
        return;

    auto tickHandlers = asArray(*mContext, evaluate(*mContext, it->value));

    if (tickHandlers.empty() || !tickHandlers.isArray())
        return;

    for (const auto& handler : tickHandlers.getArray()) {
        auto delay = std::max(propertyAsDouble(*mContext, handler, "minimumDelay", 1000),
                mCore->rootConfig().getTickHandlerUpdateLimit());
        scheduleTickHandler(handler, delay);
    }
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


/* Remove when migrated to handlePointerEvent */
void
RootContext::updateCursorPosition(Point cursorPosition) {
    handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, cursorPosition));
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

bool
RootContext::handlePointerEvent(const PointerEvent& pointerEvent) {
    assert(mCore);
    return mCore->pointerManager().handlePointerEvent(pointerEvent, mTimeManager->currentTime());
}

const RootConfig&
RootContext::getRootConfig() const
{
    assert(mCore);
    return mCore->rootConfig();
}

std::string
RootContext::getTheme() const
{
    assert(mCore);
    return mCore->getTheme();
}

const TextMeasurementPtr&
RootContext::measure() const
{
    return mCore->measure();
}

ComponentPtr
RootContext::findComponentById(const std::string& id) const
{
    assert(mCore);

    auto top = mCore->top();
    return top ? top->findComponentById(id) : nullptr;
}

std::map<std::string, Rect>
RootContext::getFocusableAreas()
{
    assert(mCore);
    return mCore->focusManager().getFocusableAreas();
}

bool
RootContext::setFocus(FocusDirection direction, const Rect& origin, const std::string& targetId)
{
    assert(mCore);
    auto top = mCore->top();
    auto target = std::dynamic_pointer_cast<CoreComponent>(findComponentById(targetId));

    if (!target) {
        LOG(LogLevel::kWarn) << "Don't have component: " << targetId;
        return false;
    }

    Rect targetRect;
    target->getBoundsInParent(top, targetRect);

    // Shift origin into target's coordinate space.
    auto offsetFocusRect = origin;
    offsetFocusRect.offset(-targetRect.getTopLeft());

    return mCore->focusManager().focus(direction, offsetFocusRect, target);
}

bool
RootContext::nextFocus(FocusDirection direction, const Rect& origin)
{
    assert(mCore);
    return mCore->focusManager().focus(direction, origin);
}

bool
RootContext::nextFocus(FocusDirection direction)
{
    assert(mCore);
    return mCore->focusManager().focus(direction);
}

void
RootContext::clearFocus()
{
    assert(mCore);
    mCore->focusManager().clearFocus(false);
}

std::string
RootContext::getFocused()
{
    assert(mCore);
    auto focused = mCore->focusManager().getFocus();
    return focused ? focused->getUniqueId() : "";
}

void
RootContext::mediaLoaded(const std::string& source)
{
    assert(mCore);
    mCore->mediaManager().mediaLoadComplete(source, true);
}

void
RootContext::mediaLoadFailed(const std::string &source)
{
    assert(mCore);
    mCore->mediaManager().mediaLoadComplete(source, false);
}

} // namespace apl
