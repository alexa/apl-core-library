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

#ifndef APL_HASH_H
#define APL_HASH_H

#include "apl/primitives/object.h"

namespace apl {

template <class T>
static inline void hashCombine(std::size_t& currentHash, const T& next) {
    // Variation of boost::hash_combine
    std::hash<T> hasher;
    currentHash ^= hasher(next) + 0x9e3779b9 + (currentHash<<6) + (currentHash>>2);
}

}

#endif //APL_HASH_H
