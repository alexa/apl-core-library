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

#include <pegtl.hh>
#include "databindinggrammar.h"
#include "databindingstack.h"
#include "node.h"

#include "apl/primitives/object.h"

namespace apl {
namespace datagrammar {

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

template<> struct action< intnum >
{
    static void apply(const input& in, Stacks& stacks) {
        int value = std::stoi(in.string());
        stacks.push(Object((double) value));
    }
};

template<> struct action< floatnum >
{
    static void apply(const input& in, Stacks& stacks) {
        double value = std::stod(in.string());
        stacks.push(Object(value));
    }
};

template<> struct action< null_ >
{
    static void apply( const input& in, Stacks& stacks) {
        stacks.push(Object::NULL_OBJECT());
    }
};

template<> struct action< true_ >
{
    static void apply( const input& in, Stacks& stacks) {
        stacks.push(Object::TRUE());
    }
};

template<> struct action< false_ >
{
    static void apply( const input& in, Stacks& stacks) {
        stacks.push(Object::FALSE());
    }
};

template<> struct action< identifier >
{
    static void apply( const input& in, Stacks& stacks) {
        stacks.push(Object(in.string()));
    }
};

// ************* Unary operations *************
template<> struct action< star< sym_unary > >
{
    static void apply( const input& in, Stacks& stacks) {
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
    static void apply(const input& in, Stacks& stacks) {
        stacks.reduceUnary(OP_UNARY);
    }
};

// ************* Multiplication, division, modulus *************
template<> struct action< sym_term > {
    static void apply(const input& in, Stacks& stacks) {
        std::string s = in.string();
        stacks.push(sTermOperators.find(s)->second);
    }
};

template<> struct action< term >
{
    static void apply(const input& in, Stacks& stacks) {
        stacks.reduceLR(OP_TERM);
    }
};

// ************* Addition, subtraction *************
template<> struct action< sym_expr > {
    static void apply(const input& in, Stacks& stacks) {
        std::string s = in.string();
        stacks.push(sExpressionOperators.find(s)->second);
    }
};

template<> struct action< expression >
{
    static void apply(const input& in, Stacks& stacks) {
        stacks.reduceLR(OP_EXPRESSION);
    }
};

// ************* Comparison *************
template<> struct action< sym_compare > {
    static void apply(const input& in, Stacks& stacks) {
        std::string s = in.string();
        stacks.push(sCompareOperators.find(s)->second);
    }
};

template<> struct action< compare >
{
    static void apply(const input& in, Stacks& stacks) {
        stacks.reduceLR(OP_COMPARISON);
    }
};

// ************* Equality *************
template<> struct action< sym_equal > {
    static void apply(const input& in, Stacks& stacks) {
        std::string s = in.string();
        stacks.push(sEqualityOperators.find(s)->second);
    }
};

template<> struct action< equate >
{
    static void apply(const input& in, Stacks& stacks) {
        stacks.reduceLR(OP_EQUALITY);
    }
};

// ************* Logical AND *************
template<> struct action< sym_and > {
    static void apply(const input& in, Stacks& stacks) {
        stacks.push(AND_OPERATOR);
    }
};

template<> struct action< logical_and >
{
    static void apply(const input& in, Stacks& stacks) {
        stacks.reduceLR(OP_LOGICAL_AND);
    }
};

// ************* Logical OR *************
template<> struct action< sym_or > {
    static void apply(const input& in, Stacks& stacks) {
        stacks.push(OR_OPERATOR);
    }
};

template<> struct action< logical_or >
{
    static void apply(const input& in, Stacks& stacks) {
        stacks.reduceLR(OP_LOGICAL_OR);
    }
};

// ************* Null coalescence *************
template<> struct action< sym_nullc > {
    static void apply(const input& in, Stacks& stacks) {
        stacks.push(NULLC_OPERATOR);
    }
};

template<> struct action< nullc >
{
    static void apply(const input& in, Stacks& stacks) {
        stacks.reduceLR(OP_NULLC);
    }
};

// ************* Ternary *************
template<> struct action < sym_colon >
{
    static void apply( const input& in, Stacks& stacks ) {
        stacks.push(TERNARY_OPERATOR);
    }
};

template<> struct action < sym_question >
{
    static void apply( const input& in, Stacks& stacks ) {
        stacks.push(TERNARY_OPERATOR);
    }
};

template<> struct action< ternary_tail >
{
    static void apply(const input& in, Stacks& stacks) {
        stacks.reduceTernary(OP_TERNARY);
    }
};

// ************* Starting parenthesis *************
template<> struct action< one<'('> >
{
    static void apply( const input& in, Stacks& stacks ) {
        stacks.push(GROUP_OPERATOR);
    }
};

// ************* Terminal parenthesis *************
template<> struct action< grouping >
{
    static void apply( const input& in, Stacks& stacks ) {
        stacks.pop(GROUP_OPERATOR);
    }
};

// ************* Resource lookup *************
template<> struct action< resource >
{
    static void apply( const input& in, Stacks& stacks ) {
        auto name = Object(in.string());
        stacks.push(Symbol(stacks.context(), std::vector<Object>{name}, "Resource"));
    }
};

// ************* Symbol lookup *************
template<> struct action< symbol >
{
    static void apply( const input& in, Stacks& stacks ) {
        auto name = Object(in.string());
        stacks.push(Symbol(stacks.context(), std::vector<Object>{name}, "Symbol"));
    }
};

// ************* Field access *************
template<> struct action< one<'.'> >
{
    static void apply( const input& in, Stacks& stacks ) {
        stacks.push(FIELD_ACCESS_OPERATOR);
    }
};

template<> struct action< attr_dot >
{
    static void apply( const input& in, Stacks& stacks ) {
        stacks.reduceLR(OP_FIELD_OR_FUNCTION);
    }
};


// ************* Array access *************
template<> struct action< one<'['> >
{
    static void apply( const input& in, Stacks& stacks ) {
        stacks.push(ARRAY_ACCESS_OPERATOR);
        stacks.open();
    }
};

template<> struct action< attr_bracket >
{
    static void apply( const input& in, Stacks& stacks ) {
        stacks.close(kCombineSingle);
        stacks.reduceLR(OP_FIELD_OR_FUNCTION);
    }
};

// ************* Functions ************
template<> struct action<func_start >
{
    static void apply( const input& in, Stacks& stacks ) {
        stacks.push(FUNCTION_OPERATOR);
        stacks.open();
    }
};

template<> struct action< func_call >
{
    static void apply( const input& in, Stacks& stacks ) {
        stacks.close(kCombineVector);
        stacks.reduceBinary(OP_FIELD_OR_FUNCTION);
    }
};

// *************** Data-binding group **************

template<> struct action< sym_dbstart >
{
    static void apply( const input& in, Stacks& stacks ) {
        stacks.push(DB_OPERATOR);
    }
};

template<> struct action< db >
{
    static void apply( const input& in, Stacks& stacks) {
        stacks.pop(DB_OPERATOR);
    }
};

// ************* Embedded String Handling **************

template<> struct action< one<'"'> >
{
    static void apply( const input& in, Stacks& stacks) {
        stacks.open();
    }
};

template<> struct action< one<'\''> >
{
    static void apply( const input& in, Stacks& stacks) {
        stacks.open();
    }
};

template<> struct action< ss_raw >
{
    static void apply( const input& in, Stacks& stacks) {
        auto s = in.string();
        if (s.length() > 0)
            stacks.push(Object(s));
    }
};

template<> struct action< ss_string >
{
    static void apply( const input& in, Stacks& stacks) {
        stacks.close(kCombineEmbeddedString);
    }
};

template<> struct action< dimension >
{
    static void apply( const input& in, Stacks& stacks) {
        auto s = in.string();
        if (s.length() > 0) {
            stacks.push(Object(Dimension(stacks.context(), s)));
        }
    }
};

template<> struct action< ds_raw >
{
    static void apply( const input& in, Stacks& stacks) {
        auto s = in.string();
        if (s.length() > 0)
            stacks.push(Object(s));
    }
};

template<> struct action< ds_string >
{
    static void apply( const input& in, Stacks& stacks) {
        stacks.close(kCombineEmbeddedString);
    }
};

template<> struct action< raw >
{
    static void apply( const input& in, Stacks& stacks) {
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