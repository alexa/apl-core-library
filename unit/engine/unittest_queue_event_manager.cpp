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

#include <memory>

#include "gtest/gtest.h"

#include "apl/engine/queueeventmanager.h"

using namespace apl;

class QueueEventManagerTest : public ::testing::Test
{
protected:
    QueueEventManager eventManager;
};

TEST_F(QueueEventManagerTest, TestPushFrontPopEmpty)
{
    ASSERT_TRUE(eventManager.empty());
    EventBag bag;
    bag.emplace(kEventPropertyName, "arbitraryName");
    Event event(kEventTypeSendEvent, std::move(bag));
    eventManager.push(event);
    ASSERT_FALSE(eventManager.empty());
    ASSERT_TRUE(event.matches(eventManager.front()));
    ASSERT_FALSE(eventManager.empty());
    eventManager.pop();
    ASSERT_TRUE(eventManager.empty());
}

TEST_F(QueueEventManagerTest, TestPushFrontPopEmptyConst)
{
    ASSERT_TRUE(eventManager.empty());
    EventBag bag;
    bag.emplace(kEventPropertyName, "arbitraryName");
    const Event event(kEventTypeSendEvent, std::move(bag));
    eventManager.push(event);
    ASSERT_FALSE(eventManager.empty());
    ASSERT_TRUE(event.matches(eventManager.front()));
    ASSERT_FALSE(eventManager.empty());
    eventManager.pop();
    ASSERT_TRUE(eventManager.empty());
}

TEST_F(QueueEventManagerTest, TestPushClearEmpty)
{
    ASSERT_TRUE(eventManager.empty());
    EventBag bag;
    bag.emplace(kEventPropertyName, "arbitraryName");
    const Event event(kEventTypeSendEvent, std::move(bag));
    eventManager.push(event);
    eventManager.push(event);
    ASSERT_FALSE(eventManager.empty());
    eventManager.clear();
    ASSERT_TRUE(eventManager.empty());
}

TEST_F(QueueEventManagerTest, TestFIFO)
{
    ASSERT_TRUE(eventManager.empty());
    EventBag firstBag;
    firstBag.emplace(kEventPropertyName, "arbitraryName");
    const Event first(kEventTypeSendEvent, std::move(firstBag));
    EventBag secondBag;
    firstBag.emplace(kEventPropertyName, "differentArbitraryName");
    const Event second(kEventTypeSendEvent, std::move(secondBag));
    eventManager.push(first);
    eventManager.push(second);

    ASSERT_TRUE(first.matches(eventManager.front()));
    eventManager.pop();
    ASSERT_TRUE(second.matches(eventManager.front()));
    eventManager.pop();
    ASSERT_TRUE(eventManager.empty());
}