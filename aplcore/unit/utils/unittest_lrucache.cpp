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

#include "../testeventloop.h"
#include <apl/utils/lrucache.h>

using namespace apl;

class LruCacheTest : public MemoryWrapper {};

TEST_F(LruCacheTest, Simple)
{
    auto cache = LruCache<int, int>(2);
    cache.put(0, 0);
    cache.put(1, 1);

    ASSERT_TRUE(cache.has(0));
    ASSERT_TRUE(cache.has(1));
    ASSERT_EQ(0, cache.get(0));
    ASSERT_EQ(1, cache.get(1));
}

TEST_F(LruCacheTest, KickOut)
{
    auto cache = LruCache<int, int>(2);
    cache.put(0, 0);
    cache.put(1, 1);
    cache.put(2, 2);

    ASSERT_FALSE(cache.has(0));
    ASSERT_TRUE(cache.has(1));
    ASSERT_TRUE(cache.has(2));
}

TEST_F(LruCacheTest, KickOutAfterAccess)
{
    auto cache = LruCache<int, int>(2);
    cache.put(0, 0);
    cache.put(1, 1);

    ASSERT_TRUE(cache.has(0));
    ASSERT_EQ(0, cache.get(0));

    cache.put(2, 2);

    ASSERT_TRUE(cache.has(0));
    ASSERT_FALSE(cache.has(1));
    ASSERT_TRUE(cache.has(2));
}