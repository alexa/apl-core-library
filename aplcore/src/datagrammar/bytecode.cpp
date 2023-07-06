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
#include "apl/datagrammar/bytecodeevaluator.h"
#include "apl/datagrammar/bytecodeoptimizer.h"
#include "apl/engine/context.h"
#include "apl/primitives/boundsymbol.h"
#include "apl/utils/session.h"

namespace apl {
namespace datagrammar {

Object
ByteCode::eval() const
{
    return ByteCode::evaluate(nullptr, 0);
}

Object
ByteCode::evaluate(BoundSymbolSet* symbols, int depth) const
{
    // Check for a trivial instruction
    if (mInstructions.size() == 1) {
        const auto& cmd = mInstructions.at(0);
        switch (cmd.type) {
            case BC_OPCODE_LOAD_BOUND_SYMBOL:
                if (symbols != nullptr)
                    symbols->emplace(mData.at(cmd.value).get<BoundSymbol>());
                return mData.at(cmd.value).eval();

            case BC_OPCODE_LOAD_DATA:
                return mData.at(cmd.value);

            case BC_OPCODE_LOAD_CONSTANT:
                return getConstant(static_cast<ByteCodeConstant>(cmd.value));

            case BC_OPCODE_LOAD_IMMEDIATE:
                return cmd.value;

            default:
                CONSOLE(mContext) << "Unexpected trivial instruction " << cmd.type;
                return Object::NULL_OBJECT();
        }
    }

    ByteCodeEvaluator evaluator(*this, symbols, depth);
    evaluator.advance();
    if (evaluator.isDone())
        return evaluator.getResult();

    CONSOLE(mContext) << "Unable to evaluate byte code data";
    return Object::NULL_OBJECT();
}

ContextPtr
ByteCode::getContext() const
{
    return mContext.lock();
}

void
ByteCode::optimize()
{
    if (!mOptimized) {
        ByteCodeOptimizer::optimize(*this);
        mOptimized = true;
    }
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
        "EVALUATE               ",
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

/**
 * Convert a generic Object into something that looks good in disassembly
 *
 * Null    -> null
 * Boolean -> true / false
 * Number  -> normal conversion
 * String  -> 'VALUE'
 */
static std::string
prettyPrint(const Object& object)
{
    if (object.isNull()) return "null";
    if (object.isString()) return "'" + object.asString() + "'";
    if (object.isMap()) return "BuiltInMap<>";
    if (object.isArray()) return "BuiltInArray<>";
    if (object.is<BoundSymbol>()) return object.toDebugString();
    return object.asString();
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
        case BC_OPCODE_LOAD_IMMEDIATE:
            result += std::string(" ") + std::to_string(cmd.value);
            break;
        case BC_OPCODE_LOAD_DATA:
        case BC_OPCODE_ATTRIBUTE_ACCESS:
        case BC_OPCODE_LOAD_BOUND_SYMBOL:
            result += " [" + prettyPrint(mData.at(cmd.value)) + "]";
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

/************* Disassembly routines **************/

Disassembly::Iterator::Iterator(const ByteCode& byteCode, size_t lineNumber)
    : mByteCode(byteCode), mLineNumber(lineNumber)
{}

Disassembly::Iterator::reference
Disassembly::Iterator::operator*() const
{
    if (mLineNumber == 0)  // Line 0 is the "Data" label
        return "DATA";
    const auto count = mByteCode.dataCount() + 1;  // Line 1-n are the data items
    if (mLineNumber < count) {
        auto index = mLineNumber - 1;
        return lineNumber(index, 6) + "  " + prettyPrint(mByteCode.dataAt(index));
    }
    if (mLineNumber == count)
        return "INSTRUCTIONS";
    auto pc = mLineNumber - count - 1;
    if (pc < mByteCode.instructionCount())
        return mByteCode.instructionAsString(pc);

    return "";   // Should never reach this line
}

Disassembly::Iterator&
Disassembly::Iterator::operator++()
{
    mLineNumber++;
    return *this;
}

bool
operator== (const Disassembly::Iterator& lhs, const Disassembly::Iterator& rhs)
{
    return &lhs.mByteCode == &rhs.mByteCode && lhs.mLineNumber == rhs.mLineNumber;
}

bool
operator!= (const Disassembly::Iterator& lhs, const Disassembly::Iterator& rhs)
{
    return &lhs.mByteCode != &rhs.mByteCode || lhs.mLineNumber != rhs.mLineNumber;
}

Disassembly::Iterator
Disassembly::begin() const
{
    return {mByteCode, 0};
}

Disassembly::Iterator
Disassembly::end() const
{
    return {mByteCode, 2 + mByteCode.instructionCount() + mByteCode.dataCount()};
}



} // namespace datagrammar
} // namespace apl