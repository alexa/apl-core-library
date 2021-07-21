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

#include <algorithm>

#include "apl/time/sequencer.h"
#include "apl/engine/event.h"

#include "../testeventloop.h"

using namespace apl;

class CommandOpenURLTest : public CommandTest {
protected:
    void SetUp() override
    {
        config->allowOpenUrl(true);
    }
};

static const char *OPEN_URL =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"parameters\": [],"
    "    \"item\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"onPress\": {"
    "        \"type\": \"OpenURL\","
    "        \"source\": \"http://www.amazon.com\","
    "        \"onFail\": {"
    "          \"type\": \"SendEvent\","
    "          \"arguments\": ["
    "            \"failed\","
    "            \"${event.source.source}\","
    "            \"${event.source.handler}\","
    "            \"${event.source.value}\""
    "          ]"
    "        }"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(CommandOpenURLTest, OpenURL)
{
    loadDocument(OPEN_URL);
    performClick(1, 1);

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeOpenURL, event.getType());
    ASSERT_EQ(Object("http://www.amazon.com"), event.getValue(kEventPropertySource));

    event.getActionRef().resolve(0);
    ASSERT_FALSE(root->hasEvent());
}


TEST_F(CommandOpenURLTest, OpenURLFail)
{
    loadDocument(OPEN_URL);
    performClick(1, 1);

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeOpenURL, event.getType());
    ASSERT_EQ(Object("http://www.amazon.com"), event.getValue(kEventPropertySource));

    event.getActionRef().resolve(23);
    ASSERT_TRUE(CheckSendEvent(root, "failed", "OpenURL", "Fail", 23));
}

TEST_F(CommandOpenURLTest, OpenURLNotAllowed)
{
    config->allowOpenUrl(false);
    loadDocument(OPEN_URL);
    performClick(1, 1);

    ASSERT_TRUE(CheckSendEvent(root, "failed", "OpenURL", "Fail", 405));
}


static const char *OPEN_URL_WITH_DELAY =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"parameters\": [],"
    "    \"item\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"onPress\": ["
    "        {"
    "          \"type\": \"OpenURL\","
    "          \"delay\": 1000,"
    "          \"source\": \"http://www.amazon.com\","
    "          \"onFail\": {"
    "            \"type\": \"SendEvent\","
    "            \"delay\": 1000,"
    "            \"arguments\": ["
    "              \"failed\","
    "              \"${event.source.source}\","
    "              \"${event.source.handler}\","
    "              \"${event.source.value}\""
    "            ]"
    "          }"
    "        },"
    "        {"
    "          \"type\": \"SendEvent\","
    "          \"delay\": 1000,"
    "          \"arguments\": ["
    "            \"succeeded\""
    "          ]"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(CommandOpenURLTest, OpenURLDelay)
{
    loadDocument(OPEN_URL_WITH_DELAY);
    performClick(1, 1);

    ASSERT_FALSE(root->hasEvent());
    loop->advanceToTime(1000);

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeOpenURL, event.getType());
    ASSERT_EQ(Object("http://www.amazon.com"), event.getValue(kEventPropertySource));

    event.getActionRef().resolve();
    ASSERT_FALSE(root->hasEvent());

    loop->advanceToTime(2000);
    ASSERT_TRUE(CheckSendEvent(root, "succeeded"));
}

TEST_F(CommandOpenURLTest, OpenURLDelayFail)
{
    loadDocument(OPEN_URL_WITH_DELAY);
    performClick(1, 1);

    ASSERT_FALSE(root->hasEvent());
    advanceTime(1000);

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeOpenURL, event.getType());
    ASSERT_EQ(Object("http://www.amazon.com"), event.getValue(kEventPropertySource));

    event.getActionRef().resolve(123);
    ASSERT_FALSE(root->hasEvent());   // The onFail runs in slow mode
    advanceTime(1000);
    ASSERT_TRUE(CheckSendEvent(root, "failed", "OpenURL", "Fail", 123));
}

const static char * OPEN_URL_WITH_CANCEL =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"parameters\": [],"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"cancel\","
    "          \"height\": 10"
    "        },"
    "        {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"go\","
    "          \"height\": 10,"
    "          \"onPress\": ["
    "            {"
    "              \"type\": \"OpenURL\","
    "              \"delay\": 1000,"
    "              \"source\": \"http://www.amazon.com\","
    "              \"onFail\": {"
    "                \"type\": \"SendEvent\","
    "                \"delay\": 1000,"
    "                \"arguments\": ["
    "                  \"failed\","
    "                  \"${event.source.source}\","
    "                  \"${event.source.handler}\","
    "                  \"${event.source.value}\""
    "                ]"
    "              }"
    "            },"
    "            {"
    "              \"type\": \"SendEvent\","
    "              \"delay\": 1000,"
    "              \"arguments\": ["
    "                \"succeeded\""
    "              ]"
    "            }"
    "          ]"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(CommandOpenURLTest, OpenURLCancel)
{
    loadDocument(OPEN_URL_WITH_CANCEL);
    auto cancelButton = root->context().findComponentById("cancel");
    auto goButton = root->context().findComponentById("go");
    ASSERT_TRUE(cancelButton);
    ASSERT_TRUE(goButton);

    performClick(1, 10);

    ASSERT_FALSE(root->hasEvent());
    loop->advanceToTime(500);

    performClick(1, 1);
    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(CommandOpenURLTest, OpenURLCancelDelay)
{
    loadDocument(OPEN_URL_WITH_CANCEL);
    auto cancelButton = root->context().findComponentById("cancel");
    auto goButton = root->context().findComponentById("go");
    ASSERT_TRUE(cancelButton);
    ASSERT_TRUE(goButton);

    performClick(1, 10);

    ASSERT_FALSE(root->hasEvent());
    loop->advanceToTime(1000);

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeOpenURL, event.getType());
    ASSERT_EQ(Object("http://www.amazon.com"), event.getValue(kEventPropertySource));

    event.getActionRef().resolve();
    ASSERT_FALSE(root->hasEvent());

    loop->advanceToTime(1500);  // Nothing should have happened yet
    ASSERT_FALSE(root->hasEvent());

    performClick(1, 0);

    loop->advanceToEnd();
    ASSERT_FALSE(root->hasEvent());
}


const char* OPEN_URL_WITH_ARRAY_FAIL = "[\n"
                                       "    {\n"
                                       "        \"type\": \"OpenURL\",\n"
                                       "        \"source\": \"http://amazon.com\",\n"
                                       "        \"onFail\": [\n"
                                       "            {\n"
                                       "                \"type\": \"SetValue\",\n"
                                       "                \"componentId\": \"text\",\n"
                                       "                \"property\": \"text\",\n"
                                       "                \"value\": \"Open URL Failed\"\n"
                                       "            },\n"
                                       "            {\n"
                                       "                \"type\": \"SetValue\",\n"
                                       "                \"componentId\": \"text\",\n"
                                       "                \"property\": \"color\",\n"
                                       "                \"value\": \"red\"\n"
                                       "            }\n"
                                       "        ]\n"
                                       "    }\n"
                                       "]";

const char* TEXT_FOR_OPEN_URL = "{\n"
                                "    \"type\": \"APL\",\n"
                                "    \"version\": \"1.0\",\n"
                                "    \"theme\": \"auto\",\n"
                                "    \"mainTemplate\": {\n"
                                "        \"item\": {\n"
                                "            \"type\": \"Text\",\n"
                                "            \"text\": \"Before Open URL\",\n"
                                "            \"id\": \"text\",\n"
                                "            \"color\": \"black\"\n"
                                "        }\n"
                                "    }\n"
                                "}";

TEST_F(CommandOpenURLTest, OpenURLArrayFail)
{
    loadDocument(TEXT_FOR_OPEN_URL);
    auto text = root->context().findComponentById("text");
    ASSERT_TRUE(text);

    rapidjson::Document doc;
    doc.Parse(OPEN_URL_WITH_ARRAY_FAIL);
    auto action = root->executeCommands(apl::Object(doc), false);
    bool actionResolved = false;
    action->then([&](const ActionPtr& ptr) {
        actionResolved = true;
    });
    loop->advanceToEnd();
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(event.getType(), kEventTypeOpenURL);
    event.getActionRef().resolve(1);
    loop->advanceToEnd();
    ASSERT_TRUE(actionResolved);

    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyColorNonKaraoke));
    ASSERT_TRUE(CheckDirty(root, text));
    ASSERT_EQ(text->getCalculated(kPropertyText).asString(), "Open URL Failed");
    ASSERT_EQ(text->getCalculated(kPropertyColor).getColor(), 0xff0000ff);
}

