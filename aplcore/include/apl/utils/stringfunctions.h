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

#include <string>

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
 * Internal utility to convert string to lowercase.

 * Applicable only to latin characters. It must not used instead of corelocalemethods.
 * @param str string to process.
 * @return lowercase version of str.
 */
std::string tolower(const std::string& str);


}

#endif // APL_STRINGFUNCTIONS_H
