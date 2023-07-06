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

#include "apl/utils/weakcache.h"
#include "apl/utils/synchronizedweakcache.h"

using namespace apl;

class Foo {
public:
    static std::shared_ptr<Foo> create(int value) { return std::make_shared<Foo>(value); }

    Foo(int value) : value(value) {}

    const int value;
};

TEST(WeakCache, Basic)
{
    WeakCache<std::string, Foo> cache;

    {
        auto f1 = Foo::create(1);
        auto f2 = Foo::create(2);

        ASSERT_FALSE(cache.find("f1"));
        ASSERT_FALSE(cache.find("f2"));

        cache.insert("f1", f1);
        cache.insert("f2", f2);

        ASSERT_TRUE(cache.find("f1"));
        ASSERT_TRUE(cache.find("f2"));
    }

    ASSERT_FALSE(cache.find("f1"));
    ASSERT_FALSE(cache.find("f2"));
    ASSERT_TRUE(cache.empty());
}

TEST(WeakCache, Prepopulate)
{
    auto f1 = Foo::create(1);
    auto f2 = Foo::create(2);

    WeakCache<std::string, Foo> cache = {
        {"f1", f1},
        {"f2", f2}
    };

    ASSERT_EQ(2, cache.size());

    {
        auto f3 = Foo::create(3);
        cache.insert("f3", f3);
        ASSERT_EQ(3, cache.size());
    }

    ASSERT_EQ(2, cache.size());
}

TEST(WeakCache, CleansOnDemand)
{
    WeakCache<std::string, Foo> cache;

    ASSERT_EQ(0, cache.size());

    {
        cache.insert("f1", Foo::create(1));
        cache.insert("f2", Foo::create(2));
    }

    // Cache contains two expired items (trust me)

    // Clean them up
    cache.clean();
    ASSERT_EQ(0, cache.size());

    // Add a mix of expired and non-expired items
    auto f3 = Foo::create(3);
    cache.insert("f3", f3);
    {
        cache.insert("f4", Foo::create(4));
        cache.insert("f5", Foo::create(5));
    }
    auto f6 = Foo::create(6);
    cache.insert("f6", f6);

    // Clean up the expired ones
    cache.clean();

    // This leaves behind exactly two non-expired ones
    ASSERT_EQ(2, cache.size());
    auto f3b = cache.find("f3");
    auto f6b = cache.find("f6");
    ASSERT_TRUE(f3b);
    ASSERT_TRUE(f6b);
    ASSERT_EQ(3, f3b->value);
    ASSERT_EQ(6, f6b->value);
}
