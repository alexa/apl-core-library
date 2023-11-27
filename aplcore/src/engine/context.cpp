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

#include "apl/engine/context.h"

#include <queue>

#include "apl/buildTimeConstants.h"
#include "apl/component/textmeasurement.h"
#include "apl/content/metrics.h"
#include "apl/content/rootconfig.h"
#include "apl/content/settings.h"
#include "apl/content/viewport.h"
#include "apl/document/documentcontextdata.h"
#include "apl/engine/builder.h"
#include "apl/engine/dependantmanager.h"
#include "apl/engine/event.h"
#include "apl/engine/resources.h"
#include "apl/engine/sharedcontextdata.h"
#include "apl/engine/styles.h"
#include "apl/extension/extensionmanager.h"
#include "apl/primitives/functions.h"
#include "apl/utils/session.h"

namespace apl {

class EvaluationContextData : public ContextData {
public:
    EvaluationContextData(
        const RootConfig& config,
        RuntimeState runtimeState,
        const SettingsPtr& settings,
        const SessionPtr& session)
        : ContextData(config, std::move(runtimeState), settings, "", kLayoutDirectionInherit),
          mSession(session)
    {}

    const SessionPtr& session() const override { return mSession; }
    bool embedded() const override { return false; }

private:
    SessionPtr mSession;
};

inline ContextDataPtr createEvaluationContextData(
    const RootConfig& config,
    const std::string& aplVersion,
    const std::string& theme,
    const SessionPtr& session)
{
    auto contextData = std::make_shared<EvaluationContextData>(
        config,
        RuntimeState(theme, aplVersion, false),
        std::make_shared<Settings>(),
        session);

    auto layoutDirection = static_cast<LayoutDirection>(config.getProperty(RootProperty::kLayoutDirection).asInt());
    contextData->lang(config.getProperty(RootProperty::kLang).asString())
        .layoutDirection(layoutDirection);

    return contextData;
}

inline ContextDataPtr createTestContextData(
    const Metrics& metrics,
    const RootConfig& config,
    const std::string& theme,
    const SessionPtr& session)
{
    auto sharedContext = std::make_shared<SharedContextData>(config);
    auto contextData = std::make_shared<DocumentContextData>(nullptr,
                                                 metrics,
                                                 config,
                                                 RuntimeState(
                                                    theme,
                                                    config.getProperty(RootProperty::kReportedVersion).getString(),
                                                    false),
                                                 std::make_shared<Settings>(),
                                                 session,
                                                 std::vector<ExtensionRequest>(),
                                                 sharedContext);

    auto layoutDirection = static_cast<LayoutDirection>(config.getProperty(RootProperty::kLayoutDirection).asInt());
    contextData->lang(config.getProperty(RootProperty::kLang).asString())
        .layoutDirection(layoutDirection);

    return contextData;
}

// Use this to create a free-standing context.  Used for testing
ContextPtr
Context::createTestContext(const Metrics& metrics, const RootConfig& config, const SessionPtr& session)
{
    auto contextData = createTestContextData(metrics, config, metrics.getTheme(), session);
    auto contextPtr = std::make_shared<Context>(metrics, contextData);
    createStandardFunctions(*contextPtr);
    return contextPtr;
}

// Use this to create a free-standing context.  Only used for testing
ContextPtr
Context::createTestContext(const Metrics& metrics, const RootConfig& config)
{
    return createTestContext(metrics, config, makeDefaultSession());
}

// Use this to create a free-standing context.  Used for testing
ContextPtr
Context::createTestContext(const Metrics& metrics, const SessionPtr& session)
{
    auto config = RootConfig();
    return createTestContext(metrics, config, session);
}

// Use this to create a free-standing context.  Used for type conversion and basic environment access.
// This method should never add custom enviroment properties to the newly created context as it is
// also used to detect collisions with the built-in variables.
ContextPtr
Context::createTypeEvaluationContext(const RootConfig& config, const std::string& aplVersion, const SessionPtr& session)
{
    auto metrics = Metrics();
    auto contextData = createEvaluationContextData(config, aplVersion, metrics.getTheme(), session);
    auto contextPtr = std::make_shared<Context>(metrics, contextData);
    createStandardFunctions(*contextPtr);
    return contextPtr;
}

ContextPtr
Context::createContentEvaluationContext(
    const Metrics& metrics,
    const RootConfig& config,
    const std::string& aplVersion,
    const std::string& theme,
    const SessionPtr& session)
{
    auto contextData = createEvaluationContextData(config, aplVersion, theme, session);
    return std::make_shared<Context>(metrics, contextData);
}

ContextPtr
Context::createRootEvaluationContext(const Metrics& metrics, const ContextDataPtr& core)
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

DocumentContextDataPtr documentContextData(const ContextDataPtr& data) {
    assert(data && data->fullContext());
    return std::static_pointer_cast<DocumentContextData>(data);
}

void
Context::init(const Metrics& metrics, const ContextDataPtr& core)
{
    auto env = std::make_shared<ObjectMap>();
    auto& config = core->rootConfig();
    env->emplace("agentName", config.getProperty(RootProperty::kAgentName).getString());
    env->emplace("agentVersion", config.getProperty(RootProperty::kAgentVersion).getString());
    env->emplace("allowOpenURL", config.getProperty(RootProperty::kAllowOpenUrl).getBoolean());
    env->emplace("animation", config.getAnimationQualityString());
    env->emplace("aplVersion", config.getProperty(RootProperty::kReportedVersion).getString());
    env->emplace("disallowDialog", config.getProperty(RootProperty::kDisallowDialog).getBoolean());
    env->emplace("disallowEditText", config.getProperty(RootProperty::kDisallowEditText).getBoolean());
    env->emplace("disallowVideo", config.getProperty(RootProperty::kDisallowVideo).getBoolean());
    if (core->fullContext()) {
        env->emplace("extension", documentContextData(core)->extensionManager().getEnvironment());
    }
    env->emplace("fontScale", config.getProperty(RootProperty::kFontScale).getDouble());
    env->emplace("lang", core->getLang());
    env->emplace("layoutDirection", sLayoutDirectionMap.get(core->getLayoutDirection(), ""));
    env->emplace("screenMode", config.getScreenMode());
    env->emplace("screenReader", config.getProperty(RootProperty::kScreenReader).getBoolean());
    env->emplace("reason", core->getReinflationFlag() ? "reinflation" : "initial");
    env->emplace("documentAPLVersion", core->getRequestedAPLVersion());

    auto timing = std::make_shared<ObjectMap>();
    timing->emplace("doublePressTimeout", config.getProperty(RootProperty::kDoublePressTimeout).getDouble());
    timing->emplace("longPressTimeout", config.getProperty(RootProperty::kLongPressTimeout).getDouble());
    timing->emplace("minimumFlingVelocity", config.getProperty(RootProperty::kMinimumFlingVelocity).getDouble());
    timing->emplace("pressedDuration", config.getProperty(RootProperty::kPressedDuration).getDouble());
    timing->emplace("tapOrScrollTimeout", config.getProperty(RootProperty::kTapOrScrollTimeout).getDouble());
    timing->emplace("maximumTapVelocity", config.getProperty(RootProperty::kMaximumTapVelocity).getInteger());
    env->emplace("timing", timing);

    env->emplace("_coreRepositoryVersion", sCoreRepositoryVersion);
    for (const auto& m : config.getEnvironmentValues())
        env->emplace(m.first, m.second);
    putConstant("environment", env);
    putConstant("viewport", makeViewport(metrics, core->getTheme()));
}

Context::Context( const Metrics& metrics, const ContextDataPtr& core )
    : mCore(core)
{
    assert(mCore);
    init(metrics, core);
}

double
Context::vwToDp(double vw) const
{
    return documentContextData(mCore)->getWidth() * vw / 100;
}

double
Context::vhToDp(double vh) const
{
    return documentContextData(mCore)->getHeight() * vh / 100;
}

double
Context::pxToDp(double px) const
{
    return documentContextData(mCore)->getPxToDp() * px;
}

double
Context::dpToPx(double dp) const
{
    return dp / documentContextData(mCore)->getPxToDp();
}

double
Context::width() const
{
    return documentContextData(mCore)->getWidth();
}

double
Context::height() const
{
    return documentContextData(mCore)->getHeight();
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
    return documentContextData(mCore)->styles()->get(shared_from_this(), name, state);
}

const JsonResource
Context::getLayout(const std::string& name) const
{
    const auto& layouts = documentContextData(mCore)->layouts();
    auto it = layouts.find(name);
    if (it != layouts.end())
        return it->second;
    return JsonResource();
}

const JsonResource
Context::getCommand(const std::string& name) const
{
    const auto& commands = documentContextData(mCore)->commands();
    auto it = commands.find(name);
    if (it != commands.end())
        return it->second;
    return JsonResource();
}

const JsonResource
Context::getGraphic(const std::string& name) const
{
    const auto& graphics = documentContextData(mCore)->graphics();
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
    return mCore->getRequestedAPLVersion();
}

int
Context::getDpi() const
{
    return documentContextData(mCore)->getDpi();
}

ScreenShape
Context::getScreenShape() const
{
    return documentContextData(mCore)->getScreenShape();
}

SharedContextDataPtr
Context::getShared() const
{
    return documentContextData(mCore)->getShared();
}

ViewportMode
Context::getViewportMode() const
{
    return documentContextData(mCore)->getViewportMode();
}

std::shared_ptr<Styles>
Context::styles() const {
    return documentContextData(mCore)->styles();
}

ComponentPtr
Context::findComponentById(const std::string& id, bool traverseHost) const
{
    auto top = documentContextData(mCore)->top();
    return top ? top->findComponentById(id, traverseHost) : nullptr;
}

ComponentPtr
Context::topComponent() const
{
    return documentContextData(mCore)->top();
}

void
Context::pushEvent(Event&& event)
{
    documentContextData(mCore)->pushEvent(std::move(event));
}

#ifdef ALEXAEXTENSIONS
void
Context::pushExtensionEvent(Event&& event)
{
    documentContextData(mCore)->getExtensionEvents().push(event);
}
#endif

void
Context::setDirty(const ComponentPtr& ptr)
{
    documentContextData(mCore)->setDirty(ptr);
}

void
Context::clearDirty(const ComponentPtr& ptr)
{
    documentContextData(mCore)->clearDirty(ptr);
}

void
Context::setDirtyVisualContext(const ComponentPtr& ptr)
{
    documentContextData(mCore)->getDirtyVisualContext().emplace(ptr);
}

bool
Context::isVisualContextDirty(const ComponentPtr& ptr)
{
    auto found = documentContextData(mCore)->getDirtyVisualContext().find(ptr);
    return found != documentContextData(mCore)->getDirtyVisualContext().end();
}

void
Context::setDirtyDataSourceContext(const DataSourceConnectionPtr& ptr)
{
    documentContextData(mCore)->getDirtyDatasourceContext().emplace(ptr);
}

Sequencer&
Context::sequencer() const
{
    return documentContextData(mCore)->sequencer();
}

FocusManager&
Context::focusManager() const
{
    return documentContextData(mCore)->focusManager();
}

HoverManager&
Context::hoverManager() const
{
    return documentContextData(mCore)->hoverManager();
}


LiveDataManager&
Context::dataManager() const
{
    return documentContextData(mCore)->dataManager();
}

ExtensionManager&
Context::extensionManager() const
{
    return documentContextData(mCore)->extensionManager();
}

LayoutManager&
Context::layoutManager() const
{
    return documentContextData(mCore)->layoutManager();
}

MediaManager&
Context::mediaManager() const
{
    return documentContextData(mCore)->mediaManager();
}

MediaPlayerFactory&
Context::mediaPlayerFactory() const
{
    return documentContextData(mCore)->mediaPlayerFactory();
}

UIDManager&
Context::uniqueIdManager() const
{
    return documentContextData(mCore)->uniqueIdManager();
}

DependantManager&
Context::dependantManager() const
{
    return documentContextData(mCore)->dependantManager();
}

const SessionPtr&
Context::session() const
{
    return mCore->session();
}

YGConfigRef
Context::ygconfig() const
{
    return documentContextData(mCore)->ygconfig();
}

const TextMeasurementPtr&
Context::measure() const
{
    return documentContextData(mCore)->measure();
}

void Context::takeScreenLock() const
{
    documentContextData(mCore)->takeScreenLock();
}

void Context::releaseScreenLock() const
{
    documentContextData(mCore)->releaseScreenLock();
}

LruCache<TextMeasureRequest, YGSize>&
Context::cachedMeasures()
{
    return documentContextData(mCore)->cachedMeasures();
}

LruCache<TextMeasureRequest, float>&
Context::cachedBaselines()
{
    return documentContextData(mCore)->cachedBaselines();
}

WeakPtrSet<CoreComponent>&
Context::pendingOnMounts()
{
    return documentContextData(mCore)->pendingOnMounts();
}

bool
Context::embedded() const
{
    return documentContextData(mCore)->embedded();
}

DocumentContextPtr
Context::documentContext() const
{
    return documentContextData(mCore)->documentContext();
}

#ifdef SCENEGRAPH
sg::TextPropertiesCache&
Context::textPropertiesCache() const
{
    return documentContextData(mCore)->textPropertiesCache();
}
#endif // SCENEGRAPH

ComponentPtr
Context::inflate(const rapidjson::Value& component)
{
    if (!component.IsObject())
        return nullptr;

    return Builder().inflate(shared_from_this(), component);
}

rapidjson::Value
Context::serialize(rapidjson::Document::AllocatorType& allocator)
{
    rapidjson::Value out(rapidjson::kArrayType);

    for (const auto& m : mMap) {
        rapidjson::Value entry(rapidjson::kObjectType);
        entry.AddMember("name", rapidjson::StringRef(m.first.c_str()), allocator);
        entry.AddMember("prov",
                        rapidjson::Value(m.second.provenance().toString().c_str(), allocator),
                        allocator);
        entry.AddMember("value", m.second.value().serialize(allocator), allocator);
        entry.AddMember("perm",
                        rapidjson::StringRef(m.second.isUserWriteable()
                                                 ? "write"
                                                 : (m.second.isMutable() ? "mutable" : "const")),
                        allocator);
        out.PushBack(entry, allocator);
    }

    return out;
}

streamer&
operator<<(streamer& os, const Context& context)
{
    for (const auto & it : context.mMap) {
        os << it.first << ": " << it.second << "\n";
    }

    if (context.mParent)
        os << *(context.mParent);

    return os;
}


bool
Context::userUpdateAndRecalculate(const std::string& key, const Object& value, bool useDirtyFlag)
{
    auto it = mMap.find(key);
    if (it != mMap.end()) {
        if (it->second.isUserWriteable()) {
            removeUpstream(key);  // Break any dependency chain
            if (it->second.set(value)) { // If the value changes, recalculate downstream values
                enqueueDownstream(key);
                dependantManager().processDependencies(useDirtyFlag);
            }
        } else {
            CONSOLE(mCore->session()) << "Data-binding field '" << key << "' is read-only";
        }

        return true;
    }

    if (mParent)
        return mParent->userUpdateAndRecalculate(key, value, useDirtyFlag);

    return false;
}

bool
Context::systemUpdateAndRecalculate(const std::string& key, const Object& value, bool useDirtyFlag)
{
    auto it = mMap.find(key);
    if (it == mMap.end())
        return false;

    if (it->second.isMutable()) {
        removeUpstream(key);  // Break any dependency chain
        if (it->second.set(value)) { // If the value changes, recalculate downstream values
            enqueueDownstream(key);
            dependantManager().processDependencies(useDirtyFlag);
        }
    }

    return true;
}

}  // namespace apl
