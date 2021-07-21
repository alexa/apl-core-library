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

#include "apl/primitives/timegrammar.h"

namespace apl {
namespace timegrammar {

std::string
timeToString(const std::string& format, double time)
{
    try {
        time_state state(time);
        pegtl::string_input<> in(format, "");
        pegtl::parse<grammar, action>(in, state);
        return state.mString;
    }
    catch (parse_error e) {
        LOG(LogLevel::kError) << "Error in '" << format << "', " << e.what();
    }

    return "";
}

} // namespace timegrammar
} // namespace apl