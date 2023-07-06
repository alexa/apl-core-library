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
#include "apl/datagrammar/bytecodeassembler.h"

using namespace apl;

class DecompileTest : public ::testing::Test {
public:
    DecompileTest() {
        context = Context::createTestContext(Metrics(), makeDefaultSession());
        context->putConstant("FixedArray", ObjectArray{10,20,30});
        context->putUserWriteable("TestArray", testArray);
        context->putUserWriteable("TestMap", testMap);
    }

    ContextPtr context;
    ObjectArrayPtr testArray = std::make_shared<ObjectArray>(ObjectArray{1,2,3});
    ObjectMapPtr testMap = std::make_shared<ObjectMap>(ObjectMap{{"a", 1}, {"b", 2}});
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
    auto it = bc->disassemble().begin();
    if (*it != "DATA")
        return ::testing::AssertionFailure() << "Missing DATA";

    ++it;
    size_t index = 0;
    bool found_instructions = false;

    while (it != bc->disassemble().end()) {
        if (*it == "INSTRUCTIONS") {
            if (found_instructions)
                return ::testing::AssertionFailure() << "Double INSTRUCTIONS!";

            found_instructions = true;
        }
        else {
            if (index >= lines.size())
                return ::testing::AssertionFailure() << "Out of bounds, index=" << index;
            auto expected = splitStringWS(lines.at(index));
            auto actual = splitStringWS(*it);
            auto result = IsEqual(expected, actual);
            if (!result)
                return result << " actual='" << *it << "' index=" << index;
            ++index;
        }
        ++it;
    }

    if (!found_instructions)
        return ::testing::AssertionFailure() << "Missing INSTRUCTIONS";

    return ::testing::AssertionSuccess();
}

struct DecompileTestCase {
    std::string expression;
    std::vector<std::string> instructions;
};

static const auto DECOMPILE_TEST_CASES = std::vector<DecompileTestCase>{
    {"${}", {"0 LOAD_CONSTANT (3) empty_string"}},
    {"${3}", {"0 LOAD_IMMEDIATE (3) "}},
    {"${'foo'}", {"0 'foo'", "0 LOAD_DATA (0) ['foo'] "}},
    {"${1 < 2}", {"0 LOAD_IMMEDIATE (1)", "1 LOAD_IMMEDIATE (2)", "2 COMPARE_OP (0) <"}},
    {"${true ? 2 : 3}",
     {"0 LOAD_CONSTANT (2) true", "1 POP_JUMP_IF_FALSE (2) GOTO 4", "2 LOAD_IMMEDIATE (2)",
      "3 JUMP (1) GOTO 5", "4 LOAD_IMMEDIATE (3)"}},
    {"${Math.min(1,2)}",
     {"0 BuiltInMap<>", "1 'min'", "0 LOAD_DATA (0) [BuiltInMap<>]",
      "1 ATTRIBUTE_ACCESS (1) ['min']", "2 LOAD_IMMEDIATE (1)", "3 LOAD_IMMEDIATE (2)",
      "4 CALL_FUNCTION (2) argument_count=2"}},
    {"${FixedArray[2]}",
     {"0 BuiltInArray<>", "0 LOAD_DATA (0) [BuildInArray<>]", "1 LOAD_IMMEDIATE (2)",
      "2 ARRAY_ACCESS (0)"}},
    {"${TestArray[2]}",
     {"0 BoundSymbol<TestArray>", "0 LOAD_BOUND_SYMBOL (0) [BoundSymbol<TestArray>]",
      "1 LOAD_IMMEDIATE (2)", "2 ARRAY_ACCESS (0)"}},
    {"${TestMap['a']}",
     {"0 BoundSymbol<TestMap>", "1 'a'", "0 LOAD_BOUND_SYMBOL (0) [BoundSymbol<TestMap>]",
      "1 LOAD_DATA (1) ['a']", "2 ARRAY_ACCESS (0)"}},
};

TEST_F(DecompileTest, Basic)
{
    for (const auto& m : DECOMPILE_TEST_CASES) {
        auto v = datagrammar::ByteCodeAssembler::parse(*context, m.expression);
        ASSERT_TRUE(v.isEvaluable());
        auto bc = v.get<datagrammar::ByteCode>();
        ASSERT_TRUE(CheckByteCode(m.instructions, bc)) << "Test case '" << m.expression << "'";
    }
}

/*
 * Ensure iterator-related methods work.
 */
TEST_F(DecompileTest, Iterator)
{
    auto result = parseAndEvaluate(*context, "${TestArray[0]}");
    ASSERT_TRUE(IsEqual(result.value, 1));
    ASSERT_EQ(1, result.symbols.size());
    ASSERT_TRUE(result.expression.is<datagrammar::ByteCode>());
    auto disassembly = result.expression.get<datagrammar::ByteCode>()->disassemble();
    ASSERT_FALSE(disassembly.begin() == disassembly.end());
    ASSERT_TRUE(disassembly.begin() != disassembly.end());
    // See the example from the Basic test for the expected disassembly values
    ASSERT_EQ(std::distance(disassembly.begin(), disassembly.end()), 6);
}