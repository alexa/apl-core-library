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

#include <string>

#include "apl/utils/url.h"

namespace apl {

// See https://tools.ietf.org/html/rfc3986#section-2.3
bool
isUsableRaw(unsigned char c) {
    return std::isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~';
}


std::string
encodeUrl(const std::string& url) {
    std::string encodedUrl;
    encodedUrl.reserve(url.length() * 3);
    char hexChar[4]; // "%20" type escapes are 4 characters with null termination
    for (char c : url) {
        if (isUsableRaw(static_cast<unsigned char>(c))) {
            encodedUrl.append(1, c);
        } else {
            // format string decoding: %% -> '%'
            // %.2hhX -> format an unsigned char (hh) as uppercase hex (X) outputting exactly two digits (.2)
            std::snprintf(hexChar, 4, "%%%.2hhX", c);
            encodedUrl.append(hexChar);
        }
    }
    return encodedUrl;
}

}

