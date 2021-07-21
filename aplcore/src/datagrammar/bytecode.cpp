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

#include "apl/datagrammar/bytecode.h"
#include "apl/datagrammar/bytecodeoptimizer.h"
#include "apl/datagrammar/bytecodeevaluator.h"
#include "apl/datagrammar/boundsymbol.h"
#include "apl/engine/context.h"
#include "apl/utils/session.h"

namespace apl {
namespace datagrammar {

Object
ByteCode::eval() const
{
    // Check for a trivial instruction
    if (mInstructions.size() == 1) {
        const auto& cmd = mInstructions.at(0);
        switch (cmd.type) {
            case BC_OPCODE_LOAD_BOUND_SYMBOL:
                return mData.at(cmd.value).eval();

            case BC_OPCODE_LOAD_DATA:
                return mData.at(cmd.value);

            case BC_OPCODE_LOAD_CONSTANT:
                return getConstant(static_cast<ByteCodeConstant>(cmd.value));

            case BC_OPCODE_LOAD_IMMEDIATE:
                return cmd.value;

            default:
                CONSOLE_CTP(mContext) << "Unexpected trivial instruction " << cmd.type;
                return Object::NULL_OBJECT();
        }
    }

    ByteCodeEvaluator evaluator(*this);
    evaluator.advance();

    if (evaluator.isDone())
        return evaluator.getResult();

    CONSOLE_CTP(mContext) << "Unable to evaluate byte code data";
    return Object::NULL_OBJECT();
}

Object
ByteCode::simplify()
{
    ByteCodeEvaluator evaluator(*this);
    evaluator.advance();
    if (evaluator.isDone() && evaluator.isConstant())
        return evaluator.getResult();

    return shared_from_this();
}

void
ByteCode::symbols(SymbolReferenceMap& symbols)
{
    auto context = mContext.lock();
    if (!context)
        return;

    if (!mOptimized) {
        ByteCodeOptimizer::optimize(*this);
        mOptimized = true;
    }

    // Find all symbol references by searching the opcodes.
    SymbolReference ref;
    Object operand;

    for (auto &cmd : mInstructions) {
        switch (cmd.type) {
            case BC_OPCODE_LOAD_DATA:
                operand = mData[cmd.value].asString();
                break;

            case BC_OPCODE_LOAD_IMMEDIATE:
                operand = cmd.value;
                break;

            case BC_OPCODE_LOAD_BOUND_SYMBOL:
                // Store the old symbol
                if (!ref.first.empty())
                    symbols.emplace(ref);

                ref = mData[cmd.value].getBoundSymbol()->getSymbol();
                operand = Object::NULL_OBJECT();
                break;

            case BC_OPCODE_ATTRIBUTE_ACCESS:
                if (!ref.first.empty())
                    ref.first += mData[cmd.value].asString() + "/";
                operand = Object::NULL_OBJECT();
                break;

            case BC_OPCODE_ARRAY_ACCESS:
                if (!ref.first.empty()) {
                    if (operand.isString() || operand.isNumber())
                        ref.first += operand.asString() + "/";
                    else {
                        symbols.emplace(ref);
                        ref.first.clear();
                    }
                }
                operand = Object::NULL_OBJECT();
                break;

            default:
                if (!ref.first.empty()) {
                    symbols.emplace(ref);
                    ref.first.clear();
                }
                operand = Object::NULL_OBJECT();
                break;
        }
    }

    if (!ref.first.empty())
        symbols.emplace(ref);
}

ContextPtr
ByteCode::getContext() const
{
    return mContext.lock();
}

void
ByteCode::dump() const
{
    LOG(LogLevel::kDebug) << "Data";
    for (int i = 0; i < mData.size(); i++)
        LOG(LogLevel::kDebug) << "  [" << i << "] " << mData.at(i).toDebugString();

    LOG(LogLevel::kDebug) << "Instructions";
    for (int pc = 0; pc < mInstructions.size(); pc++)
        LOG(LogLevel::kDebug) << instructionAsString(pc);
}


// This must match ByteCodeOpcode
static const char *BYTE_CODE_COMMAND_STRING[] = {
        "NOP                    ",
        "CALL_FUNCTION          ",  // value = # of arguments to pass
        "LOAD_CONSTANT          ",  // value = constant enum
        "LOAD_IMMEDIATE         ",  // value = number to load
        "LOAD_DATA              ",  // value = index of operand
        "LOAD_BOUND_SYMBOL      ",  // value = index of operand containing bound symbol
        "ATTRIBUTE_ACCESS       ",  // value = index of operand containing attribute name
        "ARRAY_ACCESS           ",  // Load an element from an array or map
        "UNARY_PLUS             ",
        "UNARY_MINUS            ",
        "UNARY_NOT              ",
        "BINARY_MULTIPLY        ",
        "BINARY_DIVIDE          ",
        "BINARY_REMAINDER       ",
        "BINARY_ADD             ",
        "BINARY_SUBTRACT        ",
        "COMPARE_OP             ",  // value = comparison enum (<, <=, ==, !=, >, >=)
        "JUMP                   ",  // value = offset
        "JUMP_IF_FALSE_OR_POP   ",  // value = offset
        "JUMP_IF_TRUE_OR_POP    ",  // value = offset
        "JUMP_IF_NOT_NULL_OR_POP",  // value = offset
        "POP_JUMP_IF_FALSE      ",  // value = offset
        "MERGE_AS_STRING        ",
        "APPEND_ARRAY           ",
        "APPEND_MAP             ",
};

// This must match the enumerated ByteCodeComparison values
static const char *BYTE_CODE_COMPARE_STRING[] = {
        "<",
        "<=",
        "==",
        "!=",
        ">",
        ">="
};

// This must match the enumerated ByteCodeConstant values
static const char *BYTE_CODE_CONSTANT_STRING[] = {
        "null",
        "false",
        "true",
        "empty_string",
        "empty_array",
        "empty_map"
};


static std::string
lineNumber(int i, int num)
{
    auto result = std::to_string(i);
    int offset = std::max(num - static_cast<int>(result.size()), 0);
    return std::string(offset, ' ') + result;
}


std::string
ByteCode::instructionAsString(int pc) const
{
    if (pc < 0 || pc >= mInstructions.size())
        return lineNumber(pc, 6);

    auto& cmd = mInstructions.at(pc);
    auto result = lineNumber(pc, 6) + "  " + cmd.toString();

    switch (cmd.type) {
        case BC_OPCODE_CALL_FUNCTION:
            result += " argument_count=" + std::to_string(cmd.value);
            break;
        case BC_OPCODE_LOAD_CONSTANT:
            result += std::string(" ") + BYTE_CODE_CONSTANT_STRING[cmd.value];
            break;
        case BC_OPCODE_LOAD_DATA:
        case BC_OPCODE_ATTRIBUTE_ACCESS:
        case BC_OPCODE_LOAD_BOUND_SYMBOL:
            result += " [" + mData.at(cmd.value).toDebugString() + "]";
            break;
        case BC_OPCODE_COMPARE_OP:
            result += std::string(" ") + BYTE_CODE_COMPARE_STRING[cmd.value];
            break;
        case BC_OPCODE_JUMP:
        case BC_OPCODE_JUMP_IF_FALSE_OR_POP:
        case BC_OPCODE_JUMP_IF_TRUE_OR_POP:
        case BC_OPCODE_JUMP_IF_NOT_NULL_OR_POP:
        case BC_OPCODE_POP_JUMP_IF_FALSE:
            result += " GOTO " + std::to_string(pc + cmd.value + 1);
            break;
        default:
            break;
    }

    return result;
}


std::string
ByteCodeInstruction::toString() const
{
    return std::string(BYTE_CODE_COMMAND_STRING[type]) + " (" + std::to_string(value) + ")";
}


} // namespace datagrammar
} // namespace apl