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

#include "gtest/gtest.h"

#include <memory>

#include "apl/utils/scopedset.h"

using namespace apl;

class ScopedSetTest : public ::testing::Test {};

TEST_F(ScopedSetTest, Basic)
{
    auto scopedSet = std::make_shared<ScopedSet<int,int>>();
    scopedSet->emplace(1, 1);
    scopedSet->emplace(2, 3);
    scopedSet->emplace(1, 2);
    scopedSet->emplace(2, 4);
    scopedSet->emplace(1, 2);

    ASSERT_FALSE(scopedSet->empty());
    ASSERT_EQ(4, scopedSet->size());
    ASSERT_EQ(std::set<int>({1,2,3,4}), scopedSet->getAll());
    ASSERT_EQ(std::set<int>({1,2}), scopedSet->getScoped(1));
    ASSERT_EQ(std::set<int>({3,4}), scopedSet->getScoped(2));
}

TEST_F(ScopedSetTest, Clear)
{
    auto scopedSet = std::make_shared<ScopedSet<int,int>>();
    scopedSet->emplace(1, 1);
    scopedSet->emplace(2, 3);
    scopedSet->emplace(1, 2);
    scopedSet->emplace(2, 4);
    scopedSet->emplace(1, 2);

    ASSERT_FALSE(scopedSet->empty());
    ASSERT_EQ(4, scopedSet->size());
    ASSERT_EQ(2, scopedSet->eraseScope(2));
    ASSERT_EQ(2, scopedSet->size());

    scopedSet->clear();

    ASSERT_TRUE(scopedSet->empty());
}

TEST_F(ScopedSetTest, Erase)
{
    auto scopedSet = std::make_shared<ScopedSet<int,int>>();
    scopedSet->emplace(1, 1);
    scopedSet->emplace(2, 3);
    scopedSet->emplace(1, 2);
    scopedSet->emplace(2, 4);
    scopedSet->emplace(1, 2);

    ASSERT_FALSE(scopedSet->empty());
    ASSERT_EQ(4, scopedSet->size());

    ASSERT_EQ(std::set<int>({1,2}), scopedSet->extractScope(1));
    ASSERT_EQ(std::set<int>({3,4}), scopedSet->getAll());

    ASSERT_EQ(2, scopedSet->size());

    scopedSet->eraseValue(3);

    ASSERT_EQ(std::set<int>({4}), scopedSet->getAll());

    ASSERT_EQ(1, scopedSet->size());

    ASSERT_EQ(4, scopedSet->pop());

    ASSERT_TRUE(scopedSet->empty());
}
