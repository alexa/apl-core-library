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

class CommandSelectTest : public CommandTest {
};

static const char *SIMPLE_SERIES_OF_COMMANDS =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.3\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"onPress\": {"
    "        \"type\": \"Select\","
    "        \"commands\": ["
    "          {"
    "            \"type\": \"SendEvent\","
    "            \"arguments\": ["
    "              \"Item 1\""
    "            ]"
    "          },"
    "          {"
    "            \"type\": \"SendEvent\","
    "            \"arguments\": ["
    "              \"Item 2\""
    "            ]"
    "          }"
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(CommandSelectTest, Basic)
{
    loadDocument(SIMPLE_SERIES_OF_COMMANDS);
    ASSERT_TRUE(component);

    performClick(1, 1);
    ASSERT_TRUE(CheckSendEvent(root, "Item 1"));
    ASSERT_FALSE(root->hasEvent());
}

static const char *BASIC_FIRE_SECOND =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.3\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"onPress\": {"
    "        \"type\": \"Select\","
    "        \"commands\": ["
    "          {"
    "            \"type\": \"SendEvent\","
    "            \"when\": false,"
    "            \"arguments\": ["
    "              \"Item 1\""
    "            ]"
    "          },"
    "          {"
    "            \"type\": \"SendEvent\","
    "            \"arguments\": ["
    "              \"Item 2\""
    "            ]"
    "          }"
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";


TEST_F(CommandSelectTest, BasicSecond)
{
    loadDocument(BASIC_FIRE_SECOND);
    ASSERT_TRUE(component);

    performClick(1, 1);
    ASSERT_TRUE(CheckSendEvent(root, "Item 2"));
    ASSERT_FALSE(root->hasEvent());
}

static const char *DATA_SELECTION =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.3\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"onPress\": {"
    "        \"type\": \"Select\","
    "        \"commands\": {"
    "          \"type\": \"SendEvent\","
    "          \"when\": \"${data == 3}\","
    "          \"arguments\": ["
    "            \"Value=${data}\""
    "          ]"
    "        },"
    "        \"data\": ["
    "          1,"
    "          2,"
    "          3,"
    "          4"
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(CommandSelectTest, DataSelection)
{
    loadDocument(DATA_SELECTION);
    ASSERT_TRUE(component);

    performClick(1, 1);
    ASSERT_TRUE(CheckSendEvent(root, "Value=3"));
    ASSERT_FALSE(root->hasEvent());
}


static const char *DATA_SELECTION_BY_INDEX =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.3\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"onPress\": {"
    "        \"type\": \"Select\","
    "        \"commands\": {"
    "          \"type\": \"SendEvent\","
    "          \"when\": \"${index == 3}\","
    "          \"arguments\": ["
    "            \"Value=${data}\""
    "          ]"
    "        },"
    "        \"data\": ["
    "          1,"
    "          2,"
    "          3,"
    "          4"
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(CommandSelectTest, DataSelectionByIndex)
{
    loadDocument(DATA_SELECTION_BY_INDEX);
    ASSERT_TRUE(component);

    performClick(1, 1);
    ASSERT_TRUE(CheckSendEvent(root, "Value=4"));
    ASSERT_FALSE(root->hasEvent());
}

static const char *DATA_SELECTION_BY_INDEX_AND_LENGTH =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.3\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"onPress\": {"
    "        \"type\": \"Select\","
    "        \"commands\": {"
    "          \"type\": \"SendEvent\","
    "          \"when\": \"${index == length - 3}\","  // Select the third from the end
    "          \"arguments\": ["
    "            \"Value=${data}\""
    "          ]"
    "        },"
    "        \"data\": ["
    "          1,"
    "          2,"
    "          3,"
    "          4"
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(CommandSelectTest, DataSelectionByIndexAndLength)
{
    loadDocument(DATA_SELECTION_BY_INDEX_AND_LENGTH);
    ASSERT_TRUE(component);

    performClick(1, 1);
    ASSERT_TRUE(CheckSendEvent(root, "Value=2"));
    ASSERT_FALSE(root->hasEvent());
}

static const char *DATA_SELECTION_MULTIPLE_COMMANDS =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.3\","
    "  \"mainTemplate\": {"
    "    \"parameters\": ["
    "      \"payload\""
    "    ],"
    "    \"items\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"onPress\": {"
    "        \"type\": \"Select\","
    "        \"commands\": ["
    "          {"
    "            \"type\": \"SendEvent\","
    "            \"when\": \"${data.type == payload.type}\","
    "            \"arguments\": ["
    "              \"Matched by type ${data.name}/${data.type}\""
    "            ]"
    "          },"
    "          {"
    "            \"type\": \"SendEvent\","
    "            \"when\": \"${data.name == payload.name}\","
    "            \"arguments\": ["
    "              \"Matched by name ${data.name}/${data.type}\""
    "            ]"
    "          }"
    "        ],"
    "        \"otherwise\": {"
    "          \"type\": \"SendEvent\","
    "          \"arguments\": ["
    "            \"No match\""
    "          ]"
    "        },"
    "        \"data\": ["
    "          {"
    "            \"type\": \"horse\","
    "            \"name\": \"Sam\""
    "          },"
    "          {"
    "            \"type\": \"cow\","
    "            \"name\": \"Chris\""
    "          },"
    "          {"
    "            \"type\": \"horse\","
    "            \"name\": \"Murdock\""
    "          },"
    "          {"
    "            \"type\": \"cow\","
    "            \"name\": \"Daisy\""
    "          }"
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

static std::map<std::string, std::string> sMultipleCommandTests = {
    {R"({"type": "cow", "name": "Murdock"})", "Matched by type Chris/cow"},
    {R"({"type": "pig", "name": "Murdock"})", "Matched by name Murdock/horse"},
    {R"({"type": "horse", "name": "Sam"})", "Matched by type Sam/horse"},
    {R"({"type": "pig", "name": "Oink"})", "No match"},
};

TEST_F(CommandSelectTest, DataSelectionMultipleCommands)
{
    for (const auto& m : sMultipleCommandTests) {
        loadDocument(DATA_SELECTION_MULTIPLE_COMMANDS, m.first.c_str());
        ASSERT_TRUE(component) << m.second;

        performClick(1, 1);
        ASSERT_TRUE(CheckSendEvent(root, m.second)) << m.second;

        ASSERT_FALSE(root->hasEvent()) << m.second;
    }
}

static const char *MULTIPLE_OTHERWISE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.3\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"onPress\": {"
    "        \"type\": \"Select\","
    "        \"commands\": ["
    "          {"
    "            \"type\": \"SendEvent\","
    "            \"when\": \"${data == 5}\""
    "          }"
    "        ],"
    "        \"otherwise\": ["
    "          {"
    "            \"type\": \"SendEvent\","
    "            \"arguments\": ["
    "              \"alpha\""
    "            ]"
    "          },"
    "          {"
    "            \"type\": \"SendEvent\","
    "            \"arguments\": ["
    "              \"bravo\""
    "            ]"
    "          }"
    "        ],"
    "        \"data\": ["
    "          1,"
    "          2,"
    "          3"
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(CommandSelectTest, MultipleOtherwise)
{
    loadDocument(MULTIPLE_OTHERWISE);
    ASSERT_TRUE(component);

    performClick(1, 1);

    ASSERT_TRUE(CheckSendEvent(root, "alpha"));
    ASSERT_TRUE(CheckSendEvent(root, "bravo"));

    ASSERT_FALSE(root->hasEvent());
}
