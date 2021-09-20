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

class CommandSendEventTest : public CommandTest {};

/**
 * The old version of Aria (1.0) converted all arguments into strings
 */
static const char * SEND_EVENT_OLD_ARGUMENTS = R"apl({
  "type": "APL",
  "version": "1.0",
  "resources": [
    {
      "color": {
        "accent": "#00caff"
      },
      "dimension": {
        "absDimen": "150dp",
        "relDimen": "50%",
        "autoDimen": "auto"
      }
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "onPress": {
        "type": "SendEvent",
        "arguments": [
          null,
          false,
          true,
          "string",
          10,
          2.5,
          "@accent",
          "@absDimen",
          "@relDimen",
          "@autoDimen",
          [
            1,
            2,
            3
          ],
          {
            "a": 1,
            "b": 2
          }
        ]
      }
    }
  }
})apl";

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

    performClick(1, 1);
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
static const char * SEND_EVENT_NEW_ARGUMENTS = R"apl({
  "type": "APL",
  "version": "1.1",
  "resources": [
    {
      "color": {
        "accent": "#00caff"
      },
      "dimension": {
        "absDimen": "150dp",
        "relDimen": "50%",
        "autoDimen": "auto"
      }
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "onPress": {
        "type": "SendEvent",
        "arguments": [
          null,
          false,
          true,
          "string",
          10,
          2.5,
          "@accent",
          "@absDimen",
          "@relDimen",
          "@autoDimen",
          [
            1,
            2,
            3
          ],
          {
            "a": 1,
            "b": 2
          }
        ]
      }
    }
  }
})apl";

const static std::vector<Object> EXPECTED_NEW = {
    Object::NULL_OBJECT(),  // null
    Object::FALSE_OBJECT(), // false
    Object::TRUE_OBJECT(),
    Object("string"),
    Object(10),
    Object(2.5),
    Object("#00caffff"),   // Alpha will be apppended
    Object(150),
    Object("50%"),
    Object("auto"),
    std::vector<Object>{1,2,3},   // Array
    std::make_shared<std::map<std::string, Object>>(
        std::map<std::string, Object>{{ "b", 2 }, { "a", 1 }
    })
};

TEST_F(CommandSendEventTest, WithNewArguments)
{
    loadDocument(SEND_EVENT_NEW_ARGUMENTS);

    performClick(1, 1);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    auto args = event.getValue(kEventPropertyArguments);
    ASSERT_TRUE(args.isArray());

    ASSERT_EQ(EXPECTED_NEW.size(), args.size());
    for (int i = 0 ; i < EXPECTED_NEW.size() ; i++)
        ASSERT_TRUE(IsEqual(EXPECTED_NEW.at(i), args.at(i))) << i << ": " << EXPECTED_NEW.at(i);
}

static const char * SEND_EVENT_CASE_INSENSITIVE = R"apl({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "onPress": {
        "type": "sendEvent",
        "arguments": [
          1,
          "1",
          null
        ]
      }
    }
  }
})apl";

TEST_F(CommandSendEventTest, CaSeInsEnsitIVE)
{
    loadDocument(SEND_EVENT_CASE_INSENSITIVE);

    performClick(1, 1);
    ASSERT_TRUE(CheckSendEvent(root, 1, "1", Object::NULL_OBJECT()));
}


static const char *INTERESTING_ARGUMENTS = R"apl({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "EditText",
      "onSubmit": [
        {
          "type": "SendEvent",
          "arguments": [
            "submit",
            "${Math}",
            "${event}",
            "${event.source.value}"
          ]
        }
      ]
    }
  }
})apl";

TEST_F(CommandSendEventTest, InterestingArguments)
{
    loadDocument(INTERESTING_ARGUMENTS);

    ASSERT_TRUE(component);

    component->update(kUpdateTextChange, "woof");
    component->update(kUpdateSubmit, 0);

    loop->advanceToEnd();
    ASSERT_TRUE(CheckSendEvent(root,
                               "submit",
                               "MAP[]",
                               "MAP[2]",
                               "woof"));
}


static const char *PAYLOAD_DOC = R"apl({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "parameters": [ "payload" ],
    "item": {
      "type": "EditText",
      "text": "${payload.name} the ${payload.species}",
      "onSubmit": [
        {
          "type": "SendEvent",
          "arguments": [
            "submit",
            "${payload}"
          ]
        }
      ]
    }
  }
})apl";

static const char *PAYLOAD_CONTENT = R"apl({
  "name": "Pepper",
  "species": "Dog",
  "disposition": "Happy"
})apl";

TEST_F(CommandSendEventTest, Payload)
{
    loadDocument(PAYLOAD_DOC, PAYLOAD_CONTENT);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual("Pepper the Dog", component->getCalculated(kPropertyText).asString()));

    component->update(kUpdateSubmit, 0);
    loop->advanceToEnd();

    ASSERT_TRUE(CheckSendEvent(root,
                               "submit",
                               "MAP[3]"));

    auto ptr = executeCommand("SendEvent", {{"arguments", ObjectArray{"${payload}"}}}, false);
    ASSERT_TRUE(CheckSendEvent(root, Object(std::make_shared<ObjectMap>(ObjectMap{{"disposition", "Happy"},
                                                                                  {"name",        "Pepper"},
                                                                                  {"species",     "Dog"}}))));
}

static const char *SENDEVENT_WITH_FLAGS = R"apl({
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "width": "100%",
      "height": "100%",
      "onPress": [
        {
          "type": "SendEvent",
          "arguments": ["I_AM_AN_ARGUMENT"],
          "flags": { "one": true, "two": false, "three": 7 }
        }
      ]
    }
  }
})apl";

TEST_F(CommandSendEventTest, SendEventWithFlags)
{
    loadDocument(SENDEVENT_WITH_FLAGS);
    ASSERT_TRUE(component);

    performClick(10, 10);
    advanceTime(500);

    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    auto arguments = event.getValue(kEventPropertyArguments).getArray();
    ASSERT_EQ(Object("I_AM_AN_ARGUMENT"), arguments.at(0));
    auto flags = event.getValue(kEventPropertyFlags);
    ASSERT_TRUE(flags.isMap());
    ASSERT_EQ(3, flags.size());
    ASSERT_EQ(Object(true), flags.get("one"));
    ASSERT_EQ(Object(false), flags.get("two"));
    ASSERT_EQ(Object(7), flags.get("three"));
}

TEST_F(CommandSendEventTest, SendEventWithDefaultFlags)
{
    auto defaultFlags = std::make_shared<ObjectMap>();
    defaultFlags->emplace("four", "I_AM_DEFAULT");
    defaultFlags->emplace("three", "OVERRIDE_ME");
    config->set(RootProperty::kSendEventAdditionalFlags, defaultFlags);
    loadDocument(SENDEVENT_WITH_FLAGS);
    ASSERT_TRUE(component);

    performClick(10, 10);
    advanceTime(500);

    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    auto arguments = event.getValue(kEventPropertyArguments).getArray();
    ASSERT_EQ(Object("I_AM_AN_ARGUMENT"), arguments.at(0));
    auto flags = event.getValue(kEventPropertyFlags);
    ASSERT_TRUE(flags.isMap());
    ASSERT_EQ(4, flags.size());
    ASSERT_EQ(Object(true), flags.get("one"));
    ASSERT_EQ(Object(false), flags.get("two"));
    ASSERT_EQ(Object(7), flags.get("three"));
    ASSERT_EQ(Object("I_AM_DEFAULT"), flags.get("four"));
}
