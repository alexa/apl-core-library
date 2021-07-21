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
 * Translated Color Grammar
 */

#ifndef _COLORGRAMMAR_COLOR_GRAMMAR_H
#define _COLORGRAMMAR_COLOR_GRAMMAR_H

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/abnf.hpp>
#include <cstdint>
#include <stack>
#include <algorithm>

#include "apl/primitives/color.h"
#include "colorfunctions.h"

#include "apl/utils/log.h"

namespace apl {
namespace colorgrammar {
    namespace pegtl = tao::TAO_PEGTL_NAMESPACE;
    using namespace pegtl;

    static bool DEBUG_GRAMMAR = false;

    /**
     * \cond ShowColorGrammar
     */
    struct basecolor;   // Forward declaration

    struct ws          : star< space > {};
    struct comma       : seq< ws, one<','>, ws > {};
    struct namedcolor  : plus< alpha > {};
    struct hexidecimal : seq< one< '#' >, plus< xdigit > > {};
    struct digits      : plus< abnf::DIGIT > {};
    struct floatnum    : sor< seq< digits, opt< one<'.'>, star<abnf::DIGIT> > >,
                              seq< one<'.'>, digits > > {};
    struct number      : seq< floatnum, opt< one<'%'> > > {};   // Not quite right - what about 004?
    struct firstarg    : sor< basecolor, number > {};
    struct args        : seq< firstarg, star<comma, number> > {};
    struct arglist     : seq< one<'('>, ws, args, ws, one<')'> > {};
    struct rgb         : seq< string< 'r', 'g', 'b' >, star< one<'a'> >, arglist > {};
    struct hsl         : seq< string< 'h', 's', 'l' >, star< one<'a'> >, arglist > {};
    struct basecolor   : sor< hexidecimal, hsl, rgb, namedcolor > {};
    struct grammar     : must< ws, basecolor, ws, eof > {};  // Expect a single color and the EOF

    // ******************** ACTIONS *********************

    template<typename Rule>
    struct action
        : pegtl::nothing< Rule > {
        };

    struct color_state
    {
        std::stack<double> mStack;

        void push(double value) { mStack.push(value); }
        double pop() { double v = mStack.top(); mStack.pop(); return v; }
        uint32_t getColor() {return (uint32_t) mStack.top();}
    };

    /*
     * The first argument to a function call pushes a sentinel on the stack (the value -1).
     */
    template<> struct action< firstarg >
    {
        template< typename Input >
        static void apply(const Input& in, color_state& state) {
            LOGF_IF(DEBUG_GRAMMAR, "Firstarg: '%s'", in.string().c_str());
            state.push(-1);
        }
    };

    /*
     * Hexidecimal numbers are of the form #RGB, #RGBA, #RRGGBB, and #RRGGBBAA.
     */
    template<> struct action< hexidecimal >
    {
        template< typename Input >
        static void apply( const Input& in, color_state& state ) {
            uint32_t color;

            if (!colorFromHex(in.string(), color))
                throw parse_error("Invalid hexidecimal color", in);

            LOGF_IF(DEBUG_GRAMMAR, "Hexidecimal: '%s' -> %08x", in.string().c_str(), color);
            state.push(color);
        }
    };

    /*
     * Sample numbers:  0, 1, 2.4, 20%.
     * Negative numbers are not supported by the parser.
     */
    template<> struct action< number >
    {
        template< typename Input >
        static void apply( const Input& in, color_state& state) {
            std::string s(in.string());

            double value;
            if (s.back() == '%') {
                size_t len = s.length();
                value = stod(s.substr(0, len - 1)) * 0.01;
            } else {
                value = stod(s);
            }

            LOGF_IF(DEBUG_GRAMMAR, "Number: '%s' -> %lf", in.string().c_str(), value);
            state.push(value);
        }
    };

    /**
     * Named colors come from colormap.h
     */
    template<> struct action< namedcolor >
    {
        template< typename Input >
        static void apply(const Input& in, color_state& state) {
            auto result = Color::lookup(in.string());
            if (!result.first) {
                throw parse_error((std::string("Invalid named color: '") + in.string()).c_str() + std::string("'"), in);
            }

            LOGF_IF(DEBUG_GRAMMAR, "Color map: '%s'", in.string().c_str());
            state.push(result.second);
        }
    };

    /**
     * The hsl(a) method supports three forms:
     *
     *  hsl(hue, saturation, lightness)
     *  hsla(hue, saturation, lightness, alpha)
     *
     *  where hue in the range [0,360], saturation, lightness, and alpha in [0,1]
     */
    template<> struct action< hsl >
    {
        template< typename Input >
        static void apply( const Input& in, color_state& state) {
            double v;
            double args[4];
            int argc = 0;

            while ((v = state.pop()) != -1) {
                if (argc >= 3)
                    throw parse_error("too many arguments in an hsl function", in);
                args[argc++] = v;
            }

            // Grab the last argument (it was behind the sentinel)
            args[argc++] = state.pop();

            if (argc < 3)
                throw parse_error("expected at least three arguments for an hsl function", in);

            if (argc == 3)
                state.push(colorFromHSL(args[2], args[1], args[0]));
            else
                state.push(colorFromHSLA(args[3], args[2], args[1], args[0]));
        }
    };

    /*
     * The rgb(a) method supports three forms:
     *
     *  rgb(color, percentage)          where color is a valid color and percentage is in [0,1]
     *  rgb(red, green, blue)           where red, green, and blue are in [0, 255]
     *  rgba(red, green, blue, alpha)   where red, green, blue are in [0,255] and alpha in [0,1]
     */
    template<> struct action< rgb >
    {
        template< typename Input >
        static void apply( const Input& in, color_state& state) {
            double v;
            double args[4];
            int argc = 0;

            while ((v = state.pop()) != -1) {
                if (argc >= 3)
                    throw parse_error("too many arguments in a color function", in);
                args[argc++] = v;
            }

            // Grab the last argument (it was behind the sentinel)
            args[argc++] = state.pop();

            if (argc < 2)
                throw parse_error("expected at least two arguments for a color function", in);

            if (argc == 2)
                state.push(applyAlpha(args[1], args[0]));
            else if (argc == 3)
                state.push(colorFromRGB(args[2], args[1], args[0]));
            else
                state.push(colorFromRGBA(args[3], args[2], args[1], args[0]));
        }
    };

    // ****************** Error messages *******************

    template< typename Rule >
    struct errors
        : public pegtl::normal< Rule >
    {
        static const std::string error_message;

        template< typename Input, typename ... States >
        static void raise( const Input & in, States && ... )
        {
            throw pegtl::parse_error( error_message, in );
        }
    };

    template<> const std::string errors< digits >::error_message = "expected at least one digit";
    template<> const std::string errors< pegtl::eof >::error_message = "unexpected end";

    /**
     * \endcond
     */


} // namespace colorgrammar

} // namespace apl

#endif // _COLORGRAMMAR_COLOR_GRAMMAR_H