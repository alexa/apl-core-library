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

#include "apl/animation/easingapproximation.h"

namespace apl {

static float cubic( float a, float b, float c, float d, float t)
{
    if (t <= 0)
        return a;
    if (t >= 1)
        return d;

    // This is the actual function that is calculated
    // return a*(1-t)*(1-t)*(1-t) + 3*b*(1-t)*(1-t)*t + 3*c*(1-t)*t*t + d*t*t*t;

    // This is the same function with a few less multiplications and subtractions
    auto nt = 1-t;
    return nt*(a*nt*nt + 3*t*(b*nt + c*t)) + d*t*t*t;
}

/**
 * Store a piecewise linear approximation to a set of cubic bezier curves.
 * The mData vector stores "dof" points per increment; the mCumulative vector
 * stores the cumulative length of each little segment.
 *
 * For example, if mDOF = 2 and divisions = 11, we will store the points
 *
 * mData = [ x(0), y(0), x(0.1), y(0.1), x(0.2), y(0.2), ...., x(1.0), y(1.0) ]
 * mCumulative = [ math.sqrt( (x(0.1)-x(0))^2 + (y(0.1)-y(0))^2 ),
 *                 mCumulative[0] + math.sqrt( (x(0.2)-x(0.1))^2 + (y(0.2)-y(0.1))^2 ),
 *                 ... ]
 */
std::shared_ptr<EasingApproximation>
EasingApproximation::create(int dof,
                            const float *start,
                            const float *tout,
                            const float *tin,
                            const float *end,
                            int divisions)
{
    assert(dof >= 1);
    assert(divisions >= 2);

    // The data vector implicitly stores points from percentage = 0 to percentage = 100%
    std::vector<float> data(divisions * dof);

    // Populate all of the points during the first pass
    float* p1 = data.data();
    for (int i = 0; i < divisions; i++) {
        float percentage = static_cast<float>(i) / static_cast<float>(divisions - 1);
        for (int j = 0; j < dof; j++)
            *p1++ = cubic(start[j], start[j] + tout[j], end[j] + tin[j], end[j], percentage);
    }

    // We store the cumulative distance between each pair of points
    std::vector<float> cumulative(divisions - 1);

    // Calculate the distance between each pair of points calculated earlier
    // Store this in a cumulative array where the first entry in the array
    // is the distance between the first and second point, the second entry in the
    // array is the cumulative distance between the first and the third point, etc.
    float cumulativeLength = 0;
    float* cptr = cumulative.data();
    p1 = data.data();

    for (int i = 0; i < divisions - 1; i++) {
        float sumSquares = 0.0;
        for (int j = 0; j < dof; j++) {
            float delta = p1[dof] - p1[0];
            sumSquares += delta * delta;
            p1++;
        }

        cumulativeLength += ::sqrt(sumSquares);
        *cptr++ += cumulativeLength;
    }

    return std::make_shared<EasingApproximation>(dof, std::move(data), std::move(cumulative));
}

/**
 * Look up a position along the spatial easing curve based on the percentage of distance
 * traveled along that curve and the index of the coordinate to return.  Remember, easing
 * curves are usually defined in two or three dimensions such as x(t), y(t), z(t); the
 * "coordinate" determines if the function should return x(t), y(t), or z(t).
 *
 * The overall length of the curve is mCumulative.back().
 * Given an input percentage, we have to search for the segment that contains that section
 * of that curve.  Within that segment we assume linear interpolation.
 *
 * @param percentage The percentage of distance along the curve.
 * @param coordinate The index of the coordinate to return.  This must satisfy 0 <= coordinate < mDOF
 * @return The value of that coordinate at the percentage distance.
 */
float
EasingApproximation::getPosition(float percentage, int coordinate)
{
    assert(coordinate >= 0 && coordinate < mDOF);

    if (percentage <= 0)
        return mData.at(coordinate);  // Return the coordinate in the first block

    if (percentage >= 1.0) {
        const auto last = &mData.at(mData.size() - mDOF);  // Point into last block
        return last[coordinate];  // Return the coordinate in the last block
    }

    // Target length is overall length multiplied by percentage
    auto targetLength = percentage * mCumulative.back();
    auto it = std::lower_bound(mCumulative.begin(), mCumulative.end(), targetLength); // TODO: Create faster algorithm

    if (it == mCumulative.end()) {  // This shouldn't happen
        LOG(LogLevel::kWarn) << "Illegal end segment";
        const auto last = &mData.at(mData.size() - mDOF);  // Point into last block
        return last[coordinate];  // Return the end point of the last block
    }

    // Calculate the % distance within this segment
    auto startLength = it != mCumulative.begin() ? *(std::prev(it)) : 0;
    auto segmentFraction = (targetLength - startLength) / (*it - startLength);

    // Each segment has mDOF data points.  We jump forward to the correct data by segmentIndex
    // and then retrieve the "coordinate"-ed data from there (0 <= coordinate < mDOF).
    auto segmentIndex = std::distance(mCumulative.begin(), it);
    auto offset = segmentIndex * mDOF + coordinate;
    auto startValue = mData[offset];
    auto endValue = mData[offset + mDOF];  // Look into the next segment
    return startValue + (endValue - startValue) * segmentFraction;
}

} // namespace apl
