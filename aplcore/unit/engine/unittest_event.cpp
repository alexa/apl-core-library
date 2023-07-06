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

#include "../faketextcomponent.h"

#include "apl/content/metrics.h"
#include "apl/engine/context.h"
#include "apl/engine/event.h"
#include "apl/utils/session.h"

using namespace apl;

class EventTest : public ::testing::Test {};

TEST_F(EventTest, Equality)
{
    auto context = Context::createTestContext(Metrics(), makeDefaultSession());

    auto component1 = std::make_shared<FakeTextComponent>(context, "fake1", "fake1");
    EventBag bag1;
    bag1.emplace(kEventPropertyName, "arbitraryName");
    const Event event(kEventTypeSendEvent, std::move(bag1), component1);

    EventBag bag2;
    bag2.emplace(kEventPropertyName, "arbitraryName");
    const Event sameEvent(kEventTypeSendEvent, std::move(bag2), component1);

    ASSERT_EQ(event, sameEvent);

    EventBag bag3;
    bag3.emplace(kEventPropertyName, "arbitraryName");
    const Event differentTypeEvent(kEventTypeOpenURL, std::move(bag3), component1);

    ASSERT_FALSE(event == differentTypeEvent);

    auto component2 = std::make_shared<FakeTextComponent>(context, "fake2", "fake2");
    EventBag bag4;
    bag4.emplace(kEventPropertyName, "arbitraryName");
    const Event differentComponentEvent(kEventTypeSendEvent, std::move(bag4), component2);

    ASSERT_FALSE(event == differentComponentEvent);

    EventBag bag5;
    bag5.emplace(kEventPropertyName, "arbitraryName");
    bag5.emplace(kEventPropertyExtensionURI, "no");
    const Event differentBagEvent(kEventTypeSendEvent, std::move(bag5), component2);

    ASSERT_FALSE(event == differentBagEvent);
}
