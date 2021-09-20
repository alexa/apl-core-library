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

#include <algorithm>
#include <cmath>
#ifdef APL_CORE_UWP
#include <random>
#endif
#include <stdint.h>
#include <stdlib.h>

#include "apl/animation/coreeasing.h"
#include "apl/engine/context.h"
#include "apl/primitives/functions.h"
#include "apl/primitives/object.h"
#include "apl/primitives/rangegenerator.h"
#include "apl/primitives/slicegenerator.h"
#include "apl/primitives/timefunctions.h"
#include "apl/primitives/timegrammar.h"
#include "apl/primitives/unicode.h"
#include "apl/utils/random.h"

namespace apl {

static Object
mathMin(const ObjectArray& args)
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
mathMax(const ObjectArray& args)
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
mathClamp(const ObjectArray& args)
{
    if (args.size() != 3)
        return std::numeric_limits<double>::quiet_NaN();

    double x = args[0].asNumber();
    double y = args[1].asNumber();
    double z = args[2].asNumber();

    return std::max(x, std::min(y,z));
}

#ifdef APL_CORE_UWP
    static std::mt19937 generator = Random::mt32Generator();
#endif

static Object
mathRandom(const ObjectArray& args)
{
#ifdef HAVE_DECL_ARC4RANDOM
    // By contract, arc4random can return any valid uint32_t value
    return arc4random() / ((double) UINT32_MAX);
#elif APL_CORE_UWP
    // By contract, std::mt19937 returns values between 0 and mt19937::max()
    return generator() / ((double) std::mt19937::max());
#else
    // By contract, random() returns values between 0 and RAND_MAX
     return random() / ((double) RAND_MAX);
#endif
}


// Convenience template because many math functions take a single argument
template<double (*f)(double)>
Object mathSingle(const ObjectArray& args)
{
    if (args.size() != 1)
        return std::numeric_limits<double>::quiet_NaN();
    return f(args[0].asNumber());
}

// Convenience template because some math functions take two arguments
template<double (*f)(double, double)>
Object mathDouble(const std::vector<Object>& args)
{
    if (args.size() != 2)
        return std::numeric_limits<double>::quiet_NaN();
    return f(args[0].asNumber(), args[1].asNumber());
}

static Object
mathHypot(const std::vector<Object>& args)
{
    double result = 0.0;
    for (const auto& m : args) {
        auto value = m.asNumber();
        result += value * value;
    }
    return std::sqrt(result);
}

static Object
mathSign(const std::vector<Object>& args)
{
    if (args.size() != 1)
        return std::numeric_limits<double>::quiet_NaN();
    auto v = args.at(0).asNumber();
    if (v == 0)
        return 0;
    return v < 0 ? -1 : 1;
}


template<bool (*f)(double)>
Object
mathPredicate(const std::vector<Object>& args)
{
    if (args.size() != 1)
        return false;
    return f(args[0].asNumber());
}

static Object
mathToFloat(const std::vector<Object>& args)
{
    if (args.size() != 1)
        return std::numeric_limits<double>::quiet_NaN();
    return args.at(0).asNumber();
}

static Object
mathToInt(const std::vector<Object>& args)
{
    if (args.size() < 1 || args.size() > 2)
        return std::numeric_limits<double>::quiet_NaN();

    auto base = args.size() == 2 ? args.at(1).asInt() : 10;
    if (base < 0 || base == 1 || base > 36)
        return std::numeric_limits<double>::quiet_NaN();

    return args.at(0).asInt64(base);
}

static Object
arrayIndexOf(const ObjectArray& args)
{
    if (args.size() < 2)
        return -1;

    const auto& array = args.at(0);
    if (!array.isArray())
        return -1;

    const auto len = array.size();
    const auto& value = args.at(1);
    for (std::uint64_t index = 0; index < len; index++)
        if (array.at(index) == value)
            return index;

    return -1;
}

static Object
arrayRange(const ObjectArray& args)
{
    if (args.size() < 1)
        return RangeGenerator::create(0, 0, 0);

    auto a = args.at(0).asNumber();
    if (!std::isfinite(a))
        return RangeGenerator::create(0, 0, 0);
    if (args.size() == 1)
        return RangeGenerator::create(0, a, 1);

    auto b = args.at(1).asNumber();
    if (!std::isfinite(b))
        return RangeGenerator::create(0, 0, 0);
    if (args.size() == 2)
        return RangeGenerator::create(a, b, 1);

    auto c = args.at(2).asNumber();
    if (!std::isfinite(c))
        return RangeGenerator::create(0, 0, 0);
    return RangeGenerator::create(a, b, c);
}

static Object
arraySlice(const ObjectArray& args)
{
    if (args.size() < 1)
        return Object::EMPTY_ARRAY();

    const auto& array = args.at(0);
    if (!array.isArray())
        return Object::EMPTY_ARRAY();

    if (args.size() == 1)
        return array;

    auto start = args.at(1).asInt64();
    auto end = args.size() > 2 ? args.at(2).asInt64() : array.size();
    return SliceGenerator::create(array, start, end);
}

/**
 * This method assumes a UTF-8 encoded string. It returns the number
 * of code points in the string.
 */
static Object
stringLength(const std::vector<Object>& args)
{
    if (args.size() < 1)
        return Object::NULL_OBJECT();

    std::string s = args[0].asString();
    return utf8StringLength(s);
}

static Object
stringToUpperImpl(const std::shared_ptr<LocaleMethods>& localeMethods, const std::vector<Object>& args)
{
    // passing empty string as locale is not provided in the request
    // ex. “${String.toUpperCase(‘TestString’)}”
    if (args.size() == 1)
        return localeMethods->toUpperCase(args[0].asString(), "");

    // passing locale value provided in the request
    // ex. “${String.toUpperCase(‘TestString’, ‘fr-FR’)}”
    if (args.size() == 2)
        return localeMethods->toUpperCase(args[0].asString(), args[1].asString());

    return Object::NULL_OBJECT();
}

static Object
stringToLowerImpl(const std::shared_ptr<LocaleMethods>& localeMethods, const std::vector<Object>& args)
{
    // passing empty string as locale is not provided in the request
    // ex. “${String.toLowerCase(‘TestString’)}”
    if (args.size() == 1)
        return localeMethods->toLowerCase(args[0].asString(), "");

    // passing locale value provided in the request
    // ex. “${String.toLowerCase(‘TestString’, ‘fr-FR’)}”
    if (args.size() == 2)
        return localeMethods->toLowerCase(args[0].asString(), args[1].asString());

    return Object::NULL_OBJECT();
}

/**
 * This method assumes a UTF-8 encoded string.  It returns a substring
 * based on code points, not extended grapheme clusters.
 */
static Object
stringSlice(const std::vector<Object>& args)
{
    if (args.size() < 2)
        return Object::NULL_OBJECT();

    std::string s = args[0].asString();
    int start = args[1].asInt();
    int end = (args.size() >= 3 ? args[2].asInt() : std::numeric_limits<int>::max());

    return utf8StringSlice(s, start, end);
}

Object
timeExtractYear(const ObjectArray& args)
{
    if (args.size() != 1)
        return Object::NULL_OBJECT();

    auto t = static_cast<apl_time_t>(args[0].asNumber());
    return Object(static_cast<double>(time::yearFromTime(t)));
}

Object
timeExtractMonth(const ObjectArray& args)
{
    if (args.size() != 1)
        return Object::NULL_OBJECT();

    auto t = static_cast<apl_time_t>(args[0].asNumber());
    return Object(static_cast<double>(time::monthFromTime(t)));
}

Object
timeExtractDate(const ObjectArray& args)
{
    if (args.size() != 1)
        return Object::NULL_OBJECT();

    auto t = static_cast<apl_time_t>(args[0].asNumber());
    return Object(static_cast<apl_time_t>(time::dateFromTime(t)));
}

Object
timeExtractWeekDay(const ObjectArray& args)
{
    if (args.size() != 1)
        return Object::NULL_OBJECT();

    auto daysSinceEpoch = static_cast<int>( args[0].asNumber() / time::MS_PER_DAY );
    return Object( static_cast<apl_time_t>((daysSinceEpoch + 4) % 7) );
}

template<long divisor, long modulus> Object
timeExtract(const ObjectArray& args)
{
    if (args.size() != 1)
        return Object::NULL_OBJECT();

    return Object(std::fmod(std::floor(args[0].asNumber() / divisor), modulus));
}

Object
timeFormat(const ObjectArray& args)
{
    if (args.size() != 2)
        return Object::NULL_OBJECT();

    return Object(timegrammar::timeToString(args.at(0).asString(), args.at(1).asNumber()));
}

static ObjectMapPtr
createMathMap()
{
    auto map = std::make_shared<ObjectMap>();

    map->emplace("abs", Function::create("abs", mathSingle<std::abs>));
    map->emplace("acos", Function::create("acos", mathSingle<std::acos>));
    map->emplace("acosh", Function::create("acosh", mathSingle<std::acosh>));
    map->emplace("asin", Function::create("asin", mathSingle<std::asin>));
    map->emplace("asinh", Function::create("asinh", mathSingle<std::asinh>));
    map->emplace("atan", Function::create("atan", mathSingle<std::atan>));
    map->emplace("atanh", Function::create("atanh", mathSingle<std::atanh>));
    map->emplace("atan2", Function::create("atan2", mathDouble<std::atan2>));

    map->emplace("cbrt", Function::create("cbrt", mathSingle<std::cbrt>));
    map->emplace("ceil", Function::create("ceil", mathSingle<std::ceil>));
    map->emplace("clamp", Function::create("clamp", mathClamp));
    map->emplace("cos", Function::create("cos", mathSingle<std::cos>));
    map->emplace("cosh", Function::create("cosh", mathSingle<std::cosh>));

    map->emplace("exp", Function::create("exp", mathSingle<std::exp>));
    map->emplace("exp2", Function::create("exp2", mathSingle<std::exp2>));
    map->emplace("expm1", Function::create("expm1", mathSingle<std::expm1>));

    map->emplace("float", Function::create("float", mathToFloat));
    map->emplace("floor", Function::create("floor", mathSingle<std::floor>));

    map->emplace("hypot", Function::create("hypot", mathHypot));

    map->emplace("int", Function::create("int", mathToInt));
    map->emplace("isFinite", Function::create("isFinite", mathPredicate<std::isfinite>));
    map->emplace("isInf", Function::create("isInf", mathPredicate<std::isinf>));
    map->emplace("isNaN", Function::create("isNan", mathPredicate<std::isnan>));

    map->emplace("log", Function::create("log", mathSingle<std::log>));  // ln(x)
    map->emplace("log1p", Function::create("log1p", mathSingle<std::log1p>));  // ln(1+x)
    map->emplace("log10", Function::create("log10", mathSingle<std::log10>));  // log_10(x)
    map->emplace("log2", Function::create("log2", mathSingle<std::log2>));  // log_2(x)

    map->emplace("max", Function::create("max", mathMax));
    map->emplace("min", Function::create("min", mathMin));

    map->emplace("pow", Function::create("pow", mathDouble<std::pow>));

    map->emplace("random", Function::create("random", mathRandom, false));
    map->emplace("round", Function::create("round", mathSingle<std::round>));

    map->emplace("sign", Function::create("sign", mathSign));
    map->emplace("sin", Function::create("sin", mathSingle<std::sin>));
    map->emplace("sinh", Function::create("sinh", mathSingle<std::sinh>));
    map->emplace("sqrt", Function::create("sqrt", mathSingle<std::sqrt>));

    map->emplace("tan", Function::create("tan", mathSingle<std::tan>));
    map->emplace("tanh", Function::create("tanh", mathSingle<std::tanh>));
    map->emplace("trunc", Function::create("trunc", mathSingle<std::trunc>));

    map->emplace("E", M_E);
    map->emplace("LN2", M_LN2);
    map->emplace("LN10", M_LN10);
    map->emplace("LOG2E", M_LOG2E);
    map->emplace("LOG10E", M_LOG10E);
    map->emplace("PI", M_PI);
    map->emplace("SQRT1_2", M_SQRT1_2);
    map->emplace("SQRT2", M_SQRT2);

    return map;
}

static ObjectMapPtr
createStringMap(const std::shared_ptr<LocaleMethods>& localeMethods)
{
    auto map = std::make_shared<ObjectMap>();

    map->emplace("toLowerCase", Function::create("toLower", [localeMethods] (const std::vector<Object>& args) {
        return stringToLowerImpl(localeMethods, args);
    }));
    map->emplace("toUpperCase", Function::create("toUpper", [localeMethods] (const std::vector<Object>& args) {
        return stringToUpperImpl(localeMethods, args);
    }));
    map->emplace("slice", Function::create("slice", stringSlice));
    map->emplace("length", Function::create("length", stringLength));

    return map;
}

static ObjectMapPtr
createTimeMap()
{
    auto map = std::make_shared<ObjectMap>();

    map->emplace("year", Function::create("year", timeExtractYear));
    map->emplace("month", Function::create("month", timeExtractMonth));
    map->emplace("date", Function::create("date", timeExtractDate));
    map->emplace("weekDay", Function::create("weekDay", timeExtractWeekDay));
    map->emplace("hours", Function::create("hours", timeExtract<time::MS_PER_HOUR, time::HOURS_PER_DAY>));
    map->emplace("minutes", Function::create("minutes", timeExtract<time::MS_PER_MINUTE, time::MINUTES_PER_HOUR>));
    map->emplace("seconds", Function::create("seconds", timeExtract<time::MS_PER_SECOND, time::SECONDS_PER_MINUTE>));
    map->emplace("milliseconds", Function::create("milliseconds", timeExtract<1, time::MS_PER_SECOND>));
    map->emplace("format", Function::create("format", timeFormat));

    return map;
}

static ObjectMapPtr
createArrayMap()
{
    auto map = std::make_shared<ObjectMap>();

    map->emplace("indexOf", Function::create("indexOf", arrayIndexOf));
    map->emplace("range", Function::create("range", arrayRange));
    map->emplace("slice", Function::create("slice", arraySlice));

    return map;
}

void
createStandardFunctions(Context& context)
{
    static auto sArrayFunctions = createArrayMap();
    static auto sMathFunctions = createMathMap();
    // String functions are dependent on RootConfig locale methods
    auto sStringFunctions = createStringMap(context.getLocaleMethods());
    static auto sTimeFunctions = createTimeMap();

    context.putConstant("Array", sArrayFunctions);
    context.putConstant("Math", sMathFunctions);
    context.putConstant("String", sStringFunctions);
    context.putConstant("Time", sTimeFunctions);
}

}  // namespace apl
