/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "perftest_base.h"
#include "rapidjson/document.h"

using namespace apl;

class PerftestOperations : public PerftestBase {};

TEST_F(PerftestOperations, Scrolling) {
    auto telemetry = getTelemetry();

    for(int i = 0; i<100; i++) {
        auto root = load("basic");

        telemetry->startTime("scrolling");

        // Check the layout
        auto top = root->topComponent();
        auto seq = top->getChildAt(1);
        auto child5 = seq->getChildAt(5);
        child5->ensureLayout();

        auto tw = top->getChildAt(0); // Touch wrapper
        auto text = tw->getChildAt(0);

        auto rect = child5->getCalculated(kPropertyBounds).getRect();
        seq->update(kUpdateScrollPosition, (int)rect.getTop());

        ASSERT_TRUE(root->isDirty());
        auto dirty = root->getDirty();
        ASSERT_EQ(1, dirty.count(text));
        ASSERT_EQ(1, text->getDirty().count(kPropertyText));
        ASSERT_EQ("scrolled", text->getCalculated(kPropertyText).asString());
        root->clearDirty();

        ASSERT_FALSE(root->hasEvent());

        telemetry->endTime("scrolling");
    }

    EXPECT_GT(10, extractCounter("basic"));
    EXPECT_GT(1, extractCounter("scrolling"));
}

TEST_F(PerftestOperations, Press) {
    auto telemetry = getTelemetry();

    for(int i = 0; i<100; i++) {
        auto root = load("basic");

        telemetry->startTime("press");

        // Check the layout
        auto top = root->topComponent();
        auto tw = top->getChildAt(0); // Touch wrapper
        auto text = tw->getChildAt(0);

        tw->update(kUpdatePressState, 1);
        ASSERT_TRUE(root->isDirty());
        auto dirty = root->getDirty();
        ASSERT_EQ(1, dirty.size());
        ASSERT_EQ(1, dirty.count(text));
        ASSERT_EQ(1, text->getDirty().count(kPropertyColor));
        ASSERT_EQ(Object(Color("blue")), text->getCalculated(kPropertyColor));
        root->clearDirty();

        // Simulate releasing in the touchwrapper
        tw->update(kUpdatePressState, 0);
        tw->update(kUpdatePressed);

        ASSERT_TRUE(root->hasEvent());
        auto event = root->popEvent();
        ASSERT_FALSE(root->hasEvent());
        ASSERT_EQ(kEventTypeSendEvent, event.getType());
        auto args = event.getValue(kEventPropertyArguments);
        ASSERT_EQ(1, args.size());
        ASSERT_EQ(Object("some argument with binding: 1"), args.at(0));
        ASSERT_TRUE(event.getActionRef().isEmpty());

        telemetry->endTime("press");
    }

    EXPECT_GT(10, extractCounter("basic"));
    EXPECT_GT(1, extractCounter("press"));
}

TEST_F(PerftestOperations, Command) {
    auto telemetry = getTelemetry();
    const char *commands = "[{\n"
        "  \"type\": \"ScrollToComponent\",\n"
        "  \"componentId\": \"container7\",\n"
        "  \"align\": \"center\"\n"
        "}]";
    auto* doc = new rapidjson::Document();
    doc->Parse(commands);
    apl::Object commandsObj = apl::Object(*doc);

    for(int i = 0; i<100; i++) {
        auto root = load("basic");

        telemetry->startTime("command");

        // Check the layout
        auto top = root->topComponent();
        auto seq = top->getChildAt(1);
        auto tw = top->getChildAt(0); // Touch wrapper
        auto text = tw->getChildAt(0);

        auto action = root->executeCommands(commandsObj, false);

        ASSERT_TRUE(root->hasEvent());

        auto event = root->popEvent();
        ASSERT_FALSE(root->hasEvent());
        ASSERT_EQ(kEventTypeScrollTo, event.getType());
        auto component = event.getComponent();

        component->ensureLayout();
        auto rect = component->getCalculated(kPropertyBounds).getRect();
        seq->update(kUpdateScrollPosition, (int)rect.getTop());
        action->resolve();

        ASSERT_TRUE(root->isDirty());
        auto dirty = root->getDirty();
        ASSERT_EQ(1, dirty.count(text));
        ASSERT_EQ(1, text->getDirty().count(kPropertyText));
        ASSERT_EQ("scrolled", text->getCalculated(kPropertyText).asString());
        root->clearDirty();

        ASSERT_FALSE(root->hasEvent());

        telemetry->endTime("command");
    }

    EXPECT_GT(10, extractCounter("basic"));
    EXPECT_GT(1, extractCounter("command"));
}
