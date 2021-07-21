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

#ifndef _APL_TIME_GRAMMAR_H
#define _APL_TIME_GRAMMAR_H

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/abnf.hpp>
#include <cstdint>
#include <stack>
#include <algorithm>

#include "apl/utils/log.h"
#include "apl/primitives/timefunctions.h"

namespace apl {

/**
 * Grammar for formatting time values.  The following codes may be used:
 *
 * YY   19         Year, two digits
 * YYYY 2019       Year, four digits
 *
 * M    1..12      Month (1=January)
 * MM   01..12     Month (1=January)
 *
 * D    1..31      Day of the month
 * DD   01..31     Day of the month, two digits
 * DDD  N          Days, any number
 *
 * H    0..23      24-hour clock
 * HH   00..23     24-hour clock, two digits
 * HHH  N          Hours, any number of digits
 *
 * h    1..12      12-hour clock
 * hh   01..12     12-hour clock, two digits
 *
 * m    0..59      Minutes
 * mm   00..59     Minutes, two digits
 * mmm  N          Minutes, any number of digits
 *
 * s    0..59      Seconds
 * ss   00..59     Seconds, two digits
 * sss  N          Seconds, any number of digits
 *
 * S    0..9       Decisecond
 * SS   00..99     Centiseconds
 * SSS  000..999   Milliseconds
 */
namespace timegrammar {

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;
using namespace pegtl;

struct other         : any {};

struct year_four     : string<'Y', 'Y', 'Y', 'Y'> {};
struct year_two      : string<'Y', 'Y'> {};

struct month_two     : string<'M', 'M'> {};
struct month         : one<'M'> {};

struct days_any      : string<'D', 'D', 'D'> {};
struct date_two      : string<'D', 'D'> {};
struct date          : one<'D'> {};

struct hours_any     : string<'H', 'H', 'H'>{};
struct hours_two_24  : string<'H', 'H'> {};
struct hours_24      : one<'H'> {};

struct hours_two_12  : string<'h', 'h'> {};
struct hours_12      : one<'h'> {};

struct minutes_any   : string<'m', 'm', 'm'> {};
struct minutes_two   : string<'m', 'm'> {};
struct minutes       : one<'m'> {};

struct seconds_any   : string<'s', 's', 's'> {};
struct seconds_two   : string<'s', 's'> {};
struct seconds       : string<'s'> {};

struct millisecond   : string<'S', 'S', 'S'> {};
struct centisecond   : string<'S', 'S'> {};
struct decisecond    : one<'S'> {};

struct unit : sor< year_four, year_two,
                   month_two, month,
                   days_any, date_two, date,
                   hours_any, hours_two_24, hours_24, hours_two_12, hours_12,
                   minutes_any, minutes_two, minutes,
                   seconds_any, seconds_two, seconds,
                   millisecond, centisecond, decisecond,
                   other > {};

// Note- Explicitly specifying namespace pegtl because of this collision in Windows
// error C2872: 'eof': ambiguous symbol
struct raw : until<at<pegtl::eof>, must<unit>>{};
struct grammar : must< raw, pegtl::eof > {};

// ******************** ACTIONS *********************

template<typename Rule>
struct action : pegtl::nothing<Rule> {};

struct time_state {
    time_state(double time) : mTime(static_cast<apl::time::apl_itime_t>(time)) {}

    void append(int number) {
        mString += std::to_string(number);
    }

    void appendTwo(int number) {
        if (number < 10)
            mString += "0" + std::to_string(number);
        else
            mString += std::to_string(number);
    }

    std::string mString;
    const apl::time::apl_itime_t mTime;
};

template<> struct action<other>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        state.mString += in.string();
    }
};

template<> struct action<year_four>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        auto year = std::to_string(time::yearFromTime(state.mTime));
        state.mString += year.substr(year.length() - 4);
    }
};

template<> struct action<year_two>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        auto year = std::to_string(time::yearFromTime(state.mTime));
        state.mString += year.substr(year.length() - 2);
    }
};

template<> struct action<month_two>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        state.appendTwo(time::monthFromTime(state.mTime) + 1);
    }
};

template<> struct action<month>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        state.append(time::monthFromTime(state.mTime) + 1);
    }
};

template<> struct action<days_any>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        state.append(time::day(state.mTime));
    }
};

template<> struct action<date_two>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        state.appendTwo(time::dateFromTime(state.mTime));
    }
};

template<> struct action<date>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        state.append(time::dateFromTime(state.mTime));
    }
};

template<> struct action<hours_any>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        state.append(time::hours(state.mTime));
    }
};

template<> struct action<hours_two_24>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        state.appendTwo(time::hourOfDay(state.mTime));
    }
};

template<> struct action<hours_24>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        state.append(time::hourOfDay(state.mTime));
    }
};

template<> struct action<hours_two_12>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        auto hour = time::hourOfDay(state.mTime) % 12;
        state.appendTwo(hour == 0 ? 12 : hour);
    }
};

template<> struct action<hours_12>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        auto hour = time::hourOfDay(state.mTime) % 12;
        state.append(hour == 0 ? 12 : hour);
    }
};

template<> struct action<minutes_any>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        state.append(time::minutes(state.mTime));
    }
};

template<> struct action<minutes_two>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        state.appendTwo(time::minutesOfHour(state.mTime));
    }
};

template<> struct action<minutes>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        state.append(time::minutesOfHour(state.mTime));
    }
};


template<> struct action<seconds_any>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        state.append(time::seconds(state.mTime));
    }
};

template<> struct action<seconds_two>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        state.appendTwo(time::secondsOfMinute(state.mTime));
    }
};

template<> struct action<seconds>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        state.append(time::secondsOfMinute(state.mTime));
    }
};

template<> struct action<decisecond>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        auto delta = state.mTime / 100 % 10;
        state.mString += std::to_string(delta);
    }
};

template<> struct action<centisecond>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        auto delta = state.mTime / 10 % 100;
        state.appendTwo(delta);
    }
};

template<> struct action<millisecond>
{
    template< typename Input >
    static void apply(const Input& in, time_state& state) {
        auto delta = state.mTime % 1000;
        if (delta < 10)
            state.mString += "00" + std::to_string(delta);
        else if (delta < 100)
            state.mString += "0" + std::to_string(delta);
        else
            state.mString += std::to_string(delta);
    }
};

extern std::string timeToString(const std::string& format, double time);

} // namespace timegrammar

} // namespace apl

#endif //_APL_TIME_GRAMMAR_H
