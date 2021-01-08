/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef _APL_DATA_BINDING_GRAMMAR_H
#define _APL_DATA_BINDING_GRAMMAR_H

#include <tao/pegtl.hpp>
#include "databindinggrammar.h"
#include "databindingstack.h"
#include "node.h"

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
    static void apply(const Input& in, Stacks& stacks) {
        double value = std::stod(in.string());
        stacks.push(Object(value));
    }
};

template<> struct action< key_null >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks) {
        stacks.push(Object::NULL_OBJECT());
    }
};

template<> struct action< key_true >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks) {
        stacks.push(Object::TRUE_OBJECT());
    }
};

template<> struct action< key_false >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks) {
        stacks.push(Object::FALSE_OBJECT());
    }
};

// ************* Unary operations *************
template<> struct action< star< sym_unary > >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks) {
        std::string s = in.string();

        for (auto it=s.begin() ; it != s.end() ; it++) {
            auto iter = sUnaryOperators.find(*it);
            if (iter != sUnaryOperators.end())
                stacks.push(iter->second);
        }
    }
};

template<> struct action< sfactor >
{
    template< typename Input >
    static void apply(const Input& in, Stacks& stacks) {
        stacks.reduceUnary(OP_UNARY);
    }
};

// ************* Multiplication, division, modulus *************
template<> struct action< sym_term > {
    template< typename Input >
    static void apply(const Input& in, Stacks& stacks) {
        std::string s = in.string();
        stacks.push(sTermOperators.find(s)->second);
    }
};

template<> struct action< term >
{
    template< typename Input >
    static void apply(const Input& in, Stacks& stacks) {
        stacks.reduceLR(OP_TERM);
    }
};

// ************* Addition, subtraction *************
template<> struct action< sym_expr > {
    template< typename Input >
    static void apply(const Input& in, Stacks& stacks) {
        std::string s = in.string();
        stacks.push(sExpressionOperators.find(s)->second);
    }
};

template<> struct action< expression >
{
    template< typename Input >
    static void apply(const Input& in, Stacks& stacks) {
        stacks.reduceLR(OP_EXPRESSION);
    }
};

// ************* Comparison *************
template<> struct action< sym_compare > {
    template< typename Input >
    static void apply(const Input& in, Stacks& stacks) {
        std::string s = in.string();
        stacks.push(sCompareOperators.find(s)->second);
    }
};

template<> struct action< compare >
{
    template< typename Input >
    static void apply(const Input& in, Stacks& stacks) {
        stacks.reduceLR(OP_COMPARISON);
    }
};

// ************* Equality *************
template<> struct action< sym_equal > {
    template< typename Input >
    static void apply(const Input& in, Stacks& stacks) {
        std::string s = in.string();
        stacks.push(sEqualityOperators.find(s)->second);
    }
};

template<> struct action< equate >
{
    template< typename Input >
    static void apply(const Input& in, Stacks& stacks) {
        stacks.reduceLR(OP_EQUALITY);
    }
};

// ************* Logical AND *************
template<> struct action< sym_and > {
    template< typename Input >
    static void apply(const Input& in, Stacks& stacks) {
        stacks.push(AND_OPERATOR);
    }
};

template<> struct action< logical_and >
{
    template< typename Input >
    static void apply(const Input& in, Stacks& stacks) {
        stacks.reduceLR(OP_LOGICAL_AND);
    }
};

// ************* Logical OR *************
template<> struct action< sym_or > {
    template< typename Input >
    static void apply(const Input& in, Stacks& stacks) {
        stacks.push(OR_OPERATOR);
    }
};

template<> struct action< logical_or >
{
    template< typename Input >
    static void apply(const Input& in, Stacks& stacks) {
        stacks.reduceLR(OP_LOGICAL_OR);
    }
};

// ************* Null coalescence *************
template<> struct action< sym_nullc > {
    template< typename Input >
    static void apply(const Input& in, Stacks& stacks) {
        stacks.push(NULLC_OPERATOR);
    }
};

template<> struct action< nullc >
{
    template< typename Input >
    static void apply(const Input& in, Stacks& stacks) {
        stacks.reduceLR(OP_NULLC);
    }
};

// ************* Ternary *************
template<> struct action < sym_colon >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks ) {
        stacks.push(TERNARY_OPERATOR);
    }
};

template<> struct action < sym_question >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks ) {
        stacks.push(TERNARY_OPERATOR);
    }
};

template<> struct action< ternary_tail >
{
    template< typename Input >
    static void apply(const Input& in, Stacks& stacks) {
        stacks.reduceTernary(OP_TERNARY);
    }
};

// ************* Starting parenthesis *************
template<> struct action< sym_group_start >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks ) {
        stacks.push(GROUP_OPERATOR);
    }
};

// ************* Terminal parenthesis *************
template<> struct action< grouping >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks ) {
        stacks.pop(GROUP_OPERATOR);
    }
};

// ************* Resource lookup *************
template<> struct action< resource >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks ) {
        auto name = Object(in.string());
        stacks.push(Symbol(stacks.context(), std::vector<Object>{name}, "Resource"));
    }
};

// ************* Symbol lookup *************
template<> struct action< symbol >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks ) {
        auto name = Object(in.string());
        stacks.push(Symbol(stacks.context(), std::vector<Object>{name}, "Symbol"));
    }
};

// ************* Field access *************
template<> struct action< postfix_identifier >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks) {
        stacks.push(Object(in.string()));
    }
};

template<> struct action< sym_attribute >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks ) {
        stacks.push(FIELD_ACCESS_OPERATOR);
    }
};

template<> struct action< postfix_attribute >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks ) {
        stacks.reduceLR(OP_FIELD_OR_FUNCTION);
    }
};


// ************* Array access *************
template<> struct action< sym_array_access_start >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks ) {
        stacks.push(ARRAY_ACCESS_OPERATOR);
        stacks.open();
    }
};

template<> struct action< postfix_array_access >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks ) {
        stacks.close(kCombineSingle);
        stacks.reduceLR(OP_FIELD_OR_FUNCTION);
    }
};

// ************* Functions ************
template<> struct action< postfix_left_paren >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks ) {
        stacks.push(FUNCTION_OPERATOR);
        stacks.open();
    }
};

template<> struct action< postfix_function >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks ) {
        stacks.close(kCombineVector);
        stacks.reduceBinary(OP_FIELD_OR_FUNCTION);
    }
};

// *************** Data-binding group **************

template<> struct action< sym_dbstart >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks ) {
        stacks.push(DB_OPERATOR);
    }
};

template<> struct action< db >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks) {
        stacks.pop(DB_OPERATOR);
    }
};

// ************* Embedded String Handling **************

template<> struct action< one<'"'> >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks) {
        stacks.open();
    }
};

template<> struct action< one<'\''> >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks) {
        stacks.open();
    }
};

template<> struct action< ss_raw >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks) {
        auto s = in.string();
        if (s.length() > 0)
            stacks.push(Object(s));
    }
};

template<> struct action< ss_string >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks) {
        stacks.close(kCombineEmbeddedString);
    }
};

template<> struct action< key_dp >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks) {
        stacks.push(Dimension(stacks.popNumber()));
    }
};

template<> struct action< key_px >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks) {
        stacks.push(Dimension(stacks.context().pxToDp(stacks.popNumber())));
    }
};

template<> struct action< key_vh >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks) {
        stacks.push(Dimension(stacks.context().vhToDp(stacks.popNumber())));
    }
};

template<> struct action< key_vw >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks) {
        stacks.push(Dimension(stacks.context().vwToDp(stacks.popNumber())));
    }
};

template<> struct action< ds_raw >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks) {
        auto s = in.string();
        if (s.length() > 0)
            stacks.push(Object(s));
    }
};

template<> struct action< ds_string >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks) {
        stacks.close(kCombineEmbeddedString);
    }
};

template<> struct action< raw >
{
    template< typename Input >
    static void apply( const Input& in, Stacks& stacks) {
        auto s = in.string();
        if (s.length() > 0)
            stacks.push(Object(s));
    }
};


    /**
     * \endcond
     */
} // namespace datagrammar
} // namespace apl

#endif // _APL_DATA_BINDING_GRAMMAR_H