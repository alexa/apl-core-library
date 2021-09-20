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

#include <queue>

#include "apl/buildTimeConstants.h"
#include "apl/component/textmeasurement.h"
#include "apl/content/metrics.h"
#include "apl/content/rootconfig.h"
#include "apl/content/viewport.h"
#include "apl/engine/builder.h"
#include "apl/engine/context.h"
#include "apl/engine/event.h"
#include "apl/engine/resources.h"
#include "apl/engine/rootcontextdata.h"
#include "apl/engine/styles.h"
#include "apl/primitives/functions.h"
#include "apl/primitives/object.h"
#include "apl/utils/log.h"
#include "apl/utils/lrucache.h"
#include "apl/utils/session.h"

namespace apl {

// Use this to create a free-standing context.  Used for testing
ContextPtr
Context::createTestContext(const Metrics& metrics, const SessionPtr& session)
{
    auto config = RootConfig().session(session);
    auto contextPtr = std::make_shared<Context>(metrics, config, metrics.getTheme());
    createStandardFunctions(*contextPtr);
    return contextPtr;
}

// Use this to create a free-standing context.  Only used for testing
ContextPtr
Context::createTestContext(const Metrics& metrics, const RootConfig& config)
{
    auto contextPtr = std::make_shared<Context>(metrics, config, metrics.getTheme());
    createStandardFunctions(*contextPtr);
    return contextPtr;
}

// Use this to create a free-standing context.  Used for type conversion and basic environment access.
ContextPtr
Context::createTypeEvaluationContext(const RootConfig& config)
{
    auto metrics = Metrics();
    auto contextPtr = std::make_shared<Context>(metrics, config, metrics.getTheme());
    createStandardFunctions(*contextPtr);
    return contextPtr;
}

// Use this to create a free-standing context.  Only used for background extraction
ContextPtr
Context::createBackgroundEvaluationContext(const Metrics& metrics, const RootConfig& config, const std::string& theme)
{
    return std::make_shared<Context>(metrics, config, theme);
}

ContextPtr
Context::createRootEvaluationContext(const Metrics& metrics, const std::shared_ptr<RootContextData>& core)
{
    auto contextPtr =  std::make_shared<Context>(metrics, core);
    createStandardFunctions(*contextPtr);
    return contextPtr;
}

ContextPtr
Context::createClean(const ContextPtr& other)
{
    auto context = other->top();
    return std::make_shared<Context>(context);
}

void
Context::init(const Metrics& metrics, const std::shared_ptr<RootContextData>& core)
{
    auto env = std::make_shared<ObjectMap>();
    auto& config = core->rootConfig();
    env->emplace("agentName", config.getAgentName());
    env->emplace("agentVersion", config.getAgentVersion());
    env->emplace("allowOpenURL", config.getAllowOpenUrl());
    env->emplace("animation", config.getAnimationQualityString());
    env->emplace("aplVersion", config.getReportedAPLVersion());
    env->emplace("disallowVideo", config.getDisallowVideo());
    env->emplace("extension", core->extensionManager().getEnvironment());
    env->emplace("fontScale", config.getFontScale());
    env->emplace("lang", core->getLang());
    env->emplace("layoutDirection", sLayoutDirectionMap.get(core->getLayoutDirection(), ""));
    env->emplace("screenMode", config.getScreenMode());
    env->emplace("screenReader", config.getScreenReaderEnabled());
    env->emplace("reason", core->getReinflationFlag() ? "reinflation" : "initial");

    auto timing = std::make_shared<ObjectMap>();
    timing->emplace("doublePressTimeout", config.getDoublePressTimeout());
    timing->emplace("longPressTimeout", config.getLongPressTimeout());
    timing->emplace("minimumFlingVelocity", config.getMinimumFlingVelocity());
    timing->emplace("pressedDuration", config.getPressedDuration());
    timing->emplace("tapOrScrollTimeout", config.getTapOrScrollTimeout());
    env->emplace("timing", timing);

    env->emplace("_coreRepositoryVersion", sCoreRepositoryVersion);
    for (const auto& m : config.getEnvironmentValues())
        env->emplace(m.first, m.second);
    putConstant("environment", env);
    putConstant("viewport", makeViewport(metrics, core->getTheme()));
}

Context::Context( const Metrics& metrics, const std::shared_ptr<RootContextData>& core )
    : mCore(core)
{
    init(metrics, core);
}

Context::Context(const Metrics& metrics, const RootConfig& config, const std::string& theme)
{
    auto session = config.getSession() ? config.getSession() : makeDefaultSession();
    mCore = std::make_shared<RootContextData>(metrics,
                                              config,
                                              RuntimeState(theme, config.getReportedAPLVersion(),
                                                           false),
                                              std::make_shared<Settings>(), session,
                                              std::vector<std::pair<std::string, std::string>>());

    auto layoutDirection = static_cast<LayoutDirection>(config.getProperty(RootProperty::kLayoutDirection).asInt());
    mCore->lang(config.getProperty(RootProperty::kLang).asString())
          .layoutDirection(layoutDirection);

    init(metrics, mCore);
}

double
Context::vwToDp(double vw) const
{
    assert(mCore);
    return mCore->getWidth() * vw / 100;
}

double
Context::vhToDp(double vh) const
{
    assert(mCore);
    return mCore->getHeight() * vh / 100;
}

double
Context::pxToDp(double px) const
{
    assert(mCore);
    return mCore->getPxToDp() * px;
}

double
Context::width() const
{
    assert(mCore);
    return mCore->getWidth();
}

double
Context::height() const
{
    assert(mCore);
    return mCore->getHeight();
}

const RootConfig&
Context::getRootConfig() const
{
    assert(mCore);
    return mCore->rootConfig();
}

StyleInstancePtr
Context::getStyle(const std::string& name, const State& state)
{
    assert(mCore);
    return mCore->styles()->get(shared_from_this(), name, state);
}

const JsonResource
Context::getLayout(const std::string& name) const
{
    assert(mCore);
    const auto& layouts = mCore->layouts();
    auto it = layouts.find(name);
    if (it != layouts.end())
        return it->second;
    return JsonResource();
}

const JsonResource
Context::getCommand(const std::string& name) const
{
    assert(mCore);
    const auto& commands = mCore->commands();
    auto it = commands.find(name);
    if (it != commands.end())
        return it->second;
    return JsonResource();
}

const JsonResource
Context::getGraphic(const std::string& name) const
{
    assert(mCore);

    const auto& graphics = mCore->graphics();
    auto it = graphics.find(name);
    if (it != graphics.end())
        return it->second;
    return JsonResource();
}

std::string
Context::getTheme() const
{
    assert(mCore);
    return mCore->getTheme();
}

std::string
Context::getLang() const
{
    assert(mCore);
    return mCore->getLang();
}

LayoutDirection
Context::getLayoutDirection() const
{
    assert(mCore);
    return mCore->getLayoutDirection();
}

std::shared_ptr<LocaleMethods>
Context::getLocaleMethods() const
{
    assert(mCore);
    return mCore->rootConfig().getLocaleMethods();
}

bool
Context::getReinflationFlag() const
{
    assert(mCore);
    return mCore->getReinflationFlag();
}

std::string
Context::getRequestedAPLVersion() const
{
    assert(mCore);
    return mCore->getRequestedAPLVersion();
}

std::shared_ptr<Styles>
Context::styles() const {
    assert(mCore);
    return mCore->styles();
}

ComponentPtr
Context::findComponentById(const std::string& id) const
{
    assert(mCore);

    auto top = mCore->top();
    return top ? top->findComponentById(id) : nullptr;
}

void
Context::pushEvent(Event&& event) {
    assert(mCore);
    mCore->events.push(event);
}

#ifdef ALEXAEXTENSIONS
void
Context::pushExtensionEvent(Event&& event)
{
    assert(mCore);
    mCore->extesnionEvents.push(event);
}
#endif

void
Context::setDirty(const ComponentPtr& ptr) {
    assert(mCore);
    mCore->dirty.emplace(ptr);
}

void
Context::clearDirty(const ComponentPtr& ptr)
{
    assert(mCore);
    mCore->dirty.erase(ptr);
}

void
Context::setDirtyVisualContext(const ComponentPtr& ptr) {
    assert(mCore);
    mCore->dirtyVisualContext.emplace(ptr);
}

bool
Context::isVisualContextDirty(const ComponentPtr& ptr) {
    auto found = mCore->dirtyVisualContext.find(ptr);
    return found != mCore->dirtyVisualContext.end();
}

void
Context::setDirtyDataSourceContext(const DataSourceConnectionPtr& ptr) {
    assert(mCore);
    mCore->dirtyDatasourceContext.emplace(ptr);
}

Sequencer&
Context::sequencer() const
{
    return mCore->sequencer();
}

FocusManager&
Context::focusManager() const
{
    return mCore->focusManager();
}

HoverManager&
Context::hoverManager() const {
    return mCore->hoverManager();
}

KeyboardManager &
Context::keyboardManager() const {
    return mCore->keyboardManager();
}

LiveDataManager&
Context::dataManager() const {
    return mCore->dataManager();
}

ExtensionManager&
Context::extensionManager() const {
    return mCore->extensionManager();
}

LayoutManager&
Context::layoutManager() const {
    return mCore->layoutManager();
}

MediaManager&
Context::mediaManager() const {
    return mCore->mediaManager();
}

MediaPlayerFactory&
Context::mediaPlayerFactory() const {
    return mCore->mediaPlayerFactory();
}

const SessionPtr&
Context::session() const {
    return mCore->session();
}

YGConfigRef
Context::ygconfig() const
{
    return mCore->ygconfig();
}

const TextMeasurementPtr&
Context::measure() const
{
    return mCore->measure();
}

void Context::takeScreenLock() const
{
    mCore->takeScreenLock();
}

void Context::releaseScreenLock() const
{
    mCore->releaseScreenLock();
}

LruCache<TextMeasureRequest, YGSize>&
Context::cachedMeasures()
{
    return mCore->cachedMeasures();
}

LruCache<TextMeasureRequest, float>&
Context::cachedBaselines()
{
    return mCore->cachedBaselines();
}

WeakPtrSet<CoreComponent>&
Context::pendingOnMounts()
{
    return mCore->pendingOnMounts();
}

ComponentPtr
Context::inflate(const rapidjson::Value& component)
{
    if (!component.IsObject())
        return nullptr;

    return Builder().inflate(shared_from_this(), component);
}

streamer&
operator<<(streamer& os, const Context& context)
{
    for (auto it = context.mMap.begin() ; it != context.mMap.end() ; it++) {
        os << it->first << ": " << it->second << "\n";
    }

    if (context.mParent)
        os << *(context.mParent);

    return os;
}


bool Context::userUpdateAndRecalculate(const std::string& key, const Object& value, bool useDirtyFlag) {
    auto it = mMap.find(key);
    if (it != mMap.end()) {
        if (it->second.isUserWriteable()) {
            removeUpstream(key);  // Break any dependency chain
            if (it->second.set(value))  // If the value changes, recalculate downstream values
                recalculateDownstream(key, useDirtyFlag);
        } else {
            CONSOLE_S(mCore->session()) << "Data-binding field '" << key << "' is read-only";
        }

        return true;
    }

    if (mParent)
        return mParent->userUpdateAndRecalculate(key, value, useDirtyFlag);

    return false;
}

bool Context::systemUpdateAndRecalculate(const std::string& key, const Object& value, bool useDirtyFlag) {
    auto it = mMap.find(key);
    if (it == mMap.end())
        return false;

    if (it->second.isMutable()) {
        removeUpstream(key);  // Break any dependency chain
        if (it->second.set(value))  // If the value changes, recalculate downstream values
            recalculateDownstream(key, useDirtyFlag);
    }

    return true;
}

}  // namespace apl
