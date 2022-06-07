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

#ifndef APL_STRINGFUNCTIONS_H
#define APL_STRINGFUNCTIONS_H

#include <cstddef>
#include <string>
#include <type_traits>

namespace apl {

/**
 * Remove white spaces on the right side of a string.
 *
 * @param str text to remove trail whitespaces.
 * @return copy of Str without white spaces at the end of the string.
 */
std::string rtrim(const std::string &str);

/**
 * Remove white spaces on the left side of a string.
 *
 * @param str text to remove starting whitespaces.
 * @return copy of Str without white spaces at the beginning of the string.
 */
std::string ltrim(const std::string &str);

/**
 * Remove white spaces at the beginning and end of the string.
 *
 * @param str text to remove starting and ending whitespaces.
 * @return copy of Str without white spaces at the beginning and ending of the string.
 */
std::string trim(const std::string &str);

/**
 * Pads the end of the specified string, if needed, with the padding character until the resulting
 * string reaches the minimum length. No padding is applied if the input string already has the
 * requested minimum length.
 *
 * @param str The string to pad
 * @param minWidth The minimal resulting string length
 * @param padChar The character to use when padding, defaults to ' '.
 * @return The padding string
 */
std::string rpad(const std::string &str, std::size_t minWidth, char padChar = ' ');

/**
 * Pads the beginning of the specified string, if needed, with the padding character until the
 * resulting string reaches the minimum length. No padding is applied if the input string already
 * has the requested minimum length.
 *
 * @param str The string to pad
 * @param minWidth The minimal resulting string length
 * @param padChar The character to use when padding, defaults to ' '.
 * @return The padding string
 */
std::string lpad(const std::string &str, std::size_t minWidth, char padChar = ' ');

/**
 * Internal utility to convert string to lowercase.

 * Applicable only to latin characters. It must not be used instead of corelocalemethods.
 * @param str string to process.
 * @return lowercase version of str.
 */
std::string tolower(const std::string& str);


/**
 * sutil:: functions are intended to be locale-independent versions of std:: functions of the same
 * name that are affected by the C locale.
 */
namespace sutil {

constexpr char DECIMAL_POINT = '.';

/**
 * Internal utility to convert parse the textual representation of a floating-point number with
 * a known format without risking being affected by the current C locale. Intended to be used
 * as a locale-invariant alternative to std::stof.
 *
 * @param str The string value to parse
 * @param pos If non-null, will be set to the index of the first non-parsed character in the input
 *            string.
 * @return the parsed value, or NaN if the string could not be parsed.
 */
float stof(const std::string& str, std::size_t* pos = nullptr);

/**
 * Internal utility to convert parse the textual representation of a floating-point number with
 * a known format without risking being affected by the current C locale. Intended to be used
 * as a locale-invariant alternative to std::stod.
 *
 * @param str The string value to parse
 * @param pos If non-null, will be set to the index of the first non-parsed character in the input
 *            string.
 * @return the parsed value, or NaN if the string could not be parsed.
 */
double stod(const std::string& str, std::size_t* pos = nullptr);

/**
 * Internal utility to convert parse the textual representation of a floating-point number with
 * a known format without risking being affected by the current C locale. Intended to be used
 * as a locale-invariant alternative to std::stold.
 *
 * @param str The string value to parse
 * @param pos If non-null, will be set to the index of the first non-parsed character in the input
 *            string.
 * @return the parsed value, or NaN if the string could not be parsed.
 */
long double stold(const std::string& str, std::size_t* pos = nullptr);


/**
 * Internal utility to format a single-precision value as a string. Intended to be used as a
 * locale-invariant alternative to std::to_string(double).
 *
 * @param value The value to format
 * @return the value formatted as a string
 */
std::string to_string(float value);

/**
 * Internal utility to format a double-precision value as a string. Intended to be used as a
 * locale-invariant alternative to std::to_string(double).
 *
 * @param value The value to format
 * @return the value formatted as a string
 */
std::string to_string(double value);

/**
 * Internal utility to format a extended precision value as a string. Intended to be used as a
 * locale-invariant alternative to std::to_string(long double).
 *
 * @param value The value to format
 * @return the value formatted as a string
 */
std::string to_string(long double value);


/**
 * Determines if a character is alphanumeric in the classic C locale.
 *
 * @param c The character to check
 * @return @c true if the provided character is alphanumeric in the classic C locale
 */
bool isalnum(char c);

/**
 * Determines if a character is alphanumeric in the classic C locale.
 *
 * @param c The character to check
 * @return @c true if the provided character is alphanumeric in the classic C locale
 */
bool isalnum(unsigned char c);

/**
 * Determines if a character is a whitespace character in the classic C locale.
 *
 * @param c The chacater to check
 * @return @c true of the character is a whitespace character, @c false otherwise
 */
bool isspace(char c);

/**
 * Determines if a character is a whitespace character in the classic C locale.
 *
 * @param c The chacater to check
 * @return @c true of the character is a whitespace character, @c false otherwise
 */
bool isspace(unsigned char c);

/**
 * Determines if a character is an uppercase character in the classic C locale.
 *
 * @param c The chacater to check
 * @return @c true of the character is an uppercase character, @c false otherwise
 */
bool isupper(char c);

/**
 * Determines if a character is an uppercase character in the classic C locale.
 *
 * @param c The chacater to check
 * @return @c true of the character is an uppercase character, @c false otherwise
 */
bool isupper(unsigned char c);

/**
 * Determines if a character is a lowercase character in the classic C locale.
 *
 * @param c The chacater to check
 * @return @c true of the character is a lowercase character, @c false otherwise
 */
bool islower(char c);

/**
 * Determines if a character is a lowercase character in the classic C locale.
 *
 * @param c The chacater to check
 * @return @c true of the character is a lowercase character, @c false otherwise
 */
bool islower(unsigned char c);

/**
 * Converts a character to lowercase according to the classic C locale.
 *
 * @param c The character to convert
 * @return The lowercase version of the character
 */
char tolower(char c);

/**
 * Converts a character to lowercase according to the classic C locale.
 *
 * @param c The character to convert
 * @return The lowercase version of the character
 */
unsigned char tolower(unsigned char c);

/**
 * Converts a character to uppercase according to the classic C locale.
 *
 * @param c The character to convert
 * @return The uppercase version of the character
 */
char toupper(char c);

/**
 * Converts a character to uppercase according to the classic C locale.
 *
 * @param c The character to convert
 * @return The uppercase version of the character
 */
unsigned char toupper(unsigned char c);


} // namespace sutil

}

#endif // APL_STRINGFUNCTIONS_H
