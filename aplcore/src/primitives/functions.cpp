/**
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <algorithm>
#include <climits>
#include <cmath>
#include <stdlib.h>

#include "apl/primitives/functions.h"
#include "apl/primitives/object.h"
#include "apl/primitives/timefunctions.h"
#include "apl/engine/context.h"
#include "apl/primitives/timegrammar.h"

namespace apl {

static Object
mathMin(const std::vector<Object>& args)
{
    double result = std::numeric_limits<double>::infinity();

    // Note: We follow the JavaScript standard where Math.min(2,NaN) is NaN.  C++ std::min returns 2
    for (auto& m : args) {
        auto value = m.asNumber();
        if (std::isnan(value))
            return value;
        result = std::min(result, m.asNumber());
    }

    return result;
}

static Object
mathMax(const std::vector<Object>& args)
{
    double result = -1 * std::numeric_limits<double>::infinity();

    // Note: We follow the JavaScript standard where Math.min(2,NaN) is NaN.  C++ std::max returns 2
    for (auto& m : args) {
        auto value = m.asNumber();
        if (std::isnan(value))
            return value;
        result = std::max(result, m.asNumber());
    }

    return result;
}

static Object
mathClamp(const std::vector<Object>& args)
{
    if (args.size() != 3)
        return std::numeric_limits<double>::quiet_NaN();

    double x = args[0].asNumber();
    double y = args[1].asNumber();
    double z = args[2].asNumber();

    return std::max(x, std::min(y,z));
}


static Object
mathRandom(const std::vector<Object>& args)
{
#ifdef HAVE_DECL_ARC4RANDOM
    return arc4random() / ((double) ULONG_MAX);
#else
    return ((unsigned long)random()) / ((double) ULONG_MAX);
#endif
}


// Convenience template because many math functions take a single argument
template<double (*f)(double)>
Object mathSingle(const std::vector<Object>& args)
{
    if (args.size() != 1)
        return std::numeric_limits<double>::quiet_NaN();
    return f(args[0].asNumber());
}

// Convenience template for string transformation
// TODO: These are not unicode safe...
template<int (*f)(int)>
Object stringTransform(const std::vector<Object>& args)
{
    if (args.size() != 1)
        return Object::NULL_OBJECT();

    std::string s = args[0].asString();
    std::string d;

    d.resize(s.size());
    std::transform(s.begin(), s.end(), d.begin(),  f);
    return d;
}

static Object
stringSlice(const std::vector<Object>& args)
{
    if (args.size() < 2)
        return Object::NULL_OBJECT();

    std::string s = args[0].asString();
    int len = s.size();
    int start = args[1].asNumber();
    int end = (args.size() >= 3 ? args[2].asNumber() : len);

    // Handle negative starting offset (from end) and clip to the start of the string.
    if (start < 0) start += len;
    start = std::max(0, std::min(start, len));

    // Handle negative ending offset (from end of string) and clip to the end of the string
    if (end < 0) end += len;
    end = std::max(0, std::min(end, len));

    if (end <= start)
        return "";

    return s.substr(start, end - start);
}

Object
timeExtractYear(const std::vector<Object>& args)
{
    if (args.size() != 1)
        return Object::NULL_OBJECT();

    auto t = static_cast<apl_time_t>(args[0].asNumber());
    return Object(static_cast<double>(time::yearFromTime(t)));
}

Object
timeExtractMonth(const std::vector<Object>& args)
{
    if (args.size() != 1)
        return Object::NULL_OBJECT();

    auto t = static_cast<apl_time_t>(args[0].asNumber());
    return Object(static_cast<double>(time::monthFromTime(t)));
}

Object
timeExtractDate(const std::vector<Object>& args)
{
    if (args.size() != 1)
        return Object::NULL_OBJECT();

    long t = static_cast<long>(args[0].asNumber());
    return Object(static_cast<double>(time::dateFromTime(t)));
}

Object
timeExtractWeekDay(const std::vector<Object>& args)
{
    if (args.size() != 1)
        return Object::NULL_OBJECT();

    long t = static_cast<long>(args[0].asNumber());
    return Object(static_cast<double>((t / time::MS_PER_DAY + 4) % 7));
}

template<long divisor, long modulus> Object
timeExtract(const std::vector<Object>&args )
{
    if (args.size() != 1)
        return Object::NULL_OBJECT();

    long t = static_cast<long>(args[0].asNumber());
    return Object(static_cast<double>((t / divisor) % modulus));
}

Object
timeFormat(const std::vector<Object>& args)
{
    if (args.size() != 2)
        return Object::NULL_OBJECT();

    return Object(timegrammar::timeToString(args.at(0).asString(), args.at(1).asNumber()));
}

void
createStandardFunctions(Context& context) {
    auto map = std::make_shared<ObjectMap>();
    auto smap = std::make_shared<ObjectMap>();
    auto tmap = std::make_shared<ObjectMap>();

    map->emplace("min", mathMin);
    map->emplace("max", mathMax);
    map->emplace("clamp", mathClamp);
    map->emplace("abs", mathSingle<std::abs>);
    map->emplace("ceil", mathSingle<std::ceil>);
    map->emplace("floor", mathSingle<std::floor>);
    map->emplace("round", mathSingle<std::round>);
    map->emplace("acos", mathSingle<std::acos>);
    map->emplace("asin", mathSingle<std::asin>);
    map->emplace("atan", mathSingle<std::atan>);
    map->emplace("cos", mathSingle<std::cos>);
    map->emplace("sin", mathSingle<std::sin>);
    map->emplace("tan", mathSingle<std::tan>);
    map->emplace("sqrt", mathSingle<std::sqrt>);
    map->emplace("PI", M_PI);
    map->emplace("random", mathRandom);

    smap->emplace("toLowerCase", stringTransform<::tolower>);
    smap->emplace("toUpperCase", stringTransform<::toupper>);
    smap->emplace("slice", stringSlice);

    tmap->emplace("year", timeExtractYear);
    tmap->emplace("month", timeExtractMonth);
    tmap->emplace("date", timeExtractDate);
    tmap->emplace("weekDay", timeExtractWeekDay);
    tmap->emplace("hours", timeExtract<time::MS_PER_HOUR, time::HOURS_PER_DAY>);
    tmap->emplace("minutes", timeExtract<time::MS_PER_MINUTE, time::MINUTES_PER_HOUR>);
    tmap->emplace("seconds", timeExtract<time::MS_PER_SECOND, time::SECONDS_PER_MINUTE>);
    tmap->emplace("milliseconds", timeExtract<1, time::MS_PER_SECOND>);
    tmap->emplace("format", timeFormat);

    context.putConstant("Math", map);
    context.putConstant("String", smap);
    context.putConstant("Time", tmap);
}

}  // namespace apl
