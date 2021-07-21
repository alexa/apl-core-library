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
        CONSOLE_CTP(context) << "Unrecognized animation command" << object;
        return nullptr;
    }

    auto property = propertyAsString(*context, object, "property");
    int key = sComponentPropertyBimap.get(property, -1);
    if (key == -1) {
        CONSOLE_CTP(context) << "Unrecognized animation property '" << property << "'";
        return nullptr;
    }

    if (!object.has("to")) {
        CONSOLE_CTP(context) << "Animation property has no 'to' value '" << property << "'";
        return nullptr;
    }

    // TODO: Generalize this.  We should pull the animation type from the component itself
    switch(key) {
        case kPropertyOpacity:
            if (object.has("from"))
                return std::unique_ptr<AnimatedDouble>(
                    new AnimatedDouble(static_cast<PropertyKey>(key),
                                       propertyAsDouble(*context, object, "from", 1),
                                       propertyAsDouble(*context, object, "to", 1)));
            else
                return std::unique_ptr<AnimatedDouble>(
                        new AnimatedDouble(static_cast<PropertyKey>(key),
                                           component,
                                           propertyAsDouble(*context, object, "to", 1))
                    );
            break;

        case kPropertyTransformAssigned:
            if (!object.has("from")) {
                CONSOLE_CTP(context) << "Animated transforms need a 'from' property";
                return nullptr;
            }

            return std::unique_ptr<AnimatedTransform>(
                new AnimatedTransform(
                    InterpolatedTransformation::create(*context,
                                                       arrayifyProperty(*context, object, "from"),
                                                       arrayifyProperty(*context, object, "to"))));

            break;
    }

    CONSOLE_CTP(context) << "Unable to animate property '" << property << "'";
    return nullptr;

}

AnimatedDouble::AnimatedDouble(PropertyKey key, double from, double to)
    : mKey(key), mFrom(from), mTo(to) {
}

AnimatedDouble::AnimatedDouble(PropertyKey key, const CoreComponentPtr& component, double to)
    : mKey(key), mFrom(component->getCalculated(key).asNumber()), mTo(to) {
}

void
AnimatedDouble::update(const CoreComponentPtr& component, float alpha) {
    double value = mFrom * (1 - alpha) + mTo * alpha;
    component->setProperty(mKey, value);
}

AnimatedTransform::AnimatedTransform(const std::shared_ptr<InterpolatedTransformation>& transformation)
    : mTransformation(transformation)
{
}

void
AnimatedTransform::update(const CoreComponentPtr& component, float alpha) {
    bool changed = mTransformation->interpolate(alpha);
    auto assigned = component->getCalculated(kPropertyTransformAssigned);
    if (!assigned.isTransform() || assigned.getTransformation() != mTransformation)
        component->setProperty(kPropertyTransformAssigned, Object(mTransformation));
    else if (changed)
        component->markProperty(kPropertyTransformAssigned);
}


} // namespace apl
