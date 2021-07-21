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

#ifndef _APL_TRANSFORM_H
#define _APL_TRANSFORM_H

#include "object.h"
#include "transform2d.h"
#include "apl/primitives/dimension.h"

namespace apl {

/**
 * Represent a single transformation (such as "rotate" or "skewY") in a sequence of transformations.
 */
class Transform {
public:
    enum Type { ROTATE, SKEW_X, SKEW_Y, SCALE, TRANSLATE };

    virtual ~Transform() {}

    /**
     * Convert this item into a 2D transformation matrix.
     * @param width The width of the component (used for relative dimension evaluation)
     * @param height The height of the component (used for relative dimension evaluation)
     * @return The 2D transformation matrix
     */
    virtual Transform2D evaluate(float width, float height) const = 0;

    virtual bool canInterpolate(Transform& other) const = 0;

    virtual Transform2D interpolate(Transform& other, float alpha, float width, float height) const = 0;

    virtual Type getType() const = 0;
};

/**
 * Store an array of transformations suitable for rapid conversion into a final transform.
 * This is a processed representation of a set of transforms.
 *
 * We rotate, scale, and skew about the origin of a component.  We need the WIDTH and HEIGHT in order to interpret
 * relative dimensions.
 */
class Transformation {
public:
    virtual ~Transformation() {}

    /**
     * Create a transformation from a context and an array of transformation items
     * @param context The context to evaluate the transformation in.
     * @param object The transformations
     * @return The calculated transformation
     */
    static std::shared_ptr<Transformation> create(const Context& context, const std::vector<Object>& array);


     /**
      * Calculate the transformation, given a width and height of the component
      * @param width Component width in DP
      * @param height Component height in DP
      * @return The transformation
      */
    virtual Transform2D get(float width, float height) = 0;
};

/**
 * A transformation that interpolates between two other transformations.
 */
class InterpolatedTransformation : public Transformation {
public:
    /**
     * Construct a transformation that interpolates between two sets of values
     * This method only works if the transformations have the same "shape" and members
     *
     * @param context The context to evaluate the transformation in.
     * @param from The starting point
     * @param to The ending point
     * @return The transformation
     */
    static std::shared_ptr<InterpolatedTransformation> create(const Context& context,
                                                              const std::vector<Object>& from,
                                                              const std::vector<Object>& to);

    /**
     * Set the interpolation value
     * @param alpha The interpolation value.
     * @return True if it has changed
     */
    virtual bool interpolate(float alpha) = 0;
};

}

#endif //_APL_TRANSFORM_H
