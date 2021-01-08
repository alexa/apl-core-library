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
 * Data-binding grammar
 */

#ifndef _APL_DATA_BINDING_GRAMMAR
#define _APL_DATA_BINDING_GRAMMAR

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/abnf.hpp>

namespace apl {
namespace datagrammar {

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;
using namespace pegtl;

// TODO:  These rules are not currently UTF-8 safe (which is our default encoding for RapidJSON)
/**
 * \cond ShowDataGrammar
 */
// Forward declarations
struct ternary;
struct ds_string;
struct ss_string;

struct ws : star<space> {};

struct sym_dbstart : string<'$', '{'> {};
struct sym_dbend : one<'}'> {};
struct sym_question : one<'?'> {};
struct sym_colon : one<':'> {};
struct sym_term : one<'*', '/', '%'>  {};
struct sym_expr : one<'+', '-' > {};
struct sym_compare : sor< string<'<', '='>, string<'>', '='>, one<'<', '>'> > {};
struct sym_equal : sor< string<'=', '='>, string<'!', '='> > {};
struct sym_and : string<'&', '&'> {};
struct sym_or : string<'|', '|'> {};
struct sym_nullc : string<'?', '?'> {};
struct sym_unary : one<'+', '-', '!'> {};
struct sym_comma : one<','> {};
struct sym_decimal : one<'.'> {};
struct sym_attribute : one<'.'> {};
struct sym_array_access_start : one<'['> {};
struct sym_array_access_end : one<']'> {};
struct sym_group_start : one<'('> {};
struct sym_group_end : one<')'> {};

struct str_false : string<'f', 'a', 'l', 's', 'e'> {};
struct str_null : string<'n', 'u', 'l', 'l'> {};
struct str_true : string<'t', 'r', 'u', 'e'> {};
struct str_dp : string<'d', 'p'> {};
struct str_px : string<'p', 'x'> {};
struct str_vh : string<'v', 'h'> {};
struct str_vw : string<'v', 'w'> {};

template<typename Key>
struct key : seq<Key, not_at< identifier_other >> {};

struct key_false : key<str_false> {};
struct key_null : key<str_null> {};
struct key_true : key<str_true> {};
struct key_dp : key<str_dp> {};
struct key_px : key<str_px> {};
struct key_vh : key<str_vh> {};
struct key_vw : key<str_vw> {};

struct zero : if_must<one<'0'>, not_at<digit>> {};
struct number_int : sor<zero, seq< range<'1', '9'>, star<digit>>> {};
struct number     : sor<seq<number_int, sym_decimal, star<digit>>,    // INTEGER . DIGITS*
                        seq<sym_decimal, plus<digit>>,                // . DIGITS+
                        number_int> {};                               // INTEGER

struct symbol : seq<alpha, star<identifier_other> > {};
struct resource : seq<one<'@'>, identifier> {};

struct postfix_identifier: identifier {};
struct postfix_attribute : seq<sym_attribute, postfix_identifier> {};
struct postfix_array_access : seq<sym_array_access_start, ws, ternary, ws, sym_array_access_end> {};
struct postfix_left_paren : one<'('> {};
struct postfix_right_paren : one<')'> {};
struct argument_list : list_must<ternary, sym_comma, space> {};
struct postfix_function : if_must<postfix_left_paren, pad_opt<argument_list, space>, postfix_right_paren> {};
struct postfix : sor<postfix_attribute, postfix_array_access, postfix_function> {};
struct postfix_expression : seq<sor<resource, symbol>, star<postfix>> {};

struct dimension_unit : sor<key_dp, key_px, key_vh, key_vw> {};
struct dimension_or_number : seq<number, opt<ws, dimension_unit>> {};
struct grouping : if_must<sym_group_start, ws, ternary, ws, sym_group_end> {};

struct factor : sor<
    grouping,
    dimension_or_number,
    key_true,
    key_false,
    key_null,
    postfix_expression,
    ss_string,
    ds_string> {
};

struct sfactor : seq<star<sym_unary>, factor> {};
struct term : list_must<sfactor, sym_term, space > {};
struct expression : list_must<term, sym_expr, space > {};
struct compare : list_must<expression, sym_compare, space> {};
struct equate : list_must< compare, sym_equal, space >{};
struct logical_and : list_must< equate, sym_and, space > {};
struct logical_or : list_must< logical_and, sym_or, space >{};
struct nullc : list_must< logical_or, sym_nullc, space >{};
struct ternary_tail : seq< ternary, ws, sym_colon, ws, ternary >{};
struct ternary : seq< nullc,
                      opt< ws, if_must<sym_question, ws, ternary_tail>>> {};

struct db : if_must<sym_dbstart, ws, opt<ternary, ws>, sym_dbend> {};

// TODO: This assumes UTF-8 encoding.  We're relying on RapidJSON to return UTF-8
struct char_ : utf8::any {};

struct ds_raw : until<sor<at<one<'"'> >, at<sym_dbstart> >, must<char_> > {};
struct ds_string : if_must<one<'"'>, list<ds_raw, db>, disable<one<'"'> > > {};

struct ss_raw : until<sor<at<one<'\''> >, at<sym_dbstart> >, must<char_> > {};
struct ss_string : if_must<one<'\''>, list<ss_raw, db>, disable<one<'\''> > > {};

struct raw : until<sor<at<eof>, at<sym_dbstart> >, must<char_> > {};
struct openstring : list<raw, db> {};
struct grammar : must<openstring, eof> {};

/**
 * \endcond
 */

} // namespace datagrammar
} // namespace apl

#endif // _APL_DATA_BINDING_GRAMMAR