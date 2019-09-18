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

#ifndef _APL_ANIMATION_H
#define _APL_ANIMATION_H

// #include "apl/transform.h"

namespace apl {

class Context;
class Object;
class CoreComponent;

/**
 * Store the properties needed for an animation.
 */
class Animation {
public:
    struct AnimationValue {
        enum Type { OPACITY, TRANSFORM };
        enum Action { FROM_TO, TO_ONLY, FIXED };
        Type type;
        int key;   // Used for transform
        float from;
        float to;
    };

    /**
     * Construct the animation object from a vector of animation values
     * @param context The data-binding context
     * @param array The array of animation values
     * @return
     */
    static Object create(const Context& context, const std::vector<Object>& array);

    /**
     * Initialize all of the "from" values that use the existing settings
     * @param component The component to initialize from.
     */
    void initialize(CoreComponent& component);

    /**
     * Apply the current animation value to the component.
     * @param component The component to set.
     * @param alpha The "alpha" value, where alpha is derived from the easing curve.
     */
    void apply(CoreComponent& component, float alpha);

private:
    std::vector<AnimationValue> mAnimations;
    // std::vector<TransformItem> mTransforms;
};

} // namespace apl

#endif //_APL_ANIMATION_H
