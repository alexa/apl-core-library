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

#ifndef _ALEXAEXT_RANDOM_H
#define _ALEXAEXT_RANDOM_H

#include <string>

namespace alexaext {

/**
 * Generates a base-36 token with the specified length.
 *
 * @param prefix The optional prefix to prepend to the random token
 * @param len The length of the generated token, ignoring the prefix.
 * @return The randomly generated token
 */
std::string generateBase36Token(const std::string& prefix = "", unsigned int len = 8);

};

#endif // _ALEXAEXT_RANDOM_H
