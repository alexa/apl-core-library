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

#include "gtest/gtest.h"

#include "apl/primitives/unicode.h"

#include "../testeventloop.h"

using namespace apl;

class UnicodeTest : public ::testing::Test {};


struct LengthTest {
    std::string s;
    int bytes;
    int codepoints;

    std::string toString() const {
        auto result = std::string("'") + s + "'";
        result += " bytes=" + std::to_string(bytes);
        result += " cp=" + std::to_string(codepoints);
        result += " raw=";
        for (const auto& m : s) {
            char hex[10];
            snprintf(hex, sizeof(hex), "%02x", (uint8_t)m);
            result += std::string(hex);
        }
        return result;
    }
};

static auto STRING_LENGTH_TESTS = std::vector<LengthTest>{
    {u8"", 0, 0},
    {u8"fuzzy", 5, 5},
    {u8"année", 6, 5},
    {u8"€17", 5, 3},        // The euro sign is a three byte character
    {u8"\u00a2", 2, 1},     // Two byte character
    {u8"\u0939", 3, 1},     // Three byte character
    {u8"\u20ac", 3, 1},     // Three byte character
    {u8"\ud55c", 3, 1},     // Three byte character
    {u8"\U00010348", 4, 1}, // Four byte character
    {u8"\u007f\u0001", 2, 2},  // Two single byte characters
    {u8"\u0080\u07ff", 4, 2},  // Two two-byte characters
    {u8"\u0800\uffff", 6, 2 }, // Two three-byte characters
    {u8"\U00010000\U0010ffff", 8, 2}, // Two four-byte characters
    {u8"a\u00a3\u0939\U00010349", 10, 4}, // One of each type
    {u8"hétérogénéité", 18, 13},
    {"\x80", 1, -1},   // Invalid (this should be a trailing byte)
    {"\xbf", 1, -1},   // Invalid (this should be a trailing byte)
    {"\x20\x90", 2, -1},  // Trailing byte does not follow a two-byte header
    {"\xc0\x23", 2, -1},  // A two-byte character starts with at least 0xc2
    {"\xf5", 1, -1},      // Code points above U+10FFFF are invalid
};

TEST_F(UnicodeTest, StringLength) {
    for (const auto& m : STRING_LENGTH_TESTS) {
        ASSERT_EQ(m.bytes, m.s.length()) << m.toString();
        ASSERT_EQ(m.codepoints, utf8StringLength(m.s)) << m.toString();
    }
}

struct SubstringTest {
    std::string original;
    int start;
    int end;
    std::string expected;
};

static auto STRING_SLICE_TESTS = std::vector<SubstringTest> {
    { u8"", 0, 100, u8"" },
    { u8"abcde", 1, 3, u8"bc" },
    { u8"abcde", 3, 3, u8"" },        // Start and end point the same
    { u8"abcde", 3, 2, u8"" },        // End point earlier than start point
    { u8"abcde", 0, -1, u8"abcd" },   // Negative offset from end
    { u8"abcde", -3, 100, u8"cde"},   // Negative offset from start
    { u8"abcde", -100, 2, u8"ab"},    // Seriously negative start offset
    { u8"hémidécérébellé", 0, 4, u8"hémi"},
    { u8"hémidécérébellé", 4, 8, u8"décé"},
    { u8"hémidécérébellé", 8, -1, u8"rébell"},
    { u8"عمر خیّام‎", 0, 3, u8"عمر" },  // Pull out the first word in RtoL text
};

TEST_F(UnicodeTest, StringSlice) {
    for (const auto& m : STRING_SLICE_TESTS)
        ASSERT_EQ(m.expected, utf8StringSlice(m.original, m.start, m.end));
}
