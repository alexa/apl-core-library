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

#include "apl/utils/stringfunctions.h"
#include <algorithm>

namespace apl {

std::string
rtrim(const std::string &str) {
    std::string output(str);
    output.erase(std::find_if(output.rbegin(), output.rend(), [](unsigned char ch) {
                   return !std::isspace(ch);
                 }).base(),
                 output.end());
    return output;
}

std::string
ltrim(const std::string &str) {
    std::string output(str);
    output.erase(output.begin(), std::find_if(output.begin(), output.end(), [](unsigned char ch) {
      return !std::isspace(ch);
    }));
    return output;
}

std::string
trim(const std::string &str) {
    return rtrim(ltrim(str));
}

std::string
tolower(const std::string& str) {
    std::string output = str;

    std::transform(output.begin(), output.end(), output.begin(), [](unsigned char ch) {
      return std::tolower(ch);
    });

    return output;
}

}