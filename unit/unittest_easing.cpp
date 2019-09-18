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

#include "testeventloop.h"

#include "apl/animation/coreeasing.h"
#include "apl/animation/easinggrammar.h"

using namespace apl;

class EasingTest : public MemoryWrapper {};

TEST_F(EasingTest, Linear)
{
    LinearEasing easing;
    ASSERT_EQ(0, easing.calc(-1));
    ASSERT_EQ(0, easing.calc(0));
    ASSERT_EQ(0.5, easing.calc(0.5));
    ASSERT_EQ(1, easing.calc(1));
    ASSERT_EQ(1, easing.calc(2));
}

TEST_F(EasingTest, Path)
{
    PathEasing path({0,0, 0.5, 1, 1,1});

    ASSERT_EQ(0, path.calc(-1));
    ASSERT_EQ(0, path.calc(0));
    ASSERT_EQ(1, path.calc(1));
    ASSERT_EQ(0.25, path.calc(0.125));
    ASSERT_EQ(0.5, path.calc(0.25));
    ASSERT_EQ(1, path.calc(0.5));
    ASSERT_EQ(1, path.calc(0.75));
    ASSERT_EQ(1, path.calc(0.875));
}

TEST_F(EasingTest, CubicBezier)
{
    CubicBezierEasing path(0.31, 0.31, 0.69, 0.69);   // Linear!

    ASSERT_NEAR(0, path.calc(-1), 0.0001);
    ASSERT_NEAR(0, path.calc(0), 0.0001);
    ASSERT_NEAR(0.2, path.calc(0.2), 0.0001);
    ASSERT_NEAR(0.4, path.calc(0.4), 0.0001);
    ASSERT_NEAR(0.6, path.calc(0.6), 0.0001);
    ASSERT_NEAR(0.8, path.calc(0.8), 0.0001);
    ASSERT_NEAR(1.0, path.calc(1.0), 0.0001);
    ASSERT_NEAR(1.0, path.calc(1.2), 0.0001);
}

static float f(float a, float b, float t)
{
    return 3*t*(1-t)*(1-t)*a + 3*t*t*(1-t)*b + t*t*t;
}

TEST_F(EasingTest, CubicBezierEase)
{
    CubicBezierEasing path(0.25, 0.10, 0.25, 1.0);  // Ease

    for (int i = 0 ; i <= 10 ; i++) {
        float alpha = 0.1 * i;
        float t = f(0.25, 0.25, alpha);
        float v = f(0.10, 1.0, alpha);
        ASSERT_NEAR(v, path.calc(t), 0.0001) << "alpha=" << alpha << " t=" << t << " v=" << v;
    }
}

TEST_F(EasingTest, EasingCurve)
{
    Easing linear = Easing::parse(session, "");

    ASSERT_EQ(0.25, linear(0.25));
    ASSERT_EQ(0.5, linear(0.5));
    ASSERT_EQ(0.75, linear(0.75));

    ASSERT_TRUE(ConsoleMessage());  // It was an illegal curve
}

TEST_F(EasingTest, EasingCurveEaseIn)
{
    Easing curve = Easing::parse(session, "ease-in");

    for (int i = 0 ; i <= 10 ; i++) {
        float alpha = 0.1 * i;
        float t = f(0.42, 1, alpha);
        float v = f(0, 1, alpha);
        ASSERT_NEAR(v, curve(t), 0.0001) << "alpha=" << alpha << " t=" << t << " v=" << v;
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
    ASSERT_TRUE( IsEqual(Easing(new CubicBezierEasing(0.25, 0.10, 0.25, 1)), Easing::parse(session, "ease")));
}

TEST_F(EasingTest, EasingCurveCustom)
{
    ASSERT_FALSE(Easing::has("cubic-bezier(0.33,-0.5,0.92,0.38)"));

    Easing curve = Easing::parse(session, " cubic-bezier( 0.33, -0.5, 0.92 , 0.38  ) ");

    for (int i = 0 ; i <= 10 ; i++) {
        float alpha = 0.1 * i;
        float t = f(0.33, 0.92, alpha);
        float v = f(-0.5, 0.38, alpha);
        ASSERT_NEAR(v, curve(t), 0.0001) << "alpha=" << alpha << " t=" << t << " v=" << v;
    }

    ASSERT_TRUE(Easing::has("cubic-bezier(0.33,-0.5,0.92,0.38)"));
}

TEST_F(EasingTest, EasingPathCustom)
{
    Easing curve = Easing::parse(session, " path( 0.25, 1, 0.75, 0)");

    ASSERT_NEAR(0, curve(0), 0.0001);
    ASSERT_NEAR(0.5, curve(0.125), 0.0001);
    ASSERT_NEAR(1, curve(0.25), 0.0001);
    ASSERT_NEAR(0.5, curve(0.5), 0.0001);
    ASSERT_NEAR(0, curve(0.75), 0.0001);
    ASSERT_NEAR(0.5, curve(0.875), 0.0001);
    ASSERT_NEAR(1, curve(1), 0.0001);

    curve = Easing::parse(session, "path(0.1, 1, 0.2, 0, 0.3, 1, 0.4, 0, 0.5, 1, 0.6, 0, 0.7, 1, 0.8, 0, 0.9, 1)");

    ASSERT_NEAR(0, curve(0), 0.0001);
    ASSERT_NEAR(0.5, curve(0.05), 0.0001);
    ASSERT_NEAR(1, curve(0.1), 0.0001);
    ASSERT_NEAR(0.5, curve(0.15), 0.0001);
    ASSERT_NEAR(0, curve(0.2), 0.0001);
    ASSERT_NEAR(0.5, curve(0.25), 0.0001);
    ASSERT_NEAR(1, curve(0.3), 0.0001);
    ASSERT_NEAR(0.5, curve(0.35), 0.0001);
    ASSERT_NEAR(0, curve(0.4), 0.0001);
    ASSERT_NEAR(0.5, curve(0.45), 0.0001);
    ASSERT_NEAR(1, curve(0.5), 0.0001);
    ASSERT_NEAR(0.5, curve(0.55), 0.0001);
    ASSERT_NEAR(0, curve(0.6), 0.0001);
    ASSERT_NEAR(0.5, curve(0.65), 0.0001);
    ASSERT_NEAR(1, curve(0.7), 0.0001);
    ASSERT_NEAR(0.5, curve(0.75), 0.0001);
    ASSERT_NEAR(0, curve(0.8), 0.0001);
    ASSERT_NEAR(0.5, curve(0.85), 0.0001);
    ASSERT_NEAR(1, curve(0.9), 0.0001);
    ASSERT_NEAR(1, curve(0.95), 0.0001);
    ASSERT_NEAR(1, curve(1.0), 0.0001);
}


static std::vector<std::string> sFailureCases = {
    "foo", "path(1", "path(", "path(1)", "path(1,2,3,4,5)",
    "path(0,0)",  // The 0,0 is implicit
    "path(1,1)",
    "path(1.2,1)",
    "path(-.2,0)",
    "path(0.2,0.2,0.1,0.5)",  // Out of order
    "cubic-bezier()", "cubic-bezier(1,2,3)", "cubic-bezier(1,2,3,4,5)"
};

TEST_F(EasingTest, EasingFail)
{
    for (auto& m : sFailureCases) {
        ASSERT_TRUE(IsEqual(Easing::linear(), Easing::parse(session, m.c_str())))
            << "test case: '" << m << "'";
        ASSERT_TRUE(ConsoleMessage()) << m;
    }
}