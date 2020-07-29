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

template<typename R, typename P = ws>
struct padr : internal::seq<R, internal::star<P> > {};

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

struct intnum : plus<digit> {};
struct floatnum : sor<seq<plus<digit>, disable<one<'.'> >, star<digit> >,
    seq<disable<one<'.'> >, plus<digit> > > {};
struct true_ : string<'t', 'r', 'u', 'e'> {};
struct false_ : string<'f', 'a', 'l', 's', 'e'> {};
struct null_ : string<'n', 'u', 'l', 'l'> {};
struct symbol : seq<alpha, star<identifier_other> > {};

struct attr_dot : seq<one<'.'>, identifier> {};
struct attr_bracket : seq<one<'['>, ws, ternary, ws, one<']'> > {};
struct arglist : list<ternary, sym_comma, space> {};
struct func_start : one<'('> {};
struct func_call : sor<if_must<func_start, ws, opt<arglist, ws>, one<')'> > > {};
struct resource : seq<one<'@'>, disable<identifier> > {};
struct attribute : seq<
    sor<resource, symbol>,
    star<sor<attr_dot, attr_bracket> >,
    opt<func_call> > {
};

struct dp : string<'d', 'p'> {};
struct px : string<'p', 'x'> {};
struct vh : string<'v', 'w'> {};
struct wh : string<'v', 'h'> {};
struct dimension : seq<disable<sor<floatnum, intnum> >, ws, sor<dp, px, vh, wh> > {};
struct grouping : if_must<one<'('>, ws, ternary, ws, one<')'> > {};

struct factor : sor<grouping,
    true_,
    false_,
    null_,
    dimension,
    attribute,
    floatnum,
    intnum,
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