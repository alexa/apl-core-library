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

#ifndef _APL_BYTE_CODE_ASSEMBLER_H
#define _APL_BYTE_CODE_ASSEMBLER_H

#include "apl/datagrammar/bytecode.h"

namespace apl {
namespace datagrammar {

/**
 * Ordering groups.  Processing rules with the same order "collapse" into each other.
 */
enum ByteCodeOrder {
    BC_ORDER_FIELD_OR_FUNCTION = 0,
    BC_ORDER_UNARY,
    BC_ORDER_MULTIPLICATIVE,
    BC_ORDER_ADDITIVE,
    BC_ORDER_COMPARISON,
    BC_ORDER_EQUALITY,
    BC_ORDER_LOGICAL_AND,
    BC_ORDER_LOGICAL_OR,
    BC_ORDER_NULLC,
    BC_ORDER_TERNARY_IF,
    BC_ORDER_TERNARY_ELSE,
    BC_ORDER_GROUP,
    BC_ORDER_FUNCTION,
    BC_ORDER_COMMA,
    BC_ORDER_DB,
    BC_ORDER_INLINE_ARRAY,
    BC_ORDER_INLINE_MAP,
    BC_ORDER_STRING,
    BC_ORDER_STRING_ELEMENT,
    BC_ORDER_ATTRIBUTE,
};



/**
 * A ByteCodeAssembler is passed to the PEGTL-based data grammar rules.  As the rules
 * are parsed, byte code is built up in the assembler.
 */
class ByteCodeAssembler {
public:
    /**
     * Parse a string for data-binding expressions and return the result.  The result may
     * be byte code or a simple object.
     * @param context The data-binding context
     * @param value The string to parse
     * @return The calculated result
     */
    static Object parse(const Context& context, const std::string& value);

private:
    Object retrieve() const;

    /*** Methods after this point are for use by the PEGTL parser ***/

    // Load values
    void loadOperand(const Object& value);
    void loadConstant(ByteCodeConstant value);
    void loadImmediate(bciValueType value);
    void loadGlobal(const std::string& name);

    void pushAttributeName(const std::string &name);
    void loadAttribute();

    // Unary operators
    void pushUnaryOperator(char ch);
    void reduceUnary();

    // Binary operators
    void pushBinaryOperator(const std::string& op);
    void reduceBinary(ByteCodeOrder order);

    // Parenthesis
    void pushGroup();
    void popGroup();

    // DB-group
    void pushDBGroup();
    void popDBGroup();

    // AND/OR/NULLC-statement
    void pushAnd();
    void pushOr();
    void pushNullC();
    void reduceJumps(ByteCodeOrder order);

    // Ternary
    void pushTernaryIf();
    void pushTernaryElse();
    void reduceOneJump(ByteCodeOrder order);

    // Array access
    void pushArrayAccessStart();
    void pushArrayAccessEnd();

    // Inline Array creation
    void pushInlineArrayStart();
    void appendInlineArrayArgument();
    void pushInlineArrayEnd();

    // Inline Map creation
    void pushInlineMapStart();
    void appendInlineMapArgument();
    void pushInlineMapEnd();

    // Functions
    void pushFunctionStart();
    void pushComma();
    void pushFunctionEnd();

    // Strings
    void startString();
    void addString(const std::string& s);
    void endString();

    std::string toString() const;

    ContextPtr context() const { return mContext; }

    template<class T> friend struct action;

public:
    struct Operator {
        ByteCodeOrder order;      // Group order
        ByteCodeOpcode command;   // CommandType
        int value;                // Comparison or Constant value
    };

private:
    explicit ByteCodeAssembler(const Context& context);

private:
    struct CodeUnit {
        explicit CodeUnit(const ContextPtr& context)
            : byteCode(std::make_shared<ByteCode>(context)) {}

        std::shared_ptr<ByteCode> byteCode;
        std::vector<Operator> operators;     // Operator stack
    };

    ContextPtr mContext;
    CodeUnit mCode;

    // Convenience references so we don't keep dereferencing the ByteCode
    std::vector<ByteCodeInstruction>* mInstructionRef;
    std::vector<Object>* mDataRef;
    std::vector<Operator>* mOperatorsRef;
};

} // namespace datagrammar
} // namespace apl

#endif // _APL_BYTE_CODE_ASSEMBLER_H
