/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "apl/component/componentpropdef.h"
#include "apl/component/vectorgraphiccomponent.h"
#include "apl/component/yogaproperties.h"
#include "apl/graphic/graphic.h"

namespace apl {


CoreComponentPtr
VectorGraphicComponent::create(const ContextPtr& context,
                               Properties&& properties,
                               const std::string& path)
{
    auto ptr = std::make_shared<VectorGraphicComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

VectorGraphicComponent::VectorGraphicComponent(const ContextPtr& context,
                                               Properties&& properties,
                                               const std::string& path)
    : CoreComponent(context, std::move(properties), path)
{
}

VectorGraphicComponent::~VectorGraphicComponent()
{
    auto graphic = mCalculated.get(kPropertyGraphic);
    if (graphic.isGraphic())
        graphic.getGraphic()->clearComponent();
}

const ComponentPropDefSet&
VectorGraphicComponent::propDefSet() const
{
    static ComponentPropDefSet sVectorGraphicComponentProperties(CoreComponent::propDefSet(), {
        {kPropertyAlign,       kVectorGraphicAlignCenter, sVectorGraphicAlignMap, kPropInOut | kPropStyled},
        {kPropertyGraphic,     Object::NULL_OBJECT(),     nullptr,                kPropOut},
        {kPropertyMediaBounds, Object::NULL_OBJECT(),     nullptr,                kPropOut},
        {kPropertyScale,       kVectorGraphicScaleNone,   sVectorGraphicScaleMap, kPropInOut | kPropStyled},
        {kPropertySource,      "",                        asString,               kPropInOut},
    });

    return sVectorGraphicComponentProperties;
}

void
VectorGraphicComponent::initialize()
{
    CoreComponent::initialize();

    // Default height and width
    double width = 100;
    double height = 100;

    // If the source was specified and it is a local graphic resource, we set kPropertyGraphic
    auto source = mCalculated.get(kPropertySource);
    if (source.isString()) {
        auto graphic = mContext->getGraphic(source.getString());
        if (!graphic.empty()) {
            auto g = Graphic::create(mContext, graphic.json(), Properties(mProperties), getStyle());
            if (g) {
                g->setComponent(std::static_pointer_cast<VectorGraphicComponent>(shared_from_this()));
                mCalculated.set(kPropertyGraphic, g);
                height = g->getIntrinsicHeight();
                width = g->getIntrinsicWidth();
            }
        }
    }

    // Fix up the width if it was set to auto
    if (mCalculated.get(kPropertyWidth).isAutoDimension()) {
        YGNodeStyleSetWidth(mYGNodeRef, width);
        mCalculated.set(kPropertyWidth, Dimension(width));
    }

    // Fix up the height if it was set to auto
    if (mCalculated.get(kPropertyHeight).isAutoDimension()) {
        YGNodeStyleSetHeight(mYGNodeRef, height);
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
    if (stylePtr && graphic.isGraphic() && graphic.getGraphic()->updateStyle(stylePtr))
        setDirty(kPropertyGraphic);

    // Changing the style may result in a size change or a position change
    processLayoutChanges(true);
}

std::shared_ptr<ObjectMap>
VectorGraphicComponent::getEventTargetProperties() const
{
    auto target = CoreComponent::getEventTargetProperties();
    target->emplace("source", getCalculated(kPropertySource));
    return target;
}

bool
VectorGraphicComponent::updateGraphic(const GraphicContentPtr& json)
{
    // Remove any existing graphic
    auto graphic = mCalculated.get(kPropertyGraphic);
    if (graphic.isGraphic()) {
        graphic.getGraphic()->clearComponent();
        setDirty(kPropertyGraphic);
    }

    // TODO: Check properties here.
    auto g = Graphic::create(mContext, json, Properties(mProperties), getStyle());
    if (!g)
        return false;

    g->setComponent(std::static_pointer_cast<VectorGraphicComponent>(shared_from_this()));
    mCalculated.set(kPropertyGraphic, g);
    setDirty(kPropertyGraphic);

    // Recalculate the media bounds. This will internally do a layout
    processLayoutChanges(true);
    g->clearDirty();   // Some flags may have been set; we clear them here because this is the first use of the graphic
    return true;
}

void
VectorGraphicComponent::processLayoutChanges(bool useDirtyFlag)
{
    CoreComponent::processLayoutChanges(useDirtyFlag);

    auto graphic = mCalculated.get(kPropertyGraphic);
    if (graphic.isGraphic()) {
        auto g = graphic.getGraphic();

        auto innerBounds = mCalculated.get(kPropertyInnerBounds).getRect();
        double width = g->getIntrinsicWidth();
        double height = g->getIntrinsicHeight();
        double x = 0;
        double y = 0;
        auto scale = static_cast<VectorGraphicScale>(mCalculated.get(kPropertyScale).getInteger());
        auto align = static_cast<VectorGraphicAlign>(mCalculated.get(kPropertyAlign).getInteger());

        switch (scale) {
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

        switch (align) {
            case kVectorGraphicAlignBottom:
                x = (innerBounds.getWidth() - width) / 2;
                y = innerBounds.getHeight() - height;
                break;
            case kVectorGraphicAlignBottomLeft:
                // x = 0;
                y = innerBounds.getHeight() - height;
                break;
            case kVectorGraphicAlignBottomRight:
                x = innerBounds.getWidth() - width;
                y = innerBounds.getHeight() - height;
                break;
            case kVectorGraphicAlignCenter:
                x = (innerBounds.getWidth() - width) / 2;
                y = (innerBounds.getHeight() - height) / 2;
                break;
            case kVectorGraphicAlignLeft:
                // x = 0
                y = (innerBounds.getHeight() - height) / 2;
                break;
            case kVectorGraphicAlignRight:
                x = innerBounds.getWidth() - width;
                y = (innerBounds.getHeight() - height) / 2;
                break;
            case kVectorGraphicAlignTop:
                x = (innerBounds.getWidth() - width) / 2;
                // y = 0;
                break;
            case kVectorGraphicAlignTopLeft:
                // x = 0;
                // y = 0;
                break;
            case kVectorGraphicAlignTopRight:
                x = innerBounds.getWidth() - width;
                // y = 0;
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
VectorGraphicComponent::getVisualContextType()
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

} // namespace apl