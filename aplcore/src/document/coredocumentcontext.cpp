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

#include "apl/document/coredocumentcontext.h"

#include "rapidjson/stringbuffer.h"

#include "apl/command/arraycommand.h"
#include "apl/command/configchangecommand.h"
#include "apl/command/displaystatechangecommand.h"
#include "apl/command/documentcommand.h"
#include "apl/datasource/datasource.h"
#include "apl/datasource/datasourceprovider.h"
#include "apl/engine/builder.h"
#include "apl/engine/corerootcontext.h"
#include "apl/engine/keyboardmanager.h"
#include "apl/engine/layoutmanager.h"
#include "apl/engine/resources.h"
#include "apl/engine/sharedcontextdata.h"
#include "apl/engine/styles.h"
#include "apl/engine/uidmanager.h"
#include "apl/extension/extensionmanager.h"
#include "apl/focus/focusmanager.h"
#include "apl/graphic/graphic.h"
#include "apl/livedata/livedatamanager.h"
#include "apl/livedata/livedataobjectwatcher.h"
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

static const char *DISPLAY_STATE = "displayState";
static const char *ELAPSED_TIME = "elapsedTime";
static const char *LOCAL_TIME = "localTime";
static const char *UTC_TIME = "utcTime";
static const char *ON_MOUNT_HANDLER_NAME = "Mount";

static const std::string MOUNT_SEQUENCER = "__MOUNT_SEQUENCER";

CoreDocumentContextPtr
CoreDocumentContext::create(const ContextPtr& context,
                            const ContentPtr& content,
                            const Object& env,
                            const Size& size,
                            const DocumentConfigPtr& documentConfig)
{
    const RootConfig& rootConfig = context->getRootConfig();
    const RootConfigPtr embeddedRootConfig = rootConfig.copy();
    embeddedRootConfig->set(RootProperty::kLang, env.opt("lang", context->getLang()));
    embeddedRootConfig->set(RootProperty::kLayoutDirection,
                            env.opt("layoutDirection", sLayoutDirectionMap.get(context->getLayoutDirection(), "")));

    embeddedRootConfig->set(RootProperty::kAllowOpenUrl,
                            rootConfig.getProperty(RootProperty::kAllowOpenUrl).getBoolean()
                                && env.opt("allowOpenURL", true).asBoolean());

    auto copyDisallowProp = [&]( RootProperty propName, const std::string& envName ) {
        embeddedRootConfig->set(propName, rootConfig.getProperty(propName).getBoolean() || env.opt(envName, false).asBoolean());
    };

    copyDisallowProp(RootProperty::kDisallowDialog, "disallowDialog");
    copyDisallowProp(RootProperty::kDisallowEditText, "disallowEditText");
    copyDisallowProp(RootProperty::kDisallowVideo, "disallowVideo");

    if (documentConfig != nullptr) {
#ifdef ALEXAEXTENSIONS
        embeddedRootConfig->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider);
        embeddedRootConfig->extensionMediator(documentConfig->getExtensionMediator());
#endif

        for (const auto& provider : documentConfig->getDataSourceProviders()) {
            embeddedRootConfig->dataSourceProvider(provider->getType(), provider);
        }
    }

    // std::lround use copied from Dimension::AbsoluteDimensionObjectType::asInt
    const int width = std::lround(context->dpToPx(size.getWidth()));
    const int height = std::lround(context->dpToPx(size.getHeight()));

    auto metrics = Metrics()
                       .size(width, height)
                       .shape(context->getScreenShape())
                       .theme(context->getTheme().c_str())
                       .dpi(context->getDpi())
                       .mode(context->getViewportMode());

    return CoreDocumentContext::create(context->getShared(), metrics, content, *embeddedRootConfig);
}

CoreDocumentContextPtr
CoreDocumentContext::create(
                    const SharedContextDataPtr& root,
                    const Metrics& metrics,
                    const ContentPtr& content,
                    const RootConfig& config)
{
    if (!content->isReady()) {
        LOG(LogLevel::kError).session(content->getSession()) << "Attempting to create root context with illegal content";
        return nullptr;
    }

    auto document = std::make_shared<CoreDocumentContext>(content, config);
    document->init(metrics, config, root, false);

    return document;
}

CoreDocumentContext::CoreDocumentContext(const ContentPtr& content, const RootConfig& config)
    : mContent(content),
      mDisplayState(static_cast<DisplayState>(config.getProperty(RootProperty::kInitialDisplayState).getInteger()))
{}

CoreDocumentContext::~CoreDocumentContext() {
    assert(mCore);
    mCore->terminate();
    mCore->getDirtyDatasourceContext().clear();
    mCore->getDirtyVisualContext().clear();
}

void
CoreDocumentContext::configurationChange(const ConfigurationChange& change)
{
    // If we're in the middle of a configuration change, drop it
    mCore->sequencer().terminateSequencer(ConfigChangeCommand::SEQUENCER);

    mActiveConfigurationChanges.mergeConfigurationChange(change);
    if (mActiveConfigurationChanges.empty())
        return;

    auto cmd = ConfigChangeCommand::create(shared_from_this(),
                                           mActiveConfigurationChanges.asEventProperties(mCore->rootConfig(),
                                                                                         mCore->metrics()));
    mContext->sequencer().executeOnSequencer(cmd, ConfigChangeCommand::SEQUENCER);
}

void
CoreDocumentContext::updateDisplayState(DisplayState displayState)
{
    if (!sDisplayStateMap.has(displayState)) {
        LOG(LogLevel::kWarn).session(getSession()) << "View specified an invalid display state, ignoring it";
        return;
    }

    if (displayState == mDisplayState) return;

    // If we're in the middle of a display state change, drop it
    mCore->sequencer().terminateSequencer(DisplayStateChangeCommand::SEQUENCER);

    mDisplayState = displayState;

    const auto& displayStateString = sDisplayStateMap.at(displayState);
    mContext->systemUpdateAndRecalculate(DISPLAY_STATE, displayStateString, true);

    ObjectMap properties;
    properties.emplace("displayState", displayStateString);

    auto cmd = DisplayStateChangeCommand::create(shared_from_this(), std::move(properties));
    mContext->sequencer().executeOnSequencer(cmd, DisplayStateChangeCommand::SEQUENCER);

#ifdef ALEXAEXTENSIONS
    auto mediator = getRootConfig().getExtensionMediator();
    if (mediator) {
        mediator->onDisplayStateChanged(mDisplayState);
    }
#endif
}

bool
CoreDocumentContext::reinflate(const LayoutCallbackFunc& layoutCallback)
{
    // The basic algorithm is to simply re-build CoreDocumentContexData and re-inflate the component hierarchy.
    // TODO: Re-use parts of the hierarchy and to maintain state during reinflation.
    mCore->sequencer().terminateSequencer(ConfigChangeCommand::SEQUENCER);

    auto preservedSequencers = std::map<std::string, ActionPtr>();
    for (auto& stp : mCore->sequencer().getSequencersToPreserve()) {
        preservedSequencers.emplace(stp, mCore->sequencer().detachSequencer(stp));
    }

    auto shared = mCore->mSharedData;
    if (!shared) return false;

    auto oldTop = mCore->halt();
    if (oldTop) {
        shared->layoutManager().removeAsTopNode(oldTop);
    }

    auto metrics = mActiveConfigurationChanges.mergeMetrics(mCore->mMetrics);
    auto config = mActiveConfigurationChanges.mergeRootConfig(mCore->mConfig);

    // Update the configuration with the current UTC time and time adjustment
    config.set(RootProperty::kUTCTime, mUTCTime);
    config.set(RootProperty::kLocalTimeAdjustment, mLocalTimeAdjustment);

    // The initialization routine replaces mCore with a new core
    init(metrics, config, shared, true);
    setup(oldTop);

    // If we are reinflating root document we need to re-setup.
    if (topComponent() && layoutCallback) {
        layoutCallback();
    }

    // If there was a previous top component, release it and its children to free memory
    if (oldTop)
        oldTop->release();

    // Clear the old active configuration; it is reset on a reinflation
    mActiveConfigurationChanges.clear();

    for (auto& ps : preservedSequencers) {
        if(!mCore->sequencer().reattachSequencer(ps.first, ps.second, *this)) {
            CONSOLE(getSession()) << "Can't preserve sequencer: " << ps.first;
        }
    }

    return topComponent() != nullptr;
}

void
CoreDocumentContext::resize()
{
    // Release any "onConfigChange" action
    mCore->sequencer().terminateSequencer(ConfigChangeCommand::SEQUENCER);
    mCore->layoutManager().configChange(mActiveConfigurationChanges, shared_from_this());
    // Note: we do not clear the configuration changes - there may be a reinflate() coming in the future.
}


void
CoreDocumentContext::init(const Metrics& metrics,
                          const RootConfig& config,
                          const SharedContextDataPtr& sharedData,
                          bool reinflation)
{
    APL_TRACE_BLOCK("DocumentContext:init");
    std::string theme = metrics.getTheme();
    const auto& json = mContent->getDocument()->json();
    auto themeIter = json.FindMember("theme");
    if (themeIter != json.MemberEnd() && themeIter->value.IsString())
        theme = themeIter->value.GetString();

    mCore = std::make_shared<DocumentContextData>(shared_from_this(),
                                              metrics,
                                              config,
                                              RuntimeState(theme,
                                                           mContent->getDocument()->version(),
                                                           reinflation),
                                              mContent->getDocumentSettings(),
                                              mContent->getSession(),
                                              mContent->mExtensionRequests,
                                              sharedData);

    auto env = mContent->getEnvironment(config);
    mCore->lang(env.language).layoutDirection(env.layoutDirection);

    mContext = Context::createRootEvaluationContext(metrics, mCore);
    mContext->putSystemWriteable(ELAPSED_TIME, config.getTimeManager()->currentTime());
    mContext->putSystemWriteable(DISPLAY_STATE, sDisplayStateMap.at(mDisplayState));

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

#ifdef ALEXAEXTENSIONS
    // Get all known extension clients, via legacy pathway and mediator
    auto clients = config.getLegacyExtensionClients();
    auto extensionMediator = config.getExtensionMediator();
    if (extensionMediator) {
        const auto& mediatorClients = extensionMediator->getClients();
        clients.insert(mediatorClients.begin(), mediatorClients.end());
    }
    // Insert extension-defined live data
    for (auto& client : clients) {
        const auto& schema = client.second->extensionSchema();
        for (const auto& m : schema.liveData) {
            auto ldo = LiveDataObject::create(m.second, mContext, m.first);
            client.second->registerObjectWatcher(ldo);
        }
    }
#endif
}

void
CoreDocumentContext::clearPending() const
{
    assert(mCore);

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
        mCore->sequencer().attachToSequencer(mountAction, MOUNT_SEQUENCER);
    }

#ifdef ALEXAEXTENSIONS
    // Process any extension events. There are no need to expose those externally.
    auto extensionMediator = mCore->rootConfig().getExtensionMediator();
    if (extensionMediator) {
        while (!mCore->getExtensionEvents().empty()) {
            Event event = mCore->getExtensionEvents().front();
            mCore->getExtensionEvents().pop();
            extensionMediator->invokeCommand(event);
        }
    }
#endif
}

bool
CoreDocumentContext::isVisualContextDirty() const
{
    assert(mCore);
    return !mCore->getDirtyVisualContext().empty();
}

void
CoreDocumentContext::clearVisualContextDirty()
{
    assert(mCore);
    mCore->getDirtyVisualContext().clear();
}

rapidjson::Value
CoreDocumentContext::serializeVisualContext(rapidjson::Document::AllocatorType& allocator)
{
    clearVisualContextDirty();
    return mCore->mTop->serializeVisualContext(allocator);
}

bool
CoreDocumentContext::isDataSourceContextDirty() const
{
    assert(mCore);
    return !mCore->getDirtyDatasourceContext().empty();
}

void
CoreDocumentContext::clearDataSourceContextDirty()
{
    assert(mCore);
    mCore->getDirtyDatasourceContext().clear();
}

rapidjson::Value
CoreDocumentContext::serializeDataSourceContext(rapidjson::Document::AllocatorType& allocator)
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

rapidjson::Value
CoreDocumentContext::serializeDOM(bool extended, rapidjson::Document::AllocatorType& allocator)
{
    if (extended)
        return mCore->mTop->serializeAll(allocator);
    return mCore->mTop->serialize(allocator);
}

rapidjson::Value
CoreDocumentContext::serializeContext(rapidjson::Document::AllocatorType& allocator)
{
    return mContext->serialize(allocator);
}


std::shared_ptr<ObjectMap>
CoreDocumentContext::createDocumentEventProperties(const std::string& handler) const
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
CoreDocumentContext::createDocumentContext(const std::string& handler, const ObjectMap& optional)
{
    ContextPtr ctx = Context::createFromParent(payloadContext());
    auto event = createDocumentEventProperties(handler);
    for (const auto& m : optional)
        event->emplace(m.first, m.second);
    ctx->putConstant("event", event);
    return ctx;
}


ActionPtr
CoreDocumentContext::executeCommands(const apl::Object& commands, bool fastMode)
{
    ContextPtr ctx = createDocumentContext("External");
    return mContext->sequencer().executeCommands(commands, ctx, nullptr, fastMode);
}

ActionPtr
CoreDocumentContext::invokeExtensionEventHandler(const std::string& uri, const std::string& name,
                                         const ObjectMap& data, bool fastMode,
                                         std::string resourceId)
{
    auto handlerDefinition = ExtensionEventHandler{uri, name};
    auto handler = Object::NULL_OBJECT();
    ContextPtr ctx = nullptr;
    auto comp = mCore->extensionManager().findExtensionComponent(resourceId);
    if (comp) {
        handler = comp->findHandler(handlerDefinition);
        if (handler.isNull()) {
            CONSOLE(getSession()) << "Extension Component " << comp->name()
                                            << " can't execute event handler " << handlerDefinition.getName();
            return nullptr;
        }

        // Create component context. Data is attached on event level.
        auto dataPtr = std::make_shared<ObjectMap>(data);
        ctx = comp->createEventContext(name, dataPtr);
    } else {
        handler = mCore->extensionManager().findHandler(handlerDefinition);
        if (handler.isNull()) {
            CONSOLE(getSession()) << "Extension Handler " << handlerDefinition.getName() << " don't exist.";
            return nullptr;
        }

        // Create a document-level context. Data is attached on event level.
        ctx = createDocumentContext(name, data);
    }

    for (const auto& m : data)
        ctx->putConstant(m.first, m.second);

    return mContext->sequencer().executeCommands(handler, ctx, comp, fastMode);
}

ComponentPtr
CoreDocumentContext::topComponent()
{
    return mCore->mTop;
}

ContextPtr
CoreDocumentContext::payloadContext() const
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

apl_time_t
CoreDocumentContext::currentTime()
{
    return mCore->mConfig.getTimeManager()->currentTime();
}

void
CoreDocumentContext::updateTime(apl_time_t utcTime, apl_duration_t localTimeAdjustment)
{
    mUTCTime = utcTime;
    mLocalTimeAdjustment = localTimeAdjustment;

    mContext->systemUpdateAndRecalculate(ELAPSED_TIME, currentTime(), true);
    mContext->systemUpdateAndRecalculate(UTC_TIME, mUTCTime, true);
    mContext->systemUpdateAndRecalculate(LOCAL_TIME, mUTCTime + mLocalTimeAdjustment, true);
}

const RootConfig&
CoreDocumentContext::rootConfig()
{
    return mCore->rootConfig();
}

bool
CoreDocumentContext::setup(const CoreComponentPtr& top)
{
    APL_TRACE_BLOCK("DocumentContext:setup");
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
        APL_TRACE_BEGIN("DocumentContext:readSettings");
        mCore->mSettings->read(mCore->rootConfig());
        APL_TRACE_END("DocumentContext:readSettings");
    }

    // Resource processing:
    APL_TRACE_BEGIN("DocumentContext:processResources");
    for (const auto& child : ordered) {
        const auto& json = child->json();
        const auto path = Path(trackProvenance ? child->name() : std::string());
        addNamedResourcesBlock(*mContext, json, path, "resources");
    }
    APL_TRACE_END("DocumentContext:processResources");

    // Style processing
    APL_TRACE_BEGIN("DocumentContext:processStyles");
    for (const auto& child : ordered) {
        const auto& json = child->json();
        const auto path = Path(trackProvenance ? child->name() : std::string());

        auto styleIter = json.FindMember("styles");
        if (styleIter != json.MemberEnd() && styleIter->value.IsObject())
            mCore->styles()->addStyleDefinitions(mCore->session(), &styleIter->value, path.addObject("styles"));
    }
    APL_TRACE_END("DocumentContext:processStyles");

    // Layout processing
    APL_TRACE_BEGIN("DocumentContext:processLayouts");
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
    APL_TRACE_END("DocumentContext:processLayouts");

    // Command processing
    APL_TRACE_BEGIN("DocumentContext:processCommands");
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
    APL_TRACE_END("DocumentContext:processCommands");

    // Graphics processing
    APL_TRACE_BEGIN("DocumentContext:processGraphics");
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
    APL_TRACE_END("DocumentContext:processGraphics");

    // Identify all registered event handlers in all ordered documents
    APL_TRACE_BEGIN("DocumentContext:processExtensionHandlers");
    auto& em = mCore->extensionManager();
    for (const auto& handler : em.getEventHandlerDefinitions()) {
        for (const auto& child : ordered) {
            const auto& json = child->json();
            auto h = json.FindMember(handler.first.c_str());
            if (h != json.MemberEnd()) {
                auto oldHandler = em.findHandler(handler.second);
                if (!oldHandler.isNull())
                    CONSOLE(mContext) << "Overwriting existing command handler " << handler.first;
                em.addEventHandler(handler.second, asCommand(*mContext, evaluate(*mContext, h->value)));
            }
        }
    }
    APL_TRACE_END("DocumentContext:processExtensionHandlers");

    // Inflate the top component
    Properties properties;

    APL_TRACE_BEGIN("DocumentContext:retrieveProperties");
    mContent->getMainProperties(properties);
    APL_TRACE_END("DocumentContext:retrieveProperties");

    mCore->mTop = Builder(top).inflate(mContext, properties, mContent->getMainTemplate());

    if (!mCore->mTop)
        return false;

    mCore->mTop->markGlobalToLocalTransformStale();

#ifdef ALEXAEXTENSIONS
    // Bind to the extension mediator
    // TODO ExtensionMediator is an experimental class facilitating message passing to and from extensions.
    // TODO The mediator class should be replaced by direct messaging between extensions and ExtensionManager
    auto extensionMediator = mCore->rootConfig().getExtensionMediator();
    if (extensionMediator) {
        extensionMediator->bindContext(shared_from_this());
    }
#endif

    return true;
}

bool
CoreDocumentContext::verifyAPLVersionCompatibility(const std::vector<PackagePtr>& ordered,
                                           const APLVersion& compatibilityVersion)
{
    for(const auto& child : ordered) {
        if(!compatibilityVersion.isValid(child->version())) {
            CONSOLE(mContext) << child->name() << " has invalid version: " << child->version();
            return false;
        }
    }
    return true;
}

bool
CoreDocumentContext::verifyTypeField(const std::vector<std::shared_ptr<Package>>& ordered, bool enforce)
{
    for(auto& child : ordered) {
        auto type = child->type();
        if (type.compare("APML") == 0) CONSOLE(mContext)
                    << child->name() << ": Stop using the APML document format!";
        else if (type.compare("APL") != 0) {
            CONSOLE(mContext) << child->name() << ": Document type field should be \"APL\"!";
            if(enforce) {
                return false;
            }
        }
    }
    return true;
}


streamer&
operator<<(streamer& os, const CoreDocumentContext& root)
{
    os << "DocumentContext: " << root.context();
    return os;
}


const SessionPtr&
CoreDocumentContext::getSession() const
{
    return mCore->session();
}

const RootConfig&
CoreDocumentContext::getRootConfig() const
{
    assert(mCore);
    return mCore->rootConfig();
}

std::string
CoreDocumentContext::getTheme() const
{
    assert(mCore);
    return mCore->getTheme();
}

const TextMeasurementPtr&
CoreDocumentContext::measure() const
{
    return mCore->measure();
}

ComponentPtr
CoreDocumentContext::findComponentById(const std::string& id) const
{
    assert(mCore);

    // Fast path search for uid value
    auto *ptr = findByUniqueId(id);
    if (ptr && ptr->objectType() == UIDObject::UIDObjectType::COMPONENT)
        return static_cast<Component*>(ptr)->shared_from_this();

    // Depth-first search
    auto top = mCore->top();
    return top ? top->findComponentById(id, false) : nullptr;
}

UIDObject *
CoreDocumentContext::findByUniqueId(const std::string& uid) const
{
    assert(mCore);
    return mCore->uniqueIdManager().find(uid);
}

void
CoreDocumentContext::processOnMounts()
{
    // Execute the "onMount" document command
    APL_TRACE_BEGIN("DocumentContext:executeOnMount");
    auto cmd = DocumentCommand::create(kPropertyOnMount, ON_MOUNT_HANDLER_NAME, shared_from_this());
    mCore->sequencer().execute(cmd, false);
    // Clear any pending mounts as we just executed those
    mCore->pendingOnMounts().clear();
    APL_TRACE_END("DocumentContext:executeOnMount");
}

void
CoreDocumentContext::flushDataUpdates()
{
    mCore->dataManager().flushDirty();
}

CoreDocumentContextPtr
CoreDocumentContext::cast(const DocumentContextPtr& documentContext)
{
    return std::static_pointer_cast<CoreDocumentContext>(documentContext);
}

ContextPtr
CoreDocumentContext::createKeyEventContext(const std::string& handler, const ObjectMapPtr& keyboard)
{
    ContextPtr ctx = Context::createFromParent(payloadContext());
    auto event = createDocumentEventProperties(handler);
    event->emplace("keyboard", keyboard);
    ctx->putConstant("event", event);
    return ctx;
}

} // namespace apl
