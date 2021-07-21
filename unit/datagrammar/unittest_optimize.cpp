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
#include "apl/primitives/symbolreferencemap.h"

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
        auto result = parseDataBinding(*context, m.first);
        ASSERT_TRUE(result.isEvaluable());
        ASSERT_TRUE(IsEqual(m.second, result.eval())) << m.first;
        ASSERT_FALSE(result.getByteCode()->isOptimized());

        SymbolReferenceMap symbols;
        result.symbols(symbols);
        ASSERT_TRUE(IsEqual(m.second, result.eval())) << m.first;
        ASSERT_TRUE(result.getByteCode()->isOptimized());
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
        auto result = parseDataBinding(*context, m.first);
        ASSERT_FALSE(result.isEvaluable());
        ASSERT_TRUE(IsEqual(m.second, result)) << m.first;
    }
}


TEST_F(OptimizeTest, DeadCodeRemoval)
{
    context->putUserWriteable("a", 23);
    auto result = parseDataBinding(*context, "${a?(1!=2? 10:3):4}");
    ASSERT_TRUE(result.isByteCode());
    ASSERT_TRUE(IsEqual(10, result.eval()));

    context->userUpdateAndRecalculate("a", 0, false);
    ASSERT_TRUE(IsEqual(4, result.eval()));

    // Now optimize
    SymbolReferenceMap symbols;
    result.symbols(symbols);
    ASSERT_TRUE(IsEqual(4, result.eval()));

    context->userUpdateAndRecalculate("a", 23, false);
    ASSERT_TRUE(IsEqual(10, result.eval()));
}