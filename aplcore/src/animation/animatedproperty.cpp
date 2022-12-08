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

#include "apl/animation/animatedproperty.h"
#include "apl/component/corecomponent.h"
#include "apl/engine/evaluate.h"
#include "apl/engine/arrayify.h"
#include "apl/utils/session.h"

namespace apl {

std::unique_ptr<AnimatedProperty>
AnimatedProperty::create(const ContextPtr& context,
                         const CoreComponentPtr& component,
                         const Object& object)
{
    if (!object.isMap()) {
        CONSOLE(context) << "Unrecognized animation command" << object;
        return nullptr;
    }

    auto property = propertyAsString(*context, object, "property");

    if (!object.has("to")) {
        CONSOLE(context) << "Animation property has no 'to' value '" << property << "'";
        return nullptr;
    }

    auto propRef = component->getPropertyAndWriteableState(property);
    if (!propRef.second) {
        CONSOLE(context) << "Unusable animation property '" << property << "'";
        return nullptr;
    }

    // If we find a key, we can speed up the animation process
    auto key = static_cast<PropertyKey>(sComponentPropertyBimap.get(property, -1));
    if (key == kPropertyTransformAssigned) {
        if (!object.has("from")) {
            CONSOLE(context) << "Animated transforms need a 'from' property";
            return nullptr;
        }

        return std::make_unique<AnimatedTransform>(
            InterpolatedTransformation::create(*context, arrayifyProperty(*context, object, "from"),
                                               arrayifyProperty(*context, object, "to")));
    }

    // The only other assigned key we can animate is opacity
    if (key != static_cast<PropertyKey>(-1) && key != kPropertyOpacity) {
        CONSOLE(context) << "Unable to animate property '" << property << "'";
        return nullptr;
    }

    if (!propRef.first.isNumber()) {
        CONSOLE(context) << "Only numbers and transforms can be animated '" << property << "'";
        return nullptr;
    }

    auto to = propertyAsDouble(*context, object, "to", 0);
    auto from = object.has("from") ? propertyAsDouble(*context, object, "from", 0)
                                   : propRef.first.asNumber();

    return AnimatedDouble::create(key, property, from, to);
}

void
AnimatedDouble::update(const CoreComponentPtr& component, float alpha)
{
    double value = mFrom * (1 - alpha) + mTo * alpha;
    if (mKey != static_cast<PropertyKey>(-1))
        component->setProperty(mKey, value);
    else
        component->setProperty(mProperty, value);
}


AnimatedTransform::AnimatedTransform(std::shared_ptr<InterpolatedTransformation> transformation)
    : mTransformation(std::move(transformation))
{
}

void
AnimatedTransform::update(const CoreComponentPtr& component, float alpha)
{
    bool changed = mTransformation->interpolate(alpha);
    auto assigned = component->getCalculated(kPropertyTransformAssigned);
    if (!assigned.is<Transformation>() || assigned.get<Transformation>() != mTransformation)
        component->setProperty(kPropertyTransformAssigned, Object(mTransformation));
    else if (changed)
        component->markProperty(kPropertyTransformAssigned);
}


} // namespace apl
