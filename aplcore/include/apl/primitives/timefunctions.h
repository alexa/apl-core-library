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

#ifndef _APL_TIME_FUNCTIONS_H
#define _APL_TIME_FUNCTIONS_H

#include "apl/common.h"

namespace apl {
namespace time {

static const long HOURS_PER_DAY = 24;
static const long MINUTES_PER_HOUR = 60;
static const long SECONDS_PER_MINUTE = 60;

static const long MS_PER_SECOND = 1000;
static const long MS_PER_MINUTE = SECONDS_PER_MINUTE * MS_PER_SECOND;
static const long MS_PER_HOUR = MINUTES_PER_HOUR * MS_PER_MINUTE;
static const long MS_PER_DAY = HOURS_PER_DAY * MS_PER_HOUR;

using apl_itime_t = unsigned long long;

/**
 * Calculate the year.  Only valid between 1970 and 9999
 * @param t The time
 * @return The calculated year.
 */
int yearFromTime(apl_itime_t t);

/**
 * Calculate the month of the year
 * @param t The time.
 * @return The month, where 0=Jan, 1=Feb, ... 11=Dec
 */
int monthFromTime(apl_itime_t t);

/**
 * Return the number of days since the epoch
 * @param t The time
 * @return The number of days
 */
inline constexpr apl_itime_t day(apl_itime_t t) noexcept
{
    return t / MS_PER_DAY;
}

/**
 * Return the date of the month
 * @param t The time
 * @return The date, a value between 1..31
 */
int dateFromTime(apl_itime_t t);

/**
 * Total number of hours that have passed
 * @param t The current time
 * @return The total number of hours that have passed
 */
inline constexpr apl_itime_t hours(apl_itime_t t) noexcept
{
    return (t / MS_PER_HOUR);
}

/**
 * The current hour of the current day.  Between 0 and 23.
 * @param t The current time.
 * @return The hour
 */
inline constexpr int hourOfDay(apl_itime_t t) noexcept
{
    return hours(t) % HOURS_PER_DAY;
}

/**
 * Total number of minutes that have passed
 * @param t The current time
 * @return The total number of minutes that have passed
 */
inline constexpr apl_itime_t minutes(apl_itime_t t) noexcept
{
    return (t / MS_PER_MINUTE);
}

/**
 * The current minute of the current house.  Between 0 and 59.
 * @param t The current time
 * @return The minute
 */
inline constexpr int minutesOfHour(apl_itime_t t) noexcept
{
    return minutes(t) % MINUTES_PER_HOUR;
}

/**
 * Total number of seconds that have passed
 * @param t The current time
 * @return The total number of seconds that have passed.
 */
inline constexpr apl_itime_t seconds(apl_itime_t t) noexcept
{
    return t / MS_PER_SECOND;
}

/**
 * The current second of the current minute.  Between 0 and 59.
 * @param t The current time.
 * @return The second
 */
inline constexpr int secondsOfMinute(apl_itime_t t) noexcept
{
    return seconds(t) % SECONDS_PER_MINUTE;
}

/**
 * The current milliseconds of the current second.  Between 0 and 999.
 * @param t The current time
 * @return The millisecond
 */
constexpr int millisecondsOfSecond(apl_itime_t t) noexcept
{
    return t  % MS_PER_SECOND;
}

/**
 * Calculate the total number of days in a particular year.  Either 365 or 366.
 * @param year The year
 * @return The number of days in that year.
 */
int daysInYear(int year);

/**
 * Calculate the day number of the first day in a given year at or past 1970.
 * The day number is measured relative to the Epoch (1970).  For example,
 * dayFromYear(1970) = 0, dayFromYear(1972) = 730.
 * @param year The year
 * @return The day number.
 */
constexpr int dayFromYear(int year) noexcept;

/**
 * Calculate the first millisecond of the first day of the year.
 * This is the number of milliseconds since the epoch.
 * @param year The year (>= 1970).
 * @return The millisecond
 */
constexpr apl_itime_t timeFromYear(int year) noexcept;

/**
 * Calculate if this time value occurs within a leap year.
 * @param t The time
 * @return True if it falls within a leap year.
 */
bool inLeapYear(apl_itime_t t);

/**
 * Calculate the day of the year that this time value falls upong.
 * @param t The time
 * @return The day of the year (between 0 and 365).
 */
int dayWithinYear(apl_itime_t t);

} // namespace time
} // namespace apl

#endif // _APL_TIMEFUNCTIONS_H
