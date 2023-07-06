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

#include <iostream>
#include <memory>
#include <clocale>
#include <cfenv>

#include "gtest/gtest.h"
#include "../testeventloop.h"

#include "apl/animation/easing.h"
#include "apl/component/component.h"
#include "apl/component/componenteventsourcewrapper.h"
#include "apl/content/metrics.h"
#include "apl/engine/arrayify.h"
#include "apl/engine/context.h"
#include "apl/livedata/livearrayobject.h"
#include "apl/livedata/livedataobject.h"
#include "apl/livedata/livemapobject.h"
#include "apl/primitives/gradient.h"
#include "apl/primitives/object.h"
#include "apl/primitives/rect.h"
#include "apl/primitives/transform.h"
#include "apl/utils/session.h"

using namespace apl;

TEST(ObjectTest, Constants)
{
    ASSERT_TRUE( Object::TRUE_OBJECT().isBoolean());
    ASSERT_TRUE( Object::TRUE_OBJECT().getBoolean());
    ASSERT_TRUE( Object::FALSE_OBJECT().isBoolean());
    ASSERT_FALSE( Object::FALSE_OBJECT().getBoolean());

    ASSERT_TRUE( Object::NULL_OBJECT().isNull());
    ASSERT_TRUE( Object::NAN_OBJECT().isNumber());
    ASSERT_TRUE( Object(Dimension()).isAutoDimension());
    ASSERT_TRUE( Object::EMPTY_ARRAY().isArray());
    ASSERT_TRUE( Object(Rect()).is<Rect>());
}

TEST(ObjectTest, Basic)
{
    ASSERT_TRUE( Object().isNull() );
    ASSERT_TRUE( Object(true).isBoolean());
    ASSERT_TRUE( Object(false).isBoolean());
    ASSERT_TRUE( Object(10).isNumber());
    ASSERT_TRUE( Object((uint32_t) 23).isNumber());
    ASSERT_TRUE( Object(10.2).isNumber());
    ASSERT_TRUE( Object("fuzzy").isString());
    ASSERT_TRUE( Object(std::string("fuzzy")).isString());
}

TEST(ObjectTest, Size)
{
    ASSERT_TRUE(Object::NULL_OBJECT().empty());

    ASSERT_FALSE(Object("fuzzy").empty());
    ASSERT_TRUE(Object("").empty());
    ASSERT_FALSE(Object(1).empty());
    ASSERT_FALSE(Object(false).empty());

    Object a = Object(std::make_shared<std::map<std::string, Object>>());
    ASSERT_TRUE(a.empty());
    ASSERT_EQ(0, a.size());

    a = Object(std::make_shared<std::vector<Object>>());
    ASSERT_TRUE(a.empty());
    ASSERT_EQ(0, a.size());

    a = Object(std::vector<Object>{});
    ASSERT_TRUE(a.empty());
    ASSERT_EQ(0, a.size());

    a = Object(rapidjson::Value(rapidjson::kArrayType));
    ASSERT_TRUE(a.empty());
    ASSERT_EQ(0, a.size());

    a = Object(rapidjson::Value(rapidjson::kObjectType));
    ASSERT_TRUE(a.empty());
    ASSERT_EQ(0, a.size());

    rapidjson::Document doc;
    doc.Parse("[1,2,3]");
    a = Object(std::move(doc));
    ASSERT_FALSE(a.empty());
    ASSERT_EQ(3, a.size());

    a = Object(Rect(0,0,0,0));
    ASSERT_TRUE(a.empty());
    ASSERT_EQ(0, a.size());

    ASSERT_TRUE(Object::EMPTY_ARRAY().empty());
    ASSERT_TRUE(Object(Rect()).empty());
}

TEST(ObjectTest, SharedMap)
{
    Object a = Object(std::make_shared< std::map< std::string, Object> >(
            std::map<std::string, Object>({
        { "a", 1 }, { "b", false }, { "c", "fuzzy"}
    })));
    ASSERT_TRUE(a.isMap());
    ASSERT_EQ(3, a.size());
    ASSERT_FALSE(a.empty());
    ASSERT_TRUE(a.has("a"));
    ASSERT_FALSE(a.has("z"));
    ASSERT_EQ(1, a.opt("a", 42).asNumber());
    ASSERT_EQ(42, a.opt("z", 42).asNumber());

    ASSERT_STREQ("fuzzy", a.get("c").getString().c_str());
}

TEST(ObjectTest, SharedVector)
{
    Object a = Object(std::make_shared< std::vector<Object>>(
            std::vector<Object>({ true, 1, "fuzzy" })));

    ASSERT_TRUE(a.isArray());
    ASSERT_EQ(3, a.size());
    ASSERT_FALSE(a.empty());
    ASSERT_TRUE(a.at(0).isBoolean());
    ASSERT_EQ(1, a.at(1).getInteger());
    ASSERT_STREQ("fuzzy", a.at(2).getString().c_str());
}

TEST(ObjectTest, Vector)
{
    Object a = Object({ true, 1, "fuzzy" });
    ASSERT_TRUE(a.isArray());
    ASSERT_EQ(3, a.size());
    ASSERT_FALSE(a.empty());
    ASSERT_TRUE(a.at(0).isBoolean());
    ASSERT_EQ(1, a.at(1).getInteger());
    ASSERT_STREQ("fuzzy", a.at(2).getString().c_str());
}

TEST(ObjectTest, RapidJson)
{
    // Note: We require the JSON object to be persistent.
    rapidjson::Value v(10);
    Object o = Object(v);

    ASSERT_TRUE(o.isNumber());
    ASSERT_EQ(10, o.getInteger());

    rapidjson::Value v2("twelve");
    ASSERT_TRUE(Object(v2).isString());
    ASSERT_STREQ("twelve", Object(v2).getString().c_str());

    rapidjson::Value v3(true);
    ASSERT_TRUE(Object(v3).isBoolean());
    ASSERT_TRUE(Object(v3).getBoolean());

    rapidjson::Value v4;
    ASSERT_TRUE(Object(v4).isNull());

    rapidjson::Document doc;
    rapidjson::Value v5(rapidjson::kArrayType);
    v5.PushBack(rapidjson::Value(5).Move(), doc.GetAllocator());
    v5.PushBack(rapidjson::Value(10).Move(), doc.GetAllocator());
    Object o5(v5);

    ASSERT_TRUE(o5.isArray());
    ASSERT_EQ(2, o5.size());
    ASSERT_FALSE(o5.empty());
    ASSERT_EQ(5, o5.at(0).getInteger());

    rapidjson::Value v6(rapidjson::kObjectType);
    v6.AddMember("name", rapidjson::Value("Pat").Move(), doc.GetAllocator());
    v6.AddMember("firstname", rapidjson::Value("Siva").Move(), doc.GetAllocator());
    Object o6(v6);

    ASSERT_TRUE(o6.isMap());
    ASSERT_EQ(2, o6.size());
    ASSERT_FALSE(o6.empty());
    ASSERT_STREQ("Siva", o6.get("firstname").getString().c_str());
    ASSERT_FALSE(o6.has("surname"));

    rapidjson::Document d;
    d.Parse(R"({"a":2,"b": 4})");
    o = Object(std::move(d));
    ASSERT_TRUE(o.isMap());
    ASSERT_EQ(2, o.size());
    ASSERT_EQ(2, o.get("a").getDouble());
}

TEST(ObjectTest, Color)
{
    class TestSession : public Session {
    public:
        void write(const char *filename, const char *func, const char *value) override {
            count++;
        }

        int count = 0;
    };

    auto session = std::make_shared<TestSession>();

    Object o = Object(Color(Color::RED));
    ASSERT_TRUE(o.is<Color>());
    ASSERT_EQ(Color::RED, o.asColor(session));

    o = Object(Color::RED);
    ASSERT_TRUE(o.isNumber());
    ASSERT_EQ(Color::RED, o.asColor(session));

    o = Object("red");
    ASSERT_TRUE(o.isString());
    ASSERT_EQ(Color::RED, o.asColor(session));

    o = Object::NULL_OBJECT();
    ASSERT_TRUE(o.isNull());
    ASSERT_EQ(Color::TRANSPARENT, o.asColor(session));

    o = Object("blue");
    ASSERT_EQ(Color::BLUE, o.asColor(session));
    ASSERT_EQ(0, session->count);

    o = Object("splunge");
    ASSERT_EQ(Color::TRANSPARENT, o.asColor(session));
    ASSERT_EQ(1, session->count);
}

// TDOO: Dimension unit tests

TEST(ObjectTest, Gradient)
{
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    rapidjson::Value colorRange(rapidjson::kArrayType);
    colorRange.PushBack("red", allocator);
    colorRange.PushBack("blue", allocator);
    doc.AddMember("colorRange", colorRange, allocator);
    doc.AddMember("type", "radial", allocator);

    auto context = Context::createTestContext(Metrics().size(1024,800), makeDefaultSession());

    Object a = Gradient::create(*context, doc);

    ASSERT_TRUE(a.is<Gradient>());
    ASSERT_EQ(Gradient::RADIAL, a.get<Gradient>().getType());
    ASSERT_EQ(0xff0000ff, a.get<Gradient>().getProperty(kGradientPropertyColorRange).at(0).getColor());

    Object b(a);
    ASSERT_TRUE(b.is<Gradient>());
    ASSERT_EQ(Gradient::RADIAL, b.get<Gradient>().getType());
    ASSERT_EQ(0xff0000ff, b.get<Gradient>().getProperty(kGradientPropertyColorRange).at(0).getColor());

    Object c;
    c = a;
    ASSERT_TRUE(c.is<Gradient>());
    ASSERT_EQ(Gradient::RADIAL, c.get<Gradient>().getType());
    ASSERT_EQ(0xff0000ff, c.get<Gradient>().getProperty(kGradientPropertyColorRange).at(0).getColor());

    {
        rapidjson::Document doc2;
        doc2.SetObject();
        rapidjson::Document::AllocatorType& alloc = doc2.GetAllocator();

        rapidjson::Value colorRng(rapidjson::kArrayType);
        colorRng.PushBack("blue", alloc);
        colorRng.PushBack("green", alloc);
        doc2.AddMember("colorRange", colorRng, alloc);
        doc2.AddMember("type", "linear", alloc);

        auto p = Gradient::create(*context, doc2);
        c = p;
    }

    ASSERT_TRUE(c.is<Gradient>());
    ASSERT_EQ(Gradient::LINEAR, c.get<Gradient>().getType());
    ASSERT_EQ(0x0000ffff, c.get<Gradient>().getProperty(kGradientPropertyColorRange).at(0).getColor());

    b = c;
    ASSERT_TRUE(b.is<Gradient>());
    ASSERT_EQ(Gradient::LINEAR, b.get<Gradient>().getType());
    ASSERT_EQ(0x0000ffff, b.get<Gradient>().getProperty(kGradientPropertyColorRange).at(0).getColor());

    // Make sure a has not changed
    ASSERT_TRUE(a.is<Gradient>());
    ASSERT_EQ(Gradient::RADIAL, a.get<Gradient>().getType());
    ASSERT_EQ(0xff0000ff, a.get<Gradient>().getProperty(kGradientPropertyColorRange).at(0).getColor());
}

const char *BAD_CASES =
    "{"
    "  \"badType\": {"
    "    \"type\": \"fuzzy\","
    "    \"colorRange\": ["
    "      \"red\","
    "      \"green\""
    "    ]"
    "  },"
    "  \"tooShort\": {"
    "    \"type\": \"linear\","
    "    \"colorRange\": ["
    "      \"red\""
    "    ]"
    "  },"
    "  \"mismatchedRange\": {"
    "    \"type\": \"radial\","
    "    \"colorRange\": ["
    "      \"red\","
    "      \"blue\","
    "      \"green\","
    "      \"purple\""
    "    ],"
    "    \"inputRange\": ["
    "      0,"
    "      0.5,"
    "      1"
    "    ]"
    "  },"
    "  \"rangeOutOfBounds\": {"
    "    \"type\": \"linear\","
    "    \"colorRange\": ["
    "      \"red\","
    "      \"blue\""
    "    ],"
    "    \"inputRange\": ["
    "      0,"
    "      1.2"
    "    ]"
    "  },"
    "  \"rangeOutOfBounds2\": {"
    "    \"type\": \"linear\","
    "    \"colorRange\": ["
    "      \"red\","
    "      \"blue\""
    "    ],"
    "    \"inputRange\": ["
    "      -0.3,"
    "      1.0"
    "    ]"
    "  },"
    "  \"rangeMisordered\": {"
    "    \"type\": \"linear\","
    "    \"colorRange\": ["
    "      \"red\","
    "      \"blue\""
    "    ],"
    "    \"inputRange\": ["
    "      1,"
    "      0"
    "    ]"
    "  }"
    "}";


TEST(ObjectTest, MalformedGradient)
{
    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(BAD_CASES);
    ASSERT_TRUE(ok);

    auto context = Context::createTestContext(Metrics().size(1024,800), makeDefaultSession());

    for (auto& m : doc.GetObject()) {
        Object result = Gradient::create(*context, m.value);
        ASSERT_TRUE(result.isNull()) << " Failed on test " << m.name.GetString();
    }
}

TEST(ObjectTest, Rect)
{
    Object a = Object(Rect(0,10,100,200));
    ASSERT_TRUE(a.is<Rect>());
    auto r = a.get<Rect>();
    ASSERT_EQ(0, r.getX());
    ASSERT_EQ(10, r.getY());
    ASSERT_EQ(100, r.getWidth());
    ASSERT_EQ(200, r.getHeight());
}

const static char *SCALE =
    "["
    "  {"
    "    \"scale\": 3"
    "  }"
    "]";

TEST(ObjectTest, Transform)
{
    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(SCALE);
    ASSERT_TRUE(ok);

    auto context = Context::createTestContext(Metrics().size(1024,800), makeDefaultSession());

    auto transform = Transformation::create(*context, arrayify(*context, Object(doc)));

    Object a = Object(transform);
    ASSERT_TRUE(a.is<Transformation>());
    ASSERT_EQ(Point(-20,-20), a.get<Transformation>()->get(20,20) * Point());
}

TEST(ObjectTest, Transform2)
{
    Object a = Object(Transform2D::rotate(90));
    ASSERT_TRUE(a.is<Transform2D>());
    ASSERT_EQ(Transform2D::rotate(90), a.get<Transform2D>());
}

TEST(ObjectTest, Easing)
{
    Object a = Object(Easing::linear());
    ASSERT_TRUE(a.is<Easing>());
    ASSERT_EQ(0.5, a.get<Easing>()->calc(0.5));

    auto session = makeDefaultSession();
    a = Object(Easing::parse(session, "ease"));
    ASSERT_TRUE(a.is<Easing>());
    ASSERT_NEAR(0.80240017, a.get<Easing>()->calc(0.5), 0.0001);
}

TEST(ObjectTest, Radii)
{
    Object a = Object(Radii());
    ASSERT_EQ(Object(Radii()), a);
    ASSERT_TRUE(a.get<Radii>().empty());

    Object b = Object(Radii(4));
    ASSERT_TRUE(b.is<Radii>());
    ASSERT_EQ(4, b.get<Radii>().bottomLeft());
    ASSERT_EQ(4, b.get<Radii>().bottomRight());
    ASSERT_EQ(4, b.get<Radii>().topLeft());
    ASSERT_EQ(4, b.get<Radii>().topRight());
    ASSERT_FALSE(b.get<Radii>().empty());

    Object c = Object(Radii(1,2,3,4));
    ASSERT_TRUE(c.is<Radii>());
    ASSERT_EQ(1, c.get<Radii>().topLeft());
    ASSERT_EQ(2, c.get<Radii>().topRight());
    ASSERT_EQ(3, c.get<Radii>().bottomLeft());
    ASSERT_EQ(4, c.get<Radii>().bottomRight());
    ASSERT_EQ(1, c.get<Radii>().radius(Radii::kTopLeft));
    ASSERT_EQ(2, c.get<Radii>().radius(Radii::kTopRight));
    ASSERT_EQ(3, c.get<Radii>().radius(Radii::kBottomLeft));
    ASSERT_EQ(4, c.get<Radii>().radius(Radii::kBottomRight));
    ASSERT_EQ(Radii(1,2,3,4), c.get<Radii>());
    ASSERT_NE(Radii(1,2,3,5), c.get<Radii>());
    ASSERT_FALSE(c.get<Radii>().empty());

    auto foo = std::array<float, 4>{1,2,3,4};
    ASSERT_EQ(foo, c.get<Radii>().get());
}

// NOTE: These test cases assume a '.' decimal separator.
//       Different locales will behave differently
static const std::vector<std::pair<double, std::string>> DOUBLE_TEST {
    { 0, "0" },
    { -1, "-1"},
    { 1, "1"},
    { 123451, "123451"},
    { 2147483647, "2147483647"},  // Largest 32 bit signed integer
    { 10000000000, "10000000000"}, // Larger than 32 bit integer
    { 1234567890123, "1234567890123"}, // Really big
    { -2147483648, "-2147483648"},  // Smallest 32 bit signed integer
    { -10000000000, "-10000000000"}, // Smaller than 32 bit integer
    { -1234567890123, "-1234567890123"}, // Really small
    { 0.5, "0.5"},
    { -0.5, "-0.5"},
    { 0.0001, "0.0001"},
    { -0.0001, "-0.0001"},
    { 0.050501010101, "0.050501"},
    { 0.199999999999, "0.2"},  // Should round up appropriately
//    { 0.000000001, "0"},   // TODO: This formats in scientific notation, which we don't handle
//    { -0.000000001, "0"},  // TODO: This formats in scientific notation, which we don't handle
};

TEST(ObjectTest, DoubleConversion)
{
    feclearexcept (FE_ALL_EXCEPT);
    for (const auto& m : DOUBLE_TEST) {
        Object object(m.first);
        std::string result = object.asString();
        ASSERT_EQ(m.second, result) << m.first << " : " << m.second;
    }
    // Note: Not all architectures support FE_INVALID (e.g., wasm)
#if defined(FE_INVALID)
    int fe = fetestexcept (FE_INVALID);
    ASSERT_EQ(0, fe);
#endif
}

TEST(ObjectTest, DoubleConversionIgnoresCLocale)
{
    std::string previousLocale = std::setlocale(LC_NUMERIC, nullptr);
    std::setlocale(LC_NUMERIC, "fr_FR.UTF-8");

    feclearexcept (FE_ALL_EXCEPT);
    for (const auto& m : DOUBLE_TEST) {
        Object object(m.first);
        std::string result = object.asString();
        EXPECT_EQ(m.second, result) << m.first << " : " << m.second;
    }

    std::setlocale(LC_NUMERIC, previousLocale.c_str());

    // Note: Not all architectures support FE_INVALID (e.g., wasm)
#if defined(FE_INVALID)
    int fe = fetestexcept (FE_INVALID);
    ASSERT_EQ(0, fe);
#endif
}

static const std::vector<std::pair<std::string, double>> STRING_TO_DOUBLE{
    {"0",        0},
    {"1",        1},
    {"2.5",      2.5},
    {"2.",       2},
    {"-12.25",   -12.25},
    {"    4   ", 4},
    {" 125%",    1.25},
    {"100    %", 1},
    {"100 /%",   100},  // The '/' terminates the search for %
    {"1 4",      1},
    {"1e2",      100},
    {"",         std::numeric_limits<double>::quiet_NaN()},
    {"- 10",     std::numeric_limits<double>::quiet_NaN()},
    {"%",        std::numeric_limits<double>::quiet_NaN()},
    {"% 123",    std::numeric_limits<double>::quiet_NaN()},
    {"INF",      std::numeric_limits<double>::infinity()},
    {"NAN",      std::numeric_limits<double>::quiet_NaN()},
    {"INF%",     std::numeric_limits<double>::infinity()},
    {"NAN%",     std::numeric_limits<double>::quiet_NaN()},
};

TEST(ObjectTest, StringToDouble)
{
    for (const auto& m : STRING_TO_DOUBLE) {
        Object object(m.first);
        double result = object.asNumber();
        if (std::isnan(result) && std::isnan(m.second))  // NaN do not compare as equal, but they are valid
            continue;
        ASSERT_EQ(m.second, result) << "'" << m.first << "' : " << m.second;
    }
}

TEST(ObjectTest, StringToDoubleIgnoresCLocale)
{
    std::string previousLocale = std::setlocale(LC_NUMERIC, nullptr);
    std::setlocale(LC_NUMERIC, "fr_FR.UTF-8");

    for (const auto& m : STRING_TO_DOUBLE) {
        Object object(m.first);
        double result = object.asNumber();
        if (std::isnan(result) && std::isnan(m.second))  // NaN do not compare as equal, but they are valid
            continue;
        EXPECT_EQ(m.second, result) << "'" << m.first << "' : " << m.second;
    }

    std::setlocale(LC_NUMERIC, previousLocale.c_str());
}

TEST(ObjectTest, AbsoluteDimensionConversion)
{
    auto dimension = Object(Dimension(DimensionType::Absolute, 42));
    ASSERT_EQ(42, dimension.asNumber());
    ASSERT_EQ(42, dimension.asInt());
    ASSERT_EQ("42dp", dimension.asString());
    ASSERT_TRUE(dimension.asBoolean());
}

TEST(ObjectTest, MutableObjects)
{
    ASSERT_FALSE(Object(Radii()).isMutable());
    ASSERT_FALSE(Object::NULL_OBJECT().isMutable());

    ASSERT_FALSE(Object::EMPTY_ARRAY().isMutable());
    ASSERT_TRUE(Object::EMPTY_MUTABLE_ARRAY().isMutable());
    ASSERT_FALSE(Object::EMPTY_MAP().isMutable());
    ASSERT_TRUE(Object::EMPTY_MUTABLE_MAP().isMutable());

    // ========= Shared pointer to map
    auto mapPtr = std::make_shared<ObjectMap>(ObjectMap{{"a", 1}, {"b", 2}});
    ASSERT_FALSE(Object(mapPtr).isMutable());
    ASSERT_TRUE(Object(mapPtr, true).isMutable());

    // Check that retrieving the mutable map fails if the object is not marked as mutable
    ASSERT_DEATH(Object(mapPtr).getMutableMap(), "Attempted to retrieve mutable map for non-mutable object");

    // Check that retrieving the mutable map succeeds if the object is marked as mutable
    ObjectMap& map = Object(mapPtr, true).getMutableMap();
    ASSERT_EQ(2, map.size());

    // ========= Shared pointer to array
    auto arrayPtr = std::make_shared<ObjectArray>(ObjectArray{1,2,3,4});
    ASSERT_FALSE(Object(arrayPtr).isMutable());
    ASSERT_TRUE(Object(arrayPtr, true).isMutable());
    ASSERT_EQ(Object(arrayPtr).toDebugString(), "Array<size=4>[1.000000, 2.000000, 3.000000, 4.000000, ]");
    ASSERT_EQ(Object(arrayPtr).at(10), Object::NULL_OBJECT());

    // Check that retrieving the mutable array fails if the object is not marked as mutable
    ASSERT_DEATH(Object(arrayPtr).getMutableArray(), "Attempted to retrieve mutable array for non-mutable object");

    // Check that retrieving the mutable array succeeds if the object is marked as mutable
    ObjectArray& array = Object(arrayPtr, true).getMutableArray();
    ASSERT_EQ(4, array.size());

    // ========= Emplaced object array
    ASSERT_FALSE(Object(ObjectArray{1,2}).isMutable());
    ASSERT_TRUE(Object(ObjectArray{2,3}, true).isMutable());

    ASSERT_DEATH(Object(ObjectArray{1,2,3}).getMutableArray(), "Attempted to retrieve mutable array for non-mutable object");

    array = Object(ObjectArray{2,3,4}, true).getMutableArray();
    ASSERT_EQ(3, array.size());
}

TEST(ObjectTest, IntLongFloatNumber)
{
    ASSERT_EQ(0, Object::NULL_OBJECT().asInt());
    ASSERT_EQ(0, Object::FALSE_OBJECT().asInt());
    ASSERT_EQ(1, Object::TRUE_OBJECT().asInt());
    ASSERT_EQ(32, Object(32).asInt());
    ASSERT_EQ(33, Object(32.5).asInt());
    ASSERT_EQ(23, Object("23").asInt());
    ASSERT_EQ(23, Object("0x17").asInt(0));
    ASSERT_EQ(23, Object("23.9999").asInt());
    ASSERT_EQ(23, Object(Dimension(23)).asInt());
    ASSERT_EQ(0, Object(Dimension(DimensionType::Relative, 23)).asInt());  // Relative dimensions don't have a integer type

    int max_int = std::numeric_limits<int>::max();
    int64_t max_int_in_long = max_int;
    auto bigNumber = Object(max_int_in_long + 1);
    ASSERT_EQ(max_int_in_long + 1, bigNumber.asInt64());
    if (sizeof(int) <= sizeof(4)) {
        ASSERT_NE(max_int_in_long + 1, bigNumber.asInt());
    }

    ASSERT_EQ(0LL, Object::NULL_OBJECT().asInt64());
    ASSERT_EQ(0LL, Object::FALSE_OBJECT().asInt64());
    ASSERT_EQ(1LL, Object::TRUE_OBJECT().asInt64());
    ASSERT_EQ(32LL, Object(32).asInt64());
    ASSERT_EQ(33LL, Object(32.5).asInt64());
    ASSERT_EQ(23LL, Object("23").asInt64());
    ASSERT_EQ(23LL, Object("0x17").asInt64(0));
    ASSERT_EQ(23LL, Object("23.9999").asInt64());
    ASSERT_EQ(23LL, Object(Dimension(23)).asInt64());
    ASSERT_EQ(0LL, Object(Dimension(DimensionType::Relative, 23)).asInt64());  // Relative dimensions don't have a integer type

    int64_t max_long_in_double = 9007199254740992;  // 2^53: Largest integer before we get rounding errors
    ASSERT_EQ(max_long_in_double, Object(max_long_in_double).asInt64());
    ASSERT_NE(max_long_in_double + 1, Object(max_long_in_double + 1).asInt64());

    ASSERT_TRUE(std::isnan(Object::NULL_OBJECT().asNumber()));
    ASSERT_EQ(0.0, Object::FALSE_OBJECT().asNumber());
    ASSERT_EQ(1.0, Object::TRUE_OBJECT().asNumber());
    ASSERT_EQ(32.0, Object(32).asNumber());
    ASSERT_EQ(32.5, Object(32.5).asNumber());
    ASSERT_EQ(23.0, Object("23").asNumber());
    ASSERT_NEAR(23.9999, Object("23.9999").asNumber(), 0.000001);
    ASSERT_EQ(23.5, Object(Dimension(23.5)).asNumber());
    ASSERT_TRUE(std::isnan(Object(Dimension(DimensionType::Relative, 23)).asNumber()));  // Relative dimensions don't have a integer type
}

TEST(ObjectTest, WhenDimensionIsNotFiniteSerializeReturnsZero)
{
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    auto nanDimensionTest = Dimension(DimensionType::Absolute, NAN);
    auto nanObject = Object(nanDimensionTest);
    ASSERT_EQ(nanObject.serialize(allocator), rapidjson::Value(0));

    auto infDimensionTest = Dimension(DimensionType::Absolute, INFINITY);
    auto infObject = Object(infDimensionTest);
    ASSERT_EQ(infObject.serialize(allocator), rapidjson::Value(0));
}

class DocumentObjectTest : public CommandTest {};

static const char * SEND_EVENT_DIMENSION_NAN = R"apl({
  "type": "APL",
  "version": "1.1",
  "resources": [
    {
      "dimension": {
        "absDimen": "${100/0}"
      }
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "onPress": {
        "type": "SendEvent",
        "arguments": [
          "@absDimen"
        ]
      }
    }
  }
})apl";

TEST_F(DocumentObjectTest, WithNewArguments)
{
    loadDocument(SEND_EVENT_DIMENSION_NAN);

    auto expectedObject = Object(0);

    performClick(1, 1);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    auto args = event.getValue(kEventPropertyArguments);
    ASSERT_TRUE(args.isArray());
    ASSERT_EQ(args.size(), 1);
    ASSERT_TRUE(IsEqual(expectedObject, args.at(0)));
}


static const char * SEND_EVENT_NUMBER_NAN = R"apl({
  "type": "APL",
  "version": "1.1",
  "resources": [
    {
      "number": {
        "value": "${100/0}"
      }
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "onPress": {
        "type": "SendEvent",
        "arguments": [
          "@value"
        ]
      }
    }
  }
})apl";

TEST_F(DocumentObjectTest, WhenNumberIsNotFiniteSerializeReturnsNull)
{
    loadDocument(SEND_EVENT_NUMBER_NAN);

    auto expectedObject = Object();

    performClick(1, 1);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    auto args = event.getValue(kEventPropertyArguments);
    ASSERT_TRUE(args.isArray());
    ASSERT_EQ(args.size(), 1);
    ASSERT_TRUE(IsEqual(expectedObject, args.at(0)));
}


enum SerializedType {
    kInt32,
    kInt64,
    kDouble,
    kOther
};
struct SerializedTestCase {
    double value;
    SerializedType type;
};

static const auto SERIALIZED_CASES = std::vector<SerializedTestCase>{
    { 0, kInt32 },
    { 2, kInt32 },
    { -23, kInt32 },
    { 2147483647, kInt32 },  // 2^31-1
    { -2147483648, kInt32 }, // -2^31
    { 2147483648, kInt64 },  // Just a little too large
    { -2147483649, kInt64 },  // Just a little too small
    { 9007199254740990, kInt64 },  // 2^53 - 2
    { 9007199254740991, kInt64 },  // 2^53 - 1
    { 9007199254740992, kInt64 },  // 2^53
    { -9007199254740990, kInt64 },  // 2^53 - 2
    { -9007199254740991, kInt64 },  // 2^53 - 1
    { -9007199254740992, kInt64 },  // 2^53
    { 2e54, kDouble },
    { -2e54, kDouble },
    { 0.0000001, kDouble },
    { -0.0000001, kDouble },
    { 2147483647.01, kDouble },
    { -2147483647.01, kDouble },
    { INFINITY, kOther },
    { -INFINITY, kOther },
    { NAN, kOther },
};

TEST_F(DocumentObjectTest, SerializeDoubles)
{
    rapidjson::Document doc;

    for (const auto& m : SERIALIZED_CASES) {
        auto value = Object(m.value).serialize(doc.GetAllocator());
        ASSERT_TRUE(value.IsInt() == (m.type == kInt32)) << m.value;
        ASSERT_TRUE(value.IsInt64() == (m.type == kInt32 || m.type == kInt64)) << m.value;
        ASSERT_TRUE(value.IsDouble() == (m.type == kDouble)) << m.value;
        ASSERT_TRUE(value.IsNull() == (m.type == kOther));
    }
}


TEST_F(DocumentObjectTest, ArrayComparison)
{
    rapidjson::Document jsonArrays;
    jsonArrays.Parse(R"({"array1": [0,1,2,3], "array2": [0,1,2,3], "array3": [1,1,2,3]})");

    auto& jsonArray1 = jsonArrays["array1"];
    auto& jsonArray2 = jsonArrays["array2"];
    auto& jsonArray3 = jsonArrays["array3"];

    ASSERT_TRUE(jsonArray1 == jsonArray2);
    ASSERT_TRUE(jsonArray3 != jsonArray2);

    ASSERT_TRUE(Object(jsonArray1) == Object(jsonArray2));
    ASSERT_TRUE(Object(jsonArray3) != Object(jsonArray2));

    auto objectArray1 = std::make_shared<ObjectArray>(std::vector<Object>({0,1,2,3}));
    auto objectArray2 = std::make_shared<ObjectArray>(std::vector<Object>({0,1,2,3}));
    auto objectArray3 = std::make_shared<ObjectArray>(std::vector<Object>({1,1,2,3}));

    ASSERT_TRUE(*objectArray1 == *objectArray2);
    ASSERT_TRUE(*objectArray3 != *objectArray2);

    ASSERT_TRUE(Object(objectArray1) == Object(objectArray2));
    ASSERT_TRUE(Object(objectArray3) != Object(objectArray2));

    ASSERT_TRUE(Object(jsonArray1) == Object(objectArray1));
    ASSERT_TRUE(Object(objectArray2) == Object(jsonArray2));
    ASSERT_TRUE(Object(jsonArray1) != Object(objectArray3));
    ASSERT_TRUE(Object(objectArray1) != Object(jsonArray3));

    auto fixedObjectArray1 = Object(std::vector<Object>({0,1,2,3}));
    auto fixedObjectArray2 = Object(std::vector<Object>({0,1,2,3}));
    auto fixedObjectArray3 = Object(std::vector<Object>({1,1,2,3}));

    ASSERT_TRUE(fixedObjectArray1 == fixedObjectArray2);
    ASSERT_TRUE(fixedObjectArray3 != fixedObjectArray2);

    ASSERT_TRUE(Object(fixedObjectArray1) == Object(objectArray1));
    ASSERT_TRUE(Object(fixedObjectArray1) == Object(jsonArray1));
    ASSERT_TRUE(Object(fixedObjectArray1) == Object(fixedObjectArray1));

    ASSERT_TRUE(Object(fixedObjectArray1) != Object(objectArray3));
    ASSERT_TRUE(Object(fixedObjectArray1) != Object(jsonArray3));
    ASSERT_TRUE(Object(fixedObjectArray1) != Object(fixedObjectArray3));
}

TEST_F(DocumentObjectTest, MapComparison)
{
    rapidjson::Document jsonMaps;
    jsonMaps.Parse(R"({"map1": {"one": 1, "two": 2}, "map2": {"one": 1, "two": 2}, "map3": {"one": 2, "two": 1}})");

    auto& jsonMap1 = jsonMaps["map1"];
    auto& jsonMap2 = jsonMaps["map2"];
    auto& jsonMap3 = jsonMaps["map3"];

    ASSERT_TRUE(jsonMap1 == jsonMap2);
    ASSERT_TRUE(jsonMap3 != jsonMap2);

    ASSERT_TRUE(Object(jsonMap1) == Object(jsonMap2));
    ASSERT_TRUE(Object(jsonMap3) != Object(jsonMap2));

    auto objectMap1 = std::make_shared<ObjectMap>(std::map<std::string, Object>({{"one", 1}, {"two", 2}}));
    auto objectMap2 = std::make_shared<ObjectMap>(std::map<std::string, Object>({{"one", 1}, {"two", 2}}));
    auto objectMap3 = std::make_shared<ObjectMap>(std::map<std::string, Object>({{"one", 2}, {"two", 1}}));

    ASSERT_TRUE(*objectMap1 == *objectMap2);
    ASSERT_TRUE(*objectMap3 != *objectMap2);

    ASSERT_TRUE(Object(objectMap1) == Object(objectMap2));
    ASSERT_TRUE(Object(objectMap3) != Object(objectMap2));

    ASSERT_TRUE(Object(jsonMap1) == Object(objectMap1));
    ASSERT_TRUE(Object(objectMap2) == Object(jsonMap2));
    ASSERT_TRUE(Object(jsonMap1) != Object(objectMap3));
    ASSERT_TRUE(Object(objectMap1) != Object(jsonMap3));
}

static const char * STYLED_TEXT_CAST = R"apl({
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "text": "10.5"
    }
  }
})apl";

TEST_F(DocumentObjectTest, StyledTextCast)
{
    loadDocument(STYLED_TEXT_CAST);
    auto st = component->getProperty(kPropertyText);
    ASSERT_EQ(st.asNumber(), 10.5);
    ASSERT_EQ(st.asInt(), 10);
    ASSERT_EQ(st.asInt64(), 10);
}

TEST_F(DocumentObjectTest, Truthy)
{
    loadDocument(STYLED_TEXT_CAST);
    ASSERT_TRUE(Object(ComponentEventSourceWrapper::create(component, "", Object::NULL_OBJECT())).truthy());
    ASSERT_FALSE(Object(ComponentEventSourceWrapper::create(nullptr, "", Object::NULL_OBJECT())).truthy());
}

TEST_F(DocumentObjectTest, LiveDataAccess)
{
    loadDocument(STYLED_TEXT_CAST);
    auto liveMap = Object(LiveDataObject::create(LiveMap::create(), context, "MAPPY")->asMap());
    ASSERT_TRUE(liveMap.isMap());
    ASSERT_TRUE(liveMap.isTrueMap());
    ASSERT_TRUE(liveMap.getLiveDataObject());

    auto liveArray = Object(LiveDataObject::create(LiveArray::create(), context, "ARRAYI")->asArray());
    ASSERT_TRUE(liveArray.isArray());
    ASSERT_TRUE(liveArray.getLiveDataObject());
}

struct EqualityStruct {
    Object first;
    Object second;
    bool result;
};

static const std::vector<EqualityStruct> EQUALITY{
    {0, 0, true},
    {0, 1, false},
    {true, true, true},
    {true, false, false},
    {"first", "first", true},
    {"first", "second", false},
    {Rect(0, 0, 100, 100), Rect(0, 0, 100, 100), true},
    {Rect(0, 0, 100, 100), Rect(0, 0, 100, 150), false},
    {Color(1), Color(1), true},
    {Color(1), Color(2), false},
    {Dimension(1), Dimension(1), true},
    {Dimension(1), Dimension(2), false},
};

TEST(ObjectTest, Equality)
{
    for (const auto& m : EQUALITY) {
        ASSERT_EQ(m.result, m.first == m.second) << "'" << m.first << "' : " << m.second;
    }
}
