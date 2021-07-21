/*
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
 *
 * Data-binding rules
 */

#ifndef _APL_DATA_BINDING_RULES_H
#define _APL_DATA_BINDING_RULES_H

#include <tao/pegtl.hpp>
#include "databindinggrammar.h"
#include "bytecodeassembler.h"


#include "apl/primitives/object.h"

#ifdef APL_CORE_UWP
    #undef TRUE
    #undef FALSE
#endif

namespace apl {
namespace datagrammar {

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;
using namespace pegtl;

/**
 * \cond ShowDataGrammar
 */

// ******************** ACTIONS *********************

template<typename Rule>
struct action : nothing< Rule >
{
};

// ************** Primitive Types *******************

template<> struct action< number >
{
    template< typename Input >
    static void apply(const Input& in, ByteCodeAssembler& assembler) {
        double value = std::stod(in.string());
        if (fitsInBCI(value))
            assembler.loadImmediate(asBCI(value));
        else
            assembler.loadOperand(Object(value));
    }
};

template<> struct action< key_null >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler) {
        assembler.loadConstant(BC_CONSTANT_NULL);
    }
};

template<> struct action< key_true >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler) {
        assembler.loadConstant(BC_CONSTANT_TRUE);
    }
};

template<> struct action< key_false >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler) {
        assembler.loadConstant(BC_CONSTANT_FALSE);
    }
};

// ************* Dimension *************

template<> struct action< dimension >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler) {
        assembler.loadOperand(Object(Dimension(*(assembler.context()), in.string())));
    }
};

// ************* Unary operations *************
template<> struct action< sym_unary >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler) {
        assembler.pushUnaryOperator(in.string()[0]);
    }
};

template<> struct action< unary_expression >
{
    template< typename Input >
    static void apply(const Input& in, ByteCodeAssembler& assembler) {
        assembler.reduceUnary();
    }
};

// ************* Multiplication, division, modulus *************
template<> struct action< sym_multiplicative > {
    template< typename Input >
    static void apply(const Input& in, ByteCodeAssembler& assembler) {
        assembler.pushBinaryOperator(in.string());
    }
};

template<> struct action< multiplicative_expression >
{
    template< typename Input >
    static void apply(const Input& in, ByteCodeAssembler& assembler) {
        assembler.reduceBinary(BC_ORDER_MULTIPLICATIVE);
    }
};

// ************* Addition, subtraction *************
template<> struct action< sym_additive > {
    template< typename Input >
    static void apply(const Input& in, ByteCodeAssembler& assembler) {
        assembler.pushBinaryOperator(in.string());
    }
};

template<> struct action< additive_expression >
{
    template< typename Input >
    static void apply(const Input& in, ByteCodeAssembler& assembler) {
        assembler.reduceBinary(BC_ORDER_ADDITIVE);
    }
};

// ************* Comparison *************
template<> struct action< sym_compare > {
    template< typename Input >
    static void apply(const Input& in, ByteCodeAssembler& assembler) {
        assembler.pushBinaryOperator(in.string());
    }
};

template<> struct action< comparison_expression >
{
    template< typename Input >
    static void apply(const Input& in, ByteCodeAssembler& assembler) {
        assembler.reduceBinary(BC_ORDER_COMPARISON);
    }
};

// ************* Equality *************
template<> struct action< sym_equal > {
    template< typename Input >
    static void apply(const Input& in, ByteCodeAssembler& assembler) {
        assembler.pushBinaryOperator(in.string());
    }
};

template<> struct action< equality_expression >
{
    template< typename Input >
    static void apply(const Input& in, ByteCodeAssembler& assembler) {
        assembler.reduceBinary(BC_ORDER_EQUALITY);
    }
};

// ************* Logical AND *************
template<> struct action< sym_and > {
    template< typename Input >
    static void apply(const Input& in, ByteCodeAssembler& assembler) {
        assembler.pushAnd();
    }
};

template<> struct action< logical_and_expression >
{
    template< typename Input >
    static void apply(const Input& in, ByteCodeAssembler& assembler) {
        assembler.reduceJumps(BC_ORDER_LOGICAL_AND);
    }
};

// ************* Logical OR *************
template<> struct action< sym_or > {
    template< typename Input >
    static void apply(const Input& in, ByteCodeAssembler& assembler) {
        assembler.pushOr();
    }
};

template<> struct action< logical_or_expression >
{
    template< typename Input >
    static void apply(const Input& in, ByteCodeAssembler& assembler) {
        assembler.reduceJumps(BC_ORDER_LOGICAL_OR);
    }
};

// ************* Null coalescence *************
template<> struct action< sym_nullc > {
    template< typename Input >
    static void apply(const Input& in, ByteCodeAssembler& assembler) {
        assembler.pushNullC();
    }
};

template<> struct action< nullc_expression >
{
    template< typename Input >
    static void apply(const Input& in, ByteCodeAssembler& assembler) {
        assembler.reduceJumps(BC_ORDER_NULLC);
    }
};

// ************* Ternary *************
template<> struct action < sym_question >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.pushTernaryIf();
    }
};

template<> struct action < sym_colon >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.pushTernaryElse();
    }
};

template<> struct action< ternary_tail >
{
    template< typename Input >
    static void apply(const Input& in, ByteCodeAssembler& assembler) {
        assembler.reduceOneJump(BC_ORDER_TERNARY_ELSE);
    }
};

// ************* Starting parenthesis *************
template<> struct action< group_start >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.pushGroup();
    }
};

// ************* Terminal parenthesis *************
template<> struct action< grouping >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.popGroup();
    }
};

// ************* Resource lookup *************
template<> struct action< resource >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.loadGlobal(in.string());  // TODO: Should this be treated differently?
    }
};

// ************* Symbol lookup *************
template<> struct action< plain_symbol >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.loadGlobal(in.string());
    }
};

// ************* Array *************
template<> struct action< array_start >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.pushInlineArrayStart();
    }
};

template<> struct action< array_end >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.pushInlineArrayEnd();
    }
};

template<> struct action< array_comma >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.appendInlineArrayArgument();
    }
};

template<> struct action< array_list >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.appendInlineArrayArgument();    // Insert a fake comma
    }
};

// ************* Map ***************
template<> struct action< map_start >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.pushInlineMapStart();
    }
};

template<> struct action< map_end >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.pushInlineMapEnd();
    }
};

template<> struct action< map_comma >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.appendInlineMapArgument();
    }
};

template<> struct action< map_list >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.appendInlineMapArgument();    // Insert a fake comma
    }
};

// ************* Field access *************
template<> struct action< postfix_identifier >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler) {
        assembler.pushAttributeName(in.string());
        assembler.loadAttribute();
    }
};

// ************* Array access *************
template<> struct action< sym_array_access_start >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.pushArrayAccessStart();
    }
};

template<> struct action< postfix_array_access >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.pushArrayAccessEnd();
    }
};

// ************* Functions ************
template<> struct action<postfix_left_paren >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.pushFunctionStart();
    }
};

template<> struct action< sym_comma >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.pushComma();
    }
};

template<> struct action< argument_list >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.pushComma();  // Insert a fake comma
    }
};

template<> struct action< postfix_right_paren >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.pushFunctionEnd();
    }
};

// *************** Data-binding group **************

template<> struct action< sym_dbstart >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.pushDBGroup();
    }
};

template<> struct action<db_empty>
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler ) {
        assembler.loadConstant(BC_CONSTANT_EMPTY_STRING);
    }
};

template<> struct action< db >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler) {
        assembler.popDBGroup();
    }
};

// ************* Embedded String Handling **************

template<> struct action< ds_start >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler) {
        assembler.startString();
    }
};

template<> struct action< ss_start >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler) {
        assembler.startString();
    }
};

template<> struct action< os_start >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler) {
        assembler.startString();
    }
};

template<> struct action< ss_raw >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler) {
        auto s = in.string();
        if (s.length() > 0)
            assembler.addString(s);
    }
};

template<> struct action< ds_raw >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler) {
        auto s = in.string();
        if (s.length() > 0)
            assembler.addString(s);
    }
};

template<> struct action< os_raw >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler) {
        auto s = in.string();
        if (s.length() > 0)
            assembler.addString(s);
    }
};


template<> struct action< os_string >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler) {
        assembler.endString();
    }
};

template<> struct action< ss_string >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler) {
        assembler.endString();
    }
};

template<> struct action< ds_string >
{
    template< typename Input >
    static void apply( const Input& in, ByteCodeAssembler& assembler) {
        assembler.endString();
    }
};

/**
 * \endcond
 */
} // namespace datagrammar
} // namespace apl

#endif // _APL_DATA_BINDING_RULES_H