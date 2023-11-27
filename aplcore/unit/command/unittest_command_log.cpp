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
#include "apl/primitives/object.h"

using namespace apl;

class CommandLogTest : public CommandTest {};

static const char *LOG_WITH_ARGUMENTS = R"apl({
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "width": "100%",
      "height": "100%",
      "onPress": [
        {
          "type": "Log",
          "level": "warn",
          "message": "Small warning",
          "arguments": [
            "A",
            "B",
            "${event.source.type}"
          ]
        }
      ]
    }
  }
})apl";

TEST_F(CommandLogTest, LogWithArguments)
{
    loadDocument(LOG_WITH_ARGUMENTS);
    ASSERT_TRUE(component);

    performClick(10, 10);
    advanceTime(500);

    ASSERT_EQ(1, session->logCommandMessages.size());

    auto m = session->logCommandMessages[0];
    ASSERT_EQ(LogLevel::kWarn, m.level);
    ASSERT_EQ("Small warning", m.text);

    auto arguments = m.arguments.getArray();
    ASSERT_EQ(3, arguments.size());
    ASSERT_EQ("A", arguments.at(0).asString());
    ASSERT_EQ("B", arguments.at(1).asString());
    ASSERT_EQ("TouchWrapper", arguments.at(2).asString());

    auto source = m.origin.getMap();
    ASSERT_EQ("TouchWrapper", source.at("type").asString());
}

static const char *LOG_WITH_LEVEL_VARIANTS = R"apl({
  "type": "APL",
  "version": "2023.3",
  "onMount": [
    {
      "type": "Log"
    },
    {
      "type": "Log",
      "level": "error",
      "message": "Error as enum string"
    },
    {
      "type": "Log",
      "level": "${Log.CRITICAL}",
      "message": "Critical as constant"
    },
    {
      "type": "Log",
      "level": "${Log.levelValue('warn')}",
      "message": "Warn as value"
    },
    {
      "type": "Log",
      "level": "${Log.levelName(Log.ERROR)}",
      "message": "Error as name"
    },
    {
      "type": "Log",
      "level": "whatever",
      "message": "Unsupported level defaults to info"
    },
    {
      "type": "Log",
      "level": 0,
      "message": "Zero happens to be DEBUG"
    }
  ],
  "mainTemplate": {
    "items": {
      "type": "Text",
      "text": "Hello, logger!"
    }
  }
})apl";

TEST_F(CommandLogTest, LogSupportsNumericLevels)
{
    loadDocument(LOG_WITH_LEVEL_VARIANTS);
    ASSERT_TRUE(component);

    const std::vector<std::pair<LogLevel, std::string>> expected = {
        {LogLevel::kInfo,     ""}, // Info and blank message by default
        {LogLevel::kError,    "Error as enum string"},
        {LogLevel::kCritical, "Critical as constant"},
        {LogLevel::kWarn,     "Warn as value"},
        {LogLevel::kError,    "Error as name"},
        {LogLevel::kInfo,     "Unsupported level defaults to info"},
        {LogLevel::kDebug,    "Zero happens to be DEBUG"},
    };

    auto& actual = session->logCommandMessages;
    ASSERT_EQ(expected.size(), actual.size());

    for (int i = 0; i < expected.size(); i++) {
        ASSERT_EQ(expected[i].first, actual[i].level);
        ASSERT_EQ(expected[i].second, actual[i].text);
        ASSERT_EQ(ObjectArray{}, actual[i].arguments.getArray());
        ASSERT_EQ("Document", actual[i].origin.getMap().at("type").asString());
    }
}
