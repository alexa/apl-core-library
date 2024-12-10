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

struct LengthWithRangeTest {
    std::string s;
    int start;
    int count;
    int codepoints;

    std::string toString() const {
        auto result = std::string("'") + s + "'";
        result += " start=" + std::to_string(start);
        result += " count=" + std::to_string(count);
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

static auto STRING_LENGTH_WITH_RANGE_TESTS = std::vector<LengthWithRangeTest>{
    // Test cases that start at the string boundary.
    {u8"fuzzy", 0, 2, 2},
    {u8"\u007f\u0001", 0, 1, 1},  // Two single byte characters
    {u8"\u0080\u07ff", 0, 2, 1},  // Two two-byte characters
    {u8"\u0800\uffff", 0, 3, 1}, // Two three-byte characters
    {u8"\U00010000\U0010ffff", 0, 4, 1}, // Two four-byte characters

    // Test cases that start at a codepoint boundary.
    {u8"fuzzy", 2, 2, 2},
    {u8"\u007f\u0001", 1, 1, 1},  // Two single byte characters
    {u8"\u0080\u07ff", 2, 2, 1},  // Two two-byte characters
    {u8"\u0800\uffff", 3, 3, 1}, // Two three-byte characters
    {u8"\U00010000\U0010ffff", 4, 4, 1}, // Two four-byte characters

    // Test cases that start or end in the middle of a codepoint boundary.
    {u8"\u0080\u07ff", 1, 2, -1},  // Start in the middle of a utf8 codepoint
    {u8"\u0800\uffff", 1, 3, -1}, // Start in the middle of a utf8 codepoint
    {u8"\U00010000\U0010ffff", 1, 4, -1}, // Start in the middle of a utf8 codepoint
    {u8"\u0080\u07ff", 0, 3, -1},  // End in the middle of a utf8 codepoint
    {u8"\u0800\uffff", 0, 4, -1}, // End in the middle of a utf8 codepoint
    {u8"\U00010000\U0010ffff", 0, 5, -1}, // End in the middle of a utf8 codepoint

    // Length Overflow protection test.
    {u8"fuzzy", 0, 10, 5},
};

TEST_F(UnicodeTest, StringLengthWithRange) {
    for (const auto& m : STRING_LENGTH_WITH_RANGE_TESTS) {
        const uint8_t* ptr = (uint8_t*)m.s.data() + m.start;
        ASSERT_EQ(m.codepoints, utf8StringLength(ptr, m.count)) << m.toString();
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

struct StringCharAtTest {
    std::string original;
    int index;
    std::string expected;
};

static auto STRING_CHAR_AT_TESTS = std::vector<StringCharAtTest> {
    { u8"", 0, u8"" },
    { u8"abcde", 0, u8"a" },
    { u8"abcde", 1, u8"b" },
    { u8"abcde", 3, u8"d" },
    { u8"abcde", 10, u8"" },
    { u8"abcde", -3, u8"c"},   // Negative offset
    { u8"abcde", -100, u8""},  // Seriously negative offset
    { u8"hémidécérébellé", 1, u8"é"},
    { u8"hémidécérébellé", 4, u8"d"},
    { u8"hémidécérébellé", 8, u8"r"},
    { u8"é", -1, u8"é"},
    { u8"عمر خیّام‎", 0, u8"ع" },  // RTL
};

TEST_F(UnicodeTest, StringCharAt)
{
    for (const auto& m : STRING_CHAR_AT_TESTS)
        ASSERT_EQ(m.expected, utf8StringCharAt(m.original, m.index));
}

struct StringIndexOfTest {
    std::string s;
    std::string searchString;
    int index;
    bool forward;
    int expected;

    std::string toString() const {
        auto result = " targetString=" + s;
        result += " searchString=" + searchString;
        result += " index=" + std::to_string(index);
        result += " forwardSearch=" + std::to_string(forward);
        return result;
    }
};

static auto STRING_INDEX_OF_TESTS = std::vector<StringIndexOfTest> {
    { u8"", u8"", 0, true, 0 },
    { u8"hello", u8"ello", 0, true, 1 },
    { u8"abcabc", u8"bc", 0, true, 1 },
    { u8"abcabc", u8"bc", 2, true, 4 },
    { u8"bbbbb", u8"b", 1, true, 1 },
    { u8"hémidécérébellé", u8"é", -1, true, 14 },
    { u8"abcabc", u8"b", 8, true, -1 },
    { u8"abcabc", u8"b", -8, true, -1 },
    { u8"abc", u8"abcabc", 0, true, -1 },
    { u8"abc", u8"abc", 0, true, 0 },
    { u8"", u8"", 0, false, 0 },
    { u8"hello", u8"ello", 4, false, 1 },
    { u8"abcabc", u8"bc", 5, false, 4 },
    { u8"abcabc", u8"bc", -1, false, 4 },
    { u8"abcabc", u8"b", 8, false, -1 },
    { u8"abcabc", u8"b", -8, false, -1 },
    { u8"abc", u8"abcabc", 2, false, -1 },
    { u8"abc", u8"abc", -1, false, 0 },
    { "\x80", "\x80", 0, true, -1 } // invalid utf string
};

TEST_F(UnicodeTest, StringIndexOf)
{
    for (const auto& m : STRING_INDEX_OF_TESTS)
        ASSERT_EQ(m.expected, utf8StringIndexOf(m.s, m.searchString, m.index, m.forward)) << m.toString();
}

struct StringReplaceTest {
    std::string s;
    std::string searchString;
    std::string replaceString;
    std::string expected;
    int startIndex;

    std::string toString() const {
        auto result = " originalString=" + s;
        result += " searchString=" + searchString;
        result += " replaceString=" + replaceString;
        return result;
    }
};

static auto STRING_REPLACE_TESTS = std::vector<StringReplaceTest> {
    { u8"abcdefg", u8"ab", u8"XY", u8"XYcdefg" },
    { u8"abcdefg", u8"", u8"XYZ", u8"abcdefg" },
    { u8"abcdefg", u8"bcd", u8"", u8"aefg" },
    { u8"abcdefg", u8"abcdefg", u8"", u8"" },
    { u8"abcdefg", u8"xyz", u8"123", u8"abcdefg" },
    { u8"", u8"abc", u8"XYZ", u8"" },
    { u8"مرحبا بالعالم", u8"العالم", u8"الكون", u8"مرحبا بالكون" },
    { u8"résumé café", u8"é", u8"e", u8"resumé café" },
    { u8"مرحبا بالعالم الجميل", u8"العالم", u8"الكون", u8"مرحبا بالكون الجميل", -7 },
    { u8"résumé café", u8"fé", u8"fe", u8"résumé cafe", -3 },
};

TEST_F(UnicodeTest, StringReplace)
{
    for (const auto& m : STRING_REPLACE_TESTS)
        ASSERT_EQ(m.expected, utf8StringReplace(m.s, m.searchString, m.replaceString)) << m.toString();
}

struct StringReplaceAllTest {
    std::string s;
    std::string searchString;
    std::string replaceString;
    std::string expected;

    std::string toString() const {
        auto result = " originalString=" + s;
        result += " searchString=" + searchString;
        result += " replaceString=" + replaceString;
        return result;
    }
};

static auto STRING_REPLACE_ALL_TESTS = std::vector<StringReplaceAllTest> {
        { u8"abc def abc ghi", u8"abc", u8"XYZ", u8"XYZ def XYZ ghi" },
        { u8"ab ab", u8"ab", u8"XYZ", u8"XYZ XYZ" },
        { u8"abc abc abc", u8"abc", u8"X", u8"X X X" },
        { u8"aaaa", u8"aa", u8"X", u8"XX" },
        { u8"abcdef", u8"xy", u8"XYZ", u8"abcdef" },
        { u8"", u8"ab", u8"AB", u8"" },
        { u8"abcd", u8"", u8"X", u8"abcd" },
        { u8"ababab", u8"ab", u8"", u8"" },
        { u8"abcd", u8"", u8"", u8"abcd" },
        { u8"مرحبا بالعالم", u8"م", u8"ن", u8"نرحبا بالعالن" },
        { u8"résumé café", u8"é", u8"e", u8"resume cafe" }
    };

TEST_F(UnicodeTest, StringReplaceAll)
{
    for (const auto& m : STRING_REPLACE_ALL_TESTS)
        ASSERT_EQ(m.expected, utf8StringReplaceAll(m.s, m.searchString, m.replaceString)) << m.toString();
}

struct TrimWhitespaceTest {
    std::string original;
    std::string expected;

    std::string toString() const {
        return "Original: '" + original + "', Expected: '" + expected + "'";
    }
};

static auto TRIM_WHITESPACE_TEST = std::vector<TrimWhitespaceTest> {
        { u8" 1234567890 ", u8"1234567890"},
        { u8"1234567890", u8"1234567890"},
        { u8"", u8""},
        { u8"   ", u8""},
        { u8" \t\n\r1234567890\t\n\r ", u8"1234567890"},
        { u8" Stühle ", u8"Stühle"},
        { u8"\u2028\u202917,23\u261ac\u2028", u8"17,23\u261ac"},
        { u8"\u3000こんにちは\u3000", u8"こんにちは"}, // Ideographic space (U+3000) with Japanese text
        { u8"\uFEFF Hello \u00A0 World \u2003", u8"Hello \u00A0 World"}, // Zero-width no-break space and other spaces
    };

TEST_F(UnicodeTest, TrimWhitespaceTest)
{
    for (const auto& m : TRIM_WHITESPACE_TEST) {
        std::string result = utf8StringTrimWhiteSpace(m.original);
        ASSERT_EQ(m.expected, result) << m.toString();
    }
}

struct StripTest {
    std::string original;
    std::string valid;
    std::string expected;
};

static auto STRING_STRIP_TESTS = std::vector<StripTest> {
    { u8"", u8"abcd", u8"" },
    { u8"abcd", u8"", u8"abcd"},  // Empty valid set returns everything
    { u8"abcd", u8"bd", u8"bd"},
    { u8"abcd", u8"abdefghij", u8"abd"},
    { u8"\u27a3€17,23\u261ac", u8"$€0123456789,.", u8"€17,23"},  // 3-byte characters
    { u8"123,631", u8"0-9", u8"123631"},   // Simple range
    { u8"+--+", u8"-", u8"--"},  // Just hyphens
    { u8"+*-*", u8"-+", u8"+-"},
    { u8"+*-*", u8"+-", u8"+"},  // Malformed hyphen range
};

TEST_F(UnicodeTest, StringStripInvalid)
{
    for (const auto& m : STRING_STRIP_TESTS)
        ASSERT_EQ(m.expected, utf8StripInvalid(m.original, m.valid));
}

struct ValidCharacters {
    std::string original;
    std::string valid;
    bool expected;
};

static auto VALID_CHARACTER_TESTS = std::vector<ValidCharacters> {
    { u8"This is a test with an empty string", u8"", true},
    { u8"", u8"a-z", true},  // Empty strings are generally fine
    { u8"abc", u8"a-z", true},
    { u8"ABc", u8"a-z", false},
    { u8"☜", u8"a-zA-Z0-9", false},  // Out of normal range
    { u8"⇐", u8"\u21d0", true},  // The actual character
    { u8"⇐", u8"\u2100-\uffff", true},  // Large range
    { u8"\U0001f603", u8"\u0020-\uffff", false}, // Emoji are outside of the BMP
    { u8"\U0001f603", u8"\U0001f600-\U0001f64f", true}, // Emoticon ranges are fine
};

TEST_F(UnicodeTest, StringValidCharacters)
{
    for (const auto& m : VALID_CHARACTER_TESTS)
        ASSERT_EQ(m.expected, utf8ValidCharacters(m.original, m.valid)) << m.original;
}


struct TrimTest {
    std::string original;
    std::string expected;
    int trim;
};

static auto TRIM_TEST = std::vector<TrimTest> {
    { u8"1234567890", u8"123", 3},
    { u8"1234567890", u8"1234567890", 0},  // No trimming
    { u8"", u8"", 10},  // Nothing to trim
    { u8"", u8"", -1},  // Nothing to trim
    { u8"1234567890", u8"1234567890", 10},  // Fits within the trim window
    { u8"1234567890", u8"1234567890", 20},  // Fits within the trim window
    { u8"Stühle", u8"Stü", 3},   // Two-byte character
    { u8"\u27a3€17,23\u261ac", u8"\u27a3€17", 4}, // Three-byte characters
    { u8"\U0001f601\U0001f602\U0001f603", u8"\U0001f601\U0001f602", 2 }, // Four-byte characters
};

TEST_F(UnicodeTest, TrimTest)
{
    for (const auto& m : TRIM_TEST) {
        auto s = m.original;
        utf8StringTrim(s, m.trim);
        ASSERT_EQ(m.expected, s) << m.original << ":" << m.expected << ":" << m.trim;
    }

}

struct StripTrimTest {
    std::string original;
    std::string valid;
    std::string expected;
    int trim;
};

static auto STRING_STRIP_TRIM_TESTS = std::vector<StripTrimTest> {
    { u8"", u8"abcd", u8"", 0 },
    { u8"abcd", u8"", u8"abcd", 0},  // Empty valid set returns everything
    { u8"abcd", u8"bd", u8"bd", 0},
    { u8"abcd", u8"abdefghij", u8"abd", 3},
    { u8"\u27a3€17,23\u261ac", u8"$€0123456789,.", u8"€17,", 4},  // 3-byte characters
    { u8"123,631", u8"0-9", u8"12363", 5},   // Simple range
    { u8"+--+", u8"-", u8"-", 1},  // Just hyphens
    { u8"+*-*", u8"-+", u8"+-", 2},
    { u8"+*-*", u8"+-", u8"+", 20},  // Malformed hyphen range
};

TEST_F(UnicodeTest, InvalidAndTripTest)
{
    for (const auto& m : STRING_STRIP_TRIM_TESTS) {
        auto s = utf8StripInvalidAndTrim(m.original, m.valid, m.trim);
        ASSERT_EQ(m.expected, s) << m.original << ":" << m.expected << ":" << m.trim;
    }
}
