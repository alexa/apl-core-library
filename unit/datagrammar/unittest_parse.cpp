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

#include "apl/primitives/symbolreferencemap.h"

using namespace apl;

class ParseTest : public ::testing::Test {
public:
    ParseTest() {
        session = std::make_shared<TestSession>();
        context = Context::createTestContext(Metrics(), session);
    }

    ContextPtr context;
    std::shared_ptr<TestSession> session;
};


TEST_F(ParseTest, Simple)
{
    auto foo = parseDataBinding(*context, "${}");
    ASSERT_TRUE(foo.isString());

    foo = parseDataBinding(*context, " ${}");
    ASSERT_TRUE(foo.isString());
    ASSERT_STREQ(" ", foo.asString().c_str());

    foo = parseDataBinding(*context, "${1+3}");
    ASSERT_TRUE(foo.isNumber());
    ASSERT_EQ(4, foo.asNumber());

    foo = parseDataBinding(*context, "${Math.min(23,4)}");
    ASSERT_TRUE(foo.isNumber());
    ASSERT_EQ(4, foo.asNumber());

    foo = parseDataBinding(*context, "${@red}");
    ASSERT_FALSE(foo.isEvaluable());
    ASSERT_TRUE(foo.isNull());

    context->putConstant("@red", Color(Color::RED));
    foo = parseDataBinding(*context, "${@red}");
    ASSERT_FALSE(foo.isEvaluable());
    ASSERT_TRUE(foo.isColor());
    ASSERT_TRUE(IsEqual(Color(Color::RED), foo));

    context->putUserWriteable("b", 82);
    foo = parseDataBinding(*context, "${Math.max(23,44,b)}");
    ASSERT_TRUE(foo.isEvaluable());

    foo = foo.eval();
    ASSERT_TRUE(foo.isNumber());
    ASSERT_EQ(82, foo.asNumber());
}

static const std::vector<std::pair<std::string, std::set<std::string>>> SYMBOL_TESTS = {
    {"${a+Math.min(b+(c-d),c/d)} ${e-f}",   {"a/", "b/", "c/", "d/", "e/", "f/"}},
    {"${a[b].c ? (e || f) : 'foo ${g}'}",   {"a/", "b/", "e/", "f/", "g/"}},
    {"${viewport.width > 10000 ? a : b.c}", {"b/c/"}}
};


TEST_F(ParseTest, Symbols)
{
    for (auto m : "abcdefg")
        context->putUserWriteable(std::string(1, m), "test_"+std::string(1,m));

    for (auto& m : SYMBOL_TESTS) {
        auto result = parseDataBinding(*context, m.first);
        ASSERT_TRUE(result.isEvaluable()) << m.first;

        SymbolReferenceMap symbols;
        result.symbols(symbols);
        std::set<std::string> syms;
        for (auto& p : symbols.get())
            syms.insert(p.first);

        ASSERT_EQ(m.second, syms) << m.first;
    }
}

static const std::vector<std::pair<std::string, Object>> UNARY_PLUS_TESTS = {
    {"${+1}", 1},
    {"${0}",  0},
};

static const std::vector<std::pair<Object, Object>> UNARY_PLUS_EVAL = {
    {23,                                     23},
    {0,                                      0},
    {Dimension(22),                          Dimension(22)},
    {Dimension(0),                           Dimension(0)},
    {Dimension(DimensionType::Relative, 40), Dimension(DimensionType::Relative, 40)},
    {Dimension(DimensionType::Relative, 0),  Dimension(DimensionType::Relative, 0)},
};

static const std::vector<Object> UNARY_PLUS_NAN = {
    "fuzzy", false, true, Object::NULL_OBJECT()
};

TEST_F(ParseTest, UnaryPlus)
{
    for (auto& m : UNARY_PLUS_TESTS) {
        auto result = parseDataBinding(*context, m.first);
        ASSERT_TRUE(IsEqual(m.second, result)) << m.first;
    }

    context->putUserWriteable("a", 99);
    auto result = parseDataBinding(*context, "${+a}");
    ASSERT_TRUE(result.isEvaluable());

    for (auto& m : UNARY_PLUS_EVAL) {
        ASSERT_TRUE(context->userUpdateAndRecalculate("a", m.first, false));
        ASSERT_TRUE(IsEqual(m.second, result.eval())) << m.first;
    }

    for (auto& m : UNARY_PLUS_NAN) {
        ASSERT_TRUE(context->userUpdateAndRecalculate("a", m, false));
        ASSERT_TRUE(result.eval().isNaN()) << m;
    }
}


static const std::vector<std::pair<std::string, Object>> UNARY_MINUS_TESTS = {
    {"${-1}", -1},
    {"${0}",  0},
};

static const std::vector<std::pair<Object, Object>> UNARY_MINUS_EVAL = {
    {23,                                     -23},
    {0,                                      0},
    {Dimension(22),                          Dimension(-22)},
    {Dimension(0),                           Dimension(0)},
    {Dimension(DimensionType::Relative, 40), Dimension(DimensionType::Relative, -40)},
    {Dimension(DimensionType::Relative, 0),  Dimension(DimensionType::Relative, 0)},
};

static const std::vector<Object> UNARY_MINUS_NAN = {
    "fuzzy", false, true, Object::NULL_OBJECT()
};

TEST_F(ParseTest, UnaryMinus)
{
    for (auto& m : UNARY_MINUS_TESTS) {
        auto result = parseDataBinding(*context, m.first);
        ASSERT_TRUE(IsEqual(m.second, result)) << m.first;
    }

    context->putUserWriteable("a", 99);
    auto result = parseDataBinding(*context, "${-a}");
    ASSERT_TRUE(result.isEvaluable());

    for (auto& m : UNARY_MINUS_EVAL) {
        ASSERT_TRUE(context->userUpdateAndRecalculate("a", m.first, false));
        ASSERT_TRUE(IsEqual(m.second, result.eval())) << m.first;
    }

    for (auto& m : UNARY_MINUS_NAN) {
        ASSERT_TRUE(context->userUpdateAndRecalculate("a", m, false));
        ASSERT_TRUE(result.eval().isNaN()) << m;
    }
}

static const std::vector<std::pair<std::string, Object>> UNARY_NOT_TESTS = {
    {"${!false}", true},
    {"${!true}",  false},
    {"${!0}",     true},
    {"${!-23}",   false},
    {"${!null}",  true},
    {"${!'abc'}", false},
    {"${!''}",    true},
};

static const std::vector<std::pair<Object, Object>> UNARY_NOT_EVAL = {
    {23,                                     false},
    {0,                                      true},
    {Dimension(22),                          false},
    {Dimension(0),                           true},
    {Dimension(DimensionType::Relative, 40), false},
    {Dimension(DimensionType::Relative, 0),  true},
    {Dimension(DimensionType::Auto, 0),      false},
    {"234",                                  false},
    {"",                                     true},
    {Object::NULL_OBJECT(),                  true},
    {false,                                  true},
    {true,                                   false}
};

TEST_F(ParseTest, UnaryNot)
{
    for (auto& m : UNARY_NOT_TESTS) {
        auto result = parseDataBinding(*context, m.first);
        ASSERT_TRUE(IsEqual(m.second, result)) << m.first;
    }

    context->putUserWriteable("a", 99);
    auto result = parseDataBinding(*context, "${!a}");
    ASSERT_TRUE(result.isEvaluable());

    for (auto& m : UNARY_NOT_EVAL) {
        ASSERT_TRUE(context->userUpdateAndRecalculate("a", m.first, false));
        ASSERT_TRUE(IsEqual(m.second, result.eval())) << m.first;
    }
}


static const std::vector<std::pair<std::string, Object>> MULTIPLY_TESTS = {
    {"${2*3}",               6},
    {"${-25*2*2}",           -100},
    {"${1*2*3*4*5*6*7*8*0}", 0},
};

static const std::vector<std::string> MULTIPLY_NAN_TESTS = {
    "${2*3* null}",
    "${null * 10}",
    "${2 * 3 * true}",
    "${3 * false * true * 10}",
    "${3 * 2 * 'alpha' * 5}"
};

static const std::vector<std::vector<Object>> MULTIPLY_EVAL = {
    {23,                                     10,                                     230},
    {0,                                      23,                                     0},
    {23,                                     0,                                      0},
    {Dimension(22),                          10,                                     Dimension(220)},
    {10,                                     Dimension(22),                          Dimension(220)},
    {Dimension(22),                          0,                                      Dimension(0)},
    {0,                                      Dimension(22),                          Dimension(0)},
    {10,                                     Dimension(DimensionType::Relative, 40), Dimension(DimensionType::Relative, 400)},
    {Dimension(DimensionType::Relative, 40), 10,                                     Dimension(DimensionType::Relative, 400)},
    {0,                                      Dimension(DimensionType::Relative, 40), Dimension(DimensionType::Relative, 0)},
    {Dimension(DimensionType::Relative, 40), 0,                                      Dimension(DimensionType::Relative, 0)},
};

TEST_F(ParseTest, Multiply)
{
    for (auto& m : MULTIPLY_TESTS) {
        auto result = parseDataBinding(*context, m.first);
        ASSERT_TRUE(IsEqual(m.second, result)) << m.first;
    }

    for (auto& m : MULTIPLY_NAN_TESTS) {
        ASSERT_TRUE(parseDataBinding(*context, m).isNaN()) << m;
    }

    context->putUserWriteable("a", 99);
    context->putUserWriteable("b", 99);
    auto result = parseDataBinding(*context, "${a*b}");
    ASSERT_TRUE(result.isEvaluable());

    for (auto& m : MULTIPLY_EVAL) {
        ASSERT_TRUE(context->userUpdateAndRecalculate("a", m.at(0), false));
        ASSERT_TRUE(context->userUpdateAndRecalculate("b", m.at(1), false));
        ASSERT_TRUE(IsEqual(m.at(2), result.eval())) << m.at(1) << "*" << m.at(2);
    }
}

static const std::vector<std::pair<std::string, Object>> DIVIDE_TESTS = {
    {"${12/3}",                4},
    {"${-100/2/-2}",           25},
    {"${0/1/2/3/4/5/6/7/8/9}", 0},
};

static const std::vector<std::string> DIVIDE_NAN_TESTS = {
    "${2/3/ null}",
    "${null / 10}",
    "${2 / 3 / true}",
    "${0 / 0}",
    "${3 / false / true / 10}",
    "${3 / 2 / 'alpha' / 5}"
};

static const std::vector<std::vector<Object>> DIVIDE_EVAL = {
    {230,                                    10, 23},
    {0,                                      23, 0},
    {Dimension(220),                         10, Dimension(22)},
    {Dimension(DimensionType::Relative, 40), 10, Dimension(DimensionType::Relative, 4)},
};

static const std::vector<std::vector<Object>> DIVIDE_NAN_EVAL = {
    {220, Dimension(10)},
    {40,  Dimension(DimensionType::Relative, 10)},
};

TEST_F(ParseTest, Divide)
{
    for (auto& m : DIVIDE_TESTS) {
        auto result = parseDataBinding(*context, m.first);
        ASSERT_TRUE(IsEqual(m.second, result)) << m.first;
    }

    for (auto& m : DIVIDE_NAN_TESTS) {
        ASSERT_TRUE(parseDataBinding(*context, m).isNaN()) << m;
    }

    context->putUserWriteable("a", 99);
    context->putUserWriteable("b", 99);
    auto result = parseDataBinding(*context, "${a/b}");
    ASSERT_TRUE(result.isEvaluable());

    for (auto& m : DIVIDE_EVAL) {
        ASSERT_TRUE(context->userUpdateAndRecalculate("a", m.at(0), false));
        ASSERT_TRUE(context->userUpdateAndRecalculate("b", m.at(1), false));
        ASSERT_TRUE(IsEqual(m.at(2), result.eval())) << m.at(1) << "/" << m.at(2);
    }

    for (auto& m : DIVIDE_NAN_EVAL) {
        ASSERT_TRUE(context->userUpdateAndRecalculate("a", m.at(0), false));
        ASSERT_TRUE(context->userUpdateAndRecalculate("b", m.at(1), false));
        ASSERT_TRUE(result.eval().isNaN()) << m.at(1) << "/" << m.at(2);
    }
}


static const std::vector<std::pair<std::string, Object>> REMAINDER_TESTS = {
    {"${12%7}",                5},
    {"${-100%19%-2}",          -1},
    {"${0%1%2%3%4%5%6%7%8%9}", 0},
};

static const std::vector<std::string> REMAINDER_NAN_TESTS = {
    "${2%3% null}",
    "${null % 10}",
    "${2 % 3 % true}",
    "${0 % 0}",
    "${3 % false % true % 10}",
    "${3 % 2 % 'alpha' % 5}"
};

static const std::vector<std::vector<Object>> REMAINDER_EVAL = {
    {23,                                     10, 3},
    {0,                                      23, 0},
    {Dimension(220),                         7,  Dimension(3)},
    {Dimension(DimensionType::Relative, 40), 7,  Dimension(DimensionType::Relative, 5)},
};

static const std::vector<std::vector<Object>> REMAINDER_NAN_EVAL = {
    {220, Dimension(7)},
    {40,  Dimension(DimensionType::Relative, 7)},
};

TEST_F(ParseTest, Remainder)
{
    for (auto& m : REMAINDER_TESTS) {
        auto result = parseDataBinding(*context, m.first);
        ASSERT_TRUE(IsEqual(m.second, result)) << m.first;
    }

    for (auto& m : REMAINDER_NAN_TESTS) {
        ASSERT_TRUE(parseDataBinding(*context, m).isNaN()) << m;
    }

    context->putUserWriteable("a", 99);
    context->putUserWriteable("b", 99);
    auto result = parseDataBinding(*context, "${a%b}");
    ASSERT_TRUE(result.isEvaluable());

    for (auto& m : REMAINDER_EVAL) {
        ASSERT_TRUE(context->userUpdateAndRecalculate("a", m.at(0), false));
        ASSERT_TRUE(context->userUpdateAndRecalculate("b", m.at(1), false));
        ASSERT_TRUE(IsEqual(m.at(2), result.eval())) << m.at(0) << "%" << m.at(1);
    }

    for (auto& m : REMAINDER_NAN_EVAL) {
        ASSERT_TRUE(context->userUpdateAndRecalculate("a", m.at(0), false));
        ASSERT_TRUE(context->userUpdateAndRecalculate("b", m.at(1), false));
        ASSERT_TRUE(result.eval().isNaN()) << m.at(0) << "%" << m.at(1);
    }
}


static const std::vector<std::pair<std::string, Object>> ADD_TESTS = {
    {"${12+7}",                19},
    {"${-100+19+-2}",          -83},
    {"${0+1+2+3+4+5+6+7+8+9}", 45},
};

static const std::vector<std::pair<std::string, Object>> ADD_CONCATENTATE = {
    { "${2+null}", "2"},
    { "${2+3+ null}", "5" },
    { "${null + 10}", "10" },
    { "${2 + 3 + true}", "5true" },
    { "${3 + false + true + 10}", "3falsetrue10" },
    { "${3 + 2 + 'alpha' + 5}", "5alpha5" },
};

static const std::vector<std::vector<Object>> ADD_EVAL = {
    {23,                                     10,                                      33},
    {0,                                      23,                                      23},
    {Dimension(220),                         Dimension(-2),                           Dimension(218)},
    {Dimension(220),                         5,                                       Dimension(225)},
    {220,                                    Dimension(5),                            Dimension(225)},
    {Dimension(DimensionType::Relative, 40), Dimension(DimensionType::Relative, -20), Dimension(DimensionType::Relative,
                                                                                                20)},
    {Dimension(DimensionType::Relative, 40), 7,                                       Dimension(DimensionType::Relative,
                                                                                                47)},
    {40,                                     Dimension(DimensionType::Relative, 7),   Dimension(DimensionType::Relative,
                                                                                                47)},
    {Dimension(DimensionType::Relative, 40), Dimension(7),                            "40%7dp"},
};

TEST_F(ParseTest, Add)
{
    for (auto& m : ADD_TESTS) {
        auto result = parseDataBinding(*context, m.first);
        ASSERT_TRUE(IsEqual(m.second, result)) << m.first;
    }

    for (auto& m : ADD_CONCATENTATE) {
        ASSERT_TRUE(IsEqual(m.second, parseDataBinding(*context, m.first))) << m.first;
    }

    context->putUserWriteable("a", 99);
    context->putUserWriteable("b", 99);
    auto result = parseDataBinding(*context, "${a+b}");
    ASSERT_TRUE(result.isEvaluable());

    for (auto& m : ADD_EVAL) {
        ASSERT_TRUE(context->userUpdateAndRecalculate("a", m.at(0), false));
        ASSERT_TRUE(context->userUpdateAndRecalculate("b", m.at(1), false));
        ASSERT_TRUE(IsEqual(m.at(2), result.eval())) << m.at(1) << "+" << m.at(2);
    }
}


static const std::vector<std::pair<std::string, Object>> SUBTRACT_TESTS = {
    {"${12-7}",                5},
    {"${-100-19--2}",          -117},
    {"${0-1-2-3-4-5-6-7-8-9}", -45},
};

static const std::vector<std::string> SUBTRACT_NAN = {
     "${2-null}",
     "${2-3- null}",
     "${null - 10}",
     "${2 - 3 - true}",
    "${3 - false - true - 10}",
     "${3 - 2 - 'alpha' - 5}",
};

static const std::vector<std::vector<Object>> SUBTRACT_EVAL = {
    {23,                                     10,                                      13},
    {0,                                      23,                                      -23},
    {Dimension(220),                         Dimension(-2),                           Dimension(222)},
    {Dimension(220),                         -2,                                      Dimension(222)},
    {220,                                    Dimension(-2),                           Dimension(222)},
    {Dimension(DimensionType::Relative, 40), Dimension(DimensionType::Relative, -20), Dimension(DimensionType::Relative,
                                                                                                60)},
    {Dimension(DimensionType::Relative, 40), -20,                                     Dimension(DimensionType::Relative,
                                                                                                60)},
    {40,                                     Dimension(DimensionType::Relative, -20), Dimension(DimensionType::Relative,
                                                                                                60)},
};

static const std::vector<std::vector<Object>> SUBTRACT_NAN_EVAL = {
    {Dimension(DimensionType::Relative, 40), Dimension(-20)},
};

TEST_F(ParseTest, Subtract)
{
    for (auto& m : SUBTRACT_TESTS) {
        auto result = parseDataBinding(*context, m.first);
        ASSERT_TRUE(IsEqual(m.second, result)) << m.first;
    }

    for (auto& m : SUBTRACT_NAN) {
        ASSERT_TRUE(parseDataBinding(*context, m).isNaN()) << m;
    }

    context->putUserWriteable("a", 99);
    context->putUserWriteable("b", 99);
    auto result = parseDataBinding(*context, "${a-b}");
    ASSERT_TRUE(result.isEvaluable());

    for (auto& m : SUBTRACT_EVAL) {
        ASSERT_TRUE(context->userUpdateAndRecalculate("a", m.at(0), false));
        ASSERT_TRUE(context->userUpdateAndRecalculate("b", m.at(1), false));
        ASSERT_TRUE(IsEqual(m.at(2), result.eval())) << m.at(0) << "-" << m.at(1);
    }

    for (auto& m : SUBTRACT_NAN_EVAL) {
        ASSERT_TRUE(context->userUpdateAndRecalculate("a", m.at(0), false));
        ASSERT_TRUE(context->userUpdateAndRecalculate("b", m.at(1), false));
        ASSERT_TRUE(result.eval().isNaN()) << m.at(0) << m.at(1);
    }
}

static const std::vector<std::pair<std::string, Object>> COMPARE_TESTS = {
    {"${10<5}",  false},
    {"${10<10}", false},
    {"${10<20}", true},
    {"${'b'<'a'}", false},
    {"${'b'<'b'}", false},
    {"${'b'<'c'}", true},
    {"${10>5}",  true},
    {"${10>10}", false},
    {"${10>20}", false},
    {"${'b'>'a'}", true},
    {"${'b'>'b'}", false},
    {"${'b'>'c'}", false},
    {"${10<=5}",  false},
    {"${10<=10}", true},
    {"${10<=20}", true},
    {"${'b'<='a'}", false},
    {"${'b'<='b'}", true},
    {"${'b'<='c'}", true},
    {"${10>=5}",  true},
    {"${10>=10}", true},
    {"${10>=20}", false},
    {"${'b'>='a'}", true},
    {"${'b'>='b'}", true},
    {"${'b'>='c'}", false},
    {"${10==5}",  false},
    {"${10==10}", true},
    {"${10==20}", false},
    {"${'b'=='a'}", false},
    {"${'b'=='b'}", true},
    {"${'b'=='c'}", false},
    {"${10!=5}",  true},
    {"${10!=10}", false},
    {"${10!=20}", true},
    {"${'b'!='a'}", true},
    {"${'b'!='b'}", false},
    {"${'b'!='c'}", true},
};

static const std::vector<std::vector<Object>> COMPARE_EVAL = {
    {23,                                     10,                                     1},
    {0,                                      23,                                     -1},
    {23,                                     23,                                     0},
    {Dimension(22),                          Dimension(-2),                          1},
    {Dimension(22),                          -2,                                     1},
    {22,                                     Dimension(-2),                          1},
    {Dimension(-2),                          Dimension(22),                          -1},
    {Dimension(-2),                          22,                                     -1},
    {-2,                                     Dimension(22),                          -1},
    {Dimension(22),                          Dimension(22),                          0},
    {Dimension(22),                          22,                                     0},
    {22,                                     Dimension(22),                          0},
    {Dimension(DimensionType::Relative, 22), Dimension(DimensionType::Relative, -2), 1},
    {Dimension(DimensionType::Relative, 22), -2,                                     1},
    {22,                                     Dimension(DimensionType::Relative, -2), 1},
    {Dimension(DimensionType::Relative, -2), Dimension(DimensionType::Relative, 22), -1},
    {Dimension(DimensionType::Relative, -2), 22,                                     -1},
    {-2,                                     Dimension(DimensionType::Relative, 22), -1},
    {Dimension(DimensionType::Relative, 22), Dimension(DimensionType::Relative, 22), 0},
    {Dimension(DimensionType::Relative, 22), 22,                                     0},
    {22,                                     Dimension(DimensionType::Relative, 22), 0},
    {"abd",                                  "aab",                                  1},
    {"aab",                                  "abd",                                  -1},
    {"abd",                                  "abd",                                  0}
};

TEST_F(ParseTest, Compare)
{
    for (auto& m : COMPARE_TESTS) {
        auto result = parseDataBinding(*context, m.first);
        ASSERT_TRUE(IsEqual(m.second, result)) << m.first;
    }

    context->putUserWriteable("a", 99);
    context->putUserWriteable("b", 99);

    // Less-than
    auto result = parseDataBinding(*context, "${a<b}");
    ASSERT_TRUE(result.isEvaluable());

    for (auto& m : COMPARE_EVAL) {
        auto c = Context::createFromParent(context);
        ASSERT_TRUE(context->userUpdateAndRecalculate("a", m.at(0), false));
        ASSERT_TRUE(context->userUpdateAndRecalculate("b", m.at(1), false));
        bool target = (m.at(2).asInt() == -1);  // Must be less-than
        ASSERT_TRUE(IsEqual(target, result.eval())) << m.at(0) << "<" << m.at(1);
    }

    // Greater-than
    result = parseDataBinding(*context, "${a>b}");
    ASSERT_TRUE(result.isEvaluable());

    for (auto& m : COMPARE_EVAL) {
        auto c = Context::createFromParent(context);
        ASSERT_TRUE(context->userUpdateAndRecalculate("a", m.at(0), false));
        ASSERT_TRUE(context->userUpdateAndRecalculate("b", m.at(1), false));
        bool target = (m.at(2).asInt() == 1);  // Must be less-than
        ASSERT_TRUE(IsEqual(target, result.eval())) << m.at(0) << ">" << m.at(1);
    }

    // Less-equal
    result = parseDataBinding(*context, "${a<=b}");
    ASSERT_TRUE(result.isEvaluable());

    for (auto& m : COMPARE_EVAL) {
        auto c = Context::createFromParent(context);
        ASSERT_TRUE(context->userUpdateAndRecalculate("a", m.at(0), false));
        ASSERT_TRUE(context->userUpdateAndRecalculate("b", m.at(1), false));
        bool target = (m.at(2).asInt() != 1);  // Must be equal or less-than
        ASSERT_TRUE(IsEqual(target, result.eval())) << m.at(0) << "<=" << m.at(1);
    }

    // Greater-equal
    result = parseDataBinding(*context, "${a>=b}");
    ASSERT_TRUE(result.isEvaluable());

    for (auto& m : COMPARE_EVAL) {
        auto c = Context::createFromParent(context);
        ASSERT_TRUE(context->userUpdateAndRecalculate("a", m.at(0), false));
        ASSERT_TRUE(context->userUpdateAndRecalculate("b", m.at(1), false));
        bool target = (m.at(2).asInt() != -1);  // Must be equal or greater-than
        ASSERT_TRUE(IsEqual(target, result.eval())) << m.at(0) << ">=" << m.at(1);
    }

    // Equal
    result = parseDataBinding(*context, "${a==b}");
    ASSERT_TRUE(result.isEvaluable());

    for (auto& m : COMPARE_EVAL) {
        auto c = Context::createFromParent(context);
        ASSERT_TRUE(context->userUpdateAndRecalculate("a", m.at(0), false));
        ASSERT_TRUE(context->userUpdateAndRecalculate("b", m.at(1), false));
        bool target = (m.at(2).asInt() == 0);  // Must be equal
        ASSERT_TRUE(IsEqual(target, result.eval())) << m.at(0) << "==" << m.at(1);
    }

    // Not-Equal
    result = parseDataBinding(*context, "${a!=b}");
    ASSERT_TRUE(result.isEvaluable());

    for (auto& m : COMPARE_EVAL) {
        auto c = Context::createFromParent(context);
        ASSERT_TRUE(context->userUpdateAndRecalculate("a", m.at(0), false));
        ASSERT_TRUE(context->userUpdateAndRecalculate("b", m.at(1), false));
        bool target = (m.at(2).asInt() != 0);  // Must be equal
        ASSERT_TRUE(IsEqual(target, result.eval())) << m.at(0) << "!=" << m.at(1);
    }
}

static std::vector<std::pair<std::string, Object>> AND_OR_NULLC_TESTS = {
    {"${ 1 || false }", 1},
    {"${ 1 || true }", 1},
    {"${ 0 || false }", false},
    {"${ 0 || true }", true},
    {"${ null || false }", false},
    {"${ null || true }", true},
    {"${ 1 && false }", false},
    {"${ 1 && true }", true},
    {"${ 0 && false }", 0},
    {"${ 0 && true }", 0},
    {"${ null && false }", Object::NULL_OBJECT()},
    {"${ null && true }", Object::NULL_OBJECT()},
    {"${ 1 ?? false}", 1},
    {"${ 1 ?? true }", 1},
    {"${ 0 ?? false }", 0},
    {"${ 0 ?? true }", 0},
    {"${ null ?? false }", false},
    {"${ null ?? true }", true},
    {"${ 1 || 2 || 3 }", 1},
    {"${ 0 || 1 || 2 }", 1},
    {"${ 0 || 0 || 1 }", 1},
    {"${ 1 || (2 || 3) }", 1},
    {"${ 0 || (1 || 2) }", 1},
    {"${ 0 || (0 || 1) }", 1},
    {"${ 1 && 2 && 3 }", 3},
    {"${ 0 && 1 && 2 }", 0},
    {"${ 2 && 0 && 1 }", 0},
    {"${ 1 && (2 && 3) }", 3},
    {"${ 0 && (1 && 2) }", 0},
    {"${ 2 && (0 && 1) }", 0},
    {"${ 1 ?? 2 ?? 3 }", 1},
    {"${ null ?? 1 ?? 2 }", 1},
    {"${ null ?? null ?? 1 }", 1},
    {"${ 1 ?? (2 ?? 3) }", 1},
    {"${ null ?? (1 ?? 2) }", 1},
    {"${ null ?? (null ?? 1) }", 1},
    {"${ null ?? 4 || 5 && 0 }", 4},
    {"${ 0 || 5 && 0 ?? 17 }", 0},
};

TEST_F(ParseTest, AndOrNullC)
{
    for (const auto& m : AND_OR_NULLC_TESTS) {
        auto result = parseDataBinding(*context, m.first);
        ASSERT_TRUE(IsEqual(m.second, result)) << m.first;
    }
}

static std::vector<std::pair<std::string, Object>> TERNARY_TEST = {
    {"${ 1 ? 2 : 3 }",                             2},
    {"${ 0 ? 2 : 3 }",                             3},
    {"${ true ? true ? 1 : 2 : 3 }",               1},
    {"${ true ? false ? 1 : 2 : 3 }",              2},
    {"${ false ? true ? 1 : 2 : 3 }",              3},
    {"${ false ? false ? 1 : 2 : 3 }",             3},
    {"${ true ? 1 : true ? 2 : 3 }",               1},
    {"${ true ? 1 : false ? 2 : 3 }",              1},
    {"${ false ? 1 : true ? 2 : 3 }",              2},
    {"${ false ? 1 : false ? 2 : 3 }",             3},
    {"${ true ? true ? 1 : 2 : true ? 3 : 4 }",    1},
    {"${ true ? true ? 1 : 2 : false ? 3 : 4 }",   1},
    {"${ true ? false ? 1 : 2 : true ? 3 : 4 }",   2},
    {"${ true ? false ? 1 : 2 : false ? 3 : 4 }",  2},
    {"${ true ? true ? 1 : 2 : true ? 3 : 4 }",    1},
    {"${ true ? true ? 1 : 2 : false ? 3 : 4 }",   1},
    {"${ true ? false ? 1 : 2 : true ? 3 : 4 }",   2},
    {"${ true ? false ? 1 : 2 : false ? 3 : 4 }",  2},
    {"${ false ? true ? 1 : 2 : true ? 3 : 4 }",   3},
    {"${ false ? true ? 1 : 2 : false ? 3 : 4 }",  4},
    {"${ false ? false ? 1 : 2 : true ? 3 : 4 }",  3},
    {"${ false ? false ? 1 : 2 : false ? 3 : 4 }", 4},
    {"${ false ? true ? 1 : 2 : true ? 3 : 4 }",   3},
    {"${ false ? true ? 1 : 2 : false ? 3 : 4 }",  4},
    {"${ false ? false ? 1 : 2 : true ? 3 : 4 }",  3},
    {"${ false ? false ? 1 : 2 : false ? 3 : 4 }", 4},
};

TEST_F(ParseTest, Ternary)
{
    for (const auto& m : TERNARY_TEST) {
        auto result = parseDataBinding(*context, m.first);
        ASSERT_TRUE(IsEqual(m.second, result)) << m.first;
    }
}

static std::vector<std::pair<std::string, Object>> FIELD_ARRAY_ACCESS = {
    {"${x[1]}",          2},
    {"${y.a}",           1},
    {"${y['a']}",        1},
    {"${y.c[0]}",        5},
    {"${y['c'][0]}",     5},
    {"${x[y.b]}",        3},
    {"${x[y['b']]}",     3},
    {"${x[y.c[5-3]-6]}", 2},
};

TEST_F(ParseTest, FieldArrayAccess)
{
    auto array = JsonData("[1,2,3]");
    auto map = JsonData(R"({"a": 1, "b": 2, "c": [5,6,7]})");

    context->putConstant("x", array.get());
    context->putConstant("y", map.get());

    for (const auto& m : FIELD_ARRAY_ACCESS) {
        auto result = parseDataBinding(*context, m.first);
        ASSERT_TRUE(IsEqual(m.second, result)) << m.first;
    }
}


// TODO: Check Equal and NotEqual for boolean, color, null, auto dimension
// TODO: Check combine
// TODO: Check symbol
// TODO: Check function call
