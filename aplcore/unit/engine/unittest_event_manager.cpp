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

#include "apl/engine/eventmanager.h"

using namespace apl;

class EventManagerTest : public ::testing::Test
{
protected:
    EventManager eventManager;
};

TEST_F(EventManagerTest, TestPushFrontPopEmpty)
{
    ASSERT_TRUE(eventManager.empty());
    EventBag bag;
    bag.emplace(kEventPropertyName, "arbitraryName");
    Event event(kEventTypeSendEvent, std::move(bag));
    eventManager.emplace(nullptr, event);
    ASSERT_FALSE(eventManager.empty());
    ASSERT_EQ(event, eventManager.front());
    ASSERT_FALSE(eventManager.empty());
    eventManager.pop();
    ASSERT_TRUE(eventManager.empty());
}

TEST_F(EventManagerTest, TestPushFrontPopEmptyConst)
{
    ASSERT_TRUE(eventManager.empty());
    EventBag bag;
    bag.emplace(kEventPropertyName, "arbitraryName");
    const Event event(kEventTypeSendEvent, std::move(bag));
    eventManager.emplace(nullptr, event);
    ASSERT_FALSE(eventManager.empty());
    ASSERT_EQ(event, eventManager.front());
    ASSERT_FALSE(eventManager.empty());
    eventManager.pop();
    ASSERT_TRUE(eventManager.empty());
}

TEST_F(EventManagerTest, TestPushClearEmpty)
{
    ASSERT_TRUE(eventManager.empty());
    EventBag bag;
    bag.emplace(kEventPropertyName, "arbitraryName");
    const Event event(kEventTypeSendEvent, std::move(bag));
    eventManager.emplace(nullptr, event);
    eventManager.emplace(nullptr, event);
    ASSERT_FALSE(eventManager.empty());
    eventManager.clear();
    ASSERT_TRUE(eventManager.empty());
}

TEST_F(EventManagerTest, TestFIFO)
{
    ASSERT_TRUE(eventManager.empty());
    EventBag firstBag;
    firstBag.emplace(kEventPropertyName, "arbitraryName");
    const Event first(kEventTypeSendEvent, std::move(firstBag));
    EventBag secondBag;
    secondBag.emplace(kEventPropertyName, "differentArbitraryName");
    const Event second(kEventTypeSendEvent, std::move(secondBag));
    eventManager.emplace(nullptr, first);
    eventManager.emplace(nullptr, second);

    ASSERT_EQ(first, eventManager.front());
    eventManager.pop();
    ASSERT_EQ(second, eventManager.front());
    eventManager.pop();
    ASSERT_TRUE(eventManager.empty());
}