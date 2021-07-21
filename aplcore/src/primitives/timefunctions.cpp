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

#include "apl/primitives/timefunctions.h"

namespace apl {
namespace time {

// Conversion routines match https://www.ecma-international.org/ecma-262/6.0/

// Algorithm from https://www.ecma-international.org/ecma-262/6.0/, section 20.3.1.3
int daysInYear(int year) {
    if (year % 4 != 0)
        return 365;

    if (year % 100 != 0)
        return 366;

    if (year % 400 != 0)
        return 365;

    return 366;
}

// Return the day number of the first day of year y
constexpr int dayFromYear(int year) noexcept {
    return 365 * (year - 1970) + ((year - 1969) / 4) - ((year - 1901) / 100) + ((year - 1601) / 400);
}

constexpr apl_itime_t timeFromYear(int year) noexcept {
    return MS_PER_DAY * static_cast<apl_itime_t>(dayFromYear(year));
}

int yearFromTime(apl_itime_t t) {
    // We guess the starting year by assuming that a year has 365.24 days in it.
    // Tests show that this guess is within a year or two of the correct year.
    auto year = 1970 + static_cast<int>(day(t) / 365.24);
    auto yearStartTime = timeFromYear(year);

    if (t < yearStartTime) {  // The actual year is earlier than "year"
        do {
            year -= 1;
            yearStartTime = timeFromYear(year);
        } while (t < yearStartTime);

        return year;
    }

    // The actual year is equal to or after "year"
    while (t >= yearStartTime) {
        year += 1;
        yearStartTime = timeFromYear(year);
    }

    return year - 1;
}

bool inLeapYear(apl_itime_t t) {
    auto year = yearFromTime(t);
    return daysInYear(year) == 366;
}

int dayWithinYear(apl_itime_t t) {
    return day(t) - dayFromYear(yearFromTime(t));
}

int monthFromTime(apl_itime_t t) {
    int day = dayWithinYear(t);
    if (day < 31) return 0;
    if (inLeapYear(t))
        day -= 1;
    if (day < 59) return 1;
    if (day < 90) return 2;
    if (day < 120) return 3;
    if (day < 151) return 4;
    if (day < 181) return 5;
    if (day < 212) return 6;
    if (day < 243) return 7;
    if (day < 273) return 8;
    if (day < 304) return 9;
    if (day < 334) return 10;
    return 11;
}

int dateFromTime(apl_itime_t t) {
    int day = dayWithinYear(t);
    if (day < 31) return day + 1;
    if (inLeapYear(t)) {
        if (day < 60) return day - 30;
        if (day < 91) return day - 59;
        if (day < 121) return day - 90;
        if (day < 152) return day - 120;
        if (day < 182) return day - 151;
        if (day < 213) return day - 181;
        if (day < 244) return day - 212;
        if (day < 274) return day - 243;
        if (day < 305) return day - 273;
        if (day < 335) return day - 304;
        return day - 334;
    } else {
        if (day < 59) return day - 30;
        if (day < 90) return day - 58;
        if (day < 120) return day - 89;
        if (day < 151) return day - 119;
        if (day < 181) return day - 150;
        if (day < 212) return day - 180;
        if (day < 243) return day - 211;
        if (day < 273) return day - 242;
        if (day < 304) return day - 272;
        if (day < 334) return day - 303;
        return day - 333;
    }
}

} // namespace time
} // namespace apl