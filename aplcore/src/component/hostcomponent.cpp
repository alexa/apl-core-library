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

#include "apl/component/hostcomponent.h"

#include <cmath>
#include <memory>
#include <utility>

#include "apl/content/content.h"
#include "apl/content/viewport.h"
#include "apl/datasource/datasourceprovider.h"
#include "apl/document/coredocumentcontext.h"
#include "apl/embed/documentregistrar.h"
#include "apl/engine/keyboardmanager.h"
#include "apl/engine/layoutmanager.h"
#include "apl/engine/sharedcontextdata.h"
#include "apl/engine/tickscheduler.h"
#include "apl/time/sequencer.h"
#include "apl/yoga/yogaproperties.h"

namespace {

using namespace apl;

// copied from corecomponent.cpp (similar to pagercomponent.cpp)
inline Object
defaultWidth(Component& component, const RootConfig& rootConfig)
{
    return rootConfig.getDefaultComponentWidth(component.getType());
}

// copied from corecomponent.cpp (similar to pagercomponent.cpp)
inline Object
defaultHeight(Component& component, const RootConfig& rootConfig)
{
    return rootConfig.getDefaultComponentHeight(component.getType());
}

} // unnamed namespace

static const char *EMBED_REINFLATE_SEQUENCER_PREFIX = "REINFLATE_SEQUENCER_";

static const int DEFAULT_HOST_DIMENSION = 100;

namespace apl {

CoreComponentPtr
HostComponent::create(const ContextPtr& context,
                      Properties&& properties,
                      const Path& path)
{
    auto ptr = std::make_shared<HostComponent>(
        context,
        std::move(properties),
        path);
    ptr->initialize();
    return ptr;
}

HostComponent::HostComponent(const ContextPtr& context,
                             Properties&& properties,
                             const Path& path)
    : ActionableComponent(context, std::move(properties), path)
{}

std::shared_ptr<HostComponent>
HostComponent::cast(const ComponentPtr& component)
{
    return component && (CoreComponent::cast(component)->getType() == kComponentTypeHost)
               ? std::static_pointer_cast<HostComponent>(component) : nullptr;
}

ConfigurationChange
HostComponent::filterConfigurationChange(const ConfigurationChange& configurationChange,
                                         const Metrics& metrics) const
{
    auto parentContext = mContext;
    auto overrideContext = Context::createFromParent(parentContext);

    // Override environment and viewport
    overrideContext->putConstant(
        "environment",
        std::make_shared<ObjectMap>(
            configurationChange.mergeEnvironment(
                overrideContext->opt("environment").getMap()
            )
        )
    );
    overrideContext->putConstant(
        "viewport",
        makeViewport(
            configurationChange.mergeMetrics(metrics),
            configurationChange.theme()
        )
    );

    // Re-resolve environment, if any, and add to the change
    auto env = getProperty(kPropertyEnvironment);
    auto originalEnv = std::make_shared<ObjectMap>();
    auto resolvedEnv = std::make_shared<ObjectMap>();
    if (!env.empty() && env.isMap()) {
        // Evaluate props in both original context and new context
        for (const auto& customEnv : env.getMap()) {
            originalEnv->emplace(customEnv.first, evaluate(*parentContext, customEnv.second));
            resolvedEnv->emplace(customEnv.first, evaluate(*overrideContext, customEnv.second));
        }
    }

    auto result = configurationChange.embeddedDocumentChange();
    // If any changed - apply to the change
    if (originalEnv != resolvedEnv) {
        for (const auto& entity : *resolvedEnv)
            result.environmentValue(entity.first, entity.second);
    }
    return result;
}

const ComponentPropDefSet&
HostComponent::propDefSet() const
{
    static auto sourcePropertyChanged = [](Component& component) {
        auto& host = ((HostComponent&)component);
        host.mOnLoadOnFailReported = false;
        host.mNeedToRequestDocument = true;
        host.detachEmbedded();
        host.releaseEmbedded();
        host.requestEmbedded();
    };

    static ComponentPropDefSet sHostComponentProperties(ActionableComponent::propDefSet(), {
        {kPropertyHeight,           Dimension(DEFAULT_HOST_DIMENSION), asDimension,  kPropIn | kPropDynamic | kPropStyled, yn::setHeight, defaultHeight},
        {kPropertyWidth,            Dimension(DEFAULT_HOST_DIMENSION), asDimension,  kPropIn | kPropDynamic | kPropStyled, yn::setWidth, defaultWidth},
        {kPropertyEnvironment,      Object::EMPTY_MAP(),               asAny,        kPropIn},
        {kPropertyOnFail,           Object::EMPTY_ARRAY(),             asCommand,    kPropIn},
        {kPropertyOnLoad,           Object::EMPTY_ARRAY(),             asCommand,    kPropIn},
        {kPropertyParameters,       Object::EMPTY_MAP(),               asAny,        kPropIn | kPropDynamic},
        {kPropertySource,           "",                                asUrlRequest, kPropRequired | kPropIn | kPropDynamic | kPropVisualHash | kPropEvaluated, sourcePropertyChanged},
        {kPropertyEmbeddedDocument, Object::NULL_OBJECT(),             asAny,        kPropDynamic | kPropRuntimeState},
    });

    return sHostComponentProperties;
}

void
HostComponent::refreshContent(const ContentPtr& content, const DocumentConfigPtr& documentConfig) const
{
    content->refresh(generateChildMetrics(), *generateChildConfig(documentConfig));
    resolvePendingParameters(content);
}

void
HostComponent::requestEmbedded()
{
    const auto source = URLRequest::asURLRequest(getProperty(kPropertySource));
    mRequest = EmbedRequest::create(source, mContext->documentContext(), shared_from_this());

    if (auto oldEmbeddedId = getDocumentId()) {
        mNeedToRequestDocument = false;

        auto& registrar = mContext->getShared()->documentRegistrar();
        auto embedded = registrar.get(oldEmbeddedId);

        // We have old document defined. Reuse as a result of the request.
        detachEmbedded();

        registrar.deregisterDocument(oldEmbeddedId);

        auto coreTop = CoreComponent::cast(embedded->topComponent());
        // Can't really fail
        assert(CoreComponent::insertChild(coreTop, 0, false));

        // Update with new registration
        setDocument(registrar.registerDocument(embedded), includeChildrenInVisualContext());

        mContext->layoutManager().setAsTopNode(coreTop);
        mContext->layoutManager().requestLayout(coreTop, false);

        return;
    }

    std::weak_ptr<HostComponent> weakHost = std::static_pointer_cast<HostComponent>(shared_from_this());

    mContext->getShared()->documentManager().request(
        mRequest,
        [weakHost](EmbeddedRequestSuccessResponse&& response) {
            auto host = weakHost.lock();
            return host
                ? host->onLoad(std::move(response))
                : nullptr;
        },
        [weakHost](EmbeddedRequestFailureResponse&& response) {
            auto host = weakHost.lock();
            if (host)
                host->onFail(std::move(response));
        });

    mNeedToRequestDocument = false;
}

DocumentContextPtr
HostComponent::onLoad(EmbeddedRequestSuccessResponse&& response)
{
    if (mOnLoadOnFailReported)
        return nullptr;
    mOnLoadOnFailReported = true;

    onLoadHandler();
    return initializeEmbedded(std::move(response));
}

void
HostComponent::onLoadHandler()
{
    auto& commands = getCalculated(kPropertyOnLoad);
    mContext->sequencer().executeCommands(
        commands,
        createEventContext("Load"),
        shared_from_corecomponent(),
        true);
}

DocumentContextPtr
HostComponent::initializeEmbedded(EmbeddedRequestSuccessResponse&& response)
{
    resolvePendingParameters(response.content);

    auto embedded = CoreDocumentContext::create(
        mContext->getShared(),
        generateChildMetrics(),
        response.content,
        *generateChildConfig(response.documentConfig));

    const URLRequest& url = response.request->getUrlRequest();
    if (!embedded || !embedded->setup(nullptr)) {
        mRequest = nullptr;
        onFailHandler(url, "Embedded document failed to inflate");
        return nullptr;
    }
    mCalculated.set(kPropertyBackground, embedded->content()->getBackground());

    auto coreTop = CoreComponent::cast(embedded->topComponent());

    bool result = CoreComponent::insertChild(coreTop, 0, true);
    if (!result) {
        mRequest = nullptr;
        onFailHandler(url, "Failed to insert embedded document's top component");
        return nullptr;
    }

    mContext->layoutManager().setAsTopNode(coreTop);
    mContext->layoutManager().requestLayout(coreTop, false);

    embedded->processOnMounts();
    mContext->getShared()->tickScheduler().processTickHandlers(embedded);

    auto id = mContext->getShared()->documentRegistrar().registerDocument(embedded);
    setDocument(id, response.connectedVisualContext);

    return embedded;
}

void
HostComponent::finalizeReinflate(const CoreDocumentContextPtr& document)
{
    document->finishReinflate([&](){
        auto coreTop = CoreComponent::cast(document->topComponent());
        // Can't fail
        if (insertChild(coreTop, 0, true)) {
            mContext->layoutManager().setAsTopNode(coreTop);
            mContext->layoutManager().requestLayout(coreTop, false);

            document->processOnMounts();
            mContext->getShared()->tickScheduler().processTickHandlers(document);
        } else {
            assert(false);
        }
    }, mReinflationState.first, mReinflationState.second);
}

void
HostComponent::reinflate()
{
    auto embeddedId = getDocumentId();
    if (!embeddedId) return;

    auto embedded = mContext->getShared()->documentRegistrar().get(embeddedId);
    auto coreTop = CoreComponent::cast(embedded->topComponent());

    mContext->layoutManager().removeAsTopNode(CoreComponent::cast(embedded->topComponent()));
    CoreComponent::removeChild(coreTop, false);

    auto change = embedded->activeChanges();
    auto metrics = change.mergeMetrics(embedded->currentMetrics());
    auto config = change.mergeRootConfig(embedded->currentConfig());

    auto preservedSequencers = std::map<std::string, ActionPtr>();
    auto startReinflateResult = embedded->startReinflate(preservedSequencers);

    if (!startReinflateResult.first) {
        LOG(LogLevel::kError) << "Failed to prepare for reinflation of embedded document " << embeddedId;
        return;
    }

    mReinflationState = std::make_pair(startReinflateResult.second, preservedSequencers);

    // Resolving asynchronously, required to get extensions resolved.
    embedded->content()->refresh(metrics, config);
    mCalculated.set(kPropertyBackground, embedded->content()->getBackground());

    if (embedded->content()->isWaiting()) {
        auto timers = std::static_pointer_cast<Timers>(config.getTimeManager());
        auto action = Action::make(timers, [this, embedded](ActionRef ref) {
            embedded->contextPtr()->pushEvent(Event(kEventTypeContentRefresh, shared_from_this(), ref));
        });

        auto wrapped = Action::wrapWithCallback(timers, action, [this, embedded](bool, const ActionPtr& ptr) {
            finalizeReinflate(embedded);
        });

        auto sequencer = EMBED_REINFLATE_SEQUENCER_PREFIX + getUniqueId();
        getContext()->sequencer().terminateSequencer(sequencer);
        getContext()->sequencer().attachToSequencer(wrapped, sequencer);
    } else {
        // If content has not changed - reinflate instantly
        finalizeReinflate(embedded);
    }
}

bool
HostComponent::includeChildrenInVisualContext() const
{
    auto embeddedDocument = getCalculated(kPropertyEmbeddedDocument);
    if (!embeddedDocument.isNull()) {
        return embeddedDocument.get("connectedVC").asBoolean();
    }
    return false;
}

void
HostComponent::resolvePendingParameters(const ContentPtr& content) const
{
    // If content is ready, there's nothing further to do
    if (content->isReady()) {
        return;
    }

    const auto& explicitParameters = getCalculated(kPropertyParameters);
    for (const auto& param : content->getPendingParameters()) {
        // Pending parameters will be resolved to null if we have nothing better
        Object data = Object::NULL_OBJECT();

        if (!explicitParameters.empty()) {
            // Read from explicit parameters provided
            data = explicitParameters.get(param);
        } else {
            // When there are no explicit parameters, we allow implicit parameters that do not
            // conflict with intrinsic properties.
            const auto& intrinsicValue = mCalculated.get(param);
            if (intrinsicValue.isNull()) {
               auto it = mProperties.find(param);
               if (it != mProperties.end()) {
                  data = it->second;
               }
            } else {
                CONSOLE(mContext) << "Could not read intrinsic property " << param;
            }
        }

        data = evaluate(*mContext, data);
        content->addObjectData(param, data);
    }
}

void
HostComponent::onFail(const EmbeddedRequestFailureResponse&& response)
{
    if (mOnLoadOnFailReported)
        return;
    mOnLoadOnFailReported = true;

    onFailHandler(response.request->getUrlRequest(), response.failure);
    mRequest = nullptr;
}

void
HostComponent::onFailHandler(const URLRequest& url, const std::string& failure)
{
    auto& commands = getCalculated(kPropertyOnFail);
    auto errorData = std::make_shared<ObjectMap>();
    errorData->emplace("value", url.getUrl());
    errorData->emplace("error", failure);
    auto eventContext = createEventContext("Fail", errorData);
    mContext->sequencer().executeCommands(
        commands,
        eventContext,
        shared_from_corecomponent(),
        true);
}

void
HostComponent::detachEmbedded()
{
    auto embeddedId = getDocumentId();
    auto embedded = CoreDocumentContext::cast(mContext->getShared()->documentRegistrar().get(embeddedId));
    if (embedded != nullptr) {
        auto coreTop = CoreComponent::cast(embedded->topComponent());
        mContext->getShared()->layoutManager().removeAsTopNode(coreTop);
        // Remove without marking dirty - it's no longer useful for detached hierarchy.
        coreTop->remove(false);
    }
}

void
HostComponent::preRelease()
{
    // Detach child document hierarchy. Document should handle cleanup itself.
    detachEmbedded();
    ActionableComponent::preRelease();
}

void
HostComponent::releaseSelf()
{
    releaseEmbedded();
    ActionableComponent::releaseSelf();
}

void
HostComponent::releaseEmbedded()
{
    auto embeddedId = getDocumentId();
    auto embedded = CoreDocumentContext::cast(mContext->getShared()->documentRegistrar().get(embeddedId));
    if (embedded != nullptr) {
        mContext->getShared()->documentRegistrar().deregisterDocument(embeddedId);
        mCalculated.set(kPropertyEmbeddedDocument, Object::NULL_OBJECT());
    }
    mRequest = nullptr;
}

RootConfigPtr
HostComponent::generateChildConfig(const DocumentConfigPtr& documentConfig) const
{
    auto env = getProperty(kPropertyEnvironment);
    const RootConfig& rootConfig = mContext->getRootConfig();
    RootConfigPtr embeddedRootConfig = rootConfig.copy();
    embeddedRootConfig->set(RootProperty::kLang, env.opt("lang", mContext->getLang()));
    embeddedRootConfig->set(RootProperty::kLayoutDirection,
                            env.opt("layoutDirection", sLayoutDirectionMap.get(mContext->getLayoutDirection(), "")));

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

        for (const auto& ev : documentConfig->getEnvironmentValues()) {
            embeddedRootConfig->setEnvironmentValue(ev.first, ev.second);
        }
    }

    // Any custom env props allowed. Config does check internally. Values are evaluated.
    if (env.isMap()) {
        for (const auto& customEnv : env.getMap()) {
            embeddedRootConfig->setEnvironmentValue(customEnv.first, evaluate(*mContext, customEnv.second));
        }
    }

    return embeddedRootConfig;
}

bool
HostComponent::isAutoWidth() const
{
    return getCalculated(kPropertyWidth).isAutoDimension();
}

bool
HostComponent::isAutoHeight() const
{
    return getCalculated(kPropertyHeight).isAutoDimension();
}

Metrics
HostComponent::generateChildMetrics() const
{
    auto size = getCalculated(kPropertyInnerBounds).get<Rect>().getSize();

    // std::lround use copied from Dimension::AbsoluteDimensionObjectType::asInt
    int width = (int) std::lround(mContext->dpToPx(size.getWidth()));
    int height = (int) std::lround(mContext->dpToPx(size.getHeight()));

    auto result = Metrics();

    // If auto - metrics only used for context, so don't matter what value they have. Auto will get
    // reported as true. Set target size to default.
    if (isAutoWidth()) {
        if (width == 0) width = DEFAULT_HOST_DIMENSION;
        auto minmax = mContext->layoutManager().getMinMaxWidth(*this);
        result.minAndMaxWidth((int)minmax.first, (int)minmax.second);
    }
    if (isAutoHeight()) {
        if (height == 0) height = DEFAULT_HOST_DIMENSION;
        auto minmax = mContext->layoutManager().getMinMaxHeight(*this);
        result.minAndMaxHeight((int)minmax.first, (int)minmax.second);
    }

    return result
       .size(width, height)
       .shape(mContext->getScreenShape())
       .theme(mContext->getTheme().c_str())
       .dpi(mContext->getDpi())
       .mode(mContext->getViewportMode());
}

void
HostComponent::setDocument(int id, bool connectedVC)
{
    auto embeddedDocument = std::make_shared<ObjectMap>();
    embeddedDocument->emplace("id", id);
    embeddedDocument->emplace("connectedVC", connectedVC);
    mCalculated.set(kPropertyEmbeddedDocument, embeddedDocument);
}

int
HostComponent::getDocumentId() const
{
    auto embeddedDocument = getCalculated(kPropertyEmbeddedDocument);
    if (embeddedDocument.isNull()) return 0;
    return embeddedDocument.get("id").asInt();
}

ComponentPtr
HostComponent::findComponentById(const std::string& id, const bool traverseHost) const
{
    if (mId == id || mUniqueId == id)
        return std::const_pointer_cast<Component>(shared_from_this());

    auto embeddedId = getDocumentId();
    if (!embeddedId || !traverseHost)
        return nullptr;

    if (auto embedded = mContext->getShared()->documentRegistrar().get(embeddedId))
        return CoreComponent::cast(embedded->topComponent())->findComponentById(id, traverseHost);

   return nullptr;
}

std::string
HostComponent::getVisualContextType() const
{
    // Do not leak embedded document type if not linked.
    return includeChildrenInVisualContext()
               ? ActionableComponent::getVisualContextType()
               : VISUAL_CONTEXT_TYPE_EMPTY;
}

bool
HostComponent::getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator) {
    CoreComponent::getTags(outMap, allocator);

    rapidjson::Value embedded(rapidjson::kObjectType);
    
    embedded.AddMember("source", rapidjson::Value(URLRequest::asURLRequest(getProperty(kPropertySource)).getUrl().c_str(), allocator).Move(), allocator);
    embedded.AddMember("attached", includeChildrenInVisualContext(), allocator);

    outMap.AddMember("embedded", embedded, allocator);

    return true;
}

bool
HostComponent::executeKeyHandlers(KeyHandlerType type, const Keyboard& keyboard)
{
    // Embedded document (if exists) should take document handling
    if (auto embeddedId = getDocumentId()) {
        if (auto embedded = mContext->getShared()->documentRegistrar().get(embeddedId)) {
            auto& manager = mContext->getShared()->keyboardManager();
            const auto handledByEmbedded = manager.executeDocumentKeyHandlers(
                CoreDocumentContext::cast(embedded), type, keyboard);
            if (handledByEmbedded)
                return true;
        }
    }

    return ActionableComponent::executeKeyHandlers(type, keyboard);
}

void
HostComponent::processLayoutChanges(bool useDirtyFlag, bool first)
{
    auto boundsBeforeLayout = getProperty(kPropertyInnerBounds);
    CoreComponent::processLayoutChanges(useDirtyFlag, first);

    auto embeddedId = getDocumentId();

    // If Host autosized - do not perform config change based on size, just replace the size
    if (!embeddedId || mNeedToRequestDocument || isAutoHeight() || isAutoWidth()) return;
    auto boundsAfterLayout = getProperty(kPropertyInnerBounds);

    auto embedded = mContext->getShared()->documentRegistrar().get(embeddedId);
    if (embedded && boundsBeforeLayout != boundsAfterLayout) {
        auto size = boundsAfterLayout.get<Rect>();
        embedded->configurationChange(ConfigurationChange(
            // std::lround use copied from Dimension::AbsoluteDimensionObjectType::asInt
            (int) std::lround(mContext->dpToPx(size.getWidth())),
            (int) std::lround(mContext->dpToPx(size.getHeight()))),
            false);
    }
}

void
HostComponent::postProcessLayoutChanges(bool first)
{
    ActionableComponent::postProcessLayoutChanges(first);
    if (mNeedToRequestDocument) requestEmbedded();
}

} // namespace apl
