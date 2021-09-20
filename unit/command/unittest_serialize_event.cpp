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

#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "apl/component/componenteventsourcewrapper.h"
#include "apl/component/componenteventtargetwrapper.h"
#include "apl/engine/contextwrapper.h"

#include "../testeventloop.h"

using namespace apl;


inline void dump(const Object& object)
{
    rapidjson::Document doc;
    auto json = object.serialize(doc.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    json.Accept(writer);
    std::cout << buffer.GetString() << std::endl;
}

inline
::testing::AssertionResult
CompareValue(const rapidjson::Value& actual, const rapidjson::Value& expected)
{
    // Direct value-to-value comparison
    if (expected.IsNull())
        return actual.IsNull() ? ::testing::AssertionSuccess() : ::testing::AssertionFailure() << "expected a null";

    if (expected.IsBool()) {
        if (!actual.IsBool())
            return ::testing::AssertionFailure() << "Expected a boolean";

        return actual.GetBool() == expected.GetBool() ?
               ::testing::AssertionSuccess() :
               ::testing::AssertionFailure() << "booleans don't match " << expected.GetBool() << "!="
                                             << actual.GetBool();
    }

    if (expected.IsNumber()) {
        if (!actual.IsNumber())
            return ::testing::AssertionFailure() << "Expected a number";

        return actual.GetDouble() == expected.GetDouble() ?
               ::testing::AssertionSuccess() :
               ::testing::AssertionFailure() << "numbers don't match " << expected.GetDouble() << "!="
                                             << actual.GetDouble();
    }

    if (expected.IsString()) {
        // Special string to indicate we're only checking for existence
        if (std::strcmp(expected.GetString(), "[EXISTS]") == 0)
            return ::testing::AssertionSuccess();

        if (!actual.IsString())
            return ::testing::AssertionFailure() << "Expected a string";

        return std::strcmp(actual.GetString(), expected.GetString()) == 0 ?
               ::testing::AssertionSuccess() :
               ::testing::AssertionFailure() << "strings don't match '" << expected.GetString() << "' != '"
                                             << actual.GetString() << "'";
    }

    if (expected.IsArray()) {
        if (!actual.IsArray())
            return ::testing::AssertionFailure() << "Expected an array";

        if (actual.Size() != expected.Size())
            return ::testing::AssertionFailure() << "Array size mismatch";

        for (int i = 0 ; i < expected.Size() ; i++) {
            auto result = CompareValue(actual[i], expected[i]);
            if (!result)
                return result << " array index=" << i;
        }

        return ::testing::AssertionSuccess();
    }

    // For object comparison, we only compare the defined values in the "expected" object
    if (expected.IsObject()) {
        if (!actual.IsObject())
            return ::testing::AssertionFailure() << "Expected an object";

        for (const auto& m : expected.GetObject()) {
            if (!actual.HasMember(m.name))
                return ::testing::AssertionFailure() << "Expected to find field '" << m.name.GetString() << "' in object";

            auto result = CompareValue(actual[m.name], m.value);
            if (!result)
                return result << " on member '" << m.name.GetString() << "'";
        }

        for (const auto& m : actual.GetObject()) {
            if (!expected.HasMember(m.name))
                return ::testing::AssertionFailure() << "Unexpected field '" << m.name.GetString() << "'in object";
        }

        return ::testing::AssertionSuccess();
    }

    return ::testing::AssertionFailure() << "Unknown rapidjson type";
}

inline
::testing::AssertionResult
CompareValue(const Object& actual, const char * expected)
{
    rapidjson::Document doc;
    auto actualJSON = actual.serialize(doc.GetAllocator());

    rapidjson::ParseResult ok = doc.Parse(expected);
    if (!ok)
        return ::testing::AssertionFailure() << "Bad parse of expected JSON";

    return CompareValue(actualJSON, doc);
}

class PokeCommand : public CoreCommand {
public:
    static CommandPtr create(const ContextPtr& context,
                             Properties&& properties,
                             const CoreComponentPtr& base,
                             const std::string& parentSequencer)
    {
        auto ptr = std::make_shared<PokeCommand>(context, std::move(properties), base, parentSequencer);
        return ptr->validate() ? ptr : nullptr;
    }

    PokeCommand(const ContextPtr& context,
                Properties&& properties,
                const CoreComponentPtr& base,
                const std::string& parentSequencer)
            : CoreCommand(context, std::move(properties), base, parentSequencer)
    {}

    const CommandPropDefSet&
    propDefSet() const override {
        static CommandPropDefSet sPokeCommandProperties(CoreCommand::propDefSet(), {
                {kCommandPropertyComponentId, "", asString, kPropRequiredId},
                {kCommandPropertyProperty,    "", asString, kPropRequired},
                {kCommandPropertyValue,       "", asAny,    kPropRequired}
        });

        return sPokeCommandProperties;
    }

    ActionPtr
    execute(const TimersPtr& timers, bool fastMode) override {
        if (!calculateProperties())
            return nullptr;

        mProperty = mValues.at(kCommandPropertyProperty).asString();
        mValue = mValues.at(kCommandPropertyValue);

        return nullptr;
    }

    CommandType type() const override {
        return kCommandTypeCustomEvent;
    }

    std::string mProperty;
    Object mValue;
};

class SerializeEventTest : public DocumentWrapper {
public:
    SerializeEventTest() : DocumentWrapper() {
        // Register an extension command "Validate" that can be fired by all "onXXX" commands.
        // We'll pass up the event information in the "event" property and a distinguishing "name"
        config->registerExtensionCommand(
                ExtensionCommandDefinition("aplext:Event", "Validate")
                .allowFastMode(true)
                .property("event", Object::NULL_OBJECT(), true)
                .property("name", "", true));

        // Add a new internal command.  This won't do anything, but will stash what was set in the event
        CommandFactory::instance().set("Poke",
                [&](const ContextPtr& context,
                    Properties&& properties,
                    const CoreComponentPtr& base,
                    const std::string& parentSequencer) {
            auto cmd = std::static_pointer_cast<PokeCommand>(PokeCommand::create(context,
                                                                                 std::move(properties),
                                                                                 base,
                                                                                 parentSequencer));
            mPokeQueue.push(cmd);
            return cmd;
        });
    }

    void TearDown() override {
        ASSERT_TRUE(mPokeQueue.empty());
        DocumentWrapper::TearDown();
    }

    ::testing::AssertionResult
    CheckValidate(const char *name, const char *expectedJSON)
    {
        if (!root->hasEvent())
            return ::testing::AssertionFailure() << "No event on root";

        auto event = root->popEvent();
        if (event.getType() != kEventTypeExtension)
            return ::testing::AssertionFailure() << "The event is not an extension event";

        auto result = IsEqual("Validate", event.getValue(kEventPropertyName));
        if (!result)
            return result << "Event type was not Validate";

        auto ext = event.getValue(kEventPropertyExtension);
        result = IsEqual(name, ext.get("name"));
        if (!result)
            return result << "Event name mismatch";

        rapidjson::Document doc;
        rapidjson::ParseResult ok = doc.Parse(expectedJSON);
        if (!ok)
            return ::testing::AssertionFailure() << "Bad parse of expected JSON";

        auto json = ext.get("event").serialize(doc.GetAllocator());
        return CompareValue(json, doc);
    }

    ::testing::AssertionResult
    CheckSetValueEvent(const char *name, const char *expectedJSON) {
        if (mPokeQueue.empty())
            return ::testing::AssertionFailure() << "Missing SetValue event";

        auto poke = mPokeQueue.front();
        auto propertyName = poke->mProperty;
        if (propertyName != name) {
            mPokeQueue.pop();
            return ::testing::AssertionFailure()
                    << "Mismatched property name.  Expected='" << name << "' Actual='" << propertyName << "'";
        }

        rapidjson::Document doc;
        rapidjson::ParseResult ok = doc.Parse(expectedJSON);
        if (!ok) {
            mPokeQueue.pop();
            return ::testing::AssertionFailure() << "Bad parse of expected JSON";
        }

        auto json = poke->mValue.serialize(doc.GetAllocator());
        mPokeQueue.pop();
        return CompareValue(json, doc);
    }

protected:
    std::queue<std::shared_ptr<PokeCommand>> mPokeQueue;
};

static const char * BASE_DOCUMENT = R"(
    {
      "type": "APL",
      "version": "1.4",
      "extensions": {
        "name": "E",
        "uri": "aplext:Event"
      },
      "handleKeyDown": {
        "commands": {
          "type": "E:Validate",
          "event": "${event}",
          "name": "keydown"
        }
      },
      "handleKeyUp": {
        "commands": {
          "type": "E:Validate",
          "event": "${event}",
          "name": "keyup"
        }
      },
      "onMount": {
        "type": "E:Validate",
        "event": "${event}",
        "name": "docmount"
      },
      "mainTemplate": {
        "items": {
          "type": "TouchWrapper",
          "onMount": {
            "type": "E:Validate",
            "event": "${event}",
            "name": "touchmount"
          }
        }
      }
    }
)";

TEST_F(SerializeEventTest, BaseDocument) {
    loadDocument(BASE_DOCUMENT);
    ASSERT_TRUE(component);

    // The first event should be the TouchWrapper onMount
    ASSERT_TRUE(CheckValidate("touchmount", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "handler": "Mount",
            "height": 800.0,
            "id": "",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "TouchWrapper",
            "uid": "[EXISTS]",
            "width": 1024.0,
            "source": "TouchWrapper",
            "value": false
          }
        }
    )"));

    // The second event is the Document onMount
    ASSERT_TRUE(CheckValidate("docmount", R"(
        {
          "source": {
            "handler": "Mount",
            "id": null,
            "source": "Document",
            "type": "Document",
            "uid": null,
            "value": null
          }
        }
    )"));

    // Send a key down event
    root->handleKeyboard(kKeyDown, Keyboard("KeyB", "b"));
    ASSERT_TRUE(CheckValidate("keydown", R"(
        {
          "keyboard": {
            "altKey": false,
            "code": "KeyB",
            "ctrlKey": false,
            "key": "b",
            "metaKey": false,
            "repeat": false,
            "shiftKey": false
          },
          "source": {
            "handler": "KeyDown",
            "id": null,
            "source": "Document",
            "type": "Document",
            "uid": null,
            "value": null
          }
        }
    )"));

    // Send the key up event
    root->handleKeyboard(kKeyUp, Keyboard("KeyB", "b"));
    ASSERT_TRUE(CheckValidate("keyup", R"(
        {
          "keyboard": {
            "altKey": false,
            "code": "KeyB",
            "ctrlKey": false,
            "key": "b",
            "metaKey": false,
            "repeat": false,
            "shiftKey": false
          },
          "source": {
            "handler": "KeyUp",
            "id": null,
            "source": "Document",
            "type": "Document",
            "uid": null,
            "value": null
          }
        }
    )"));
}

static const char *TOUCH_DOCUMENT = R"(
    {
      "type": "APL",
      "version": "1.4",
      "extensions": {
        "name": "E",
        "uri": "aplext:Event"
      },
      "mainTemplate": {
        "items": {
          "type": "TouchWrapper",
          "onCursorEnter": {
            "type": "E:Validate",
            "event": "${event}",
            "name": "cursorenter"
          },
          "onCursorExit": {
            "type": "E:Validate",
            "event": "${event}",
            "name": "cursorexit"
          },
          "onFocus": {
            "type": "E:Validate",
            "event": "${event}",
            "name": "focus"
          },
          "onBlur": {
            "type": "E:Validate",
            "event": "${event}",
            "name": "blur"
          },
          "onPress": {
            "type": "E:Validate",
            "event": "${event}",
            "name": "press"
          },
          "handleKeyDown": {
            "commands": {
              "type": "E:Validate",
              "event": "${event}",
              "name": "keydown"
            }
          },
          "handleKeyUp": {
            "commands": {
              "type": "E:Validate",
              "event": "${event}",
              "name": "keyup"
            }
          }
        }
      }
    }
)";

TEST_F(SerializeEventTest, TouchDocument) {
    loadDocument(TOUCH_DOCUMENT);
    ASSERT_TRUE(component);

    // Cursor enter
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(10, 10)));
    ASSERT_TRUE(CheckValidate("cursorenter", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 800.0,
            "id": "",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "TouchWrapper",
            "width": 1024.0,
            "source": "TouchWrapper",
            "value": false,
            "handler": "CursorEnter",
            "uid": "[EXISTS]"
          }
        }
    )"));

    // Cursor exit
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(-1, -1)));
    ASSERT_TRUE(CheckValidate("cursorexit", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 800.0,
            "id": "",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "TouchWrapper",
            "width": 1024.0,
            "source": "TouchWrapper",
            "value": false,
            "handler": "CursorExit",
            "uid": "[EXISTS]"
          }
        }
    )"));


    // Send a key down event to verify that the TouchWrapper doesn't have focus
    root->handleKeyboard(kKeyDown, Keyboard("KeyB", "b"));
    ASSERT_FALSE(root->hasEvent());

    // Give the TouchWrapper focus
    component->update(kUpdateTakeFocus, 1);
    ASSERT_TRUE(CheckValidate("focus", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": true,
            "height": 800.0,
            "id": "",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "TouchWrapper",
            "width": 1024.0,
            "source": "TouchWrapper",
            "value": false,
            "handler": "Focus",
            "uid": "[EXISTS]"
          }
        }
    )"));

    // Send the key down event
    root->handleKeyboard(kKeyDown, Keyboard("KeyB", "b"));
    ASSERT_TRUE(CheckValidate("keydown", R"(
        {
          "keyboard": {
            "altKey": false,
            "code": "KeyB",
            "ctrlKey": false,
            "key": "b",
            "metaKey": false,
            "repeat": false,
            "shiftKey": false
          },
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": true,
            "height": 800.0,
            "id": "",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "TouchWrapper",
            "width": 1024.0,
            "source": "TouchWrapper",
            "value": false,
            "handler": "KeyDown",
            "uid": "[EXISTS]"
          }
        }
    )"));

    // Send the key up event
    root->handleKeyboard(kKeyUp, Keyboard("KeyB", "b"));
    ASSERT_TRUE(CheckValidate("keyup", R"(
        {
          "keyboard": {
            "altKey": false,
            "code": "KeyB",
            "ctrlKey": false,
            "key": "b",
            "metaKey": false,
            "repeat": false,
            "shiftKey": false
          },
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": true,
            "height": 800.0,
            "id": "",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "TouchWrapper",
            "width": 1024.0,
            "source": "TouchWrapper",
            "value": false,
            "handler": "KeyUp",
            "uid": "[EXISTS]"
          }
        }
    )"));

    // Remove focus
    component->update(kUpdateTakeFocus, 0);
    ASSERT_TRUE(CheckValidate("blur", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 800.0,
            "id": "",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "TouchWrapper",
            "width": 1024.0,
            "source": "TouchWrapper",
            "value": false,
            "handler": "Blur",
            "uid": "[EXISTS]"
          }
        }
    )"));

    // On press
    component->update(kUpdatePressed, 1);
    ASSERT_TRUE(CheckValidate("press", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 800.0,
            "id": "",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "TouchWrapper",
            "width": 1024.0,
            "source": "TouchWrapper",
            "value": false,
            "handler": "Press",
            "uid": "[EXISTS]"
          }
        }
    )"));
}

static const char *PAGER_DOCUMENT = R"(
    {
      "type": "APL",
      "version": "1.4",
      "extensions": {
        "name": "E",
        "uri": "aplext:Event"
      },
      "mainTemplate": {
        "items": {
          "type": "Pager",
          "onPageChanged": {
            "type": "E:Validate",
            "event": "${event}",
            "name": "pageit"
          },
          "items": {
            "type": "Text",
            "text": "${data}"
          },
          "data": [
            1,
            2,
            3
          ]
        }
      }
    }
)";


TEST_F(SerializeEventTest, PagerDocument) {
    loadDocument(PAGER_DOCUMENT);
    ASSERT_TRUE(component);

    // Go to the next page
    component->update(kUpdatePagerPosition, 2);

    ASSERT_TRUE(CheckValidate("pageit", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 100.0,
            "id": "",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "page": 2.0,
            "pressed": false,
            "type": "Pager",
            "width": 100.0,
            "source": "Pager",
            "value": 2,
            "handler": "Page",
            "uid": "[EXISTS]"
          }
        }
    )"));
}

static const char *SCROLL_VIEW_DOCUMENT = R"(
    {
      "type": "APL",
      "version": "1.4",
      "extensions": {
        "name": "E",
        "uri": "aplext:Event"
      },
      "mainTemplate": {
        "items": {
          "type": "ScrollView",
          "height": 1000,
          "width": 100,
          "onScroll": {
            "type": "E:Validate",
            "event": "${event}",
            "name": "scrollit"
          },
          "items": {
            "type": "Frame",
            "height": 2000,
            "width": 100
          }
        }
      }
    }
)";

TEST_F(SerializeEventTest, ScrollViewDocument) {
    loadDocument(SCROLL_VIEW_DOCUMENT);
    ASSERT_TRUE(component);

    component->update(kUpdateScrollPosition, 500);
    ASSERT_TRUE(CheckValidate("scrollit", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 1000.0,
            "id": "",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "ScrollView",
            "width": 100.0,
            "source": "ScrollView",
            "value": 0.5,
            "handler": "Scroll",
            "uid": "[EXISTS]",
            "position": 0.5
          }
        }
    )"));
}

static const char* GRIDSEQ_SCROLLING_EVENT_DOC = R"({
  "type":"APL",
  "version":"1.4",
  "extensions":{
    "name":"E",
    "uri":"aplext:Event"
  },
  "mainTemplate":{
    "parameters":[

    ],
    "item":{
      "type":"GridSequence",
      "scrollDirection":"vertical",
      "onScroll":{
        "type":"E:Validate",
        "event":"${event}",
        "name":"gridScroll"
      },
      "width":60,
      "height":80,
      "childWidth":"100%",
      "childHeight":"20dp",
      "items":{
        "type":"Text",
        "text": "${data}"
      },
      "data":[ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 ]
    }
  }
})";

TEST_F(SerializeEventTest, GridSequenceScrollEvent) {
    loadDocument(GRIDSEQ_SCROLLING_EVENT_DOC);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(kScrollDirectionVertical, component->getCalculated(kPropertyScrollDirection).asInt());

    // scroll to 30
    component->update(kUpdateScrollPosition, 30);
    ASSERT_TRUE(CheckValidate("gridScroll", R"(
        {
          "source":{
            "bind":{},
            "checked":false,
            "disabled":false,
            "focused":false,
            "height":80.0,
            "id":"",
            "opacity":1.0,
            "pressed":false,
            "type":"GridSequence",
            "width":60.0,
            "source":"GridSequence",
            "value":0.375,
            "handler":"Scroll",
            "layoutDirection": "LTR",
            "uid":"[EXISTS]",
            "position":0.375,
            "itemsPerCourse":1,
            "firstVisibleChild": 1,
            "firstFullyVisibleChild": 2,
            "lastFullyVisibleChild": 4,
            "lastVisibleChild": 5
          }
        }
    )"));

    // scroll to 120
    component->update(kUpdateScrollPosition, 120);
    ASSERT_TRUE(CheckValidate("gridScroll", R"(
        {
          "source":{
            "bind":{},
            "checked":false,
            "disabled":false,
            "focused":false,
            "height":80.0,
            "id":"",
            "layoutDirection": "LTR",
            "opacity":1,
            "pressed":false,
            "type":"GridSequence",
            "width":60.0,
            "source":"GridSequence",
            "value":1.5,
            "handler":"Scroll",
            "uid":"[EXISTS]",
            "position":1.5,
            "itemsPerCourse":1,
            "firstVisibleChild": 6,
            "firstFullyVisibleChild": 6,
            "lastFullyVisibleChild": 9,
            "lastVisibleChild": 9
          }
        }
    )"));

    // scroll to 230 will make lastChildBottom(240) - ViewHeight(80)
    component->update(kUpdateScrollPosition, 230);
    ASSERT_EQ(160, component->getCalculated(kPropertyScrollPosition).asInt());
    ASSERT_TRUE(CheckValidate("gridScroll", R"(
        {
          "source":{
            "bind":{},
            "checked":false,
            "disabled":false,
            "focused":false,
            "height":80.0,
            "id":"",
            "layoutDirection": "LTR",
            "opacity":1.0,
            "pressed":false,
            "type":"GridSequence",
            "width":60.0,
            "source":"GridSequence",
            "value":2,
            "handler":"Scroll",
            "uid":"[EXISTS]",
            "position":2,
            "itemsPerCourse":1,
            "firstVisibleChild": 8,
            "firstFullyVisibleChild": 8,
            "lastFullyVisibleChild": 11,
            "lastVisibleChild": 11
          }
        }
    )"));
}

static const char* GRIDSEQ_OPACITY_UPDATE_EVENT_DOC = R"(
{
  "type":"APL",
  "version":"1.5",
  "extensions":{
    "name":"E",
    "uri":"aplext:Event"
  },
  "mainTemplate":{
    "parameters":[ ],
    "items":[
      {
        "type":"GridSequence",
        "id":"MyGrid",
        "scrollDirection":"vertical",
        "width":60,
        "height":80,
        "opacity":0,
        "childWidth":"100%",
        "childHeight":"20dp",
        "items":{
          "type":"Frame",
          "backgroundColor":"${data}"
        },
        "onMount":[
          {
            "type":"AnimateItem",
            "duration":1000,
            "value":[
              {
                "property":"opacity",
                "from":0,
                "to":1
              }
            ]
          }
        ],
        "onFocus":{
          "type":"E:Validate",
          "event":"${event}",
          "name":"GridFocus"
        },
        "data":[ "red", "blue", "green", "yellow", "gray", "orange", "white", "purple",
          "magenta", "cyan" ]
      }
    ]
  }
}
)";

TEST_F(SerializeEventTest, GridSequenceOpacityUpEvent) {
    loadDocument(GRIDSEQ_OPACITY_UPDATE_EVENT_DOC);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(kScrollDirectionVertical, component->getCalculated(kPropertyScrollDirection).asInt());

    // Give the GridSequence focus
    component->update(kUpdateTakeFocus, 1);
    ASSERT_TRUE(CheckValidate("GridFocus", R"(
        {
          "source":{
            "bind":{},
            "checked":false,
            "disabled":false,
            "focused":true,
            "height":80.0,
            "id":"MyGrid",
            "opacity":0.0,
            "pressed":false,
            "type":"GridSequence",
            "width":60.0,
            "source":"GridSequence",
            "value":0.0,
            "handler":"Focus",
            "layoutDirection": "LTR",
            "uid":"[EXISTS]",
            "position":0.0,
            "itemsPerCourse":1,
            "firstVisibleChild": -1,
            "firstFullyVisibleChild": -1,
            "lastFullyVisibleChild": -1,
            "lastVisibleChild": -1
          }
        }
    )"));

    // Update time and give the GridSequence focus
    component->update(kUpdateTakeFocus, 0);
    advanceTime(1000);
    component->update(kUpdateTakeFocus, 1);
    ASSERT_TRUE(CheckValidate("GridFocus", R"(
        {
          "source":{
            "bind":{},
            "checked":false,
            "disabled":false,
            "focused":true,
            "height":80.0,
            "id":"MyGrid",
            "layoutDirection": "LTR",
            "opacity":1.0,
            "pressed":false,
            "type":"GridSequence",
            "width":60.0,
            "source":"GridSequence",
            "value":0.0,
            "handler":"Focus",
            "uid":"[EXISTS]",
            "position":0.0,
            "itemsPerCourse":1,
            "firstVisibleChild": 0,
            "firstFullyVisibleChild": 0,
            "lastFullyVisibleChild": 3,
            "lastVisibleChild": 3
          }
        }
    )"));
}

static const char* GRIDSEQ_ZERO_OPACITY_DOC = R"({
  "type":"APL",
  "version":"1.4",
  "extensions":{
    "name":"E",
    "uri":"aplext:Event"
  },
  "mainTemplate":{
    "parameters":[

    ],
    "item":{
      "type":"GridSequence",
      "scrollDirection":"vertical",
      "onScroll":{
        "type":"E:Validate",
        "event":"${event}",
        "name":"gridScroll"
      },
      "width":60,
      "height":80,
      "opacity": 0,
      "childWidth":"100%",
      "childHeight":"20dp",
      "items":{
        "type":"Text",
        "text": "${data}"
      },
      "data":[ 1, 2, 3, 4, 5, 6, 7, 8 ]
    }
  }
})";

TEST_F(SerializeEventTest, GridSequenceZeroOpacityScrollEvent) {
    loadDocument(GRIDSEQ_ZERO_OPACITY_DOC);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(kScrollDirectionVertical, component->getCalculated(kPropertyScrollDirection).asInt());

    // scroll to 30
    component->update(kUpdateScrollPosition, 30);
    ASSERT_TRUE(CheckValidate("gridScroll", R"(
        {
          "source":{
            "bind":{},
            "checked":false,
            "disabled":false,
            "focused":false,
            "height":80.0,
            "id":"",
            "layoutDirection": "LTR",
            "opacity":0.0,
            "pressed":false,
            "type":"GridSequence",
            "width":60.0,
            "source":"GridSequence",
            "value":0.375,
            "handler":"Scroll",
            "uid":"[EXISTS]",
            "position":0.375,
            "itemsPerCourse":1,
            "firstVisibleChild": -1,
            "firstFullyVisibleChild": -1,
            "lastFullyVisibleChild": -1,
            "lastVisibleChild": -1
          }
        }
    )"));
}

static const char* GRIDSEQ_MULTI_CHILD_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "extensions":{
    "name":"E",
    "uri":"aplext:Event"
  },
  "mainTemplate": {
    "parameters": [],
    "item": {
      "type": "GridSequence",
      "scrollDirection": "vertical",
      "onScroll":{
        "type":"E:Validate",
        "event":"${event}",
        "name":"gridScroll"
      },
      "width": 60,
      "height": 40,
      "childWidth": "15dp",
      "childHeight": "20dp",
      "items": {
        "type": "Frame",
        "backgroundColor": "${data}"
      },
      "data": [ "red", "blue", "green", "yellow", "gray", "orange", "white", "purple", "magenta", "cyan"  ]
    }
  }
})";

TEST_F(SerializeEventTest, GridSequenceMultiChildEvent) {
    loadDocument(GRIDSEQ_MULTI_CHILD_DOC);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeGridSequence, component->getType());
    ASSERT_EQ(kScrollDirectionVertical, component->getCalculated(kPropertyScrollDirection).asInt());

    // scroll to 10
    component->update(kUpdateScrollPosition, 10);
    ASSERT_TRUE(CheckValidate("gridScroll", R"(
        {
          "source":{
            "bind":{},
            "checked":false,
            "disabled":false,
            "focused":false,
            "height":40.0,
            "id":"",
            "layoutDirection": "LTR",
            "opacity":1.0,
            "pressed":false,
            "type":"GridSequence",
            "width":60.0,
            "source":"GridSequence",
            "value":0.25,
            "handler":"Scroll",
            "uid":"[EXISTS]",
            "position":0.25,
            "itemsPerCourse":4,
            "firstVisibleChild": 0,
            "firstFullyVisibleChild": 4,
            "lastFullyVisibleChild": 7,
            "lastVisibleChild": 9
          }
        }
    )"));
}

static const char *SEQUENCE_DOCUMENT = R"(
    {
      "type": "APL",
      "version": "1.4",
      "extensions": {
        "name": "E",
        "uri": "aplext:Event"
      },
      "mainTemplate": {
        "items": {
          "type": "Sequence",
          "height": 1000,
          "width": 100,
          "onScroll": {
            "type": "E:Validate",
            "event": "${event}",
            "name": "scrolled"
          },
          "items": {
            "type": "Frame",
            "height": 500,
            "width": 100
          },
          "data": [ 1, 2, 3, 4 ]
        }
      }
    }
)";

TEST_F(SerializeEventTest, SequenceDocument) {
    loadDocument(SEQUENCE_DOCUMENT);
    ASSERT_TRUE(component);

    component->update(kUpdateScrollPosition, 500);
    // viewPort is 1024x800
    ASSERT_TRUE(CheckValidate("scrolled", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 1000.0,
            "id": "",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "Sequence",
            "width": 100.0,
            "source": "Sequence",
            "value": 0.5,
            "handler": "Scroll",
            "uid": "[EXISTS]",
            "position": 0.5,
            "firstVisibleChild": 1,
            "firstFullyVisibleChild": 1,
            "lastFullyVisibleChild": 1,
            "lastVisibleChild": 2
          }
        }
    )"));
}

static const char *VIDEO_DOCUMENT = R"(
    {
      "type": "APL",
      "version": "1.4",
      "extensions": {
        "name": "E",
        "uri": "aplext:Event"
      },
      "mainTemplate": {
        "items": {
          "type": "Video",
          "source": [
            "Video1",
            "Video2",
            "Video3"
          ],
          "width": 720,
          "height": 480,
          "onEnd": {
            "type": "E:Validate",
            "event": "${event}",
            "name": "endit"
          },
          "onPause": {
            "type": "E:Validate",
            "event": "${event}",
            "name": "pauseit"
          },
          "onPlay": {
            "type": "E:Validate",
            "event": "${event}",
            "name": "playit"
          },
          "onTrackReady": {
            "type": "E:Validate",
            "event": "${event}",
            "name": "readyit"
          },
          "onTimeUpdate": {
            "type": "E:Validate",
            "event": "${event}",
            "name": "timeit"
          },
          "onTrackUpdate": {
            "type": "E:Validate",
            "event": "${event}",
            "name": "trackit"
          },
          "onTrackFail": {
            "type": "E:Validate",
            "event": "${event}",
            "name": "failit"
          }
        }
      }
    }
)";

TEST_F(SerializeEventTest, VideoDocument) {
    loadDocument(VIDEO_DOCUMENT);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->hasEvent());

    // Start playing
    auto state = MediaState(0, 3, 100, 1000, false, false)
                     .withTrackState(kTrackReady);  // Track 0 of 3, @100 ms of 1000 ms, not paused/ended, ready
    component->updateMediaState(state);

    // The first event we should get is "Ready"
    ASSERT_TRUE(CheckValidate("readyit", R"(
            {
              "source": {
                "bind": {},
                "checked": false,
                "currentTime": 100.0,
                "disabled": false,
                "duration": 1000.0,
                "ended": false,
                "focused": false,
                "height": 480.0,
                "id": "",
                "layoutDirection": "LTR",
                "opacity": 1.0,
                "paused": false,
                "pressed": false,
                "source": "Video1",
                "trackIndex": 0.0,
                "trackCount": 3.0,
                "trackState": "ready",
                "type": "Video",
                "uid": "[EXISTS]",
                "url": "Video1",
                "width": 720.0,
                "value": null,
                "handler": "TrackReady"
              },
              "trackIndex": 0.0,
              "trackState": "ready"
            }
        )"));

    // The next event we should get is "Play"
    ASSERT_TRUE(CheckValidate("playit", R"(
        {
          "currentTime": 100.0,
          "duration": 1000.0,
          "ended": false,
          "paused": false,
          "source": {
            "bind": {},
            "checked": false,
            "currentTime": 100.0,
            "disabled": false,
            "duration": 1000.0,
            "ended": false,
            "focused": false,
            "height": 480.0,
            "id": "",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "paused": false,
            "pressed": false,
            "source": "Video1",
            "trackIndex": 0.0,
            "trackCount": 3.0,
            "trackState": "ready",
            "type": "Video",
            "uid": "[EXISTS]",
            "url": "Video1",
            "width": 720.0,
            "value": null,
            "handler": "Play"
          },
          "trackCount": 3.0,
          "trackIndex": 0.0,
          "trackState": "ready"
        }
    )"));

    // We should also receive a "TimeUpdate" event since we've moved time forwards
    ASSERT_TRUE(CheckValidate("timeit", R"(
        {
          "currentTime": 100.0,
          "duration": 1000.0,
          "ended": false,
          "paused": false,
          "source": {
            "bind": {},
            "checked": false,
            "currentTime": 100.0,
            "disabled": false,
            "duration": 1000.0,
            "ended": false,
            "focused": false,
            "height": 480.0,
            "layoutDirection": "LTR",
            "id": "",
            "opacity": 1.0,
            "paused": false,
            "pressed": false,
            "source": "Video1",
            "trackIndex": 0.0,
            "trackCount": 3.0,
            "trackState": "ready",
            "type": "Video",
            "uid": "[EXISTS]",
            "url": "Video1",
            "width": 720.0,
            "value": 100.0,
            "handler": "TimeUpdate"
          },
          "trackCount": 3.0,
          "trackIndex": 0.0,
          "trackState": "ready"
        }
    )"));

    ASSERT_FALSE(root->hasEvent());

    // Move forward 100 milliseconds
    state = MediaState(0, 3, 200, 1000, false, false)
                .withTrackState(kTrackReady);  // Track 0 of 3, @200 ms of 1000 ms, not paused/ended and ready
    component->updateMediaState(state);

    ASSERT_TRUE(CheckValidate("timeit", R"(
        {
          "currentTime": 200.0,
          "duration": 1000.0,
          "ended": false,
          "paused": false,
          "source": {
            "bind": {},
            "checked": false,
            "currentTime": 200.0,
            "disabled": false,
            "duration": 1000.0,
            "ended": false,
            "focused": false,
            "height": 480.0,
            "id": "",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "paused": false,
            "pressed": false,
            "source": "Video1",
            "trackCount": 3.0,
            "trackIndex": 0.0,
            "trackState": "ready",
            "type": "Video",
            "uid": "[EXISTS]",
            "url": "Video1",
            "width": 720.0,
            "value": 200.0,
            "handler": "TimeUpdate"
          },
          "trackCount": 3.0,
          "trackIndex": 0.0,
          "trackState": "ready"
        }
    )"));

    // Jump to the next track
    state = MediaState(1, 3, 0, 1000, false, false)
                .withTrackState(kTrackNotReady);  // Track 1 of 3, @0 ms of 1000 ms, not paused/ended, not ready
    component->updateMediaState(state);

    // The onTrackUpdate fires - note that value=1.0 (the new track)
    ASSERT_TRUE(CheckValidate("trackit", R"(
        {
          "currentTime": 0.0,
          "duration": 1000.0,
          "ended": false,
          "paused": false,
          "source": {
            "bind": {},
            "checked": false,
            "currentTime": 0.0,
            "disabled": false,
            "duration": 1000.0,
            "ended": false,
            "focused": false,
            "height": 480.0,
            "id": "",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "paused": false,
            "pressed": false,
            "source": "Video2",
            "trackCount": 3.0,
            "trackIndex": 1.0,
            "trackState": "notReady",
            "type": "Video",
            "uid": "[EXISTS]",
            "url": "Video2",
            "width": 720.0,
            "value": 1.0,
            "handler": "TrackUpdate"
          },
          "trackCount": 3.0,
          "trackIndex": 1.0,
          "trackState": "notReady"
        }
    )"));

    // The onTimeUpdate fires - note that value=0.0
    ASSERT_TRUE(CheckValidate("timeit", R"(
        {
          "currentTime": 0.0,
          "duration": 1000.0,
          "ended": false,
          "paused": false,
          "source": {
            "bind": {},
            "checked": false,
            "currentTime": 0.0,
            "disabled": false,
            "duration": 1000.0,
            "ended": false,
            "focused": false,
            "height": 480.0,
            "id": "",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "paused": false,
            "pressed": false,
            "source": "Video2",
            "trackCount": 3.0,
            "trackIndex": 1.0,
            "trackState": "notReady",
            "type": "Video",
            "uid": "[EXISTS]",
            "url": "Video2",
            "width": 720.0,
            "value": 0.0,
            "handler": "TimeUpdate"
          },
          "trackCount": 3.0,
          "trackIndex": 1.0,
          "trackState": "notReady"
        }
    )"));

    // Pause the video playback
    state = MediaState(1, 3, 0, 1000, true, false);  // Track 1 of 3, @0 ms of 1000 ms, paused/not ended, not ready
    component->updateMediaState(state);

    ASSERT_TRUE(CheckValidate("pauseit", R"(
        {
          "currentTime": 0.0,
          "duration": 1000.0,
          "ended": false,
          "paused": true,
          "source": {
            "bind": {},
            "checked": false,
            "currentTime": 0.0,
            "disabled": false,
            "duration": 1000.0,
            "ended": false,
            "focused": false,
            "height": 480.0,
            "id": "",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "paused": true,
            "pressed": false,
            "source": "Video2",
            "trackCount": 3.0,
            "trackIndex": 1.0,
            "trackState": "notReady",
            "type": "Video",
            "uid": "[EXISTS]",
            "url": "Video2",
            "width": 720.0,
            "value": null,
            "handler": "Pause"
          },
          "trackCount": 3.0,
          "trackIndex": 1.0,
          "trackState": "notReady"
        }
    )"));

    // Track gets ready at paused state
    state = MediaState(1, 3, 0, 1000, true, false)
                .withTrackState(kTrackReady);  // Track 1 of 3, @0 ms of 1000 ms, paused/not ended, ready
    component->updateMediaState(state);

    ASSERT_TRUE(CheckValidate("readyit", R"(
            {
              "source": {
                "bind": {},
                "checked": false,
                "currentTime": 0.0,
                "disabled": false,
                "duration": 1000.0,
                "ended": false,
                "focused": false,
                "height": 480.0,
                "id": "",
                "layoutDirection": "LTR",
                "opacity": 1.0,
                "paused": true,
                "pressed": false,
                "source": "Video2",
                "trackCount": 3.0,
                "trackIndex": 1.0,
                "trackState": "ready",
                "type": "Video",
                "uid": "[EXISTS]",
                "url": "Video2",
                "width": 720.0,
                "value": null,
                "handler": "TrackReady"
              },
              "trackIndex": 1.0,
              "trackState": "ready"
            }
        )"));

    // Error occurred while playing track
    state = MediaState(1, 3, 500, 1000, false, false)
                .withTrackState(kTrackFailed)
                .withErrorCode(99);  // Track 1 of 3, @500 ms of 1000 ms, not paused/not ended and not ready
    component->updateMediaState(state);

    ASSERT_TRUE(CheckValidate("failit", R"(
            {
              "currentTime": 500.0,
              "errorCode": 99,
              "source": {
                "bind": {},
                "checked": false,
                "currentTime": 500.0,
                "disabled": false,
                "duration": 1000.0,
                "ended": false,
                "focused": false,
                "height": 480.0,
                "id": "",
                "layoutDirection": "LTR",
                "opacity": 1.0,
                "paused": false,
                "pressed": false,
                "source": "Video2",
                "trackCount": 3.0,
                "trackIndex": 1.0,
                "trackState": "failed",
                "type": "Video",
                "uid": "[EXISTS]",
                "url": "Video2",
                "width": 720.0,
                "value": null,
                "handler": "TrackFail"
              },
              "trackIndex": 1.0,
              "trackState": "failed"
            }
        )"));

    // End the video playback
    state = MediaState(1,3,500,1000,false,true);
    component->updateMediaState(state);

    ASSERT_TRUE(CheckValidate("endit", R"(
        {
          "currentTime": 500.0,
          "duration": 1000.0,
          "ended": true,
          "paused": false,
          "source": {
            "bind": {},
            "checked": false,
            "currentTime": 500.0,
            "disabled": false,
            "duration": 1000.0,
            "ended": true,
            "focused": false,
            "height": 480.0,
            "id": "",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "paused": false,
            "pressed": false,
            "source": "Video2",
            "trackCount": 3.0,
            "trackIndex": 1.0,
            "trackState": "notReady",
            "type": "Video",
            "uid": "[EXISTS]",
            "url": "Video2",
            "width": 720.0,
            "value": null,
            "handler": "End"
          },
          "trackCount": 3.0,
          "trackIndex": 1.0,
          "trackState": "notReady"
        }
    )"));
}


static const char * TARGET_CONTAINER = R"(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "items": [
            {
              "type": "Container",
              "id": "MyTarget",
              "width": 100,
              "height": 100
            },
            {
              "type": "TouchWrapper",
              "id": "MyButton",
              "height": 20,
              "width": 30,
              "onPress": {
                "type": "Poke",
                "componentId": "MyTarget",
                "property": "container",
                "value": "${event}"
              }
            }
          ]
        }
      }
    }
)";

TEST_F(SerializeEventTest, TargetContainer) {
    loadDocument(TARGET_CONTAINER);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->hasEvent());

    auto touchWrapper = component->findComponentById("MyButton");
    ASSERT_TRUE(touchWrapper);

    touchWrapper->update(kUpdatePressed, 1);
    ASSERT_TRUE(mPokeQueue.front());

    ASSERT_TRUE(CheckSetValueEvent("container", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 20.0,
            "layoutDirection": "LTR",
            "id": "MyButton",
            "opacity": 1.0,
            "pressed": false,
            "type": "TouchWrapper",
            "uid": "[EXISTS]",
            "width": 30.0,
            "source": "TouchWrapper",
            "value": false,
            "handler": "Press"
          },
          "target": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 100.0,
            "id": "MyTarget",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "Container",
            "uid": "[EXISTS]",
            "width": 100.0
          }
        }
    )"));
}

static const char * TARGET_FRAME = R"(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "items": [
            {
              "type": "Frame",
              "id": "MyTarget",
              "width": 100,
              "height": 100
            },
            {
              "type": "TouchWrapper",
              "id": "MyButton",
              "height": 20,
              "width": 30,
              "onPress": {
                "type": "Poke",
                "componentId": "MyTarget",
                "property": "frame",
                "value": "${event}"
              }
            }
          ]
        }
      }
    }
)";

TEST_F(SerializeEventTest, TargetFrame) {
    loadDocument(TARGET_FRAME);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->hasEvent());

    auto touchWrapper = component->findComponentById("MyButton");
    ASSERT_TRUE(touchWrapper);

    touchWrapper->update(kUpdatePressed, 1);
    ASSERT_TRUE(mPokeQueue.front());

    ASSERT_TRUE(CheckSetValueEvent("frame", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 20.0,
            "id": "MyButton",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "TouchWrapper",
            "uid": "[EXISTS]",
            "width": 30.0,
            "source": "TouchWrapper",
            "value": false,
            "handler": "Press"
          },
          "target": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 100.0,
            "id": "MyTarget",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "Frame",
            "uid": "[EXISTS]",
            "width": 100.0
          }
        }
    )"));
}


static const char * TARGET_IMAGE = R"(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "items": [
            {
              "type": "Image",
              "id": "MyTarget",
              "width": 100,
              "height": 100,
              "source": "ImageURL"
            },
            {
              "type": "TouchWrapper",
              "id": "MyButton",
              "height": 20,
              "width": 30,
              "onPress": {
                "type": "Poke",
                "componentId": "MyTarget",
                "property": "image",
                "value": "${event}"
              }
            }
          ]
        }
      }
    }
)";

TEST_F(SerializeEventTest, TargetImage) {
    loadDocument(TARGET_IMAGE);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->hasEvent());

    auto touchWrapper = component->findComponentById("MyButton");
    ASSERT_TRUE(touchWrapper);

    touchWrapper->update(kUpdatePressed, 1);
    ASSERT_TRUE(mPokeQueue.front());

    ASSERT_TRUE(CheckSetValueEvent("image", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 20.0,
            "id": "MyButton",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "TouchWrapper",
            "uid": "[EXISTS]",
            "width": 30.0,
            "source": "TouchWrapper",
            "value": false,
            "handler": "Press"
          },
          "target": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 100.0,
            "id": "MyTarget",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "Image",
            "uid": "[EXISTS]",
            "source": "ImageURL",
            "url": "ImageURL",
            "width": 100.0
          }
        }
    )"));
}


static const char * TARGET_PAGER = R"(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "items": [
            {
              "type": "Pager",
              "id": "MyTarget",
              "width": 100,
              "height": 100,
              "items": {
                "type": "Text"
              },
              "data": [ 1, 2, 3 ],
              "initialPage": 2
            },
            {
              "type": "TouchWrapper",
              "id": "MyButton",
              "height": 20,
              "width": 30,
              "onPress": {
                "type": "Poke",
                "componentId": "MyTarget",
                "property": "pager",
                "value": "${event}"
              }
            }
          ]
        }
      }
    }
)";

TEST_F(SerializeEventTest, TargetPager) {
    loadDocument(TARGET_PAGER);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->hasEvent());

    auto touchWrapper = component->findComponentById("MyButton");
    ASSERT_TRUE(touchWrapper);

    touchWrapper->update(kUpdatePressed, 1);
    ASSERT_TRUE(mPokeQueue.front());

    ASSERT_TRUE(CheckSetValueEvent("pager", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 20.0,
            "id": "MyButton",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "TouchWrapper",
            "uid": "[EXISTS]",
            "width": 30.0,
            "source": "TouchWrapper",
            "value": false,
            "handler": "Press"
          },
          "target": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 100.0,
            "id": "MyTarget",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "page": 2,
            "pressed": false,
            "type": "Pager",
            "uid": "[EXISTS]",
            "width": 100.0
          }
        }
    )"));
}


static const char * TARGET_SCROLL_VIEW = R"(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "items": [
            {
              "type": "ScrollView",
              "id": "MyTarget",
              "width": 100,
              "height": 100,
              "items": {
                "type": "Text",
                "height": 300,
                "width": 100
              }
            },
            {
              "type": "TouchWrapper",
              "id": "MyButton",
              "height": 20,
              "width": 30,
              "onPress": {
                "type": "Poke",
                "componentId": "MyTarget",
                "property": "scrollview",
                "value": "${event}"
              }
            }
          ]
        }
      }
    }
)";

TEST_F(SerializeEventTest, TargetScrollView) {
    loadDocument(TARGET_SCROLL_VIEW);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->hasEvent());

    auto touchWrapper = component->findComponentById("MyButton");
    ASSERT_TRUE(touchWrapper);

    auto scrollView = component->findComponentById("MyTarget");
    ASSERT_TRUE(scrollView);
    scrollView->update(kUpdateScrollPosition, 100);  // Should be position 1.0 (height = 100, scrolled by 100)

    touchWrapper->update(kUpdatePressed, 1);
    ASSERT_TRUE(mPokeQueue.front());

    ASSERT_TRUE(CheckSetValueEvent("scrollview", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 20.0,
            "id": "MyButton",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "TouchWrapper",
            "uid": "[EXISTS]",
            "width": 30.0,
            "source": "TouchWrapper",
            "value": false,
            "handler": "Press"
          },
          "target": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 100.0,
            "id": "MyTarget",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "position": 1.0,
            "pressed": false,
            "type": "ScrollView",
            "uid": "[EXISTS]",
            "width": 100.0
          }
        }
    )"));
}


static const char * TARGET_SEQUENCE = R"(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "items": [
            {
              "type": "Sequence",
              "id": "MyTarget",
              "width": 100,
              "height": 100,
              "items": {
                "type": "Text",
                "height": 100,
                "width": 100
              },
              "data": [1,2,3,4]
            },
            {
              "type": "TouchWrapper",
              "id": "MyButton",
              "height": 20,
              "width": 30,
              "onPress": {
                "type": "Poke",
                "componentId": "MyTarget",
                "property": "sequence",
                "value": "${event}"
              }
            }
          ]
        }
      }
    }
)";

TEST_F(SerializeEventTest, TargetSequence) {
    loadDocument(TARGET_SEQUENCE);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->hasEvent());

    auto touchWrapper = component->findComponentById("MyButton");
    ASSERT_TRUE(touchWrapper);

    auto sequence = component->findComponentById("MyTarget");
    ASSERT_TRUE(sequence);

    // Update the scroll position.  Because we limit scrolling to the "laid-out" range, we have to call
    // this repeatedly until the desired value is reached.
    double targetPosition = 250;
    while (sequence->getCalculated(kPropertyScrollPosition).asNumber() != targetPosition)
        sequence->update(kUpdateScrollPosition, targetPosition);

    touchWrapper->update(kUpdatePressed, 1);
    ASSERT_TRUE(mPokeQueue.front());

    ASSERT_TRUE(CheckSetValueEvent("sequence", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 20.0,
            "id": "MyButton",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "TouchWrapper",
            "uid": "[EXISTS]",
            "width": 30.0,
            "source": "TouchWrapper",
            "value": false,
            "handler": "Press"
          },
          "target": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 100.0,
            "id": "MyTarget",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "position": 2.5,
            "pressed": false,
            "type": "Sequence",
            "uid": "[EXISTS]",
            "width": 100.0,
            "firstVisibleChild": 2,
            "firstFullyVisibleChild": -1,
            "lastFullyVisibleChild": -1,
            "lastVisibleChild": 3
          }
        }
    )"));
}


static const char * TARGET_TEXT = R"(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "items": [
            {
              "type": "Text",
              "id": "MyTarget",
              "width": 100,
              "height": 100,
              "text": "My <b>text</b> is bold",
              "color": "blue"
            },
            {
              "type": "TouchWrapper",
              "id": "MyButton",
              "height": 20,
              "width": 30,
              "onPress": {
                "type": "Poke",
                "componentId": "MyTarget",
                "property": "muchtext",
                "value": "${event}"
              }
            }
          ]
        }
      }
    }
)";

TEST_F(SerializeEventTest, TargetText) {
    loadDocument(TARGET_TEXT);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->hasEvent());

    auto touchWrapper = component->findComponentById("MyButton");
    ASSERT_TRUE(touchWrapper);

    touchWrapper->update(kUpdatePressed, 1);
    ASSERT_TRUE(mPokeQueue.front());

    ASSERT_TRUE(CheckSetValueEvent("muchtext", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 20.0,
            "id": "MyButton",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "TouchWrapper",
            "uid": "[EXISTS]",
            "width": 30.0,
            "source": "TouchWrapper",
            "value": false,
            "handler": "Press"
          },
          "target": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 100.0,
            "id": "MyTarget",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "Text",
            "uid": "[EXISTS]",
            "width": 100.0,
            "text": "My text is bold",
            "color": "#0000ffff"
          }
        }
    )"));
}


static const char * TARGET_TOUCH_WRAPPER = R"(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "items": [
            {
              "type": "TouchWrapper",
              "id": "MyTarget",
              "width": 100,
              "height": 100,
              "checked": true,
              "disabled": true
            },
            {
              "type": "TouchWrapper",
              "id": "MyButton",
              "height": 20,
              "width": 30,
              "onPress": {
                "type": "Poke",
                "componentId": "MyTarget",
                "property": "pushbutton",
                "value": "${event}"
              }
            }
          ]
        }
      }
    }
)";

TEST_F(SerializeEventTest, TargetTouchWrapper) {
    loadDocument(TARGET_TOUCH_WRAPPER);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->hasEvent());

    auto target = component->findComponentById("MyTarget");
    ASSERT_TRUE(target);

    auto touchWrapper = component->findComponentById("MyButton");
    ASSERT_TRUE(touchWrapper);

    touchWrapper->update(kUpdatePressed, 1);
    ASSERT_TRUE(mPokeQueue.front());

    ASSERT_TRUE(CheckSetValueEvent("pushbutton", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 20.0,
            "id": "MyButton",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "TouchWrapper",
            "uid": "[EXISTS]",
            "width": 30.0,
            "source": "TouchWrapper",
            "value": false,
            "handler": "Press"
          },
          "target": {
            "bind": {},
            "checked": true,
            "disabled": true,
            "focused": false,
            "height": 100.0,
            "id": "MyTarget",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "TouchWrapper",
            "uid": "[EXISTS]",
            "width": 100.0
          }
        }
    )"));
}

static const char * TOUCH_VECTOR_GRAPHIC = R"(
    {
      "type": "APL",
      "version": "1.4",
      "graphics": {
        "MyIcon": {
          "type": "AVG",
          "version": "1.0",
          "height": 100,
          "width": 100,
          "items": {
            "type": "path",
            "pathData": "M0,0 h100 v100 h-100 z",
            "fill": "red"
          }
        }
      },
      "mainTemplate": {
        "items": {
          "type": "Container",
          "items": [
            {
              "type": "VectorGraphic",
              "id": "MyTarget",
              "width": 100,
              "height": 100,
              "source": "MyIcon"
            },
            {
              "type": "VectorGraphic",
              "id": "MyButton",
              "height": 20,
              "width": 30,
              "source": "MyIcon",
              "onPress": {
                "type": "Poke",
                "componentId": "MyTarget",
                "property": "pressbutton",
                "value": "${event}"
              },
              "onUp": {
                "type": "Poke",
                "componentId": "MyTarget",
                "property": "upbutton",
                "value": "${event}"
              },
              "onDown": {
                "type": "Poke",
                "componentId": "MyTarget",
                "property": "downbutton",
                "value": "${event}"
              },
              "onMove": {
                "type": "Poke",
                "componentId": "MyTarget",
                "property": "movebutton",
                "value": "${event}"
              }
            }
          ]
        }
      }
    }
)";

TEST_F(SerializeEventTest, TouchVectorGraphic) {
    loadDocument(TOUCH_VECTOR_GRAPHIC);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->hasEvent());

    auto target = component->findComponentById("MyTarget");
    ASSERT_TRUE(target);

    auto touchWrapper = component->findComponentById("MyButton");
    ASSERT_TRUE(touchWrapper);

    // We click in global coordinates, which should be at (1,1) in the VectorGraphic component.
    // The graphic contained in the vector graphic has NOT been scaled and has default alignment (center),
    // so the graphic top-left is at (-35, -40).  That click at (1,1) translates to (36,41) in
    // viewport coordinates.
    root->handlePointerEvent(PointerEvent(kPointerDown, Point(1, 101), 0, kTouchPointer));
    ASSERT_TRUE(mPokeQueue.front());

    ASSERT_TRUE(CheckSetValueEvent("downbutton", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 20.0,
            "id": "MyButton",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": true,
            "type": "VectorGraphic",
            "uid": "[EXISTS]",
            "width": 30.0,
            "source": "MyIcon",
            "url": "MyIcon",
            "value": false,
            "handler": "Down"
          },
          "target": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 100.0,
            "id": "MyTarget",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "VectorGraphic",
            "uid": "[EXISTS]",
            "width": 100.0,
            "source": "MyIcon",
            "url": "MyIcon"
          },
          "component": {
            "x": 1,
            "y": 1,
            "width": 30,
            "height": 20
          },
          "viewport": {
            "x": 36,
            "y": 41,
            "width": 100,
            "height": 100,
            "inBounds": true
          }
        }
    )"));

    root->handlePointerEvent(PointerEvent(kPointerMove, Point(5, 105), 0, kTouchPointer));
    ASSERT_TRUE(mPokeQueue.front());

    ASSERT_TRUE(CheckSetValueEvent("movebutton", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 20.0,
            "id": "MyButton",
            "opacity": 1.0,
            "layoutDirection": "LTR",
            "pressed": true,
            "type": "VectorGraphic",
            "uid": "[EXISTS]",
            "width": 30.0,
            "source": "MyIcon",
            "url": "MyIcon",
            "value": false,
            "handler": "Move"
          },
          "target": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 100.0,
            "id": "MyTarget",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "VectorGraphic",
            "uid": "[EXISTS]",
            "width": 100.0,
            "source": "MyIcon",
            "url": "MyIcon"
          },
          "component": {
            "x": 5,
            "y": 5,
            "width": 30,
            "height": 20
          },
          "viewport": {
            "x": 40,
            "y": 45,
            "width": 100,
            "height": 100,
            "inBounds": true
          },
          "inBounds": true
        }
    )"));

    root->handlePointerEvent(PointerEvent(kPointerMove, Point(31, 105), 0, kTouchPointer));
    ASSERT_TRUE(mPokeQueue.front());

    ASSERT_TRUE(CheckSetValueEvent("movebutton", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 20.0,
            "id": "MyButton",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": true,
            "type": "VectorGraphic",
            "uid": "[EXISTS]",
            "width": 30.0,
            "source": "MyIcon",
            "url": "MyIcon",
            "value": false,
            "handler": "Move"
          },
          "target": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 100.0,
            "id": "MyTarget",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "VectorGraphic",
            "uid": "[EXISTS]",
            "width": 100.0,
            "source": "MyIcon",
            "url": "MyIcon"
          },
          "component": {
            "x": 31,
            "y": 5,
            "width": 30,
            "height": 20
          },
          "viewport": {
            "x": 66,
            "y": 45,
            "width": 100,
            "height": 100,
            "inBounds": true
          },
          "inBounds": false
        }
    )"));

    root->handlePointerEvent(PointerEvent(kPointerUp, Point(30, 105), 0, kTouchPointer));
    ASSERT_TRUE(mPokeQueue.front());

    ASSERT_TRUE(CheckSetValueEvent("upbutton", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 20.0,
            "id": "MyButton",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "VectorGraphic",
            "uid": "[EXISTS]",
            "width": 30.0,
            "source": "MyIcon",
            "url": "MyIcon",
            "value": false,
            "handler": "Up"
          },
          "target": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 100.0,
            "id": "MyTarget",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "VectorGraphic",
            "uid": "[EXISTS]",
            "width": 100.0,
            "source": "MyIcon",
            "url": "MyIcon"
          },
          "component": {
            "x": 30,
            "y": 5,
            "width": 30,
            "height": 20
          },
          "viewport": {
            "x": 65,
            "y": 45,
            "width": 100,
            "height": 100,
            "inBounds": true
          },
          "inBounds": true
        }
    )"));

    ASSERT_TRUE(CheckSetValueEvent("pressbutton", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 20.0,
            "id": "MyButton",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "VectorGraphic",
            "uid": "[EXISTS]",
            "width": 30.0,
            "source": "MyIcon",
            "url": "MyIcon",
            "value": false,
            "handler": "Press"
          },
          "target": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 100.0,
            "id": "MyTarget",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "VectorGraphic",
            "uid": "[EXISTS]",
            "width": 100.0,
            "source": "MyIcon",
            "url": "MyIcon"
          }
        }
    )"));
}

static const char * TARGET_VECTOR_GRAPHIC = R"(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "items": [
            {
              "type": "VectorGraphic",
              "id": "MyTarget",
              "width": 100,
              "height": 100,
              "source": "MyIcon"
            },
            {
              "type": "TouchWrapper",
              "id": "MyButton",
              "height": 20,
              "width": 30,
              "onPress": {
                "type": "Poke",
                "componentId": "MyTarget",
                "property": "pushbutton",
                "value": "${event}"
              }
            }
          ]
        }
      }
    }
)";

TEST_F(SerializeEventTest, TargetVectorGraphic) {
    loadDocument(TARGET_VECTOR_GRAPHIC);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->hasEvent());

    auto target = component->findComponentById("MyTarget");
    ASSERT_TRUE(target);

    auto touchWrapper = component->findComponentById("MyButton");
    ASSERT_TRUE(touchWrapper);

    performTap(0,100);
    ASSERT_TRUE(mPokeQueue.front());

    ASSERT_TRUE(CheckSetValueEvent("pushbutton", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 20.0,
            "id": "MyButton",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "TouchWrapper",
            "uid": "[EXISTS]",
            "width": 30.0,
            "source": "TouchWrapper",
            "value": false,
            "handler": "Press"
          },
          "target": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 100.0,
            "id": "MyTarget",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "VectorGraphic",
            "uid": "[EXISTS]",
            "width": 100.0,
            "source": "MyIcon",
            "url": "MyIcon"
          }
        }
    )"));
}

TEST_F(SerializeEventTest, TargetVectorGraphicDirectPress) {
    loadDocument(TARGET_VECTOR_GRAPHIC);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->hasEvent());

    auto target = component->findComponentById("MyTarget");
    ASSERT_TRUE(target);

    auto touchWrapper = component->findComponentById("MyButton");
    ASSERT_TRUE(touchWrapper);

    touchWrapper->update(kUpdatePressed, 0);
    ASSERT_TRUE(mPokeQueue.front());

    ASSERT_TRUE(CheckSetValueEvent("pushbutton", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 20.0,
            "id": "MyButton",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "TouchWrapper",
            "uid": "[EXISTS]",
            "width": 30.0,
            "source": "TouchWrapper",
            "value": false,
            "handler": "Press"
          },
          "target": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 100.0,
            "id": "MyTarget",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "VectorGraphic",
            "uid": "[EXISTS]",
            "width": 100.0,
            "source": "MyIcon",
            "url": "MyIcon"
          }
        }
    )"));
}

static const char * TARGET_VIDEO = R"(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "items": [
            {
              "type": "Video",
              "id": "MyTarget",
              "width": 100,
              "height": 100,
              "source": [
                "MyVideo1",
                "MyVideo2"
              ]
            },
            {
              "type": "TouchWrapper",
              "id": "MyButton",
              "height": 20,
              "width": 30,
              "onPress": {
                "type": "Poke",
                "componentId": "MyTarget",
                "property": "pushbutton",
                "value": "${event}"
              }
            }
          ]
        }
      }
    }
)";


TEST_F(SerializeEventTest, TargetVideo) {
    loadDocument(TARGET_VIDEO);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->hasEvent());

    auto target = component->findComponentById("MyTarget");
    ASSERT_TRUE(target);

    auto touchWrapper = component->findComponentById("MyButton");
    ASSERT_TRUE(touchWrapper);

    touchWrapper->update(kUpdatePressed, 1);
    ASSERT_TRUE(mPokeQueue.front());

    ASSERT_TRUE(CheckSetValueEvent("pushbutton", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 20.0,
            "id": "MyButton",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "TouchWrapper",
            "uid": "[EXISTS]",
            "width": 30.0,
            "source": "TouchWrapper",
            "value": false,
            "handler": "Press"
          },
          "target": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "duration": 0.0,
            "ended": false,
            "focused": false,
            "height": 100.0,
            "id": "MyTarget",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "Video",
            "uid": "[EXISTS]",
            "width": 100.0,
            "source": "MyVideo1",
            "url": "MyVideo1",
            "currentTime": 0,
            "paused": true,
            "trackCount": 2.0,
            "trackIndex": 0,
            "trackState": "notReady"
          }
        }
    )"));
}


static const char * SEND_EVENT = R"(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "TouchWrapper",
          "height": 200,
          "width": 300,
          "onPress": {
            "type": "SendEvent",
            "arguments": "Freeze"
          }
        }
      }
    }
)";

// SendEvent serializes "event.source".  Since this can depend on ComponentEventWrapper data,
// we need to freeze the "event.source" value when the SendEvent is dispatched.  We verify
// that it is frozen here by sending the event, then modifying properties in the sending component.

TEST_F(SerializeEventTest, SendEvent) {
    loadDocument(SEND_EVENT);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->hasEvent());

    component->update(kUpdatePressed, 1);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    // Change the state of the component before verifying the value
    component->setState(kStatePressed, true);

    ASSERT_TRUE(CompareValue(event.getValue(kEventPropertySource), R"(
        {
          "bind": {},
          "checked": false,
          "disabled": false,
          "focused": false,
          "height": 200.0,
          "id": "",
          "layoutDirection": "LTR",
          "opacity": 1.0,
          "pressed": false,
          "type": "TouchWrapper",
          "uid": "[EXISTS]",
          "width": 300.0,
          "source": "TouchWrapper",
          "value": false,
          "handler": "Press"
        }
    )"));
}


static const char *EXTENSION_EVENT = R"(
    {
      "type": "APL",
      "version": "1.4",
      "extensions": {
        "name": "FireTest",
        "uri": "aplext:SerializeTest"
      },
      "mainTemplate": {
        "items": {
          "type": "TouchWrapper",
          "height": 200,
          "width": 300,
          "onPress": {
            "type": "FireTest:Fire",
            "name": "buttonPressed"
          }
        }
      }
    }
)";

// Extension commands serialize "event.source".  Since this can depend on ComponentEventWrapper data,
// we need to freeze the "event.source" value when the ExtensionEvent is dispatched.  We verify
// that it is frozen here by sending the event, then modifying properties in the sending component.

TEST_F(SerializeEventTest, ExtensionEvent) {
    config->registerExtensionCommand(
            ExtensionCommandDefinition("aplext:SerializeTest", "Fire")
            .property("name", "", true));

    loadDocument(EXTENSION_EVENT);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->hasEvent());

    component->update(kUpdatePressed, 1);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    auto ext = event.getValue(kEventPropertyExtension);
    ASSERT_TRUE(IsEqual("buttonPressed", ext.get("name")));

    // Change the state of the component before verifying the value
    component->setState(kStatePressed, true);

    ASSERT_TRUE(CompareValue(event.getValue(kEventPropertySource), R"(
        {
          "bind": {},
          "checked": false,
          "disabled": false,
          "focused": false,
          "height": 200.0,
          "id": "",
          "layoutDirection": "LTR",
          "opacity": 1.0,
          "pressed": false,
          "type": "TouchWrapper",
          "uid": "[EXISTS]",
          "width": 300.0,
          "source": "TouchWrapper",
          "value": false,
          "handler": "Press"
        }
    )"));
}


static const char * OPEN_URL_EVENT = R"(
    {
      "type": "APL",
      "version": "1.4",
      "extensions": {
        "name": "E",
        "uri": "aplext:Event"
      },
      "mainTemplate": {
        "items": {
          "type": "TouchWrapper",
          "height": 200,
          "width": 300,
          "onPress": {
            "type": "OpenURL",
            "source": "FirstURL",
            "onFail": {
              "type": "E:Validate",
              "event": "${event}",
              "name": "failed"
            }
          }
        }
      }
    }
)";

/*
 * The OpenURL command has an "onFail" handler that doesn't reflect the source component
 * that originally sent OpenURL.  Arguably this is a bad idea in the specification, but we
 * still need to test it.
 */
TEST_F(SerializeEventTest, OpenURL) {
    config->allowOpenUrl(true);

    loadDocument(OPEN_URL_EVENT);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->hasEvent());

    component->update(kUpdatePressed, 1);

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeOpenURL, event.getType());

    event.getActionRef().resolve(23);  // Fail the open command

    ASSERT_TRUE(CheckValidate("failed", R"(
        {
          "source": {
            "type": "OpenURL",
            "source": "OpenURL",
            "handler": "Fail",
            "value": 23
          }
        }
    )"));
}


static const char * BIND_REFERENCES = R"(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "bind": {
            "name": "A",
            "value": "Buzz"
          },
          "items": [
            {
              "type": "Text",
              "id": "MyText",
              "bind": {
                "name": "B",
                "value": "Bar"
              },
              "text": ""
            },
            {
              "type": "TouchWrapper",
              "id": "MyButton",
              "bind": {
                "name": "C",
                "value": "Foo"
              },
              "height": 35,
              "width": 118,
              "onPress": {
                "type": "SetValue",
                "componentId": "MyText",
                "property": "text",
                "value": "${event.source.bind.C} ${event.target.bind.B} ${event.source.bind.A} ${event.target.bind.A}"
              }
            }
          ]
        }
      }
    }
)";

TEST_F(SerializeEventTest, BindReferences) {
    config->allowOpenUrl(true);

    loadDocument(BIND_REFERENCES);
    ASSERT_TRUE(component);
    ASSERT_FALSE(root->hasEvent());

    auto button = component->findComponentById("MyButton");
    ASSERT_TRUE(button);
    auto text = component->findComponentById("MyText");
    ASSERT_TRUE(text);

    performTap(0, 10);
    ASSERT_TRUE(IsEqual("Foo Bar Buzz Buzz", text->getCalculated(kPropertyText).asString()));
}

static const char * COMPARE_WEAK_REFERENCES = R"(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "items": [
            {
              "type": "TouchWrapper",
              "id": "A"
            },
            {
              "type": "TouchWrapper",
              "id": "B"
            }
          ]
        }
      }
    }
)";

TEST_F(SerializeEventTest, WeakReferences) {
    loadDocument(COMPARE_WEAK_REFERENCES);
    ASSERT_TRUE(component);

    auto a = component->findComponentById("A");
    auto b = component->findComponentById("B");
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);

    auto target_a = Object(ComponentEventTargetWrapper::create(std::static_pointer_cast<const CoreComponent>(a)));
    auto target_a2 = Object(ComponentEventTargetWrapper::create(std::static_pointer_cast<const CoreComponent>(a)));
    auto target_b = Object(ComponentEventTargetWrapper::create(std::static_pointer_cast<const CoreComponent>(b)));

    ASSERT_TRUE(target_a == target_a);
    ASSERT_TRUE(target_b == target_b);
    ASSERT_FALSE(target_a == target_b);
    ASSERT_TRUE(target_a == target_a2);

    auto source_a = Object(ComponentEventSourceWrapper::create(std::static_pointer_cast<const CoreComponent>(a), "ha", 2));
    auto source_a2 = Object(ComponentEventSourceWrapper::create(std::static_pointer_cast<const CoreComponent>(a), "ha", 2));
    auto source_b = Object(ComponentEventSourceWrapper::create(std::static_pointer_cast<const CoreComponent>(b), "hb", 3));
    auto source_b2 = Object(ComponentEventSourceWrapper::create(std::static_pointer_cast<const CoreComponent>(b), "hb", 7));

    ASSERT_TRUE(source_a == source_a);
    ASSERT_TRUE(source_b == source_b);
    ASSERT_FALSE(source_a == source_b);
    ASSERT_TRUE(source_a == source_a2);
    ASSERT_FALSE(source_b == source_b2);

    ASSERT_FALSE(target_a == source_a);
    ASSERT_FALSE(target_b == source_b);

    auto context_a = Object(ContextWrapper::create(std::const_pointer_cast<const Context>(a->getContext())));
    auto context_a2 = Object(ContextWrapper::create(std::const_pointer_cast<const Context>(a->getContext())));
    auto context_b = Object(ContextWrapper::create(std::const_pointer_cast<const Context>(b->getContext())));

    ASSERT_TRUE(context_a == context_a);
    ASSERT_TRUE(context_b == context_b);
    ASSERT_FALSE(context_a == context_b);
    ASSERT_TRUE(context_a == context_a2);
}

static const char * EDIT_TEXT = R"(
    {
      "type": "APL",
      "version": "1.4",
      "extensions": {
        "name": "E",
        "uri": "aplext:Event"
      },
      "mainTemplate": {
        "items": {
          "type": "EditText",
          "height": 200,
          "width": 300,
          "onTextChange": {
            "type": "E:Validate",
            "event": "${event}",
            "name": "textchange"
          },
          "onSubmit": {
            "type": "E:Validate",
            "event": "${event}",
            "name": "submit"
          }
        }
      }
    }
)";

TEST_F(SerializeEventTest, EditText) {
    loadDocument(EDIT_TEXT);
    ASSERT_TRUE(component);

    component->update(kUpdateTextChange, "78");

    ASSERT_TRUE(CheckValidate("textchange", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 200.0,
            "id": "",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "EditText",
            "width": 300.0,
            "source": "EditText",
            "value": "78",
            "text": "78",
            "color": "#fafafaff",
            "handler": "TextChange",
            "uid": "[EXISTS]"
          }
        }
    )"));

    component->update(kUpdateSubmit, 1);

    ASSERT_TRUE(root->hasEvent());

    ASSERT_TRUE(CheckValidate("submit", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 200.0,
            "id": "",
            "layoutDirection": "LTR",
            "opacity": 1.0,
            "pressed": false,
            "type": "EditText",
            "width": 300.0,
            "source": "EditText",
            "value": "78",
            "text": "78",
            "color": "#fafafaff",
            "handler": "Submit",
            "uid": "[EXISTS]"
          }
        }
    )"));
}

/**
 * Redo test with RTL layout to test layout direction
 */
TEST_F(SerializeEventTest, EditTextRTL) {
    loadDocument(EDIT_TEXT);
    ASSERT_TRUE(component);

    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending(); // Force layout

    component->update(kUpdateTextChange, "78");

    ASSERT_TRUE(CheckValidate("textchange", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 200.0,
            "id": "",
            "layoutDirection": "RTL",
            "opacity": 1.0,
            "pressed": false,
            "type": "EditText",
            "width": 300.0,
            "source": "EditText",
            "value": "78",
            "text": "78",
            "color": "#fafafaff",
            "handler": "TextChange",
            "uid": "[EXISTS]"
          }
        }
    )"));

    component->update(kUpdateSubmit, 1);

    ASSERT_TRUE(root->hasEvent());

    ASSERT_TRUE(CheckValidate("submit", R"(
        {
          "source": {
            "bind": {},
            "checked": false,
            "disabled": false,
            "focused": false,
            "height": 200.0,
            "id": "",
            "layoutDirection": "RTL",
            "opacity": 1.0,
            "pressed": false,
            "type": "EditText",
            "width": 300.0,
            "source": "EditText",
            "value": "78",
            "text": "78",
            "color": "#fafafaff",
            "handler": "Submit",
            "uid": "[EXISTS]"
          }
        }
    )"));
}