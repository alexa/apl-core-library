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

#ifndef _APL_RANDOM_H
#define _APL_RANDOM_H

#include <random>

namespace apl {

/**
 * Utilities for random generators.
 */
class Random
{
public:

    /**
     * @return 32-bit Mersenne Twister random number generator.
     */
    static std::mt19937
    mt32Generator()
    {
        static std::random_device random_device;
        static std::mt19937 generator(random_device());
        return generator;
    }

    static std::string
    generateToken(const std::string& base = "")
    {
        static auto gen = Random::mt32Generator();
        static std::uniform_int_distribution<> dis(11, 42);
        static std::uniform_int_distribution<> dis2(42, 64);

        streamer ss;
        if (!base.empty())
            ss << base << "-" ;

        int i;
        for (i = 0; i < 8; i++) {
            ss << dis(gen);
        }
        ss << "-";
        ss << dis2(gen);
        for (i = 0; i < 8; i++) {
            ss << dis(gen);
        }
        return ss.str();
    }

};

} // namespace apl

#endif // _APL_RANDOM_H
