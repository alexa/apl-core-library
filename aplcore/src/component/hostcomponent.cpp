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

#include "apl/component/componentproperties.h"
#include "apl/component/corecomponent.h"
#include "apl/component/yogaproperties.h"
#include "apl/content/content.h"
#include "apl/document/coredocumentcontext.h"
#include "apl/embed/documentregistrar.h"
#include "apl/engine/keyboardmanager.h"
#include "apl/engine/layoutmanager.h"
#include "apl/engine/propdef.h"
#include "apl/engine/sharedcontextdata.h"
#include "apl/engine/tickscheduler.h"
#include "apl/primitives/dimension.h"
#include "apl/primitives/object.h"
#include "apl/primitives/rect.h"
#include "apl/time/sequencer.h"

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

const ComponentPropDefSet&
HostComponent::propDefSet() const
{
    static auto resetOnLoadOnFailFlag = [](Component& component) {
        auto& host = ((HostComponent&)component);
        host.mOnLoadOnFailReported = false;
        host.mNeedToRequestDocument = true;
        host.releaseEmbedded();
        host.requestEmbedded();
    };

    static ComponentPropDefSet sHostComponentProperties(ActionableComponent::propDefSet(), {
        {kPropertyHeight,           Dimension(100),        asNonAutoDimension, kPropIn | kPropDynamic | kPropStyled, yn::setHeight, defaultHeight},
        {kPropertyWidth,            Dimension(100),        asNonAutoDimension, kPropIn | kPropDynamic | kPropStyled, yn::setWidth, defaultWidth},
        {kPropertyEnvironment,      Object::EMPTY_MAP(),   asAny,              kPropIn},
        {kPropertyOnFail,           Object::EMPTY_ARRAY(), asCommand,          kPropIn},
        {kPropertyOnLoad,           Object::EMPTY_ARRAY(), asCommand,          kPropIn},
        {kPropertySource,           "",                    asUrlRequest,       kPropRequired | kPropIn | kPropDynamic | kPropVisualHash | kPropEvaluated, resetOnLoadOnFailFlag},
        {kPropertyEmbeddedDocument, Object::NULL_OBJECT(), asAny,              kPropDynamic | kPropRuntimeState},
    });

    return sHostComponentProperties;
}

void
HostComponent::requestEmbedded()
{
    const auto source = URLRequest::asURLRequest(getProperty(kPropertySource));
    mRequest = EmbedRequest::create(source, mContext->documentContext());

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
HostComponent::onLoad(const EmbeddedRequestSuccessResponse&& response)
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
HostComponent::initializeEmbedded(const EmbeddedRequestSuccessResponse&& response)
{
    resolvePendingParameters(response.content);

    auto embedded = CoreDocumentContext::create(
        mContext,
        response.content,
        getProperty(kPropertyEnvironment),
        getCalculated(kPropertyInnerBounds).get<Rect>().getSize(),
        response.documentConfig);

    const URLRequest& url = response.request->getUrlRequest();
    if (!embedded || !embedded->setup(nullptr)) {
        mRequest = nullptr;
        onFailHandler(url, "Embedded document failed to inflate");
        return nullptr;
    }

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
HostComponent::reinflate()
{
    auto embeddedId = getDocumentId();
    if (!embeddedId) return;

    auto embedded = mContext->getShared()->documentRegistrar().get(embeddedId);
    auto coreTop = CoreComponent::cast(embedded->topComponent());

    mContext->layoutManager().removeAsTopNode(CoreComponent::cast(embedded->topComponent()));
    CoreComponent::removeChild(coreTop, false);

    embedded->reinflate([&](){
        auto coreTop = CoreComponent::cast(embedded->topComponent());
        // Can't fail
        assert(insertChild(coreTop, 0, true));

        mContext->layoutManager().setAsTopNode(coreTop);
        mContext->layoutManager().requestLayout(coreTop, false);

        embedded->processOnMounts();
        mContext->getShared()->tickScheduler().processTickHandlers(embedded);
    });
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
HostComponent::resolvePendingParameters(const ContentPtr& content)
{
    if (!content->isReady()) {
        for (const auto& param : content->getPendingParameters()) {
            auto it = mProperties.find(param);
            if (it != mProperties.end()) {
                auto parameter = it->second;
                if (parameter.isString()) {
                    parameter = evaluate(*mContext, parameter);
                }
                content->addObjectData(param, parameter);
            }
            else {
                CONSOLE(mContext) << "Missing value for parameter " << param;
            }
        }
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
    if (!embeddedId || mNeedToRequestDocument) return;
    auto boundsAfterLayout = getProperty(kPropertyInnerBounds);

    auto embedded = mContext->getShared()->documentRegistrar().get(embeddedId);
    if (embedded && boundsBeforeLayout != boundsAfterLayout) {
        auto size = boundsAfterLayout.get<Rect>();
        embedded->configurationChange(ConfigurationChange(
            // std::lround use copied from Dimension::AbsoluteDimensionObjectType::asInt
            std::lround(mContext->dpToPx(size.getWidth())),
            std::lround(mContext->dpToPx(size.getHeight()))));
    }
}

void
HostComponent::postProcessLayoutChanges()
{
    ActionableComponent::postProcessLayoutChanges();
    if (mNeedToRequestDocument) requestEmbedded();
}

} // namespace apl
