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

#ifndef _APL_BYTE_CODE_H
#define _APL_BYTE_CODE_H

#include <vector>

#include "apl/datagrammar/functions.h"
#include "apl/primitives/objectdata.h"


namespace apl {
namespace datagrammar {

using bciValueType = std::int32_t;
const unsigned OPCODE_BITS = 8;
const unsigned BCI_BITS = 24;
const int MAX_BCI_VALUE = (1 << (BCI_BITS-1)) - 1;
const int MIN_BCI_VALUE = -(1 << (BCI_BITS-1));

static_assert(MAX_BCI_VALUE == 8388607, "Incorrect MAX BCI value");
static_assert(MIN_BCI_VALUE == -8388608, "Incorrect MIN BCI value");


template<class T>
bool fitsInBCI(T value) {
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "Numeric type required");
    auto v = static_cast<bciValueType>(value);
    return value == v && v <= MAX_BCI_VALUE && v >= MIN_BCI_VALUE;
}

template<class T>
bciValueType asBCI(T value) {
    assert(fitsInBCI(value));
    return static_cast<bciValueType>(value);
}

/**
 * Valid byte code commands. This list will grow over time.
 * Do not write code that depends on the order of the commands.
 */
enum ByteCodeOpcode {
    BC_OPCODE_NOP = 0,
    BC_OPCODE_CALL_FUNCTION,     // TOS = TOS_n(TOS_(n-1), ..., TOS) where n = value
    BC_OPCODE_LOAD_CONSTANT,     // TOS = ByteCodeConstant(value)
    BC_OPCODE_LOAD_IMMEDIATE,    // TOS = value
    BC_OPCODE_LOAD_DATA,         // TOS = data[value]
    BC_OPCODE_LOAD_BOUND_SYMBOL, // TOS = TOS.eval()
    BC_OPCODE_ATTRIBUTE_ACCESS,  // TOS = TOS[data[value]]
    BC_OPCODE_ARRAY_ACCESS,      // TOS = TOS_1[TOS]
    BC_OPCODE_UNARY_PLUS,        // TOS = +TOS
    BC_OPCODE_UNARY_MINUS,       // TOS = -TOS
    BC_OPCODE_UNARY_NOT,         // TOS = !TOS
    BC_OPCODE_BINARY_MULTIPLY,   // TOS = TOS_1 * TOS
    BC_OPCODE_BINARY_DIVIDE,     // TOS = TOS_1 / TOS
    BC_OPCODE_BINARY_REMAINDER,  // TOS = TOS_1 % TOS
    BC_OPCODE_BINARY_ADD,        // TOS = TOS_1 + TOS
    BC_OPCODE_BINARY_SUBTRACT,   // TOS = TOS_1 - TOS
    BC_OPCODE_COMPARE_OP,        // TOS = Compare(ByteCodeComparison(value), TOS_1, TOS)
    BC_OPCODE_JUMP,              // pc += value + 1
    BC_OPCODE_JUMP_IF_FALSE_OR_POP,    // If TOS is false, pc += value + 1 else pop
    BC_OPCODE_JUMP_IF_TRUE_OR_POP,     // If TOS is true, pc += value + 1 else pop
    BC_OPCODE_JUMP_IF_NOT_NULL_OR_POP, // If TOS is not nul, pc += value + 1 else pop
    BC_OPCODE_POP_JUMP_IF_FALSE,       // If TOS is values, pc += value + 1.  Pop in either case.
    BC_OPCODE_MERGE_STRING,     // TOS = TOS_n + ... + TOS where n = value - 1
    BC_OPCODE_APPEND_ARRAY,     // TOS = TOS_1.append(TOS)
    BC_OPCODE_APPEND_MAP,       // TOS = TOS_2.append(TOS_1, TOS)
};

/**
 * Sub-category of BC_OPCODE_COMPARE_OP
 */
enum ByteCodeComparison {
    BC_COMPARE_LESS_THAN = 0,
    BC_COMPARE_LESS_THAN_OR_EQUAL,
    BC_COMPARE_EQUAL,
    BC_COMPARE_NOT_EQUAL,
    BC_COMPARE_GREATER_THAN,
    BC_COMPARE_GREATER_THAN_OR_EQUAL
};

/**
 * Pre-defined constants that don't need to be added to the operands vector.
 */
enum ByteCodeConstant {
    BC_CONSTANT_NULL = 0,
    BC_CONSTANT_FALSE,
    BC_CONSTANT_TRUE,
    BC_CONSTANT_EMPTY_STRING,
    BC_CONSTANT_EMPTY_ARRAY,
    BC_CONSTANT_EMPTY_MAP,
};

/**
 * Convert a named constant into an appropriate Object
 * @param value The named constant
 * @return The appropriate Object
 */
inline Object getConstant(ByteCodeConstant value) {
    switch (value) {
        case BC_CONSTANT_NULL:
            return Object::NULL_OBJECT();
        case BC_CONSTANT_FALSE:
            return Object::FALSE_OBJECT();
        case BC_CONSTANT_TRUE:
            return Object::TRUE_OBJECT();
        case BC_CONSTANT_EMPTY_STRING:
            return Object("");
        case BC_CONSTANT_EMPTY_ARRAY:
            return Object::EMPTY_MUTABLE_ARRAY();
        case BC_CONSTANT_EMPTY_MAP:
            return Object::EMPTY_MUTABLE_MAP();
        default:
            // Refer to http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1766
            return Object::NULL_OBJECT();
    }
}

/**
 * Evaluate the comparison as per given operator for two values
 * @param comparison The comparison operator.
 * @param a LHS operand.
 * @param b RHS operand.
 * @return True if the comparison evaluates to true, false otherwise.
 */
inline bool CompareOp(ByteCodeComparison comparison, const Object& a, const Object& b) {
    if(a.isNaN() || b.isNaN())
        return comparison == BC_COMPARE_NOT_EQUAL;

    int value = Compare(a, b);
    switch (comparison) {
        case BC_COMPARE_LESS_THAN:
            return value == -1;
        case BC_COMPARE_LESS_THAN_OR_EQUAL:
            return value != 1;
        case BC_COMPARE_EQUAL:
            return value == 0;
        case BC_COMPARE_NOT_EQUAL:
            return value != 0;
        case BC_COMPARE_GREATER_THAN:
            return value == 1;
        case BC_COMPARE_GREATER_THAN_OR_EQUAL:
            return value != -1;
        default:
            // Refer to http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1766
            return value == 0;
    }
}

/**
 * A single byte code instruction contains an opcode and a value
 */
struct ByteCodeInstruction {
    ByteCodeOpcode type : OPCODE_BITS;
    bciValueType value : BCI_BITS;

    std::string toString() const;
};

static_assert(sizeof(ByteCodeInstruction) == 4, "Wrong size of ByteCodeInstruction");

/**
 * Store an expression that has been compiled into byte code.
 */
class ByteCode : public ObjectData, public std::enable_shared_from_this<ByteCode>
{
public:
    explicit ByteCode(const ContextPtr& context)
        : mContext(context)
    {}

    /**
     * Evaluate this byte code in the associated context.
     * @return The result of the evaluation.
     */
    Object eval() const override;

    /**
     * @return A simplified version of this byte code.  This may be a constant expression.
     */
    Object simplify();

    /**
     * Retrieve the list of mutable global symbols used by this byte code and suitable
     * for building data-binding dependencies.  This method also optimizes the underlying
     * byte code for higher performance.
     * @param symbols An empty symbols map to be populated.
     */
    void symbols(SymbolReferenceMap& symbols);

    /**
     * Decompile the byte code and write the disassembled code to the LOG.
     */
    void dump() const;

    /**
     * @return True if this byte code has been passed through the optimizer
     */
    bool isOptimized() const { return mOptimized; }

    /**
     * @return Lock and return the context referenced by this byte code
     */
    ContextPtr getContext() const;

    /**
     * Return a formatted de-compiled line
     * @param pc The program counter location
     * @return The de-compiled line as a string
     */
    std::string instructionAsString(int pc) const;

    /**
     * @return Number of instructions
     */
    size_t instructionCount() const { return mInstructions.size(); }

    std::string toDebugString() const override { return "Compiled Byte Code"; }

    friend class ByteCodeAssembler;
    friend class ByteCodeOptimizer;
    friend class ByteCodeEvaluator;

private:
    std::weak_ptr<Context> mContext;
    std::vector<ByteCodeInstruction> mInstructions;
    std::vector<Object> mData;

    bool mOptimized = false;
};

} // namespace datagrammar
} // namespace apl

#endif // _APL_BYTE_CODE_H
