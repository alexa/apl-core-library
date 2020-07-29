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

#include "apl/engine/event.h"

using namespace apl;

class CommandPageTest : public CommandTest {
public:
    ActionPtr executeSetPage(const std::string& component, const std::string& position, int value) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "SetPage", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("position", rapidjson::Value(position.c_str(), alloc).Move(), alloc);
        cmd.AddMember("value", value, alloc);
        doc.SetArray().PushBack(cmd, alloc);
        return root->executeCommands(doc, false);
    }

    ActionPtr executeAutoPage(const std::string& component, int count, int duration) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "AutoPage", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("count", count, alloc);
        cmd.AddMember("duration", duration, alloc);
        doc.SetArray().PushBack(cmd, alloc);
        return root->executeCommands(doc, false);
    }

    ::testing::AssertionResult
    CheckChild(size_t idx, const std::string& id, const Rect& bounds) {
        auto child = component->getChildAt(idx);

        auto actualId = child->getId();
        if (id != actualId) {
            return ::testing::AssertionFailure()
                    << "child " << idx
                    << " id is wrong. Expected: " << id
                    << ", actual: " << actualId;
        }

        auto actualBounds = child->getCalculated(kPropertyBounds).getRect();
        if (bounds != actualBounds) {
            return ::testing::AssertionFailure()
                    << "child " << idx
                    << " bounds is wrong. Expected: " << bounds.toString()
                    << ", actual: " << actualBounds.toString();
        }

        return ::testing::AssertionSuccess();
    }

    rapidjson::Document doc;
};


static const char *PAGER_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Pager\","
    "      \"id\": \"myPager\","
    "      \"width\": 100,"
    "      \"height\": 100,"
    "      \"navigation\": \"normal\","
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"id\": \"id${data}\","
    "        \"text\": \"TEXT${data}\","
    "        \"speech\": \"URL${data}\""
    "      },"
    "      \"data\": [ 1, 2, 3, 4, 5 ],"
    "      \"onPageChanged\": {"
    "        \"type\": \"SendEvent\","
    "        \"arguments\": ["
    "          \"${event.target.page}\""
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(CommandPageTest, Pager)
{
    loadDocument(PAGER_TEST);

    ASSERT_EQ(5, component->getChildCount());
    // Only initial pages ensured
    ASSERT_TRUE(CheckChild(0, "id1", Rect(0, 0, 100, 100)));
    ASSERT_TRUE(CheckChild(1, "id2", Rect(0, 0, 100, 100)));
    ASSERT_TRUE(CheckChild(2, "id3", Rect(0, 0, 0, 0)));

    executeSetPage("myPager", "relative", 2);  // Page forward twice
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    ASSERT_EQ(kEventTypeSetPage, event.getType());
    ASSERT_EQ(component, event.getComponent());
    ASSERT_EQ(2, event.getValue(kEventPropertyPosition).getInteger());
    ASSERT_EQ(kEventDirectionForward, event.getValue(kEventPropertyDirection).getInteger());

    root->updateTime(500);
    ASSERT_FALSE(root->hasEvent());
    // Target one becomes ensured
    ASSERT_TRUE(CheckChild(2, "id3", Rect(0, 0, 100, 100)));
    ASSERT_TRUE(CheckChild(3, "id4", Rect(0, 0, 0, 0)));

    // Update the page and resolve the event
    component->update(kUpdatePagerPosition, 2);
    ASSERT_EQ(2, component->getCalculated(kPropertyCurrentPage).getInteger());
    event.getActionRef().resolve();

    // Ones around visible page ensured too
    ASSERT_TRUE(CheckChild(3, "id4", Rect(0, 0, 100, 100)));
    ASSERT_TRUE(CheckChild(4, "id5", Rect(0, 0, 0, 0)));

    // We should have a SendEvent
    ASSERT_TRUE(CheckSendEvent(root, 2));

    ASSERT_TRUE(CheckNoActions());
}

static const char *SIMPLE_PAGER =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Pager\","
    "      \"id\": \"myPager\","
    "      \"width\": 100,"
    "      \"height\": 100,"
    "      \"initialPage\": 2,"
    "      \"navigation\": \"normal\","
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"id\": \"id${data}\","
    "        \"text\": \"TEXT${data}\","
    "        \"speech\": \"URL${data}\""
    "      },"
    "      \"data\": [ 1, 2, 3, 4, 5 ]"
    "    }"
    "  }"
    "}";

TEST_F(CommandPageTest, SimplePageRelative)
{
    loadDocument(SIMPLE_PAGER);

    for (int i = -3 ; i <= 3 ; i++) {
        executeSetPage("myPager", "relative", i);
        std::string msg = "Relative(" + std::to_string(i) + ")";

        int target = i + 2;
        EventDirection direction = i < 0 ? kEventDirectionBackward : kEventDirectionForward;
        if (i == 0 || target < 0 || target > 4) {
            ASSERT_FALSE(root->hasEvent()) << msg;
        }
        else {
            ASSERT_TRUE(root->hasEvent()) << msg;
            auto event = root->popEvent();
            ASSERT_EQ(kEventTypeSetPage, event.getType()) << msg;
            ASSERT_EQ(component, event.getComponent()) << msg;
            ASSERT_EQ(target, event.getValue(kEventPropertyPosition).getInteger()) << msg;
            ASSERT_EQ(direction, event.getValue(kEventPropertyDirection).getInteger()) << msg;
            event.getActionRef().resolve();   // Resolve without moving

            ASSERT_FALSE(root->hasEvent()) << msg;
        }
    }
}

TEST_F(CommandPageTest, SimplePageAbsolute)
{
    loadDocument(SIMPLE_PAGER);

    for (int i = -8 ; i <= 8 ; i++) {
        executeSetPage("myPager", "absolute", i);
        std::string msg = "Absolute(" + std::to_string(i) + ")";

        int target = i;
        if (target < 0) target += 5;   // Negative values measure from the end
        if (target < 0) target = 0;
        if (target > 4) target = 4;

        EventDirection direction = target < 2 ? kEventDirectionBackward : kEventDirectionForward;

        if (target == 2) {
            ASSERT_FALSE(root->hasEvent()) << msg;
        }
        else {
            ASSERT_TRUE(root->hasEvent()) << msg;
            auto event = root->popEvent();
            ASSERT_EQ(kEventTypeSetPage, event.getType()) << msg;
            ASSERT_EQ(component, event.getComponent()) << msg;
            ASSERT_EQ(target, event.getValue(kEventPropertyPosition).getInteger()) << msg;
            ASSERT_EQ(direction, event.getValue(kEventPropertyDirection).getInteger()) << msg;
            event.getActionRef().resolve();   // Resolve without moving

            ASSERT_FALSE(root->hasEvent()) << msg;
        }
    }
}



static const char *SIMPLE_PAGER_WRAP =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Pager\","
    "      \"id\": \"myPager\","
    "      \"width\": 100,"
    "      \"height\": 100,"
    "      \"initialPage\": 2,"
    "      \"navigation\": \"wrap\","
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"id\": \"id${data}\","
    "        \"text\": \"TEXT${data}\","
    "        \"speech\": \"URL${data}\""
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4,"
    "        5"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(CommandPageTest, SimplePageRelativeWrap)
{
    loadDocument(SIMPLE_PAGER_WRAP);

    // Wrap results in all pages ensured straight away.
    ASSERT_TRUE(CheckChild(0, "id1", Rect(0, 0, 100, 100)));
    ASSERT_TRUE(CheckChild(1, "id2", Rect(0, 0, 100, 100)));
    ASSERT_TRUE(CheckChild(2, "id3", Rect(0, 0, 100, 100)));
    ASSERT_TRUE(CheckChild(3, "id4", Rect(0, 0, 100, 100)));
    ASSERT_TRUE(CheckChild(4, "id5", Rect(0, 0, 100, 100)));

    for (int i = -8 ; i <= 8 ; i++) {
        executeSetPage("myPager", "relative", i);
        std::string msg = "Relative(" + std::to_string(i) + ")";

        int target = i + 2;
        EventDirection direction = i < 0 ? kEventDirectionBackward : kEventDirectionForward;
        while (target < 0) target += 5;
        while (target >= 5) target -= 5;
        if (target == 2) {
            ASSERT_FALSE(root->hasEvent()) << msg;
        }
        else {
            ASSERT_TRUE(root->hasEvent()) << msg;
            auto event = root->popEvent();
            ASSERT_EQ(kEventTypeSetPage, event.getType()) << msg;
            ASSERT_EQ(component, event.getComponent()) << msg;
            ASSERT_EQ(target, event.getValue(kEventPropertyPosition).getInteger()) << msg;
            ASSERT_EQ(direction, event.getValue(kEventPropertyDirection).getInteger()) << msg;
            event.getActionRef().resolve();   // Resolve without moving

            ASSERT_FALSE(root->hasEvent()) << msg;
        }
    }
}

TEST_F(CommandPageTest, SimplePageAbsoluteWrap)
{
    loadDocument(SIMPLE_PAGER_WRAP);

    for (int i = -8 ; i <= 8 ; i++) {
        executeSetPage("myPager", "absolute", i);
        std::string msg = "Absolute(" + std::to_string(i) + ")";

        int target = i;
        if (target < 0) target += 5;   // Negative values measure from the end
        if (target < 0) target = 0;
        if (target > 4) target = 4;

        EventDirection direction = target < 2 ? kEventDirectionBackward : kEventDirectionForward;

        if (target == 2) {
            ASSERT_FALSE(root->hasEvent()) << msg;
        }
        else {
            ASSERT_TRUE(root->hasEvent()) << msg;
            auto event = root->popEvent();
            ASSERT_EQ(kEventTypeSetPage, event.getType()) << msg;
            ASSERT_EQ(component, event.getComponent()) << msg;
            ASSERT_EQ(target, event.getValue(kEventPropertyPosition).getInteger()) << msg;
            ASSERT_EQ(direction, event.getValue(kEventPropertyDirection).getInteger()) << msg;
            event.getActionRef().resolve();   // Resolve without moving

            ASSERT_FALSE(root->hasEvent()) << msg;
        }
    }
}


static const char *AUTO_PAGE_BASIC =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Pager\","
    "      \"id\": \"myPager\","
    "      \"width\": 100,"
    "      \"height\": 100,"
    "      \"initialPage\": 1,"
    "      \"navigation\": \"wrap\","
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"id\": \"id${data}\","
    "        \"text\": \"TEXT${data}\","
    "        \"speech\": \"URL${data}\""
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4,"
    "        5"
    "      ]"
    "    }"
    "  }"
    "}";


TEST_F(CommandPageTest, AutoPage)
{
    loadDocument(AUTO_PAGE_BASIC);

    executeAutoPage("myPager", 100000, 1000);  // Play all, pausing for 1000 milliseconds between them

    for (int index = 2 ; index < 5 ; index++) {
        std::string msg = "Auto(" + std::to_string(index) + ")";

        if (index != 2) {
            ASSERT_FALSE(root->hasEvent()) << msg;
            root->updateTime(root->currentTime() + 1000);  // 1000 milliseconds AFTER the resolution
        }

        ASSERT_TRUE(root->hasEvent()) << msg;
        auto event = root->popEvent();
        ASSERT_EQ(kEventTypeSetPage, event.getType()) << msg;
        ASSERT_EQ(component, event.getComponent()) << msg;
        ASSERT_EQ(index, event.getValue(kEventPropertyPosition).getInteger()) << msg;
        ASSERT_EQ(kEventDirectionForward, event.getValue(kEventPropertyDirection).getInteger()) << msg;

        root->updateTime(root->currentTime() + 500);
        component->update(kUpdatePagerByEvent, index);
        event.getActionRef().resolve();   // Resolve without moving
    }

    root->clearPending();
    ASSERT_EQ(1, loop->size());  // Waiting for the final delay
    root->updateTime(root->currentTime() + 1000);

    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());
}

TEST_F(CommandPageTest, AutoPageNoDelay)
{
    loadDocument(AUTO_PAGE_BASIC);

    executeAutoPage("myPager", 100000, 0);  // Play all, no delay

    for (int index = 2 ; index < 5 ; index++) {
        std::string msg = "Auto(" + std::to_string(index) + ")";

        ASSERT_TRUE(root->hasEvent()) << msg;
        auto event = root->popEvent();
        ASSERT_EQ(kEventTypeSetPage, event.getType()) << msg;
        ASSERT_EQ(component, event.getComponent()) << msg;
        ASSERT_EQ(index, event.getValue(kEventPropertyPosition).getInteger()) << msg;
        ASSERT_EQ(kEventDirectionForward, event.getValue(kEventPropertyDirection).getInteger()) << msg;

        root->updateTime(root->currentTime() + 500);
        component->update(kUpdatePagerByEvent, index);
        event.getActionRef().resolve();   // Resolve without moving
    }

    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());
}

TEST_F(CommandPageTest, AutoPageShort)
{
    loadDocument(AUTO_PAGE_BASIC);

    executeAutoPage("myPager", 2, 1000);  // Just show two pages

    for (int index = 2 ; index < 4 ; index++) {
        std::string msg = "Auto(" + std::to_string(index) + ")";

        if (index != 2) {
            ASSERT_FALSE(root->hasEvent()) << msg;
            root->updateTime(root->currentTime() + 1000);  // 1000 milliseconds AFTER the resolution
        }

        ASSERT_TRUE(root->hasEvent()) << msg;
        auto event = root->popEvent();
        ASSERT_EQ(kEventTypeSetPage, event.getType()) << msg;
        ASSERT_EQ(component, event.getComponent()) << msg;
        ASSERT_EQ(index, event.getValue(kEventPropertyPosition).getInteger()) << msg;
        ASSERT_EQ(kEventDirectionForward, event.getValue(kEventPropertyDirection).getInteger()) << msg;

        root->updateTime(root->currentTime() + 500);
        component->update(kUpdatePagerByEvent, index);
        event.getActionRef().resolve();   // Resolve without moving
    }

    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(1, loop->size());

    root->updateTime(root->currentTime() + 1000);
    ASSERT_EQ(0, loop->size());
}

TEST_F(CommandPageTest, AutoPageTerminateInDelay)
{
    loadDocument(AUTO_PAGE_BASIC);

    auto action = executeAutoPage("myPager", 2, 1000);  // Just show two pages

    for (int index = 2 ; index < 4 ; index++) {
        std::string msg = "Auto(" + std::to_string(index) + ")";

        if (index != 2) {
            ASSERT_FALSE(root->hasEvent()) << msg;
            action->terminate();  // Terminate while we're waiting for the next timeout
            break;
        }

        ASSERT_TRUE(root->hasEvent()) << msg;
        auto event = root->popEvent();
        ASSERT_EQ(kEventTypeSetPage, event.getType()) << msg;
        ASSERT_EQ(component, event.getComponent()) << msg;
        ASSERT_EQ(index, event.getValue(kEventPropertyPosition).getInteger()) << msg;
        ASSERT_EQ(kEventDirectionForward, event.getValue(kEventPropertyDirection).getInteger()) << msg;

        root->updateTime(root->currentTime() + 500);
        component->update(kUpdatePagerPosition, index);
        ASSERT_EQ(index, component->getCalculated(kPropertyCurrentPage).getInteger());
        event.getActionRef().resolve();   // Resolve without moving
    }

    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());
}

TEST_F(CommandPageTest, AutoPageAbortSetPage)
{
    loadDocument(AUTO_PAGE_BASIC);

    auto action = executeAutoPage("myPager", 2, 1000);  // Just show two pages
    bool terminated = false;
    action->addTerminateCallback([&terminated](const TimersPtr&){
        terminated = true;
    });

    for (int index = 2 ; index < 4 ; index++) {
        std::string msg = "Auto(" + std::to_string(index) + ")";

        if (index != 2) {
            ASSERT_FALSE(root->hasEvent()) << msg;
            root->updateTime(root->currentTime() + 1000);  // 1000 milliseconds AFTER the resolution
        }

        ASSERT_TRUE(root->hasEvent()) << msg;
        auto event = root->popEvent();
        ASSERT_EQ(kEventTypeSetPage, event.getType()) << msg;
        ASSERT_EQ(component, event.getComponent()) << msg;
        ASSERT_EQ(index, event.getValue(kEventPropertyPosition).getInteger()) << msg;
        ASSERT_EQ(kEventDirectionForward, event.getValue(kEventPropertyDirection).getInteger()) << msg;

        root->updateTime(root->currentTime() + 500);
        root->cancelExecution();
        break;
    }

    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(terminated);
    ASSERT_TRUE(CheckNoActions());
    ASSERT_EQ(0, loop->size());
}

TEST_F(CommandPageTest, AutoPageNone)
{
    loadDocument(AUTO_PAGE_BASIC);

    executeAutoPage("myPager", 0, 0);  // Ask for zero
    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());

    executeAutoPage("myPager", -2, 0);  // Ask for negative
    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());
}


static const char *EMPTY_PAGER =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Pager\","
    "      \"id\": \"myPager\","
    "      \"width\": 100,"
    "      \"height\": 100,"
    "      \"initialPage\": 2,"
    "      \"navigation\": \"wrap\","
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"id\": \"id${data}\","
    "        \"text\": \"TEXT${data}\","
    "        \"speech\": \"URL${data}\""
    "      },"
    "      \"data\": ["
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(CommandPageTest, EmptyPagerSetPage)
{
    loadDocument(EMPTY_PAGER);

    executeSetPage("myPager", "absolute", 0);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());

    executeSetPage("myPager", "relative", 1);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());
}

TEST_F(CommandPageTest, EmptyPagerAutoPage)
{
    loadDocument(EMPTY_PAGER);

    executeAutoPage("myPager", 2, 0);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());
}


static const char *SINGLE_PAGE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Pager\","
    "      \"id\": \"myPager\","
    "      \"width\": 100,"
    "      \"height\": 100,"
    "      \"initialPage\": 2,"
    "      \"navigation\": \"wrap\","
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"id\": \"id${data}\","
    "        \"text\": \"TEXT${data}\","
    "        \"speech\": \"URL${data}\""
    "      },"
    "      \"data\": ["
    "        1"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(CommandPageTest, SinglePageSetPage)
{
    loadDocument(SINGLE_PAGE);

    executeSetPage("myPager", "absolute", 0);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());

    executeSetPage("myPager", "relative", 1);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());
}

TEST_F(CommandPageTest, SinglePageAutoPage)
{
    loadDocument(SINGLE_PAGE);

    executeAutoPage("myPager", 1, 0);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());
}
