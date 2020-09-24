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

int
utf8StringLength(const std::string& utf8String) {
    uint8_t *ptr = (uint8_t *) utf8String.data();
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

    auto startPtr = utf8AdvanceCodePointsUnsafe((uint8_t*)utf8String.data(), start);
    auto endPtr = utf8AdvanceCodePointsUnsafe(startPtr, end - start);
    return std::string((char *)startPtr, endPtr - startPtr);
}

} // namespace apl

