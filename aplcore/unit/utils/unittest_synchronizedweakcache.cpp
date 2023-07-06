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

#include "apl/utils/synchronizedweakcache.h"

using namespace apl;

class Bar {
public:
    static std::shared_ptr<Bar> create(int value) { return std::make_shared<Bar>(value); }

    Bar(int value) : value(value) {}

    const int value;
};

TEST(SynchronizedWeakCache, WrapsWeakCache)
{
    auto f1 = Bar::create(1);
    auto f2 = Bar::create(2);

    SynchronizedWeakCache<std::string, Bar> cache = {
        {"f1", f1},
        {"f2", f2}
    };

    ASSERT_TRUE(cache.find("f1"));
    ASSERT_TRUE(cache.find("f2"));

    {
        auto f3 = Bar::create(3);
        auto f4 = Bar::create(4);

        ASSERT_FALSE(cache.find("f3"));
        ASSERT_FALSE(cache.find("f4"));

        cache.insert("f3", f3);
        cache.insert("f4", f4);

        ASSERT_TRUE(cache.find("f3"));
        ASSERT_TRUE(cache.find("f4"));

        ASSERT_EQ(4, cache.size());
    }

    ASSERT_EQ(2, cache.size());
    ASSERT_FALSE(cache.empty());

    cache.clean();
    ASSERT_EQ(2, cache.size());
}

TEST(SynchronizedWeakCache, AutomaticallyCleansWhenDirty)
{
    SynchronizedWeakCache<std::string, Bar> cache;
    ASSERT_FALSE(cache.dirty());

    auto f1 = Bar::create(1);
    auto f2 = Bar::create(2);
    cache.insert("f1", f1);
    cache.insert("f2", f2);

    ASSERT_TRUE(cache.find("f1"));
    ASSERT_TRUE(cache.find("f2"));
    ASSERT_FALSE(cache.dirty());

    {
        auto f3 = Bar::create(3);
        cache.insert("f3", f3);
    }
    cache.markDirty();
    ASSERT_TRUE(cache.dirty());

    // The cache has been marked as dirty, so next insert should clean it
    auto f4 = Bar::create(4);
    cache.insert("f4", f4);

    // Cache shouldn't be dirty anymore, this is a side-effect validation
    // that the cleaning has taken place, because size() also cleans.
    ASSERT_FALSE(cache.dirty());
    ASSERT_EQ(3, cache.size());
}
