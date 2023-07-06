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
#include "apl/datagrammar/bytecode.h"
#include "apl/primitives/boundsymbolset.h"

using namespace apl;

class OptimizeTest : public ::testing::Test {
public:
    OptimizeTest() {
        Metrics m;
        context = Context::createTestContext(m, makeDefaultSession());
    }

    ContextPtr context;
};

static std::vector<std::pair<std::string, Object>> BASIC = {
    {"${1+2+a}", 4},
    {"${a || b}", 1},
    {"${false || a}", 1},
    {"${b || 100 || a}", 100},
    {"${a && b}", 0},
    {"${c[0]}", 1},
    {"${d.y}", "foobar"},
    {"${d.x}", 1},
    {"${c[0] - d.x}", 0},
    {"${c[0] - d.x ? d['y'] : d['z'][0]}", -1},
    {"${Math.min( a, b, c.length, d.x, d.z[0] ) }", -1},
    {"${Math.max( a , b , c.length , d.x , d.z[3-3] ) }", 3},
    {"${+2+a}", 3},
    {"${!(a<b) ? 10 : 11}", 10},
    {"${[a][3] ?? {'a':b}['c'] ?? 13}", 13},
    {"_${a}_${'#${2}#'}", "_1_#2#"},
};


TEST_F(OptimizeTest, Basic)
{
    context->putUserWriteable("a", 1);
    context->putUserWriteable("b", 0);

    auto array = JsonData("[1,2,3]");
    ASSERT_TRUE(array);
    context->putUserWriteable("c", array.get());

    auto map = JsonData(R"({"x": 1, "y": "foobar", "z": [-1, 0, false]})");
    ASSERT_TRUE(map);
    context->putUserWriteable("d", map.get());

    for (const auto& m : BASIC) {
        auto result = parseAndEvaluate(*context, m.first, false);
        ASSERT_TRUE(IsEqual(m.second, result.value)) << m.first;
        ASSERT_TRUE(result.expression.isEvaluable()) << m.first;
        ASSERT_FALSE(result.expression.get<datagrammar::ByteCode>()->isOptimized()) << m.first;

        result = parseAndEvaluate(*context, m.first, true);
        ASSERT_TRUE(IsEqual(m.second, result.value)) << m.first;
        ASSERT_TRUE(result.expression.isEvaluable()) << m.first;
        ASSERT_TRUE(result.expression.get<datagrammar::ByteCode>()->isOptimized()) << m.first;
    }
}

static std::vector<std::pair<std::string, Object>> MERGE_STRINGS = {
    {"This value is ${23}", "This value is 23"},
    {"${1+1} is the value", "2 is the value"},
    {"Where are ${1+1} tigers?", "Where are 2 tigers?"},
    {"A ${null ?? 'friendly'} tiger is not ${3-1} easy ${4/2} find", "A friendly tiger is not 2 easy 2 find"}
};

TEST_F(OptimizeTest, MergeStrings)
{
    context->putUserWriteable("a", 23);

    for (const auto& m : MERGE_STRINGS) {
        auto result = parseAndEvaluate(*context, m.first, true);
        ASSERT_TRUE(IsEqual(m.second, result.value)) << m.first;
        ASSERT_EQ(result.symbols.size(), 0) << m.first;
        ASSERT_TRUE(result.expression.isEvaluable()) << m.first;
    }
}


TEST_F(OptimizeTest, DeadCodeRemoval)
{
    context->putUserWriteable("a", 23);
    auto result = parseAndEvaluate(*context, "${a?(1!=2? 10:3):4}");
    ASSERT_TRUE(IsEqual(result.value, 10));
    ASSERT_TRUE(result.expression.is<datagrammar::ByteCode>());
    ASSERT_TRUE(IsEqual(10, result.expression.eval()));

    context->userUpdateAndRecalculate("a", 0, false);
    ASSERT_TRUE(IsEqual(4, result.expression.eval()));

    context->userUpdateAndRecalculate("a", 23, false);
    ASSERT_TRUE(IsEqual(10, result.expression.eval()));
}

TEST_F(OptimizeTest, RemoveDuplicateOperands)
{
    context->putUserWriteable("a", 10);

    BoundSymbolSet expected;
    expected.emplace({context, "a"});

    // An un-optimized expression has duplicated references
    auto result = parseAndEvaluate(*context, "${a+a+a}", false);
    ASSERT_TRUE(IsEqual(30, result.value));       // Correct final value
    ASSERT_EQ(result.symbols, expected);  // Correct set of symbols
    ASSERT_TRUE(result.expression.is<datagrammar::ByteCode>());
    auto bc = result.expression.get<datagrammar::ByteCode>();
    ASSERT_EQ(3, bc->dataCount());   // But three copies of the "a" symbol
    for (int i = 0 ; i < 3 ; i++)
        ASSERT_TRUE(IsEqual(BoundSymbol{context, "a"}, bc->dataAt(i))) << i;

    // An optimized expression has no duplicated references
    result = parseAndEvaluate(*context, "${a+a+a}", true);
    ASSERT_TRUE(IsEqual(30, result.value));       // Correct final value
    ASSERT_EQ(result.symbols, expected);  // Correct set of symbols
    ASSERT_TRUE(result.expression.is<datagrammar::ByteCode>());
    bc = result.expression.get<datagrammar::ByteCode>();
    ASSERT_EQ(1, bc->dataCount());   // Just a single copy
    ASSERT_TRUE(IsEqual(BoundSymbol{context, "a"}, bc->dataAt(0)));
}

TEST_F(OptimizeTest, RemoveDuplicateOperands2)
{
    context->putUserWriteable("a", 10);
    context->putUserWriteable("b", 7);

    BoundSymbolSet expected;
    expected.emplace({context, "a"});
    expected.emplace({context, "b"});

    // An un-optimized expression has duplicated references
    auto result = parseAndEvaluate(*context, "${b+a+b+a}", false);
    ASSERT_TRUE(IsEqual(34, result.value));       // Correct final value
    ASSERT_EQ(result.symbols, expected);  // Correct set of symbols
    ASSERT_TRUE(result.expression.is<datagrammar::ByteCode>());
    auto bc = result.expression.get<datagrammar::ByteCode>();
    ASSERT_EQ(4, bc->dataCount());   // But four copies of the symbols
    for (int i = 0 ; i < 4 ; i++)
        ASSERT_TRUE(IsEqual(BoundSymbol{context, i % 2 == 0 ? "b" : "a"}, bc->dataAt(i))) << i;

    // An optimized expression has no duplicated references
    result = parseAndEvaluate(*context, "${b+a+b+a}", true);
    ASSERT_TRUE(IsEqual(34, result.value));       // Correct final value
    ASSERT_EQ(result.symbols, expected);  // Correct set of symbols
    ASSERT_TRUE(result.expression.is<datagrammar::ByteCode>());
    bc = result.expression.get<datagrammar::ByteCode>();
    ASSERT_EQ(2, bc->dataCount());   // Just two symbols
    ASSERT_TRUE(IsEqual(BoundSymbol{context, "b"}, bc->dataAt(0)));
    ASSERT_TRUE(IsEqual(BoundSymbol{context, "a"}, bc->dataAt(1)));
}

TEST_F(OptimizeTest, ShrinkCode)
{
    context->putUserWriteable("a", 10);

    auto result = parseAndEvaluate(*context, "${false ? a : 10}", false);
    ASSERT_TRUE(IsEqual(10, result.value));
    ASSERT_EQ(0, result.symbols.size());
    ASSERT_TRUE(result.expression.is<datagrammar::ByteCode>());
    auto bc = result.expression.get<datagrammar::ByteCode>();
    ASSERT_EQ(1, bc->dataCount());  // There's a reference to "a" in the byte code
    auto unoptimized_length = bc->instructionCount();

    result = parseAndEvaluate(*context, "${false ? a : 10}", true);
    ASSERT_TRUE(IsEqual(10, result.value));
    ASSERT_EQ(0, result.symbols.size());
    ASSERT_TRUE(result.expression.is<datagrammar::ByteCode>());
    bc = result.expression.get<datagrammar::ByteCode>();
    ASSERT_EQ(0, bc->dataCount());
    auto optimized_length = bc->instructionCount();

    ASSERT_TRUE( optimized_length < unoptimized_length );
}