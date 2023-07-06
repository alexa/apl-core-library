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
#include <iomanip>

#include "gtest/gtest.h"

#include "apl/engine/evaluate.h"
#include "apl/content/metrics.h"
#include "apl/engine/context.h"
#include "apl/primitives/dimension.h"
#include "apl/utils/session.h"

using namespace apl;

class ArithmeticTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        auto m = Metrics().size(2048,2048)
                      .dpi(320)
                      .theme("green")
                      .shape(apl::ROUND);
        c = Context::createTestContext(m, makeDefaultSession());
        c->putConstant("a", Dimension());
        c->putConstant("w", Dimension(0));
        c->putConstant("x", Dimension(10));
        c->putConstant("y", Dimension(20));
        c->putConstant("z", Dimension(30));
        c->putConstant("o", Dimension(DimensionType::Relative, 0));
        c->putConstant("p", Dimension(DimensionType::Relative, 10));
        c->putConstant("q", Dimension(DimensionType::Relative, 20));
        c->putConstant("r", Dimension(DimensionType::Relative, 30));
    }

    bool e(std::string value)
    {
        return evaluate(*c, std::string("${") + value + "}").asBoolean();
    }

    ContextPtr c;
};


TEST_F(ArithmeticTest, Truthy)
{
    EXPECT_TRUE(e("a"));
    EXPECT_TRUE(e("x"));
    EXPECT_TRUE(e("p"));

    EXPECT_FALSE(e("w"));
    EXPECT_FALSE(e("o"));

    EXPECT_FALSE(e("!a"));
    EXPECT_TRUE(e("!w"));
    EXPECT_FALSE(e("!x"));
    EXPECT_TRUE(e("!o"));
    EXPECT_FALSE(e("!p"));

    EXPECT_TRUE(e("!!a"));
    EXPECT_TRUE(e("!!x"));
    EXPECT_TRUE(e("!!y"));
    EXPECT_TRUE(e("!!p"));
    EXPECT_TRUE(e("!!q"));
}

TEST_F(ArithmeticTest, Compare)
{
    EXPECT_TRUE(e("x < y"));
    EXPECT_TRUE(e("y > x"));
    EXPECT_TRUE(e("x <= x"));
    EXPECT_TRUE(e("x >= x"));
    EXPECT_TRUE(e("x <= y"));
    EXPECT_TRUE(e("y >= x"));
    EXPECT_TRUE(e("y != x"));
    EXPECT_TRUE(e("!(y == x)"));
    EXPECT_TRUE(e("p < q"));
    EXPECT_TRUE(e("q > p"));
    EXPECT_TRUE(e("p <= q"));
    EXPECT_TRUE(e("q >= p"));
    EXPECT_TRUE(e("q <= q"));
    EXPECT_TRUE(e("q >= q"));
    EXPECT_TRUE(e("p != q"));
    EXPECT_TRUE(e("!(p == q)"));
    EXPECT_TRUE(e("x != p"));
    EXPECT_TRUE(e("y != q"));
    EXPECT_TRUE(e("a != x"));
    EXPECT_TRUE(e("a != p"));
    EXPECT_TRUE(e("a == a"));
    EXPECT_TRUE(e("x == x"));
    EXPECT_TRUE(e("p == p"));
}

TEST_F(ArithmeticTest, Add)
{
    EXPECT_TRUE(e("x == w + x"));
    EXPECT_TRUE(e("z == x + y"));
    EXPECT_TRUE(e("x + w == x"));
    EXPECT_TRUE(e("o + p == p"));
    EXPECT_TRUE(e("o + o == o"));
    EXPECT_TRUE(e("p + q == r"));

    EXPECT_FALSE(e("o + x == x"));
    EXPECT_FALSE(e("a + x == x"));
}

TEST_F(ArithmeticTest, Subtract)
{
    EXPECT_TRUE(e("x == x - w"));
    EXPECT_TRUE(e("z - x == y"));
    EXPECT_TRUE(e("z - z == w"));
    EXPECT_TRUE(e("p - o == p"));
    EXPECT_TRUE(e("r - p == q"));
    EXPECT_TRUE(e("p - p == o"));

    EXPECT_FALSE(e("x - o == x"));
}

TEST_F(ArithmeticTest, Multiply)
{
    EXPECT_TRUE(e("y == 2 * x"));
    EXPECT_TRUE(e("y == x * 2"));
    EXPECT_TRUE(e("q == p * 2"));
    EXPECT_TRUE(e("q == 2 * p"));

    // Can't multiply dimensions
    EXPECT_FALSE(e("w == w * x"));
    EXPECT_FALSE(e("o == o * p"));

    EXPECT_TRUE(e("x == y / 2"));
    EXPECT_TRUE(e("p == q / 2"));

    // Can't divide by a dimension
    EXPECT_FALSE(e("x == 100 / x"));
    EXPECT_FALSE(e("p == 100 / p"));
}