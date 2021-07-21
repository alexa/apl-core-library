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

#include <tao/pegtl.hpp>

#include "apl/datagrammar/boundsymbol.h"
#include "apl/datagrammar/bytecodeassembler.h"
#include "apl/datagrammar/bytecode.h"
#include "apl/datagrammar/bytecodeevaluator.h"
#include "apl/datagrammar/databindingrules.h"
#include "apl/datagrammar/databindingerrors.h"

#include "apl/engine/context.h"
#include "apl/utils/log.h"
#include "apl/utils/session.h"

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;

namespace apl {
namespace datagrammar {


// NOTE: This might be faster without the lookup - but we'd have to extend the
// PEGTL grammar.
static const std::map<std::string, ByteCodeAssembler::Operator> sByteCodeBinaryOperators = {
    {"*",  {BC_ORDER_MULTIPLICATIVE,    BC_OPCODE_BINARY_MULTIPLY,  0}},
    {"/",  {BC_ORDER_MULTIPLICATIVE,    BC_OPCODE_BINARY_DIVIDE,    0}},
    {"%",  {BC_ORDER_MULTIPLICATIVE,    BC_OPCODE_BINARY_REMAINDER, 0}},
    {"+",  {BC_ORDER_ADDITIVE,          BC_OPCODE_BINARY_ADD,       0}},
    {"-",  {BC_ORDER_ADDITIVE,          BC_OPCODE_BINARY_SUBTRACT,  0}},
    {"<",  {BC_ORDER_COMPARISON,        BC_OPCODE_COMPARE_OP,       BC_COMPARE_LESS_THAN}},
    {">",  {BC_ORDER_COMPARISON,        BC_OPCODE_COMPARE_OP,       BC_COMPARE_GREATER_THAN}},
    {"<=", {BC_ORDER_COMPARISON,        BC_OPCODE_COMPARE_OP,       BC_COMPARE_LESS_THAN_OR_EQUAL}},
    {">=", {BC_ORDER_COMPARISON,        BC_OPCODE_COMPARE_OP,       BC_COMPARE_GREATER_THAN_OR_EQUAL}},
    {"==", {BC_ORDER_EQUALITY,          BC_OPCODE_COMPARE_OP,       BC_COMPARE_EQUAL}},
    {"!=", {BC_ORDER_EQUALITY,          BC_OPCODE_COMPARE_OP,       BC_COMPARE_NOT_EQUAL}},
    {"[",  {BC_ORDER_FIELD_OR_FUNCTION, BC_OPCODE_ARRAY_ACCESS,     0}}
};

static const std::map<char, ByteCodeAssembler::Operator> sByteCodeUnaryOperators = {
    {'+', {BC_ORDER_UNARY, BC_OPCODE_UNARY_PLUS,  0}},
    {'-', {BC_ORDER_UNARY, BC_OPCODE_UNARY_MINUS, 0}},
    {'!', {BC_ORDER_UNARY, BC_OPCODE_UNARY_NOT,   0}},
};

// Uncomment this line to get line-by-line tracing in the PEGTL grammar
// #define PEGTL_ERROR_CTRL datagrammar::traced_error_control

#ifndef PEGTL_ERROR_CTRL
#define PEGTL_ERROR_CTRL datagrammar::error_control
#endif


Object
ByteCodeAssembler::parse(const Context& context, const std::string& value)
{
    // Short-circuit the parser if there are no embedded expressions
    if (value.find("${") == std::string::npos)
        return value;

    pegtl::string_input<> in(value, "");
    try {
        datagrammar::ByteCodeAssembler assembler(context);

        pegtl::parse<datagrammar::grammar, datagrammar::action, PEGTL_ERROR_CTRL>(in, assembler);
        return assembler.retrieve();
    }
    catch (const pegtl::parse_error& e) {
        const auto p = e.positions.front();
        CONSOLE_CTX(context) << "Syntax error: " << e.what();
        CONSOLE_CTX(context) << in.line_at(p);
        CONSOLE_CTX(context) << std::string(p.byte_in_line, ' ') << "^";
    }

    return value;
}

ByteCodeAssembler::ByteCodeAssembler(const Context& context)
    : mContext(std::const_pointer_cast<Context>(context.shared_from_this())),
      mCode{CodeUnit(mContext)}
{
    mInstructionRef = &mCode.byteCode->mInstructions;
    mDataRef = &mCode.byteCode->mData;
    mOperatorsRef = &mCode.operators;
}


Object
ByteCodeAssembler::retrieve() const
{
    return mCode.byteCode;
}

void
ByteCodeAssembler::loadOperand(const apl::Object& value)
{
    // The "value" of the command is the location in the operand list
    auto len = mDataRef->size();
    mDataRef->emplace_back(value);
    mInstructionRef->emplace_back(ByteCodeInstruction{BC_OPCODE_LOAD_DATA, asBCI(len)});
}

void
ByteCodeAssembler::loadConstant(ByteCodeConstant value)
{
    mInstructionRef->emplace_back(ByteCodeInstruction{BC_OPCODE_LOAD_CONSTANT, value});
}

void
ByteCodeAssembler::loadImmediate(bciValueType value)
{
    mInstructionRef->emplace_back(ByteCodeInstruction{BC_OPCODE_LOAD_IMMEDIATE, value});
}

void
ByteCodeAssembler::loadGlobal(const std::string& name)
{
    auto cr = mContext->find(name);
    if (cr.empty()) { // Not found -> load NULL
        mInstructionRef->emplace_back(
            ByteCodeInstruction{BC_OPCODE_LOAD_CONSTANT, BC_CONSTANT_NULL});
        return;
    }

    auto len = asBCI(mDataRef->size());
    // Immutable globals can be replaced by a constant value
    if (!cr.object().isMutable()) {
        mDataRef->emplace_back(cr.object().value());
        mInstructionRef->emplace_back(ByteCodeInstruction{BC_OPCODE_LOAD_DATA, len});
        return;
    }

    // Mutable globals have a bound symbol
    mDataRef->emplace_back(Object(std::make_shared<BoundSymbol>(cr.context(), name)));
    mInstructionRef->emplace_back(ByteCodeInstruction{BC_OPCODE_LOAD_BOUND_SYMBOL, len});
}

void
ByteCodeAssembler::pushAttributeName(const std::string &name)
{
    // The "value" of the command is the location in the operand list
    auto len = mDataRef->size();
    mDataRef->emplace_back(name);
    mOperatorsRef->push_back({BC_ORDER_ATTRIBUTE, BC_OPCODE_ATTRIBUTE_ACCESS, asBCI(len)});
}

void
ByteCodeAssembler::loadAttribute()
{
    // The "value" of the command is the location in the operand list
    auto& back = mOperatorsRef->back();
    assert(back.order == BC_ORDER_ATTRIBUTE);
    mInstructionRef->emplace_back(ByteCodeInstruction{BC_OPCODE_ATTRIBUTE_ACCESS, asBCI(back.value)});
    mOperatorsRef->pop_back();
}

void
ByteCodeAssembler::pushUnaryOperator(char ch)
{
    auto iter = sByteCodeUnaryOperators.find(ch);
    assert(iter != sByteCodeUnaryOperators.end());

    // TODO: Should we just hold a pointer instead of copying the entire object?
    mOperatorsRef->push_back(iter->second);
}

void
ByteCodeAssembler::reduceUnary()
{
    while (!mOperatorsRef->empty()) {
        auto& back = mOperatorsRef->back();
        if (back.order != BC_ORDER_UNARY)
            return;

        mInstructionRef->emplace_back(ByteCodeInstruction{back.command, asBCI(back.value)});
        mOperatorsRef->pop_back();
    }
}

void
ByteCodeAssembler::pushBinaryOperator(const std::string& op)
{
    auto iter = sByteCodeBinaryOperators.find(op);
    assert(iter != sByteCodeBinaryOperators.end());

    reduceBinary(iter->second.order);
    mOperatorsRef->push_back(iter->second);
}

void
ByteCodeAssembler::reduceBinary(ByteCodeOrder order)
{
    // Reduce the top operator on the stack if it matches this order
    auto back = mOperatorsRef->rbegin();
    if (back == mOperatorsRef->rend() || back->order != order)
        return;

    mInstructionRef->emplace_back(ByteCodeInstruction{back->command, asBCI(back->value)});
    mOperatorsRef->pop_back();
}


/**
 * Called once per AND statement on the stack (that's a '&&')
 * Each AND statement gets a single JUMP_IF_FALSE_OR_POP statement
 * with the value set to the offset to be added to the stack counter.
 * This creates a short-circuiting AND operation.
 *
 * We push the AND operator on the operators stack and use the value field
 * of that operator to store the absolute position of the JUMP_IF_FALSE_OR_POP
 * statement in the command stack.  When all AND statements have been
 * gathered, walk backwards through the operator stack and fix up all of the
 * JUMP locations.
 *
 *    JUMP_IF_FALSE_OR_POP <label>
 *    <Other AND Blocks...>
 * label:
 */
void
ByteCodeAssembler::pushAnd()
{
    auto current_index = static_cast<int>(mInstructionRef->size());
    mInstructionRef->emplace_back(ByteCodeInstruction{BC_OPCODE_JUMP_IF_FALSE_OR_POP, 10000});  // Leave a large number
    mOperatorsRef->emplace_back(Operator{BC_ORDER_LOGICAL_AND, BC_OPCODE_NOP, current_index});
}

/**
 * Called once per OR statement on the stack (that's a '||')
 * Each OR statement gets a single JUMP_IF_TRUE_OR_POP statement
 * with the value set to the offset to be added to the stack counter.
 * This creates a short-circuiting OR operation
 *
 * We push the OR operator on the operators stack and use the value field
 * of that operator to store the absolute position of the JUMP_IF_TRUE_OR_POP
 * statement in the command stack.  When all OR statements have been
 * gathered, walk backwards through the operator stack and fix up all of the
 * JUMP locations.
 *
 *    JUMP_IF_TRUE_OR_POP <label>
 *    <Other OR Blocks...>
 * label:
 */
void
ByteCodeAssembler::pushOr()
{
    auto current_index = static_cast<int>(mInstructionRef->size());
    mInstructionRef->emplace_back(ByteCodeInstruction{BC_OPCODE_JUMP_IF_TRUE_OR_POP, 10000});  // Leave a large number
    mOperatorsRef->emplace_back(Operator{BC_ORDER_LOGICAL_OR, BC_OPCODE_NOP, current_index});
}

void
ByteCodeAssembler::pushNullC()
{
    auto current_index = static_cast<int>(mInstructionRef->size());
    mInstructionRef->emplace_back(ByteCodeInstruction{BC_OPCODE_JUMP_IF_NOT_NULL_OR_POP, 10000});  // Leave a large number
    mOperatorsRef->emplace_back(Operator{BC_ORDER_NULLC, BC_OPCODE_NOP, current_index});
}

/**
 * Called when the AND/OR/NULLC statements are done
 */
void
ByteCodeAssembler::reduceJumps(ByteCodeOrder order)
{
    auto current_index = static_cast<int>(mInstructionRef->size());  // This is where we jump to

    // Fix all of the jump offsets
    while (!mOperatorsRef->empty()) {
        auto& back = mOperatorsRef->back();
        if (back.order != order)
            return;
        auto jump_index = back.value;
        mInstructionRef->at(jump_index).value = current_index - jump_index - 1;
        mOperatorsRef->pop_back();
    }
}

/*
 * Ternary structure:
 *
 *    POP_JUMP_IF_FALSE <label1>
 *    ...then commands...
 *    JUMP <label2>
 * label1:
 *    ...else commands...
 * label2:
 */

void
ByteCodeAssembler::pushTernaryIf()
{
    auto current_index = static_cast<int>(mInstructionRef->size());
    mInstructionRef->emplace_back(ByteCodeInstruction{BC_OPCODE_POP_JUMP_IF_FALSE, 10000});  // Leave a large number
    mOperatorsRef->emplace_back(Operator{BC_ORDER_TERNARY_IF, BC_OPCODE_NOP, current_index});
}

void
ByteCodeAssembler::pushTernaryElse()
{
    auto current_index = static_cast<int>(mInstructionRef->size());
    mInstructionRef->emplace_back(ByteCodeInstruction{BC_OPCODE_JUMP, 10000});  // Leave a large number

    // Reduce the "IF" clause (this is where it jumps to)
    reduceOneJump(BC_ORDER_TERNARY_IF);

    // Add the ELSE operator
    mOperatorsRef->emplace_back(Operator{BC_ORDER_TERNARY_ELSE, BC_OPCODE_NOP, current_index});
}

void
ByteCodeAssembler::pushComma()
{
    mOperatorsRef->emplace_back(Operator{BC_ORDER_COMMA, BC_OPCODE_NOP, 0});
}

void
ByteCodeAssembler::pushInlineArrayStart()
{
    mOperatorsRef->emplace_back(Operator{BC_ORDER_INLINE_ARRAY, BC_OPCODE_NOP, 0});
    mInstructionRef->emplace_back(ByteCodeInstruction{BC_OPCODE_LOAD_CONSTANT, BC_CONSTANT_EMPTY_ARRAY});
}

void
ByteCodeAssembler::appendInlineArrayArgument()
{
    mInstructionRef->emplace_back(ByteCodeInstruction{BC_OPCODE_APPEND_ARRAY, 0});
}

void
ByteCodeAssembler::pushInlineArrayEnd()
{
    assert(!mOperatorsRef->empty() && mOperatorsRef->back().order == BC_ORDER_INLINE_ARRAY);
    mOperatorsRef->pop_back();
}

void
ByteCodeAssembler::pushInlineMapStart()
{
    mOperatorsRef->emplace_back(Operator{BC_ORDER_INLINE_MAP, BC_OPCODE_NOP, 0});
    mInstructionRef->emplace_back(ByteCodeInstruction{BC_OPCODE_LOAD_CONSTANT, BC_CONSTANT_EMPTY_MAP});
}

void
ByteCodeAssembler::appendInlineMapArgument()
{
    mInstructionRef->emplace_back(ByteCodeInstruction{BC_OPCODE_APPEND_MAP, 0});
}

void
ByteCodeAssembler::pushInlineMapEnd()
{
    assert(!mOperatorsRef->empty() && mOperatorsRef->back().order == BC_ORDER_INLINE_MAP);
    mOperatorsRef->pop_back();
}

void
ByteCodeAssembler::pushFunctionStart()
{
    mOperatorsRef->emplace_back(Operator{BC_ORDER_FUNCTION, BC_OPCODE_NOP, 0});
}

void
ByteCodeAssembler::pushFunctionEnd()
{
    bciValueType comma_count = 0;

    while (!mOperatorsRef->empty()) {
        auto& back = mOperatorsRef->back();

        if (back.order == BC_ORDER_COMMA) {
            comma_count++;
            mOperatorsRef->pop_back();
        } else if (back.order == BC_ORDER_FUNCTION) {
            mInstructionRef->emplace_back(ByteCodeInstruction{BC_OPCODE_CALL_FUNCTION, comma_count});
            mOperatorsRef->pop_back();
            return;
        } else {
            assert(false);
            mOperatorsRef->pop_back();
        }
    }
}

void
ByteCodeAssembler::pushArrayAccessStart()
{
    mOperatorsRef->emplace_back(Operator{BC_ORDER_FIELD_OR_FUNCTION, BC_OPCODE_NOP, 0});
}

void
ByteCodeAssembler::pushArrayAccessEnd()
{
    assert(!mOperatorsRef->empty() && mOperatorsRef->back().order == BC_ORDER_FIELD_OR_FUNCTION);
    mInstructionRef->emplace_back(ByteCodeInstruction{BC_OPCODE_ARRAY_ACCESS, 0});
    mOperatorsRef->pop_back();
}

/**
 * The top of the operator stack should match "order" and can be reduced.
 * @param order
 */
void
ByteCodeAssembler::reduceOneJump(ByteCodeOrder order)
{
    assert(!mOperatorsRef->empty());
    assert(mOperatorsRef->back().order == order);

    auto jump_index = mOperatorsRef->back().value;
    auto current_index = static_cast<int>(mInstructionRef->size());  // This is where we jump to
    mInstructionRef->at(jump_index).value = current_index - jump_index - 1;
    mOperatorsRef->pop_back();
}

void
ByteCodeAssembler::pushGroup()
{
    mOperatorsRef->emplace_back(Operator{BC_ORDER_GROUP, BC_OPCODE_NOP, 0});
}

void
ByteCodeAssembler::popGroup()
{
    assert(!mOperatorsRef->empty());
    assert(mOperatorsRef->back().order == BC_ORDER_GROUP);
    mOperatorsRef->pop_back();
}

void
ByteCodeAssembler::pushDBGroup()
{
    mOperatorsRef->emplace_back(Operator{BC_ORDER_DB, BC_OPCODE_NOP, 0});
}

void
ByteCodeAssembler::popDBGroup()
{
    assert(!mOperatorsRef->empty());
    assert(mOperatorsRef->back().order == BC_ORDER_DB);
    mOperatorsRef->pop_back();
    mOperatorsRef->push_back(Operator{BC_ORDER_STRING_ELEMENT, BC_OPCODE_NOP, 0});
}

void
ByteCodeAssembler::startString()
{
    mOperatorsRef->push_back(Operator{BC_ORDER_STRING, BC_OPCODE_NOP, 0});
}

void
ByteCodeAssembler::addString(const std::string& s)
{
    auto index = asBCI(mDataRef->size());
    mDataRef->emplace_back(s);
    mInstructionRef->emplace_back(ByteCodeInstruction{BC_OPCODE_LOAD_DATA, index});
    mOperatorsRef->emplace_back(Operator{BC_ORDER_STRING_ELEMENT, BC_OPCODE_NOP, 0});
}

void
ByteCodeAssembler::endString()
{
    bciValueType elementCount = 0;
    while (!mOperatorsRef->empty() && mOperatorsRef->back().order == BC_ORDER_STRING_ELEMENT) {
        elementCount++;
        mOperatorsRef->pop_back();
    }

    assert(mOperatorsRef->back().order == BC_ORDER_STRING);
    mOperatorsRef->pop_back();

    if (elementCount == 0)
        mInstructionRef->emplace_back(ByteCodeInstruction{BC_OPCODE_LOAD_CONSTANT, BC_CONSTANT_EMPTY_STRING});
    else if (elementCount > 1)
        mInstructionRef->emplace_back(ByteCodeInstruction{BC_OPCODE_MERGE_STRING, elementCount});
}


std::string
ByteCodeAssembler::toString() const
{
    return "Assembler";
}

} // namespace datagrammar
} // namespace apl