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

} // namespace apl

#endif // _APL_UNICODE_H
