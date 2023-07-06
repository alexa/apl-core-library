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

#include "apl/graphic/graphicelement.h"

#include "apl/component/corecomponent.h"
#include "apl/engine/propdef.h"
#include "apl/engine/typeddependant.h"
#include "apl/graphic/graphic.h"
#include "apl/graphic/graphicpattern.h"
#include "apl/graphic/graphicpropdef.h"
#include "apl/primitives/color.h"
#include "apl/primitives/gradient.h"
#include "apl/primitives/transform2d.h"
#include "apl/utils/session.h"

#ifdef SCENEGRAPH
#include "apl/scenegraph/builder.h"
#include "apl/scenegraph/graphicfragment.h"
#include "apl/scenegraph/scenegraphupdates.h"
#endif // SCENEGRAPH

namespace apl {

using GraphicDependant = TypedDependant<GraphicElement, GraphicPropertyKey>;

Object
GraphicElement::asAvgFill(const Context& context, const Object& object) {
    if (object.is<GraphicPattern>() || object.is<Gradient>() || object.is<Color>())
        return object;

    if (object.isMap()) {
        auto gradient = asAvgGradient(context, object);
        if (gradient.is<Gradient>()) return gradient;
    }

    return asColor(context, object);
}

/**************************************************************************/

GraphicElement::GraphicElement(const GraphicPtr& graphic, const ContextPtr& context)
    : UIDObject(context, UIDObject::UIDObjectType::GRAPHIC_ELEMENT),
      mGraphic(graphic)
{
}

id_type
GraphicElement::getId() const {
    if (mCachedTempId <= 0) {
        auto result = getUniqueId();
        result.erase(0,1);
        mCachedTempId = std::stoi(result);
    }

    return mCachedTempId;
}


bool
GraphicElement::initialize(const GraphicPtr& graphic, const Object& json)
{
    mProperties.emplace(json);
    mStyle = mProperties.asString(*mContext, "style", "");
    auto stylePtr = getStyle(graphic);

    for (const auto& cpd : propDefSet()) {
        const auto& pd = cpd.second;
        auto defValue = pd.defaultFunc ? pd.defaultFunc(*this, mContext->getRootConfig()) : pd.defvalue;
        auto value = defValue;

        if ((pd.flags & kPropIn) != 0) {
            auto p = mProperties.find(pd.names);
            if (p != mProperties.end()) {
                if (p->second.isString() || (pd.flags & kPropEvaluated) != 0) {
                    auto result = parseAndEvaluate(*mContext, p->second);
                    if (!result.symbols.empty() && mGraphic.lock())
                        GraphicDependant::create(shared_from_this(), pd.key,
                                                 std::move(result.expression),
                                                 mContext, pd.getBindingFunction(),
                                                 std::move(result.symbols));
                    value = pd.calculate(*mContext, result.value);
                }
                else {
                    value = pd.calculate(*mContext, p->second);
                }
                mAssigned.emplace(pd.key);
            }
            else {
                // Check for a styled property, every property is styled in Graphic.
                if (stylePtr) {
                    auto s = stylePtr->find(pd.names);
                    if (s != stylePtr->end())
                        value = pd.calculate(*mContext, s->second);
                }

                // If this was a required property, and not in style, abort
                if ((pd.flags & kPropRequired) != 0 && (value == defValue)) {
                    CONSOLE(mContext) << "Missing required graphic property: " << pd.names;
                    return false;
                }
            }
        }

        mValues.set(pd.key, value);
    }

    return true;
}

std::string
GraphicElement::getLang() const {
    auto graphic = mGraphic.lock();
    if (!graphic) { return ""; }

    return graphic->getRoot()->getValue(kGraphicPropertyLang).asString();
}

GraphicLayoutDirection
GraphicElement::getLayoutDirection() const {
    auto graphic = mGraphic.lock();
    if (!graphic) { return kGraphicLayoutDirectionLTR; }

    return static_cast<GraphicLayoutDirection>(
        graphic->getRoot()->getValue(kGraphicPropertyLayoutDirection).asInt());
}

void
GraphicElement::setValue(GraphicPropertyKey key, const Object& value, bool useDirtyFlag)
{
    // Assume that the property already exists
    if (mValues.get(key) == value)
        return;

    // See if we have it at all
    const auto& pds = propDefSet();
    auto it = pds.find(key);

    // There shouldn't be a way to set a graphic property value that's not in the PDS.
    assert(it != pds.end());

    mValues.set(key, value);

    if (it->second.trigger != nullptr)
        it->second.trigger(*this);

    if (useDirtyFlag && (it->second.flags & kPropOut) && mDirtyProperties.emplace(key).second) {
        markAsDirty();
    }
}

rapidjson::Value
GraphicElement::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    using rapidjson::Value;
    Value v(rapidjson::kObjectType);
    v.AddMember("id", rapidjson::Value(getUniqueId().c_str(), allocator), allocator);
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
#ifdef SCENEGRAPH
    v.AddMember("layer", rapidjson::Value(mContainingLayer ? mContainingLayer->getName().c_str() : "<unset>", allocator), allocator);
#endif
    return v;
}

void
GraphicElement::clearDirtyProperties() {
    // TODO: Why are we clearing the children?  Shouldn't this be handled by the Graphic?
    for(const auto& child : mChildren) {
        child->clearDirtyProperties();
    }
    mDirtyProperties.clear();
}

StyleInstancePtr
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
GraphicElement::updateStyleInternal(const StyleInstancePtr& stylePtr,
                                    const GraphicPropDefSet& gds)
{
    for (const auto& it : gds) {
        const GraphicPropDef& pd = it.second;

        // If the property was explicitly assigned by the user, the style won't change it.
        if (mAssigned.count(pd.key))
            continue;

        // Check to see if the value has changed.
        auto value = pd.defaultFunc ? pd.defaultFunc(*this, mContext->getRootConfig()) : pd.defvalue;
        auto s = stylePtr->find(pd.names);
        if (s != stylePtr->end())
            value = pd.calculate(*mContext, s->second);

        setValue(pd.key, value, true);
    }
}

void
GraphicElement::updateStyle(const GraphicPtr& graphic)
{
    auto stylePtr = getStyle(graphic);
    if (stylePtr) {
        updateStyleInternal(stylePtr, propDefSet());
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

    const auto& currentTransform = mValues.get(outKey).get<Transform2D>();

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
    element.updateTransform(*element.mContext, kGraphicPropertyFillTransformAssigned,
            kGraphicPropertyFillTransform, true);
}

void
GraphicElement::fixStrokeTransform(GraphicElement& element) {
    element.updateTransform(*element.mContext, kGraphicPropertyStrokeTransformAssigned,
            kGraphicPropertyStrokeTransform, true);
}

void
GraphicElement::release() {
    RecalculateTarget::removeUpstreamDependencies();

#ifdef SCENEGRAPH
    mContainingLayer = nullptr;
#endif

    for (auto& child : mChildren) {
        child->release();
    }
}

#ifdef SCENEGRAPH

/**
 * Construct a GraphicFragment for this element that contains all of the GraphicFragments
 * from child elements.
 *
 * Two child fragments may be merged if they match "sufficiently well".  Refer to the
 * GraphicFragment::mergeWith() method for details.
 */
sg::GraphicFragmentPtr
GraphicElement::ensureSceneGraphChildren(bool allowLayers, sg::SceneGraphUpdates& sceneGraph)
{
    auto result = sg::GraphicFragment::create(shared_from_this());
    sg::GraphicFragmentPtr accumulator;

    for (auto& childElement : mChildren) {
        auto fragment = childElement->buildSceneGraph(allowLayers, sceneGraph);
        if (!accumulator)
            accumulator = fragment;
        else if (!accumulator->mergeWith(fragment)) {
            // Failed to merge
            result->ensureLayer(sceneGraph);
            result->addChild(accumulator, sceneGraph);
            accumulator = fragment;
        }
    }

    result->addChild(accumulator, sceneGraph);
    return result;
}

void
GraphicElement::assignSceneGraphLayer(const sg::LayerPtr& containingLayer)
{
    mContainingLayer = containingLayer;
}

void
GraphicElement::requestRedraw(sg::SceneGraphUpdates& sceneGraph)
{
    if (mContainingLayer) {
        mContainingLayer->setFlag(sg::Layer::kFlagRedrawContent);
        sceneGraph.changed(mContainingLayer);
    }
}

void
GraphicElement::requestSizeCheck(sg::SceneGraphUpdates& sceneGraph)
{
    if (mContainingLayer)
        sceneGraph.resize(mContainingLayer);
}

/**
 * Decide if a certain graphic property should be included in the scene graph content node.  For
 * example, a path with a stroke operation should be included in the scene graph iff (a) the
 * stroke color/gradient/pattern is not transparent, (b) the stroke width is > 0, and (c) the
 * stroke opacity > 0; if any of these are false, there's no point in adding the stroke to the
 * scene graph because nothing will be drawn.
 *
 * Note that having an upstream driver for one of these properties means that it _could_ be
 * valid in the future, and so this method will return true without checking the current value.
 *
 * This method is not useful for arbitrary keys; it is meaningless for properties like
 * kGraphicPropertyScaleX or kGraphicPropertyFontStyle.
 *
 * @param key The graphic key to examine for this element
 * @return True if the key doesn't block drawing
 */
bool
GraphicElement::includeInSceneGraph(GraphicPropertyKey key)
{
    if (hasUpstream(key))
        return true;

    auto value = getValue(key);

    // Numbers are used for opacity and stroke width
    if (value.isNumber())
        return value.asNumber() > 0;

    // Fills and strokes are either colors, gradients, or patterns
    if (value.is<Color>())
        return Color(value.getColor()).alpha() != 0;

    if (value.is<Gradient>()) {
        auto colors = value.get<Gradient>().getColorRange();
        return std::any_of(colors.begin(), colors.end(), [](Color c){ return c.alpha() != 0; });
    }

    // Assume patterns render and everything else doesn't
    return value.is<GraphicPattern>();
}

#endif // SCENEGRAPH

} // namespace apl
