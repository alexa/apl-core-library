/*
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
#ifndef EXTENSIONS_EXTENSIONUUID_H
#define EXTENSIONS_EXTENSIONUUID_H

#include <array>
#include <functional>
#include <random>
#include <string>
#include <tuple>

namespace alexaext {
namespace uuid {

inline std::string generateUUIDV4() {
    const int dataSize = 16;
    static const std::array<int8_t, 16> hexValues = {'0', '1', '2', '3', '4', '5', '6', '7',
                                                     '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    std::array<uint8_t, dataSize> randomData {};

    for(int i = 0; i < dataSize; ++i)
        randomData[i] = static_cast<uint8_t>(dis(rd));

    // Not fully required, based on the function format_uuid_v3or5 from the RFC 4122 to help
    // format
    randomData[6] = 0x40 | (randomData[6] & 0xf);
    randomData[8] = 0x80 | (randomData[8] & 0x3f);
    // Transform to string, we rely on small string optimization so += doesn't take
    // too much time
    std::string returnValue;
    auto hexValueToUuidValue = [&](uint8_t value) {
        return std::make_tuple(hexValues[(value & 0xF0) >> 4], hexValues[value & 0xF]);
    };
    auto transformHex = [&hexValueToUuidValue](int start,
                            int bytesToTransform,
                            const std::array<uint8_t, dataSize> &randomData,
                            std::string &returnValue) {
        for (int i = start; i < start + bytesToTransform; i++) {
            auto hexPair = hexValueToUuidValue(randomData[i]);
            returnValue += std::get<0>(hexPair);
            returnValue += std::get<1>(hexPair);
        }
    };
    transformHex(0, 4, randomData, returnValue);
    returnValue += "-";
    transformHex(4, 2, randomData, returnValue);
    returnValue += "-";
    transformHex(6, 2, randomData, returnValue);
    returnValue += "-";
    transformHex(8, 2, randomData, returnValue);
    returnValue += "-";
    transformHex(10, 6, randomData, returnValue);
    return returnValue;
}

using UUIDFunction = std::function<std::string(void)>;

} // namespace uuid
} // namespace alexaext

#endif // EXTENSIONS_EXTENSIONUUID_H