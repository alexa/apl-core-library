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

#include "testeventloop.h"

using namespace apl;

class EventLoopWrapper : public ActionWrapper {};

// This is an internal test to make sure our fake Timers behaves.

TEST_F(EventLoopWrapper, EventLoop)
{
    bool id1_fired = false;
    auto id1 = loop->setTimeout([&](){
        id1_fired = true;
    }, 100);

    ASSERT_FALSE(id1_fired);
    ASSERT_EQ(100, id1);

    bool id2_fired = false;
    auto id2 = loop->setTimeout([&](){
        id2_fired = true;
    }, 0);

    ASSERT_FALSE(id2_fired);
    ASSERT_EQ(101, id2);

    ASSERT_EQ(0, loop->currentTime());
    ASSERT_EQ(2, loop->size());

    loop->advance();

    ASSERT_EQ(0, loop->currentTime());
    ASSERT_EQ(1, loop->size());
    ASSERT_TRUE(id2_fired);
    ASSERT_FALSE(id1_fired);

    loop->advance();
    ASSERT_EQ(100, loop->currentTime());
    ASSERT_EQ(0, loop->size());
    ASSERT_TRUE(id1_fired);
    ASSERT_TRUE(id2_fired);

    // Add a bunch of timers
    std::vector<int> timers = { 130, 20, 0, 0, 20 };
    for (int i = 0 ; i < timers.size() ; i++) {
        ASSERT_EQ(102 + i, loop->setTimeout([&, i]() {
            timers[i] = -1;
        }, timers[i]));
    }

    ASSERT_EQ(5, loop->size());

    // Clear the pending timeouts to remove the "0" offsets
    loop->runPending();
    ASSERT_EQ(3, loop->size());
    ASSERT_EQ(100, loop->currentTime());
    ASSERT_EQ(timers, std::vector<int>({130, 20, -1, -1, 20}));

    // Advance to remove the "20" offsets
    loop->advance();
    ASSERT_EQ(1, loop->size());
    ASSERT_EQ(120, loop->currentTime());
    ASSERT_EQ(timers, std::vector<int>({130, -1, -1, -1, -1}));

    // Test adding and removing
    ASSERT_FALSE(loop->clearTimeout(101));
    ASSERT_TRUE(loop->clearTimeout(102));
    ASSERT_EQ(0, loop->size());

    // Add a bunch and remove one in the middle
    timers = { 10, 20, 30 };
    for (int i = 0 ; i < timers.size() ; i++) {
        ASSERT_EQ(107 + i, loop->setTimeout([&, i]() {
            timers[i] = -1;
        }, timers[i]));
    }

    ASSERT_TRUE(loop->clearTimeout(108));
    loop->advance();
    loop->advance();
    ASSERT_EQ(timers, std::vector<int>({-1, 20, -1}));
    ASSERT_EQ(150, loop->currentTime());
}

// Verify that animators work correctly
TEST_F(EventLoopWrapper, Animations)
{
    ASSERT_EQ(0, loop->currentTime());
    ASSERT_EQ(std::numeric_limits<apl_time_t>::max(), loop->nextTimeout());

    int value = 0;
    // Set up a single animator
    loop->setAnimator([&value](apl_duration_t delta) {
        value = delta;
    }, 1000);

    ASSERT_EQ(1, loop->nextTimeout());
    for (int i = 0; i <= 1000; i += 100) {
        loop->advanceToTime(i);
        ASSERT_EQ(value, i) << i;
    }

    ASSERT_EQ(1000, loop->currentTime());
    ASSERT_EQ(0, loop->animatorCount());
    ASSERT_EQ(0, loop->size());
}

TEST_F(EventLoopWrapper, SeveralAnimations)
{
    loop->advanceToTime(12345);   // Set a starting time

    std::vector<int> timers = { 500, 1500, 2500 };
    for (int i = 0 ; i < timers.size() ; i++) {
        loop->setTimeout([&, i]() {
            timers[i] = -1;
        }, timers[i]);
    }

    std::vector<int> animators = { 1000, 2000, 3000};
    std::vector<int> animationCount = {0, 0, 0};

    for (int i = 0 ; i < animators.size() ; i++) {
        loop->setAnimator([&, i](apl_duration_t delta) {
            animators[i] = delta;
            animationCount[i] += 1;
        }, animators[i]);
    }

    ASSERT_EQ(6, loop->size());
    ASSERT_EQ(3, loop->animatorCount());

    int count = 0;
    for (int i = 100 ; i <= 3000 ; i += 100) {
        loop->updateTime(12345 + i);
        count++;
        ASSERT_EQ(std::vector<int>({i < 500 ? 500 : -1, i < 1500 ? 1500 : -1, i < 2500 ? 2500 : -1}), timers);
        ASSERT_EQ(std::vector<int>({i < 1000 ? i : 1000, i < 2000 ? i : 2000, i < 3000 ? i : 3000}), animators);
        ASSERT_EQ(std::vector<int>({i < 1000 ? count : 10, i < 2000 ? count : 20, count}), animationCount);
    }

    ASSERT_EQ(15345, loop->currentTime());
    ASSERT_EQ(0, loop->size());
    ASSERT_EQ(0, loop->animatorCount());
}

TEST_F(EventLoopWrapper, ParallelAnimations)
{
    std::vector<int> timers = {0, 0};

    for (int i = 0 ; i < timers.size() ; i++) {
        loop->setAnimator([&, i](apl_duration_t delta) {
            timers[i]++;
        }, 1000);
    }

    int count = 0;
    for (int i = 0 ; i < 1000 ; i+= 100) {
        loop->setTimeout([&count](){
            count++;
        }, i);
    }

    ASSERT_EQ(12, loop->size());
    ASSERT_EQ(2, loop->animatorCount());

    while (loop->size())
        loop->advanceBy(100);

    ASSERT_EQ(10, count);
    ASSERT_EQ(std::vector<int>({10, 10}), timers);
}

TEST_F(EventLoopWrapper, AnimatorCreatesTimeout)
{
    int timeoutCalls = 0;
    int animatorCalls = 0;
    loop->setAnimator([&](apl_duration_t delta) {
        animatorCalls++;
        // This timeout created during animator execution trips up the previous
        // implementation of the timer heap and causes a segmentation fault
        loop->setTimeout([&]() {
            timeoutCalls++;
        }, 100);
    }, 100);

    loop->setAnimator([&](apl_duration_t delta) {
        animatorCalls++;
    }, 300);

    ASSERT_EQ(2, loop->size());
    ASSERT_EQ(2, loop->animatorCount());

    loop->advanceBy(50);

    ASSERT_EQ(2, animatorCalls);
    ASSERT_EQ(0, timeoutCalls);
    ASSERT_EQ(3, loop->size());

    loop->advanceBy(1000);

    ASSERT_EQ(2, timeoutCalls);
}
