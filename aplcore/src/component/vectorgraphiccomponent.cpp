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

#include "apl/component/vectorgraphiccomponent.h"

#include "apl/component/componentpropdef.h"
#include "apl/component/yogaproperties.h"
#include "apl/graphic/graphic.h"
#include "apl/media/mediamanager.h"
#include "apl/media/mediaobject.h"
#include "apl/time/sequencer.h"
#include "apl/utils/url.h"
#ifdef SCENEGRAPH
#include "apl/content/rootconfig.h"
#include "apl/scenegraph/builder.h"
#include "apl/scenegraph/graphicfragment.h"
#include "apl/scenegraph/scenegraph.h"
#endif // SCENEGRAPH

namespace apl {


CoreComponentPtr
VectorGraphicComponent::create(const ContextPtr& context,
                               Properties&& properties,
                               const Path& path)
{
    auto ptr = std::make_shared<VectorGraphicComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

VectorGraphicComponent::VectorGraphicComponent(const ContextPtr& context,
                                               Properties&& properties,
                                               const Path& path)
    : TouchableComponent(context, std::move(properties), path)
{
}

void
VectorGraphicComponent::releaseSelf()
{
    auto graphic = mCalculated.get(kPropertyGraphic);
    if (graphic.is<Graphic>())
        graphic.get<Graphic>()->release();

    ActionableComponent::releaseSelf();
    MediaComponentTrait::release();
}

const ComponentPropDefSet&
VectorGraphicComponent::propDefSet() const
{
    static auto checkLayout = [](Component& component)
    {
        ((VectorGraphicComponent&)component).processLayoutChanges(true, false);
    };

    static auto resetMediaState = [](Component& component) {
        ((VectorGraphicComponent&)component).sourcePropertyChanged();
    };

    static auto sVectorGraphicComponentProperties = ComponentPropDefSet(TouchableComponent::propDefSet(), MediaComponentTrait::propDefList())
        .add({
            {kPropertyAlign,       kVectorGraphicAlignCenter, sVectorGraphicAlignMap, kPropInOut | kPropStyled | kPropDynamic | kPropVisualHash, checkLayout},
            {kPropertyGraphic,     Object::NULL_OBJECT(),     nullptr,                kPropOut | kPropVisualHash},
            {kPropertyMediaBounds, Object::NULL_OBJECT(),     nullptr,                kPropOut | kPropVisualHash},
            {kPropertyOnFail,      Object::EMPTY_ARRAY(),     asCommand,              kPropIn},
            {kPropertyOnLoad,      Object::EMPTY_ARRAY(),     asCommand,              kPropIn},
            {kPropertyScale,       kVectorGraphicScaleNone,   sVectorGraphicScaleMap, kPropInOut | kPropStyled | kPropDynamic | kPropVisualHash, checkLayout},
            {kPropertySource,      "",                        asVectorGraphicSource,  kPropInOut | kPropDynamic | kPropVisualHash | kPropEvaluated, resetMediaState},
    });

    return sVectorGraphicComponentProperties;
}

void
VectorGraphicComponent::initialize()
{
    TouchableComponent::initialize();

    // Default height and width
    double width = 100;
    double height = 100;

    // If the source was specified and it is a local graphic resource, we set kPropertyGraphic
    auto source = mCalculated.get(kPropertySource);
    if (source.isString()) {
        auto graphicResource = mContext->getGraphic(source.getString());
        if (!graphicResource.empty()) {
            auto graphic = Graphic::create(mContext,
                                           graphicResource,
                                           Properties(mProperties),
                                           shared_from_corecomponent());
            if (graphic) {
                mCalculated.set(kPropertyGraphic, graphic);
                height = graphic->getIntrinsicHeight();
                width = graphic->getIntrinsicWidth();
            }
        }
    }

    // Fix up the width if it was set to auto
    if (mCalculated.get(kPropertyWidth).isAutoDimension()) {
        YGNodeStyleSetWidth(mYGNodeRef, static_cast<float>(width));
        mCalculated.set(kPropertyWidth, Dimension(width));
    }

    // Fix up the height if it was set to auto
    if (mCalculated.get(kPropertyHeight).isAutoDimension()) {
        YGNodeStyleSetHeight(mYGNodeRef, static_cast<float>(height));
        mCalculated.set(kPropertyHeight, Dimension(height));
    }
}

void
VectorGraphicComponent::updateStyle()
{
    CoreComponent::updateStyle();

    // Changing the style is likely to change the graphic
    auto graphic = mCalculated.get(kPropertyGraphic);
    auto stylePtr = getStyle();
    if (graphic.is<Graphic>() && graphic.get<Graphic>()->updateStyle(stylePtr))
        setDirty(kPropertyGraphic);

    // Changing the style may result in a size change or a position change
    processLayoutChanges(true, false);
}

const EventPropertyMap&
VectorGraphicComponent::eventPropertyMap() const
{
    static EventPropertyMap sVectorGraphicEventProperties = eventPropertyMerge(
            CoreComponent::eventPropertyMap(),
            {
                    {"source", [](const CoreComponent *c) { return c->getCalculated(kPropertySource); }},
                    {"url", [](const CoreComponent *c) { return c->getCalculated(kPropertySource); }},
            });

    return sVectorGraphicEventProperties;
}

bool
VectorGraphicComponent::updateGraphic(const GraphicContentPtr& json)
{
    if (!json)
        return false;

    // Release any existing graphic
    releaseGraphic();

    auto url = encodeUrl(getCalculated(kPropertySource).asString());
    auto path = Path("_url").addObject(url);
    auto g = Graphic::create(mContext,
                             json->get(),
                             Properties(mProperties),
                             shared_from_corecomponent(),
                             path,
                             getStyle());
    if (!g)
        return false;

    mCalculated.set(kPropertyGraphic, g);
    setDirty(kPropertyGraphic);
    mGraphicReplaced = true;

    // Recalculate the media bounds. This will internally do a layout
    processLayoutChanges(true, false);
    g->clearDirty();   // Some flags may have been set; we clear them here because this is the first use of the graphic
    return true;
}

void
VectorGraphicComponent::releaseGraphic()
{
    auto graphic = mCalculated.get(kPropertyGraphic);
    if (!graphic.isNull()) {
        if (graphic.is<Graphic>())
            graphic.get<Graphic>()->release();
        setDirty(kPropertyGraphic);
        mCalculated.set(kPropertyGraphic, Object::NULL_OBJECT());
        mGraphicReplaced = true;
    }
}

void
VectorGraphicComponent::sourcePropertyChanged()
{
    mOnLoadOnFailReported = false;
    releaseGraphic();

    // Load a local graphic if it exists
    auto source = mCalculated.get(kPropertySource);
    if (source.isString()) {
        auto graphicResource = mContext->getGraphic(source.getString());
        if (!graphicResource.empty()) {
            auto graphic = Graphic::create(mContext,
                                           graphicResource,
                                           Properties(mProperties),
                                           shared_from_corecomponent());
            if (graphic) {
                mCalculated.set(kPropertyGraphic, graphic);
                setDirty(kPropertyGraphic);
                mGraphicReplaced = true;

                // Recalculate the media bounds.  This will internally run a layout
                processLayoutChanges(true, false);
                graphic->clearDirty();  // First use of the graphic; clear dirty flags
            }
        }
    }

    // Fix the media state and toggle a download for a URL-based graphic
    resetMediaFetchState();
}

void
VectorGraphicComponent::processLayoutChanges(bool useDirtyFlag, bool first)
{
    CoreComponent::processLayoutChanges(useDirtyFlag, first);

    auto graphic = mCalculated.get(kPropertyGraphic);
    if (graphic.is<Graphic>()) {
        auto g = graphic.get<Graphic>();

        const auto& innerBounds = mCalculated.get(kPropertyInnerBounds).get<Rect>();
        double width = g->getIntrinsicWidth();
        double height = g->getIntrinsicHeight();
        auto scaleProp = static_cast<VectorGraphicScale>(mCalculated.get(kPropertyScale).getInteger());
        auto alignProp = static_cast<VectorGraphicAlign>(mCalculated.get(kPropertyAlign).getInteger());

        switch (scaleProp) {
            case kVectorGraphicScaleNone:
                // No change
                break;

            case kVectorGraphicScaleBestFit: {
                double scaleWidth = innerBounds.getWidth() / width;
                double scaleHeight = innerBounds.getHeight() / height;
                double scale = std::min(scaleWidth, scaleHeight);
                width *= scale;
                height *= scale;
            }
                break;

            case kVectorGraphicScaleBestFill: {
                double scaleWidth = innerBounds.getWidth() / width;
                double scaleHeight = innerBounds.getHeight() / height;
                double scale = std::max(scaleWidth, scaleHeight);
                width *= scale;
                height *= scale;
            }
                break;

            case kVectorGraphicScaleFill:
                width = innerBounds.getWidth();
                height = innerBounds.getHeight();
                break;
        }

        double x = 0;
        double y = 0;

        switch (alignProp) {
            case kVectorGraphicAlignBottom:
                x = innerBounds.getCenterX() - width / 2;
                y = innerBounds.getBottom() - height;
                break;
            case kVectorGraphicAlignBottomLeft:
                x = innerBounds.getLeft();
                y = innerBounds.getBottom() - height;
                break;
            case kVectorGraphicAlignBottomRight:
                x = innerBounds.getRight() - width;
                y = innerBounds.getBottom() - height;
                break;
            case kVectorGraphicAlignCenter:
                x = innerBounds.getCenterX() - width / 2;
                y = innerBounds.getCenterY() - height / 2;
                break;
            case kVectorGraphicAlignLeft:
                x = innerBounds.getLeft();
                y = innerBounds.getCenterY() - height / 2;
                break;
            case kVectorGraphicAlignRight:
                x = innerBounds.getRight() - width;
                y = innerBounds.getCenterY() - height / 2;
                break;
            case kVectorGraphicAlignTop:
                x = innerBounds.getCenterX() - width / 2;
                y = innerBounds.getTop();
                break;
            case kVectorGraphicAlignTopLeft:
                x = innerBounds.getLeft();
                y = innerBounds.getTop();
                break;
            case kVectorGraphicAlignTopRight:
                x = innerBounds.getRight() - width;
                y = innerBounds.getTop();
                break;
        }

        Rect r(x, y, width, height);
        auto mediaBounds = mCalculated.get(kPropertyMediaBounds);
        if (!mediaBounds.is<Rect>() || r != mediaBounds.get<Rect>()) {
            mCalculated.set(kPropertyMediaBounds, std::move(r));
            if (useDirtyFlag)
                setDirty(kPropertyMediaBounds);
        }

        // The graphic may need to be resized based on the new layout
        if (g->layout(width, height, useDirtyFlag) && useDirtyFlag)
            setDirty(kPropertyGraphic);
    }
}

std::string
VectorGraphicComponent::getVisualContextType() const
{
    return getCalculated(kPropertyGraphic).isNull() ? VISUAL_CONTEXT_TYPE_EMPTY : VISUAL_CONTEXT_TYPE_GRAPHIC;
}

bool
VectorGraphicComponent::setPropertyInternal(const std::string& key, const apl::Object& value)
{
    auto graphic = mCalculated.get(kPropertyGraphic);
    if (graphic.is<Graphic>()) {
        auto g = graphic.get<Graphic>();
        return g->setProperty(key, value);
    }

    return false;
}

std::pair<Object, bool>
VectorGraphicComponent::getPropertyInternal(const std::string& key) const
{
    auto graphic = mCalculated.get(kPropertyGraphic);
    if (graphic.is<Graphic>())
        return graphic.get<Graphic>()->getProperty(key);

    return { Object::NULL_OBJECT(), false };
}


void VectorGraphicComponent::clearDirty() {
    // order is important.  clear the component before the graphic
    CoreComponent::clearDirty();
    auto graphic = mCalculated.get(kPropertyGraphic);
    if (graphic.is<Graphic>()) {
        auto g = graphic.get<Graphic>();
        g->clearDirty();
    }
}

std::shared_ptr<ObjectMap>
VectorGraphicComponent::createTouchEventProperties(const Point &localPoint) const
{
    std::shared_ptr<ObjectMap> eventProperties = TouchableComponent::createTouchEventProperties(localPoint);
    auto graphic = mCalculated.get(kPropertyGraphic).get<Graphic>();

    const auto& mediaBounds = mCalculated.get(kPropertyMediaBounds).get<Rect>();
    auto viewportWidth = graphic->getViewportWidth();
    auto viewportHeight = graphic->getViewportHeight();

    auto x = mediaBounds.getWidth() <= 0 ? 0 :
             (localPoint.getX() - mediaBounds.getLeft()) * (viewportWidth / mediaBounds.getWidth());
    auto y = mediaBounds.getHeight() <= 0 ? 0 :
             (localPoint.getY() - mediaBounds.getTop()) * (viewportHeight / mediaBounds.getHeight());

    auto viewportPropertyMap = std::make_shared<ObjectMap>();
    viewportPropertyMap->emplace("x", x);
    viewportPropertyMap->emplace("y", y);
    viewportPropertyMap->emplace("width", viewportWidth);
    viewportPropertyMap->emplace("height", viewportHeight);
    // Note: "event.viewport.inBounds" is an undocumented feature for the viewport object.  It has
    // been added here because the "event.inBounds" property tracks if the touch occurs inside the
    // component, not the drawing bounds of the _graphic_.
    viewportPropertyMap->emplace("inBounds", x >= 0 && y >= 0 && x <= viewportWidth && y <= viewportHeight);

    eventProperties->emplace("viewport", viewportPropertyMap);
    return eventProperties;
}

bool
VectorGraphicComponent::isFocusable() const
{
    // According to the APL specification, a Vector Graphic component should only receive keyboard
    // focus if at least one of the following handlers are defined:  onFocus, onBlur, handleKeyDown, handleKeyUp,
    // onDown, and onPress.
    return isTouchable() ||
           !getCalculated(kPropertyOnFocus).empty()       || !getCalculated(kPropertyOnBlur).empty() ||
           !getCalculated(kPropertyHandleKeyDown).empty() || !getCalculated(kPropertyHandleKeyUp).empty();
}

bool
VectorGraphicComponent::isActionable() const
{
    // Same rules as for focus
    return isFocusable();
}

bool
VectorGraphicComponent::isTouchable() const
{
    return !getCalculated(kPropertyOnDown).empty() || !getCalculated(kPropertyOnPress).empty() ||
           !getCalculated(kPropertyGestures).empty();
}

std::vector<URLRequest>
VectorGraphicComponent::getSources() {
    std::vector<URLRequest> sources;

    auto graphic = mCalculated.get(kPropertyGraphic);
    if (graphic.is<Graphic>()) {
        // Graphic is already present, nothing to load
        return sources;
    }

    auto source = mCalculated.get(kPropertySource);
    if (source.isString()) {
        auto graphicResource = mContext->getGraphic(source.getString());
        if (graphicResource.empty()) {
            // Graphic is not a local resource, treat as URI
            sources.emplace_back(URLRequest::asURLRequest(source));
        }
    } else if (source.is<URLRequest>()) {
        sources.emplace_back(source.get<URLRequest>());
    }

    return sources;
}

void
VectorGraphicComponent::postProcessLayoutChanges() {
    CoreComponent::postProcessLayoutChanges();
    MediaComponentTrait::postProcessLayoutChanges();
}

#ifdef SCENEGRAPH
/**
 * The scenegraph structure:
 *
 * Layer (CoreComponent)
 *   Layer (MediaLayer)   [bounds match the vector graphic]
 *     Transform (within MediaLayer - scales the coordinate system from graphic to screen)
 *       Graphic
 *
 * If the graphic is not valid, we still construct the mediaLayer and transform for later use.
 */
sg::LayerPtr
VectorGraphicComponent::constructSceneGraphLayer(sg::SceneGraphUpdates& sceneGraph)
{
    auto layer = TouchableComponent::constructSceneGraphLayer(sceneGraph);
    assert(layer);

    // Build the basic data structures
    auto mediaLayer = sg::layer(mUniqueId + "-mediaLayer", Rect{0,0,1,1}, 1.0f, Transform2D());
    mediaLayer->setCharacteristic(sg::Layer::kCharacteristicRenderOnly);
    auto transform = sg::transform();
    mediaLayer->setContent(transform);
    layer->appendChild(mediaLayer);
    fixMediaLayer(sceneGraph, mediaLayer);

    sceneGraph.created(mediaLayer);

    return layer;
}

bool
VectorGraphicComponent::updateSceneGraphInternal(sg::SceneGraphUpdates& sceneGraph)
{
    auto boundsChanged = isDirty(kPropertyMediaBounds);  // The media bounds changed (for example, due to layout changes)
    auto graphicDirty = isDirty(kPropertyGraphic);  // The current graphic had some property changes or it is a new graphic

    if (!boundsChanged && !graphicDirty && !mGraphicReplaced)
        return false;

    auto mediaLayer = mSceneGraphLayer->children().at(0);

    // Update the media layer and transform.  Force a re-attachment of the graphic if there is a new one
    auto validGraphic = fixMediaLayer(sceneGraph, mediaLayer);

    // If the media layer size changed, then we also need to redraw its content
    if (mediaLayer->isFlagSet(sg::Layer::kFlagSizeChanged) && mediaLayer->content())
        mediaLayer->setFlag(sg::Layer::kFlagRedrawContent);

    // If we changed the graphic, we definitely need to redraw the content
    if (mGraphicReplaced) {
        if (mediaLayer->content())
           mediaLayer->setFlag(sg::Layer::kFlagRedrawContent);
        mGraphicReplaced = false;
    }
    else if (graphicDirty && validGraphic) {  // If it didn't change but was dirty, let the update routine decide
        auto graphic = getCalculated(kPropertyGraphic).get<Graphic>();
        graphic->updateSceneGraph(sceneGraph);
    }

    // Copy over media layer flags, if set
    sceneGraph.changed(mediaLayer);

    return false;  // The vector graphic layer didn't change; the media layer did
}

/**
 * If there is a valid graphic object, fix the media layer and transform object
 * to correctly set the bounds and transformation.
 * @param sceneGraph
 * @return True if there was a valid graphic object
 */
bool
VectorGraphicComponent::fixMediaLayer(sg::SceneGraphUpdates& sceneGraph,
                                      const sg::LayerPtr& mediaLayer)
{
    auto object = getCalculated(kPropertyGraphic);
    if (!object.is<Graphic>()) {
        mediaLayer->setContent(nullptr);
        mediaLayer->removeAllChildren();
        return false;
    }

    auto graphic = object.get<Graphic>();
    if (!graphic || !graphic->isValid()) {
        mediaLayer->setContent(nullptr);
        mediaLayer->removeAllChildren();
        return false;
    }

    const auto& mediaBounds = getCalculated(kPropertyMediaBounds).get<Rect>();
    mediaLayer->setBounds(mediaBounds);

    auto transform = Transform2D::scale(mediaBounds.getWidth() / graphic->getViewportWidth(),
                                        mediaBounds.getHeight() / graphic->getViewportHeight());
    auto useLayers = getRootConfig().experimentalFeatureEnabled(RootConfig::kExperimentalFeatureGraphicLayers);
    auto fragment = graphic->getSceneGraph(useLayers, sceneGraph, mediaLayer);
    assert(fragment);

    if (fragment->isLayer()) {
        // The graphic is a layer
        auto layer = fragment->layer();

        transform = Transform2D::translate(-layer->getContentOffset()) * transform *
                    Transform2D::translate(layer->getContentOffset());
        layer->setTransform(transform);
        mediaLayer->setContent(nullptr);

        // Fix up the child layers. There should be only one.
        if (mediaLayer->children().size() != 1 || mediaLayer->children().at(0) != layer) {
            mediaLayer->removeAllChildren();
            mediaLayer->appendChild(layer);
        }
        return true;   // We have a valid graphic
    }
    else if (fragment->isNode()) {
        // Check to see if we already have a transform node in place
        auto node = fragment->node();  // The child node we're expecting

        // If we already have content, then this should be a transform node
        if (mediaLayer->content()) {
            assert(sg::TransformNode::is_type(mediaLayer->content()));
            auto t = sg::TransformNode::cast(mediaLayer->content());
            auto result = t->setTransform(transform);
            result |= t->setChild(node);
            if (result)
                mediaLayer->setFlag(sg::Layer::kFlagRedrawContent);
        }
        else {
            mediaLayer->setContent(sg::transform(transform, node));
        }
        return true;
    }
    else {  // Clear any existing content
        mediaLayer->setContent(nullptr);
        mediaLayer->removeAllChildren();
        return false;
    }
}
#endif // SCENEGRAPH

void
VectorGraphicComponent::onFail(const MediaObjectPtr& mediaObject)
{
    if (mOnLoadOnFailReported)
        return;
    auto component = getComponent();
    auto errorData = std::make_shared<ObjectMap>();
    errorData->emplace("value",  mediaObject->url());
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
VectorGraphicComponent::onLoad()
{
    if (mOnLoadOnFailReported)
        return;

    assert(mMediaObjectHolders.size() == 1);
    updateGraphic(mMediaObjectHolders.at(0).getMediaObject()->graphic());

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
