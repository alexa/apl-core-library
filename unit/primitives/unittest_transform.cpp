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

#include "gtest/gtest.h"

#include "../testeventloop.h"

#include <cmath>

using namespace apl;

const double EPSILON = 0.00001;

class TransformTest : public MemoryWrapper {
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
        context = Context::createTestContext(metrics, session);
        json = std::unique_ptr<JsonData>(new JsonData(data));

        array = Transformation::create(*context, arrayify(*context, json->get()));
    }

    void interpolate(const std::string& data)
    {
        metrics = Metrics().size(1024,800).dpi(dpi);
        context = Context::createTestContext(metrics, session);
        json = std::unique_ptr<JsonData>(new JsonData(data));

        array = InterpolatedTransformation::create(*context,
                                                   arrayify(*context, json->get()["from"]),
                                                   arrayify(*context, json->get()["to"]));
    }

    void loadWithContext(const std::string& data, const ObjectMap& values)
    {
        metrics = Metrics().size(1024,800).dpi(dpi);
        context = Context::createTestContext(metrics, session);
        for (auto it : values)
            context->putConstant(it.first, it.second);
        json = std::unique_ptr<JsonData>(new JsonData(data));
        array = Transformation::create(*context, arrayify(*context, json->get()));
    }

    void TearDown() override
    {
        context = nullptr;
        MemoryWrapper::TearDown();
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

    ASSERT_EQ(Point(10,0), Transform2D::translate(Point(10,0)) * Point());
    ASSERT_EQ(Point(20,10), Transform2D::translate(Point(10,0)) * Point(10,10));
    ASSERT_EQ(Point(0,12), Transform2D::translate(Point(0,12)) * Point());
    ASSERT_EQ(Point(37.5,-23), Transform2D::translate(Point(37.5,-23))* Point());

    ASSERT_EQ(Point(10,0), Transform2D::translateX(10) * Point());
    ASSERT_EQ(Point(0,10), Transform2D::translateY(10) * Point());

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

TEST_F(TransformTest, Singular)
{
    ASSERT_FALSE(Transform2D().singular());
    ASSERT_TRUE(Transform2D::scale(0).singular());
    ASSERT_TRUE(Transform2D::scaleX(0).singular());
    ASSERT_TRUE(Transform2D::scaleY(0).singular());
    ASSERT_FALSE(Transform2D::rotate(30).singular());
    ASSERT_TRUE(Transform2D::scale(0).inverse().singular());

    ASSERT_TRUE(Transform2D::translateX(INFINITY).singular());
    ASSERT_TRUE(Transform2D::translateY(INFINITY).singular());
    ASSERT_TRUE(Transform2D::rotate(INFINITY).singular());
    ASSERT_TRUE(Transform2D::scale(INFINITY).singular());
    ASSERT_TRUE(Transform2D::skewX(INFINITY).singular());
    ASSERT_TRUE(Transform2D::skewY(INFINITY).singular());
    
    ASSERT_TRUE(Transform2D::translateX(NAN).singular());
    ASSERT_TRUE(Transform2D::translateY(NAN).singular());
    ASSERT_TRUE(Transform2D::rotate(NAN).singular());
    ASSERT_TRUE(Transform2D::scale(NAN).singular());
    ASSERT_TRUE(Transform2D::skewX(NAN).singular());
    ASSERT_TRUE(Transform2D::skewY(NAN).singular());
}

TEST_F(TransformTest, Inverse) {
    // The unit matrix is its own inverse
    ASSERT_TRUE(IsEqual(Transform2D().inverse(), Transform2D()));

    // Inverting a scaling matrix
    ASSERT_TRUE(IsEqual(Transform2D::scale(0.5).inverse(), Transform2D::scale(2)));
    ASSERT_TRUE(IsEqual(Transform2D::scaleX(0.25).inverse(), Transform2D::scaleX(4)));

    // Inverting translation
    ASSERT_TRUE(
        IsEqual(Transform2D::translate(10, 20).inverse(), Transform2D::translate(-10, -20)));

    // Inverting rotation
    ASSERT_TRUE(IsEqual(Transform2D::rotate(45).inverse(), Transform2D::rotate(-45)));

    // Complicated combined transformation
    ASSERT_TRUE(IsEqual((Transform2D::rotate(90) * Transform2D::translateX(20)).inverse(),
                        (Transform2D::translateX(-20) * Transform2D::rotate(-90))));

    auto t = Transform2D({1.1, 1.2, 1.3, 1.4, 1.5});
    ASSERT_TRUE(IsEqual(t * t.inverse(), Transform2D()));
    ASSERT_TRUE(IsEqual(t.inverse() * t, Transform2D()));
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

// Assuming a width=40, height=20  [delta=(20,10)]
static const std::vector<TestCase> PARSE_TEST_CASES = {
        {"rotate(90 20 10)",                               {10, 10}, {20,  0}},  // (10,10) -> (-10,0) -> (0,-10) -> (20,0)
        {"rotate(90)",                                     {10, 10}, {-10, 10}},
        {"scale(2)",                                       {40, 20}, {80,  40}},
        {"scale(0.5,2)",                                   {40, 20}, {20,  40}},
        {"scale(2,0.5)",                                   {40, 20}, {80,  10}},
        {"skewX(45)",                                      {40, 20}, {60,  20}},
        {"skewY(45)",                                      {40, 20}, {40,  60}},
        {"translate(+100)",                                {10, 10}, {110, 10}},
        {"translate(0,100)",                               {10, 10}, {10,  110}},
        {"translate(20 10) scale(2,1) translate(-20 -10)", {20, 10}, {20,  10}},  // Scale about center: (20,10) -> (0, 0) -> (0,0) -> (20, 10)
        {"translate(20 10) scale(2,1) translate(-20 -10)", {40, 20}, {60,  20}},  // Scale about center: (40,20) -> (20, 10) -> (40,10) -> (60, 20)
        {"translate(10)scale(2)",                          {10, 10}, {30,  20}},  // (10,10) -> (20,20) -> (30,20)
};

TEST_F(TransformTest, ParseTestCases)
{
    for (const auto& test : PARSE_TEST_CASES) {
        auto transform = Transform2D::parse(session, test.data);
        EXPECT_EQ(test.end, transform * test.start) << "Test case: " << test.data;
    }
}

TEST_F(TransformTest, NumberParsing)
{
    // Move these tests inside the function because they call Transform2D::scale when initialized
    std::map<std::string, Transform2D> const NUMBER_PARSE = {
        {"scale(2)",          Transform2D::scale(2)},
        {"scale(2.5)",        Transform2D::scale(2.5)},
        {"scale(00002)",      Transform2D::scale(2)},
        {"scale(.5)",         Transform2D::scale(0.5)},
        {"scale(.500000000)", Transform2D::scale(0.5)},
        {"scale(+2)",         Transform2D::scale(2)},
        {"scale(-2)",         Transform2D::scale(-2)},
        {"scale(2e1)",        Transform2D::scale(20)},
        {"scale(2E1)",        Transform2D::scale(20)},
        {"scale(2e+2)",       Transform2D::scale(200)},
        {"scale(10e-1)",      Transform2D::scale(1)},
        {"scale(5e0)",        Transform2D::scale(5)},
        {"scale(1+2)",        Transform2D::scale(1,2)},   // Note that "1+2" is valid
        {"scale(1-2)",        Transform2D::scale(1,-2)},
    };

    for (const auto& test : NUMBER_PARSE) {
        auto transform = Transform2D::parse(session, test.first);
        EXPECT_EQ(test.second, transform) << "Test case: " << test.first;
        EXPECT_FALSE(session->checkAndClear());
    }
}

static const std::vector<std::string> EMPTY_TRANSFORMS = {
        "",
        "    ",
        "  translate(0)",
        "translate(0,0)",
        "   rotate(0  23 42) ",
        "skewX ( 0 )",
        "skewY ( 0.0 )",
        "scale(1)",
        "scale (1,1) ",
        "scale( 1     0001)",
};

TEST_F(TransformTest, ParseEmptyTransforms)
{
    for (const auto& test : EMPTY_TRANSFORMS) {
        auto transform = Transform2D::parse(session, test);
        EXPECT_EQ(Transform2D(), transform) << "Test case: " << test;
        EXPECT_FALSE(session->checkAndClear());
    }
}

static const std::vector<std::string> BAD_TRANSFORMS = {
        "22",
        "t",
        "transl(0)",
        "translate",
        "translate(",
        "translate()",
        "translate(1 2 3)",
        "translate(+)",
        "translate(-)",
        "translate(++2)",
        "translate(--2)",
        "translate(1 2) rotate",
        "rotate 45",
        "rotate(45",
        "rotate 45)",
        "rotate()",
        "rotate(45 2)",
        "rotate(45 2 3 4)",
        "rotate(45.4.4)",
        "rotate(45,,2)",
        "rotate(,22)",
        "rotate,(22)",
        "rotate(22,)",
        "rotate(22,   )",
        "rotate(22,13)",
        "rotate(22 , 12 , )",
        "rotate( 22, 12, 31,)",
        "skewx(10)",
        "skewy(10)",
        "skewX(10,20)",
        "skewY()",
        "scale()",
        "scale(1,2,4)",
        "rotate(45) + translate(0)",
        "rotate(45 /* comment */)",
        "scale(2e)",
        "scale(2E)",
        "scale(2E+)",
        "scale(2e-)",
        "scale(2e11+)"
};

TEST_F(TransformTest, ParseBadTransforms)
{
    for (const auto& test : BAD_TRANSFORMS) {
        auto transform = Transform2D::parse(session, test);
        ASSERT_EQ(Transform2D(), transform) << "Test case: " << test;
        ASSERT_TRUE(session->checkAndClear());
    }
}


/**
 * Axis aligned bounding box
 */
TEST_F(TransformTest, AABB)
{

    auto rect  = Rect (-1,-1,2,2);

    auto t2d = Transform2D();
    ASSERT_EQ(Rect(-1,-1,2,2), t2d.calculateAxisAlignedBoundingBox(rect));

    t2d = Transform2D::translate(1,1);
    ASSERT_EQ(Rect(0,0,2,2), t2d.calculateAxisAlignedBoundingBox(rect));

    t2d = Transform2D::scale(20,10);
    ASSERT_EQ(Rect(-20,-10,40,20) , t2d.calculateAxisAlignedBoundingBox(rect));

    t2d = Transform2D::skewX(45);
    ASSERT_EQ(Rect(-2,-1,4,2), t2d.calculateAxisAlignedBoundingBox(rect));
    t2d = Transform2D::skewY(45);
    ASSERT_EQ(Rect(-1,-2,2,4), t2d.calculateAxisAlignedBoundingBox(rect));

    t2d = Transform2D::rotate(90);
    ASSERT_EQ(Rect(-1,-1,2,2), t2d.calculateAxisAlignedBoundingBox(rect));

    t2d = Transform2D::rotate(45);
    auto r = Rect(-1.414214,-1.414214,2.828428,2.828428);
    auto result = t2d.calculateAxisAlignedBoundingBox(rect);
    ASSERT_PRED2(Close,r.getTopLeft(), result.getTopLeft());
    ASSERT_PRED2(Close,r.getBottomRight(), result.getBottomRight());
}

// TEST ARRAYIFICATION
