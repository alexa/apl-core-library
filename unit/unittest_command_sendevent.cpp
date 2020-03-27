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

#include "testeventloop.h"

#include "apl/engine/event.h"
#include "apl/primitives/object.h"

using namespace apl;

class CommandSendEventTest : public CommandTest {};

/**
 * The old version of Aria (1.0) converted all arguments into strings
 */
static const char * SEND_EVENT_OLD_ARGUMENTS =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"resources\": ["
    "    {"
    "      \"color\": {"
    "        \"accent\": \"#00caff\""
    "      },"
    "      \"dimension\": {"
    "        \"absDimen\": \"150dp\","
    "        \"relDimen\": \"50%\","
    "        \"autoDimen\": \"auto\""
    "      }"
    "    }"
    "  ],"
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"onPress\": {"
    "        \"type\": \"SendEvent\","
    "        \"arguments\": ["
    "          null,"
    "          false,"
    "          true,"
    "          \"string\","
    "          10,"
    "          2.5,"
    "          \"@accent\","
    "          \"@absDimen\","
    "          \"@relDimen\","
    "          \"@autoDimen\","
    "          ["
    "            1,"
    "            2,"
    "            3"
    "          ],"
    "          {"
    "            \"a\": 1,"
    "            \"b\": 2"
    "          }"
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

const static std::vector<std::string> EXPECTED = {
    "",  // null
    "false", // false
    "true",
    "string",
    "10",
    "2.5",
    "#00caffff",   // Alpha will be apppended
    "150dp",
    "50%",
    "auto",
    "[1.0,2.0,3.0]",   // Array - note that we use the rapidjson serialization of a number
    "{\"a\":1.0,\"b\":2.0}"    // Object
};

TEST_F(CommandSendEventTest, WithOldArguments)
{
    loadDocument(SEND_EVENT_OLD_ARGUMENTS);

    component->update(kUpdatePressed, 1);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    auto args = event.getValue(kEventPropertyArguments);
    ASSERT_TRUE(args.isArray());

    ASSERT_EQ(EXPECTED.size(), args.size());
    for (int i = 0 ; i < EXPECTED.size() ; i++)
        ASSERT_TRUE(IsEqual(EXPECTED.at(i), args.at(i))) << i << ": " << EXPECTED.at(i);
}

/**
 * The new version of APL (1.1) return JSON objects
 */
static const char * SEND_EVENT_NEW_ARGUMENTS =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"resources\": ["
    "    {"
    "      \"color\": {"
    "        \"accent\": \"#00caff\""
    "      },"
    "      \"dimension\": {"
    "        \"absDimen\": \"150dp\","
    "        \"relDimen\": \"50%\","
    "        \"autoDimen\": \"auto\""
    "      }"
    "    }"
    "  ],"
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"onPress\": {"
    "        \"type\": \"SendEvent\","
    "        \"arguments\": ["
    "          null,"
    "          false,"
    "          true,"
    "          \"string\","
    "          10,"
    "          2.5,"
    "          \"@accent\","
    "          \"@absDimen\","
    "          \"@relDimen\","
    "          \"@autoDimen\","
    "          ["
    "            1,"
    "            2,"
    "            3"
    "          ],"
    "          {"
    "            \"a\": 1,"
    "            \"b\": 2"
    "          }"
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";;

const static std::vector<Object> EXPECTED_NEW = {
    Object::NULL_OBJECT(),  // null
    Object::FALSE_OBJECT(), // false
    Object::TRUE_OBJECT(),
    Object("string"),
    Object(10),
    Object(2.5),
    Object(Color(0x00caffff)),   // Alpha will be apppended
    Object(Dimension(150)),
    Object(Dimension(DimensionType::Relative, 50)),
    Object(Dimension(DimensionType::Auto, 0)),
    std::vector<Object>{1,2,3},   // Array
    std::make_shared<std::map<std::string, Object>>(
        std::map<std::string, Object>{{ "b", 2 }, { "a", 1 }
    })
};

TEST_F(CommandSendEventTest, WithNewArguments)
{
    loadDocument(SEND_EVENT_NEW_ARGUMENTS);

    component->update(kUpdatePressed, 1);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    auto args = event.getValue(kEventPropertyArguments);
    ASSERT_TRUE(args.isArray());

    ASSERT_EQ(EXPECTED_NEW.size(), args.size());
    for (int i = 0 ; i < EXPECTED_NEW.size() ; i++)
        ASSERT_TRUE(IsEqual(EXPECTED_NEW.at(i), args.at(i))) << i << ": " << EXPECTED_NEW.at(i);
}

static const char * SEND_EVENT_CASE_INSENSITIVE =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"item\": {"
        "      \"type\": \"TouchWrapper\","
        "      \"onPress\": {"
        "        \"type\": \"sendEvent\","
        "        \"arguments\": ["
        "          1,"
        "          \"1\","
        "          null"
        "        ]"
        "      }"
        "    }"
        "  }"
        "}";

TEST_F(CommandSendEventTest, CaSeInsEnsitIVE)
{
    loadDocument(SEND_EVENT_CASE_INSENSITIVE);

    component->update(kUpdatePressed, 1);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    auto args = event.getValue(kEventPropertyArguments);
    ASSERT_TRUE(args.isArray());
    ASSERT_EQ(3, args.size());
    ASSERT_TRUE(IsEqual(1, args.at(0)));
    ASSERT_TRUE(IsEqual("1", args.at(1)));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), args.at(2)));
}