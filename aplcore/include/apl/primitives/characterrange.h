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
#include <vector>
#include <string>

#include "apl/common.h"

#ifndef _APL_CHARACTERRANGE_H
#define _APL_CHARACTERRANGE_H

namespace apl {

class CharacterRangeData {
public:
    CharacterRangeData(wchar_t first, wchar_t second) :
        mLower(first < second ? first : second),
        mUpper(first < second ? second : first) {}
    bool isCharacterValid(const wchar_t& wc) const {
        return (wc <= mUpper && wc >= mLower);
    }
private:
    const wchar_t mLower;
    const wchar_t mUpper;
};

class CharacterRanges {
public:
    /**
     * Build a character ranges holder from a string expression
     *
     * e.g -
     * "a-zA-Z0-9" expresses that the characters in the ranges
     *    a-z, A-Z, and 0-9 are valid
     * "--=" expresses that characters in the range of '-' through '=' are valid.
     *   A dash can be represented as a valid character, but only in the first term of the expression.
     *
     * @param session The logging session
     * @param rangeExpression The character range expression.
     */
    CharacterRanges(const SessionPtr &session, const char *rangeExpression) :
        mRanges(parse(session, rangeExpression)) {}

    /**
     * Build a character ranges holder from a string expression
     * @param session The logging session
     * @param rangeExpression The character range expression.
     */
    CharacterRanges(const SessionPtr &session, const std::string& rangeExpression) :
        CharacterRanges(session, rangeExpression.data()) {}

    const std::vector<CharacterRangeData>& getRanges() const { return mRanges; }

private:
    static std::vector<CharacterRangeData> parse(const SessionPtr &session, const char* rangeExpression);
    const std::vector<CharacterRangeData> mRanges;
};
} // namespace apl

#endif //_APL_CHARACTERRANGE_H
