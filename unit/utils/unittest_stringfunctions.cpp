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

#include "../testeventloop.h"

#include <vector>

#include <apl/utils/stringfunctions.h>

using namespace apl;

namespace {

struct ParseDoubleTestCase {
    std::string input;
    double expectedValue;
    std::size_t expectedPosition;
};

}

static const std::vector<ParseDoubleTestCase> PARSE_FP_TEST_CASES = {
    // Finite decimal values
    {"4", 4.0, 1},
    {"-4", -4.0, 2},
    {"4.0", 4.0, 3},
    {"4.", 4.0, 2},
    {"14.5", 14.5, 4},
    {"14.5", 14.5, 4},
    {".5", 0.5, 2},
    {".5000", 0.5, 5},
    {".5000X", 0.5, 5},
    {".5F", 0.5, 2},
    {"012.45", 12.45, 6},
    {"14.5E2", 1450.0, 6},
    {"14.5E2X", 1450.0, 6},
    {"14.E2", 1400.0, 5},
    {"14.5E+2", 1450.0, 7},
    {"14.5e+2", 1450.0, 7},
    {"14.625e+10", 1.4625e+11, 10},
    {"14.56E-2", 0.1456, 8},
    {"14.56e-2", 0.1456, 8},

    // Finite hex values
    {"0XFF", 255.0, 4},
    {"0X12.", 18.0, 5},
    {"  0X12.", 18.0, 7},
    {"0X12.F", 18.9375, 6},
    {"0X12.50", 18.3125, 7},
    {"0X12.AX", 18.625, 6},
    {"0X12.AP2", 74.5, 8},
    {"0X12.Ap2", 74.5, 8},
    {"0X12.AP2X", 74.5, 8},
    {"0X12.AP+2", 74.5, 9},
    {"0X12.AP+2X", 74.5, 9},
    {"0X12.AP-2", 4.65625, 9},
    {"0X12.AP-2X", 4.65625, 9},
    {"0X1.BC70A3D70A3D7P+6", 111.11, 20},

    // Infinite cases
    {"INF", std::numeric_limits<double>::infinity(), 3},
    {"inf", std::numeric_limits<double>::infinity(), 3},
    {"+inf", std::numeric_limits<double>::infinity(), 4},
    {"-INF", -std::numeric_limits<double>::infinity(), 4},
    {"-inf", -std::numeric_limits<double>::infinity(), 4},
    {"INFINITY", std::numeric_limits<double>::infinity(), 8},
    {"infinity", std::numeric_limits<double>::infinity(), 8},
    {"+INFINITY", std::numeric_limits<double>::infinity(), 9},
    {"-INFINITY", -std::numeric_limits<double>::infinity(), 9},
    {"-infinity", -std::numeric_limits<double>::infinity(), 9},

    // NaN cases
    {"NAN", std::numeric_limits<double>::quiet_NaN(), 3},
    {"NaN", std::numeric_limits<double>::quiet_NaN(), 3},
    {"nan", std::numeric_limits<double>::quiet_NaN(), 3},
    {"-NAN", std::numeric_limits<double>::quiet_NaN(), 4},

    // Whitespace
    {"  4", 4.0, 3},
    {" -4", -4.0, 3},
    {"   4.5", 4.5, 6},
    {"  NAN ", std::numeric_limits<double>::quiet_NaN(), 5},
    {"  +INF ", std::numeric_limits<double>::infinity(), 6},
    {"   -INF ", -std::numeric_limits<double>::infinity(), 7},

    // Suffixes
    {"  4%", 4.0, 3},
    {" -4%", -4.0, 3},
    {" -4.5%", -4.5, 5},
    {"  NANX ", std::numeric_limits<double>::quiet_NaN(), 5},
    {"  +INFX ", std::numeric_limits<double>::infinity(), 6},

    // Edge cases
    {"", std::numeric_limits<double>::quiet_NaN(), 0},
    {"\t", std::numeric_limits<double>::quiet_NaN(), 1},
    {"  ", std::numeric_limits<double>::quiet_NaN(), 2},

    // Invalid numbers
    {"e2", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"e+2", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"e-2", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"p2", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"p+2", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"p-2", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"X34", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"   X34", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"+X", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"-X", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"14.56e", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"14.56e+", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"14.56e-", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"14.56eX", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"14.56e+X", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"14.56e-X", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"0X12P", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"0X12P+", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"0X12P-", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"0X12P+X", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"0X12P-X", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"0X12PX", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"0X12.PX", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
    {"0X12.APX", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<std::size_t>::max()},
};

TEST(StringFunctionsTest, ParseFloatLiteral)
{
    double max_delta = 1e-6;
    for (const auto &testCase : PARSE_FP_TEST_CASES) {
        std::size_t pos = std::numeric_limits<std::size_t>::max();
        float parsed = sutil::stof(testCase.input, &pos);

        if (std::isnan(testCase.expectedValue)) {
            ASSERT_TRUE(std::isnan(parsed)) << "Input: '" << testCase.input << "'";
        } else if (std::isinf(testCase.expectedValue)) {
            ASSERT_EQ(testCase.expectedValue, parsed);
        } else {
            ASSERT_NEAR((float) testCase.expectedValue, parsed, max_delta) << "Input: '" << testCase.input << "'";
        }

        ASSERT_EQ(testCase.expectedPosition, pos) << "Input: '" << testCase.input << "'";
    }
}

TEST(StringFunctionsTest, ParseDoubleLiteral)
{
    double max_delta = 1e-6;
    for (const auto &testCase : PARSE_FP_TEST_CASES) {
        std::size_t pos = std::numeric_limits<std::size_t>::max();
        double parsed = sutil::stod(testCase.input, &pos);

        if (std::isnan(testCase.expectedValue)) {
            ASSERT_TRUE(std::isnan(parsed)) << "Input: '" << testCase.input << "'";
        } else if (std::isinf(testCase.expectedValue)) {
            ASSERT_EQ(testCase.expectedValue, parsed);
        } else {
            ASSERT_NEAR(testCase.expectedValue, parsed, max_delta) << "Input: '" << testCase.input << "'";
        }

        ASSERT_EQ(testCase.expectedPosition, pos) << "Input: '" << testCase.input << "'";
    }
}

TEST(StringFunctionsTest, ParseLongDoubleLiteral)
{
    double max_delta = 1e-6;
    for (const auto &testCase : PARSE_FP_TEST_CASES) {
        std::size_t pos = std::numeric_limits<std::size_t>::max();
        long double parsed = sutil::stold(testCase.input, &pos);

        if (std::isnan(testCase.expectedValue)) {
            ASSERT_TRUE(std::isnan(parsed)) << "Input: '" << testCase.input << "'";
        } else if (std::isinf(testCase.expectedValue)) {
            ASSERT_EQ(testCase.expectedValue, parsed);
        } else {
            ASSERT_NEAR(testCase.expectedValue, parsed, max_delta) << "Input: '" << testCase.input << "'";
        }

        ASSERT_EQ(testCase.expectedPosition, pos) << "Input: '" << testCase.input << "'";
    }
}

TEST(StringFunctionsTest, FormatFloat)
{
    ASSERT_EQ("4.000000", sutil::to_string(4.0f));
    ASSERT_EQ("-4.000000", sutil::to_string(-4.0f));
    ASSERT_EQ("4.500000", sutil::to_string(4.5f));
    ASSERT_EQ("-4.500000", sutil::to_string(-4.5f));
    ASSERT_EQ("1004.500000", sutil::to_string(1004.5f));
    ASSERT_EQ("0.666667", sutil::to_string((float) 2.0/3));
    ASSERT_EQ("-0.500000", sutil::to_string(-0.5f));
    ASSERT_EQ("0.005000", sutil::to_string(0.005f));
    ASSERT_EQ("-0.005000", sutil::to_string(-0.005f));
    ASSERT_EQ("0.000001", sutil::to_string(1e-6f));
    ASSERT_EQ("0.000000", sutil::to_string(1e-7f));
    ASSERT_EQ("0.000000", sutil::to_string(0.0f));
    ASSERT_EQ("1.000000", sutil::to_string(0.9999997f));
    ASSERT_EQ("-1.000000", sutil::to_string(-0.9999997f));
    ASSERT_EQ("10.000000", sutil::to_string(9.9999997f));
    ASSERT_EQ("9.999999", sutil::to_string(9.9999993f));
    ASSERT_EQ("-10.000000", sutil::to_string(-9.9999997f));
    ASSERT_EQ("-9.999999", sutil::to_string(-9.9999993f));
    ASSERT_EQ("inf", sutil::to_string(std::numeric_limits<float>::infinity()));
    ASSERT_EQ("-inf", sutil::to_string(-std::numeric_limits<float>::infinity()));
    ASSERT_EQ("nan", sutil::to_string(std::numeric_limits<float>::quiet_NaN()));
}

TEST(StringFunctionsTest, FormatDouble)
{
    ASSERT_EQ("4.000000", sutil::to_string(4.0));
    ASSERT_EQ("-4.000000", sutil::to_string(-4.0));
    ASSERT_EQ("4.500000", sutil::to_string(4.5));
    ASSERT_EQ("-4.500000", sutil::to_string(-4.5));
    ASSERT_EQ("1004.500000", sutil::to_string(1004.5));
    ASSERT_EQ("0.666667", sutil::to_string(2.0/3));
    ASSERT_EQ("-0.500000", sutil::to_string(-0.5));
    ASSERT_EQ("0.005000", sutil::to_string(0.005));
    ASSERT_EQ("-0.005000", sutil::to_string(-0.005));
    ASSERT_EQ("0.000001", sutil::to_string(1e-6));
    ASSERT_EQ("0.000000", sutil::to_string(1e-7));
    ASSERT_EQ("0.000000", sutil::to_string(0.0));
    ASSERT_EQ("1.000000", sutil::to_string(0.9999997));
    ASSERT_EQ("-1.000000", sutil::to_string(-0.9999997));
    ASSERT_EQ("10.000000", sutil::to_string(9.9999997));
    ASSERT_EQ("9.999999", sutil::to_string(9.9999993));
    ASSERT_EQ("-10.000000", sutil::to_string(-9.9999997));
    ASSERT_EQ("-9.999999", sutil::to_string(-9.9999993));
    ASSERT_EQ("inf", sutil::to_string(std::numeric_limits<double>::infinity()));
    ASSERT_EQ("-inf", sutil::to_string(-std::numeric_limits<double>::infinity()));
    ASSERT_EQ("nan", sutil::to_string(std::numeric_limits<double>::quiet_NaN()));
}

TEST(StringFunctionsTest, FormatLongDouble)
{
    ASSERT_EQ("4.000000", sutil::to_string((long double) 4.0));
    ASSERT_EQ("-4.000000", sutil::to_string((long double) -4.0));
    ASSERT_EQ("4.500000", sutil::to_string((long double) 4.5));
    ASSERT_EQ("-4.500000", sutil::to_string((long double) -4.5));
    ASSERT_EQ("1004.500000", sutil::to_string((long double) 1004.5));
    ASSERT_EQ("0.666667", sutil::to_string((long double) 2.0/3));
    ASSERT_EQ("-0.500000", sutil::to_string((long double) -0.5));
    ASSERT_EQ("0.005000", sutil::to_string((long double) 0.005));
    ASSERT_EQ("-0.005000", sutil::to_string((long double) -0.005));
    ASSERT_EQ("0.000001", sutil::to_string((long double) 1e-6));
    ASSERT_EQ("0.000000", sutil::to_string((long double) 1e-7));
    ASSERT_EQ("0.000000", sutil::to_string((long double) 0.0));
    ASSERT_EQ("1.000000", sutil::to_string((long double) 0.9999997));
    ASSERT_EQ("-1.000000", sutil::to_string((long double) -0.9999997));
    ASSERT_EQ("10.000000", sutil::to_string((long double) 9.9999997));
    ASSERT_EQ("9.999999", sutil::to_string((long double) 9.9999993));
    ASSERT_EQ("-10.000000", sutil::to_string((long double) -9.9999997));
    ASSERT_EQ("-9.999999", sutil::to_string((long double) -9.9999993));
    ASSERT_EQ("inf", sutil::to_string(std::numeric_limits<long double>::infinity()));
    ASSERT_EQ("-inf", sutil::to_string(-std::numeric_limits<long double>::infinity()));
    ASSERT_EQ("nan", sutil::to_string(std::numeric_limits<long double>::quiet_NaN()));
}

TEST(StringFunctionsTest, CharacterChecks)
{
    ASSERT_EQ('.', sutil::DECIMAL_POINT);

    ASSERT_TRUE(sutil::isspace(' '));
    ASSERT_TRUE(sutil::isspace('\t'));
    ASSERT_TRUE(sutil::isspace('\r'));
    ASSERT_TRUE(sutil::isspace('\n'));
    ASSERT_TRUE(sutil::isspace('\v'));
    ASSERT_TRUE(sutil::isspace('\f'));
    ASSERT_FALSE(sutil::isspace('0'));
    ASSERT_FALSE(sutil::isspace('\0'));
    ASSERT_FALSE(sutil::isspace('A'));

    ASSERT_TRUE(sutil::isspace((unsigned char) ' '));
    ASSERT_TRUE(sutil::isspace((unsigned char) '\t'));
    ASSERT_TRUE(sutil::isspace((unsigned char) '\r'));
    ASSERT_TRUE(sutil::isspace((unsigned char) '\n'));
    ASSERT_TRUE(sutil::isspace((unsigned char) '\v'));
    ASSERT_TRUE(sutil::isspace((unsigned char) '\f'));
    ASSERT_FALSE(sutil::isspace((unsigned char) '0'));
    ASSERT_FALSE(sutil::isspace((unsigned char) '\0'));
    ASSERT_FALSE(sutil::isspace((unsigned char) 'A'));

    ASSERT_FALSE(sutil::isalnum(' '));
    ASSERT_TRUE(sutil::isalnum('0'));
    ASSERT_TRUE(sutil::isalnum('A'));
    ASSERT_TRUE(sutil::isalnum('x'));
    ASSERT_FALSE(sutil::isalnum('-'));

    ASSERT_FALSE(sutil::isalnum((unsigned char) ' '));
    ASSERT_TRUE(sutil::isalnum((unsigned char) '0'));
    ASSERT_TRUE(sutil::isalnum((unsigned char) 'A'));
    ASSERT_TRUE(sutil::isalnum((unsigned char) 'x'));
    ASSERT_FALSE(sutil::isalnum((unsigned char) '-'));

    ASSERT_FALSE(sutil::isupper(' '));
    ASSERT_FALSE(sutil::isupper(','));
    ASSERT_FALSE(sutil::isupper('0'));
    ASSERT_FALSE(sutil::isupper('a'));
    ASSERT_TRUE(sutil::isupper('A'));

    ASSERT_FALSE(sutil::isupper((unsigned char) ' '));
    ASSERT_FALSE(sutil::isupper((unsigned char) ','));
    ASSERT_FALSE(sutil::isupper((unsigned char) '0'));
    ASSERT_FALSE(sutil::isupper((unsigned char) 'a'));
    ASSERT_TRUE(sutil::isupper((unsigned char) 'A'));

    ASSERT_FALSE(sutil::islower(' '));
    ASSERT_FALSE(sutil::islower(','));
    ASSERT_FALSE(sutil::islower('0'));
    ASSERT_TRUE(sutil::islower('a'));
    ASSERT_FALSE(sutil::islower('A'));

    ASSERT_FALSE(sutil::islower((unsigned char) ' '));
    ASSERT_FALSE(sutil::islower((unsigned char) ','));
    ASSERT_FALSE(sutil::islower((unsigned char) '0'));
    ASSERT_TRUE(sutil::islower((unsigned char) 'a'));
    ASSERT_FALSE(sutil::islower((unsigned char) 'A'));
}

TEST(StringFunctionsTest, CaseConversions)
{
    ASSERT_EQ('a', sutil::tolower('a'));
    ASSERT_EQ('a', sutil::tolower('A'));
    ASSERT_EQ('0', sutil::tolower('0'));
    ASSERT_EQ('-', sutil::tolower('-'));

    ASSERT_EQ('a', sutil::tolower((unsigned char) 'a'));
    ASSERT_EQ('a', sutil::tolower((unsigned char) 'A'));
    ASSERT_EQ('0', sutil::tolower((unsigned char) '0'));
    ASSERT_EQ('-', sutil::tolower((unsigned char) '-'));

    ASSERT_EQ('A', sutil::toupper('a'));
    ASSERT_EQ('A', sutil::toupper('A'));
    ASSERT_EQ('0', sutil::toupper('0'));
    ASSERT_EQ('-', sutil::toupper('-'));

    ASSERT_EQ('A', sutil::toupper((unsigned char) 'a'));
    ASSERT_EQ('A', sutil::toupper((unsigned char) 'A'));
    ASSERT_EQ('0', sutil::toupper((unsigned char) '0'));
    ASSERT_EQ('-', sutil::toupper((unsigned char) '-'));
}


