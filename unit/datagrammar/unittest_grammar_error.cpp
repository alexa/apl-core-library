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

#include <apl/datagrammar/grammarerror.h>

#include "../testeventloop.h"

using namespace apl;

class GrammarErrorTest : public ::testing::Test {
public:
    GrammarErrorTest() {
        session = std::make_shared<TestSession>();
        context = Context::createTestContext(Metrics(), session);
    }

    bool ConsoleMessage(const std::string& msg) {
        return session->checkAndClear(msg);
    }

    Object eval(const std::string& expression) {
        auto result = parseDataBinding(*context, expression);
        return result;
    }

    ContextPtr context;
    std::shared_ptr<TestSession> session;
};

auto TEST_CASES = std::vector<std::pair<std::string, GrammarError>>{
    {"${02}",               INVALID_NUMBER_FORMAT},
    {"${1.1.1}",            UNEXPECTED_TOKEN},
    {"${ ] }",              UNEXPECTED_TOKEN},
    {"${ Math.random( }",   EXPECTED_POSTFIX_RIGHT_PAREN},
    {"${1.1.}",             UNEXPECTED_TOKEN},
    {"${23*}",              EXPECTED_OPERAND_AFTER_MULTIPLICATIVE},
    {"${23+}",              EXPECTED_OPERAND_AFTER_ADDITIVE},
    {"${23 <=}",            EXPECTED_OPERAND_AFTER_COMPARISON},
    {"${23 !=}",            EXPECTED_OPERAND_AFTER_EQUALITY},
    {"${23 &&}",            EXPECTED_OPERAND_AFTER_LOGICAL_AND},
    {"${23 ||}",            EXPECTED_OPERAND_AFTER_LOGICAL_OR},
    {"${23??}",             EXPECTED_OPERAND_AFTER_NULLC},
    {"${ true ? }",         MALFORMED_TERNARY_EXPRESSION},
    {"${ true ? false }",   MALFORMED_TERNARY_EXPRESSION},
    {"${ true ? false : }", MALFORMED_TERNARY_EXPRESSION},
    {"${ true ? : }",       MALFORMED_TERNARY_EXPRESSION},
    {"${ [1,] }",           EXPECTED_EXPRESSION},
    {"${ [,] }",            MALFORMED_ARRAY},
    {"${ [ }",              MALFORMED_ARRAY},
    {"${'  }",              UNTERMINATED_SS_STRING},
    {"${\" }",              UNTERMINATED_DS_STRING},
    {"${'  ${ '}",          UNTERMINATED_SS_STRING},
    {"${\"${\"}",           UNTERMINATED_DS_STRING},
    {"${'${\"}'}",          UNTERMINATED_DS_STRING},
    {"${ {'foo'} ",         EXPECTED_MAP_VALUE_ASSIGNMENT},
    {"${ {'foo': } ",       EXPECTED_EXPRESSION},
    {"${ {'foo': 2, } ",    EXPECTED_MAP_ASSIGNMENT},
    {"${ { }",              UNEXPECTED_TOKEN},
    {"${ {,} }",            MALFORMED_MAP},
    {"${ {x:2} }",          MALFORMED_MAP},
    {"${ x[ }",             UNEXPECTED_TOKEN},
    {"${ x[] }",            UNEXPECTED_TOKEN},
    {"${ x. }",             UNEXPECTED_TOKEN}
};

TEST_F(GrammarErrorTest, Tests) {
    for (const auto& m : TEST_CASES) {
        auto expected = errorToString(m.second);
        ASSERT_TRUE(IsEqual(m.first, eval(m.first)));  // Failed evaluation returns the original string
        ASSERT_TRUE(ConsoleMessage(expected)) << m.first << ":" << expected;
    }
}