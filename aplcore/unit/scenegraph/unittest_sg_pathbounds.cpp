/*
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
 *
 */

#include "../testeventloop.h"
#include "test_sg.h"
#include "apl/scenegraph/builder.h"
#include "apl/scenegraph/pathparser.h"
#include "apl/scenegraph/pathbounds.h"

using namespace apl;

class SGPathBoundsTest : public DocumentWrapper {};

struct PathBoundsTest {
    std::string source;
    Rect bounds;
};

::testing::AssertionResult
CheckScaleExpansion(Point expected, Transform2D transform, float epsilon=1e-6)
{
    return IsEqual(expected, transform.scaleExpansion(), epsilon);
}

const std::vector<PathBoundsTest> BOUNDS_TESTS = {
    {"", {}},               // Null set
    {"M20,20 10,10", {}},   // Still a null set
    {"h20", {0, 0, 20, 0}}, // Simple horizontal line
    {"h20 v20", {0, 0, 20, 20}}, // Simple vertical
    {"M10,10 h20", {10, 10, 20, 0}},  // Offset horizontal line
    {"m-10,0 h20 M0,-10 v20", {-10, -10, 20, 20}}, // Plus-sign

    // Quadratic paths
    {"Q10,10 20,0", {0,0,20,5}},  // The bottom of the quadratic is halfway down
    {"Q10,40 20,0", {0,0,20,20}},
    {"Q10,10 0,20", {0,0,5,20}},
    {"Q40,10 0,20", {0,0,20,20}},
    {"Q10,0 10,10", {0,0,10,10}},

    // Cubic paths
    {"C10,20 20,20 30,0", {0,0,30,15}},  // The bottom is 3/4 of the way up
    {"C20,10 20,20 0,30", {0,0,15,30}},
    {"C10,20 20,-20 30,0", {0,-5.77350235,30,2*5.77350235}},  // Two roots

    // Path closure (no effect)
    {"L10,0 v30 z", {0,0,10,30}},
};

TEST_F(SGPathBoundsTest, GeneralPath)
{
    for (const auto& m : BOUNDS_TESTS) {
        auto path = sg::parsePathString(m.source);
        ASSERT_TRUE(IsEqual(path->boundingBox(Transform2D()), m.bounds)) << m.source;
    }
}

TEST_F(SGPathBoundsTest, OtherPaths)
{
    // All other path types are fundamentally rectangles
    ASSERT_TRUE(IsEqual(sg::path(Rect{20,30,40,50})->boundingBox(Transform2D()), Rect{20,30,40,50}));
    ASSERT_TRUE(IsEqual(sg::path(Rect{20,30,40,50}, 20)->boundingBox(Transform2D()), Rect{20,30,40,50}));
    ASSERT_TRUE(IsEqual(sg::path(RoundedRect({20,30,40,50},4), 10)->boundingBox(Transform2D()), Rect{20,30,40,50}));
}

TEST_F(SGPathBoundsTest, TransformScaleExpansion)
{
    ASSERT_TRUE(CheckScaleExpansion(Point(1,1), Transform2D()));
    ASSERT_TRUE(CheckScaleExpansion(Point(1.414214, 1.414214), Transform2D::rotate(45)));
    ASSERT_TRUE(CheckScaleExpansion(Point(2,2), Transform2D::scale(2)));
    ASSERT_TRUE(CheckScaleExpansion(Point(2,0.5), Transform2D::scale(2,0.5)));
    ASSERT_TRUE(CheckScaleExpansion(Point(1,1), Transform2D::translate(23.5, 17)));

    // Order matters.  Rotating and then scaling is different from scaling and then rotating.
    ASSERT_TRUE(CheckScaleExpansion(Point(2,0.5),Transform2D::scale(2,0.5) * Transform2D::rotate(90)));
    ASSERT_TRUE(CheckScaleExpansion(Point(0.5,2),Transform2D::rotate(90)*Transform2D::scale(2,0.5)));
}

TEST_F(SGPathBoundsTest, StrokePathMaxWidth)
{
    auto paint = sg::paint(Color(Color::BLACK));
    ASSERT_EQ(0, sg::fill(paint).get()->maxWidth());

    auto op = sg::stroke(paint).strokeWidth(4.0).lineJoin(apl::kGraphicLineJoinRound).get();
    ASSERT_EQ(4.0, op->maxWidth());

    op = sg::stroke(paint).strokeWidth(4.0).lineJoin(apl::kGraphicLineJoinMiter).miterLimit(6).get();
    ASSERT_EQ(24.0, op->maxWidth());
}

const std::vector<float> T1 = {
    94.077423,
    67.9983673,
    188.476852,
    -22.6661224,
    341.527985,
    -22.6661224,
    435.926422,
    67.9983673,};

const std::vector<float> T2 = {
    94.077423,
    67.9984665,
    188.476852,
    -22.6660252,
    341.527985,
    -22.6660252,
    435.926422,
    67.9984665,
};

/**
 * This test case checks for numerical instability in the calculation of the bounding box
 * of a cubic spline.  The two splines differ only slightly, but one of them triggered a
 * "can't find root" condition and resulted in a straight line instead of an arc.
 */
TEST_F(SGPathBoundsTest, NumericalInstability)
{
    ASSERT_TRUE(IsEqual(sg::calculatePathBounds("MC", T1), sg::calculatePathBounds("MC", T2)));
}