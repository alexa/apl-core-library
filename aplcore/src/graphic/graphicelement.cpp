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

#include "apl/engine/styles.h"
#include "apl/engine/resources.h"
#include "apl/component/componentproperties.h"
#include "apl/engine/context.h"
#include "apl/engine/propdef.h"
#include "apl/component/corecomponent.h"
#include "apl/primitives/transform2d.h"

#include "apl/graphic/graphic.h"
#include "apl/graphic/graphicelement.h"
#include "apl/graphic/graphicdependant.h"

#include "apl/utils/session.h"

#include "apl/graphic/graphicpropdef.h"

namespace apl {

static GraphicElementPtr createChild(const GraphicPtr& graphic,
                                     const ContextPtr& context,
                                     const Object& json);

static Object asAvgFill(const Context& context, const Object& object) {
    if (object.isGraphicPattern() || object.isGradient() || object.isColor())
        return object;

    if (object.isMap()) {
        auto gradient = asAvgGradient(context, object);
        if (gradient.isGradient()) return gradient;
    }

    return asColor(context, object);
}

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
    for (auto& item : arrayifyProperty(*context, json, "item", "items")) {
        auto child = createChild(graphic, context, item);
        if (child)
            mChildren.push_back(child);
    }
}

bool
GraphicElement::initialize(const GraphicPtr& graphic, const ContextPtr& context, const Object& json)
{
    mProperties.emplace(json);
    mStyle = mProperties.asString(*context, "style", "");
    auto stylePtr = getStyle(graphic);

    for (const auto& cpd : propDefSet()) {
        const auto& pd = cpd.second;
        auto value = pd.defvalue;

        if ((pd.flags & kPropIn) != 0) {
            auto p = mProperties.find(pd.names);
            if (p != mProperties.end()) {
                // If the user assigned a string, we need to check for data-binding
                if (p->second.isString()) {
                    auto tmp = parseDataBinding(*context, p->second.getString());
                    // Can be standalone without parent -> no dependency in such case.
                    if (mGraphic.lock() && tmp.isEvaluable()) {
                        auto self = std::static_pointer_cast<GraphicElement>(shared_from_this());
                        GraphicDependant::create(self, pd.key, tmp, context, pd.func);
                    }
                    value = pd.calculate(*context, evaluate(*context, tmp));
                } else if (mGraphic.lock() && (pd.flags & kPropEvaluated) != 0) {
                    // Explicitly marked for evaluation, so do it.
                    // Will not attach dependant if no valid symbols.
                    auto tmp = parseDataBindingRecursive(*context, p->second);
                    auto self = std::static_pointer_cast<GraphicElement>(shared_from_this());
                    GraphicDependant::create(self, pd.key, tmp, context, pd.getBindingFunction());
                    value = pd.calculate(*context, p->second);
                } else {  // Not a string - just calculate it directly
                    value = pd.calculate(*context, p->second);
                }
                mAssigned.emplace(pd.key);
            }
            else {
                // Check for a styled property, every property is styled in Graphic.
                if (stylePtr) {
                    auto s = stylePtr->find(pd.names);
                    if (s != stylePtr->end())
                        value = pd.calculate(*context, s->second);
                }

                // If this was a required property, and not in style, abort
                if ((pd.flags & kPropRequired) != 0 && (value == pd.defvalue)) {
                    CONSOLE_CTP(context) << "Missing required graphic property: " << pd.names;
                    return false;
                }


            }
        }

        mValues.set(pd.key, value);
    }

    return true;
}

bool
GraphicElement::setValue(GraphicPropertyKey key, const Object& value, bool useDirtyFlag)
{
    // Assume that the property already exists
    if (mValues.get(key) == value)
        return false;

    // See if we have it at all
    auto pds = propDefSet();
    auto it = pds.find(key);

    // There shouldn't be a way to set a graphic property value that's not in the PDS.
    assert(it != pds.end());

    mValues.set(key, value);

    if (it->second.trigger != nullptr)
        it->second.trigger(*this);

    if (useDirtyFlag && (it->second.flags & kPropOut) && mDirtyProperties.emplace(key).second) {
        markAsDirty();
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
                    rapidjson::StringRef(pds.second.names[0].c_str()),   // We assume long-lived strings here
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

void
GraphicElement::clearDirtyProperties() {
    for(const auto& child : mChildren) {
        child->clearDirtyProperties();
    }
    mDirtyProperties.clear();
}

const StyleInstancePtr
GraphicElement::getStyle(const GraphicPtr& graphic) const
{
    if (!mStyle.empty()) {
        auto parentComponent = graphic->mComponent.lock();
        State state;
        if (parentComponent) {
            state = parentComponent->getState();
        }

        return graphic->styles()->get(graphic->getContext(), mStyle, state);
    }
    return nullptr;
}

void
GraphicElement::updateStyleInternal(
        const GraphicPtr& graphic,
        const StyleInstancePtr& stylePtr,
        const GraphicPropDefSet& gds) {
    for (const auto& it : gds) {
        const GraphicPropDef& pd = it.second;

        // If the property was explicitly assigned by the user, the style won't change it.
        if (mAssigned.count(pd.key))
            continue;

        // Check to see if the value has changed.
        auto value = pd.defvalue;
        auto s = stylePtr->find(pd.names);
        if (s != stylePtr->end())
            value = pd.calculate(*graphic->getContext(), s->second);

        setValue(pd.key, value, true);
    }
}

void
GraphicElement::updateStyle(const GraphicPtr& graphic) {
    auto stylePtr = getStyle(graphic);
    if (stylePtr) {
        updateStyleInternal(graphic, stylePtr, propDefSet());
    }

    for (const auto& child : mChildren) {
        child->updateStyle(graphic);
    }
}

void
GraphicElement::markAsDirty() {
    auto graphic = mGraphic.lock();
    if (graphic)
        graphic->addDirtyChild(shared_from_this());
}

void
GraphicElement::updateTransform(const Context& context, GraphicPropertyKey inKey, GraphicPropertyKey outKey, bool useDirtyFlag)
{
    auto inTransformStr = mValues.get(inKey).asString();
    auto inTransform = inTransformStr.empty()
                       ? Transform2D()
                       : Transform2D::parse(context.session(), inTransformStr);

    auto currentTransform = mValues.get(outKey).getTransform2D();

    if (inTransform != currentTransform) {
        mValues.set(outKey, Object(std::move(inTransform)));
        if (useDirtyFlag) {
            mDirtyProperties.emplace(outKey);
            markAsDirty();
        }
    }
}

void
GraphicElement::fixFillTransform(GraphicElement& element) {
    auto graphic = element.mGraphic.lock();
    if (!graphic) {
        return;
    }
    element.updateTransform(*graphic->getContext(), kGraphicPropertyFillTransformAssigned,
            kGraphicPropertyFillTransform, true);
}

void
GraphicElement::fixStrokeTransform(GraphicElement& element) {
    auto graphic = element.mGraphic.lock();
    if (!graphic) {
        return;
    }
    element.updateTransform(*graphic->getContext(), kGraphicPropertyStrokeTransformAssigned,
            kGraphicPropertyStrokeTransform, true);
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
        if (!path->initialize(graphic, context, json))
            return nullptr;

        return path;
    }

    explicit GraphicElementPath(const GraphicPtr& graphic) : GraphicElement(graphic) {}

    GraphicElementType getType() const override { return kGraphicElementTypePath; }

    std::string toDebugString() const override { return "GraphicElementPath<>"; }

protected:
    const GraphicPropDefSet& propDefSet() const override {
        static GraphicPropDefSet sPathProperties = GraphicPropDefSet()
            .add({
                     {kGraphicPropertyFill,                    Color(),               asAvgFill,             kPropInOut | kPropDynamic | kPropEvaluated},
                     {kGraphicPropertyFillOpacity,             1.0,                   asOpacity,             kPropInOut | kPropDynamic},
                     {kGraphicPropertyFillTransform,           Object::IDENTITY_2D(), nullptr,               kPropOut},
                     {kGraphicPropertyFillTransformAssigned,   "",                    asString,              kPropIn    | kPropDynamic, fixFillTransform},
                     {kGraphicPropertyPathData,                "",                    asString,              kPropInOut | kPropRequired | kPropDynamic},
                     {kGraphicPropertyPathLength,              0,                     asNumber,              kPropInOut | kPropDynamic},
                     {kGraphicPropertyStroke,                  Color(),               asAvgFill,             kPropInOut | kPropDynamic | kPropEvaluated},
                     {kGraphicPropertyStrokeDashArray,         Object::EMPTY_ARRAY(), asDashArray,           kPropInOut | kPropDynamic | kPropEvaluated},
                     {kGraphicPropertyStrokeDashOffset,        0,                     asNumber,              kPropInOut | kPropDynamic},
                     {kGraphicPropertyStrokeLineCap,           kGraphicLineCapButt,   sGraphicLineCapBimap,  kPropInOut | kPropDynamic},
                     {kGraphicPropertyStrokeLineJoin,          kGraphicLineJoinMiter, sGraphicLineJoinBimap, kPropInOut | kPropDynamic},
                     {kGraphicPropertyStrokeMiterLimit,        4,                     asNumber,              kPropInOut | kPropDynamic},
                     {kGraphicPropertyStrokeOpacity,           1.0,                   asOpacity,             kPropInOut | kPropDynamic},
                     {kGraphicPropertyStrokeTransform,         Object::IDENTITY_2D(), nullptr,               kPropOut},
                     {kGraphicPropertyStrokeTransformAssigned, "",                    asString,              kPropIn    | kPropDynamic, fixStrokeTransform},
                     {kGraphicPropertyStrokeWidth,             1.0,                   asNonNegativeNumber,   kPropInOut | kPropDynamic}
                 });

        return sPathProperties;
    }

    bool initialize(const GraphicPtr& graphic, const ContextPtr& context, const Object& json) override {
        if (!GraphicElement::initialize(graphic, context, json))
            return false;

        // Update output transforms
        updateTransform(*context, kGraphicPropertyFillTransformAssigned, kGraphicPropertyFillTransform, false);
        updateTransform(*context, kGraphicPropertyStrokeTransformAssigned, kGraphicPropertyStrokeTransform, false);

        return true;
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
        if (!group->initialize(graphic, context, json))
            return nullptr;  // this fork is impossible to reach since group has no required properties

        group->addChildren(graphic, context, json);
        return group;
    }

    explicit GraphicElementGroup(const GraphicPtr& graphic) : GraphicElement(graphic) {}

    GraphicElementType getType() const override { return kGraphicElementTypeGroup; }


    std::string toDebugString() const override
    {
        std::string result = "GraphicElementGroup<children=[";
        for (auto& child : mChildren) {
            result += " ";
            result += child->toDebugString();
        }
        result +=  ">";

        return result;
    }

protected:
    const GraphicPropDefSet& propDefSet() const override {
        static GraphicPropDefSet sGroupProperties = GraphicPropDefSet()
            .add({
                     {kGraphicPropertyClipPath,          "",                    asString,  kPropInOut | kPropDynamic},
                     {kGraphicPropertyOpacity,           1.0,                   asOpacity, kPropInOut | kPropDynamic},
                     {kGraphicPropertyTransform,         Object::IDENTITY_2D(), nullptr,   kPropOut},
                     {kGraphicPropertyTransformAssigned, "",                    asString,  kPropIn    | kPropDynamic, fixTransform},
                     {kGraphicPropertyRotation,          0,                     asNumber,  kPropIn    | kPropDynamic, fixTransform},
                     {kGraphicPropertyPivotX,            0,                     asNumber,  kPropIn    | kPropDynamic, fixTransform},
                     {kGraphicPropertyPivotY,            0,                     asNumber,  kPropIn    | kPropDynamic, fixTransform},
                     {kGraphicPropertyScaleX,            1.0,                   asNumber,  kPropIn    | kPropDynamic, fixTransform},
                     {kGraphicPropertyScaleY,            1.0,                   asNumber,  kPropIn    | kPropDynamic, fixTransform},
                     {kGraphicPropertyTranslateX,        0,                     asNumber,  kPropIn    | kPropDynamic, fixTransform},
                     {kGraphicPropertyTranslateY,        0,                     asNumber,  kPropIn    | kPropDynamic, fixTransform},
                 });

        return sGroupProperties;
    }

    bool initialize(const GraphicPtr& graphic, const ContextPtr& context, const Object& json) override {
        if (!GraphicElement::initialize(graphic, context, json))
            return false;

        // Update output transforms
        updateTransform(*context, false);

        return true;
    }

    void
    updateTransform(const Context& context, bool useDirtyFlag)
    {
        Transform2D updated;

        auto inTransformStr = mValues.get(kGraphicPropertyTransformAssigned).asString();

        Transform2D outTransform;
        if (!inTransformStr.empty()) {
            outTransform = inTransformStr.empty() ? Transform2D()
                                                  : Transform2D::parse(context.session(), inTransformStr);
        } else {
            auto scaleX = mValues.get(kGraphicPropertyScaleX).asNumber();
            auto scaleY = mValues.get(kGraphicPropertyScaleY).asNumber();
            auto pivotX = mValues.get(kGraphicPropertyPivotX).asNumber();
            auto pivotY = mValues.get(kGraphicPropertyPivotY).asNumber();
            auto rotation = mValues.get(kGraphicPropertyRotation).asNumber();
            auto translateX = mValues.get(kGraphicPropertyTranslateX).asNumber();
            auto translateY = mValues.get(kGraphicPropertyTranslateY).asNumber();

            // Remember that transformations apply from right-to-left.  The documented order is:
            // "translate(tx ty) translate(px py) rotate(rotation) scale(sx sy) translate(-px -py)"
            // This follows the common model of scale and rotate about the pivot point, followed by translation.

            outTransform = Transform2D::translate(translateX + pivotX, translateY + pivotY);
            outTransform *= Transform2D::rotate(rotation);
            outTransform *= Transform2D::scale(scaleX, scaleY);
            outTransform *= Transform2D::translate(-pivotX, -pivotY);
        }

        if (outTransform != mValues.get(kGraphicPropertyTransform).getTransform2D()) {
            // TODO: We do not explicitly update "single property" values as going to deprecate them to kPropIn-only
            //  after viewhosts act on that.
            mValues.set(kGraphicPropertyTransform, Object(std::move(outTransform)));
            if (useDirtyFlag) {
                mDirtyProperties.emplace(kGraphicPropertyTransform);
                markAsDirty();
            }
        }
    }

    static void fixTransform(GraphicElement& element) {
        auto& group = static_cast<GraphicElementGroup&>(element);
        auto graphic = group.mGraphic.lock();
        if (!graphic) {
            return;
        }
        group.updateTransform(*graphic->getContext(), true);
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
        if (!container->initialize(graphic, context, json))
            return nullptr;

        container->addChildren(graphic, context, json);
        return container;
    }

    explicit GraphicElementContainer(const GraphicPtr& graphic) : GraphicElement(graphic) {}

    GraphicElementType getType() const override { return kGraphicElementTypeContainer; }

    std::string toDebugString() const override
    {
        std::string result = "GraphicElementContainer<children=[";
        for (auto& child : mChildren) {
            result += " ";
            result += child->toDebugString();
        }
        result +=  ">";

        return result;
    }

protected:
    const GraphicPropDefSet& propDefSet() const override {
        static GraphicPropDefSet sContainerProperties = GraphicPropDefSet()
            .add({
                     {kGraphicPropertyHeightActual,           Dimension(0),      nullptr,              kPropOut},
                     {kGraphicPropertyHeightOriginal,         Dimension(100),    asAbsoluteDimension,  kPropIn | kPropRequired},
                     {kGraphicPropertyScaleTypeHeight,        kGraphicScaleNone, sGraphicScaleBimap,   kPropIn},
                     {kGraphicPropertyScaleTypeWidth,         kGraphicScaleNone, sGraphicScaleBimap,   kPropIn},
                     {kGraphicPropertyViewportHeightActual,   0.0,               nullptr,              kPropOut},
                     {kGraphicPropertyViewportWidthActual,    0.0,               nullptr,              kPropOut},
                     {kGraphicPropertyVersion,                kGraphicVersion11, sGraphicVersionBimap, kPropIn | kPropRequired},
                     {kGraphicPropertyViewportHeightOriginal, 0,                 asNonNegativeNumber,  kPropIn},
                     {kGraphicPropertyViewportWidthOriginal,  0,                 asNonNegativeNumber,  kPropIn},
                     {kGraphicPropertyWidthActual,            Dimension(0),      nullptr,              kPropOut},
                     {kGraphicPropertyWidthOriginal,          Dimension(100),    asAbsoluteDimension,  kPropIn | kPropRequired}
                 });

        return sContainerProperties;
    }

    bool initialize(const GraphicPtr& graphic, const ContextPtr& context, const Object& json) override {
        if (!GraphicElement::initialize(graphic, context, json))
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

/**************************************************************************/

class GraphicElementText : public GraphicElement {
public:
    static std::shared_ptr<GraphicElementText> create(const GraphicPtr &graphic,
                                                      const ContextPtr &context,
                                                      const Object &json)
    {
        Properties properties;
        properties.emplace(json);

        auto text = std::make_shared<GraphicElementText>(graphic);
        if (!text->initialize(graphic, context, json))
            return nullptr;

        return text;
    }

    explicit GraphicElementText(const GraphicPtr& graphic) : GraphicElement(graphic) {}

    GraphicElementType getType() const override { return kGraphicElementTypeText; }

    std::string toDebugString() const override
    {
        return "GraphicElementText<text=" + getValue(kGraphicPropertyText).asString() +">";
    }

protected:
    const GraphicPropDefSet& propDefSet() const override {
        static GraphicPropDefSet sTextProperties = GraphicPropDefSet()
            .add({
                     {kGraphicPropertyFill,                    Color(Color::BLACK),     asAvgFill,               kPropInOut | kPropDynamic | kPropEvaluated},
                     {kGraphicPropertyFillOpacity,             1,                       asOpacity,               kPropInOut | kPropDynamic},
                     {kGraphicPropertyFillTransform,           Object::IDENTITY_2D(),   nullptr,                 kPropOut},
                     {kGraphicPropertyFillTransformAssigned,   "",                      asString,                kPropIn    | kPropDynamic, fixFillTransform},
                     {kGraphicPropertyFontFamily,              "sans-serif",            asString,                kPropInOut | kPropDynamic},
                     {kGraphicPropertyFontSize,                40,                      asNonNegativeNumber,     kPropInOut | kPropDynamic},
                     {kGraphicPropertyFontStyle,               kFontStyleNormal,        sFontStyleMap,           kPropInOut | kPropDynamic},
                     {kGraphicPropertyFontWeight,              400,                     sFontWeightMap,          kPropInOut | kPropDynamic},
                     {kGraphicPropertyLetterSpacing,           0,                       asNumber,                kPropInOut | kPropDynamic},
                     {kGraphicPropertyStroke,                  Color(),                 asAvgFill,               kPropInOut | kPropDynamic | kPropEvaluated},
                     {kGraphicPropertyStrokeOpacity,           1,                       asOpacity,               kPropInOut | kPropDynamic},
                     {kGraphicPropertyStrokeTransform,         Object::IDENTITY_2D(),   nullptr,                 kPropOut},
                     {kGraphicPropertyStrokeTransformAssigned, "",                      asString,                kPropIn    | kPropDynamic, fixStrokeTransform},
                     {kGraphicPropertyStrokeWidth,             0,                       asNonNegativeNumber,     kPropInOut | kPropDynamic},
                     {kGraphicPropertyText,                    "",                      asFilteredText,          kPropInOut | kPropRequired | kPropDynamic},
                     {kGraphicPropertyTextAnchor,              kGraphicTextAnchorStart, sGraphicTextAnchorBimap, kPropInOut | kPropDynamic},
                     {kGraphicPropertyCoordinateX,             0,                       asNumber,                kPropInOut | kPropDynamic},
                     {kGraphicPropertyCoordinateY,             0,                       asNumber,                kPropInOut | kPropDynamic}
                 });

        return sTextProperties;
    }

    bool initialize(const GraphicPtr& graphic, const ContextPtr& context, const Object& json) override {
        if (!GraphicElement::initialize(graphic, context, json))
            return false;

        // Update output transforms
        updateTransform(*context, kGraphicPropertyFillTransformAssigned, kGraphicPropertyFillTransform, false);
        updateTransform(*context, kGraphicPropertyStrokeTransformAssigned, kGraphicPropertyStrokeTransform, false);

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
    if (type == "text") return GraphicElementText::create(graphic, context, json);

    CONSOLE_CTP(context) << "Invalid graphic child type '" << type << "'";
    return nullptr;
}

GraphicElementPtr
GraphicElement::build(const GraphicPtr& graphic, const Object& json)
{
    return GraphicElementContainer::create(graphic, graphic->getContext(), json);
}

GraphicElementPtr
GraphicElement::build(const Context& context, const Object& json)
{
    return createChild(nullptr, std::make_shared<Context>(context), json);
}

} // namespace apl
