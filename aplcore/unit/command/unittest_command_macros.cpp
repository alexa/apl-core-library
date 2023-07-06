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

#include "apl/time/sequencer.h"
#include "apl/engine/event.h"

#include "../testeventloop.h"

using namespace apl;

static const char *BASIC_MACRO =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"commands\": {"
    "    \"basic\": {"
    "      \"parameters\": [],"
    "      \"commands\": {"
    "        \"type\": \"SendEvent\","
    "        \"arguments\": ["
    "          \"Hello\""
    "        ]"
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"onPress\": {"
    "        \"type\": \"basic\""
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(CommandTest, BasicMacro)
{
    loadDocument(BASIC_MACRO);

    auto map = component->getCalculated();
    auto onPress = map[kPropertyOnPress];

    ASSERT_TRUE(onPress.isArray());
    ASSERT_EQ(1, onPress.size());

    performClick(1, 1);

    loop->advanceToEnd();
    ASSERT_EQ(1, mCommandCount[kCommandTypeSendEvent]);
    ASSERT_EQ(1, mActionCount[kCommandTypeSendEvent]);
    ASSERT_EQ(1, mIssuedCommands.size());

    auto command = std::static_pointer_cast<CoreCommand>(mIssuedCommands.at(0));
    ASSERT_EQ(Object("Hello"), command->getValue(kCommandPropertyArguments).at(0));

    ASSERT_TRUE(CheckSendEvent(root, "Hello"));
}

TEST_F(CommandTest, BasicMacroInfo)
{
    loadDocument(BASIC_MACRO);

    auto count = root->info().count(Info::kInfoTypeCommand);
    ASSERT_EQ(1, count);

    auto p = root->info().at(Info::kInfoTypeCommand, 0);
    ASSERT_STREQ("basic", p.first.c_str());
    ASSERT_STREQ("_main/commands/basic", p.second.c_str());
}

static const char *ARG_MACRO =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"commands\": {"
    "    \"basic\": {"
    "      \"parameters\": ["
    "        {"
    "          \"name\": \"arg\","
    "          \"default\": \"Hello\""
    "        }"
    "      ],"
    "      \"commands\": {"
    "        \"type\": \"SendEvent\","
    "        \"arguments\": \"${arg}\""
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"onPress\": {"
    "        \"type\": \"basic\","
    "        \"arg\": \"Goodbye\""
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(CommandTest, ArgumentMacro)
{
    loadDocument(ARG_MACRO);

    auto map = component->getCalculated();
    auto onPress = map[kPropertyOnPress];

    ASSERT_TRUE(onPress.isArray());
    ASSERT_EQ(1, onPress.size());

    performClick(1, 1);
    loop->advanceToEnd();

    ASSERT_TRUE(CheckSendEvent(root, "Goodbye"));
}

static const char *ENABLED_CHOICES =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"commands\": {"
    "    \"basic\": {"
    "      \"parameters\": ["
    "        {"
    "          \"name\": \"arg\","
    "          \"default\": \"Hello\""
    "        },"
    "        {"
    "          \"name\": \"enable\","
    "          \"default\": true"
    "        }"
    "      ],"
    "      \"commands\": {"
    "        \"type\": \"SendEvent\","
    "        \"when\": \"${enable}\","
    "        \"arguments\": \"${arg}\""
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"onPress\": ["
    "        {"
    "          \"type\": \"basic\","
    "          \"enable\": false"
    "        },"
    "        {"
    "          \"type\": \"basic\","
    "          \"arg\": \"Goodbye\""
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(CommandTest, EnabledArguments)
{
    loadDocument(ENABLED_CHOICES);

    auto map = component->getCalculated();
    auto onPress = map[kPropertyOnPress];

    ASSERT_TRUE(onPress.isArray());
    ASSERT_EQ(2, onPress.size());

    performClick(1, 1);
    loop->advanceToEnd();

    ASSERT_TRUE(CheckSendEvent(root, "Goodbye"));
}


static const char *PASSING_COMMAND_AS_ARGUMENT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"commands\": {"
    "    \"lower\": {"
    "      \"parameters\": ["
    "        \"insertedCommand\""
    "      ],"
    "      \"commands\": ["
    "        {"
    "          \"type\": \"SendEvent\","
    "          \"arguments\": \"Starting\""
    "        },"
    "        \"${insertedCommand}\","
    "        {"
    "          \"type\": \"SendEvent\","
    "          \"arguments\": \"Ending\""
    "        }"
    "      ]"
    "    },"
    "    \"upper\": {"
    "      \"parameters\": ["
    "        \"arg\""
    "      ],"
    "      \"commands\": {"
    "        \"type\": \"lower\","
    "        \"insertedCommand\": \"${arg}\""
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"onPress\": {"
    "        \"type\": \"upper\","
    "        \"arg\": {"
    "          \"type\": \"SendEvent\","
    "          \"arguments\": \"Middle\""
    "        }"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(CommandTest, PassingCommandAsArgument)
{
    loadDocument(PASSING_COMMAND_AS_ARGUMENT);

    auto map = component->getCalculated();
    auto onPress = map[kPropertyOnPress];

    performClick(1, 1);
    loop->advanceToEnd();

    // First command should be Starting
    ASSERT_TRUE(CheckSendEvent(root, "Starting"));

    // Second command should be Middle
    ASSERT_TRUE(CheckSendEvent(root, "Middle"));

    // Third command should be Ending
    ASSERT_TRUE(CheckSendEvent(root, "Ending"));
}

static const char *NESTED_MACRO =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"commands\": {"
    "    \"basic\": {"
    "      \"parameters\": ["
    "        {"
    "          \"name\": \"arg\","
    "          \"default\": \"Hello\""
    "        }"
    "      ],"
    "      \"commands\": {"
    "        \"type\": \"SendEvent\","
    "        \"arguments\": \"${arg}\""
    "      }"
    "    },"
    "    \"basic1\": {"
    "      \"commands\": {"
    "        \"type\": \"basic\","
    "        \"arg\": \"Goodbye\""
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"onPress\": {"
    "        \"type\": \"basic1\""
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(CommandTest, NestedMacro)
{
    loadDocument(NESTED_MACRO);

    auto map = component->getCalculated();
    auto onPress = map[kPropertyOnPress];

    ASSERT_TRUE(onPress.isArray());
    ASSERT_EQ(1, onPress.size());

    performClick(1, 1);
    loop->advanceToEnd();

    ASSERT_TRUE(CheckSendEvent(root, "Goodbye"));
}
