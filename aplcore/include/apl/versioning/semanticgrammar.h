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

#ifndef APL_SEMANTIC_GRAMMAR_H
#define APL_SEMANTIC_GRAMMAR_H

#include <tao/pegtl.hpp>

#include "apl/datagrammar/grammarpolyfill.h"

#include "apl/versioning/semanticversion.h"
#include "apl/versioning/semanticpattern.h"

namespace apl {

namespace svgrammar {

/**
 * Grammar for formatting semantic versions and semantic version pattern matches
 *
 * The semantic versioning definition: https://semver.org/
 * The NPM semantic versioning calculator: https://semver.npmjs.com/
 * Semantic versioning cheat sheet: https://devhints.io/semver
 */
namespace pegtl = tao::TAO_PEGTL_NAMESPACE;
using namespace pegtl;

struct req_ws : plus<one<' '>> {};
struct ws : star<one<' '>> {};

struct zero : seq<one<'0'>, not_at<digit>> {};
struct non_zero_number : seq<range<'1', '9'>, star<digit>> {};
struct number : sor<zero, non_zero_number> {};
struct sym_dot : one<'.'> {};
struct sym_plus : one<'+'> {};
struct sym_minus : one<'-'> {};

struct version : must<number, rep_max<2, sym_dot, number>> {};

struct alnum_character : ranges<'a', 'z', 'A', 'Z', '0', '9', '-'> {};
struct alnum_identifier : plus<alnum_character> {};
struct prerelease_identifier : sor<number, alnum_identifier> {};
struct prerelease : opt<sym_minus, list_must<prerelease_identifier, sym_dot>> {};

struct build_identifier : plus<alnum_character> {};
struct build : opt<sym_plus, list_must<build_identifier, sym_dot>> {};

struct basic_semvar : seq<version, prerelease, build> {};
struct semver : must<ws, basic_semvar, ws, eof> {};

// Dependency matching

struct sym_equal : one<'='> {};
struct sym_gt : one<'>'> {};
struct sym_lt : one<'<'> {};
struct sym_ge : string<'>', '='> {};
struct sym_le : string<'<', '='> {};
struct sym_or : string<'|', '|'> {};

struct op : sor<sym_le, sym_ge, sym_equal, sym_lt, sym_gt> {};
struct operand : basic_semvar {};
struct base_pattern : seq<opt<op>, operand> {};
struct and_pattern : list<base_pattern, req_ws> {};
struct or_pattern : list<and_pattern, sym_or, one<' '>> {};
struct pattern : must<ws, or_pattern, ws, eof> {};

// ****** Encode string offset and len into uint32_t ********

const uint32_t kSemanticStringFlag = 0x80000000;
inline bool isEncodedString(uint32_t value) { return (value & kSemanticStringFlag) != 0; }
inline bool numberFits(uint32_t value) { return (value & kSemanticStringFlag) == 0; }
inline uint32_t encodeString(uint8_t offset, uint8_t len) { return offset << 8 | len | kSemanticStringFlag; }
inline uint8_t encodedOffset(uint32_t value) { return ((value >> 8) & 0xff); }
inline uint8_t encodedLen(uint32_t value) { return value & 0xff; }

// ******************** Data Structures *********************

struct semvar_pattern_state : fail_state {
    std::vector<SemanticVersionPtr> versions;
    std::vector<SemanticPattern::OpType> operators;
    SemanticPattern::OpType op = SemanticPattern::OpType::kSemanticOpEquals;  // Default operator
};

struct semvar_state : fail_state {
    template<typename Input>
    explicit semvar_state(const Input& in): start(in.current()) {}

    // Entry point when we're switching from Semantic Pattern to Semantic Version matching
    template<typename Input>
    semvar_state(const Input& in, semvar_pattern_state& /* unused */)
            : semvar_state(in) {}

    // Called when a semantic version has matched and we're switching back to Semantic Pattern matching
    template<typename Input>
    void success(const Input& in, semvar_pattern_state& pattern)
    {
        pattern.versions.emplace_back(
                std::make_shared<SemanticVersion>(std::move(elements), std::string(start, in.current())));
    }

    std::vector<uint32_t> elements;
    std::string string;
    const char *start = nullptr;
};

// ============== Semantic Version Rules ===================

template<typename Rule>
struct sv_action : pegtl::nothing<Rule> {};

template<>
struct sv_action<number> {
    template<typename Input>
    static void apply(const Input& in, semvar_state& state)
    {
        if (state.failed) return;

        auto value = std::stoul(in.string());
        // Check if the number is too large
        if (!numberFits(value))
            state.fail("Numeric value too large", in);
        else
            state.elements.emplace_back(value);
    }
};

template<>
struct sv_action<alnum_identifier> {
    template<typename Input>
    static void apply(const Input& in, semvar_state& state)
    {
        if (state.failed) return;
        // Encode the string offset and length in the lower two bytes and set the "it's a string" flag.
        state.elements.emplace_back(encodeString(std::distance(state.start, in.begin()),
                                                 std::distance(in.begin(), in.end())));
    }
};

template<>
struct sv_action<version> {
    template<typename Input>
    static void apply(const Input& in, semvar_state& state)
    {
        if (state.failed) return;
        // We've finished matching the version. If the minor or patch version wasn't supplied, set them to 0.
        while (state.elements.size() < 3)
            state.elements.emplace_back(0);
    }
};

template<>
struct sv_action<basic_semvar> {
    template<typename Input>
    static void apply(const Input& in, semvar_state& state)
    {
        if (state.failed) return;
        // Store the string containing the actual semantic version (this avoids any whitespace)
        state.string = in.string();
    }
};

// ===================== Semantic Pattern Rules ==============================

template<typename Rule>
struct sp_action : pegtl::nothing<Rule> {
};

// Operand parsing switches to the SemanticVersion rules and state.
template<>
struct sp_action<operand> : change_action_and_state<sv_action, semvar_state> {
    static void apply0(semvar_state& /*unused*/) {}
};

template<>
struct sp_action<base_pattern> {
    template<typename Input>
    static void apply(const Input& in, semvar_pattern_state& state)
    {
        if (state.failed) return;
        state.operators.emplace_back(state.op);
        state.op = SemanticPattern::OpType::kSemanticOpEquals; // Reset the op back to the default.
    }
};

template<>
struct sp_action<sym_or> {
    static void apply0(semvar_pattern_state& state)
    {
        if (state.failed) return;
        state.operators.emplace_back(SemanticPattern::OpType::kSemanticOpOr);
    }
};

template<>
struct sp_action<sym_le> {
    static void apply0(semvar_pattern_state& state)
    {
        if (state.failed) return;
        state.op = SemanticPattern::OpType::kSemanticOpLessThanOrEquals;
    }
};

template<>
struct sp_action<sym_lt> {
    static void apply0(semvar_pattern_state& state)
    {
        if (state.failed) return;
        state.op = SemanticPattern::OpType::kSemanticOpLessThan;
    }
};

template<>
struct sp_action<sym_ge> {
    static void apply0(semvar_pattern_state& state)
    {
        if (state.failed) return;
        state.op = SemanticPattern::OpType::kSemanticOpGreaterThanOrEquals;
    }
};

template<>
struct sp_action<sym_gt> {
    static void apply0(semvar_pattern_state& state)
    {
        if (state.failed) return;
        state.op = SemanticPattern::OpType::kSemanticOpGreaterThan;
    }
};


// ****************** Error messages *******************

template<typename Rule>
struct sv_control : public apl_control<Rule> {
    static const std::string error_message;

    template<typename Input, typename ... States>
    static void raise(const Input& in, States&& ... st)
    {
        apl_control<Rule>::fail(in, st...);
    }
};

} // namespace svgrammar

} // namespace apl

#endif //APL_SEMANTIC_GRAMMAR_H
