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

#include <algorithm>

#include "apl/utils/corelocalemethods.h"

namespace apl {

std::string
CoreLocaleMethods::toUpperCase(const std::string& value, const std::string& locale) {
    std::string result;
    result.resize(value.size());

    std::transform(value.begin(), value.end(), result.begin(),
                            [](unsigned char c) -> unsigned char { return std::toupper(c); });

    return result;
}

std::string
CoreLocaleMethods::toLowerCase(const std::string& value, const std::string& locale) {
    std::string result;
    result.resize(value.size());

    std::transform(value.begin(), value.end(), result.begin(),
                            [](unsigned char c) -> unsigned char { return std::tolower(c); });

    return result;
}

} // namespace apl

