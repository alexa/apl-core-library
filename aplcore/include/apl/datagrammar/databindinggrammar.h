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
 * Data-binding grammar: BNF format
 *
 * exp ::= true | false | null | Number | String | dimension | resource |
 *         exp binop exp | unop exp | prefixexp | exp '?' exp ':' exp | '(' exp ')'
 * dimension ::= Number dimunit
 * dimunit ::= dp | px | vh | vw
 * resource ::= '@' Name
 * prefixexp ::= var | prefixexp '(' [explist] ')'
 * var ::= Name | prefixexp '[' exp ']' | prefixexp '.' Name
 * explist ::= {exp ','} exp
 * binop ::= '+' | '-' | '*' | '/' | '%' |
 *           '<' | '>' | '<=' | '>=' | '==' | '!='
 *           '&&' | '||' | '??'
 * unop ::= '+' | '-' | '!'
 */

#ifndef _APL_DATA_BINDING_GRAMMAR_H
#define _APL_DATA_BINDING_GRAMMAR_H

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
struct expression;
struct ds_string;
struct ss_string;

// ******* Symbols *******
struct sym_dbstart : string<'$', '{'> {};
struct sym_dbend : one<'}'> {};
struct sym_question : one<'?'> {};
struct sym_colon : one<':'> {};
struct sym_multiplicative : one<'*', '/', '%'>  {};
struct sym_additive : one<'+', '-' > {};
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

struct sep : space {};
struct ws : star<sep> {};

struct str_false : string<'f', 'a', 'l', 's', 'e'> {};
struct str_null : string<'n', 'u', 'l', 'l'> {};
struct str_true : string<'t', 'r', 'u', 'e'> {};
struct str_dp : string<'d', 'p'> {};
struct str_px : string<'p', 'x'> {};
struct str_vh : string<'v', 'h'> {};
struct str_vw : string<'v', 'w'> {};
struct str_keyword : sor< str_false, str_null, str_true, str_dp, str_px, str_vh, str_vw>{};

template<typename Key>
struct key : seq<Key, not_at< identifier_other >> {};

struct key_false : key<str_false> {};
struct key_null : key<str_null> {};
struct key_true : key<str_true> {};
struct keyword : key<str_keyword> {};

struct zero : if_must<one<'0'>, not_at<digit>> {};
struct number_int : sor<zero, seq< range<'1', '9'>, star<digit>>> {};
struct number     : sor<seq<number_int, sym_decimal, star<digit>>,    // INTEGER . DIGITS*
                        seq<sym_decimal, plus<digit>>,                // . DIGITS+
                        number_int> {};                               // INTEGER

struct symbol : seq< not_at<keyword>, alpha, star<identifier_other> > {};

// Inline Arrays (e.g., [1,2,3])
struct array_comma : one<','> {};
struct array_list : list_must<expression, array_comma, sep> {};
struct array_body : pad_opt<array_list, sep> {};
struct array_start : one<'['> {};
struct array_end : one<']'> {};
struct array : if_must<array_start, array_body, array_end> {};

// Inline maps
struct map_start : one<'{'> {};
struct map_comma : one<','> {};
struct map_assign : one<':'> {};
struct map_end : one<'}'> {};
struct map_element : if_must<sor<ss_string, ds_string>, ws, map_assign, ws, expression> {};
struct map_list : list_must<map_element, map_comma, sep> {};
struct map_body : pad_opt<map_list, sep> {};
struct map : if_must<map_start, map_body, map_end> {};

struct postfix_identifier : identifier {};
struct postfix_attribute : seq<sym_attribute, postfix_identifier> {};
struct postfix_array_access : seq<sym_array_access_start, ws, expression, ws, sym_array_access_end> {};
struct argument_list : list_must<expression, sym_comma, sep> {};
struct postfix_left_paren : one<'('> {};
struct postfix_right_paren : one<')'> {};
struct postfix_function : if_must<postfix_left_paren, pad_opt<argument_list, sep>, postfix_right_paren> {};
struct postfix : sor< postfix_attribute,
                      postfix_array_access,
                      postfix_function >{};
struct plain_symbol : symbol {};
struct resource : seq<one<'@'>, identifier > {};
struct postfix_expression : seq< sor<plain_symbol, resource, array, map>, star< postfix >> {};

// TODO: Can I collapse this so we don't parse numbers twice?
struct dimension : seq<disable<number>, ws, sor<str_dp, str_px, str_vh, str_vw> > {};
struct group_start : one<'('> {};
struct group_end : one<')'> {};
struct grouping : if_must<group_start, ws, expression, ws, group_end > {};

struct factor : sor<
    grouping,
    key_true,
    key_false,
    key_null,
    dimension,
    postfix_expression,
    number,
    ss_string,
    ds_string> {
};

struct unary_expression : seq<star<sym_unary>, factor> {};
struct multiplicative_expression : list_must<unary_expression, sym_multiplicative, sep > {};
struct additive_expression : list_must<multiplicative_expression, sym_additive, sep > {};
struct comparison_expression : list_must<additive_expression, sym_compare, sep> {};
struct equality_expression : list_must< comparison_expression, sym_equal, sep >{};
struct logical_and_expression : list_must< equality_expression, sym_and, sep > {};
struct logical_or_expression : list_must< logical_and_expression, sym_or, sep >{};
struct nullc_expression : list_must< logical_or_expression, sym_nullc, sep >{};
struct ternary_tail : seq< expression, ws, sym_colon, ws, expression >{};
struct ternary_expression : seq< nullc_expression,
                      opt< ws, if_must<sym_question, ws, ternary_tail>>> {};

struct expression : ternary_expression {};

struct db_empty : success {};    // No expression - by default we insert an empty string
struct db_body : pad_opt<sor<expression, db_empty>, sep> {};
struct db : if_must<sym_dbstart, db_body, sym_dbend> {};

// TODO: This assumes UTF-8 encoding.  We're relying on RapidJSON to return UTF-8
struct char_ : utf8::any {};

// Double-quoted string.  E.g.: ${"foo"}
struct sym_double_quote : one<'"'> {};
struct ds_char : char_ {};
struct ds_raw : until<sor<at<sym_double_quote>, at<sym_dbstart> >, must<ds_char> > {};
struct ds_start : sym_double_quote {};
struct ds_end : sym_double_quote {};
struct ds_body : list<ds_raw, db> {};
struct ds_string : if_must<ds_start, ds_body, ds_end> {};

// Single-quoted string.  E.g.: ${'foo'}
struct sym_single_quote : one<'\''> {};
struct ss_char : char_ {};
struct ss_raw : until<sor<at<sym_single_quote>, at<sym_dbstart> >, must<ss_char> > {};
struct ss_start : sym_single_quote {};
struct ss_end : sym_single_quote {};
struct ss_body : list<ss_raw, db> {};
struct ss_string : if_must<ss_start, ss_body, ss_end> {};

// NOTE: Probably can change this to until< at<sym_dbstart>, char_ > {};
// Open-string:  E.g. "this is a ${1+3} generic string"
struct os_raw : until<sor<at<eof>, at<sym_dbstart> >, must<char_> > {};
struct os_start : success {};
struct os_string : seq<os_start, list<os_raw, db>> {};

struct grammar : must<os_string, eof> {};


/**
 * \endcond
 */

} // namespace datagrammar
} // namespace apl

#endif // _APL_DATA_BINDING_GRAMMAR_H