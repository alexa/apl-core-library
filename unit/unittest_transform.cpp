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


#include "gtest/gtest.h"

#include "testeventloop.h"

#include "apl/primitives/transform2d.h"
#include "apl/primitives/transform.h"
#include "apl/content/jsondata.h"
#include "apl/content/metrics.h"
#include "apl/engine/context.h"
#include "apl/engine/arrayify.h"

using namespace apl;

const double EPSILON = 0.00001;

class TransformTest : public ::testing::Test {
public:
    TransformTest() : dpi(160) {}

    static bool Close(const Point& a, const Point& b)
    {
        return std::abs(a.getX() - b.getX()) < EPSILON &&
               std::abs(a.getY() - b.getY()) < EPSILON;
    }

    void setDpi(double value) { dpi = value; }

    void load(const std::string& data)
    {
        metrics = Metrics().size(1024,800).dpi(dpi);
        context = Context::create(metrics, makeDefaultSession());
        json = std::unique_ptr<JsonData>(new JsonData(data));

        array = Transformation::create(*context, arrayify(context, json->get()));
    }

    void interpolate(const std::string& data)
    {
        metrics = Metrics().size(1024,800).dpi(dpi);
        context = Context::create(metrics, makeDefaultSession());
        json = std::unique_ptr<JsonData>(new JsonData(data));

        array = InterpolatedTransformation::create(*context,
                                                   arrayify(context, json->get()["from"]),
                                                   arrayify(context, json->get()["to"]));
    }

    void loadWithContext(const std::string& data, const ObjectMap& values)
    {
        metrics = Metrics().size(1024,800).dpi(dpi);
        context = Context::create(metrics, makeDefaultSession());
        for (auto it : values)
            context->putConstant(it.first, it.second);
        json = std::unique_ptr<JsonData>(new JsonData(data));
        array = Transformation::create(*context, arrayify(context, json->get()));
    }

public:
    ContextPtr context;
    Metrics metrics;
    double dpi;
    std::unique_ptr<JsonData> json;
    std::shared_ptr<Transformation> array;
};

TEST_F(TransformTest, Basic)
{
    ASSERT_EQ( Point(0,0), Transform2D() * Point(0,0));
    ASSERT_EQ(Point(10,20.5), Transform2D() * Point(10,20.5));
}

TEST_F(TransformTest, Translate)
{
    ASSERT_EQ(Point(10,0), Transform2D::translate(10,0) * Point());
    ASSERT_EQ(Point(20,10), Transform2D::translate(10,0) * Point(10,10));
    ASSERT_EQ(Point(0,12), Transform2D::translate(0,12) * Point());
    ASSERT_EQ(Point(37.5,-23), Transform2D::translate(37.5,-23)* Point());

    auto t1 = Transform2D::translate(10,-20);
    auto t2 = Transform2D::translate(20,20);
    ASSERT_EQ(Point(30,0), (t1*t2)*Point());
    ASSERT_EQ(Point(30,0), t1*(t2*Point()));
    auto p = Point(12,-13);
    ASSERT_EQ( Point(42,-13), t1*t2*p);

    ASSERT_EQ(Point(10,0), Transform2D::translateX(10) * Point());
    ASSERT_EQ(Point(0,10), Transform2D::translateY(10) * Point());
}

TEST_F(TransformTest, Scale)
{
    ASSERT_EQ( Point(), Transform2D::scaleX(2) * Point(0,0));
    ASSERT_EQ( Point(2,0), Transform2D::scaleX(2) * Point(1,0));
    ASSERT_EQ( Point(0,1), Transform2D::scaleX(2) * Point(0,1));

    ASSERT_EQ( Point(), Transform2D::scaleY(2) * Point(0,0));
    ASSERT_EQ( Point(1,0), Transform2D::scaleY(2) * Point(1,0));
    ASSERT_EQ( Point(0,2), Transform2D::scaleY(2) * Point(0,1));

    ASSERT_EQ( Point(), Transform2D::scale(2) * Point(0,0));
    ASSERT_EQ( Point(2,2), Transform2D::scale(2) * Point(1,1));

    ASSERT_EQ( Point(6,6), Transform2D::scale(2) * Transform2D::scale(3) * Point(1,1));
}

TEST_F(TransformTest, Rotate)
{
    ASSERT_EQ(Point(0,0), Transform2D::rotate(45) * Point(0,0));
    ASSERT_PRED2(Close, Point(0,1), Transform2D::rotate(90) * Point(1,0));
    ASSERT_PRED2(Close, Point(-1,0), Transform2D::rotate(180) * Point(1,0));
    ASSERT_PRED2(Close, Point(0,-1), Transform2D::rotate(-90) * Point(1,0));
}

TEST_F(TransformTest, Skew)
{
    ASSERT_EQ(Point(0,0), Transform2D::skewX(45) * Point());
    ASSERT_EQ(Point(1,1), Transform2D::skewX(45) * Point(0,1));
    ASSERT_EQ(Point(2,1), Transform2D::skewX(45) * Point(1,1));

    ASSERT_EQ(Point(0,0), Transform2D::skewY(45) * Point());
    ASSERT_EQ(Point(1,1), Transform2D::skewY(45) * Point(1,0));
    ASSERT_EQ(Point(1,2), Transform2D::skewY(45) * Point(1,1));
}

TEST_F(TransformTest, Mixed)
{
    // Rotate about the point (1,1)
    auto t = Transform2D::translate(1,1) * Transform2D::rotate(90) * Transform2D::translate(-1,-1);
    ASSERT_PRED2(Close, Point(2,0), t * Point(0,0));
    ASSERT_PRED2(Close, Point(2,2), t * Point(2,0));
    ASSERT_PRED2(Close, Point(0,2), t * Point(2,2));
    ASSERT_PRED2(Close, Point(0,0), t * Point(0,2));
}

TEST_F(TransformTest, Comparison)
{
    ASSERT_EQ(Transform2D(), Transform2D());
    ASSERT_EQ(Transform2D(), Transform2D::rotate(0));
    ASSERT_NE(Transform2D(), Transform2D::rotate(10));
}

static const char *ARRAY_TEST_SCALE =
    "{"
    "  \"scale\": 2"
    "}";

TEST_F(TransformTest, SingleScale)
{
    load(ARRAY_TEST_SCALE);

    auto transform = array->get(40,20);
    ASSERT_EQ(Point(-20,-10), transform * Point(0,0));
    ASSERT_EQ(Point(20,10), transform * Point(20,10));
    ASSERT_EQ(Point(60,30), transform * Point(40,20));
}

static const char *ARRAY_TEST_PAIR =
    "["
    "  {"
    "    \"scale\": 2"
    "  },"
    "  {"
    "    \"rotate\": 90"
    "  }"
    "]";

TEST_F(TransformTest, ScaleAndRotate)
{
    load(ARRAY_TEST_PAIR);

    auto transform = array->get(40,20);  // Should be rotated about the center first, and then scaled

    // (0,0) -> (-20,-10) -> (10,-20) -> (20, -40) -> (40, -30)
    ASSERT_EQ(Point(40,-30), transform * Point(0,0));

    // (20,10) -> (0,0) -> (0,0) -> (0,0) -> (20, 10)
    ASSERT_EQ(Point(20,10), transform * Point(20,10));

    // (40,20) -> (20,10) -> (-10,20) -> (-20,40) -> (0, 50)
    ASSERT_EQ(Point(0,50), transform * Point(40,20));
}


struct TestCase {
    std::string data;
    Point start;
    Point end;
};

// Assuming a width=40, height=20  [delta=(20,10)]
static const std::vector<TestCase> ARRAY_TEST_CASES = {
    {"{\"rotate\": 90}", {10,10}, {20, 0}},        // (10,10) -> (-10,0) -> (0,-10) -> (20,0)
    {"{\"scaleX\": 2}", {40,20}, {60, 20}},        // (40,20) -> (20,10) -> (40,10) -> (60,20)
    {"{\"scaleY\": 2}", {40,20}, {40, 30}},        // (40,20) -> (20,10) -> (20,20) -> (40,30)
    {"{\"scale\": 2}", {40,20}, {60, 30}},         // (40,20) -> (20,10) -> (40,20) -> (60,30)
    {"{\"skewX\": 45}", {40,20}, {50, 20}},        // (40,20) -> (20,10) -> (30,10) -> (50,20)
    {"{\"skewY\": 45}", {40,20}, {40, 40}},        // (40,20) -> (20,10) -> (20,30) -> (40,40)
    {"{\"translateX\": 100}", {10,10}, {110, 10}}, // (10,10) -> (-10,0) -> (90,0) -> (110,10)
    {"{\"translateY\": 100}", {10,10}, {10, 110}}, // (10,10) -> (-10,0) -> (-10,100) -> (10,110)
    {"[{\"translateX\":\"-50%\",\"translateY\":\"-50%\"},{\"scaleX\":2},{\"translateX\":\"50%\",\"translateY\":\"50%\"}]",
     {20,10}, {40,10}},       // Scale about the top-left corner

    };

TEST_F(TransformTest, ManyTestCases)
{
    for (auto& test : ARRAY_TEST_CASES) {
        load(test.data);
        auto transform = array->get(40,20);
        EXPECT_EQ(test.end, transform * test.start) << "Test case: " << test.data;
    }
}

static const char * DATA_BINDING_TEST =
    "["
    "  {"
    "    \"rotate\": \"${myRotation}\""
    "  },"
    "  {"
    "    \"scaleX\": \"${myScale}\""
    "  },"
    "  {"
    "    \"translateX\": \"${myTranslate}\""
    "  }"
    "]";

TEST_F(TransformTest, ApplyDataBinding)
{
    loadWithContext(DATA_BINDING_TEST, {
        { "myRotation", 90 },
        { "myScale", 2 },
        { "myTranslate", 10}
    });

    // (0,0) -> (-10,-10) -> (0,-10) -> (0,-10) -> (10,0) -> (20,10)
    ASSERT_EQ(Point(20,10), array->get(20,20) * Point());

    // (0,0) -> (-50,-10) -> (-40,-10) -> (-80,-10) -> (10,-80) -> (60,-70)
    ASSERT_EQ(Point(60,-70), array->get(100,20) * Point());
}

static const char *SIMPLE_INTERPOLATION =
    "{"
    "  \"from\": {"
    "    \"scale\": 1"
    "  },"
    "  \"to\": {"
    "    \"scale\": 2"
    "  }"
    "}";

TEST_F(TransformTest, SimpleInterpolation)
{
    interpolate(SIMPLE_INTERPOLATION);

    auto interpolator = std::dynamic_pointer_cast<InterpolatedTransformation>(array);
    ASSERT_TRUE(interpolator);

    // (0,0) -> (-50,-10) -> (-50,-10) -> (0,0)
    ASSERT_EQ(Point(), array->get(100,20) * Point());

    interpolator->interpolate(0.5);  // Scale = 1.5
    // (0,0) -> (-50,-10) -> (-75,-15) -> (-25,-5)
    ASSERT_EQ(Point(-25,-5), array->get(100,20) * Point());

    interpolator->interpolate(1.0);  // Scale = 2
    // (0,0) -> (-50,-10) -> (-100,-20) -> (-50,-10)
    ASSERT_EQ(Point(-50,-10), array->get(100,20) * Point());
}

static const char *COMPLEX_INTERPOLATION =
    "{"
    "  \"from\": ["
    "    {"
    "      \"translateX\": \"-100dp\","
    "      \"translateY\": \"-100%\""
    "    },"
    "    {"
    "      \"scaleX\": 2"
    "    },"
    "    {"
    "      \"rotate\": 360"
    "    }"
    "  ],"
    "  \"to\": ["
    "    {"
    "      \"translateX\": \"100%\""
    "    },"
    "    {"
    "      \"scaleY\": 2"
    "    },"
    "    {"
    "      \"rotate\": 0"
    "    }"
    "  ]"
    "}";

TEST_F(TransformTest, ComplexInterpolation)
{
    interpolate(COMPLEX_INTERPOLATION);

    auto interpolator = std::dynamic_pointer_cast<InterpolatedTransformation>(array);
    ASSERT_TRUE(interpolator);

    //     Center     Rotate: 0    Scale X=2     Trans(-100, -20)   Center
    // (0,0) -> (-50,-10) -> (-50,-10) -> (-100, -10) -> (-200,-30) -> (-150, -20)
    ASSERT_EQ(Point(-150,-20), array->get(100,20) * Point());

    interpolator->interpolate(0.5);
    //     Center    Rot(180)    Scale(1.5,1.5)  Trans(0, -10)   Center
    // (0,0) -> (-50,-10) -> (50,10) -> (75, 15) -> (75,5) -> (125, 15)
    ASSERT_EQ(Point(125,15), array->get(100,20) * Point());

    interpolator->interpolate(1.0);
    //     Center      Rot(0)       Scale(1,2)    Trans(100, 0)   Center
    // (0,0) -> (-50,-10) -> (-50,-10) -> (-50, -20) -> (50,-20) -> (100, -10)
    ASSERT_EQ(Point(100,-10), array->get(100,20) * Point());
}

// TEST ARRAYIFICATION