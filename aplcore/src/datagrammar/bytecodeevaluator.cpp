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

#include "apl/datagrammar/bytecodeevaluator.h"

#include "apl/datagrammar/boundsymbol.h"
#include "apl/engine/context.h"
#include "apl/datagrammar/functions.h"
#include "apl/utils/session.h"

namespace apl {
namespace datagrammar {

static const bool DEBUG_BYTE_CODE = false;

ByteCodeEvaluator::ByteCodeEvaluator(const ByteCode& byteCode)
    : mByteCode(byteCode)
{
}

static std::string
stackToString(const std::vector<Object>& stack)
{
    std::string result;
    for (const auto& m : stack) {
        result += m.toDebugString() + " ";
    }
    return result.substr(0, result.size() - 1);
}

void
ByteCodeEvaluator::advance()
{
    assert(mState == INIT);

    const auto &instructions = mByteCode.mInstructions;
    const auto &data = mByteCode.mData;
    auto number_of_commands = instructions.size();

    auto pop = [&]() -> Object {
        auto result = mStack.back();
        mStack.pop_back();
        return result;
    };

    // For now, we'll consider a program done when it runs off the end of the code
    for (; mProgramCounter < number_of_commands; mProgramCounter++) {
        const auto &cmd = instructions.at(mProgramCounter);
        LOG_IF(DEBUG_BYTE_CODE) << mByteCode.instructionAsString(mProgramCounter)
                                << " stack={" << stackToString(mStack) << "}";
        switch (cmd.type) {
            case BC_OPCODE_NOP:
                break;

            case BC_OPCODE_CALL_FUNCTION: {
                auto argCount = cmd.value;
                std::vector<Object> args(argCount);  // Reserve enough space
                while (argCount > 0)
                    args[--argCount] = pop();
                auto f = pop();
                if (f.isCallable()) {  // Normal function or Easing function
                    if (!f.isPure())
                        mIsConstant = false;
                    mStack.emplace_back(f.call(args));
                } else {
                    CONSOLE_CTP(mByteCode.getContext()) << "Invalid function pc=" << mProgramCounter;
                    mStack.emplace_back(Object::NULL_OBJECT());
                }
            }
                break;

            case BC_OPCODE_LOAD_CONSTANT:
                mStack.emplace_back(getConstant(static_cast<ByteCodeConstant>(cmd.value)));
                break;

            case BC_OPCODE_LOAD_IMMEDIATE:
                mStack.emplace_back(cmd.value);
                break;

            case BC_OPCODE_LOAD_DATA:
                mStack.emplace_back(data.at(cmd.value));
                break;

            case BC_OPCODE_LOAD_BOUND_SYMBOL:
                mStack.emplace_back(data.at(cmd.value).eval());
                mIsConstant = false;
                break;

            case BC_OPCODE_ATTRIBUTE_ACCESS:
                mStack.emplace_back(CalcFieldAccess(pop(), data.at(cmd.value)));
                break;

            case BC_OPCODE_ARRAY_ACCESS: {
                auto b = pop();
                auto a = pop();
                mStack.emplace_back(CalcArrayAccess(a, b));
            }
                break;

            case BC_OPCODE_UNARY_PLUS:
                mStack.emplace_back(CalculateUnaryPlus(pop()));
                break;

            case BC_OPCODE_UNARY_MINUS:
                mStack.emplace_back(CalculateUnaryMinus(pop()));
                break;

            case BC_OPCODE_UNARY_NOT:
                mStack.emplace_back(CalculateUnaryNot(pop()));
                break;

            case BC_OPCODE_BINARY_MULTIPLY: {
                auto b = pop();
                auto a = pop();
                mStack.emplace_back(CalculateMultiply(a, b));
            }
                break;

            case BC_OPCODE_BINARY_DIVIDE: {
                auto b = pop();
                auto a = pop();
                mStack.emplace_back(CalculateDivide(a, b));
            }
                break;

            case BC_OPCODE_BINARY_REMAINDER: {
                auto b = pop();
                auto a = pop();
                mStack.emplace_back(CalculateRemainder(a, b));
            }
                break;

            case BC_OPCODE_BINARY_ADD: {
                auto b = pop();
                auto a = pop();
                mStack.emplace_back(CalculateAdd(a, b));
            }
                break;

            case BC_OPCODE_BINARY_SUBTRACT: {
                auto b = pop();
                auto a = pop();
                mStack.emplace_back(CalculateSubtract(a, b));
            }
                break;

            case BC_OPCODE_COMPARE_OP: {
                auto b = pop();
                auto a = pop();
                mStack.emplace_back(CompareOp(static_cast<ByteCodeComparison>(cmd.value), a, b)
                                        ? Object::TRUE_OBJECT() : Object::FALSE_OBJECT());
            }
                break;

            case BC_OPCODE_JUMP:
                assert(cmd.value != -1);
                mProgramCounter += cmd.value;
                break;

            case BC_OPCODE_JUMP_IF_FALSE_OR_POP:
                assert(cmd.value != -1);
                if (!mStack.back().truthy())
                    mProgramCounter += cmd.value;
                else
                    mStack.pop_back();
                break;

            case BC_OPCODE_JUMP_IF_TRUE_OR_POP:
                assert(cmd.value != -1);
                if (mStack.back().truthy())
                    mProgramCounter += cmd.value;
                else
                    mStack.pop_back();
                break;

            case BC_OPCODE_JUMP_IF_NOT_NULL_OR_POP:
                assert(cmd.value != -1);
                if (!mStack.back().isNull())
                    mProgramCounter += cmd.value;
                else
                    mStack.pop_back();
                break;

            case BC_OPCODE_POP_JUMP_IF_FALSE:
                assert(cmd.value != -1);
                if (!pop().truthy())
                    mProgramCounter += cmd.value;
                break;

            case BC_OPCODE_MERGE_STRING: {
                auto result = pop();
                for (int i = 1; i < cmd.value; i++)
                    result = MergeOp(pop(), result);
                mStack.emplace_back(std::move(result));
            }
                break;

            case BC_OPCODE_APPEND_ARRAY: {
                auto b = pop();
                auto a = pop();
                assert(a.isArray());
                a.getMutableArray().push_back(std::move(b));
                mStack.emplace_back(std::move(a));
            }
                break;

            case BC_OPCODE_APPEND_MAP: {
                auto c = pop();
                auto b = pop();
                auto a = pop();
                assert(a.isMap());
                a.getMutableMap().emplace(b.asString(), std::move(c));
                mStack.emplace_back(std::move(a));
            }
                break;
        }
    }

    // If we get to this point, the program has finished executing.
    mState = DONE;
}

Object
ByteCodeEvaluator::getResult() const
{
    assert(mState == DONE);

    auto len = mStack.size();
    if (len == 0)
        return Object::NULL_OBJECT();

    if (len > 1)
        LOG(LogLevel::kError) << "Expected no items on stack; found " << len << " instead";

    return mStack.back();
}


} // namespace datagramamr
} // namespace apl