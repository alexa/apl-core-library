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

#include "apl/engine/context.h"
#include "apl/engine/propdef.h"

#include "apl/graphic/graphic.h"
#include "apl/graphic/graphicelement.h"
#include "apl/graphic/graphicdependant.h"

#include "apl/utils/session.h"

namespace apl {

class GraphicPropDef : public PropDef<GraphicPropertyKey, sGraphicPropertyBimap>
{
public:
    GraphicPropDef(GraphicPropertyKey key, int defvalue, Bimap<int, std::string>& map, int flags)
        : PropDef(key, defvalue, map, flags) {}

    GraphicPropDef(GraphicPropertyKey key, const Object& defvalue, BindingFunction func, int flags)
        : PropDef(key, defvalue, std::move(func), flags) {}
};

class GraphicPropDefSet : public PropDefSet<GraphicPropertyKey, sGraphicPropertyBimap, GraphicPropDef >
{
public:
    GraphicPropDefSet& add(const std::vector<GraphicPropDef>& list) {
        addInternal(list);
        return *this;
    }
};


static GraphicElementPtr createChild(const GraphicPtr& graphic,
                                                   const ContextPtr& context,
                                                   const Object& json);

/**************************************************************************/

id_type GraphicElement::sUniqueGraphicIdGenerator = 1000;

GraphicElement::GraphicElement(const GraphicPtr& graphic)
    : mId(sUniqueGraphicIdGenerator++),
      mGraphic(graphic)
{
}

void
GraphicElement::addChildren(const GraphicPtr& graphic, const ContextPtr& context, const Object& json)
{
    for (auto& item : arrayifyProperty(context, json, "item", "items")) {
        auto child = createChild(graphic, context, item);
        if (child)
            mChildren.push_back(child);
    }
}

bool
GraphicElement::initialize(const ContextPtr& context, const Object& json)
{
    mProperties.emplace(json);

    for (const auto& cpd : propDefSet()) {
        const auto& pd = cpd.second;
        auto value = pd.defvalue;

        if ((pd.flags & kPropIn) != 0) {
            auto p = mProperties.find(pd.name);
            if (p != mProperties.end()) {
                // If the user assigned a string, we need to check for data-binding
                if (p->second.isString()) {
                    auto tmp = parseDataBinding(context, p->second.getString());
                    if (tmp.isEvaluable()) {
                        auto self = std::static_pointer_cast<GraphicElement>(shared_from_this());
                        GraphicDependant::create(self, pd.key, tmp, context, pd.func);
                    }
                    value = pd.calculate(*context, evaluate(*context, tmp));
                }
                else {  // Not a string - just calculate it directly
                    value = pd.calculate(*context, p->second);
                }
            }
            else {
                // If this was a required property, abort
                if ((pd.flags & kPropRequired) != 0) {
                    CONSOLE_CTP(context) << "Missing required graphic property: " << pd.name;
                    return false;
                }
            }
            mValues.set(pd.key, value);
        }
    }

    return true;
}

bool
GraphicElement::setValue(GraphicPropertyKey key, const Object& value, bool useDirtyFlag)
{
    // Assume that the property already exists
    if (mValues.get(key) == value)
        return false;

    mValues.set(key, value);
    if (useDirtyFlag && mDirtyProperties.emplace(key).second) {
        auto graphic = mGraphic.lock();
        if (graphic)
            graphic->addDirtyChild(shared_from_this());
    }

    return true;
}

rapidjson::Value
GraphicElement::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    using rapidjson::Value;
    Value v(rapidjson::kObjectType);
    v.AddMember("id", getId(), allocator);
    v.AddMember("type", static_cast<int>(getType()), allocator);
    Value props(rapidjson::kObjectType);
    for(const auto& pds : propDefSet()) {
        if ((pds.second.flags & kPropOut) != 0)
            props.AddMember(
                    rapidjson::StringRef(pds.second.name.c_str()),   // We assume long-lived strings here
                    mValues.get(pds.first).serialize(allocator),
                    allocator);
    }
    v.AddMember("props", props.Move(), allocator);
    Value children(rapidjson::kArrayType);
    for(const auto& child : mChildren) {
        children.PushBack(child->serialize(allocator), allocator);
    }
    v.AddMember("children", children.Move(), allocator);
    return v;
}

/**************************************************************************/

class GraphicElementPath : public GraphicElement {
public:
    static std::shared_ptr<GraphicElementPath> create(const GraphicPtr& graphic,
                                                      const ContextPtr& context,
                                                      const Object& json)
    {
        Properties properties;
        properties.emplace(json);

        auto path = std::make_shared<GraphicElementPath>(graphic);
        if (!path->initialize(context, json))
            return nullptr;

        return path;
    }

    explicit GraphicElementPath(const GraphicPtr& graphic) : GraphicElement(graphic) {}

    GraphicElementType getType() const override { return kGraphicElementTypePath; }

protected:
    const GraphicPropDefSet& propDefSet() const override {
        static GraphicPropDefSet sPathProperties = GraphicPropDefSet()
            .add({
                     {kGraphicPropertyFill,          Color(), asColor,             kPropInOut | kPropDynamic},
                     {kGraphicPropertyFillOpacity,   1.0,     asOpacity,           kPropInOut | kPropDynamic},
                     {kGraphicPropertyPathData,      "",      asString,            kPropInOut | kPropRequired |
                                                                                   kPropDynamic},
                     {kGraphicPropertyStroke,        Color(), asColor,             kPropInOut | kPropDynamic},
                     {kGraphicPropertyStrokeOpacity, 1.0,     asOpacity,           kPropInOut | kPropDynamic},
                     {kGraphicPropertyStrokeWidth,   1.0,     asNonNegativeNumber, kPropInOut | kPropDynamic}
                 });

        return sPathProperties;
    }
};

/**************************************************************************/

class GraphicElementGroup : public GraphicElement {
public:
    static std::shared_ptr<GraphicElementGroup> create(const GraphicPtr& graphic,
                                                       const ContextPtr& context,
                                                       const Object& json)
    {
        Properties properties;
        properties.emplace(json);

        auto group = std::make_shared<GraphicElementGroup>(graphic);
        if (!group->initialize(context, json))
            return nullptr;

        group->addChildren(graphic, context, json);
        return group;
    }

    explicit GraphicElementGroup(const GraphicPtr& graphic) : GraphicElement(graphic) {}

    GraphicElementType getType() const override { return kGraphicElementTypeGroup; }

protected:
    const GraphicPropDefSet& propDefSet() const override {
        static GraphicPropDefSet sGroupProperties = GraphicPropDefSet()
            .add({
                     {kGraphicPropertyOpacity,    1.0, asOpacity, kPropInOut | kPropDynamic},
                     {kGraphicPropertyRotation,   0,   asNumber,  kPropInOut | kPropDynamic},
                     {kGraphicPropertyPivotX,     0,   asNumber,  kPropInOut | kPropDynamic},
                     {kGraphicPropertyPivotY,     0,   asNumber,  kPropInOut | kPropDynamic},
                     {kGraphicPropertyScaleX,     1.0, asNumber,  kPropInOut | kPropDynamic},
                     {kGraphicPropertyScaleY,     1.0, asNumber,  kPropInOut | kPropDynamic},
                     {kGraphicPropertyTranslateX, 0,   asNumber,  kPropInOut | kPropDynamic},
                     {kGraphicPropertyTranslateY, 0,   asNumber,  kPropInOut | kPropDynamic},
                 });

        return sGroupProperties;
    }
};

/**************************************************************************/


class GraphicElementContainer : public GraphicElement {
public:
    static std::shared_ptr<GraphicElementContainer> create(const GraphicPtr& graphic,
                                                           const ContextPtr& context,
                                                           const Object& json)
    {
        auto container = std::make_shared<GraphicElementContainer>(graphic);
        if (!container->initialize(context, json))
            return nullptr;

        container->addChildren(graphic, context, json);
        return container;
    }

    explicit GraphicElementContainer(const GraphicPtr& graphic) : GraphicElement(graphic) {}

    GraphicElementType getType() const override { return kGraphicElementTypeContainer; }

protected:
    const GraphicPropDefSet& propDefSet() const override {
        static GraphicPropDefSet sContainerProperties = GraphicPropDefSet()
            .add({
                     {kGraphicPropertyHeightActual,           Dimension(0),      nullptr,             kPropOut},
                     {kGraphicPropertyHeightOriginal,         Dimension(100),    asAbsoluteDimension, kPropIn |
                                                                                                      kPropRequired},
                     {kGraphicPropertyScaleTypeHeight,        kGraphicScaleNone, sGraphicScaleBimap,  kPropIn},
                     {kGraphicPropertyScaleTypeWidth,         kGraphicScaleNone, sGraphicScaleBimap,  kPropIn},
                     {kGraphicPropertyViewportHeightActual,   0.0,               nullptr,             kPropOut},
                     {kGraphicPropertyViewportWidthActual,    0.0,               nullptr,             kPropOut},
                     {kGraphicPropertyVersion,                "",                asString,            kPropIn |
                                                                                                      kPropRequired},
                     {kGraphicPropertyViewportHeightOriginal, 0,                 asNonNegativeNumber, kPropIn},
                     {kGraphicPropertyViewportWidthOriginal,  0,                 asNonNegativeNumber, kPropIn},
                     {kGraphicPropertyWidthActual,            Dimension(0),      nullptr,             kPropOut},
                     {kGraphicPropertyWidthOriginal,          Dimension(100),    asAbsoluteDimension, kPropIn |
                                                                                                      kPropRequired}
                 });

        return sContainerProperties;
    }

    bool initialize(const ContextPtr& context,
                            const Object& json) override {
        if (!GraphicElement::initialize(context, json))
            return false;

        auto height = mValues.get(kGraphicPropertyHeightOriginal).getAbsoluteDimension();
        if (height <= 0) {
            CONSOLE_CTP(context) << "Invalid graphic height - must be positive";
            return false;
        }

        auto width = mValues.get(kGraphicPropertyWidthOriginal).getAbsoluteDimension();
        if (width <= 0) {
            CONSOLE_CTP(context) << "Invalid graphic width - must be positive";
            return false;
        }

        // Configure the original output values.  A later "layout" pass may update these
        mValues.set(kGraphicPropertyHeightActual, Dimension(height));
        mValues.set(kGraphicPropertyWidthActual, Dimension(width));

        // Update the viewport values.  If the viewport original values weren't set, we use the width/height
        double viewportHeight = mValues.get(kGraphicPropertyViewportHeightOriginal).getDouble();
        double viewportWidth = mValues.get(kGraphicPropertyViewportWidthOriginal).getDouble();

        if (viewportHeight <= 0) {
            viewportHeight = height;
            mValues.set(kGraphicPropertyViewportHeightOriginal, viewportHeight);
        }

        if (viewportWidth <= 0) {
            viewportWidth = width;
            mValues.set(kGraphicPropertyViewportWidthOriginal, viewportWidth);
        }

        // Configure the viewport actuals to match the originals
        mValues.set(kGraphicPropertyViewportHeightActual, viewportHeight);
        mValues.set(kGraphicPropertyViewportWidthActual, viewportWidth);

        // Update the context to include width and height (these are the viewport width and height)
        context->systemUpdateAndRecalculate("width", viewportWidth, false);
        context->systemUpdateAndRecalculate("height", viewportHeight, false);

        return true;
    }
};

static GraphicElementPtr
createChild(const GraphicPtr& graphic, const ContextPtr& context, const Object& json)
{
    if (!json.isMap()) {
        CONSOLE_CTP(context) << "Child is not a mapping type";
        return nullptr;
    }

    auto type = propertyAsString(*context, json, "type");
    if (type == "group") return GraphicElementGroup::create(graphic, context, json);
    if (type == "path") return GraphicElementPath::create(graphic, context, json);

    CONSOLE_CTP(context) << "Invalid graphic child type '" << type << "'";
    return nullptr;
}

GraphicElementPtr
GraphicElement::build(const GraphicPtr& graphic, const ContextPtr& context, const rapidjson::Value& json)
{
    return GraphicElementContainer::create(graphic, context, json);
}

void
GraphicElement::clearDirtyProperties() {
    for(const auto& child : mChildren) {
        child->clearDirtyProperties();
    }
    mDirtyProperties.clear();
}


} // namespace apl
