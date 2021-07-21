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

#include "apl/component/component.h"
#include "apl/engine/event.h"
#include "apl/primitives/mediastate.h"

#include "../testeventloop.h"

using namespace apl;

class ComponentEventsTest : public CommandTest {};

static const char *DATA = "{}";

static const char *TOUCH_WRAPPER_PRESSED =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.0\","
        "  \"mainTemplate\": {"
        "    \"parameters\": ["
        "      \"payload\""
        "    ],"
        "    \"items\": {"
        "      \"type\": \"TouchWrapper\","
        "      \"onPress\": {"
        "        \"type\": \"SetValue\","
        "        \"componentId\": \"textComp\","
        "        \"property\": \"text\","
        "        \"value\": \"Two\""
        "      },"
        "      \"items\": {"
        "        \"type\": \"Text\","
        "        \"id\": \"textComp\","
        "        \"text\": \"One\""
        "      }"
        "    }"
        "  }"
        "}";

TEST_F(ComponentEventsTest, TouchWrapperPressed)
{
    loadDocument(TOUCH_WRAPPER_PRESSED, DATA);
    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeTouchWrapper, component->getType());

    auto text = context->findComponentById("textComp");
    ASSERT_TRUE(text);

    ASSERT_EQ("One", text->getCalculated(kPropertyText).asString());

    // Simulate pressed event
    performTap(0, 0);
    loop->advanceToEnd();
    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_EQ("Two", text->getCalculated(kPropertyText).asString());
}

static const char *TOUCH_WRAPPER_PRESSED_NO_ID_CHILD =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.0\","
        "  \"mainTemplate\": {"
        "    \"parameters\": ["
        "      \"payload\""
        "    ],"
        "    \"items\": {"
        "      \"type\": \"TouchWrapper\","
        "      \"onPress\": {"
        "        \"type\": \"SetValue\","
        "        \"property\": \"text\","
        "        \"value\": \"Two\""
        "      },"
        "      \"items\": {"
        "        \"type\": \"Text\","
        "        \"id\": \"textComp\","
        "        \"text\": \"One\""
        "      }"
        "    }"
        "  }"
        "}";

/**
 * Verify that a missing componentId prevents the SetValue command from succeeding.
 */
TEST_F(ComponentEventsTest, TouchWrapperPressedNoIdChild)
{
    loadDocument(TOUCH_WRAPPER_PRESSED_NO_ID_CHILD, DATA);
    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeTouchWrapper, component->getType());

    auto text = context->findComponentById("textComp");
    ASSERT_TRUE(text);

    ASSERT_EQ("One", text->getCalculated(kPropertyText).asString());

    // Simulate pressed event
    ASSERT_FALSE(ConsoleMessage());
    performTap(0, 0);
    loop->advanceToEnd();
    ASSERT_EQ(0, root->getDirty().size());
    ASSERT_EQ("One", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(ConsoleMessage());  // We should be warned about the missing componentId/invalid property
}

static const char *TOUCH_WRAPPER_PRESSED_NO_ID_NOT_CHILD =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.0\","
        "  \"mainTemplate\": {"
        "    \"parameters\": ["
        "      \"payload\""
        "    ],"
        "    \"items\": {"
        "      \"type\": \"Container\","
        "      \"items\": ["
        "        {"
        "          \"type\": \"TouchWrapper\","
        "          \"id\": \"touch\","
        "          \"height\": 10,"
        "          \"onPress\": {"
        "            \"type\": \"SetValue\","
        "            \"property\": \"text\","
        "            \"value\": \"Two\""
        "          }"
        "        },"
        "        {"
        "          \"type\": \"Text\","
        "          \"id\": \"textComp\","
        "          \"text\": \"One\""
        "        }"
        "      ]"
        "    }"
        "  }"
        "}";

/**
 * If no ID provided for command in case of event handler it should not be executed if target component
 * is not component issuing event
 */
TEST_F(ComponentEventsTest, TouchWrapperPressedNoIdNotChild)
{
    loadDocument(TOUCH_WRAPPER_PRESSED_NO_ID_NOT_CHILD, DATA);
    ASSERT_TRUE(component);

    auto touch = context->findComponentById("touch");
    ASSERT_TRUE(touch);
    ASSERT_EQ(kComponentTypeTouchWrapper, touch->getType());

    auto text = context->findComponentById("textComp");
    ASSERT_TRUE(text);

    ASSERT_EQ("One", text->getCalculated(kPropertyText).asString());

    // Simulate pressed event
    ASSERT_FALSE(ConsoleMessage());
    performTap(0, 0);
    loop->advanceToEnd();
    ASSERT_EQ(0, root->getDirty().size());
    ASSERT_EQ("One", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(ConsoleMessage());
}

static const char *COMPONENT_SCROLLED =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.0\","
        "  \"mainTemplate\": {"
        "    \"parameters\": ["
        "      \"payload\""
        "    ],"
        "    \"items\": {"
        "      \"type\": \"ScrollView\","
        "      \"height\": 10,"
        "      \"onScroll\": {"
        "        \"type\": \"SetValue\","
        "        \"componentId\": \"textComp\","
        "        \"property\": \"text\","
        "        \"value\": \"Two\""
        "      },"
        "      \"item\": {"
        "        \"type\": \"Text\","
        "        \"height\": 50,"
        "        \"id\": \"textComp\","
        "        \"text\": \"One\""
        "      }"
        "    }"
        "  }"
        "}";

TEST_F(ComponentEventsTest, ComponentScrolled)
{
    loadDocument(COMPONENT_SCROLLED, DATA);
    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeScrollView, component->getType());

    auto text = context->findComponentById("textComp");
    ASSERT_TRUE(text);

    ASSERT_EQ("One", text->getCalculated(kPropertyText).asString());

    // Simulate scroll "not happening"
    component->update(kUpdateScrollPosition, 0);
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
    loop->advanceToEnd();
    ASSERT_EQ(0, root->getDirty().size());
    ASSERT_EQ("One", text->getCalculated(kPropertyText).asString());

    // Simulate scroll
    component->update(kUpdateScrollPosition, 10);
    ASSERT_EQ(Point(0, 10), component->scrollPosition());
    loop->advanceToEnd();
    ASSERT_TRUE(CheckDirty(text, kPropertyText));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_TRUE(CheckDirty(root, component, text));
    ASSERT_EQ("Two", text->getCalculated(kPropertyText).asString());
}

static const char *PAGER_CHANGED =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.0\","
        "  \"mainTemplate\": {"
        "    \"parameters\": ["
        "      \"payload\""
        "    ],"
        "    \"items\": {"
        "      \"type\": \"Pager\","
        "      \"initialPage\": 1,"
        "      \"onPageChanged\": {"
        "        \"type\": \"SetValue\","
        "        \"componentId\": \"textComp\","
        "        \"property\": \"text\","
        "        \"value\": \"Two\""
        "      },"
        "      \"items\": ["
        "        {"
        "          \"type\": \"Text\","
        "          \"id\": \"textComp\","
        "          \"text\": \"One\""
        "        },"
        "        {"
        "          \"type\": \"Text\","
        "          \"text\": \"Not one\""
        "        }"
        "      ]"
        "    }"
        "  }"
        "}";

TEST_F(ComponentEventsTest, PagerChanged)
{
    loadDocument(PAGER_CHANGED, DATA);
    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypePager, component->getType());
    advanceTime(10);
    root->clearDirty();

    auto text = context->findComponentById("textComp");
    ASSERT_TRUE(text);

    ASSERT_EQ("One", text->getCalculated(kPropertyText).asString());

    ASSERT_EQ(1, component->getCalculated(kPropertyInitialPage).asInt());
    ASSERT_EQ(1, component->getCalculated(kPropertyCurrentPage).getInteger());

    // Simulate page "not happening"
    component->update(kUpdatePagerPosition, 1);
    root->clearPending();
    ASSERT_EQ(0, root->getDirty().size());
    ASSERT_EQ("One", text->getCalculated(kPropertyText).asString());
    ASSERT_EQ(1, component->getCalculated(kPropertyCurrentPage).getInteger());

    // Simulate page
    component->update(kUpdatePagerPosition, 0);
    root->clearPending();
    ASSERT_EQ(2, root->getDirty().size());
    ASSERT_EQ("Two", text->getCalculated(kPropertyText).asString());
    ASSERT_EQ(0, component->getCalculated(kPropertyCurrentPage).getInteger());

    // Simulate page with float value
    component->update(kUpdatePagerPosition, 1.25);
    root->clearPending();
    ASSERT_EQ(2, root->getDirty().size());
    ASSERT_EQ(1, component->getCalculated(kPropertyCurrentPage).getInteger());
}

static const char *MEDIA_STATE_CHANGES =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.0\","
        "  \"mainTemplate\": {"
        "    \"parameters\": ["
        "      \"payload\""
        "    ],"
        "    \"items\": {"
        "      \"type\": \"Container\","
        "      \"items\": ["
        "        {"
        "          \"type\": \"Video\","
        "          \"id\": \"video\","
        "          \"scale\": \"best-fill\","
        "          \"source\": \"URL1\","
        "          \"onEnd\": {"
        "            \"type\": \"SetValue\","
        "            \"componentId\": \"textComp\","
        "            \"property\": \"text\","
        "            \"value\": \"END\""
        "          },"
        "          \"onPlay\": {"
        "            \"type\": \"SetValue\","
        "            \"componentId\": \"textComp\","
        "            \"property\": \"text\","
        "            \"value\": \"PLAY\""
        "          },"
        "          \"onPause\": {"
        "            \"type\": \"SetValue\","
        "            \"componentId\": \"textComp\","
        "            \"property\": \"text\","
        "            \"value\": \"PAUSE\""
        "          },"
        "          \"onTrackUpdate\": {"
        "            \"type\": \"SetValue\","
        "            \"componentId\": \"textComp\","
        "            \"property\": \"text\","
        "            \"value\": \"TRACK_UPDATE\""
        "          }"
        "        },"
        "        {"
        "          \"type\": \"Text\","
        "          \"id\": \"textComp\","
        "          \"text\": \"One\""
        "        }"
        "      ]"
        "    }"
        "  }"
        "}";

TEST_F(ComponentEventsTest, MediaStateChanges)
{
    loadDocument(MEDIA_STATE_CHANGES, DATA);
    ASSERT_TRUE(component);

    auto video = context->findComponentById("video");
    ASSERT_TRUE(video);
    ASSERT_EQ(kComponentTypeVideo, video->getType());

    auto text = context->findComponentById("textComp");
    ASSERT_TRUE(text);

    ASSERT_EQ("One", text->getCalculated(kPropertyText).asString());

    // Simulate "no change"
    MediaState state;
    video->updateMediaState(state);
    CheckMediaState(state, video->getCalculated());
    loop->advanceToEnd();
    ASSERT_EQ(0, root->getDirty().size());
    ASSERT_EQ("One", text->getCalculated(kPropertyText).asString());

    // Simulate playback start
    state = MediaState(0, 2, 7, 10, false, false);
    video->updateMediaState(state);
    CheckMediaState(state, video->getCalculated());
    loop->advanceToEnd();
    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_EQ("PLAY", text->getCalculated(kPropertyText).asString());

    // Simulate playback pause
    state = MediaState(0, 2, 7, 10, true, false);
    video->updateMediaState(state);
    CheckMediaState(state, video->getCalculated());
    loop->advanceToEnd();
    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_EQ("PAUSE", text->getCalculated(kPropertyText).asString());

    // Simulate track change
    state = MediaState(1, 2, 7, 10, true, false);
    video->updateMediaState(state);
    CheckMediaState(state, video->getCalculated());
    loop->advanceToEnd();
    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_EQ("TRACK_UPDATE", text->getCalculated(kPropertyText).asString());

    // Simulate playback end
    state = MediaState(1, 2, 7, 10, true, true);
    video->updateMediaState(state);
    CheckMediaState(state, video->getCalculated());
    loop->advanceToEnd();
    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_EQ("END", text->getCalculated(kPropertyText).asString());
}

static const char *TOUCH_WRAPPER_SEND_EVENT =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"parameters\": ["
        "      \"payload\""
        "    ],"
        "    \"items\": {"
        "      \"type\": \"TouchWrapper\","
        "      \"onPress\": {"
        "        \"type\": \"SendEvent\","
        "        \"arguments\": [ "
        "          \"${event.source.handler}\","
        "          \"${event.source.value}\","
        "          \"${event.target.opacity}\" "
        "        ],"
        "        \"components\": [ \"textComp\" ]"
        "      },"
        "      \"items\": {"
        "        \"type\": \"Text\","
        "        \"id\": \"textComp\","
        "        \"text\": \"<b>One</b>\""
        "      }"
        "    }"
        "  }"
        "}";

TEST_F(ComponentEventsTest, TouchWrapperSendEvent)
{
    loadDocument(TOUCH_WRAPPER_SEND_EVENT, DATA);
    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeTouchWrapper, component->getType());

    auto text = context->findComponentById("textComp");
    ASSERT_TRUE(text);

    ASSERT_EQ("<b>One</b>", text->getCalculated(kPropertyText).asString());

    // Simulate pressed event
    performTap(0, 0);
    loop->advanceToEnd();

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());

    auto arguments = event.getValue(kEventPropertyArguments);
    ASSERT_EQ(apl::Object::kArrayType, arguments.getType());
    ASSERT_EQ("Press", arguments.at(0).asString());
    ASSERT_EQ(false, arguments.at(1).asBoolean());
    ASSERT_EQ("1", arguments.at(2).asString());

    auto source = event.getValue(kEventPropertySource);
    ASSERT_TRUE(source.isMap());
    ASSERT_EQ("Press", source.get("handler").asString());
    ASSERT_EQ(component->getId(), source.get("id").asString());
    ASSERT_EQ(component->getUniqueId(), source.get("uid").asString());
    ASSERT_EQ("TouchWrapper", source.get("source").asString());
    ASSERT_EQ("TouchWrapper", source.get("type").asString());
    ASSERT_EQ(false, source.get("value").asBoolean());

    auto components = event.getValue(kEventPropertyComponents);
    ASSERT_EQ(apl::Object::kMapType, components.getType());
    ASSERT_EQ("<b>One</b>", components.get("textComp").asString());
}

static const char *PAGER_SEND_EVENT =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.0\","
        "  \"mainTemplate\": {"
        "    \"parameters\": ["
        "      \"payload\""
        "    ],"
        "    \"items\": {"
        "      \"type\": \"Pager\","
        "      \"initialPage\": 1,"
        "      \"onPageChanged\": {"
        "        \"type\": \"SendEvent\","
        "        \"arguments\": [ "
        "          \"${event.source.handler}\","
        "          \"${event.source.value}\","
        "          \"${event.target.opacity}\" "
        "        ],"
        "        \"components\": [ \"text1\", \"text2\", \"text3\" ]"
        "      },"
        "      \"items\": ["
        "        {"
        "          \"type\": \"Text\","
        "          \"id\": \"text1\","
        "          \"text\": \"One\""
        "        },"
        "        {"
        "          \"type\": \"Text\","
        "          \"id\": \"text2\","
        "          \"text\": \"Two\""
        "        },"
        "        {"
        "          \"type\": \"Text\","
        "          \"id\": \"text3\","
        "          \"text\": \"Three\""
        "        }"
        "      ]"
        "    }"
        "  }"
        "}";

TEST_F(ComponentEventsTest, PagerSendEvent)
{
    loadDocument(PAGER_SEND_EVENT, DATA);
    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypePager, component->getType());

    // Simulate page change
    component->update(kUpdatePagerPosition, 2);
    loop->advanceToEnd();

    ASSERT_EQ(2, component->getCalculated(kPropertyCurrentPage).getInteger());

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());

    auto arguments = event.getValue(kEventPropertyArguments);
    ASSERT_EQ(apl::Object::kArrayType, arguments.getType());
    ASSERT_EQ("Page", arguments.at(0).asString());
    ASSERT_EQ("2", arguments.at(1).asString());
    ASSERT_EQ("1", arguments.at(2).asString());

    auto source = event.getValue(kEventPropertySource);
    ASSERT_TRUE(source.isMap());
    ASSERT_EQ("Page", source.get("handler").asString());
    ASSERT_EQ("", source.get("id").asString());
    ASSERT_EQ(component->getUniqueId(), source.get("uid").asString());
    ASSERT_EQ("Pager", source.get("source").asString());
    ASSERT_EQ("Pager", source.get("type").asString());
    ASSERT_EQ(2, source.get("value").asNumber());

    auto text1 = context->findComponentById("text1");
    auto text2 = context->findComponentById("text2");
    auto text3 = context->findComponentById("text3");

    auto components = event.getValue(kEventPropertyComponents);
    ASSERT_EQ(apl::Object::kMapType, components.getType());
    ASSERT_EQ("One", components.get(text1->getId()).asString());
    ASSERT_EQ("Two", components.get(text2->getId()).asString());
    ASSERT_EQ("Three", components.get(text3->getId()).asString());
}

static const char *SCROLLABLE_SEND_EVENT =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.0\","
        "  \"mainTemplate\": {"
        "    \"parameters\": ["
        "      \"payload\""
        "    ],"
        "    \"items\": {"
        "      \"type\": \"ScrollView\","
        "      \"height\": 10,"
        "      \"onScroll\": {"
        "        \"type\": \"Custom\","
        "        \"arguments\": [ "
        "          \"${event.source.handler}\","
        "          \"${event.source.value}\","
        "          \"${event.target.opacity}\" "
        "        ],"
        "        \"components\": [ \"textComp\" ]"
        "      },"
        "      \"item\": {"
        "        \"type\": \"Text\","
        "        \"id\": \"textComp\","
        "        \"height\": 50,"
        "        \"text\": \"One\""
        "      }"
        "    }"
        "  }"
        "}";

TEST_F(ComponentEventsTest, ScrollableSendCustomEvent)
{
    loadDocument(SCROLLABLE_SEND_EVENT, DATA);
    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeScrollView, component->getType());

    // Simulate scrolling
    component->update(kUpdateScrollPosition, 15);
    loop->advanceToEnd();

    ASSERT_EQ(Point(0, 15), component->scrollPosition());
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(sEventTypeBimap.at("CustomEvent"), event.getType());

    auto arguments = event.getValue(kEventPropertyArguments);
    ASSERT_EQ(apl::Object::kArrayType, arguments.getType());
    ASSERT_EQ("Scroll", arguments.at(0).asString());
    ASSERT_EQ("1.5", arguments.at(1).asString());
    ASSERT_EQ("1", arguments.at(2).asString());

    auto source = event.getValue(kEventPropertySource);
    ASSERT_TRUE(source.isMap());
    ASSERT_EQ("Scroll", source.get("handler").asString());
    ASSERT_EQ("", source.get("id").asString());
    ASSERT_EQ(component->getUniqueId(), source.get("uid").asString());
    ASSERT_EQ("ScrollView", source.get("source").asString());
    ASSERT_EQ("ScrollView", source.get("type").asString());
    ASSERT_EQ(Object(1.5), source.get("value"));
}

static const char *MEDIA_SEND_EVENT =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.0\","
        "  \"mainTemplate\": {"
        "    \"parameters\": ["
        "      \"payload\""
        "    ],"
        "    \"items\": {"
        "      \"type\": \"Container\","
        "      \"items\": ["
        "        {"
        "          \"type\": \"Video\","
        "          \"id\": \"video\","
        "          \"scale\": \"best-fill\","
        "          \"source\": [\"URL1\",\"URL2\"],"
        "          \"onEnd\": {"
        "            \"type\": \"Custom\","
        "            \"arguments\": [ "
        "              \"${event.source.handler}\","
        "              \"${event.source.value}\","
        "              \"${event.target.opacity}\","
        "              \"${event.target.source}\","
        "              \"${event.trackIndex}\","
        "              \"${event.trackCount}\","
        "              \"${event.currentTime}\","
        "              \"${event.duration}\","
        "              \"${event.paused}\","
        "              \"${event.ended}\""
        "            ],"
        "            \"components\": [ \"textComp\" ]"
        "          },"
        "          \"onPlay\": {"
        "            \"type\": \"Custom\","
        "            \"arguments\": [ "
        "              \"${event.source.handler}\","
        "              \"${event.source.value}\","
        "              \"${event.target.opacity}\","
        "              \"${event.target.source}\","
        "              \"${event.trackIndex}\","
        "              \"${event.trackCount}\","
        "              \"${event.currentTime}\","
        "              \"${event.duration}\","
        "              \"${event.paused}\","
        "              \"${event.ended}\""
        "            ],"
        "            \"components\": [ \"textComp\" ]"
        "          },"
        "          \"onPause\": {"
        "            \"type\": \"Custom\","
        "            \"arguments\": [ "
        "              \"${event.source.handler}\","
        "              \"${event.source.value}\","
        "              \"${event.target.opacity}\","
        "              \"${event.target.source}\","
        "              \"${event.trackIndex}\","
        "              \"${event.trackCount}\","
        "              \"${event.currentTime}\","
        "              \"${event.duration}\","
        "              \"${event.paused}\","
        "              \"${event.ended}\""
        "            ],"
        "            \"components\": [ \"textComp\" ]"
        "          },"
        "          \"onTrackUpdate\": {"
        "            \"type\": \"Custom\","
        "            \"arguments\": [ "
        "              \"${event.source.handler}\","
        "              \"${event.source.value}\","
        "              \"${event.target.opacity}\","
        "              \"${event.target.source}\","
        "              \"${event.trackIndex}\","
        "              \"${event.trackCount}\","
        "              \"${event.currentTime}\","
        "              \"${event.duration}\","
        "              \"${event.paused}\","
        "              \"${event.ended}\""
        "            ],"
        "            \"components\": [ \"textComp\" ]"
        "          }"
        "        },"
        "        {"
        "          \"type\": \"Text\","
        "          \"id\": \"textComp\","
        "          \"text\": \"One\""
        "        }"
        "      ]"
        "    }"
        "  }"
        "}";

void
validateMediaEvent(RootContextPtr root,
                   const std::string& handler,
                   const std::string& trackIndex,
                   const std::string& paused,
                   const std::string& ended,
                   const std::string& url,
                   const Object& value)
{
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(TestEventCommand::eventType(), event.getType());

    auto arguments = event.getValue(kEventPropertyArguments);
    ASSERT_EQ(apl::Object::kArrayType, arguments.getType());
    ASSERT_EQ(handler, arguments.at(0).asString());
    ASSERT_TRUE(IsEqual(value, arguments.at(1))) << handler;
    ASSERT_EQ("1", arguments.at(2).asString());
    ASSERT_EQ(url, arguments.at(3).asString());
    ASSERT_EQ(trackIndex, arguments.at(4).asString()) << handler; // trackIndex
    ASSERT_EQ("2", arguments.at(5).asString()); // trackCount
    ASSERT_EQ("7", arguments.at(6).asString()); // currentTime
    ASSERT_EQ("10", arguments.at(7).asString()); // duration
    ASSERT_EQ(paused, arguments.at(8).asString()); // paused
    ASSERT_EQ(ended, arguments.at(9).asString()); // ended

    auto video = root->context().findComponentById("video");

    auto source = event.getValue(kEventPropertySource);
    ASSERT_TRUE(source.isMap());
    ASSERT_EQ(handler, source.get("handler").asString());
    ASSERT_EQ("video", source.get("id").asString());
    ASSERT_EQ(video->getUniqueId(), source.get("uid").asString());
    ASSERT_EQ("Video", source.get("source").asString());
    ASSERT_EQ("Video", source.get("type").asString());
    ASSERT_EQ(value, source.get("value"));

    auto text = root->context().findComponentById("textComp");

    auto components = event.getValue(kEventPropertyComponents);
    ASSERT_EQ(apl::Object::kMapType, components.getType());
    ASSERT_TRUE(IsEqual("One", components.get("textComp").asString()));
}

TEST_F(ComponentEventsTest, MediaSendEvent)
{
    loadDocument(MEDIA_SEND_EVENT, DATA);
    ASSERT_TRUE(component);

    auto video = context->findComponentById("video");
    ASSERT_TRUE(video);
    ASSERT_EQ(kComponentTypeVideo, video->getType());

    // Simulate "no change"
    MediaState state;
    video->updateMediaState(state);
    CheckMediaState(state, video->getCalculated());
    loop->advanceToEnd();

    ASSERT_FALSE(root->hasEvent());

    // Simulate playback start
    state = MediaState(0, 2, 7, 10, false, false);
    video->updateMediaState(state);
    CheckMediaState(state, video->getCalculated());
    loop->advanceToEnd();

    validateMediaEvent(root, "Play", "0", "false", "false", "URL1", Object::NULL_OBJECT());

    // Simulate playback pause
    state = MediaState(0, 2, 7, 10, true, false);
    video->updateMediaState(state);
    CheckMediaState(state, video->getCalculated());
    loop->advanceToEnd();

    validateMediaEvent(root, "Pause", "0", "true", "false", "URL1", Object::NULL_OBJECT());

    // Simulate track change
    state = MediaState(1, 2, 7, 10, true, false);
    video->updateMediaState(state);
    CheckMediaState(state, video->getCalculated());
    loop->advanceToEnd();//

    validateMediaEvent(root, "TrackUpdate", "1", "true", "false", "URL2", Object(1));

    // Simulate playback end
    state = MediaState(1, 2, 7, 10, true, true);
    video->updateMediaState(state);
    CheckMediaState(state, video->getCalculated());
    loop->advanceToEnd();

    validateMediaEvent(root, "End", "1", "true", "true", "URL2", Object::NULL_OBJECT());
}

static const char *MEDIA_SEND_EVENT_TIME_UPDATE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"parameters\": ["
    "      \"payload\""
    "    ],"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Video\","
    "          \"id\": \"video\","
    "          \"scale\": \"best-fill\","
    "          \"source\": [\"URL1\",\"URL2\"],"
    "          \"onTimeUpdate\": {"
    "            \"type\": \"Custom\","
    "            \"arguments\": [ "
    "              \"${event.source.handler}\","
    "              \"${event.source.value}\","
    "              \"${event.target.opacity}\","
    "              \"${event.target.source}\","
    "              \"${event.trackIndex}\","
    "              \"${event.trackCount}\","
    "              \"${event.currentTime}\","
    "              \"${event.duration}\","
    "              \"${event.paused}\","
    "              \"${event.ended}\""
    "            ],"
    "            \"components\": [ \"textComp\" ]"
    "          }"
    "        },"
    "        {"
    "          \"type\": \"Text\","
    "          \"id\": \"textComp\","
    "          \"text\": \"One\""
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(ComponentEventsTest, MediaOnTimeUpdate)
{
    loadDocument(MEDIA_SEND_EVENT_TIME_UPDATE, DATA);
    ASSERT_TRUE(component);

    auto video = context->findComponentById("video");
    ASSERT_TRUE(video);
    ASSERT_EQ(kComponentTypeVideo, video->getType());

    // Simulate "no change"
    MediaState state;
    video->updateMediaState(state);
    CheckMediaState(state, video->getCalculated());
    loop->advanceToEnd();

    ASSERT_FALSE(root->hasEvent());

    // Simulate time update
    state = MediaState(0, 1, 5, 10, false, false);
    video->updateMediaState(state);
    CheckMediaState(state, video->getCalculated());

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    ASSERT_EQ(sEventTypeBimap.at("CustomEvent"), event.getType());

    auto arguments = event.getValue(kEventPropertyArguments);
    ASSERT_EQ(apl::Object::kArrayType, arguments.getType());
    ASSERT_EQ(Object("TimeUpdate"), arguments.at(0));  // Handler
    ASSERT_EQ(Object(5), arguments.at(1));  // Value (current time)
    ASSERT_EQ(Object(1), arguments.at(2));  // Opacity
    ASSERT_EQ(Object("URL1"), arguments.at(3));   // Source
    ASSERT_EQ(Object(0), arguments.at(4)); // trackIndex
    ASSERT_EQ(Object(1), arguments.at(5)); // trackCount
    ASSERT_EQ(Object(5), arguments.at(6)); // currentTime
    ASSERT_EQ(Object(10), arguments.at(7)); // duration
    ASSERT_EQ(Object(false), arguments.at(8)); // paused
    ASSERT_EQ(Object(false), arguments.at(9)); // ended
}

static const char *MEDIA_FAST_NORMAL =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.0\","
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"Container\","
        "      \"items\": ["
        "        {"
        "          \"type\": \"Video\","
        "          \"id\": \"video\","
        "          \"source\": \"URL1\","
        "          \"onTimeUpdate\": {"
        "            \"type\": \"Parallel\","
        "            \"commands\": ["
        "              {"
        "                \"type\": \"SendEvent\""
        "              },"
        "              {"
        "                \"type\": \"SetValue\","
        "                \"componentId\": \"textComp\","
        "                \"property\": \"text\","
        "                \"value\": \"Two\""
        "              }"
        "            ]"
        "          },"
        "          \"onPause\": {"
        "            \"type\": \"Parallel\","
        "            \"commands\": ["
        "              {"
        "                \"type\": \"SendEvent\""
        "              },"
        "              {"
        "                \"type\": \"SetValue\","
        "                \"componentId\": \"textComp\","
        "                \"property\": \"text\","
        "                \"value\": \"Three\""
        "              }"
        "            ]"
        "          },"
        "          \"onPlay\": {"
        "            \"type\": \"SetValue\","
        "            \"componentId\": \"textComp\","
        "            \"property\": \"text\","
        "            \"value\": \"One\""
        "          }"
        "        },"
        "        {"
        "          \"type\": \"Text\","
        "          \"id\": \"textComp\","
        "          \"text\": \"One\""
        "        }"
        "      ]"
        "    }"
        "  }"
        "}";

TEST_F(ComponentEventsTest, MediaFastNormal)
{
    loadDocument(MEDIA_FAST_NORMAL);
    ASSERT_TRUE(component);

    auto video = context->findComponentById("video");
    ASSERT_TRUE(video);
    ASSERT_EQ(kComponentTypeVideo, video->getType());

    auto text = context->findComponentById("textComp");
    ASSERT_TRUE(text);
    ASSERT_EQ(kComponentTypeText, text->getType());

    ASSERT_EQ("One", text->getCalculated(kPropertyText).asString());

    // Simulate time update
    ASSERT_FALSE(ConsoleMessage());
    MediaState state(0, 1, 5, 10, false, false);
    video->updateMediaState(state);
    CheckMediaState(state, video->getCalculated());

    // Should have SetValue executed but not SendEvent considering on onTimeUpdate is fast
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();

    ASSERT_TRUE(ConsoleMessage());  // The onTimeUpdate event handler won't run SendEvent in fast mode
    ASSERT_EQ("Two", text->getCalculated(kPropertyText).asString());

    // Simulate pause from event
    state = MediaState(0, 1, 5, 10, true, false);
    video->updateMediaState(state, true);
    CheckMediaState(state, video->getCalculated());

    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();

    ASSERT_TRUE(ConsoleMessage());  // The onPause event handler won't run SendEvent in fast mode
    ASSERT_EQ("Three", text->getCalculated(kPropertyText).asString());

    // Switch back to play state
    state = MediaState(0, 1, 5, 10, false, false);
    video->updateMediaState(state, true);
    CheckMediaState(state, video->getCalculated());

    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();

    ASSERT_FALSE(ConsoleMessage());  // The onPlay event handler only changes the text value
    ASSERT_EQ("One", text->getCalculated(kPropertyText).asString());

    // Pause by user event
    state = MediaState(0, 1, 5, 10, true, false);
    video->updateMediaState(state);
    CheckMediaState(state, video->getCalculated());

    // The onPause event handler runs the SendEvent command in normal mode
    ASSERT_TRUE(root->hasEvent());
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();

    ASSERT_EQ("Three", text->getCalculated(kPropertyText).asString());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
}
