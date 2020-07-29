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

#include "apl/primitives/characterrange.h"
#include "apl/primitives/characterrangegrammar.h"
#include "apl/utils/session.h"

namespace apl {

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;

/**
 * Parse a rangeExpression and return a stack of CharacterRange objects
 * @param rangeExpression String representing 0 or more character ranges
 * @return a stack of CharacterRange objects
 */
std::vector<CharacterRangeData> CharacterRanges::parse(const apl::SessionPtr &session, const char* rangeExpression)
{
    if (rangeExpression != nullptr &&
        rangeExpression[0] != '\0') {
        try {
            character_range_grammar::character_range_state state;
            pegtl::string_input<> in(rangeExpression, "");
            pegtl::parse<character_range_grammar::grammar, character_range_grammar::action>(in, state);
            return state.getRanges();
        }
        catch (pegtl::parse_error e) {
            CONSOLE_S(session) << "Error parsing character range '" << rangeExpression << "', " << e.what();
        }
    }
    return std::vector<CharacterRangeData>();
}

}
