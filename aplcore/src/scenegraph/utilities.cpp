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

#include "apl/component/componentproperties.h"
#include "apl/component/corecomponent.h"
#include "apl/content/rootconfig.h"
#include "apl/scenegraph/textmeasurement.h"
#include "apl/scenegraph/textproperties.h"
#include "apl/utils/session.h"

#include "apl/datagrammar/grammarpolyfill.h"

namespace apl {
namespace sg {

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace grammar {

// See parsing rules at: https://developer.mozilla.org/en-US/docs/Web/CSS/font-family
//                       https://drafts.csswg.org/css-fonts-3/#font-family-prop
//
// The CSS rules for parsing identifiers are complicated.  We make some simplifying assumptions.
// We also support single and double quotes.

static const bool DEBUG_GRAMMAR = false;

struct ws : star<space> {};
struct identifier_char : sor<range<'a', 'z'>, range<'A', 'Z'>, range<'0', '9'>, one<'_'>, one<'-'>> {};
struct identifier : plus<identifier_char> {};

struct squote : if_must<one<'\''>, until<one<'\''>>> {};
struct dquote : if_must<one<'"'>, until<one<'"'>>> {};
struct quoted : sor<squote, dquote> {};
struct unquoted : seq<identifier, star<ws, identifier>> {};
struct item : sor<quoted, unquoted> {};
struct items : list_must<item, one<','>, space> {};
struct textlist : must<ws, opt<items>, ws, eof> {};

template<typename Rule>
struct action
    : pegtl::nothing< Rule > {
};

struct split_state : fail_state
{
    std::vector<std::string> strings;
    std::string working;
};

template<> struct action< identifier >
{
    template< typename Input >
    static void apply( const Input& in, split_state& state ) {
        LOG_IF(DEBUG_GRAMMAR) << "identifier '" << in.string() << "'";
        if (!state.working.empty())
            state.working += ' ' + in.string();
        else
            state.working = in.string();
    }
};

template<> struct action< unquoted >
{
    template< typename Input >
    static void apply( const Input& in, split_state& state ) {
        LOG_IF(DEBUG_GRAMMAR) << "unquoted '" << in.string() << "'";
        if (!state.working.empty()) {
            state.strings.emplace_back(state.working);
            state.working.clear();
        }
    }
};

template<> struct action< quoted >
{
    template< typename Input >
    static void apply( const Input& in, split_state& state ) {
        LOG_IF(DEBUG_GRAMMAR) << "quoted " << in.string();
        auto s = in.string();
        state.strings.emplace_back( s.substr(1, s.size() - 2));
    }
};

} // namespace grammar


std::vector<std::string>
splitFontString(const RootConfig& rootConfig, const SessionPtr& session, const std::string& text)
{
    grammar::split_state state;
    pegtl::string_input<> in(text, "");
    if (!pegtl::parse<grammar::textlist, grammar::action, apl_control>(in, state) || state.failed) {
        CONSOLE(session) << "Parse error in '" << text << "' - " << state.what();
        state.strings.clear();  // Throw away any partial data that was parsed
    }

    // Append the default font from the root config if it is not already at the end of the list
    std::string defaultFont = rootConfig.getDefaultFontFamily();
    if (state.strings.empty() || state.strings.back() != defaultFont)
        state.strings.emplace_back(defaultFont);

    return state.strings;
}


} // namespace sg
} // namespace apl