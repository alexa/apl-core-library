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

#ifndef _APL_VERSION_H
#define _APL_VERSION_H

#include <cstdint>

namespace apl {

class APLVersion {
public:
    enum Value : uint32_t {
        kAPLVersionIgnore = 0x0, /// Ignore version numbers
        kAPLVersion10  = 0x1, /// Support version 1.0
        kAPLVersion11  = 0x1U << 1, /// Support version 1.1
        kAPLVersion12  = 0x1U << 2, /// Support version 1.2
        kAPLVersion13  = 0x1U << 3, /// Support version 1.3
        kAPLVersion14  = 0x1U << 4, /// Support version 1.4
        kAPLVersion15  = 0x1U << 5, /// Support version 1.5
        kAPLVersion16  = 0x1U << 6, /// Support version 1.6
        kAPLVersion17  = 0x1U << 7, /// Support version 1.7
        kAPLVersion18  = 0x1U << 8, /// Support version 1.8
        kAPLVersion19  = 0x1U << 9, /// Support version 1.9
        kAPLVersion20221 = 0x1U << 10, /// Support version 2022.1
        kAPLVersion20222 = 0x1U << 11, /// Support version 2022.2
        kAPLVersion20231 = 0x1U << 12, /// Support version 2023.1
        kAPLVersion20232 = 0x1U << 13, /// Support version 2023.2
        kAPLVersion20233 = 0x1U << 14, /// Support version 2023.3
        kAPLVersion10to11  = kAPLVersion10 | kAPLVersion11, /// Convenience ranges from 1.0 to latest,
        kAPLVersion10to12  = kAPLVersion10to11 | kAPLVersion12,
        kAPLVersion10to13  = kAPLVersion10to12 | kAPLVersion13,
        kAPLVersion10to14  = kAPLVersion10to13 | kAPLVersion14,
        kAPLVersion10to15  = kAPLVersion10to14 | kAPLVersion15,
        kAPLVersion10to16  = kAPLVersion10to15 | kAPLVersion16,
        kAPLVersion10to17  = kAPLVersion10to16 | kAPLVersion17,
        kAPLVersion10to18  = kAPLVersion10to17 | kAPLVersion18,
        kAPLVersion10to19  = kAPLVersion10to18 | kAPLVersion19,
        kAPLVersion10to20221 = kAPLVersion10to19 | kAPLVersion20221,
        kAPLVersion20221to20222 = kAPLVersion10to20221 | kAPLVersion20222,
        kAPLVersion20222to20231 = kAPLVersion20221to20222 | kAPLVersion20231,
        kAPLVersion20231to20232 = kAPLVersion20222to20231 | kAPLVersion20232,
        kAPLVersion20232to20233 = kAPLVersion20231to20232 | kAPLVersion20233,
        kAPLVersionLatest = kAPLVersion20232to20233, /// Support the most recent engine version
        kAPLVersionDefault = kAPLVersion20232to20233, /// Default value
        kAPLVersionReported = kAPLVersion20233, /// Default reported version
        kAPLVersionAny = 0xffffffff, /// Support any versions in the list
    };

    APLVersion() = default;
    constexpr APLVersion(Value v) : mValue(v) {}

    bool isValid(Value other) const;
    bool isValid(const std::string& other) const;

    bool operator==(const APLVersion& rhs) const { return mValue == rhs.mValue; }
    bool operator!=(const APLVersion& rhs) const { return mValue != rhs.mValue; }

    static std::string getDefaultReportedVersionString();
private:
    Value mValue;
};

} // namespace apl

#endif // _APL_VERSION_H
