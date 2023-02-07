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

#include "apl/primitives/unicode.h"

#include <algorithm>
#include <locale>
#include <codecvt>

namespace apl {

/*
 * Note: Ideally we would use the ICU library for these functions.  It is rather large,
 * so we are currently working around that by implementing few UTF-8 specific functions here.
 *
 * Some basic facts about the UTF-8 encoding.
 *
 * Bytes   Start      End  Byte 1    Byte 2    Byte 3    Byte 4
 *
 *   1    U+0000   U+007f  0xxxxxxx
 *   2    U+0080   U+07ff  110xxxxx  10xxxxxx
 *   3    U+0800   U+ffff  1110xxxx  10xxxxxx  10xxxxxx
 *   4   U+10000 U+10ffff  11110xxx  10xxxxxx  10xxxxxx  10xxxxxx
 *
 * The leading byte for a two-byte code is   (0xc2 <= x <= 0xdf)
 * The leading byte for a three byte code is (0xe0 <= x <= 0xef)
 * The leading byte for a four byte code is  (0xf0 <= x <= 0xf4)
 *
 * Trailing bytes satisfy (0x80 <= x <= 0xbf)
 */

inline size_t
countUTF8TrailingBytes(uint8_t leadingByte) {
    // The first valid 2 byte UTF-8 encoded character starts with 0xc2  (U+0080)
    // The last valid 4 byte UTF-8 encoded character starts with 0xf4   (U+10FFFF)
    if (leadingByte < 0xc2 || leadingByte > 0xf4)
        return 0;

    return 1 + (leadingByte >= 0xe0) + (leadingByte >= 0xf0);
}

inline bool
isValidUTF8TrailingByte(uint8_t byte) {
    return (byte >= 0x80 && byte <= 0xbf);
}

inline bool
isValidUTF8StartingByte(uint8_t byte) {
    return (byte <= 0x7f || (byte >= 0xc2 && byte <= 0xf4));
}

/**
 * UNSAFE function to advance over a number of code points.  This function does not check
 * for a valid UTF-8 string, nor does it check to see if you've walked over the end of the string.
 */
inline uint8_t *
utf8AdvanceCodePointsUnsafe(uint8_t *ptr, int n)
{
    while (n > 0) {
        ptr += countUTF8TrailingBytes(*ptr) + 1;
        n -= 1;
    }
    return ptr;
}

/**
 * Compare two UTF-8 characters for smaller, equal, or larger.
 * The UTF-8 encoding conveniently allows for direct byte-to-byte comparisons.
 * @param lhs The first character
 * @param rhs The second character
 * @param trailing The maximum number of trailing bytes to compare
 * @return -1 if lhs < rhs, +1 if lhs > rhs, 0 if they are equal
 */
int
compareUTF8(const uint8_t *lhs, const uint8_t *rhs, const int trailing)
{
    for (int i = 0 ; i <= trailing ; lhs++, rhs++, i++) {
        if (*lhs < *rhs)
            return -1;
        if (*lhs > *rhs)
            return 1;
    }
    return 0;
}


int
utf8StringLength(const std::string& utf8String) {
    uint8_t *ptr = (uint8_t *) utf8String.c_str();
    int length = 0;

    while (*ptr) {
        auto byte = *ptr++;
        if (!isValidUTF8StartingByte(byte))
            return -1;

        length += 1;
        for (auto trailing = countUTF8TrailingBytes(byte) ; trailing > 0 ; trailing--) {
            if (!isValidUTF8TrailingByte(*ptr++))
                return -1;
        }
    }
    return length;
}

std::string
utf8StringSlice(const std::string& utf8String, int start, int end)
{
    auto len = utf8StringLength(utf8String);
    if (len <= 0)
        return "";

    // Handle a negative starting offset.  Note that we don't support multiple wraps
    if (start < 0)
        start += len;
    start = std::max(0, std::min(start, len));

    // Handle a negative ending offset
    if (end < 0)
        end += len;
    end = std::max(0, std::min(end, len));

    if (end <= start)
        return "";

    auto startPtr = utf8AdvanceCodePointsUnsafe((uint8_t*)utf8String.c_str(), start);
    auto endPtr = utf8AdvanceCodePointsUnsafe(startPtr, end - start);
    return std::string((char *)startPtr, endPtr - startPtr);
}

/**
 * Internal method to check a single UTF-8 character and see if it appears in a
 * string of valid characters.  This method assumes that the inputs are valid UTF-8 strings
 * and that the validation string is non-empty.
 *
 * The APL specification states that the format of the valid string is:
 *
 * VSTRING
 *    ''              // Empty string
 *    ('-')? CODE*    // Optional hyphen followed by zero or more CODES
 * CODE
 *    CHAR '-' CHAR   // Character, hyphen, character
 *    CHAR            // Single unicode character
 * CHAR
 *    '0001' . '10FFFF'  // Any character in the range 0x01 to 0x10FFFF
 *
 * The APL specification states that an empty validation string accepts everything
 *
 * This implementation uses linear search to find UTF-8 code units in the valid character
 * string.  That makes the performance O(N^2), which could be improved with the construction of
 * some lookup bit streams, valid character set sorting (into 1-4 code unit sets), etc.
 * The advantage of this approach is that no intermediate data structures need to be
 * constructed or stored.
 *
 * @param ptr Pointer to the UTF-8 character to test for validity
 * @param pcount Number of trailing bytes in ptr
 * @param vptr Pointer to the UTF-8 string of valid characters and character ranges
 * @return True if this character is a valid character
 */
bool
utf8ValidCharacter(const uint8_t *ptr, const int pcount, const uint8_t *vptr)
{
    while (*vptr) {
        auto lower = compareUTF8(ptr, vptr, pcount);
        if (lower == 0)
            return true;

        vptr += countUTF8TrailingBytes(*vptr) + 1;

        // Check for a hyphen
        if (*vptr == '-') {
            vptr++;

            if (!*vptr)
                return false; // Trailing hyphen is ignored

            auto upper = compareUTF8(ptr, vptr, pcount);
            if (lower == 1 && upper != 1) // Greater than lower and less than or equal to upper
                return true;

            vptr += countUTF8TrailingBytes(*vptr) + 1;
        }
    }

    return false;
}

std::string
utf8StripInvalid(const std::string& utf8String, const std::string& validCharacters)
{
    if (validCharacters.empty())
        return utf8String;

    uint8_t *ptr = (uint8_t *) utf8String.c_str();
    uint8_t *vptr = (uint8_t *) validCharacters.c_str();

    // For now we'll just copy valid code points over
    std::string result;

    while (*ptr) {
        const auto pcount = static_cast<int>(countUTF8TrailingBytes(*ptr));
        if (utf8ValidCharacter(ptr, pcount, vptr))
            result.append((char *) ptr, pcount + 1);
        ptr += pcount + 1;
    }

    return result;
}

bool
utf8ValidCharacters(const std::string& utf8String, const std::string& validCharacters)
{
    if (validCharacters.empty())
        return true;

    uint8_t *ptr = (uint8_t *) utf8String.c_str();
    uint8_t *vptr = (uint8_t *) validCharacters.c_str();

    while (*ptr) {
        const auto pcount = static_cast<int>(countUTF8TrailingBytes(*ptr));
        if (!utf8ValidCharacter(ptr, pcount, vptr))
            return false;
        ptr += pcount + 1;
    }

    return true;
}

bool
wcharValidCharacter(wchar_t wc, const std::string& validCharacters)
{
    if (validCharacters.empty())
        return true;

    // Convert the single character into a UTF-8 string for checking.
    static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    auto result = converter.to_bytes(wc);
    auto* ptr = result.c_str();
    return utf8ValidCharacter((uint8_t*)ptr,
                              static_cast<int>(countUTF8TrailingBytes(*ptr)),
                              (uint8_t*)validCharacters.c_str());
}

bool
utf8StringTrim(std::string& utf8String, int maxLength)
{
    if (maxLength <= 0)
        return false;

    auto it = utf8String.begin();
    for (int i = 0 ; i < maxLength ; i++) {
        if (it == utf8String.end())
            return false;
        it += countUTF8TrailingBytes(*it) + 1;
    }

    utf8String.erase(it, utf8String.end());
    return true;
}

std::string
utf8StripInvalidAndTrim(const std::string& utf8String, const std::string& validCharacters, int maxLength)
{
    auto result = utf8StripInvalid(utf8String, validCharacters);
    utf8StringTrim(result, maxLength);
    return result;
}

} // namespace apl

