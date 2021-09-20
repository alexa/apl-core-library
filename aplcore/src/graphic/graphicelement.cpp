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

#include "apl/graphic/graphic.h"

#include "apl/graphic/graphicdependant.h"
#include "apl/graphic/graphicpropdef.h"

#include "apl/utils/session.h"

namespace apl {


Object
GraphicElement::asAvgFill(const Context& context, const Object& object) {
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

GraphicElement::GraphicElement(const GraphicPtr& graphic, const ContextPtr& context)
    : mId(sUniqueGraphicIdGenerator++),
      mGraphic(graphic),
      mContext(context)
{
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
                // If the user assigned a string, we need to check for data-binding
                if (p->second.isString()) {
                    auto tmp = parseDataBinding(*mContext, p->second.getString());
                    // Can be standalone without parent -> no dependency in such case.
                    if (mGraphic.lock() && tmp.isEvaluable()) {
                        auto self = std::static_pointer_cast<GraphicElement>(shared_from_this());
                        GraphicDependant::create(self, pd.key, tmp, mContext, pd.getBindingFunction());
                    }
                    value = pd.calculate(*mContext, evaluate(*mContext, tmp));
                } else if (mGraphic.lock() && (pd.flags & kPropEvaluated) != 0) {
                    // Explicitly marked for evaluation, so do it.
                    // Will not attach dependant if no valid symbols.
                    auto tmp = parseDataBindingRecursive(*mContext, p->second);
                    auto self = std::static_pointer_cast<GraphicElement>(shared_from_this());
                    GraphicDependant::create(self, pd.key, tmp, mContext, pd.getBindingFunction());
                    value = pd.calculate(*mContext, p->second);
                } else {  // Not a string - just calculate it directly
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
                    CONSOLE_CTP(mContext) << "Missing required graphic property: " << pd.names;
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
GraphicElement::updateStyleInternal(
        const StyleInstancePtr& stylePtr,
        const GraphicPropDefSet& gds) {
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
GraphicElement::updateStyle(const GraphicPtr& graphic) {
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
    for (auto& child : mChildren) {
        child->release();
    }
}


} // namespace apl
