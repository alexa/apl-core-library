/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <pegtl.hh>
#include <pegtl/contrib/abnf.hh>

namespace apl {

namespace easinggrammar {

using namespace pegtl;

static bool DEBUG_GRAMMAR = false;

// This grammar assumes that all space characters have been removed.
struct digits      : plus< abnf::DIGIT > {};
struct floatnum    : sor< seq< opt< one<'-'>>, digits, opt< one<'.'>, star<abnf::DIGIT> > >,
                          seq< opt< one<'-'>>, one<'.'>, digits > > {};
struct args        : list< floatnum, one<','> > {};
struct arglist     : seq< one<'('>, opt<args>, one<')'> >{};
struct path        : seq< string<'p','a','t','h'>, arglist> {};
struct cubicbezier : seq< string<'c','u','b','i','c','-','b','e','z','i','e','r'>, arglist> {};
struct curve       : sor< path, cubicbezier > {};
struct grammar     : must<curve, eof> {};  // Expect a single curve

// ******************** ACTIONS ****************

template<typename Rule>
struct action
    : pegtl::nothing< Rule > {
};

struct easing_state
{
    enum Type { PATH, CUBIC_BEZIER };

    Type type;
    std::vector<float> args;
};

template<> struct action<floatnum>
{
    static void apply(const input& in, easing_state& state) {
        state.args.push_back(std::stof(in.string()));
    }
};

template<> struct action< path >
{
    static void apply(const input& in, easing_state& state) {
        state.type = easing_state::PATH;
    }
};

template<> struct action< cubicbezier >
{
    static void apply(const input& in, easing_state& state) {
        state.type = easing_state::CUBIC_BEZIER;
    }
};

}
} // namespace apl
#endif //_APL_EASING_GRAMMAR_H
