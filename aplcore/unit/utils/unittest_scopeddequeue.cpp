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

#include "apl/utils/scopeddequeue.h"

using namespace apl;

class ScopedDequeueTest : public ::testing::Test {};

TEST_F(ScopedDequeueTest, Basic)
{
    auto scopedDequeue = std::make_shared<ScopedDequeue<int,int>>();
    scopedDequeue->emplace(1, 1);
    scopedDequeue->emplace(2, 3);
    scopedDequeue->emplace(1, 2);
    scopedDequeue->emplace(2, 4);
    scopedDequeue->emplace(1, 2);

    ASSERT_FALSE(scopedDequeue->empty());
    ASSERT_EQ(5, scopedDequeue->size());

    auto comp = std::deque<std::pair<int, int>>();
    comp.emplace_back(1, 1);
    comp.emplace_back(2, 3);
    comp.emplace_back(1, 2);
    comp.emplace_back(2, 4);
    comp.emplace_back(1, 2);

    ASSERT_EQ(comp, scopedDequeue->getAll());


    ASSERT_EQ(std::vector<int>({1,2,2}), scopedDequeue->getScoped(1));
    ASSERT_EQ(std::vector<int>({3,4}), scopedDequeue->getScoped(2));
}

TEST_F(ScopedDequeueTest, Clear)
{
    auto scopedDequeue = std::make_shared<ScopedDequeue<int,int>>();
    scopedDequeue->emplace(1, 1);
    scopedDequeue->emplace(2, 3);
    scopedDequeue->emplace(1, 2);
    scopedDequeue->emplace(2, 4);
    scopedDequeue->emplace(1, 2);

    ASSERT_FALSE(scopedDequeue->empty());
    ASSERT_EQ(5, scopedDequeue->size());

    ASSERT_EQ(2, scopedDequeue->eraseScope(2));
    ASSERT_EQ(3, scopedDequeue->size());

    scopedDequeue->clear();

    ASSERT_TRUE(scopedDequeue->empty());
}

TEST_F(ScopedDequeueTest, ExtractScope)
{
    auto scopedDequeue = std::make_shared<ScopedDequeue<int,int>>();
    scopedDequeue->emplace(1, 1);
    scopedDequeue->emplace(2, 3);
    scopedDequeue->emplace(1, 2);
    scopedDequeue->emplace(2, 4);
    scopedDequeue->emplace(1, 2);

    ASSERT_FALSE(scopedDequeue->empty());
    ASSERT_EQ(5, scopedDequeue->size());

    ASSERT_EQ(std::vector<int>({1, 2, 2}), scopedDequeue->extractScope(1));

    auto comp = std::deque<std::pair<int, int>>();
    comp.emplace_back(2, 3);
    comp.emplace_back(2, 4);

    ASSERT_EQ(comp, scopedDequeue->getAll());

    ASSERT_EQ(2, scopedDequeue->size());
    ASSERT_EQ(3, scopedDequeue->pop());
    ASSERT_EQ(4, scopedDequeue->pop());

    ASSERT_TRUE(scopedDequeue->empty());
}

TEST_F(ScopedDequeueTest, TestPushFrontPopEmpty)
{
    auto scopedDequeue = std::make_shared<ScopedDequeue<int,int>>();
    ASSERT_TRUE(scopedDequeue->empty());
    scopedDequeue->emplace(0, 1);
    ASSERT_FALSE(scopedDequeue->empty());
    ASSERT_EQ(1, scopedDequeue->front());
    ASSERT_FALSE(scopedDequeue->empty());
    scopedDequeue->pop();
    ASSERT_TRUE(scopedDequeue->empty());
}

TEST_F(ScopedDequeueTest, TestPushFrontPopEmptyConst)
{
    auto scopedDequeue = std::make_shared<ScopedDequeue<int,int>>();
    ASSERT_TRUE(scopedDequeue->empty());
    scopedDequeue->emplace(0, 1);
    ASSERT_FALSE(scopedDequeue->empty());
    ASSERT_EQ(1, scopedDequeue->front());
    ASSERT_FALSE(scopedDequeue->empty());
    scopedDequeue->pop();
    ASSERT_TRUE(scopedDequeue->empty());
}

TEST_F(ScopedDequeueTest, TestPushClearEmpty)
{
    auto scopedDequeue = std::make_shared<ScopedDequeue<int,int>>();
    ASSERT_TRUE(scopedDequeue->empty());
    scopedDequeue->emplace(0, 1);
    scopedDequeue->emplace(0, 1);
    ASSERT_FALSE(scopedDequeue->empty());
    scopedDequeue->clear();
    ASSERT_TRUE(scopedDequeue->empty());
}

TEST_F(ScopedDequeueTest, TestFIFO)
{
    auto scopedDequeue = std::make_shared<ScopedDequeue<int,int>>();
    ASSERT_TRUE(scopedDequeue->empty());
    scopedDequeue->emplace(0,1);
    scopedDequeue->emplace(0,2);

    ASSERT_EQ(1, scopedDequeue->front());
    scopedDequeue->pop();
    ASSERT_EQ(2, scopedDequeue->front());
    scopedDequeue->pop();
    ASSERT_TRUE(scopedDequeue->empty());
}
