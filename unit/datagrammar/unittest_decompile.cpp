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

using namespace apl;

class DecompileTest : public ::testing::Test {
public:
    DecompileTest() {
        context = Context::createTestContext(Metrics(), makeDefaultSession());
    }

    ContextPtr context;
};

// Static method for splitting strings on whitespace
static std::vector<std::string>
splitStringWS(const std::string& text, int maxCount=3)
{
    std::vector<std::string> lines;
    auto it = text.begin();
    while (it != text.end()) {
        it = std::find_if_not(it, text.end(), isspace);
        if (it != text.end()) {
            auto it2 = std::find_if(it, text.end(), isspace);
            lines.emplace_back(std::string(it, it2));
            it = it2;
            if (lines.size() == maxCount)
                return lines;
        }
    }

    return lines;
}

::testing::AssertionResult
CheckByteCode(const std::vector<std::string>& lines, const std::shared_ptr<datagrammar::ByteCode>& bc)
{
    if (lines.size() != bc->instructionCount())
        return ::testing::AssertionFailure() << "Mismatched line count expected=" << lines.size()
                                             << " actual=" << bc->instructionCount();

    for (int lineNumber = 0 ; lineNumber < lines.size() ; lineNumber++) {
        auto expected = splitStringWS(lines.at(lineNumber));
        auto actual = splitStringWS(bc->instructionAsString(lineNumber));
        auto result = IsEqual(expected, actual);
        if (!result)
            return result;
    }

    return ::testing::AssertionSuccess();
}

struct DecompileTestCase {
    std::string expression;
    std::vector<std::string> instructions;
};

static const auto DECOMPILE_TEST_CASES = std::vector<DecompileTestCase>{
    {"${}", {"0 LOAD_CONSTANT (3) empty_string"}},
    {"${3}", {"0 LOAD_IMMEDIATE (3) "}},
    {"${'foo'}", {"0 LOAD_DATA (0) ['foo'] "}},
    {"${1 < 2}", {"0 LOAD_IMMEDIATE (1)", "1 LOAD_IMMEDIATE (2)", "2 COMPARE_OP (0) <"}},
    {"${true ? 2 : 3}",
     {"0 LOAD_CONSTANT (2) true", "1 POP_JUMP_IF_FALSE (2) GOTO 4", "2 LOAD_IMMEDIATE (2)",
      "3 JUMP (1) GOTO 5", "4 LOAD_IMMEDIATE (3)"}},
    {"${Math.min(1,2)}",
     {"0 LOAD_DATA (0) ['Math']", "1 ATTRIBUTE_ACCESS (1) ['min']", "2 LOAD_IMMEDIATE (1)",
      "3 LOAD_IMMEDIATE (2)", "4 CALL_FUNCTION (2) argument_count=2"}}
};

TEST_F(DecompileTest, Basic)
{
    for (const auto& m : DECOMPILE_TEST_CASES) {
        auto v = getDataBinding(*context, m.expression);
        ASSERT_TRUE(v.isEvaluable());
        auto bc = v.getByteCode();
        ASSERT_TRUE(CheckByteCode(m.instructions, bc)) << "Test case '" << m.expression << "'";
    }
}