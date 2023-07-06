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

#include "apl/engine/corerootcontext.h"

#include "rapidjson/stringbuffer.h"

#include "apl/action/scrolltoaction.h"
#include "apl/command/arraycommand.h"
#include "apl/datasource/datasource.h"
#include "apl/datasource/datasourceprovider.h"
#include "apl/embed/documentregistrar.h"
#include "apl/engine/builder.h"
#include "apl/engine/keyboardmanager.h"
#include "apl/engine/layoutmanager.h"
#include "apl/engine/resources.h"
#include "apl/engine/sharedcontextdata.h"
#include "apl/engine/tickscheduler.h"
#include "apl/engine/uidmanager.h"
#include "apl/extension/extensionmanager.h"
#include "apl/focus/focusmanager.h"
#include "apl/graphic/graphic.h"
#include "apl/livedata/livedatamanager.h"
#include "apl/media/mediamanager.h"
#include "apl/time/sequencer.h"
#include "apl/time/timemanager.h"
#include "apl/touch/pointermanager.h"
#include "apl/utils/tracing.h"
#ifdef SCENEGRAPH
#include "apl/scenegraph/builder.h"
#include "apl/scenegraph/scenegraph.h"
#endif // SCENEGRAPH


namespace apl {

static const std::string SCROLL_TO_RECT_SEQUENCER = "__SCROLL_TO_RECT_SEQUENCE";

RootContextPtr
CoreRootContext::create(const Metrics& metrics,
                    const ContentPtr& content,
                    const RootConfig& config,
                    std::function<void(const RootContextPtr&)> callback)
{
    if (!content->isReady()) {
        LOG(LogLevel::kError).session(content) << "Attempting to create root context with illegal content";
        return nullptr;
    }

    auto root = std::make_shared<CoreRootContext>(
        metrics,
        content,
        config);

    root->init(metrics, config, content);

    if (callback)
        callback(root);
    if (!root->setup(false))
        return nullptr;

    return root;
}

CoreRootContext::CoreRootContext(const Metrics& metrics,
                         const ContentPtr& content,
                         const RootConfig& config)
    : mTimeManager(config.getTimeManager()),
      mDisplayState(static_cast<DisplayState>(config.getProperty(RootProperty::kInitialDisplayState).getInteger()))
{
}

CoreRootContext::~CoreRootContext() {
    assert(mShared);
    mTimeManager->terminate();
    mTopDocument = nullptr;
    mShared->halt();
}

void
CoreRootContext::configurationChange(const ConfigurationChange& change)
{
    assert(mTopDocument);
    mTopDocument->configurationChange(change);

    mShared->documentRegistrar().forEach([change](const std::shared_ptr<CoreDocumentContext>& document) {
        // Pass change through as is, document will figure it out itself
        return document->configurationChange(change);
    });
}

void
CoreRootContext::updateDisplayState(DisplayState displayState)
{
    assert(mTopDocument);
    mTopDocument->updateDisplayState(displayState);
}

void
CoreRootContext::reinflate()
{
#ifdef SCENEGRAPH
    // Release the existing scene graph
    mSceneGraph = nullptr;
#endif // SCENEGRAPH

    mTopDocument->updateTime(mUTCTime, mLocalTimeAdjustment);
    if (!mTopDocument->reinflate([&] () { setup(true); } )) {
        LOG(LogLevel::kError) << "Can't reinflate top document, ignoring reinflate command.";
        return;
    }
}

void
CoreRootContext::init(const Metrics& metrics,
                  const RootConfig& config,
                  const ContentPtr& content)
{
    APL_TRACE_BLOCK("RootContext:init");
    mShared = std::make_shared<SharedContextData>(shared_from_this(), metrics, config);

    mTopDocument = CoreDocumentContext::create(mShared, metrics, content, config);

    // Hm. Time is interesting. Because it's actually initialized in the context.
    mUTCTime = config.getUTCTime();
    mLocalTimeAdjustment = config.getLocalTimeAdjustment();
}

void
CoreRootContext::clearPending() const
{
    clearPendingInternal(false);
}

void
CoreRootContext::clearPendingInternal(bool first) const
{
    assert(mTopDocument && mShared);

    APL_TRACE_BLOCK("RootContext:clearPending");
    // Flush any dynamic data changes, for all documents
    mTopDocument->mCore->dataManager().flushDirty();
    mShared->documentRegistrar().forEach([](const std::shared_ptr<CoreDocumentContext>& document) {
        return document->mCore->dataManager().flushDirty();
    });

    // Make sure any pending events have executed
    mTimeManager->runPending();

    // If we need a layout pass, do it now - it will update the dirty events
    if (mShared->layoutManager().needsLayout())
        mShared->layoutManager().layout(true, first);

    mShared->mediaManager().processMediaRequests(std::static_pointer_cast<CoreDocumentContext>(mTopDocument)->mContext);

    // Clear pending on all docs
    mTopDocument->clearPending();
    mShared->documentRegistrar().forEach([](const std::shared_ptr<CoreDocumentContext>& document) {
        return document->clearPending();
    });
}

bool
CoreRootContext::hasEvent() const
{
    assert(mShared);
    clearPending();

    return !mShared->eventManager().empty();
}

Event
CoreRootContext::popEvent()
{
    assert(mShared);
    clearPending();

    if (!mShared->eventManager().empty()) {
        return mShared->eventManager().pop();
    }

    // This should never be reached.
    LOG(LogLevel::kError) << "No events available";
    std::exit(EXIT_FAILURE);
}

bool
CoreRootContext::isDirty() const
{
    assert(mTopDocument);
    clearPending();
    return !mShared->dirtyComponents().empty();
}

const std::set<ComponentPtr>&
CoreRootContext::getDirty()
{
    assert(mTopDocument);
    clearPending();
    return mShared->dirtyComponents().getAll();
}

void
CoreRootContext::clearDirty()
{
    assert(mTopDocument);
    APL_TRACE_BLOCK("RootContext:clearDirty");
    mShared->dirtyComponents().clear();
}

bool
CoreRootContext::isVisualContextDirty() const
{
    assert(mTopDocument);
    // TODO: Visual context supposed to be central, but deduped on cloud side. So won't be per doc?
    //  Or do we stitch it together?
    return mTopDocument->isVisualContextDirty();
}

void
CoreRootContext::clearVisualContextDirty()
{
    assert(mTopDocument);
    mTopDocument->clearVisualContextDirty();
}

rapidjson::Value
CoreRootContext::serializeVisualContext(rapidjson::Document::AllocatorType& allocator)
{
    clearVisualContextDirty();
    return mTopDocument->serializeVisualContext(allocator);
}

bool
CoreRootContext::isDataSourceContextDirty() const
{
    assert(mTopDocument);
    return mTopDocument->isDataSourceContextDirty();
}

void
CoreRootContext::clearDataSourceContextDirty()
{
    assert(mTopDocument);
    mTopDocument->clearDataSourceContextDirty();
}

rapidjson::Value
CoreRootContext::serializeDataSourceContext(rapidjson::Document::AllocatorType& allocator)
{
    assert(mTopDocument);
    return mTopDocument->serializeDataSourceContext(allocator);
}

rapidjson::Value
CoreRootContext::serializeDOM(bool extended, rapidjson::Document::AllocatorType& allocator)
{
    assert(mTopDocument);
    return mTopDocument->serializeDOM(extended, allocator);
}

rapidjson::Value
CoreRootContext::serializeContext(rapidjson::Document::AllocatorType& allocator)
{
    assert(mTopDocument);
    return mTopDocument->serializeContext(allocator);
}


std::shared_ptr<ObjectMap>
CoreRootContext::createDocumentEventProperties(const std::string& handler) const
{
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
CoreRootContext::createDocumentContext(const std::string& handler, const ObjectMap& optional)
{
    ContextPtr ctx = Context::createFromParent(payloadContext());
    auto event = createDocumentEventProperties(handler);
    for (const auto& m : optional)
        event->emplace(m.first, m.second);
    ctx->putConstant("event", event);
    return ctx;
}

ActionPtr
CoreRootContext::executeCommands(const apl::Object& commands, bool fastMode)
{
    assert(mTopDocument);
    return mTopDocument->executeCommands(commands, fastMode);
}

ActionPtr
CoreRootContext::invokeExtensionEventHandler(const std::string& uri, const std::string& name,
                                         const ObjectMap& data, bool fastMode,
                                         std::string resourceId)
{
    assert(mTopDocument);
    return mTopDocument->invokeExtensionEventHandler(uri, name, data, fastMode, resourceId);
}

void
CoreRootContext::cancelExecution()
{
    assert(mTopDocument);
    mTopDocument->mCore->sequencer().reset();
}

ComponentPtr
CoreRootContext::topComponent() const
{
    assert(mTopDocument);
    return mTopDocument->topComponent();
}

DocumentContextPtr
CoreRootContext::topDocument() const
{
    assert(mTopDocument);
    return mTopDocument;
}

ContextPtr
CoreRootContext::payloadContext() const
{
    assert(mTopDocument);
    return mTopDocument->payloadContext();
}

Sequencer&
CoreRootContext::sequencer() const
{
    return mTopDocument->mCore->sequencer();
}

double
CoreRootContext::getPxToDp() const
{
    return mTopDocument->mCore->getPxToDp();
}

void
CoreRootContext::updateTimeInternal(apl_time_t elapsedTime, apl_time_t utcTime)
{
    APL_TRACE_BLOCK("RootContext:updateTime");
    auto lastTime = mTimeManager->currentTime();

    APL_TRACE_BEGIN("RootContext:flushDirtyData");
    // Flush any dynamic data changes
    mTopDocument->flushDataUpdates();
    APL_TRACE_END("RootContext:flushDirtyData");

    APL_TRACE_BEGIN("RootContext:timeManagerUpdateTime");
    mTimeManager->updateTime(elapsedTime);
    APL_TRACE_END("RootContext:timeManagerUpdateTime");

    if (utcTime > 0) {
        mUTCTime = utcTime;
    } else {
        // Update the local time by how much time passed on the "elapsed" timer
        mUTCTime += mTimeManager->currentTime() - lastTime;
    }

    APL_TRACE_BEGIN("RootContext:systemUpdateAndRecalculateTime");
    mTopDocument->updateTime(mUTCTime, mLocalTimeAdjustment);
    mShared->documentRegistrar().forEach([&](const std::shared_ptr<CoreDocumentContext>& document) {
        document->updateTime(mUTCTime, mLocalTimeAdjustment);
    });
    APL_TRACE_END("RootContext:systemUpdateAndRecalculateTime");

    APL_TRACE_BEGIN("RootContext:pointerHandleTimeUpdate");
    mShared->pointerManager().handleTimeUpdate(elapsedTime);
    APL_TRACE_END("RootContext:pointerHandleTimeUpdate");
}

void
CoreRootContext::updateTime(apl_time_t elapsedTime)
{
    updateTimeInternal(elapsedTime, 0);
}

void
CoreRootContext::updateTime(apl_time_t elapsedTime, apl_time_t utcTime)
{
    updateTimeInternal(elapsedTime, utcTime);
}

void
CoreRootContext::scrollToRectInComponent(const ComponentPtr& component, const Rect &bounds, CommandScrollAlign align)
{
    auto scrollToAction = ScrollToAction::make(
            mTimeManager, align, bounds, std::static_pointer_cast<CoreDocumentContext>(mTopDocument)->mContext, CoreComponent::cast(component));
    if (scrollToAction && scrollToAction->isPending()) {
        mTopDocument->mCore->sequencer().attachToSequencer(scrollToAction, SCROLL_TO_RECT_SEQUENCER);
    }
}


apl_time_t
CoreRootContext::nextTime()
{
    return mTimeManager->nextTimeout();
}

apl_time_t
CoreRootContext::currentTime() const
{
    return mTimeManager->currentTime();
}

bool
CoreRootContext::screenLock() const
{
    assert(mShared);
    clearPending();
    return mShared->screenLock();
}

/**
 * @deprecated Use Content->getDocumentSettings()
 */
const Settings&
CoreRootContext::settings() const
{
    return *mTopDocument->content()->getDocumentSettings();
}

const RootConfig&
CoreRootContext::rootConfig() const
{
    return mTopDocument->rootConfig();
}

bool
CoreRootContext::setup(bool reinflate)
{
    APL_TRACE_BLOCK("RootContext:setup");
    if (!reinflate) {
        if (!mTopDocument->setup(nullptr))
            return false;
    } else {
        // Update LayoutManager with the new overall size
        const auto& metrics = mTopDocument->mCore->metrics();
        mShared->layoutManager().setSize(
            Size(
                static_cast<float>(metrics.getWidth()),
                static_cast<float>(metrics.getHeight())
                )
            );
    }

    mShared->layoutManager().firstLayout();

    //TODO: Verify onMount and Tick handlers when reinflate supported for EmbeddedDocs.
    mTopDocument->processOnMounts();

    // A bunch of commands may be queued up at the start time.  Clear those out.
    clearPendingInternal(true);

    // Those commands may have set the dirty flags.  Clear them.
    clearDirty();

    // Commands or layout may have marked visual context dirty.  Clear visual context.
    mTopDocument->clearVisualContextDirty();

    // Process and schedule tick handlers.
    mShared->tickScheduler().processTickHandlers(mTopDocument);

    return true;
}

/* Remove when migrated to handlePointerEvent */
void
CoreRootContext::updateCursorPosition(Point cursorPosition)
{
    handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, cursorPosition));
}

bool
CoreRootContext::handleKeyboard(KeyHandlerType type, const Keyboard &keyboard)
{
    assert(mShared);
    auto &km = mShared->keyboardManager();
    auto &fm = mShared->focusManager();
    return km.handleKeyboard(type, fm.getFocus(), keyboard, shared_from_this());
}

bool
CoreRootContext::handlePointerEvent(const PointerEvent& pointerEvent)
{
    assert(mShared);
    return mShared->pointerManager().handlePointerEvent(pointerEvent, mTimeManager->currentTime());
}

const RootConfig&
CoreRootContext::getRootConfig() const
{
    assert(mTopDocument);
    return mTopDocument->rootConfig();
}

std::string
CoreRootContext::getTheme() const
{
    assert(mTopDocument);
    return mTopDocument->getTheme();
}

const TextMeasurementPtr&
CoreRootContext::measure() const
{
    return mShared->measure();
}

ComponentPtr
CoreRootContext::findComponentById(const std::string& id) const
{
    assert(mTopDocument);

    // Fast path search for uid value
    auto *ptr = findByUniqueId(id);
    if (ptr && ptr->objectType() == UIDObject::UIDObjectType::COMPONENT)
        return static_cast<Component*>(ptr)->shared_from_this();

    // Depth-first search
    auto top = CoreComponent::cast(mTopDocument->topComponent());
    return top ? top->findComponentById(id, true) : nullptr;
}

UIDObject*
CoreRootContext::findByUniqueId(const std::string& uid) const
{
    assert(mTopDocument);
    // Traverse all documents
    auto result = mTopDocument->findByUniqueId(uid);
    if (result) return result;

    for (auto& document : mShared->documentRegistrar().list()) {
        result = document.second->findByUniqueId(uid);
        if (result) return result;
    }

    return nullptr;
}

std::map<std::string, Rect>
CoreRootContext::getFocusableAreas()
{
    assert(mShared);
    return mShared->focusManager().getFocusableAreas();
}

bool
CoreRootContext::setFocus(FocusDirection direction, const Rect& origin, const std::string& targetId)
{
    assert(mShared);
    assert(mTopDocument);
    auto top = mTopDocument->topComponent();
    auto target = CoreComponent::cast(findComponentById(targetId));

    if (!target) {
        LOG(LogLevel::kWarn) << "Don't have component: " << targetId;
        return false;
    }

    Rect targetRect;
    target->getBoundsInParent(top, targetRect);

    // Shift origin into target's coordinate space.
    auto offsetFocusRect = origin;
    offsetFocusRect.offset(-targetRect.getTopLeft());

    return mShared->focusManager().focus(direction, offsetFocusRect, target);
}

bool
CoreRootContext::nextFocus(FocusDirection direction, const Rect& origin)
{
    assert(mShared);
    return mShared->focusManager().focus(direction, origin);
}

bool
CoreRootContext::nextFocus(FocusDirection direction)
{
    assert(mShared);
    return mShared->focusManager().focus(direction);
}

void
CoreRootContext::clearFocus()
{
    assert(mShared);
    mShared->focusManager().clearFocus(false);
}

std::string
CoreRootContext::getFocused()
{
    assert(mShared);
    auto focused = mShared->focusManager().getFocus();
    return focused ? focused->getUniqueId() : "";
}

void
CoreRootContext::mediaLoaded(const std::string& source)
{
    assert(mShared);
    mShared->mediaManager().mediaLoadComplete(source, true, -1, std::string());
}

void
CoreRootContext::mediaLoadFailed(const std::string& source, int errorCode, const std::string& error)
{
    assert(mShared);
    mShared->mediaManager().mediaLoadComplete(source, false, errorCode, error);
}

Info
CoreRootContext::info() const
{
    return Info(mTopDocument->contextPtr(), mTopDocument->mCore);
}

/**
     * @return The top-level context.
 */
Context&
CoreRootContext::context() const
{
    return mTopDocument->context();
}

/**
     * @return The top-level context as a shared pointer
 */
ContextPtr
CoreRootContext::contextPtr() const
{
    return mTopDocument->contextPtr();
}

const ContentPtr&
CoreRootContext::content() const
{
    return mTopDocument->content();
}

bool
CoreRootContext::getAutoWidth() const
{
    return mTopDocument->mCore->metrics().getAutoWidth(); }

bool
CoreRootContext::getAutoHeight() const
{
    return mTopDocument->mCore->metrics().getAutoHeight();
}

#ifdef SCENEGRAPH
/*
 * If it does exist, we clean out any dirty markings for the scene graph, then walk
 * the list of dirty components and update the scene graph.  If the scene graph does not exist,
 * we create a new one.
 */
sg::SceneGraphPtr
CoreRootContext::getSceneGraph()
{
    assert(mTopDocument);

    // If we need a layout pass, do it now - this avoids screen flicker when a Text component
    // with "auto" width has a new, longer content but a full layout has not yet executed.
    if (mShared->layoutManager().needsLayout())
        mShared->layoutManager().layout(true, false);

    if (!mSceneGraph)
        mSceneGraph = sg::SceneGraph::create();

    if (mSceneGraph->getLayer()) {
        mSceneGraph->updates().clear();
        for (auto& component : getDirty())
            CoreComponent::cast(component)->updateSceneGraph(mSceneGraph->updates());
    } else {
        auto top = CoreComponent::cast(mTopDocument->topComponent());
        if (top)
            mSceneGraph->setLayer(top->getSceneGraph(mSceneGraph->updates()));
    }

    mSceneGraph->updates().fixCreatedFlags();
    clearDirty();
    return mSceneGraph;
}
#endif // SCENEGRAPH
} // namespace apl
