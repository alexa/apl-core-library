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
#ifndef _APL_CHARACTERRANGEGRAMMAR_H
#define _APL_CHARACTERRANGEGRAMMAR_H

#include <iostream>
#include <vector>

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/abnf.hpp>
#include <codecvt>
#include <locale>

#include "apl/primitives/characterrange.h"
#include "apl/utils/log.h"

/**
 * Character converter to be used for multi-byte sized characters.
 */
std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> wchar_converter;

namespace apl {
namespace character_range_grammar {

    namespace pegtl = tao::TAO_PEGTL_NAMESPACE;
    using namespace pegtl;

    struct firstTerm : utf8::not_one<L'-'> {};
    struct dash : utf8::one<L'-'> {};
    struct leadingDash : utf8::one<L'-'> {};
    struct secondTerm : utf8::not_one<L'-'> {};
    struct rangeExpression : seq<firstTerm, dash, secondTerm> {};
    struct singleCharTerm : utf8::not_one<L'-'> {};
    struct grammar : must< opt<leadingDash>, plus<sor<rangeExpression, singleCharTerm>>, eof > {};

    // ******************** ACTIONS *********************
    template<typename Rule>
    struct action
            : pegtl::nothing<Rule> {
    };

    // state struct
    struct character_range_state
    {
        std::vector<CharacterRangeData> mRanges;
        wchar_t firstTerm;

        void push(CharacterRangeData value) { mRanges.push_back(value); }
        const std::vector<CharacterRangeData> getRanges() const { return mRanges; }
    };

    inline void pushToRangeState(character_range_state& state, wchar_t first, wchar_t second)
    {
        CharacterRangeData range(first, second);
        state.push(range);
    }

    /*
     * When we parse the first character in the range expression we update our state
     */
    template<> struct action< firstTerm >
    {
        template< typename Input >
        static void apply(const Input& in, character_range_state& state) {
            wchar_t rangeChar = wchar_converter.from_bytes(in.string().data()).at(0);
            state.firstTerm = rangeChar;
        }
    };

    /*
     * When we parse the second character in the range expression create a CharacterRangeData and push it
     */
    template<> struct action< secondTerm >
    {
        template< typename Input >
        static void apply(const Input& in, character_range_state& state) {
            wchar_t rangeChar = wchar_converter.from_bytes(in.string().data()).at(0);
            pushToRangeState(state, state.firstTerm, rangeChar);
        }
    };

    /*
     * When we parse single character in the expression create a CharacterRangeData with single(same) value and push it
     */
    template<> struct action< singleCharTerm >
    {
        template< typename Input >
        static void apply(const Input& in, character_range_state& state) {
            wchar_t rangeChar = wchar_converter.from_bytes(in.string().data()).at(0);
            pushToRangeState(state, rangeChar, rangeChar);
        }
    };

    /*
     * When we parse starting dash in the expression create a CharacterRangeData with single(same) value and push it
     */
    template<> struct action< leadingDash >
    {
        template< typename Input >
        static void apply(const Input& in, character_range_state& state) {
            wchar_t rangeChar = wchar_converter.from_bytes(in.string().data()).at(0);
            pushToRangeState(state, rangeChar, rangeChar);
        }
    };
} // namespace character_range_grammar
} // namespace apl
#endif //_APL_CHARACTERRANGEGRAMMAR_H
