/**
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <iomanip>

#include "gtest/gtest.h"

#include "apl/engine/evaluate.h"
#include "apl/content/metrics.h"
#include "apl/content/content.h"
#include "apl/engine/rootcontext.h"
#include "apl/engine/context.h"

#include "testeventloop.h"

using namespace apl;

static Object o(const char *s){ return Object(s); }
static Object o(bool b){ return Object(b); }
static Object o(int i){ return Object(i); }
static Object o(double d){ return Object(d); }
static Object oad(double d){ return Object(Dimension(d)); }
static Object ord(double d){ return Object(Dimension(DimensionType::Relative, d)); }


::testing::AssertionResult MatchString(const char *target, const char *source, const ContextPtr& context)
{
    auto result = evaluate(*context, source).asString();
    if (result == target)
        return ::testing::AssertionSuccess();
    else
        return ::testing::AssertionFailure() << "source '" << source
                                             << "' evaluated to '" << result
                                             << "' instead of '" << target << "'";

}

class GrammarTest : public ::testing::Test {
public:
    void loadDocument(const char *doc, int width, int height) {
        auto content = Content::create(doc, makeDefaultSession());
        ASSERT_TRUE(content);
        ASSERT_TRUE(content->isReady());  // No parameters, no imports

        auto m = Metrics().size(width, height);
        root = RootContext::create(m, content);
        ASSERT_TRUE(root);
        c = root->contextPtr();
        ASSERT_TRUE(c);
    }

    void loadDocument(const char *doc) {
        loadDocument(doc, 1024, 800);
    }

    Object
    eval(const char *expression, int width, int height, int dpi)
    {
        auto m = Metrics().size(width, height).dpi(dpi);
        auto c = Context::create(m, makeDefaultSession());
        rapidjson::Document person;
        person.SetObject();
        person.AddMember("surname", rapidjson::Value("Pat").Move(), person.GetAllocator());
        c->putConstant("person", person);
        return evaluate(*c, expression);
    }

    Object
    eval(const char *expression, int width, int height)
    {
        return eval(expression, width, height, 160);
    }

    Object
    eval(const char *expression)
    {
        return eval(expression, 1024, 800);
    }

    RootContextPtr root;
    ContextPtr c;
};

TEST_F(GrammarTest, Strings)
{
    EXPECT_EQ(o(""), eval(""));
    EXPECT_EQ(o("   "), eval("   "));
    EXPECT_EQ(o("\n"), eval("\n"));
    EXPECT_EQ(o("ख़ुशी"), eval("ख़ुशी"));
    EXPECT_EQ(o(u8"ख़ुशी"), eval(u8"\u0916\u093C\u0941\u0936\u0940"));

    // Sanity check that rapidjson is converting into UTF-8
    rapidjson::Document doc;
    doc.Parse("\"\\u0916\\u093C\\u0941\\u0936\\u0940\"");  // String version
    EXPECT_STREQ(u8"ख़ुशी", doc.GetString());
}

TEST_F(GrammarTest, Symbols)
{
    EXPECT_EQ(o(""), eval(""));
    EXPECT_EQ(o("nothing"), eval("nothing"));
    EXPECT_EQ(o("   "), eval("   "));
    EXPECT_EQ(o(""), eval("${}"));
    EXPECT_EQ(o(""), eval("${''}"));
    EXPECT_EQ(Object::TRUE(), eval("${true}"));
    EXPECT_EQ(Object::FALSE(), eval("${false}"));
    EXPECT_EQ(Object::NULL_OBJECT(), eval("${null}"));
    EXPECT_EQ(o(6), eval("${6}"));
    EXPECT_EQ(o("${    "), eval("${    "));
}

TEST_F(GrammarTest, UnaryOperations)
{
    EXPECT_EQ(o(false), eval("${!true}"));
    EXPECT_EQ(o(false), eval("${!!false}"));
    EXPECT_EQ(o(-2.5), eval("${-2.5}"));
    EXPECT_EQ(o(12), eval("${++12}"));
    EXPECT_EQ(o(true), eval("${!!-12}"));
    EXPECT_EQ(o(false), eval("${!-12}"));
}

TEST_F(GrammarTest, Arithmetic)
{
    // Examples from documentation
    EXPECT_EQ(o(3), eval("${1+2}"));
    EXPECT_EQ(o(-1), eval("${1-2}"));
    EXPECT_EQ(o(2), eval("${1*2}"));
    EXPECT_EQ(o(0.5), eval("${1/2}"));
    EXPECT_EQ(o(1), eval("${1%2}"));
    EXPECT_TRUE(eval("${0/0}").isNaN());

    EXPECT_EQ(o("27"), eval("${27+''}"));
    EXPECT_EQ(o("1 dog"), eval("${1+ ' dog'}"));
    EXPECT_EQ(o("have 3"), eval("${'have '+3 }"));

    EXPECT_EQ(o(1), eval("${ 10 % 3 }"));
    EXPECT_EQ(o(-1), eval("${ -1 % 2 }"));
    EXPECT_EQ(o(3), eval("${ 3 % -6 }"));
    EXPECT_EQ(o(0.5), eval("${6.5 % 2}"));

    // Other tests
    EXPECT_EQ(o(10), eval("${8- -2}"));
    EXPECT_EQ(o(-4), eval("${1-2-3}"));
    EXPECT_EQ(o(-5), eval("${1-2*3}"));
    EXPECT_EQ(o(-3), eval("${(1-2)*3}"));
    EXPECT_EQ(o(-1), eval("${((2-+3)*(-2--3))}"));
    EXPECT_EQ(o(5), eval("${2*3-1}"));
    EXPECT_EQ(o(5), eval("${10%3*5}"));
    EXPECT_EQ(o(1), eval("${22%3%2}"));
    EXPECT_EQ(o(1), eval("${10%-3}"));
    EXPECT_EQ(o(2.5), eval("${10/4}"));
    EXPECT_EQ(o(1.25), eval("${20/4/4}"));
    EXPECT_EQ(o(-2), eval("${-20%3}"));
    EXPECT_EQ(o(-2), eval("${-20 % -3}"));
}


TEST_F(GrammarTest, Logical)
{
    // Examples from documentation
    EXPECT_EQ(o(true), eval("${true || false}"));
    EXPECT_EQ(o(false), eval("${true && false}"));
    EXPECT_EQ(o(false), eval("${!true}"));

    EXPECT_EQ(o(2), eval("${7 && 2}"));
    EXPECT_EQ(Object::NULL_OBJECT(), eval("${null && 3}"));
    EXPECT_EQ(o(7), eval("${7 || 2}"));
    EXPECT_EQ(o(-16), eval("${0 || -16}"));
}

TEST_F(GrammarTest, Comparison)
{
    auto m = Metrics().size(1024,800);
    auto c = Context::create(m, makeDefaultSession());

    rapidjson::Document person;
    person.SetObject();
    person.AddMember("surname", rapidjson::Value("Pat").Move(), person.GetAllocator());
    Object object(person);
    c->putConstant("person", object);

    // Examples from documentation
    EXPECT_EQ(o(true), evaluate(*c, "${1<2}"));
    EXPECT_EQ(o(true), evaluate(*c, "${75 <= 100}"));
    EXPECT_EQ(o(true), evaluate(*c, "${3 > -1}"));
    EXPECT_EQ(o(true), evaluate(*c, "${4 >= 4}"));
    EXPECT_EQ(o("Pat"), evaluate(*c, "${person.surname}"));

    EXPECT_EQ(o(true), evaluate(*c, "${person.name == null}"));
    EXPECT_EQ(o(true), evaluate(*c, "${2>1 == true}"));
    EXPECT_EQ(o(true), evaluate(*c, "${1 != 2}"));

    EXPECT_EQ(o("Pat"), evaluate(*c, "${person.name ?? person.surname ?? 'Hey, you!'}"));
}

TEST_F(GrammarTest, Ternary)
{
    EXPECT_EQ(o(23), eval( "${true ? 23 : 32}"));
    EXPECT_EQ(o(23), eval( "${false ? 2 : 23}"));
    EXPECT_EQ(o(1), eval("${10==11?2:1}"));
    EXPECT_EQ(o(true), eval("${ 2 == 3 ? 4==3 : 5==1+4}"));
    EXPECT_EQ(o(false), eval("${ 2+(2 - 1) == 3 ? 4==3 : 5==1+4}"));
    EXPECT_EQ(o(23), eval("${true ? true ? 23 : 10 : 7}"));
    EXPECT_EQ(o(10), eval("${true ? false ? 23 : 10 : 7}"));
    EXPECT_EQ(o(7), eval("${false ? true ? 23 : 10 : 7}"));
    EXPECT_EQ(o(7), eval("${false ? false ? 23 : 10 : 7}"));

    EXPECT_EQ(o(7), eval("${true ? 7 : true ? 23 : 10}"));
    EXPECT_EQ(o(7), eval("${true ? 7 : false ? 23 : 10}"));
    EXPECT_EQ(o(23), eval("${false ? 7 : true ? 23 : 10}"));
    EXPECT_EQ(o(10), eval("${false ? 7 : false ? 23 : 10}"));

    EXPECT_EQ(o(23), eval("${true ? true ? 23 : 10 : true ? 22 : 9}"));
    EXPECT_EQ(o(23), eval("${true ? true ? 23 : 10 : false ? 22 : 9}"));
    EXPECT_EQ(o(10), eval("${true ? false ? 23 : 10 : true ? 22 : 9}"));
    EXPECT_EQ(o(10), eval("${true ? false ? 23 : 10 : false ? 22 : 9}"));
    EXPECT_EQ(o(22), eval("${false ? true ? 23 : 10 : true ? 22 : 9}"));
    EXPECT_EQ(o(9), eval("${false ? true ? 23 : 10 : false ? 22 : 9}"));
    EXPECT_EQ(o(22), eval("${false ? false ? 23 : 10 : true ? 22 : 9}"));
    EXPECT_EQ(o(9), eval("${false ? false ? 23 : 10 : false ? 22 : 9}"));

    EXPECT_EQ(o(false), eval("${10==11-1 ? 4 < 3 ? 'a' : 7 == 8 : 'b'}"));
    EXPECT_EQ(o("90%"), eval("${viewport.width < 500 ? '80%' : viewport.height > 10 ? '90%' : '50%'}"));
    EXPECT_EQ(o("50%"), eval("${viewport.width < 500 ? '80%' : viewport.height < 10 ? '90%' : '50%'}"));
}

TEST_F(GrammarTest, Basic)
{
    EXPECT_EQ(o(""), eval( "" ));
    EXPECT_EQ(o("1"), eval( "1" ));
    EXPECT_EQ(o(-1), eval("${-1}"));
    EXPECT_EQ(o(1), eval( "${2-1}"));
    EXPECT_EQ(o(-10), eval( "${2-3*4}"));
    EXPECT_EQ(o(-4), eval( "${(2-3)*4}"));
    EXPECT_EQ(o("height=800"), eval( "height=${viewport.height}"));
    EXPECT_EQ(o(true), eval( "${viewport.dpi >= 0}"));
    EXPECT_EQ(o(false), eval( "${viewport.dpi >= 0 && viewport.dpi < 140}"));
    EXPECT_EQ(o(true), eval( "${viewport.dpi >= 140 && viewport.dpi < 200}"));
    EXPECT_EQ(o("bunny"), eval( "${'bunny'}"));
    EXPECT_EQ(o("rabbit"), eval( "${\"rabbit\"}"));
    EXPECT_EQ(o("height=800"), eval( "${'height=' + viewport.height}"));
    EXPECT_EQ(o("embedded string 1024"), eval( "embedded ${'string ${viewport.width}'}"));

    EXPECT_EQ(o(1024), eval( "${viewport['width']}"));
    EXPECT_EQ(o(false), eval("${10==11}"));
}

TEST_F(GrammarTest, Functions)
{
    ASSERT_TRUE(IsEqual(23, eval("${Math.min(23)}")));
    ASSERT_TRUE(IsEqual(std::numeric_limits<double>::infinity(), eval("${Math.min()}")));
    ASSERT_TRUE(IsEqual(2, eval("${Math.min(2,34)}")));
    ASSERT_TRUE(IsEqual(2, eval("${Math.min(234, 23412, 2, viewport.width, 234.2)}")));

    ASSERT_TRUE(IsEqual(2, eval("${Math.max(-3,-6 * 200, 2)}")));
    ASSERT_TRUE(IsEqual(2, eval("${Math.clamp(2,-2,22)}")));
    ASSERT_TRUE(IsEqual(2, eval("${Math.abs(-2)}")));
    ASSERT_TRUE(IsEqual(-1, eval("${Math.ceil(-1.432)}")));
    ASSERT_TRUE(IsEqual(2, eval("${Math.floor(2.99999)}")));
    ASSERT_TRUE(IsEqual(2, eval("${Math.round(2.3)}")));
    ASSERT_TRUE(IsEqual(2, eval("${Math.round(1.51)}")));

    ASSERT_TRUE(IsEqual(2, eval("${Math.sqrt(4)}")));

    ASSERT_TRUE(IsEqual("1.0", eval("${environment.agentVersion}")));

    ASSERT_TRUE(IsEqual("fuzzy", eval("${String.toLowerCase('FUzZY')}")));
    ASSERT_TRUE(IsEqual("FUZZY", eval("${String.toUpperCase('FUzZY')}")));
    ASSERT_TRUE(IsEqual("rr", eval("${String.slice('berry', 2, 4)}")));
    ASSERT_TRUE(IsEqual("ry", eval("${String.slice('berry', -2)}")));
    // TODO: TEST_F LISTS OF 0, 1, and N arguments
}

TEST_F(GrammarTest, FunctionsNaN)
{
    ASSERT_TRUE(eval("${Math.max(2,3,'fuzzy')}").isNaN());
    ASSERT_TRUE(eval("${Math.min(2,3,'fuzzy')}").isNaN());
}

TEST_F(GrammarTest, Resources)
{
    auto m = Metrics().size(1024,800);
    auto c = Context::create(m, makeDefaultSession());
    c->putConstant("@name", "fred");

    EXPECT_EQ("fred", c->opt("@name").asString());
    EXPECT_EQ("fred", evaluate(*c, "${@name}").asString());
    EXPECT_EQ("fredfred", evaluate(*c, "${@name + @name}").asString());
}

TEST_F(GrammarTest, Objects)
{
    auto m = Metrics().size(1024,800);
    auto c = Context::create(m, makeDefaultSession());
    c->putConstant("ages", std::vector<Object>{10, 24, 82});

    EXPECT_EQ(3, evaluate(*c, "${ages.length}").asNumber());
    EXPECT_EQ(3, evaluate(*c, "${ages['length']}").asNumber());
    EXPECT_EQ(10, evaluate(*c, "${ages[0]}").asNumber());
    EXPECT_EQ(24, evaluate(*c, "${ages[1]}").asNumber());
    EXPECT_EQ(82, evaluate(*c, "${ages[2]}").asNumber());
    EXPECT_EQ(Object::NULL_OBJECT(), evaluate(*c, "${ages[4]}"));
    EXPECT_EQ(80, evaluate(*c, "${ages[-1]-2}").asNumber());
}

static const char *RICH_OBJECT =
    "{"
    "  \"name\": \"Random band name\","
    "  \"members\": ["
    "    {"
    "      \"name\": {"
    "        \"first\": \"Fred\","
    "        \"last\": \"Flintstone\""
    "      },"
    "      \"age\": 43"
    "    },"
    "    {"
    "      \"name\": {"
    "        \"first\": \"Wilma\","
    "        \"last\": \"Flintstone\""
    "      },"
    "      \"age\": 44"
    "    }"
    "  ]"
    "}";

TEST_F(GrammarTest, RichObject)
{
    auto m = Metrics().size(1024, 800);
    auto c = Context::create(m, makeDefaultSession());
    JsonData data(RICH_OBJECT);
    c->putConstant("payload", data.get());

    EXPECT_EQ(43, evaluate(*c, "${payload.members[0].age}").asNumber());
    EXPECT_EQ(44, evaluate(*c, "${payload.members[-1].age}").asNumber());
    EXPECT_EQ(std::string("Flintstone"), evaluate(*c, "${payload.members[0].name.last}").asString());
}

const char *STRING_RESOURCES =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\""
    "    }"
    "  },"
    "  \"resources\": ["
    "    {"
    "      \"colors\": {"
    "        \"myRed\": \"red\""
    "      },"
    "      \"dimensions\": {"
    "        \"myAbsolute\": \"20px\","
    "        \"myRelative\": \"20%\","
    "        \"myAuto\": \"auto\""
    "      },"
    "      \"gradients\": {"
    "        \"myGradient\": {"
    "          \"colorRange\": ["
    "            \"blue\","
    "            \"red\""
    "          ]"
    "        }"
    "      }"
    "    }"
    "  ]"
    "}";


TEST_F(GrammarTest, ImplicitStringConversion)
{
    loadDocument(STRING_RESOURCES);

    c->putConstant("myArray", std::vector<Object>{10, 24, 82});
    c->putConstant("myMap", std::make_shared<ObjectMap>(
        std::initializer_list<ObjectMap::value_type>{{"a", 1}}));

    EXPECT_TRUE(MatchString("", "${null}", c));
    EXPECT_TRUE(MatchString("true", "${1==1}", c));
    EXPECT_TRUE(MatchString("false", "${1==0}", c));
    EXPECT_TRUE(MatchString("-23", "${3-26}", c));
    EXPECT_TRUE(MatchString("0.333333", "${1/3}", c));
    EXPECT_TRUE(MatchString("My dog ", "${'My dog '}", c));
    EXPECT_TRUE(MatchString("3 blind mice", "${3+' blind mice'}", c));
    EXPECT_TRUE(MatchString("", "${myArray}", c));
    EXPECT_TRUE(MatchString("", "${myMap}", c));

    EXPECT_TRUE(MatchString("#ff0000ff", "${@myRed}", c));
    EXPECT_TRUE(MatchString("", "${@myGradient}", c));
    EXPECT_TRUE(MatchString("20dp", "${@myAbsolute}", c));
    EXPECT_TRUE(MatchString("20%", "${@myRelative}", c));
    EXPECT_TRUE(MatchString("auto", "${@myAuto}", c));
    EXPECT_TRUE(MatchString("", "${Math.min}", c));
}

static const std::vector<const char *> MALFORMED = {
    "${",
    "${'}",
    "${'''}  ",
    "${${}",
    // sym_term operators: "*", "%", "/"
    "${*}",
    "${/}",
    "${%}",
    "${* *}",
    "${/ *}",
    "${% /}",
    "${3 * }",
    "${* 3}",
    "${4/}",
    // sym_expr operators: "+", "-"
    "${+}",
    "${2+}",
    "${23 - 234 -}",
    // Comparisons
    "${ <= 2}",
    "${ 3 > }",
    "${ == == }",
    "${====}",
    "${55 === 55}",
    "${2 !=== 3}",
    "${!= 4}",
    "${2 >=== 1}",
    "${2 >== 1}",
    // Logical or and and
    "${ && 23 }",
    "${ 23 || }",
    // Null coalescence
    "${ null ?? !}",
    "${ null ?? }",
    "${ ?? }",
    "${ ?? foo }",
    // Ternary
    "${?:}",
    "${2?:}",
    "${?2:}",
    "${?:2}",
    "${2?1:}",
    "${0?1:}",
    "${2 ?: 1}",
    "${? 2 : 1}",
    // Grouping
    "${ 2 * ()}",
    "${()}",
    "${ ( }",
    "${ ) }",
    "${ 2 + (1 + (",
    // Attribute access
    "${ foo[ }",
    "${ foo] }",
    "${ [] }",
    "${ ] }",
    "${ [ }",
    "${ ][ }",
    "${ foo[bar[] }",
    "${ foo. }",
    "${ .foo }",
    "${ foo.bar[.] }",
    // Embedded strings
    "${ ' }",
    "${ \" }",
    "${ '${23'}' }",
    // Function calls
    "${ Math.min(}",
    "${ Math.min(1,)}",
    "${ Math.min(,2)}",
    "${Math.min(2,3,4,5+)}",
    // Various other incorrect orderings
    "${0 0}",
    "${1 -}",
    "${- 2 +}",
    "${true ? false}",
    "${tru %}",
    "${% 2}",
    "${ true ! }",
    "${ true !! false }",
    "${ 234..34 }",
    "${ 2.3.4 }",
    "${ 2.3. }",
    "${ ..23 }",
    //dimensions
    "${50 vwvh}",
    "${50 vhvw}",
    "${50 dpx}",
    "${50 pxdp}",
    "${50 dppx}",
    "${px 50}",
    "${dp 50}",
    "${vh 50}",
    "${vw 50}",
    "${10e-3dp}",
    "${10.4534e-3dp}",
    "${4!dp}",
    "${4@px}",
};

TEST_F(GrammarTest, Malformed)
{
    for (const auto& m : MALFORMED)
        EXPECT_TRUE(IsEqual(m, eval(m))) << m;
}

static const char* DIMENSIONS_DOC =
    "{\n"
    "  \"type\": \"APL\",\n"
    "  \"version\": \"1.0\",\n"
    "  \"mainTemplate\": {\n"
    "    \"item\": {\n"
    "      \"type\": \"Container\",\n"
    "      \"width\": \"${viewport.width > 500 ? 500px : 50vw}\",\n"
    "      \"height\": \"${viewport.height < 500 ? 500px : 50vh}\"\n"
    "    }\n"
    "  }\n"
    "}";

struct DimensionTestCase {
    DimensionTestCase(int width, int height, const Rect&& bounds)
        : width(width), height(height), bounds(std::move(bounds)) {}
    int width;
    int height;
    Rect bounds;
};

static const std::vector<DimensionTestCase> DIMENSION_TEST_CASES = {
    { 100, 800, Rect(0, 0, 50, 400)  }, // false, false
    { 100, 200, Rect(0, 0, 50, 500)  }, // false, true
    { 600, 900, Rect(0, 0, 500, 450) }, // true, false
    { 600, 200, Rect(0, 0, 500, 500) }, // true, true
};

TEST_F(GrammarTest, DimensionsDoc)
{
    for (const auto& m : DIMENSION_TEST_CASES) {
        loadDocument( DIMENSIONS_DOC, m.width, m.height);
        auto bounds = root->topComponent()->getCalculated(kPropertyBounds);
        IsEqual(Object(Rect(m.bounds)), bounds)
            << "width: " << m.bounds.getWidth()
            << "height: " << m.bounds.getHeight();
    }
}

TEST_F(GrammarTest, ViewportSizes)
{
    //in quotes should evaluate to just strings
    EXPECT_EQ(o("100px"), eval("${'100px'}"));
    EXPECT_EQ(o("100dp"), eval("${'100dp'}"));
    EXPECT_EQ(o("100vh"), eval("${'100vh'}"));
    EXPECT_EQ(o("100vw"), eval("${'100vw'}"));
    EXPECT_EQ(o("50vh60vh"), eval("${'50vh' + '60vh'}")); //should concat

    //simple evaluation of each unit
    EXPECT_EQ(oad(50), eval("${100px}", 1000, 1000, 320));
    EXPECT_EQ(oad(100), eval("${100dp}"));
    EXPECT_EQ(oad(600), eval("${50vw}", 1200, 800));
    EXPECT_EQ(oad(400), eval("${50vh}", 1200, 800));

    //with whitespace
    EXPECT_EQ(oad(50), eval("${100 px}", 1000, 1000, 320));
    EXPECT_EQ(oad(100), eval("${100  dp}"));
    EXPECT_EQ(oad(600), eval("${50    vw}", 1200, 800));
    EXPECT_EQ(oad(400), eval("${50     vh}", 1200, 800));
    EXPECT_EQ(oad(50), eval("${ 100 px}", 1000, 1000, 320));
    EXPECT_EQ(oad(100), eval("${  100  dp}"));
    EXPECT_EQ(oad(600), eval("${    50    vw}", 1200, 800));
    EXPECT_EQ(oad(400), eval("${     50     vh}", 1200, 800));
    EXPECT_EQ(oad(50), eval("${ 100px }", 1000, 1000, 320));
    EXPECT_EQ(oad(100), eval("${  100dp  }"));
    EXPECT_EQ(oad(600), eval("${    50vw   }", 1200, 800));
    EXPECT_EQ(oad(400), eval("${     50vh     }", 1200, 800));

    //undefined operations
    EXPECT_TRUE(eval("${5vw * 10vw}").isNaN()); //can't multiple two dims
    EXPECT_TRUE(eval("${'50vh' - 60vh}").isNaN()); //can't subtract dim and string
    EXPECT_TRUE(eval("${'50vh' * 60vh}").isNaN()); //can't multiply dim and string
    EXPECT_TRUE(eval("${'50vh' / 60vh}").isNaN()); //can't divide dim and string
    EXPECT_TRUE(eval("${'50vh' % 60vh}").isNaN()); //can't mod dim and string
    EXPECT_TRUE(eval("${105 % 10px}").isNaN()); //modulus of scalar and dim is undefined

    //math and combinations
    EXPECT_EQ(o(10), eval("${100vw / 10vw}")); //division of two rel dims is a scalar
    EXPECT_EQ(o(10), eval("${100px / 10px}")); //division of two abs dims is a scalar
    EXPECT_EQ(o(5), eval("${105px % 10px}")); //modulus of two abs dims is a scalar
    EXPECT_EQ(oad(5), eval("${105px % 10}")); //modulus of dims and scalar is a dim
    EXPECT_EQ(oad(400), eval("${100vw * 0.5}", 800, 600)); //dim * scalar
    EXPECT_EQ(oad(400), eval("${0.5 * 100vw}", 800, 600)); //scalar * dim
    EXPECT_EQ(oad(150), eval("${10vw + 5vw}", 1000, 800));
    EXPECT_EQ(oad(300), eval("${10vh + 5vh}", 1000, 2000));
    EXPECT_EQ(oad(300), eval("${10vh + 100}", 1000, 2000));
    EXPECT_EQ(oad(150), eval("${10vh + 5vh - 150dp}", 1000, 2000)); //mixed units

    // coersion
    EXPECT_EQ(o("300dp60vh"), eval("${50vh + '60vh'}", 800, 600));
    EXPECT_EQ(o("50vh360dp"), eval("${'50vh' + 60vh}", 800, 600));

    //comparison
    EXPECT_TRUE(eval("${50vw == 600}", 1200, 800).getBoolean());
    EXPECT_TRUE(eval("${600 == 50vw}", 1200, 800).getBoolean());
    EXPECT_TRUE(eval("${50vw == 600dp}", 1200, 800).getBoolean());
    EXPECT_TRUE(eval("${600dp == 50vw}", 1200, 800).getBoolean());
    EXPECT_TRUE(eval("${50vw == 1200px}", 2400, 1600, 320).getBoolean());
    EXPECT_TRUE(eval("${1200px == 50vw}", 2400, 1600, 320).getBoolean());
    EXPECT_TRUE(eval("${1201px > 50vw}", 2400, 1600, 320).getBoolean());
    EXPECT_TRUE(eval("${1201px >= 50vw}", 2400, 1600, 320).getBoolean());
    EXPECT_TRUE(eval("${1200px >= 50vw}", 2400, 1600, 320).getBoolean());
    EXPECT_TRUE(eval("${1199px < 50vw}", 2400, 1600, 320).getBoolean());
    EXPECT_TRUE(eval("${1199px <= 50vw}", 2400, 1600, 320).getBoolean());
    EXPECT_TRUE(eval("${1200px <= 50vw}", 2400, 1600, 320).getBoolean());
    EXPECT_FALSE(eval("${50vw != 600}", 1200, 800).getBoolean());
    EXPECT_FALSE(eval("${600 != 50vw}", 1200, 800).getBoolean());
    EXPECT_FALSE(eval("${50vw != 600dp}", 1200, 800).getBoolean());
    EXPECT_FALSE(eval("${600dp != 50vw}", 1200, 800).getBoolean());
    EXPECT_FALSE(eval("${50vw != 1200px}", 2400, 1600, 320).getBoolean());
    EXPECT_FALSE(eval("${1200px != 50vw}", 2400, 1600, 320).getBoolean());

    //in a ternary expression
    EXPECT_EQ(oad(500), eval("${true ? 50vh : 100vw}", 2000, 1000));
    EXPECT_EQ(oad(2000), eval("${false ? 50vh : 100vw}", 2000, 1000));
    EXPECT_EQ(oad(250), eval("${50vw == 1000 ? (100dp + 150dp) : 100vw}", 2000, 1000));
    EXPECT_EQ(oad(250), eval("${1000 == 50vw ? (100dp + 150dp) : 100vw}", 2000, 1000));

    //more complex expression using quotes
    EXPECT_EQ(o("50vh"), eval("${true ? '50vh' : '100vw'}"));
    EXPECT_EQ(o("100vw"), eval("${false ? '50vh' : '100vw'}"));
    EXPECT_EQ(o("50dp"), eval("${true ? '50dp' : '100%'}"));
    EXPECT_EQ(o("100%"), eval("${false ? '50dp' : '100%'}"));
    EXPECT_EQ(o("50px"), eval("${true ? '50px' : '100px'}"));
    EXPECT_EQ(o("100px"), eval("${false ? '50px' : '100px'}"));
}

TEST_F(GrammarTest, CustomFunctionsAndAttributes)
{
    auto m = Metrics().size(1024,800);
    auto c = Context::create(m, makeDefaultSession());

    auto map = std::make_shared<ObjectMap>();
    map->emplace("alwaysOne", [](const std::vector<Object>& args){ return Object(1); });
    map->emplace("firstArg", [](const std::vector<Object>& args){ return args.at(0); });
    map->emplace("argCount", [](const std::vector<Object>& args){ return Object(args.size()); });
    map->emplace("foo", std::vector<Object>{"a", "b", "c", "d"});
    c->putConstant("Test", map);
    c->putConstant("myArray", std::vector<Object>({10,20,30,40}));
    c->putConstant("myShortArray", std::vector<Object>({3,2,1,0}));

    // Examples from documentation
    ASSERT_TRUE(IsEqual(1, evaluate(*c, "${Test.alwaysOne()}")));
    ASSERT_TRUE(IsEqual("fuzzy", evaluate(*c, "${Test.firstArg('fuzzy', 'dice')}")));
    ASSERT_TRUE(IsEqual(3, evaluate(*c, "${Test.argCount(1,2,3)}")));
    ASSERT_TRUE(IsEqual(20, evaluate(*c, "${myArray[1]}")));
    ASSERT_TRUE(IsEqual(20, evaluate(*c, "${myArray[Test.alwaysOne()]}")));
    ASSERT_TRUE(IsEqual("d", evaluate(*c, "${Test.foo[-1]}")));
    ASSERT_TRUE(IsEqual("b", evaluate(*c, "${Test['foo'][Test.argCount(99)]}")));
    ASSERT_TRUE(IsEqual(30, evaluate(*c, "${myArray[Math.min(2,10)]}")));
    ASSERT_TRUE(IsEqual(10, evaluate(*c, "${myArray[myShortArray[-1]]}")));
}