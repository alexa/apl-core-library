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

#include "apl/animation/coreeasing.h"
#include "apl/utils/noncopyable.h"

#ifndef _APL_EASING_APPROXIMATION_H
#define _APL_EASING_APPROXIMATION_H

namespace apl {

/**
 * Store a piecewise linear approximation to a cubic-bezier path defined in "N" dimensions.
 */
class EasingApproximation : public EasingSegment::Data,
                            public NonCopyable {

public:
    /**
     * Create an easing curve approximation.
     * @param dof The number of entries in the start, tout, tin, and end arrays
     * @param start An array of starting values
     * @param tout An array of tangent-out values (relative to the start value)
     * @param tin An array of tangent-in values (relative to the end value)
     * @param end  An array of ending values
     * @param blockCount The total number of subsegments to create.
     * @return A pointer to the easing approximation.
     */
    static std::shared_ptr<EasingApproximation> create(int dof,
                                                       const float *start,
                                                       const float *tout,
                                                       const float *tin,
                                                       const float *end,
                                                       int blockCount);

    // Private constructor - use the "create" method instead.
    EasingApproximation(int dof,
                        std::vector<float>&& data,
                        std::vector<float>&& cumulative)
        : mDOF(dof),
          mData(std::move(data)),
          mCumulative(std::move(cumulative)) {}

    /**
     * Calculate a position along the easing curve.
     * @param percentage The percentage of the length of the path.
     * @param coordinate The index of the coordinate to return.  Must be less than the DOF of the curve.
     * @return The position of that indexed coordinate at the requested percentage of the path.
     */
    float getPosition(float percentage, int coordinate);

private:
    const int mDOF;        // Degrees of freedom.  Either 1, 2, or 3
    const std::vector<float> mData;
    const std::vector<float> mCumulative;
};


} // namespace apl

#endif // _APL_EASING_APPROXIMATION_H
