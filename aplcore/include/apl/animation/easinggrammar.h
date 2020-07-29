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

#ifndef _APL_EASING_GRAMMAR_H
#define _APL_EASING_GRAMMAR_H

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/abnf.hpp>

#include "apl/animation/coreeasing.h"
#include "apl/utils/log.h"

namespace apl {

namespace easinggrammar {

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;
using namespace pegtl;

/**
 * This grammar assumes that all space characters have been removed.
 *
 * cubic-bezier(a,b,c,d)
 * path(x,y,....)
 * (line(t,v) | curve(t,v,a,b,c,d))* end(t,v)
 *
 * spatial(n, i) scurve(t, v1...vn, to1...ton, ti1...tin, a, b, c, d )+ send(t, v1...vn)
 */

struct str_path        : string<'p','a','t','h'> {};
struct str_cubicbezier : string<'c','u','b','i','c','-','b','e','z','i','e','r'> {};

struct str_end         : string<'e', 'n', 'd'> {};
struct str_line        : string<'l', 'i', 'n', 'e'> {};
struct str_curve       : string<'c', 'u', 'r', 'v', 'e'> {};

struct str_scurve      : string<'s', 'c', 'u', 'r', 'v', 'e'> {};
struct str_send        : string<'s', 'e', 'n', 'd'> {};
struct str_spatial     : string<'s', 'p', 'a', 't', 'i', 'a', 'l'> {};

struct digits      : plus< abnf::DIGIT > {};
struct floatnum    : sor< seq< opt< one<'-'>>, digits, opt< one<'.'>, star<abnf::DIGIT> > >,
                          seq< opt< one<'-'>>, one<'.'>, digits > > {};
struct args        : list< floatnum, one<','> > {};
struct arglist     : seq< one<'('>, opt<args>, one<')'> >{};
struct path        : if_must< str_path, arglist> {};
struct cubicbezier : if_must< str_cubicbezier, arglist> {};
struct end         : if_must< str_end, arglist > {};
struct line        : if_must< str_line, arglist> {};
struct curve       : if_must< str_curve, arglist> {};
struct segment     : seq< plus< sor<line, curve> >, end> {};

struct scurve      : if_must< str_scurve, arglist> {};
struct send        : if_must< str_send, arglist> {};
struct spatial     : if_must< str_spatial, arglist> {};
struct psegment    : if_must< spatial, plus< scurve>, send> {};

struct easing      : sor< path, cubicbezier, segment, psegment > {};

// Using namespace pegtl,
// because of this collision in Windows:
// error C2872: 'eof': ambiguous symbol
struct grammar     : must<easing, pegtl::eof> {}; // Expect a single curve.

// ******************** ACTIONS ****************

template<typename Rule>
struct action
    : nothing< Rule > {
};

struct easing_state
{
    float lastTime = 0;
    size_t startIndex = 0;
    std::vector<EasingSegment> segments;
    std::vector<float> args;
};

template<> struct action<floatnum>
{
    template< typename Input >
    static void apply(const Input& in, easing_state& state) {
        state.args.push_back(std::stof(in.string()));
    }
};

template<> struct action< str_path >
{
    template< typename Input >
    static void apply(const Input& in, easing_state& state) {
        state.segments.emplace_back(EasingSegment(kLinearSegment, state.startIndex));
        state.args.emplace_back(0);
        state.args.emplace_back(0);
        state.startIndex = state.args.size();
    }
};

template<> struct action< path >
{
    template< typename Input >
    static void apply(const Input& in, easing_state& state) {
        auto argCount = state.args.size() - state.startIndex;
        if (argCount % 2 == 1)
            throw parse_error("Path easing function needs an even number of arguments", in);

        // Push each linear segment.  Check to ensure time is incrementing
        for (auto offset = state.startIndex ; offset < state.args.size() ; offset += 2) {
            auto time = state.args.at(offset);
            if (time <= state.lastTime || time >= 1)
                throw parse_error("Path easing function needs ordered array of segments", in);
            state.lastTime = time;
            state.segments.emplace_back(EasingSegment(kLinearSegment, offset));
        }

        // Add a final segment at (1,1)
        state.segments.emplace_back(EasingSegment(kEndSegment, state.args.size()));
        state.args.emplace_back(1);  // Time=1
        state.args.emplace_back(1);  // Value=1
    }
};

template<> struct action< str_cubicbezier >
{
    template< typename Input >
    static void apply(const Input& in, easing_state& state) {
        state.segments.emplace_back(EasingSegment(kCurveSegment, state.startIndex));
        state.args.emplace_back(0);
        state.args.emplace_back(0);
        state.startIndex = state.args.size();
    }
};

template<> struct action< cubicbezier >
{
    template< typename Input >
    static void apply(const Input& in, easing_state& state) {
        auto argCount = state.args.size() - state.startIndex;
        if (argCount != 4)
            throw parse_error("Cubic-bezier easing function requires 4 arguments", in);

        // Add a final segment at (1,1)
        state.segments.emplace_back(EasingSegment(kEndSegment, state.args.size()));
        state.args.emplace_back(1);  // Time=1
        state.args.emplace_back(1);  // Value=1
    }
};

template<> struct action< str_end >
{
    template< typename Input >
    static void apply(const Input& in, easing_state& state) {
        state.startIndex = state.args.size();
    }
};

template<> struct action< end >
{
    template< typename Input >
    static void apply(const Input& in, easing_state& state) {
        auto argCount = state.args.size() - state.startIndex;
        if (argCount != 2)
            throw parse_error("End easing function segment requires 2 arguments", in);

        auto time = state.args.at(state.startIndex);
        if (time <= state.lastTime && state.startIndex > 0)
            throw parse_error("End easing function segment cannot start at this time", in);

        state.lastTime = time;
        state.segments.emplace_back(EasingSegment(kEndSegment, state.startIndex));
    }
};

template<> struct action< str_line >
{
    template< typename Input >
    static void apply(const Input& in, easing_state& state) {
        state.startIndex = state.args.size();
    }
};

template<> struct action< line >
{
    template< typename Input >
    static void apply(const Input& in, easing_state& state) {
        auto argCount = state.args.size() - state.startIndex;
        if (argCount != 2)
            throw parse_error("Line easing function segment requires 2 arguments", in);

        auto time = state.args.at(state.startIndex);
        if (time <= state.lastTime && state.startIndex > 0)
            throw parse_error("Line easing function segment cannot start at this time", in);

        state.lastTime = time;
        state.segments.emplace_back(EasingSegment(kLinearSegment, state.startIndex));
    }
};

template<> struct action< str_curve >
{
    template< typename Input >
    static void apply(const Input& in, easing_state& state) {
        state.startIndex = state.args.size();
    }
};

template<> struct action< curve >
{
    template< typename Input >
    static void apply(const Input& in, easing_state& state) {
        auto argCount = state.args.size() - state.startIndex;
        if (argCount != 6)
            throw parse_error("Curve easing function segment requires 6 arguments", in);

        auto time = state.args.at(state.startIndex);
        if (time <= state.lastTime && state.startIndex > 0)
            throw parse_error("Curve easing function segment cannot start at this time", in);

        state.lastTime = time;
        state.segments.emplace_back(EasingSegment(kCurveSegment, state.startIndex));
    }
};


template<> struct action< spatial >
{
    template< typename Input >
    static void apply(const Input& in, easing_state& state) {
        assert(state.startIndex == 0);

        auto argCount = state.args.size();
        if (argCount != 2)
            throw parse_error("Wrong number of arguments to spatial", in);

        auto dof = static_cast<int>(state.args.at(0));
        if (dof < 2)
            throw parse_error("invalid number of indices in spatial segment", in);

        auto index = static_cast<int>(state.args.at(1));
        if (index < 0 || index >= dof)
            throw parse_error("select index out of range in spatial segment", in);
    }
};

template<> struct action< str_send >
{
    template< typename Input >
    static void apply(const Input& in, easing_state& state) {
        state.startIndex = state.args.size();
    }
};

template<> struct action< send >
{
    template< typename Input >
    static void apply(const Input& in, easing_state& state) {
        auto argCount = state.args.size() - state.startIndex;

        // Time, pcount for value
        auto dof = ::abs(static_cast<int>(state.args[0]));
        if (argCount != 1 + dof)
            throw parse_error("Wrong number of arguments to send", in);

        auto time = state.args.at(state.startIndex);
        if (time <= state.lastTime && !state.segments.empty())
            throw parse_error("send easing function segment cannot start at this time", in);

        state.lastTime = time;
        state.segments.emplace_back(EasingSegment(kSEndSegment, state.startIndex));
    }
};

template<> struct action< str_scurve >
{
    template< typename Input >
    static void apply(const Input& in, easing_state& state) {
        state.startIndex = state.args.size();
    }
};

template<> struct action< scurve >
{
    template< typename Input >
    static void apply(const Input& in, easing_state& state) {
        auto argCount = state.args.size() - state.startIndex;

        // Time, pcount * 3 for value, tin, tout, 4 for the time curve =
        auto dof = ::abs(static_cast<int>(state.args[0]));
        if (argCount != 5 + dof * 3)
            throw parse_error("Wrong number of arguments to scurve", in);

        auto time = state.args.at(state.startIndex);
        if (time <= state.lastTime && !state.segments.empty())
            throw parse_error("scurve easing function segment cannot start at this time", in);

        state.lastTime = time;
        state.segments.emplace_back(EasingSegment(kSCurveSegment, state.startIndex));
    }
};
}
} // namespace apl
#endif //_APL_EASING_GRAMMAR_H
