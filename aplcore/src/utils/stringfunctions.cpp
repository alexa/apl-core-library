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

#include "apl/utils/stringfunctions.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <locale>

namespace apl {

std::string
rtrim(const std::string &str)
{
    std::string output(str);
    output.erase(std::find_if(output.rbegin(), output.rend(), [](unsigned char ch) {
                   return !sutil::isspace(ch);
                 }).base(),
                 output.end());
    return output;
}

std::string
ltrim(const std::string &str)
{
    std::string output(str);
    output.erase(output.begin(), std::find_if(output.begin(), output.end(), [](unsigned char ch) {
        return !sutil::isspace(ch);
    }));
    return output;
}

std::string
trim(const std::string &str)
{
    return rtrim(ltrim(str));
}

std::string
rpad(const std::string &str, std::size_t minWidth, char padChar)
{
    auto slen = str.length();
    if (slen >= minWidth) return str;

    return str + std::string(minWidth - slen, padChar);
}

std::string
lpad(const std::string &str, std::size_t minWidth, char padChar)
{
    auto slen = str.length();
    if (slen >= minWidth) return str;

    return std::string(minWidth - slen, padChar) + str;
}

std::string
tolower(const std::string& str)
{
    std::string output = str;

    std::transform(output.begin(), output.end(), output.begin(), [](unsigned char ch) {
      return sutil::tolower(ch);
    });

    return output;
}

std::string
doubleToAplFormattedString(double value)
{
    if (value < static_cast<double>(std::numeric_limits<std::int64_t>::max())
        && value > static_cast<double>(std::numeric_limits<std::int64_t>::min())) {
        auto iValue = static_cast<std::int64_t>(value);
        if (value == iValue)
            return std::to_string(iValue);
    }

    auto s = sutil::to_string(value);
    auto it = s.find_last_not_of('0');
    if (it != s.find(sutil::DECIMAL_POINT))   // Remove a trailing decimal point
        it++;
    s.erase(it, std::string::npos);
    return s;
}

double
aplFormattedStringToDouble(const std::string& string)
{
    auto len = string.size();
    auto idx = len;
    double result = sutil::stod(string, &idx);
    // Handle percentages.  We skip over whitespace and stop on any other character
    while (idx < len) {
        auto c = string[idx];
        if (c == '%') {
            result *= 0.01;
            break;
        }
        if (!sutil::isspace(c))
            break;
        idx++;
    }
    return result;
}

namespace sutil {

// ---- Internal utilities for parsing/formatting floating-point values

/**
 * Tries to consume the candidate token characters (case-insensitive) from the input string starting
 * at the specified offset position. If the candidate token matches the next characters, the offset
 * is updated to point to the next offset after the match. If the candidate token does not match,
 * the offset isn't updated.
 *
 * @param s The input string
 * @param offset The offset into the input string to start matching
 * @param candidate The candidate token to consume (must be all lowercase)
 * @return @c true if the candidate token matches the next characters in the string, @c false otherwise.
 */
bool
consumeToken(const std::string& s, std::size_t* offset, const char* candidate)
{
    auto slen = s.length();
    auto clen = strlen(candidate);
    for (std::size_t i = 0; i < clen; i++) {
        std::size_t index = *offset + i;
        if (index >= slen)
            return false;
        if (sutil::tolower(s.at(index)) != candidate[i])
            return false;
    }

    *offset = *offset + clen;
    return true;
}

/**
 * Tries to consume at least one digit from the input stream starting at the specified offset.
 * If successful, the offset if update to point to the first non-digit character after the match.
 * If no digit was matched, the offset isn't updated.
 *
 * @param s The input string
 * @param offset The offset into the input string to start matching
 * @param base The base in which the expected digits are expressed
 * @return @c true if at least one digit was matched, @c false otherwise.
 */
bool
consumeDigits(const std::string& s, std::size_t* offset, int base = 10)
{
    static const char* digits = "0123456789ABCDEF";
    auto slen = s.length();
    auto index = *offset;

    while (index < slen) {
        char normalizedChar = sutil::toupper(s.at(index));
        const char* digit = std::strchr(digits, normalizedChar);
        auto charIndex = digit - digits;
        if (digit && charIndex < base) {
            index += 1;
        } else {
            break;
        }
    }

    if (index > *offset) {
        *offset = index;
        return true;
    } else {
        return false;
    }
}

/**
 * Tries to consume a +/- sign from the specified string at the specified offset, and returns
 * the sign as +1 or -1. If no sign could be read, returns +1. If a sign is read, the offset
 * is updated to point to the character following the sign character.
 *
 * @param s The input string
 * @param offset The offset from which to read.
 * @return The sign that was read, or +1 if no sign character was present.
 */
int
consumeSign(const std::string& s, std::size_t* offset)
{
    auto index = *offset;
    if (s[index] == '+') {
        *offset += 1;
        return 1;
    } else if (s[index] == '-') {
        *offset += 1;
        return -1;
    }

    return 1;
}

template <typename T>
T
parseFloatingPointLiteral(const std::string& str, std::size_t* pos)
{
    auto len = str.length();
    std::size_t firstChar = 0;
    while (firstChar < len && sutil::isspace(str.at(firstChar))) {
        firstChar += 1;
    }

    if (firstChar >= len) {
        if (pos) *pos = len;
        return std::numeric_limits<T>::quiet_NaN();
    }

    std::size_t current = firstChar;

    // Parse sign, if present
    int sign = consumeSign(str, &current);

    if (consumeToken(str, &current, "infinity") || consumeToken(str, &current, "inf")) {
        if (pos) *pos = current;
        return sign * std::numeric_limits<T>::infinity();
    }

    if (consumeToken(str, &current, "nan")) {
        if (pos) *pos = current;
        return std::numeric_limits<T>::quiet_NaN();
    }

    int base;
    if (consumeToken(str, &current, "0x")) {
        base = 16;
    } else {
        base = 10;
    }

    auto significandStart = current;
    T value = 0;
    bool needDecimalSep = false;
    if (consumeDigits(str, &current, base)) {
        // No digits found, invalid number
        value = std::stoll(str.substr(significandStart, current - significandStart), nullptr, base);
    } else {
        // No digits found, the next character must be '.'
        needDecimalSep = true;
    }

    // Check for a fractional part
    if (consumeToken(str, &current, ".")) {
        auto fractionalStart = current;
        if (consumeDigits(str, &current, base)) {
            auto count = static_cast<int>(current - fractionalStart);
            auto fractional = std::stoll(str.substr(fractionalStart, count), nullptr, base);
            value += fractional / std::pow(base, count);
        }
    } else if (needDecimalSep) {
        return std::numeric_limits<T>::quiet_NaN();
    }

    // Parse exponent. Base 10 uses 'e' whereas base 16 uses 'p' as a delimiter
    if (base == 10 && consumeToken(str, &current, "e")) {
        auto exponentSign = consumeSign(str, &current);

        auto exponentStart = current;
        if (!consumeDigits(str, &current, base)) {
            // exponent has to have at least one decimal digit
            return std::numeric_limits<T>::quiet_NaN();
        }

        auto exponent = std::stoll(str.substr(exponentStart, current - exponentStart));
        value = value * std::pow(10, exponentSign * exponent);
    } else if (base == 16 && consumeToken(str, &current, "p")) {
        auto exponentSign = consumeSign(str, &current);

        auto exponentStart = current;
        if (!consumeDigits(str, &current, 10)) {
            // exponent has to have at least one decimal digit
            return std::numeric_limits<T>::quiet_NaN();
        }

        // exponent is base 10 even when parsing as hex
        auto exponent = std::stoll(str.substr(exponentStart, current - exponentStart), nullptr, 10);
        value = value * std::pow(2, exponentSign * exponent);
    }

    if (pos) *pos = current;
    return sign * value;
}

template <typename T>
std::string
format(T value) {
    if (std::isnan(value)) {
        return "nan";
    } else if (std::isinf(value)) {
        return value > 0 ? "inf" : "-inf";
    }

    auto normalized = std::abs(value);
    auto integerPart = (std::int64_t) std::abs(normalized);
    // We want 6 digits after the decimal point
    static const char *digits = "0123456789";
    static const int numDigits = 6;
    static const int multiplier = std::pow(10, numDigits);
    auto fractionalPart = std::abs(std::llround((normalized - integerPart) * multiplier));
    if (fractionalPart >= multiplier) {
        // Rounding caused overflow (e.g. 9.9999...9 -> "10.000000")
        integerPart += 1;
        fractionalPart -= multiplier;
    }
    std::string fractionalStr = "";
    for (int i = 0; i < numDigits; i++) {
        auto digit = fractionalPart % 10;
        fractionalStr = digits[digit] + fractionalStr;
        fractionalPart = fractionalPart / 10;
    }
    auto sign = value < 0 ? "-" : "";
    return sign + std::to_string(integerPart) + "." + rpad(fractionalStr, numDigits, '0');
}

// ---- End of internal utilities

float
stof(const std::string& str, std::size_t* pos)
{
    return parseFloatingPointLiteral<float>(str, pos);
}

double
stod(const std::string& str, std::size_t* pos)
{
    return parseFloatingPointLiteral<double>(str, pos);
}


long double
stold(const std::string& str, std::size_t* pos)
{
    return parseFloatingPointLiteral<long double>(str, pos);
}

/// Naive implementation of Numeric Conversions [string.conversions] spec.

struct ErrnoPreserve {
    ErrnoPreserve() : mErrno(errno) { errno = 0; }
    ~ErrnoPreserve() { if (errno == 0) errno = mErrno; }
    int mErrno;
};

int
stoi(const std::string& str, std::size_t* pos, int base)
{
    ErrnoPreserve _errnoPreserve;

    int result;
    char* tEnd;
    const char* cStr = str.c_str();
    // On success, the function returns the converted integral number as a long int value.
    // If no valid conversion could be performed, a zero value is returned (0L).
    // If the value read is out of the range of representable values by a long int, the function
    // returns LONG_MAX or LONG_MIN (defined in <climits>), and errno is set to ERANGE.
    // Additionally we account for integer range.
    const auto temp = std::strtol(str.c_str(), &tEnd, base);
    if (tEnd == cStr ||
        errno == ERANGE ||
        (temp < (long)std::numeric_limits<int>::min() || temp > (long)std::numeric_limits<int>::max())) {
        return 0;
    }

    result = (int)temp;

    if (pos) {
        *pos = tEnd - cStr;
    }

    return result;
}

long long
stoll(const std::string& str, std::size_t* pos, int base)
{
    ErrnoPreserve _errnoPreserve;

    char* tEnd;
    const char* cStr = str.c_str();

    // On success, the function returns the converted integral number as a long long int value.
    // If no valid conversion could be performed, a zero value is returned (0LL).
    // If the value read is out of the range of representable values by a long long int, the
    // function returns LLONG_MAX or LLONG_MIN (defined in <climits>), and errno is set to ERANGE.
    const auto result = std::strtoll(cStr, &tEnd, base);
    if (tEnd == cStr ||
        errno == ERANGE) {
        return 0;
    }

    if (pos) {
        *pos = tEnd - cStr;
    }

    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////

std::string
to_string(float value)
{
    return format<float>(value);
}

std::string
to_string(double value)
{
    return format<double>(value);
}

std::string
to_string(long double value)
{
    return format<long double>(value);
}

bool
isalnum(char c)
{
    return std::isalnum(c, std::locale::classic());
}

bool
isalnum(unsigned char c)
{
    return std::isalnum((char) c, std::locale::classic());
}

bool
isspace(char c)
{
    return std::isspace(c, std::locale::classic());
}

bool
isspace(unsigned char c)
{
    return std::isspace((char) c, std::locale::classic());
}

bool
isupper(char c)
{
    return std::isupper(c, std::locale::classic());
}

bool
isupper(unsigned char c)
{
    return std::isupper((char) c, std::locale::classic());
}

bool
islower(char c)
{
    return std::islower(c, std::locale::classic());
}

bool
islower(unsigned char c)
{
    return std::islower((char) c, std::locale::classic());
}

char
tolower(char c)
{
    return std::tolower(c, std::locale::classic());
}

unsigned char
tolower(unsigned char c)
{
    return std::tolower((char) c, std::locale::classic());
}

char
toupper(char c)
{
    return std::toupper(c, std::locale::classic());
}

unsigned char
toupper(unsigned char c)
{
    return std::toupper((char) c, std::locale::classic());
}


} // namespace sutil

}
