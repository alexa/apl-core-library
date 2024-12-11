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

#include "apl/primitives/transformgrammar.h"

#include <tao/pegtl.hpp>

#include "apl/datagrammar/grammarpolyfill.h"
#include "apl/utils/session.h"

namespace apl {
namespace t2grammar {

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;
using namespace pegtl;

static const bool DEBUG_GRAMMAR = false;

/**
 * \cond Show t2grammar
 *
 * This grammar parses AVG-style transform expressions.
 * Sample expressions in the grammar include:
 *     translate(10, 0)            --> Translate 10 units to the right
 *     rotate(45, +5, -5)          --> Rotate 45 degrees clockwise about the point (5,-5)
 *     scale(10)                   --> Scale uniformly by a factor of 5
 *     rotate(90) translate(10)    --> Translate 10 units to the right, then rotate 90 degrees clockwise about (0,0)
 */

struct ws         : star<space> {};                    // White space
struct cws        : pad_opt< one<','>, space > {};     // White space with an optional comma

// Note: Use the <number-token> definition from https://www.w3.org/TR/css-syntax-3/#typedef-number-token
struct floatnum   : sor< seq<plus<digit>, opt_must<one<'.'>, star<digit>> >,
                      seq<one<'.'>, plus<digit> >> {};
struct prefix     : sor<one<'+'>,one<'-'>, success> {};
struct postfix    : opt_must<sor<one<'e'>,one<'E'>>, prefix, plus<digit>> {};
struct number     : seq<prefix, floatnum, postfix> {};

// Note: The comma-separated whitespace definition matches the CSS definition.
struct arglist11  : must<one<'('>, ws, number, ws, one<')'>> {};
struct arglist12  : must<one<'('>, ws, number, opt<seq<cws, number>>, ws, one<')'>> {};
struct arglist13  : must<one<'('>, ws, number, opt<seq<cws, number, cws, number>>, ws, one<')'>> {};

struct rotate     : if_must<string<'r','o','t','a','t','e'>, ws, arglist13> {};
struct translate  : if_must<string<'t','r','a','n','s','l','a','t','e'>, ws, arglist12> {};
struct scale      : if_must<string<'s','c','a','l','e'>, ws, arglist12> {};
struct skewX      : if_must<string<'s','k','e','w','X'>, ws, arglist11> {};
struct skewY      : if_must<string<'s','k','e','w','Y'>, ws, arglist11> {};
struct transform  : sor<rotate, translate, scale, skewX, skewY> {};
struct transforms : list<ws, transform> {};
struct grammar    : must<transforms, eof> {};

// **************** Error reporting ***************

template<typename Rule>
struct t2_control : apl_control< Rule > {
    static const std::string error_message;

    template<typename Input, typename...States >
    static void raise( const Input& in, States&&... st) {
        apl_control<Rule>::fail(in, st...);
    }
};

template<> const std::string t2_control<eof>::error_message = "Unrecognized transformation";
template<> const std::string t2_control<one<')'>>::error_message = "Expected a right parenthesis ')'";
template<> const std::string t2_control<one<'('>>::error_message = "Expected a left parenthesis '('";
template<> const std::string t2_control<number>::error_message = "Expected a number";
template<> const std::string t2_control<plus<digit>>::error_message = "Expected a digit";

// This template will catch any other parsing errors
template<typename T> const std::string t2_control<T>::error_message = "parse error matching " + pegtl::internal::demangle<T>();

// **************** Actions ***********/

template<typename Rule>
struct action : pegtl::nothing< Rule > {
};

struct transform_state : fail_state {
    transform_state(TransformationAccumulator& accum) : accumulator(accum) {}

    void push(double value) {
        args[arg_count++] = value;
    }

    std::string argsToString() {
        std::string result;
        for (int i = 0 ; i < arg_count ; i++) {
            if (i > 0)
                result += " ";
            result += sutil::to_string(args[i]);
        }
        return result;
    }

    void rotate() {
        LOGF_IF(DEBUG_GRAMMAR, "Rotate %s", argsToString().c_str());
        assert(arg_count == 1 || arg_count == 3);
        if (arg_count == 3)
            accumulator.add(Transform::translate(args[1], args[2]));

        accumulator.add(Transform::rotate(args[0]));
        if (arg_count == 3)
            accumulator.add(Transform::translate(-args[1], -args[2]));
        arg_count = 0;
    }

    void translate() {
        LOGF_IF(DEBUG_GRAMMAR, "Translate %s", argsToString().c_str());
        assert(arg_count == 1 || arg_count == 2);
        double tx = args[0];
        double ty = arg_count == 1 ? 0 : args[1];
        accumulator.add(Transform::translate(tx, ty));
        arg_count = 0;
    }

    void scale() {
        LOGF_IF(DEBUG_GRAMMAR, "Scale %s", argsToString().c_str());
        assert(arg_count == 1 || arg_count == 2);
        double sx = args[0];
        double sy = arg_count == 1 ? sx : args[1];
        accumulator.add(Transform::scale(sx, sy));
        arg_count = 0;
    }

    void skewX() {
        LOGF_IF(DEBUG_GRAMMAR, "Skew X %s", argsToString().c_str());
        assert(arg_count == 1);
        accumulator.add(Transform::skewX(args[0]));
        arg_count = 0;
    }

    void skewY() {
        LOGF_IF(DEBUG_GRAMMAR, "Skew Y %s", argsToString().c_str());
        assert(arg_count == 1);
        accumulator.add(Transform::skewY(args[0]));
        arg_count = 0;
    }

    TransformationAccumulator& accumulator;
    int arg_count = 0;
    double args[3];
};

/*
 * Sample numbers:  0, 1, -2.4
 */
template<> struct action< number >
{
    template< typename Input >
    static void apply( const Input& in, transform_state& state) {
        std::string s(in.string());
        double value = sutil::stod(s);

        LOGF_IF(DEBUG_GRAMMAR, "Number: '%s' -> %lf", in.string().c_str(), value);
        state.push(value);
    }
};

template<> struct action< rotate >
{
    template< typename Input >
    static void apply( const Input& in, transform_state& state) {
        state.rotate();
    }
};

template<> struct action< translate >
{
    template< typename Input >
    static void apply( const Input& in, transform_state& state) {
        state.translate();
    }
};

template<> struct action< scale >
{
    template< typename Input >
    static void apply( const Input& in, transform_state& state) {
        state.scale();
    }
};

template<> struct action< skewX >
{
    template< typename Input >
    static void apply( const Input& in, transform_state& state) {
        state.skewX();
    }
};

template<> struct action< skewY >
{
    template< typename Input >
    static void apply( const Input& in, transform_state& state) {
        state.skewY();
    }
};

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;

bool
parse(const SessionPtr& session, const std::string& transform, TransformationAccumulator& out)
{
    t2grammar::transform_state state(out);
    pegtl::string_input<> in(transform, "");

    if (!pegtl::parse<t2grammar::grammar, t2grammar::action, t2grammar::t2_control>(in, state) || state.failed) {
        CONSOLE(session) << "Error parsing transform '" << transform << "'" << state.what();
    } else {
        return true;
    }

    return false;
}

} // namespace t2grammar
} // namespace apl