/*
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
 *
 */

#ifndef _APL_IDENTIFIER_H
#define _APL_IDENTIFIER_H

#include <string>

namespace apl {

// Note: The isalpha() and isalnum() methods from <cctype> may take into account locale.
// The std::regex library is slower than an explicit character-by-character check.
// A faster approach would be to unwrap a 128 or 256 byte array (or bit array) and pre-populate
// it with the valid characters.  This approach uses interval checking.
inline bool isIdentifierStart(unsigned char p)
{
    return p == '_' || (p >= 'A' && p <= 'Z') || (p >= 'a' && p <= 'z');
}

inline bool isIdentifierTail(unsigned char p) {
    return p == '_' || (p >= 'A' && p <= 'Z') || (p >= 'a' && p <= 'z') || (p >= '0' && p <= '9');
}

inline bool isValidIdentifier(const std::string& s)
{
    if (s.empty())
        return false;

    auto it = s.begin();
    if (!isIdentifierStart(*it))
        return false;

    while (++it != s.end())
        if (!isIdentifierTail(*it))
            return false;

    return true;
}

}

#endif // _APL_IDENTIFIER_H