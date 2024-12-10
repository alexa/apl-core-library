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

#include "apl/primitives/transformgrammar.h"

using namespace apl;

class TransformGrammarTest : public MemoryWrapper {};

struct TransformGrammarTestCase {
    std::string data;
    std::vector<Transform2D> transforms;
};

static const std::vector<TransformGrammarTestCase> TEST_CASES = {
    {R"(rotate(45))", {Transform2D::rotate(45)}},
    {R"(rotate(45 10 15))", {Transform2D::translate(10, 15), Transform2D::rotate(45), Transform2D::translate(-10, -15)}},
    {R"(scale(2 3))", {Transform2D::scale(2, 3)}},
    {R"(scale(2))", {Transform2D::scale(2)}},
    {R"(translate(2))", {Transform2D::translate(2, 0)}},
    {R"(translate(2 2))", {Transform2D::translate(2, 2)}},
    {R"(skewX(5))", {Transform2D::skewX(5)}},
    {R"(skewY(5))", {Transform2D::skewY(5)}},
    {R"(skewY(5) scale(2 3) translate(2 2))", {Transform2D::skewY(5), Transform2D::scale(2, 3), Transform2D::translate(2, 2)}},
};

class TestAccumulator : public TransformationAccumulator {
public:
    void add(const apl::TransformPtr &transform) override {
        transforms.push_back(transform->evaluate(0, 0));
    }

    std::vector<Transform2D> transforms;
};

TEST_F(TransformGrammarTest, ManyTestCases)
{
    for (auto& test : TEST_CASES) {
        TestAccumulator accum;

        ASSERT_TRUE(t2grammar::parse(session, test.data, accum));
        ASSERT_EQ(test.transforms, accum.transforms);
    }
}

TEST_F(TransformGrammarTest, FailToParse)
{
    TestAccumulator accum;

    ASSERT_FALSE(t2grammar::parse(session, "translate(2 x 2)", accum));
    ASSERT_TRUE(session->checkAndClear());
}