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

using namespace apl;

class ScreenLockTest : public DocumentWrapper {};

static const char *SCROLLVIEW =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"ScrollView\","
    "      \"id\": \"myScroll\","
    "      \"height\": 100,"
    "      \"width\": 100,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"height\": 1000,"
    "        \"width\": 100"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(ScreenLockTest, Basic) {
    loadDocument(SCROLLVIEW);

    auto ptr = executeCommand("Scroll", {{"componentId", "myScroll"},
                                         {"distance",    1},
                                         {"screenLock",  true}}, false);

    ASSERT_TRUE(root->screenLock());
    advanceTime(1000);
    ASSERT_EQ(Point(0, 100), component->scrollPosition());
    ASSERT_FALSE(root->screenLock());
}

TEST_F(ScreenLockTest, BasicWithDelay) {
    loadDocument(SCROLLVIEW);

    auto ptr = executeCommand("Scroll", {{"componentId", "myScroll"},
                                         {"distance",    1},
                                         {"screenLock",  true},
                                         {"delay",       1000}}, false);

    ASSERT_TRUE(root->screenLock());
    advanceTime(1000);
    ASSERT_EQ(Point(0, 0), component->scrollPosition());
    advanceTime(1000);
    ASSERT_EQ(Point(0, 100), component->scrollPosition());
    ASSERT_FALSE(root->screenLock());
}

TEST_F(ScreenLockTest, BasicInFastMode)
{
    loadDocument(SCROLLVIEW);

    auto ptr = executeCommand("Scroll", {{"componentId", "myScroll"},
                                         {"distance",    1},
                                         {"screenLock",  true},
                                         {"delay",       1000}}, true);

    ASSERT_FALSE(root->screenLock());
    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());

    ASSERT_TRUE(ConsoleMessage());  // The Scroll command fails in fast mode
}

TEST_F(ScreenLockTest, BasicSendEvent) {
    loadDocument(SCROLLVIEW);

    auto ptr = executeCommand("SendEvent", {{"componentId", "myScroll"},
                                            {"arguments",   std::vector<Object>{1}},
                                            {"screenLock",  true},
                                            {"delay",       0}}, false);

    ASSERT_FALSE(root->screenLock());

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());

    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());
}

TEST_F(ScreenLockTest, BasicSendEventWithDelay) {
    loadDocument(SCROLLVIEW);

    auto ptr = executeCommand("SendEvent", {{"componentId", "myScroll"},
                                            {"arguments",   std::vector<Object>{1}},
                                            {"screenLock",  true},
                                            {"delay",       1000}}, false);

    ASSERT_TRUE(root->screenLock());
    ASSERT_FALSE(root->hasEvent());

    advanceTime(1000);

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());

    ASSERT_FALSE(root->hasEvent());
    ASSERT_FALSE(root->screenLock());
    ASSERT_EQ(0, loop->size());
}


TEST_F(ScreenLockTest, BasicSendEventWithDelayFastMode) {
    loadDocument(SCROLLVIEW);

    auto ptr = executeCommand("SendEvent", {{"componentId", "myScroll"},
                                            {"arguments",   std::vector<Object>{1}},
                                            {"screenLock",  true},
                                            {"delay",       1000}}, true);

    ASSERT_FALSE(root->screenLock());
    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(0, loop->size());

    ASSERT_TRUE(ConsoleMessage());  // The SendEvent command fails in fast mode
}

static const char *ON_MOUNT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"ScrollView\","
    "      \"id\": \"myScroll\","
    "      \"height\": 100,"
    "      \"width\": 100,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"height\": 1000,"
    "        \"width\": 100"
    "      }"
    "    }"
    "  },"
    "  \"onMount\": {"
    "    \"description\": \"At start up, wait one second and scroll to the end\","
    "    \"type\": \"Scroll\","
    "    \"distance\": \"10000\","
    "    \"componentId\": \"myScroll\","
    "    \"delay\": 1000,"
    "    \"screenLock\": true"
    "  }"
    "}";


TEST_F(ScreenLockTest, OnMount) {
    loadDocument(ON_MOUNT);

    ASSERT_TRUE(root->screenLock());
    ASSERT_FALSE(root->hasEvent());

    advanceTime(1000);
    ASSERT_TRUE(root->screenLock());

    advanceTime(500);
    ASSERT_TRUE(root->screenLock());

    advanceTime(1000);
    ASSERT_EQ(Point(0, 900), component->scrollPosition());
    ASSERT_FALSE(root->screenLock());
}

static const char *ON_MOUNT_INTERRUPT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"ScrollView\","
    "      \"id\": \"myScroll\","
    "      \"height\": 100,"
    "      \"width\": 100,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"height\": 1000,"
    "        \"width\": 100,"
    "        \"items\": ["
    "          {"
    "            \"type\": \"TouchWrapper\","
    "            \"id\": \"myTouch\","
    "            \"width\": 100,"
    "            \"height\": 400,"
    "            \"onPress\": {"
    "              \"type\": \"SendEvent\","
    "              \"arguments\": ["
    "                \"a\""
    "              ]"
    "            }"
    "          }"
    "        ]"
    "      }"
    "    }"
    "  },"
    "  \"onMount\": {"
    "    \"description\": \"At start up, wait one second and scroll to the end\","
    "    \"type\": \"Scroll\","
    "    \"distance\": \"10000\","
    "    \"componentId\": \"myScroll\","
    "    \"delay\": 1000,"
    "    \"screenLock\": true"
    "  }"
    "}";

TEST_F(ScreenLockTest, OnMountInterrupt) {
    loadDocument(ON_MOUNT_INTERRUPT);

    ASSERT_TRUE(root->screenLock());
    ASSERT_FALSE(root->hasEvent());

    advanceTime(1000);
    ASSERT_TRUE(root->screenLock());

    advanceTime(400);
    ASSERT_TRUE(root->screenLock());
    auto currentPos = component->scrollPosition();
    ASSERT_TRUE(currentPos.getY() > 0);

    auto touch = context->findComponentById("myTouch");
    ASSERT_TRUE(touch);

    performTap(0, 0);
    advanceTime(600);
    ASSERT_FALSE(root->screenLock());
    ASSERT_EQ(currentPos, component->scrollPosition());

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
}

static const char *OVERLAPPING =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"commands\": {"
    "    \"BigMess\": {"
    "      \"command\": {"
    "        \"type\": \"Sequential\","
    "        \"repeatCount\": 1,"
    "        \"commands\": ["
    "          {"
    "            \"type\": \"Parallel\","
    "            \"commands\": ["
    "              {"
    "                \"type\": \"SendEvent\","
    "                \"delay\": 500,"
    "                \"arguments\": ["
    "                  \"alpha\""
    "                ],"
    "                \"screenLock\": true"
    "              },"
    "              {"
    "                \"type\": \"Sequential\","
    "                \"commands\": ["
    "                  {"
    "                    \"type\": \"Scroll\","
    "                    \"distance\": 100,"
    "                    \"componentId\": \"myScroll\","
    "                    \"screenLock\": true"
    "                  },"
    "                  {"
    "                    \"type\": \"SendEvent\","
    "                    \"delay\": 500,"
    "                    \"arguments\": ["
    "                      \"beta\""
    "                    ]"
    "                  }"
    "                ]"
    "              }"
    "            ]"
    "          }"
    "        ]"
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"ScrollView\","
    "      \"id\": \"myScroll\","
    "      \"height\": 100,"
    "      \"width\": 100,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"height\": 1000,"
    "        \"width\": 100,"
    "        \"items\": ["
    "          {"
    "            \"type\": \"TouchWrapper\","
    "            \"id\": \"myTouch\","
    "            \"width\": 100,"
    "            \"height\": 100,"
    "            \"onPress\": {"
    "              \"type\": \"BigMess\","
    "              \"delay\": 1000"
    "            }"
    "          }"
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(ScreenLockTest, Overlapping) {
    loadDocument(OVERLAPPING);

    ASSERT_FALSE(root->screenLock());
    auto touch = context->findComponentById("myTouch");
    performTap(0, 0);

    ASSERT_TRUE(root->screenLock());

    // Advance forward long enough to trigger the first SendEvent
    advanceTime(500);
    ASSERT_TRUE(component->scrollPosition().getY() > 0);
    ASSERT_TRUE(CheckSendEvent(root, "alpha"));

    // The "Scroll" event is still holding the screen lock
    ASSERT_TRUE(root->screenLock());
    advanceTime(500);
    ASSERT_EQ(Point(0, 900), component->scrollPosition());
    ASSERT_FALSE(root->screenLock());

    // The next SendEvent will fire after 500 milliseconds
    advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "beta"));

    // Once that send event fired, we immediately start the next repeat, which locks the screen again
    ASSERT_TRUE(root->screenLock());

    // Can't scroll any further
    advanceTime(500);
    ASSERT_EQ(Point(0, 900), component->scrollPosition());
    ASSERT_FALSE(root->screenLock());  // Still being held by the Send command

    advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "alpha"));
    ASSERT_TRUE(CheckSendEvent(root, "beta"));
}

TEST_F(ScreenLockTest, OverlappingWithInterrupt) {
    loadDocument(OVERLAPPING);

    ASSERT_FALSE(root->screenLock());
    auto touch = context->findComponentById("myTouch");
    performTap(0, 0);

    ASSERT_TRUE(root->screenLock());

    // Advance forward long enough to trigger the first SendEvent
    advanceTime(500);
    ASSERT_TRUE(component->scrollPosition().getY() > 0);
    ASSERT_TRUE(CheckSendEvent(root, "alpha"));

    // The "Scroll" event is still holding the screen lock
    ASSERT_TRUE(root->screenLock());
    advanceTime(500);
    ASSERT_EQ(Point(0, 900), component->scrollPosition());
    ASSERT_FALSE(root->screenLock());

    // The next SendEvent will fire after 500 milliseconds
    advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "beta"));

    // Once that send event fired, we immediately start the next repeat, which locks the screen again
    ASSERT_TRUE(root->screenLock());

    // This time we'll cancel everything with an external command
    executeCommand("SpeakItem", {{"componentId", "myScroll"}}, false);

    advanceTime(500);
    ASSERT_FALSE(root->hasEvent());
}

