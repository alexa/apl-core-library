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

#ifndef _APL_TEXT_PROPERTIES_CACHE_H
#define _APL_TEXT_PROPERTIES_CACHE_H

#include "apl/scenegraph/common.h"
#include "apl/utils/weakcache.h"

namespace apl {
namespace sg {

/**
 * Basic cache implementation for the TextProperties.
 * We wrap this in a class rather than directly exposing WeakCache to give ourselves
 * options for changing it in the future.
 */
class TextPropertiesCache : public WeakCache<std::size_t, TextProperties> {};

} // namespace sg
} // namespace apl

#endif // _APL_TEXT_PROPERTIES_CACHE_H
