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

#include "apl/engine/resources.h"
#include "apl/utils/url.h"
#include "apl/component/componentpropdef.h"
#include "apl/component/vectorgraphiccomponent.h"
#include "apl/component/yogaproperties.h"
#include "apl/graphic/graphic.h"
#include "apl/media/mediamanager.h"
#include "apl/media/mediaobject.h"
#include "apl/time/sequencer.h"

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
VectorGraphicComponent::release()
{
    auto graphic = mCalculated.get(kPropertyGraphic);
    if (graphic.isGraphic())
        graphic.getGraphic()->release();

    ActionableComponent::release();
    MediaComponentTrait::release();
}

const ComponentPropDefSet&
VectorGraphicComponent::propDefSet() const
{
    static auto checkLayout = [](Component& component)
    {
        auto& vg = static_cast<VectorGraphicComponent&>(component);
        vg.processLayoutChanges(true, false);
    };

    static auto resetOnLoadOnFailFlag = [](Component& component) {
        auto& comp = dynamic_cast<VectorGraphicComponent&>(component);
        comp.mOnLoadOnFailReported = false;
    };

    static auto sVectorGraphicComponentProperties = ComponentPropDefSet(TouchableComponent::propDefSet(), MediaComponentTrait::propDefList())
        .add({
            {kPropertyAlign,       kVectorGraphicAlignCenter, sVectorGraphicAlignMap, kPropInOut | kPropStyled | kPropDynamic | kPropVisualHash, checkLayout},
            {kPropertyGraphic,     Object::NULL_OBJECT(),     nullptr,                kPropOut | kPropVisualHash},
            {kPropertyMediaBounds, Object::NULL_OBJECT(),     nullptr,                kPropOut | kPropVisualHash},
            {kPropertyOnFail,      Object::EMPTY_ARRAY(),     asCommand,              kPropIn},
            {kPropertyOnLoad,      Object::EMPTY_ARRAY(),     asCommand,              kPropIn},
            {kPropertyScale,       kVectorGraphicScaleNone,   sVectorGraphicScaleMap, kPropInOut | kPropStyled | kPropDynamic | kPropVisualHash,   checkLayout},
            {kPropertySource,      "",                        asString,               kPropInOut | kPropDynamic | kPropVisualHash,               resetOnLoadOnFailFlag},
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
                                           std::static_pointer_cast<CoreComponent>(shared_from_this()));
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
    if (graphic.isGraphic() && graphic.getGraphic()->updateStyle(stylePtr))
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
    if (!json) {
        return false;
    }
    // Remove any existing graphic
    auto graphic = mCalculated.get(kPropertyGraphic);
    if (graphic.isGraphic()) {
        graphic.getGraphic()->release();
        setDirty(kPropertyGraphic);
    }

    auto url = encodeUrl(getCalculated(kPropertySource).asString());
    auto path = Path("_url").addObject(url);
    auto g = Graphic::create(mContext,
                             json->get(),
                             Properties(mProperties),
                             std::static_pointer_cast<CoreComponent>(shared_from_this()),
                             path,
                             getStyle());
    if (!g)
        return false;

    mCalculated.set(kPropertyGraphic, g);
    setDirty(kPropertyGraphic);

    // Recalculate the media bounds. This will internally do a layout
    processLayoutChanges(true, false);
    g->clearDirty();   // Some flags may have been set; we clear them here because this is the first use of the graphic
    return true;
}

void
VectorGraphicComponent::processLayoutChanges(bool useDirtyFlag, bool first)
{
    CoreComponent::processLayoutChanges(useDirtyFlag, first);

    auto graphic = mCalculated.get(kPropertyGraphic);
    if (graphic.isGraphic()) {
        auto g = graphic.getGraphic();

        auto innerBounds = mCalculated.get(kPropertyInnerBounds).getRect();
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
        if (!mediaBounds.isRect() || r != mediaBounds.getRect()) {
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
    if (graphic.isGraphic()) {
        auto g = graphic.getGraphic();
        return g->setProperty(key, value);
    }

    return false;
}


void VectorGraphicComponent::clearDirty() {
    // order is important.  clear the component before the graphic
    Component::clearDirty();
    auto graphic = mCalculated.get(kPropertyGraphic);
    if (graphic.isGraphic()) {
        auto g = graphic.getGraphic();
        g->clearDirty();
    }
}

std::shared_ptr<ObjectMap>
VectorGraphicComponent::createTouchEventProperties(const Point &localPoint) const
{
    std::shared_ptr<ObjectMap> eventProperties = TouchableComponent::createTouchEventProperties(localPoint);
    auto graphic = mCalculated.get(kPropertyGraphic).getGraphic();

    const auto mediaBounds = mCalculated.get(kPropertyMediaBounds).getRect();
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
    return !getCalculated(kPropertyOnFocus).empty()       || !getCalculated(kPropertyOnBlur).empty() ||
           !getCalculated(kPropertyHandleKeyDown).empty() || !getCalculated(kPropertyHandleKeyUp).empty() ||
           !getCalculated(kPropertyOnDown).empty()        || !getCalculated(kPropertyOnPress).empty() ||
           !getCalculated(kPropertyGestures).empty();
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
    // Same rules as for focus
    return isFocusable();
}

std::vector<std::string>
VectorGraphicComponent::getSources() {
    std::vector<std::string> sources;

    auto graphic = mCalculated.get(kPropertyGraphic);
    if (graphic.isGraphic()) {
        // Graphic is already present, nothing to load
        return sources;
    }

    auto source = mCalculated.get(kPropertySource);
    if (source.isString()) {
        auto graphicResource = mContext->getGraphic(source.getString());
        if (graphicResource.empty()) {
            // Graphic is not a local resource, treat as URI
            sources.emplace_back(source.getString());
        }
    }

    return sources;
}

void
VectorGraphicComponent::postProcessLayoutChanges() {
    CoreComponent::postProcessLayoutChanges();
    MediaComponentTrait::postProcessLayoutChanges();
}

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
