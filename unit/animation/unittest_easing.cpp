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

#include "apl/animation/coreeasing.h"
#include "apl/animation/easinggrammar.h"

using namespace apl;

class EasingTest : public DocumentWrapper {};

TEST_F(EasingTest, Linear)
{
    auto path = Easing::linear();

    ASSERT_EQ(0, path->calc(-1));
    ASSERT_EQ(0, path->calc(0));
    ASSERT_EQ(0.5, path->calc(0.5));
    ASSERT_EQ(1, path->calc(1));
    ASSERT_EQ(1, path->calc(2));
}

TEST_F(EasingTest, CubicBezier)
{
    auto path = CoreEasing::bezier(0.31, 0.31, 0.69, 0.69);   // Cubic-bezier by default

    ASSERT_NEAR(0, path->calc(-1), 0.0001);
    ASSERT_NEAR(0, path->calc(0), 0.0001);
    ASSERT_NEAR(0.2, path->calc(0.2), 0.0001);
    ASSERT_NEAR(0.4, path->calc(0.4), 0.0001);
    ASSERT_NEAR(0.6, path->calc(0.6), 0.0001);
    ASSERT_NEAR(0.8, path->calc(0.8), 0.0001);
    ASSERT_NEAR(1.0, path->calc(1.0), 0.0001);
    ASSERT_NEAR(1.0, path->calc(1.2), 0.0001);
}

static float f(float a, float b, float t)
{
    return 3*t*(1-t)*(1-t)*a + 3*t*t*(1-t)*b + t*t*t;
}

TEST_F(EasingTest, CubicBezierEase)
{
    auto path = CoreEasing::bezier(0.25, 0.10, 0.25, 1.0);  // Ease

    for (int i = 0 ; i <= 10 ; i++) {
        float alpha = 0.1 * i;
        float t = f(0.25, 0.25, alpha);
        float v = f(0.10, 1.0, alpha);
        ASSERT_NEAR(v, path->calc(t), 0.0001) << "alpha=" << alpha << " t=" << t << " v=" << v;
    }
}

TEST_F(EasingTest, EasingCurve)
{
    auto linear = Easing::parse(session, "");

    ASSERT_EQ(0.25, linear->calc(0.25));
    ASSERT_EQ(0.5, linear->calc(0.5));
    ASSERT_EQ(0.75, linear->calc(0.75));

    ASSERT_TRUE(ConsoleMessage());  // It was an illegal curve
}

TEST_F(EasingTest, EasingCurveEaseIn)
{
    auto curve = Easing::parse(session, "ease-in");

    for (int i = 0 ; i <= 10 ; i++) {
        float alpha = 0.1 * i;
        float t = f(0.42, 1, alpha);
        float v = f(0, 1, alpha);
        ASSERT_NEAR(v, curve->calc(t), 0.0001) << "alpha=" << alpha << " t=" << t << " v=" << v;
    }
}

TEST_F(EasingTest, ExistingCurves)
{
    ASSERT_TRUE(Easing::has("linear"));
    ASSERT_TRUE(Easing::has("ease"));
    ASSERT_TRUE(Easing::has("ease-in"));
    ASSERT_TRUE(Easing::has("ease-out"));
    ASSERT_TRUE(Easing::has("ease-in-out"));

    ASSERT_TRUE( IsEqual(Easing::linear(), Easing::parse(session, "linear")));
    ASSERT_TRUE( IsEqual(CoreEasing::bezier(0.25, 0.10, 0.25, 1), Easing::parse(session, "ease")));
}

TEST_F(EasingTest, EasingCurveCustom)
{
    ASSERT_FALSE(Easing::has("cubic-bezier(0.33,-0.5,0.92,0.38)"));

    auto curve = Easing::parse(session, " cubic-bezier( 0.33, -0.5, 0.92 , 0.38  ) ");

    for (int i = 0 ; i <= 10 ; i++) {
        float alpha = 0.1 * i;
        float t = f(0.33, 0.92, alpha);
        float v = f(-0.5, 0.38, alpha);
        ASSERT_NEAR(v, curve->calc(t), 0.0001) << "alpha=" << alpha << " t=" << t << " v=" << v;
    }

    ASSERT_TRUE(Easing::has("cubic-bezier(0.33,-0.5,0.92,0.38)"));
}

TEST_F(EasingTest, EasingPathCustom)
{
    auto curve = Easing::parse(session, " path( 0.25, 1, 0.75, 0)");

    ASSERT_NEAR(0, curve->calc(0), 0.0001);
    ASSERT_NEAR(0.5, curve->calc(0.125), 0.0001);
    ASSERT_NEAR(1, curve->calc(0.25), 0.0001);
    ASSERT_NEAR(0.5, curve->calc(0.5), 0.0001);
    ASSERT_NEAR(0, curve->calc(0.75), 0.0001);
    ASSERT_NEAR(0.5, curve->calc(0.875), 0.0001);
    ASSERT_NEAR(1, curve->calc(1), 0.0001);

    curve = Easing::parse(session, "path(0.1, 1, 0.2, 0, 0.3, 1, 0.4, 0, 0.5, 1, 0.6, 0, 0.7, 1, 0.8, 0, 0.9, 1)");

    ASSERT_NEAR(0, curve->calc(0), 0.0001);
    ASSERT_NEAR(0.5, curve->calc(0.05), 0.0001);
    ASSERT_NEAR(1, curve->calc(0.1), 0.0001);
    ASSERT_NEAR(0.5, curve->calc(0.15), 0.0001);
    ASSERT_NEAR(0, curve->calc(0.2), 0.0001);
    ASSERT_NEAR(0.5, curve->calc(0.25), 0.0001);
    ASSERT_NEAR(1, curve->calc(0.3), 0.0001);
    ASSERT_NEAR(0.5, curve->calc(0.35), 0.0001);
    ASSERT_NEAR(0, curve->calc(0.4), 0.0001);
    ASSERT_NEAR(0.5, curve->calc(0.45), 0.0001);
    ASSERT_NEAR(1, curve->calc(0.5), 0.0001);
    ASSERT_NEAR(0.5, curve->calc(0.55), 0.0001);
    ASSERT_NEAR(0, curve->calc(0.6), 0.0001);
    ASSERT_NEAR(0.5, curve->calc(0.65), 0.0001);
    ASSERT_NEAR(1, curve->calc(0.7), 0.0001);
    ASSERT_NEAR(0.5, curve->calc(0.75), 0.0001);
    ASSERT_NEAR(0, curve->calc(0.8), 0.0001);
    ASSERT_NEAR(0.5, curve->calc(0.85), 0.0001);
    ASSERT_NEAR(1, curve->calc(0.9), 0.0001);
    ASSERT_NEAR(1, curve->calc(0.95), 0.0001);
    ASSERT_NEAR(1, curve->calc(1.0), 0.0001);
}

using TestPair = std::vector<std::pair<float, float>>;

::testing::AssertionResult CheckCurve(std::shared_ptr<TestSession>& session,
                                      const std::string& easingCurve,
                                      const TestPair&& args,
                                      double epsilon=0.001)
{
    auto curve = Easing::parse(session, easingCurve);
    if (session->checkAndClear())
        return ::testing::AssertionFailure() << "Failed to parse easing curve '" << easingCurve << "'";

    for (auto& m : args) {
        auto time = m.first;
        auto expected = m.second;
        auto actual = curve->calc(time);

        if (std::fabs(expected - actual) >= epsilon)
            return ::testing::AssertionFailure() << "Values don't match time=" << time
                                                 << " expected=" << expected
                                                 << " actual=" << actual;
    }

    return ::testing::AssertionSuccess();
};

TEST_F(EasingTest, SegmentedLinear)
{
    // Flat until 0.5, then linear increase to (1,1)
    ASSERT_TRUE(CheckCurve(session, "line(0.5, 0) end(1,1)",
                           {{0,    0},
                            {0.25, 0},
                            {0.5,  0},
                            {0.75, 0.50},
                            {1.0,  1.0}}));

    // Flat until (0.5, 0.25), then steep slope to (1,1)
    ASSERT_TRUE(CheckCurve(session, "line(0.5, 0.25) end(1,1)",
                           {{0,    0.25},
                            {0.25, 0.25},
                            {0.5,  0.25},
                            {0.75, 0.625},
                            {1.0,  1.0}}));

    // Sawtooth
    ASSERT_TRUE(CheckCurve(session, "line(0,0) line(0.25, 1) line(0.5,0) line(0.75,1) end(1,1)",
                           {{0,     0},
                            {0.125, 0.5},
                            {0.25,  1},
                            {0.375, 0.5},
                            {0.5,   0},
                            {0.625, 0.5},
                            {0.75,  1},
                            {0.875, 1},
                            {1.0,   1}}));
}

TEST_F(EasingTest, SegmentedCurve)
{
    ASSERT_TRUE(CheckCurve(session, "curve(0, 0, 0.25, 0.10, 0.25, 1.0) end(1,1)",
                           {{f(0.25, 0.25, 0.0), f(0.10, 1.0, 0.0)},
                            {f(0.25, 0.25, 0.1), f(0.10, 1.0, 0.1)},
                            {f(0.25, 0.25, 0.2), f(0.10, 1.0, 0.2)},
                            {f(0.25, 0.25, 0.3), f(0.10, 1.0, 0.3)},
                            {f(0.25, 0.25, 0.4), f(0.10, 1.0, 0.4)},
                            {f(0.25, 0.25, 0.5), f(0.10, 1.0, 0.5)},
                            {f(0.25, 0.25, 0.6), f(0.10, 1.0, 0.6)},
                            {f(0.25, 0.25, 0.7), f(0.10, 1.0, 0.7)},
                            {f(0.25, 0.25, 0.8), f(0.10, 1.0, 0.8)},
                            {f(0.25, 0.25, 0.9), f(0.10, 1.0, 0.9)},
                            {f(0.25, 0.25, 1.0), f(0.10, 1.0, 1.0)}}));

    ASSERT_TRUE(CheckCurve(session, "curve(0, 0, 0.25, 0.10, 0.25, 1.0) end(10,10)",
                           {{0, 0}, {10 * f(0.25, 0.25, 0.5), 10 * f(0.10, 1.0, 0.5)}, {10, 10}}));

    ASSERT_TRUE(CheckCurve(
        session, "curve(0, 0, 0.25, 0.10, 0.25, 1.0) curve(5, 5, 0.25, 0.10, 0.25, 1.0) end(10,10)",
        {{0, 0},
         {5 * f(0.25, 0.25, 0.5), 5 * f(0.10, 1.0, 0.5)},
         {5 + 5 * f(0.25, 0.25, 0.5), 5 + 5 * f(0.10, 1.0, 0.5)},
         {10, 10}}));

    ASSERT_TRUE(CheckCurve(
        session,
        "curve(0, 0, 0.25, 0.10, 0.25, 1.0) curve(0.25, 0.5, 0.31, 0.31, 0.69, 0.69) end(1,1)",
        {{0.25 * f(0.25, 0.25, 0.0), 0.5 * f(0.10, 1.0, 0.0)},
         {0.25 * f(0.25, 0.25, 0.1), 0.5 * f(0.10, 1.0, 0.1)},
         {0.25 * f(0.25, 0.25, 0.2), 0.5 * f(0.10, 1.0, 0.2)},
         {0.25 * f(0.25, 0.25, 0.3), 0.5 * f(0.10, 1.0, 0.3)},
         {0.25 * f(0.25, 0.25, 0.4), 0.5 * f(0.10, 1.0, 0.4)},
         {0.25 * f(0.25, 0.25, 0.5), 0.5 * f(0.10, 1.0, 0.5)},
         {0.25 * f(0.25, 0.25, 0.6), 0.5 * f(0.10, 1.0, 0.6)},
         {0.25 * f(0.25, 0.25, 0.7), 0.5 * f(0.10, 1.0, 0.7)},
         {0.25 * f(0.25, 0.25, 0.8), 0.5 * f(0.10, 1.0, 0.8)},
         {0.25 * f(0.25, 0.25, 0.9), 0.5 * f(0.10, 1.0, 0.9)},
         {0.25 * f(0.25, 0.25, 1.0), 0.5 * f(0.10, 1.0, 1.0)}}));
}

static std::vector<std::string> sFailureCases = {
    "foo",
    "path(1",
    "path(",
    "path(1)",
    "path(1,2,3,4,5)",
    "path(0,0)",  // The 0,0 is implicit
    "path(1,1)",
    "path(1.2,1)",
    "path(-.2,0)",
    "path(0.2,0.2,0.1,0.5)",  // Out of order
    "cubic-bezier()",
    "cubic-bezier(1,2,3)",
    "cubic-bezier(1,2,3,4,5)"
    "line() end(1,1)",      // Wrong number of arguments
    "line(1) end(1,1)",
    "line(a",
    "line(1, end(1,1)",
    "line(1,1)",   // No end value
    "line(1,2,3)",
    "line(1,1) end(0,1)",  // Invalid times
    "curve(0,0) end(1,1)",  // Wrong number of arguments
    "curve(0,1,2,3,4,5,6,7,8) end(1,2)",
    "curve(1,0,1,1,1,1) end(0,1)", // Invalid times
    "end(0,1)",
    "line(0,1) line(2,1) end(1,1)", // Invalid times
    "send(1,2,3)",
    "spatial(2,0)",   // Must have at least one curve segment
    "spatial(2,0) send(0,1,1)",  // Must have at least one segment
    "spatial(1,0) scurve(0, 0,0,0, 0.25,0.25,0.25,0.25) send(1,0)",  // DOF must be >= 2
//  This next one is a valid two-dimensional curve, for reference
//    "spatial(2,0) scurve(0, 0,0, 0,0, 0,0, 0.25,0.25,0.25,0.25) send(1,0,0)",
    "spatial(2,2) scurve(0, 0,0, 0,0, 0,0, 0.25,0.25,0.25,0.25) send(1,0,0)", // Invalid spatial index
    "spatial(2,-1) scurve(0, 0,0, 0,0, 0,0, 0.25,0.25,0.25,0.25) send(1,0,0)", // Invalid spatial index
    "spatial(3,0) scurve(0, 0,0, 0,0, 0,0, 0.25,0.25,0.25,0.25) send(1,0,0)", // DOF mismatch
    "spatial(2,0) scurve(0, 0,0, 0,0, 0,0, 0.25,0.25,0.25,0.25) send(-1,0,0)",  // Invalid time
};

TEST_F(EasingTest, EasingFail)
{
    auto linear = Easing::linear();
    for (auto& m : sFailureCases) {
        ASSERT_TRUE(IsEqual(linear, Easing::parse(session, m.c_str())))
            << "test case: '" << m << "'";
        ASSERT_TRUE(ConsoleMessage()) << m;
    }
}

static const char *ROTATE = R"apl(
    {
      "type": "APL",
      "version": "1.4",
      "graphics": {
        "clock": {
          "type": "AVG",
          "version": "1.1",
          "width": 444,
          "height": 237,
          "description": "TestAnimationRotate",
          "items": [
            {
              "items": [
                {
                  "type": "group",
                  "items": {
                    "type": "path",
                    "pathData": "M50.957 0 C50.957,28.143 28.143,50.957 0,50.957 C-28.143,50.957 -50.957,28.143 -50.957,0 C-50.957,-28.143 -28.143,-50.957 0,-50.957 C28.143,-50.957 50.957,-28.143 50.957,0zM0 -39.704 C-21.928,-39.704 -39.704,-21.928 -39.704,0 C-39.704,21.927 -21.928,39.703 0,39.703 C0,39.703 0,50.957 0,50.957 C7.604,42.372 18.585,37.796 28.769,27.363 C35.635,20.328 39.704,10.606 39.704,0 C39.704,-21.928 21.928,-39.704 0,-39.704z",
                    "fill": "rgb(72,195,249,1)"
                  },
                  "translateX": 51.207,
                  "translateY": 51.207
                }
              ],
              "type": "group",
              "translateX": 222,
              "translateY": 118.5,
              "anchorX": 51.5,
              "anchorY": 51.5,
              "rotation": "${@ease1(time)}"
            }
          ],
          "parameters": [
            {
              "name": "time",
              "value": 0
            }
          ],
          "resources": [
            {
              "easing": {
                "ease1": "line(0,0) end(100,360) "
              }
            }
          ]
        }
      },
      "mainTemplate": {
        "items": {
          "type": "VectorGraphic",
          "source": "clock",
          "width": "100%",
          "height": "100%",
          "scale": "best-fit",
          "align": "center",
          "time": "${elapsedTime}"
        }
      }
    }
)apl";

TEST_F(EasingTest, Rotate)
{
    loadDocument(ROTATE);
    ASSERT_TRUE(component);

    auto graphicObject = component->getCalculated(kPropertyGraphic);
    ASSERT_TRUE(graphicObject.isGraphic());

    auto graphic = graphicObject.getGraphic();
    ASSERT_TRUE(graphic);

    auto graphicRoot = graphic->getRoot();
    ASSERT_TRUE(graphicRoot);
    ASSERT_EQ(kGraphicElementTypeContainer, graphicRoot->getType());
    ASSERT_EQ(1, graphicRoot->getChildCount());

    auto group1 = graphicRoot->getChildAt(0);
    ASSERT_TRUE(group1);
    ASSERT_EQ(kGraphicElementTypeGroup, group1->getType());
    ASSERT_EQ(1, group1->getChildCount());

    auto group2 = group1->getChildAt(0);
    ASSERT_TRUE(group2);
    ASSERT_EQ(kGraphicElementTypeGroup, group2->getType());
    ASSERT_EQ(1, group2->getChildCount());

    // Check that the initial matrix for group1 is just a translation
    ASSERT_TRUE(IsEqual(222, group1->getValue(kGraphicPropertyTranslateX)));
    ASSERT_TRUE(IsEqual(Transform2D::translate(222, 118.5), group1->getValue(kGraphicPropertyTransform)));

    advanceTime(5000);

    ASSERT_TRUE(IsEqual(222, group1->getValue(kGraphicPropertyTranslateX)));
    ASSERT_TRUE(IsEqual(Transform2D::translate(222, 118.5), group1->getValue(kGraphicPropertyTransform).getTransform2D()));
}


TEST_F(EasingTest, SegmentedP)
{
    /*
       Start at (0,0).  Go to (1,1) with both control points at (1,0)
       scurve(
           0,     // Time
           0,0,   // Starting point 0,0
           1,0,   // Tout. Control point == Tout+Start == (1,0)
           0,-1,   // Tin.  Control point == Tin + End  == (1,0)
           0.1, 0.1, 0.5, 0.5   // Linear time interpolation
           )

       send(
           1      // Time
           1,1    // Ending point (1,1)
           )
    */

    const std::string TEST = "scurve(0,0,0,1,0,0,-1,0.1,0.1,0.5,0.5) send(1,1,1)";

    // X-coordinate
    ASSERT_TRUE(CheckCurve(
        session,
        "spatial(2,0) " + TEST,
        {{0, 0},
         {0.25, 0.450455},
         {0.50, 0.875000},
         {0.75, 0.994079},
         {1.00, 1}}));

    // Y-coordinate
    ASSERT_TRUE(CheckCurve(
        session,
        "spatial(2,1) " + TEST,
        {{0, 0},
         {0.25, 0.005922},
         {0.50, 0.125000},
         {0.75, 0.549546},
         {1.00, 1}}));
}


TEST_F(EasingTest, SegmentedPTime)
{
    /*
       Start at (0,0).  Go to (1,1) following a straight line.  Time is non-linear.
       scurve(
           0,     // Time
           0,0,   // Starting point 0,0
           0,0,   // Tout. Control point == Tout+Start == (0,0)
           0,0,   // Tin.  Control point == Tin + End  == (1,1)
           0, 1, 1, 0   // Non-linear time interpolation (goes rapidly from 0 to near 0.5, flattens,
                           and then leaps up to 1.0 at the end)
           )

       send(
           1      // Time
           1,1    // Ending point (1,1)
           )
    */

    // The 2D curve is a straight line.  Time interpolation is curved.
    const std::string TEST = "scurve(0,0,0,0,0,0,0,0,1,1,0) send(1,1,1)";

    // X-coordinate
    ASSERT_TRUE(CheckCurve(
        session,
        "spatial(2,0) " + TEST,
        {{0, 0},
         {0.25, 0.479056},
         {0.50, 0.500000},
         {0.75, 0.520943},
         {1.00, 1}}));

    // Y-coordinate
    ASSERT_TRUE(CheckCurve(
        session,
        "spatial(2,1) " + TEST,
        {{0, 0},
         {0.25, 0.479056},
         {0.50, 0.500000},
         {0.75, 0.520943},
         {1.00, 1}}));
}


TEST_F(EasingTest, SegmentedScaledP)
{
    /*
       Same as SegmentedP, but we scale on the x and y-axis
       Start at (0,0).  Go to (10,10) with both control points at (10,0)
       scurve(
           0,     // Time
           0,0,   // Starting point 0,0
           10,0,  // Tout. Control point == Tout+Start == (10,0)
           0,-10, // Tin.  Control point == Tin + End  == (10,0)
           0.1, 0.1, 0.5, 0.5   // Linear time interpolation
           )

       send(
           1      // Time
           10,10  // Ending point (10,10)
           )
    */
    const std::string TEST = "scurve(0,0,0,10,0,0,-10,0.1,0.1,0.5,0.5) send(1,10,10)";

    // X-coordinate
    ASSERT_TRUE(CheckCurve(
        session,
        "spatial(2,0) " + TEST,
        {{0, 0},
         {0.25, 0.450455 * 10},
         {0.50, 0.875000 * 10},
         {0.75, 0.994079 * 10},
         {1.00, 10}}));

    // Y-coordinate
    ASSERT_TRUE(CheckCurve(
        session,
        "spatial(2,1) " + TEST,
        {{0, 0},
         {0.25, 0.005922 * 10},
         {0.50, 0.125000 * 10},
         {0.75, 0.549546 * 10},
         {1.00, 10}}));
}


TEST_F(EasingTest, SegmentedOffsetP)
{
    /*
       Same as SegmentdP with offset X and Y values
       Start at (0,0).  Go to (10,10) with both control points at (10,0)
       scurve(
           0,     // Time
           10,20,   // Starting point 0,0
           1,0,  // Tout. Control point == Tout+Start == (11,20)
           0,-1, // Tin.  Control point == Tin + End  == (11,20)
           0.1, 0.1, 0.5, 0.5   // Linear time interpolation
           )

       send(
           1      // Time
           11,21  // Ending point (11,21)
           )
    */
    const std::string TEST = "scurve(0,10,20,1,0,0,-1,0.1,0.1,0.5,0.5) send(1,11,21)";

    // X-coordinate
    ASSERT_TRUE(CheckCurve(
        session,
        "spatial(2,0) " + TEST,
        {{0, 10+0},
         {0.25, 10+0.450455},
         {0.50, 10+0.875000},
         {0.75, 10+0.994079},
         {1.00, 10+1}}));

    // Y-coordinate
    ASSERT_TRUE(CheckCurve(
        session,
        "spatial(2,1) " + TEST,
        {{0, 20+0},
         {0.25, 20+0.005922},
         {0.50, 20+0.125000},
         {0.75, 20+0.549546},
         {1.00, 20+1}}));
}

TEST_F(EasingTest, MultiSegmentPositionCurve)
{
    /*
       Start at (0,0).  Go to (1,0) with control points at (0,1) and (1,1)
       scurve(
           0,     // Time
           0,0,   // Starting point 0,0
           0,1,   // Tout. Control point == Tout+Start == (0,1)
           0,1,   // Tin.  Control point == Tin + End  == (1,1)
           0.25, 0.25, 0.75, 0.75   // Linear time interpolation
           )

       Loop back to (0,0), with control points at (1,-1) and (0,-1)
       scurve(
           0.5,   // Time
           1,0,   // Starting point (1,0)
           0,-1,  // Tout. Control point == Tout+Start == (1,-1)
           0,-1,  // Tin.  Control point == Tin + End  == (0,-1)
           0.25, 0.25, 0.75, 0.75   // Linear time interpolation
           )

       send(
           1      // Time
           0,0    // Ending point (0,0)
           )
    */

    const std::string TEST = "scurve(0.0, 0,0, 0,1, 0,1, 0.25,0.25,0.75,0.75) "
                             "scurve(0.5, 1,0, 0,-1, 0,-1, 0.25,0.25,0.75,0.75) "
                             "send(1.0, 0,0)";

    // X-coordinate
    ASSERT_TRUE(CheckCurve(
        session,
        "spatial(2,0) " + TEST,
        {{0, 0},
         {0.25, 0.5},
         {0.50, 1.0},
         {0.75, 0.5},
         {1.00, 0}}));

    // Y-coordinate
    ASSERT_TRUE(CheckCurve(
        session,
        "spatial(2,1) " + TEST,
        {{0, 0},
         {0.25, 0.75},
         {0.50, 0},
         {0.75, -0.75},
         {1.00, 0}}));
}

