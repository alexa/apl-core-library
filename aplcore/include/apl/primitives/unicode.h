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

#ifndef _APL_UNICODE_H
#define _APL_UNICODE_H

#include <string>
#include <limits>

namespace apl {

/**
 * Count the number of code points in a UTF-8 string.
 * @param utf8String A reference to a std::string holding UTF-8 data
 * @return The number of code points in the string.  Return -1 if the string is malformed.
 */
int utf8StringLength(const std::string& utf8String);

/**
 * Count the number of code points in a uint8_t byte range containing UTF-8 data.
 * @param utf8StringPtr A pointer to the beginning of the uint8_t byte range where UTF-8 code points should be counted.
 * @param count The number of bytes in the uint8_t byte range to check when counting UTF-8 code points.
 * @return The number of code points in the string.  Return -1 if the string is malformed.
 */
int utf8StringLength(const uint8_t* utf8StringPtr, int count);

/**
 * Slice a UTF-8 string
 * @param utf8String A reference to a std::string holding UTF-8 data
 * @param start The offset of the first code point
 * @param end The offset of the last code point.  If negative, count from the end of the string
 * @return The extracted code points.  May be empty.
 */
std::string utf8StringSlice(const std::string& utf8String, int start, int end = std::numeric_limits<int>::max());

/**
 * Return a single character from a UTF-8 string
 * @param utf8String A reference to a std::string holding UTF-8 data
 * @param index The offset of the code point.  If negative, count from the end of the string
 * @return The extracted code point.  May be empty.
 */
std::string utf8StringCharAt(const std::string& utf8String, int index);

/**
 * Returns the index of the first occurrence of a specified substring.
 * @param utf8String A reference to a std::string to search within
 * @param utf8SearchString A reference to a std::string to search for
 * @param index The offset of the code point to start the search from. If negative, count from the end.
 * @param forwardSearch true if the search should progress forwards from the index, false if it
 *        should progress backwards from the index.
 * @return The offset of the code point where the substring first appears, or -1 if not found.
 */
int utf8StringIndexOf(const std::string& utf8String, const std::string& utf8SearchString, int index, bool forwardSearch);

/**
 * Replace first occurrence of a substring in a UTF-8 string with a replacement string.
 * @param utf8String The original string
 * @param utf8SearchString The substring to search for
 * @param utf8ReplaceString The replacement string
 * @param startIndex The position to start the search from (optional, default is 0)
 *                   If positive or zero, search starts from the beginning towards the end.
 *                   If negative, search starts from the end towards the beginning.
 * @return The string with the first occurrence of searchString replaced by replaceString
*/
std::string utf8StringReplace(const std::string& utf8String, const std::string& utf8SearchString, const std::string& utf8ReplaceString, int startIndex = 0);

/**
 * Replace all occurrences of a substring in a UTF-8 string with a replacement string.
 * @param utf8String The original string
 * @param utf8SearchString The substring to search for
 * @param utf8ReplaceString The replacement string
 * @return The string with all occurrences of searchString replaced by replaceString
*/
std::string utf8StringReplaceAll(const std::string& utf8String, const std::string& utf8SearchString, const std::string& utf8ReplaceString);

/**
 * Trim leading and trailing white space from a UTF-8 string.
 * @param utf8String A reference to a std::string holding UTF-8 data
 * @return The trimmed string
 */
std::string utf8StringTrimWhiteSpace(const std::string& utf8String);

/**
 * Strip invalid characters out of a UTF-8 string.  The "validCharacters" property in the EditText
 * component defines the schema for the valid character string.
 *
 * @param utf8String The starting string
 * @param validCharacters The valid set of characters
 * @return The stripped string
 */
std::string utf8StripInvalid(const std::string& utf8String, const std::string& validCharacters);

/**
 * Check if all characters in a string are valid.  The "validCharacters" property in the EditText
* component defines the schema for the valid character string.
*
 * @param utf8String A UTF-8 string
 * @param validCharacters The set of valid characters
 * @return True if all of the characters in utf8String are valid
 */
bool utf8ValidCharacters(const std::string& utf8String, const std::string& validCharacters);

/**
 * Check a single wchar_t character to see if it is a valid character.  The "validCharacters" property
 * in the EditText component defines the schema for the valid character string.
 *
 * @param wc The wchar_t character
 * @param validCharacters The valid set of characters
 * @return True if this is a valid character
 */
bool wcharValidCharacter(wchar_t wc, const std::string& validCharacters);

/**
 * Trim the length of a UTF-8 string to a maximum number of code points (not bytes).
 * @param utf8String The string to trim in place.
 * @param maxLength The maximum number of allowed code points.  If zero, no trimming occurs.
 * @return True if the string was trimmed
 */
bool utf8StringTrim(std::string& utf8String, int maxLength);

/**
 * Strip the string of invalid characters and trim the length to a maximum number of code points.
 * @param utf8String The string to strip and trim.
 * @param validCharacters The set of valid characters.  If empty, all characters are valid.
 * @param maxLength The maximum number of code points.  If zero, no trimming occurs.
 * @return The stripped and trimmed string.
 */
std::string utf8StripInvalidAndTrim(const std::string& utf8String, const std::string& validCharacters, int maxLength);

} // namespace apl

#endif // _APL_UNICODE_H
