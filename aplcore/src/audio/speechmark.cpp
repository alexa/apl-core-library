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

#include "apl/audio/speechmark.h"

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/abnf.hpp>
#include <tao/pegtl/contrib/unescape.hpp>

namespace apl {
namespace pollygrammar {

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;
using namespace pegtl;

struct quote : one<'"'> {};
struct sym_start : one<'{'> {};
struct sym_end : one<'}'> {};
struct sym_colon : one<':'> {};
struct sym_comma : one<','> {};

struct ws : one<' ', '\t', '\n', '\r'> {};
struct ews : one<'[', ']', ',', ' ', '\t', '\n', '\r'> {};
struct sep : star<ws> {};

// Support JSON Unicode encodings
struct xdigit : abnf::HEXDIG {};
struct unicode : list<seq<one<'u'>, rep<4, must<xdigit>>>, one<'\\'>> {};
struct escaped_char : one<'"', '\\', '/', 'b', 'f', 'n', 'r', 't'> {};
struct escaped : sor<escaped_char, unicode> {};
struct unescaped : utf8::range<0x20, 0x10FFFF> {};
struct char_ : if_then_else<one<'\\'>, must<escaped>, unescaped> {}; // NOLINT

struct number : plus<abnf::DIGIT> {};

struct string_content : until<at<quote>, must<char_>> {};
struct string : seq<quote, must<string_content>, any> {
    using content = string_content;
};

struct key_content : until<at<quote>, must<char_>> {};
struct key : seq<quote, must<key_content>, any> {
    using content = key_content;
};

struct sm_entry : seq<key, sep, sym_colon, sep, sor<number, string>> {};
struct sm_list : list_must<sm_entry, sym_comma, ws> {};
struct sm_body : pad_opt<sm_list, ws> {};
struct speechmark : if_must<sym_start, sm_body, sym_end> {};

struct grammar : pad< list<speechmark, ews>, ews> {};

// ******************** ACTIONS *********************

// These rules are copied from PEGTL JSON escape-sequence parsing.  They pull out the content
// of a string and fix up any escape sequences.
template< typename Rule > struct unescape_action {};
template<> struct unescape_action< unicode > : unescape::unescape_j {};
template<> struct unescape_action< escaped_char > : unescape::unescape_c<escaped_char, '"', '\\', '/', '\b', '\f', '\n', '\r', '\t' > {};
template<> struct unescape_action< unescaped > : unescape::append_all {};
using unescape = change_action_and_states< unescape_action, std::string >;

// Local definitions

struct SMTemp {
    std::string keyword;
};

template <typename Rule> struct action : nothing<Rule> {};


template <>
struct action<number> {
    template <typename Input>
    static void apply(const Input& in, std::vector<SpeechMark>& speechMarks, SMTemp& temp) {
        if (temp.keyword == "time")
            speechMarks.back().time = std::stoul(in.string());
        else if (temp.keyword == "start")
            speechMarks.back().start = std::stoul(in.string());
        else if (temp.keyword == "end")
            speechMarks.back().end = std::stoul(in.string());
    }
};

template <>
struct action<string::content> : unescape {
    template <typename Input>
    static void success(const Input& /* unused */, std::string& s,
                        std::vector<SpeechMark>& speechMarks, SMTemp& temp) {
        if (temp.keyword == "type") {
            if (s == "word")
                speechMarks.back().type = kSpeechMarkWord;
            else if (s == "sentence")
                speechMarks.back().type = kSpeechMarkSentence;
            else if (s == "ssml")
                speechMarks.back().type = kSpeechMarkSSML;
            else if (s == "viseme")
                speechMarks.back().type = kSpeechMarkViseme;
        }
        else if (temp.keyword == "value")
            speechMarks.back().value = s;
    }
};

template <>
struct action<key::content> : unescape {
    template <typename Input>
    static void success(const Input& /* unused */, std::string& s,
                        std::vector<SpeechMark>& speechMarks, SMTemp& temp) {
        temp.keyword = s;
    }
};


template <>
struct action<sym_start> {
    template <typename Input>
    static void apply(const Input& in, std::vector<SpeechMark>& speechMarks, SMTemp& temp) {
        speechMarks.emplace_back(SpeechMark{kSpeechMarkUnknown, 0, 0, 0, ""});
    }
};

std::vector<SpeechMark>
parseData(const char* data, unsigned long length) {
    std::vector<SpeechMark> result;

    pegtl::memory_input<> in(data, length, "");
    try {
        pollygrammar::SMTemp temp;
        pegtl::parse<grammar, action>(in, result, temp);
    }
    catch (const pegtl::parse_error& e) {
    }
    return result;
}

} // namespace pollygrammar

std::vector<SpeechMark>
parsePollySpeechMarks(const char* data, unsigned long length) {
    return pollygrammar::parseData(data, length);
}

Bimap<int, std::string> sSpeechMarkTypeMap = {
    {kSpeechMarkViseme, "viseme"},
    {kSpeechMarkSentence, "sentence"},
    {kSpeechMarkSSML, "ssml"},
    {kSpeechMarkWord, "word"},
    {kSpeechMarkUnknown, "unknown"}
};

} // namespace apl