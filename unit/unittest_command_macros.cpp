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

#include "apl/time/sequencer.h"
#include "apl/engine/event.h"

#include "testeventloop.h"

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

    component->update(kUpdatePressed, 1);

    loop->advanceToEnd();
    ASSERT_EQ(1, mCommandCount[kCommandTypeSendEvent]);
    ASSERT_EQ(1, mActionCount[kCommandTypeSendEvent]);
    ASSERT_EQ(1, mIssuedCommands.size());

    auto command = std::static_pointer_cast<CoreCommand>(mIssuedCommands.at(0));
    ASSERT_EQ(Object("Hello"), command->getValue(kCommandPropertyArguments).at(0));

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    ASSERT_TRUE( event.getValue(kEventPropertyArguments).isArray());
    ASSERT_EQ(Object("Hello"), event.getValue(kEventPropertyArguments).at(0));
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

    component->update(kUpdatePressed, 1);
    loop->advanceToEnd();

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    ASSERT_EQ(Object("Goodbye"), event.getValue(kEventPropertyArguments).at(0));
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

    component->update(kUpdatePressed, 1);
    loop->advanceToEnd();

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    ASSERT_EQ(Object("Goodbye"), event.getValue(kEventPropertyArguments).at(0));
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

    component->update(kUpdatePressed, 1);
    loop->advanceToEnd();

    // First command should be Starting
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    ASSERT_EQ(Object("Starting"), event.getValue(kEventPropertyArguments).at(0));

    // Second command should be Middle
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    ASSERT_EQ(Object("Middle"), event.getValue(kEventPropertyArguments).at(0));

    // Third command should be Ending
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    ASSERT_EQ(Object("Ending"), event.getValue(kEventPropertyArguments).at(0));
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

    component->update(kUpdatePressed, 1);
    loop->advanceToEnd();

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    ASSERT_EQ(Object("Goodbye"), event.getValue(kEventPropertyArguments).at(0));
}