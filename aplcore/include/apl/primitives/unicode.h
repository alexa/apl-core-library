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
 * Slice a UTF-8 string
 * @param utf8String A reference to a std::string holding UTF-8 data
 * @param start The offset of the first code point
 * @param end The offset of the last code point.  If negative, count from the end of the string
 * @return The extracted code points.  May be empty.
 */
std::string utf8StringSlice(const std::string& utf8String, int start, int end = std::numeric_limits<int>::max());

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
