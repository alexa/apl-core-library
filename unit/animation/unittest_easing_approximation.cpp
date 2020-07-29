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

#include "../testeventloop.h"
#include "testeasingcurve.h"

#include "apl/animation/easingapproximation.h"

using namespace apl;

static ::testing::AssertionResult
compareCurve(const TestCurve& testCurve,
             const std::shared_ptr<EasingApproximation>& approx,
             double epsilon,
             double epsilon2)
{
    for (double t = 0.0; t <= 1.0; t += 0.01) {
        for (int index = 0; index < testCurve.dof(); index++) {
            if (::abs(testCurve.position(t, index) - approx->getPosition(t, index)) > epsilon)
                return ::testing::AssertionFailure()
                       << " position mismatch at percentage=" << t << " index=" << index
                       << " expected=" << testCurve.position(t, index)
                       << " actual=" << approx->getPosition(t, index);
        }
    }
    return ::testing::AssertionSuccess();
}

TEST(EasingApproximation, StraightLine)
{
    // Construct a straight line x(t) = t   (a=0, b=1/3, c=2/3, d=1)
    float start[] = {0};      // a
    float end[] = {1};        // d
    float tout[] = {0.33333};   // b - a
    float tin[] = {-0.33333};    // c - d

    auto approx = EasingApproximation::create(1, start, tout, tin, end, 11);
    ASSERT_TRUE(approx);

    ASSERT_EQ(0, approx->getPosition(0, 0));
    ASSERT_NEAR(0.25, approx->getPosition(0.25, 0), 0.0001);
    ASSERT_NEAR(0.5, approx->getPosition(0.5, 0), 0.0001);
    ASSERT_NEAR(0.75, approx->getPosition(0.75, 0), 0.0001);
    ASSERT_EQ(1, approx->getPosition(1, 0));

    ASSERT_TRUE(compareCurve(std::vector<Cubic>{Cubic(0, 1, 0, 1.0/3, 2./3, 1.0)}, approx, 0.005, 0.01));
}

TEST(EasingApproximation, OffsetStraightLine)
{
    // Construct a straight line x(t) = kt + a
    // Then b=k/3+a, c=2k/3+a, d=k+a

    // Choose a=6, k=9.  =>  b=9, c=12, d=15
    float start[] = {6};  // a
    float end[] = {15};   // d
    float tout[] = {3};  // b - a
    float tin[] = {-3};   // c - d

    auto approx = EasingApproximation::create(1, start, tout, tin, end, 101);
    ASSERT_TRUE(approx);

    ASSERT_EQ(6, approx->getPosition(0, 0));
    ASSERT_NEAR(8.25, approx->getPosition(0.25, 0), 0.0001);
    ASSERT_NEAR(10.5, approx->getPosition(0.5, 0), 0.0001);
    ASSERT_NEAR(12.75, approx->getPosition(0.75, 0), 0.0001);
    ASSERT_EQ(15, approx->getPosition(1, 0));

    ASSERT_TRUE(compareCurve(std::vector<Cubic>{Cubic(0, 1, 6, 9, 12, 15)}, approx, 0.01, 0.01));
}

TEST(EasingApproximation, Parabola)
{
    // Construct a parabola x(t)=t, y(t)=4(t-1/2)^2
    // x(t): a=0, b=+1/3, c=+2/3, d=1
    // y(t): a=1, b=-1/3, c=-1/3, d=1

    float start[] = {0,1};  // a
    float end[] = {1,1};   // d
    float tout[] = {0.33333,-1.33333};  // b - a
    float tin[] = {-0.33333,-1.33333};   // c - d

    auto approx = EasingApproximation::create(2, start, tout, tin, end, 101);
    ASSERT_TRUE(approx);

    ASSERT_EQ(0, approx->getPosition(0, 0));  // x(0) = 0
    ASSERT_EQ(1, approx->getPosition(0, 1));  // y(0) = 1
    ASSERT_EQ(1, approx->getPosition(1, 0));  // x(1) = 1
    ASSERT_EQ(1, approx->getPosition(1, 1));  // y(1) = 1

    // Halfway through we should be at (0.5, 0)
    ASSERT_NEAR(0.5, approx->getPosition(0.5, 0), 0.0001);
    ASSERT_NEAR(0, approx->getPosition(0.5, 1), 0.0001);

    // The overall path length is ~2.3234   [1/8 * (4 * sqrt(17) + asinh(4))]

    // One quarter the way through we should be at x(t) = 0.16685 approximately
    ASSERT_NEAR(0.16685, approx->getPosition(0.25, 0), 0.001);
    ASSERT_NEAR(4 * (0.16685 - 0.5) * (0.16685 - 0.5), approx->getPosition(0.25, 1), 0.001);

    // Three-quarters of the way through we should be in a mirrored situation
    ASSERT_NEAR(1 - 0.16685, approx->getPosition(0.75, 0), 0.001);
    ASSERT_NEAR(4 * (0.16685 - 0.5) * (0.16685 - 0.5), approx->getPosition(0.75, 1), 0.001);

    ASSERT_TRUE(compareCurve(std::vector<Cubic>{Cubic(0, 1, 0, 1./3, 2./3, 1),
                                                Cubic(0, 1, 1,-1./3,-1./3,1)},
                             approx, 0.02, 0.02));
}
