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

#ifndef _APL_FLAGS_H
#define _APL_FLAGS_H

namespace apl {

/**
 * Simple bitset flags container.
 * @tparam T enum type for flag set.
 */
template<
    class T,
    class Storage = typename std::underlying_type<T>::type>
class Flags {
public:
    static_assert(std::is_enum<T>::value, "Requires enum type.");

    constexpr Flags() : mFlags(0) {}
    constexpr explicit Flags(Storage initialValue) : mFlags(initialValue) {}

    void set(T flag) { mFlags |= flag; }
    void clear(T flag) { mFlags &= ~flag; }
    bool isSet(T flag) const { return (mFlags & flag) == flag; }
    bool checkAndClear(T flag) {
        auto result = isSet(flag);
        clear(flag);
        return result;
    }

private:

    Storage mFlags;
};

}

#endif //_APL_FLAGS_H
