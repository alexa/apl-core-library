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

#include "alexaext/random.h"

#include <cstring>
#include <random>

namespace alexaext {

/**
 * @return 64-bit Mersenne Twister random number generator.
 */
std::mt19937_64
mt64Generator()
{
    static std::random_device random_device;
    static std::mt19937_64 generator(random_device());
    return generator;
}

static const char *base36chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

std::string
generateBase36Token(const std::string& prefix, unsigned int len) {
    static auto gen = mt64Generator();
    static std::uniform_int_distribution<> distrib(0, 35);

    auto prefixLen = prefix.length();
    auto totalLen = prefixLen + len + 1;
    char token[totalLen];
    std::strcpy(token, prefix.c_str());

    for (int i = 0; i < len; i++) {
        token[prefixLen + i] = base36chars[distrib(gen)];
    }

    token[totalLen - 1] = '\0';

    return std::string(token);
}

} // namespace alexaext