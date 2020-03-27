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
#ifdef APL_CORE_UWP
#include <random>
#endif
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

#ifdef APL_CORE_UWP
static std::random_device random_device;
static std::mt19937 generator(random_device());
static unsigned long random() { return generator(); }
#endif

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

    apl_time_t t = static_cast<apl_time_t>(args[0].asNumber());
    return Object(static_cast<apl_time_t>(time::dateFromTime(t)));
}

Object
timeExtractWeekDay(const std::vector<Object>& args)
{
    if (args.size() != 1)
        return Object::NULL_OBJECT();

    auto daysSinceEpoch = static_cast<int>( args[0].asNumber() / time::MS_PER_DAY );
    return Object( static_cast<apl_time_t>((daysSinceEpoch + 4) % 7) );
}

template<long divisor, long modulus> Object
timeExtract(const std::vector<Object>&args )
{
    if (args.size() != 1)
        return Object::NULL_OBJECT();

    return Object(std::fmod(std::floor(args[0].asNumber() / divisor), modulus));
}

Object
timeFormat(const std::vector<Object>& args)
{
    if (args.size() != 2)
        return Object::NULL_OBJECT();

    return Object(timegrammar::timeToString(args.at(0).asString(), args.at(1).asNumber()));
}

static std::shared_ptr<ObjectMap> createMathMap()
{
    auto map = std::make_shared<ObjectMap>();

    map->emplace("min", Function::create("min", mathMin));
    map->emplace("max", Function::create("max", mathMax));
    map->emplace("clamp", Function::create("clamp", mathClamp));
    map->emplace("abs", Function::create("abs", mathSingle<std::abs>));
    map->emplace("ceil", Function::create("ceil", mathSingle<std::ceil>));
    map->emplace("floor", Function::create("floor", mathSingle<std::floor>));
    map->emplace("round", Function::create("round", mathSingle<std::round>));
    map->emplace("acos", Function::create("acos", mathSingle<std::acos>));
    map->emplace("asin", Function::create("asin", mathSingle<std::asin>));
    map->emplace("atan", Function::create("atan", mathSingle<std::atan>));
    map->emplace("cos", Function::create("cos", mathSingle<std::cos>));
    map->emplace("sin", Function::create("sin", mathSingle<std::sin>));
    map->emplace("tan", Function::create("tan", mathSingle<std::tan>));
    map->emplace("sqrt", Function::create("sqrt", mathSingle<std::sqrt>));
    map->emplace("PI", M_PI);
    map->emplace("random", Function::create("random", mathRandom, false));

    return map;
}

static std::shared_ptr<ObjectMap> createStringMap()
{
    auto map = std::make_shared<ObjectMap>();

    map->emplace("toLowerCase", Function::create("toLower", stringTransform<::tolower>));
    map->emplace("toUpperCase", Function::create("toUpper", stringTransform<::toupper>));
    map->emplace("slice", Function::create("slice", stringSlice));

    return map;
}

static std::shared_ptr<ObjectMap> createTimeMap()
{
    auto map = std::make_shared<ObjectMap>();

    map->emplace("year", Function::create("year", timeExtractYear));
    map->emplace("month", Function::create("year", timeExtractMonth));
    map->emplace("date", Function::create("year", timeExtractDate));
    map->emplace("weekDay", Function::create("year", timeExtractWeekDay));
    map->emplace("hours", Function::create("year", timeExtract<time::MS_PER_HOUR, time::HOURS_PER_DAY>));
    map->emplace("minutes", Function::create("year", timeExtract<time::MS_PER_MINUTE, time::MINUTES_PER_HOUR>));
    map->emplace("seconds", Function::create("year", timeExtract<time::MS_PER_SECOND, time::SECONDS_PER_MINUTE>));
    map->emplace("milliseconds", Function::create("year", timeExtract<1, time::MS_PER_SECOND>));
    map->emplace("format", Function::create("year", timeFormat));

    return map;
}

void
createStandardFunctions(Context& context)
{
    static auto sMathFunctions = createMathMap();
    static auto sStringFunctions = createStringMap();
    static auto sTimeFunctions = createTimeMap();

    context.putConstant("Math", sMathFunctions);
    context.putConstant("String", sStringFunctions);
    context.putConstant("Time", sTimeFunctions);
}

}  // namespace apl
