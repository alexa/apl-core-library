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

#ifndef _APL_DATA_BINDING_ERRORS_H
#define _APL_DATA_BINDING_ERRORS_H

#include <string>

#include "grammarerror.h"
#include "databindingrules.h"

namespace apl {
namespace datagrammar {

/**
 * Standard PEGTL parsing error controller.  This raises a parse_error exception
 * if a problem occurs.  The static "error_value" defined in this template converts
 * from a templated action to a numbered error message.  The "errorToString" method
 * further converts the error message into a human-readable string.
 * @tparam Rule
 */
template< typename Rule >
struct error_control : tao::pegtl::normal< Rule > {
    static const GrammarError error_value;

    template<typename Input, typename... States>
    static void raise(const Input &in, States &&...) {
        throw tao::pegtl::parse_error(errorToString(error_value), in);
    }
};

/**
 * Convenience routine for printing out the current character being processed
 * by the PEGTL grammar.
 * @tparam Input
 * @param in
 * @return A string showing the character (if printable) and the numeric value of the character.
 */
template< typename Input > std::string
get_current(const Input& in)
{
    streamer out;

    if( in.empty() ) {
        out << "<eof>";
    }
    else {
        const auto c = in.peek_uint8();
        switch( c ) {
            case 0:
                out << "<nul> = ";
                break;
            case 9:
                out << "<ht> = ";
                break;
            case 10:
                out << "<lf> = ";
                break;
            case 13:
                out << "<cr> = ";
                break;
            default:
                if( isprint( c ) ) {
                    out << "'" << (char) c << "' = ";
                }
        }
        out << "(char)" << unsigned( c );
    }

    return out.str();
}

// These are only enabled if DEBUG_DATA_BINDING is true
const bool TRACED_ERROR_CONTROL_SHOW_START = false;   // Log starting blocks
const bool TRACED_ERROR_CONTROL_SHOW_SUCCESS = false; // Log successful blocks
const bool TRACED_ERROR_CONTROL_SHOW_FAILURE = false; // Log failed blocks

/**
 * Fancing PEGTL parsing error controller.  This is enabled with DEBUG_DATA_BINDING.
 * The messages are output as the PEGTL grammar is parsed.
 * @tparam Rule
 */
template< typename Rule >
struct traced_error_control : error_control<Rule>
{
    template<typename Input, typename... States>
    static void start(const Input& in, States&& ... /*unused*/ ) {
        LOG_IF(TRACED_ERROR_CONTROL_SHOW_START) << pegtl::to_string(in.position())
                                  << "  start  " << pegtl::internal::demangle<Rule>()
                                  << "; current " << get_current(in);
    }

    template<typename Input, typename... States>
    static void success(const Input& in, States&& ... /*unused*/ ) {
        LOG_IF(TRACED_ERROR_CONTROL_SHOW_SUCCESS) << pegtl::to_string(in.position())
                                    << " success " << pegtl::internal::demangle<Rule>()
                                    << "; next " << get_current(in);
    }

    template<typename Input, typename... States>
    static void failure(const Input& in, States&& ... /*unused*/ ) {
        LOG_IF(TRACED_ERROR_CONTROL_SHOW_FAILURE) << pegtl::to_string(in.position())
                                    << " failure " << pegtl::internal::demangle<Rule>();
    }

    template<template<typename...> class Action, typename Iterator, typename Input, typename... States>
    static auto apply(const Iterator& begin, const Input& in, States&& ... st)
    -> decltype(pegtl::normal<Rule>::template apply<Action>(begin, in, st...)) {
        LOG(LogLevel::kDebug) << pegtl::to_string(in.position())
                             << "  apply  " << pegtl::internal::demangle<Rule>()
                             << " '" << std::string(in.begin() + begin.byte, in.current())
                             << "' position=" << begin.byte;
        return pegtl::normal<Rule>::template apply<Action>(begin, in, st...);
    }

    template<template<typename...> class Action, typename Input, typename... States>
    static auto apply0(const Input& in, States&& ... st)
    -> decltype(pegtl::normal<Rule>::template apply0<Action>(in, st...)) {
        LOG(LogLevel::kDebug) << pegtl::to_string(in.position())
                             << "  apply  " << pegtl::internal::demangle<Rule>()
                             << " '" << std::string(in.begin() + in.byte, in.current())
                             << "' position=" << in.byte;
        return pegtl::normal<Rule>::template apply0<Action>(in, st...);
    }
};

template<> const GrammarError error_control<not_at<digit>>::error_value = INVALID_NUMBER_FORMAT;
template<> const GrammarError error_control<sym_dbend>::error_value = UNEXPECTED_TOKEN;
template<> const GrammarError error_control<eof>::error_value = UNEXPECTED_TOKEN_BEFORE_EOF;

template<> const GrammarError error_control<unary_expression>::error_value = EXPECTED_OPERAND_AFTER_MULTIPLICATIVE;
template<> const GrammarError error_control<multiplicative_expression>::error_value = EXPECTED_OPERAND_AFTER_ADDITIVE;
template<> const GrammarError error_control<additive_expression>::error_value = EXPECTED_OPERAND_AFTER_COMPARISON;
template<> const GrammarError error_control<comparison_expression>::error_value = EXPECTED_OPERAND_AFTER_EQUALITY;
template<> const GrammarError error_control<equality_expression>::error_value = EXPECTED_OPERAND_AFTER_LOGICAL_AND;
template<> const GrammarError error_control<logical_and_expression>::error_value = EXPECTED_OPERAND_AFTER_LOGICAL_OR;
template<> const GrammarError error_control<logical_or_expression>::error_value = EXPECTED_OPERAND_AFTER_NULLC;

template<> const GrammarError error_control<expression>::error_value = EXPECTED_EXPRESSION;

template<> const GrammarError error_control<array_end>::error_value = MALFORMED_ARRAY;
template<> const GrammarError error_control<ss_char>::error_value = UNTERMINATED_SS_STRING;
template<> const GrammarError error_control<ds_char>::error_value = UNTERMINATED_DS_STRING;
template<> const GrammarError error_control<map_assign>::error_value = EXPECTED_MAP_VALUE_ASSIGNMENT;
template<> const GrammarError error_control<map_element>::error_value = EXPECTED_MAP_ASSIGNMENT;
template<> const GrammarError error_control<map_end>::error_value = MALFORMED_MAP;
template<> const GrammarError error_control<ternary_tail>::error_value = MALFORMED_TERNARY_EXPRESSION;
template<> const GrammarError error_control<postfix_right_paren>::error_value = EXPECTED_POSTFIX_RIGHT_PAREN;

// Untested items

template<> const GrammarError error_control<char_>::error_value = static_cast<GrammarError>(105);
template<> const GrammarError error_control<ws>::error_value = static_cast<GrammarError>(106);
template<> const GrammarError error_control<postfix_expression>::error_value = static_cast<GrammarError>(107);
template<> const GrammarError error_control<ternary_expression>::error_value = static_cast<GrammarError>(111);
template<> const GrammarError error_control<db_body>::error_value = static_cast<GrammarError>(117);
template<> const GrammarError error_control<map_body>::error_value = static_cast<GrammarError>(118);
template<> const GrammarError error_control<array_body>::error_value = static_cast<GrammarError>(119);
template<> const GrammarError error_control<ds_end>::error_value = static_cast<GrammarError>(120);
template<> const GrammarError error_control<ss_end>::error_value = static_cast<GrammarError>(121);
template<> const GrammarError error_control<ss_body>::error_value = static_cast<GrammarError>(122);
template<> const GrammarError error_control<ds_body>::error_value = static_cast<GrammarError>(123);
template<> const GrammarError error_control<group_end>::error_value = static_cast<GrammarError>(127);
template<> const GrammarError error_control<os_string>::error_value = static_cast<GrammarError>(131);
template<> const GrammarError error_control<pad_opt<argument_list, sep>>::error_value = static_cast<GrammarError>(141);

template<> const GrammarError error_control<one<(char)93>>::error_value = static_cast<GrammarError>(204);

} // namespace datagrammar
} // namespace apl

#endif // _APL_DATA_BINDING_ERRORS_H
