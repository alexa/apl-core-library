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

#include "gtest/gtest.h"

#include "apl/utils/ringbuffer.h"

using namespace apl;

class RingBufferTest : public ::testing::Test {};

TEST_F(RingBufferTest, Basic)
{
    auto rb = RingBuffer<int>(5);

    ASSERT_EQ(5, rb.capacity());
    ASSERT_TRUE(rb.empty());

    // Fill it in
    for (int i = 0; i < 5; i++) {
        rb.enqueue(i);
    }

    ASSERT_EQ(5, rb.size());
    ASSERT_TRUE(rb.full());

    ASSERT_EQ(0, rb.dequeue());

    ASSERT_EQ(4, rb.size());
    ASSERT_FALSE(rb.full());

    ASSERT_EQ(1, rb.front());
    ASSERT_EQ(4, rb.back());
}

TEST_F(RingBufferTest, Clear)
{
    auto rb = RingBuffer<int>(5);

    ASSERT_EQ(5, rb.capacity());
    ASSERT_TRUE(rb.empty());

    // Fill it in
    for (int i = 0; i < 3; i++) {
        rb.enqueue(i);
    }

    ASSERT_EQ(3, rb.size());
    ASSERT_FALSE(rb.full());

    rb.clear();
    ASSERT_EQ(0, rb.size());
    ASSERT_TRUE(rb.empty());

    // Fill it in
    for (int i = 0; i < 3; i++) {
        rb.enqueue(i);
    }

    ASSERT_EQ(3, rb.size());
    ASSERT_FALSE(rb.full());

    ASSERT_EQ(0, rb.dequeue());
    ASSERT_EQ(1, rb.dequeue());
    ASSERT_EQ(2, rb.dequeue());
}

TEST_F(RingBufferTest, Access)
{
    auto rb = RingBuffer<int>(5);

    // Fill it in
    for (int i = 0; i < 5; i++) {
        rb.enqueue(i);
    }

    ASSERT_EQ(5, rb.size());
    ASSERT_TRUE(rb.full());

    ASSERT_EQ(1, rb[1]);
}

TEST_F(RingBufferTest, CycleUp)
{
    auto rb = RingBuffer<int>(5);

    ASSERT_EQ(5, rb.capacity());
    ASSERT_TRUE(rb.empty());

    // Fill it but over
    for (int i = 0; i < 7; i++) {
        rb.enqueue(i);
    }

    ASSERT_EQ(5, rb.size());
    ASSERT_TRUE(rb.full());

    ASSERT_EQ(2, rb.dequeue());
    ASSERT_EQ(3, rb.dequeue());

    ASSERT_EQ(3, rb.size());
    ASSERT_FALSE(rb.full());
}

TEST_F(RingBufferTest, AfterDequeue)
{
    auto rb = RingBuffer<int>(5);

    ASSERT_EQ(5, rb.capacity());
    ASSERT_TRUE(rb.empty());

    // Fill it
    for (int i = 0; i < 5; i++) {
        rb.enqueue(i);
    }

    ASSERT_EQ(0, rb.dequeue());
    ASSERT_EQ(1, rb.dequeue());

    ASSERT_EQ(3, rb.size());
    ASSERT_FALSE(rb.full());

    // Fill it up
    rb.enqueue(5);
    rb.enqueue(6);

    ASSERT_TRUE(rb.full());
    ASSERT_EQ(5, rb.size());

    ASSERT_EQ(2, rb.dequeue());
    ASSERT_EQ(3, rb.dequeue());
}

TEST_F(RingBufferTest, RangeAccess)
{
    auto rb = RingBuffer<int>(5);

    // Fill it but over
    for (int i = 0; i < 7; i++) {
        rb.enqueue(i);
    }

    int i = 2;
    for (auto r : rb) {
        ASSERT_EQ(i, r);
        i++;
    }
}

TEST_F(RingBufferTest, Iterator)
{
    auto rb = RingBuffer<int>(5);

    // Fill it but over
    for (int i = 0; i < 7; i++) {
        rb.enqueue(i);
    }

    int i = 2;
    for (auto it = rb.begin(); it != rb.end(); it++) {
        ASSERT_EQ(i, *it);
        i++;
    }
}

TEST_F(RingBufferTest, ReverseIterator)
{
    auto rb = RingBuffer<int>(5);

    // Fill it but over
    for (int i = 0; i < 7; i++) {
        rb.enqueue(i);
    }

    int i = 6;
    for (auto it = rb.rbegin(); it != rb.rend(); it++) {
        ASSERT_EQ(i, *it);
        i--;
    }
}

TEST_F(RingBufferTest, ConstIterator)
{
    auto rb = RingBuffer<int>(5);

    // Fill it but over
    for (int i = 0; i < 7; i++) {
        rb.enqueue(i);
    }

    int i = 2;
    for (auto it = rb.cbegin(); it != rb.cend(); it++) {
        ASSERT_EQ(i, *it);
        i++;
    }
}

TEST_F(RingBufferTest, ConstReverseIterator)
{
    auto rb = RingBuffer<int>(5);

    // Fill it but over
    for (int i = 0; i < 7; i++) {
        rb.enqueue(i);
    }

    int i = 6;
    for (auto it = rb.crbegin(); it != rb.crbegin(); it++) {
        ASSERT_EQ(i, *it);
        i--;
    }
}