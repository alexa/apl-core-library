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
#include "apl/primitives/timegrammar.h"

using namespace apl;

struct TimeTest {
    const char *format;
    double value;
    std::string result;
};

static const std::vector<TimeTest> BASIC_TESTS = {
    {"", 0, ""},
    {"...",   0,   "..."},
    {"s.SSS", 1,   "0.001"},
    {"s.SSS", 22,  "0.022"},
    {"s.SSS", 200, "0.200"},
    {"s.SSS", 1000, "1.000"},
    {"s.SSS", 1001, "1.001"},
    {"s.SSS", 12345, "12.345"},
    {"s.SS", 1,   "0.00"},
    {"s.SS", 22,  "0.02"},
    {"s.SS", 200, "0.20"},
    {"s.SS", 1000, "1.00"},
    {"s.SS", 1001, "1.00"},
    {"s.SS", 12345, "12.34"},
    {"s.S", 1,   "0.0"},
    {"s.S", 22,  "0.0"},
    {"s.S", 200, "0.2"},
    {"s.S", 1000, "1.0"},
    {"s.S", 1001, "1.0"},
    {"s.S", 12345, "12.3"},
    {"ss", 12345, "12"},
    {"s", 123 * 1000 - 1, "2"},
    {"ss", 123 * 1000 - 1, "02"},
    {"sss", 123 * 1000 - 1, "122"},
    {"m:ss", 60 * 1000, "1:00"},
    {"mm:ss", 60 * 1000, "01:00"},
    {"m", 0, "0"},
    {"mm", 0, "00"},
    {"mmm", 0, "0"},
    {"m", 127 * 60 * 1000, "7"},
    {"mm", 127 * 60 * 1000, "07"},
    {"mmm", 127 * 60 * 1000, "127"},
    {"h", 0, "12"},
    {"hh", 0, "12"},
    {"h", 7 * 60 * 60 * 1000, "7"},
    {"hh", 7 * 60 * 60 * 1000, "07"},
    {"h", 17 * 60 * 60 * 1000, "5"},
    {"hh", 17 * 60 * 60 * 1000, "05"},
    {"h", 123 * 60 * 60 * 1000, "3"},
    {"hh", 123 * 60 * 60 * 1000, "03"},
    {"H", 0, "0"},
    {"HH", 0, "00"},
    {"HHH", 0, "0"},
    {"H", 7 * 60 * 60 * 1000, "7"},
    {"HH", 7 * 60 * 60 * 1000, "07"},
    {"HHH", 7 * 60 * 60 * 1000, "7"},
    {"H", 17 * 60 * 60 * 1000, "17"},
    {"HH", 17 * 60 * 60 * 1000, "17"},
    {"HHH", 17 * 60 * 60 * 1000, "17"},
    {"H", 123 * 60 * 60 * 1000, "3"},
    {"HH", 123 * 60 * 60 * 1000, "03"},
    {"HHH", 123 * 60 * 60 * 1000, "123"},
    {"D", 0, "1"},    // First day of the month
    {"DD", 0, "01"},
    {"DDD", 0, "0"},  // No days have passed
    {"D", 7 * 60 * 60 * 24 * 1000 , "8"},   // Eighth of the month
    {"DD", 7 * 60 * 60 * 24 * 1000, "08"},
    {"DDD", 7 * 60 * 60 * 24 * 1000, "7"},   // Seven days have passed

	// for the Microsoft compiler, for values greater than MAX_INT,
	// we need to use the 'LL' suffix to force it to use a 64 bit long
    {"D", 123LL * 60 * 60 * 24 * 1000, "4"},  // May 4th
    {"DD", 123LL * 60 * 60 * 24 * 1000, "04"},
    {"DDD", 123LL * 60 * 60 * 24 * 1000, "123"},
    {"M", 0, "1"},   // First month of the year (January)
    {"MM", 0, "01"},
    {"M", 180LL * 24 * 60 * 60 * 1000, "6"}, // June
    {"MM", 180LL * 24 * 60 * 60 * 1000, "06"},
    {"M", 360LL * 24 * 60 * 60 * 1000, "12"}, // December
    {"MM", 360LL * 24 * 60 * 60 * 1000, "12"},
    {"M", 367LL * 24 * 60 * 60 * 1000, "1"}, // January
    {"MM", 367LL * 24 * 60 * 60 * 1000, "01"},
    {"YY", 0, "70"},
    {"YYY", 0, "70Y"},  // Notice the trailing "Y"
    {"YYYY", 0, "1970"},
    {"YY", 367LL * 24 * 60 * 60 * 1000, "71"},
    {"YYYY", 367LL * 24 * 60 * 60 * 1000, "1971"},
    {"YY", 40LL * 367 * 24 * 60 * 60 * 1000, "10"},
    {"YYYY", 40LL * 367 * 24 * 60 * 60 * 1000, "2010"},
    // ensure compliance with unix time, 946684817 (seconds since epoch) is Sat Jan  1 00:00:17 UTC 2000
    // If this time were incorrectly treated as the true number of seconds elapsed since Jan 1 1970, then it would
    // incorrectly be rendered as Fri Dec 31 23:59:45 UTC 1999 due to incorrect compensation for the 32 leap seconds
    // between 1970 and 2000
    {"YYYY", 946684817000LL, "2000"},
    {"MM", 946684817000LL, "01"},
    {"DD", 946684817000LL, "01"},
    {"HH", 946684817000LL, "00"},
    {"mm", 946684817000LL, "00"},
    {"ss", 946684817000LL, "17"},
    {"sss", 946684817000LL, "946684817"},
};

TEST(TimeGrammarTest, Basic)
{
    for (const auto& m : BASIC_TESTS) {
        ASSERT_EQ(m.result, timegrammar::timeToString(m.format, m.value))
            << " Format: '" << m.format << "'"
            << " Value " << m.value;
    }
}
