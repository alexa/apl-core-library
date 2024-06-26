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

#include "apl/component/imagecomponent.h"

#include "apl/component/componentpropdef.h"
#include "apl/component/yogaproperties.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/event.h"
#include "apl/media/mediamanager.h"
#include "apl/time/sequencer.h"
#ifdef SCENEGRAPH
#include "apl/scenegraph/builder.h"
#include "apl/scenegraph/scenegraph.h"
#endif // SCENEGRAPH

namespace apl {

CoreComponentPtr
ImageComponent::create(const ContextPtr& context,
                       Properties&& properties,
                       const Path& path)
{
    auto ptr = std::make_shared<ImageComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

ImageComponent::ImageComponent(const ContextPtr& context,
                               Properties&& properties,
                               const Path& path)
    : CoreComponent(context, std::move(properties), path)
{
}

void
ImageComponent::releaseSelf()
{
    CoreComponent::releaseSelf();
    MediaComponentTrait::release();
}

const ComponentPropDefSet&
ImageComponent::propDefSet() const
{
    static auto resetMediaState = [](Component& component) {
        auto& comp = (ImageComponent&)component;
        comp.mOnLoadOnFailReported = false;
        comp.resetMediaFetchState();
    };

    static ComponentPropDefSet sImageComponentProperties = ComponentPropDefSet(
        CoreComponent::propDefSet(), MediaComponentTrait::propDefList()).add({
        {kPropertyAlign,           kImageAlignCenter,                     sAlignMap,           kPropInOut | kPropStyled | kPropDynamic | kPropVisualHash}, // Doesn't match 1.0 spec
        {kPropertyBorderRadius,    Dimension(DimensionType::Absolute, 0), asAbsoluteDimension, kPropInOut | kPropStyled | kPropDynamic | kPropVisualHash},
        {kPropertyFilters,         Object::EMPTY_ARRAY(),                 asFilterArray,       kPropInOut | kPropVisualHash}, // Takes part in hash even though it's not dynamic.
        {kPropertyOnFail,          Object::EMPTY_ARRAY(),                 asCommand,           kPropIn},
        {kPropertyOnLoad,          Object::EMPTY_ARRAY(),                 asCommand,           kPropIn},
        {kPropertyOverlayColor,    Color(),                               asColor,             kPropInOut | kPropStyled | kPropDynamic | kPropVisualHash},
        {kPropertyOverlayGradient, Object::NULL_OBJECT(),                 asGradient,          kPropInOut | kPropStyled | kPropDynamic | kPropVisualHash},
        {kPropertyScale,           kImageScaleBestFit,                    sScaleMap,           kPropInOut | kPropStyled | kPropDynamic | kPropVisualHash},
        {kPropertySource,          "",                                    asImageSourceArray,  kPropInOut | kPropDynamic | kPropVisualHash | kPropEvaluated, resetMediaState},
    });

    return sImageComponentProperties;
}

std::vector<URLRequest>
ImageComponent::getSources()
{
    std::vector<URLRequest> sources;

    auto& sourceProp = getCalculated(kPropertySource);
    // Check if there is anything to fetch
    if (sourceProp.empty()) {
        return sources;
    }

    if (sourceProp.isString()) { // Single source
        sources.emplace_back(URLRequest::asURLRequest(sourceProp));
    } else if (sourceProp.isArray()) {
        auto& filters = getCalculated(kPropertyFilters);
        if (filters.empty()) { // If no filters use last
            sources.emplace_back(URLRequest::asURLRequest(sourceProp.at(sourceProp.size() - 1)));
        } else { // Else request everything
            for (auto& source : sourceProp.getArray()) {
                sources.emplace_back(URLRequest::asURLRequest(source));
            }
        }
    }

    return sources;
}

const EventPropertyMap&
ImageComponent::eventPropertyMap() const
{
    static EventPropertyMap sImageEventProperties = eventPropertyMerge(
        CoreComponent::eventPropertyMap(),
        {
            {"source", [](const CoreComponent *c) { return c->getCalculated(kPropertySource); }},
            {"url", [](const CoreComponent *c) { return c->getCalculated(kPropertySource); }},
        });
    return sImageEventProperties;
}

std::string
ImageComponent::getVisualContextType() const
{
    return getCalculated(kPropertySource).empty() ? VISUAL_CONTEXT_TYPE_EMPTY : VISUAL_CONTEXT_TYPE_GRAPHIC;
}

void
ImageComponent::postProcessLayoutChanges(bool first)
{
    CoreComponent::postProcessLayoutChanges(first);
    MediaComponentTrait::postProcessLayoutChanges();
}

#ifdef SCENEGRAPH
sg::LayerPtr
ImageComponent::constructSceneGraphLayer(sg::SceneGraphUpdates& sceneGraph)
{
    auto layer = CoreComponent::constructSceneGraphLayer(sceneGraph);
    assert(layer);

    layer->setCharacteristic(sg::Layer::kCharacteristicHasMedia);

    auto filterPtr = getFilteredImage();
    auto rects = getImageRects(filterPtr);

    // TODO: Shadow - adjust the boundary of the object to put the shadow in the right spot
    // TODO: Use a child layer instead?
    layer->setContent(
        sg::clip(
           sg::path(rects.target,
                    getCalculated(kPropertyBorderRadius).asFloat()),
           sg::image(filterPtr, rects.target, rects.source)));

    return layer;
}

bool
ImageComponent::updateSceneGraphInternal(sg::SceneGraphUpdates& sceneGraph)
{
    // TODO: This doesn't cache intelligently and it doesn't address all of the interactions
    const bool fixMediaState = isDirty(kPropertyMediaState);
    const bool fixBounds =
        isDirty(kPropertyAlign) || isDirty(kPropertyScale) || isDirty(kPropertyInnerBounds);
    const bool fixFilter = isDirty(kPropertyOverlayGradient) || isDirty(kPropertyOverlayColor) ||
                           isDirty(kPropertySource);
    const bool fixBorderRadius = isDirty(kPropertyBorderRadius);
    const bool fixSomething = fixBounds || fixFilter || fixBorderRadius || fixMediaState;

    if (!fixSomething)
        return false;

    auto *layer = mSceneGraphLayer.get();
    auto* clip = sg::ClipNode::cast(layer->content());
    auto* image = sg::ImageNode::cast(clip->child());

    if (fixFilter || fixMediaState) {
        image->setImage(getFilteredImage());
    }

    auto rects = getImageRects(image->getImage());

    if (fixBounds || fixMediaState) {
        image->setTarget(rects.target);
        image->setSource(rects.source);
    }

    if (fixBounds || fixBorderRadius || fixMediaState) {
        auto* clipPath = sg::RoundedRectPath::cast(clip->getPath());
        clipPath->setRoundedRect(
            RoundedRect(rects.target, getCalculated(kPropertyBorderRadius).asFloat()));
    }

    return true;
}

sg::FilterPtr
ImageComponent::getFilteredImage()
{
    if (mMediaObjectHolders.empty())
        return nullptr;

    std::vector<sg::FilterPtr> stack;
    for (const auto& m : mMediaObjectHolders)
        stack.push_back(sg::filter(m.getMediaObject()));

    // Convenience function for returning the correct item on the stack
    auto fromStack = [&stack](int value) -> sg::FilterPtr {
        if (value < 0)
            value += static_cast<int>(stack.size());
        return value < 0 || value >= stack.size() ? nullptr : stack.at(value);
    };

    auto filters = getCalculated(kPropertyFilters);

    for (int i = 0; i < filters.size(); i++) {
        const auto& filter = filters.at(i).get<Filter>();
        switch (filter.getType()) {
            case kFilterTypeBlend:
                stack.push_back(sg::blend(
                    fromStack(filter.getValue(kFilterPropertyDestination).getInteger()),
                    fromStack(filter.getValue(kFilterPropertySource).getInteger()),
                    static_cast<BlendMode>(filter.getValue(kFilterPropertyMode).getInteger())));
                break;
            case kFilterTypeBlur:  // TODO: Do we need to convert dimensions to pixels?
                stack.push_back(sg::blur(
                    fromStack(filter.getValue(kFilterPropertySource).getInteger()),
                    filter.getValue(kFilterPropertyRadius).asFloat()));
                break;
            case kFilterTypeColor:
                stack.push_back(sg::solid(
                    sg::paint(filter.getValue(kFilterPropertyColor))));
                break;
            case kFilterTypeExtension:
                // TODO: What do we do here?
                break;
            case kFilterTypeGradient:
                stack.push_back(sg::solid(
                    sg::paint(filter.getValue(kFilterPropertyGradient))));
                break;
            case kFilterTypeGrayscale:
                stack.push_back(sg::grayscale(
                    fromStack(filter.getValue(kFilterPropertySource).getInteger()),
                    filter.getValue(kFilterPropertyAmount).asFloat()));
                break;
            case kFilterTypeNoise:
                stack.push_back(sg::noise(
                    fromStack(filter.getValue(kFilterPropertySource).getInteger()),
                    static_cast<NoiseFilterKind>(filter.getValue(kFilterPropertyKind).getInteger()),
                    filter.getValue(kFilterPropertyUseColor).getBoolean(),
                    filter.getValue(kFilterPropertySigma).getDouble()));
                break;
            case kFilterTypeSaturate:
                stack.push_back(sg::saturate(
                    fromStack(filter.getValue(kFilterPropertySource).getInteger()),
                    filter.getValue(kFilterPropertyAmount).asFloat()));
                break;
        }
    }

    return
        sg::blend(  // The overlay gradient goes on top
            sg::blend(  // The overlay color goes next
                stack.back(),  // The back of the stack is the final object
                sg::solid(
                    sg::paint(getCalculated(kPropertyOverlayColor))),
                BlendMode::kBlendModeNormal),
            sg::solid(
                sg::paint(getCalculated(kPropertyOverlayGradient))),
            BlendMode::kBlendModeNormal);
}

ImageComponent::ImageRects
ImageComponent::getImageRects(const sg::FilterPtr& filter)
{
    if (!filter)
        return {};

    const auto& innerBounds = getCalculated(kPropertyInnerBounds).get<Rect>();

    // Calculate the scaled size of the image fit to the component.  This is in PIXELS and must be
    // CONVERTED to DP.
    auto imageSizePixels = filter->size();
    auto imagePixelWidth = imageSizePixels.getWidth();
    auto imagePixelHeight = imageSizePixels.getHeight();

    // Convert from pixel size to DP.  We -assume- a 160 dpi screen.  In other words,
    // a 160 pixel wide image will always be drawn at 160 dp even if the screen is actually 320 ppi.
    float targetWidth = imagePixelWidth;
    float targetHeight = imagePixelHeight;
    if (targetWidth <= 0 || targetHeight <= 0)
        return {};

    switch (getCalculated(kPropertyScale).asInt()) {
        case apl::kImageScaleNone:
            break;
        case apl::kImageScaleFill:
            // Scale non-uniformly to fill the target
            targetWidth = innerBounds.getWidth();
            targetHeight = innerBounds.getHeight();
            break;
        case apl::kImageScaleBestFill: {
            // Scale uniformly up or down so the bounding box is covered
            float scaleBy = std::max(innerBounds.getWidth() / targetWidth,
                                     innerBounds.getHeight() / targetHeight);
            targetWidth *= scaleBy;
            targetHeight *= scaleBy;
        } break;
        case apl::kImageScaleBestFit: {
            // Scale uniformly up or down so mImage just fits in the bounding box
            float scaleBy = std::min(innerBounds.getWidth() / targetWidth,
                                     innerBounds.getHeight() / targetHeight);
            targetWidth *= scaleBy;
            targetHeight *= scaleBy;
        } break;
        case apl::kImageScaleBestFitDown:
            // Scale uniformly down so the mImage just fits.  Never scale up
            if (innerBounds.getWidth() < targetWidth || innerBounds.getHeight() < targetHeight) {
                float scaleBy = std::min(innerBounds.getWidth() / targetWidth,
                                         innerBounds.getHeight() / targetHeight);
                targetWidth *= scaleBy;
                targetHeight *= scaleBy;
            }
            break;
    }

    // Now position the scaled image relative to the innerBounds of the component
    auto targetRect = Rect{innerBounds.getLeft(), innerBounds.getTop(), targetWidth, targetHeight};

    switch (getCalculated(kPropertyAlign).asInt()) {
        case apl::kImageAlignBottom:
            targetRect.offset((innerBounds.getWidth() - targetWidth) / 2,
                              innerBounds.getHeight() - targetHeight);
            break;
        case apl::kImageAlignBottomLeft:
            targetRect.offset(0, innerBounds.getHeight() - targetHeight);
            break;
        case apl::kImageAlignBottomRight:
            targetRect.offset(innerBounds.getWidth() - targetWidth,
                              innerBounds.getHeight() - targetHeight);
            break;
        case apl::kImageAlignCenter:
            targetRect.offset((innerBounds.getWidth() - targetWidth) / 2,
                              (innerBounds.getHeight() - targetHeight) / 2);
            break;
        case apl::kImageAlignLeft:
            targetRect.offset(0, (innerBounds.getHeight() - targetHeight) / 2);
            break;
        case apl::kImageAlignRight:
            targetRect.offset(innerBounds.getWidth() - targetWidth,
                              (innerBounds.getHeight() - targetHeight) / 2);
            break;
        case apl::kImageAlignTop:
            targetRect.offset((innerBounds.getWidth() - targetWidth) / 2, 0);
            break;
        case apl::kImageAlignTopLeft:
            break;
        case apl::kImageAlignTopRight:
            targetRect.offset(innerBounds.getWidth() - targetWidth, 0);
            break;
    }

    // Intersect the targetRect with the component bounds to find the part of the screen that will
    // be drawn.
    ImageRects result{.source = Rect{0, 0, imagePixelWidth, imagePixelHeight},
                      .target = targetRect.intersect(innerBounds)};

    // If the intersection of targetRect with component bounds is smaller than the targetRect, we
    // need to trim the source rect (from the bitmap) to match.
    if (result.target != targetRect) {
        auto scaleX = imagePixelWidth / targetRect.getWidth();
        auto scaleY = imagePixelHeight / targetRect.getHeight();
        result.source = Rect{(result.target.getLeft() - targetRect.getLeft()) * scaleX,
                             (result.target.getTop() - targetRect.getTop()) * scaleY,
                              result.target.getWidth() * scaleX,
                              result.target.getHeight() * scaleY};
    }

    return result;
}
#endif // SCENEGRAPH

void
ImageComponent::onFail(const MediaObjectPtr& mediaObject)
{
    if (mOnLoadOnFailReported)
        return;
    auto component = getComponent();
    auto errorData = std::make_shared<ObjectMap>();
    errorData->emplace("value", mediaObject->url());
    errorData->emplace("error", mediaObject->errorDescription());
    errorData->emplace("errorCode", mediaObject->errorCode());
    auto& commands = component->getCalculated(kPropertyOnFail);
    auto eventContext = component->createEventContext("Fail", errorData);
    component->getContext()->sequencer().executeCommands(
        commands,
        eventContext,
        component->shared_from_corecomponent(),
        true);
    mOnLoadOnFailReported = true;
}

void
ImageComponent::onLoad()
{
    if (mOnLoadOnFailReported)
        return;
    auto component = getComponent();
    auto& commands = component->getCalculated(kPropertyOnLoad);
    component->getContext()->sequencer().executeCommands(
        commands,
        component->createEventContext("Load"),
        component->shared_from_corecomponent(),
        true);
    mOnLoadOnFailReported = true;
}

} // namespace apl
