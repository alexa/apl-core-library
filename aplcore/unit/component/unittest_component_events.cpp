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
#include "apl/component/componenteventsourcewrapper.h"
#include "apl/component/componenteventtargetwrapper.h"
#include "apl/engine/event.h"
#include "apl/primitives/mediastate.h"

#include "../testeventloop.h"


using namespace apl;

class ComponentEventsTest : public CommandTest {};

static const char *DATA = "{}";

static const char *TOUCH_WRAPPER_PRESSED = R"(
         {  
           "type": "APL",  
           "version": "1.0",  
           "mainTemplate": {  
             "parameters": [  
               "payload"  
             ],  
             "items": {  
               "type": "TouchWrapper",  
               "onPress": {  
                 "type": "SetValue",  
                 "componentId": "textComp",  
                 "property": "text",  
                 "value": "Two"  
               },  
               "items": {  
                 "type": "Text",  
                 "id": "textComp",  
                 "text": "One"  
               }  
             }  
           }  
         }
)";

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

static const char *TOUCH_WRAPPER_PRESSED_NO_ID_CHILD = R"(
        {
           "type": "APL",
           "version": "1.0",
           "mainTemplate": {
             "parameters": [
               "payload"
             ],
             "items": {
               "type": "TouchWrapper",
               "onPress": {
                 "type": "SetValue",
                 "property": "text",
                 "value": "Two"
               },
               "items": {
                 "type": "Text",
                 "id": "textComp",
                 "text": "One"
               }
             }
           }
        }
)";

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

static const char *TOUCH_WRAPPER_PRESSED_NO_ID_NOT_CHILD = R"(
        {
           "type": "APL",
           "version": "1.0",
           "mainTemplate": {
             "parameters": [
               "payload"
             ],
             "items": {
               "type": "Container",
               "items": [
                 {
                   "type": "TouchWrapper",
                   "id": "touch",
                   "height": 10,
                   "onPress": {
                     "type": "SetValue",
                     "property": "text",
                     "value": "Two"
                   }
                 },
                 {
                   "type": "Text",
                   "id": "textComp",
                   "text": "One"
                 }
               ]
             }
           }
        }
)";

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

static const char *COMPONENT_SCROLLED = R"(
        {
           "type": "APL",
           "version": "1.0",
           "mainTemplate": {
             "parameters": [
               "payload"
             ],
             "items": {
               "type": "ScrollView",
               "height": 10,
               "onScroll": {
                 "type": "SetValue",
                 "componentId": "textComp",
                 "property": "text",
                 "value": "Two"
               },
               "item": {
                 "type": "Text",
                 "height": 50,
                 "id": "textComp",
                 "text": "One"
               }
             }
           }
        }"
)";

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
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));
    ASSERT_TRUE(CheckDirty(root, component, text));
    ASSERT_EQ("Two", text->getCalculated(kPropertyText).asString());
}

static const char *PAGER_CHANGED = R"(
        {
           "type": "APL",
           "version": "1.0",
           "mainTemplate": {
             "parameters": [
               "payload"
             ],
             "items": {
               "type": "Pager",
               "initialPage": 1,
               "onPageChanged": {
                 "type": "SetValue",
                 "componentId": "textComp",
                 "property": "text",
                 "value": "Two"
               },
               "items": [
                 {
                   "type": "Text",
                   "id": "textComp",
                   "text": "One"
                 },
                 {
                   "type": "Text",
                   "text": "Not one"
                 }
               ]
             }
           }
        }
)";

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

static const char *MEDIA_STATE_CHANGES = R"(
        {
           "type": "APL",
           "version": "1.0",
           "mainTemplate": {
             "items": {
               "type": "Container",
               "items": [
                 {
                   "type": "Video",
                   "id": "video",
                   "scale": "best-fill",
                   "source": [
                     {"url": "Video1", "duration": 1000},
                     {"url": "Video2", "duration": 1000}
                   ],
                   "onEnd": {
                     "type": "SetValue",
                     "componentId": "textComp",
                     "property": "text",
                     "value": "END"
                   },
                   "onPlay": {
                     "type": "SetValue",
                     "componentId": "textComp",
                     "property": "text",
                     "value": "PLAY"
                   },
                   "onPause": {
                     "type": "SetValue",
                     "componentId": "textComp",
                     "property": "text",
                     "value": "PAUSE"
                   },
                   "onTrackUpdate": {
                     "type": "SetValue",
                     "componentId": "textComp",
                     "property": "text",
                     "value": "TRACK_UPDATE"
                   },
                   "onTrackFail": {
                     "type": "SetValue",
                     "componentId": "textComp",
                     "property": "text",
                     "value": "${event.errorCode}"
                   }
                 },
                 {
                   "type": "Text",
                   "id": "textComp",
                   "text": "One"
                 }
               ]
             }
           }
        }
)";

TEST_F(ComponentEventsTest, MediaStateChanges)
{
    mediaPlayerFactory->addFakeContent({
        {"Video1", 1000, 0, -1},
        {"Video2", 1000, 0, -1},
    });

    loadDocument(MEDIA_STATE_CHANGES);
    ASSERT_TRUE(component);

    auto video = context->findComponentById("video");
    ASSERT_TRUE(video);
    ASSERT_EQ(kComponentTypeVideo, video->getType());

    auto text = context->findComponentById("textComp");
    ASSERT_TRUE(text);

    ASSERT_EQ("One", text->getCalculated(kPropertyText).asString());

    mediaPlayerFactory->advanceTime(100);
    clearEvents();

    // Simulate playback start
    executeCommand("ControlMedia", {{"componentId", "video"}, {"command", "play"}}, false);
    advanceTime(10);
    ASSERT_EQ(1, root->getDirty().size());
    root->clearDirty();
    ASSERT_EQ("PLAY", text->getCalculated(kPropertyText).asString());

    // Simulate playback pause
    executeCommand("ControlMedia", {{"componentId", "video"}, {"command", "pause"}}, false);
    advanceTime(10);
    ASSERT_EQ(1, root->getDirty().size());
    root->clearDirty();
    ASSERT_EQ("PAUSE", text->getCalculated(kPropertyText).asString());

    // Simulate track change
    executeCommand("ControlMedia", {{"componentId", "video"}, {"command", "next"}}, false);
    advanceTime(10);
    ASSERT_EQ(1, root->getDirty().size());
    root->clearDirty();
    ASSERT_EQ("TRACK_UPDATE", text->getCalculated(kPropertyText).asString());

    // Simulate playback end
    executeCommand("ControlMedia", {{"componentId", "video"}, {"command", "play"}}, false);
    mediaPlayerFactory->advanceTime(100);
    mediaPlayerFactory->advanceTime(900);

    ASSERT_EQ(1, root->getDirty().size());
    root->clearDirty();
    ASSERT_EQ("END", text->getCalculated(kPropertyText).asString());
}

TEST_F(ComponentEventsTest, MediaErrorStateChanges)
{
    mediaPlayerFactory->addFakeContent({
        {"Video1", 1000, 0, 0},
        {"Video2", 1000, 0, 500},
    });

    loadDocument(MEDIA_STATE_CHANGES);
    ASSERT_TRUE(component);

    auto video = context->findComponentById("video");
    ASSERT_TRUE(video);
    ASSERT_EQ(kComponentTypeVideo, video->getType());

    auto text = context->findComponentById("textComp");
    ASSERT_TRUE(text);
    ASSERT_EQ("One", text->getCalculated(kPropertyText).asString());

    // Error at playback start
    executeCommand("ControlMedia", {{"componentId", "video"}, {"command", "play"}}, false);
    mediaPlayerFactory->advanceTime(10);
    advanceTime(10);

    ASSERT_EQ("99", text->getCalculated(kPropertyText).asString());
    ASSERT_FALSE(root->screenLock());

    // Switch to the next track and play until error
    executeCommand("ControlMedia", {{"componentId", "video"}, {"command", "next"}}, false);
    ASSERT_EQ("TRACK_UPDATE", text->getCalculated(kPropertyText).asString());
    ASSERT_FALSE(root->screenLock());

    executeCommand("ControlMedia", {{"componentId", "video"}, {"command", "play"}}, false);
    mediaPlayerFactory->advanceTime(100);
    advanceTime(100);

    ASSERT_EQ("PLAY", text->getCalculated(kPropertyText).asString());

    mediaPlayerFactory->advanceTime(400);
    advanceTime(400);
    ASSERT_EQ("99", text->getCalculated(kPropertyText).asString());
}

static const char *TOUCH_WRAPPER_SEND_EVENT = R"(
        {
           "type": "APL",
           "version": "1.1",
           "mainTemplate": {
             "parameters": [
               "payload"
             ],
             "items": {
               "type": "TouchWrapper",
               "onPress": {
                 "type": "SendEvent",
                 "arguments": [ 
                   "${event.source.handler}",
                   "${event.source.value}",
                   "${event.target.opacity}" 
                 ],
                 "components": [ "textComp" ]
               },
               "items": {
                 "type": "Text",
                 "id": "textComp",
                 "text": "<b>One</b>"
               }
             }
           }
        }
)";

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
    ASSERT_TRUE(arguments.isArray());
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
    ASSERT_TRUE(components.isMap());
    ASSERT_EQ("<b>One</b>", components.get("textComp").asString());
}

static const char *PAGER_SEND_EVENT = R"(
        {
           "type": "APL",
           "version": "1.0",
           "mainTemplate": {
             "parameters": [
               "payload"
             ],
             "items": {
               "type": "Pager",
               "initialPage": 1,
               "onPageChanged": {
                 "type": "SendEvent",
                 "arguments": [ 
                   "${event.source.handler}",
                   "${event.source.value}",
                   "${event.target.opacity}" 
                 ],
                 "components": [ "text1", "text2", "text3" ]
               },
               "items": [
                 {
                   "type": "Text",
                   "id": "text1",
                   "text": "One"
                 },
                 {
                   "type": "Text",
                   "id": "text2",
                   "text": "Two"
                 },
                 {
                   "type": "Text",
                   "id": "text3",
                   "text": "Three"
                 }
               ]
             }
           }
        }
)";

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
    ASSERT_TRUE(arguments.isArray());
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
    ASSERT_TRUE(components.isMap());
    ASSERT_EQ("One", components.get(text1->getId()).asString());
    ASSERT_EQ("Two", components.get(text2->getId()).asString());
    ASSERT_EQ("Three", components.get(text3->getId()).asString());
}

static const char *SCROLLABLE_SEND_EVENT = R"(
        {
           "type": "APL",
           "version": "1.0",
           "mainTemplate": {
             "parameters": [
               "payload"
             ],
             "items": {
               "type": "ScrollView",
               "height": 10,
               "onScroll": {
                 "type": "Custom",
                 "arguments": [ 
                   "${event.source.handler}",
                   "${event.source.value}",
                   "${event.target.opacity}" 
                 ],
                 "components": [ "textComp" ]
               },
               "item": {
                 "type": "Text",
                 "id": "textComp",
                 "height": 50,
                 "text": "One"
               }
             }
           }
        }"
)";

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
    ASSERT_TRUE(arguments.isArray());
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

static const char *CHILDREN_CHANGED = R"apl({
  "type": "APL",
  "version": "2023.3",
  "theme": "dark",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "id": "baseContainer",
      "width": "100%",
      "height": 100,
      "items": [],
      "onChildrenChanged": {
        "type": "Sequential",
        "data": "${event.changes}",
        "commands": {
          "type": "SendEvent",
          "sequencer": "SE",
          "arguments": [
            "${event.source.handler}",
            "${data.index ? data.index : 0}",
            "${data.action}",
            "${event.length}"
          ],
          "components": [ "textComp" ]
        }
      }
    }
  }
})apl";

TEST_F(ComponentEventsTest, ChildrenChanged)
{
    loadDocument(CHILDREN_CHANGED);
    ASSERT_TRUE(component);

    rapidjson::Document  doc;
    doc.Parse(R"([
      {
        "type": "InsertItem",
        "componentId": "baseContainer",
        "at": 10,
        "item": {
          "type": "Frame",
          "height": 200,
          "width": "100%"
        }
      }
    ])");

    executeCommands(apl::Object(doc), false);

    root->clearPending();
    advanceTime(500);

    ASSERT_TRUE(CheckSendEvent(root, "ChildrenChanged", 0, "insert", 1));

    executeCommands(apl::Object(doc), false);

    root->clearPending();
    advanceTime(500);

    ASSERT_TRUE(CheckSendEvent(root, "ChildrenChanged", 1, "insert", 2));


    executeCommands(apl::Object(doc), false);
    executeCommands(apl::Object(doc), false);

    doc.Parse(R"apl([
      {
        "type": "RemoveItem",
        "componentId": "baseContainer:child(0)"
      }
    ])apl");

    executeCommands(apl::Object(doc), false);

    root->clearPending();
    advanceTime(500);

    // Single event group
    ASSERT_TRUE(CheckSendEvent(root, "ChildrenChanged", 1, "insert", 3));
    ASSERT_TRUE(CheckSendEvent(root, "ChildrenChanged", 2, "insert", 3));
    ASSERT_TRUE(CheckSendEvent(root, "ChildrenChanged", 0, "remove", 3));
}

/**
 * These tests are more intrusive into the ComponentEventWrapperStructure
 */

static const char *TEXT_COMPONENT = R"(
 {
   "type": "APL",
   "version": "1.0",
   "mainTemplate": {
     "items": {
       "type": "Text",
       "text": "Hello"
     }
   }
 }
)";

static const std::vector<std::string> TARGET_KEYS = {
    "bind",
    "checked",
    "color",
    "disabled",
    "focused",
    "height",
    "id",
    "layoutDirection",
    "opacity",
    "pressed",
    "text",
    "type",
    "uid",
    "width",
};

static const std::vector<std::string> SOURCE_KEYS = {
    "bind",
    "checked",
    "color",
    "disabled",
    "focused",
    "height",
    "id",
    "layoutDirection",
    "opacity",
    "pressed",
    "text",
    "type",
    "uid",
    "width",
    "value",
    "handler",
    "source"
};


TEST_F(ComponentEventsTest, InnerLogic)
{
    loadDocument(TEXT_COMPONENT);
    ASSERT_TRUE(component);

    auto target_context = ComponentEventTargetWrapper::create(component);
    // Check possession of keys
    ASSERT_EQ(TARGET_KEYS.size(), target_context->size());
    for (auto i = 0 ; i < TARGET_KEYS.size() ; i++) {
        ASSERT_TRUE(target_context->has(TARGET_KEYS[i]));
        ASSERT_EQ(target_context->keyAt(i).first, TARGET_KEYS[i]);
    }

    auto source_context = ComponentEventSourceWrapper::create(component, "Test", 243);
    ASSERT_EQ(SOURCE_KEYS.size(), source_context->size());
    for (auto i = 0 ; i < SOURCE_KEYS.size() ; i++) {
        ASSERT_TRUE(source_context->has(SOURCE_KEYS[i]));
        ASSERT_EQ(source_context->keyAt(i).first, SOURCE_KEYS[i]);
    }
    ASSERT_TRUE(IsEqual("Test", source_context->opt("handler", 23)));
    ASSERT_TRUE(IsEqual(243, source_context->opt("value", 20000)));
    ASSERT_TRUE(IsEqual("Text", source_context->opt("source", "Error")));
    ASSERT_TRUE(IsEqual("Fuzzy", source_context->opt("MissingProperty", "Fuzzy")));
}

static const char *BINDING_CONTEXT = R"apl(
{
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "bind": {
        "name": "FOO",
        "value": "BAR"
      },
      "text": "Bind is ${BAR}"
    }
  }
}
)apl";

TEST_F(ComponentEventsTest, BindingContext)
{
    loadDocument(BINDING_CONTEXT);
    ASSERT_TRUE(component);
    advanceTime(123);  // Move time forward

    auto target_context = ComponentEventTargetWrapper::create(component);
    ASSERT_TRUE(target_context->has("bind"));

    auto bindings = target_context->get("bind");
    ASSERT_EQ(0, bindings.size());    // The context bindings are hidden from the component event

    // "elapsedTime" is a global that we can read from the context
    // Because ContextWrapper is a "Map-Like" object, you can't read the size of the bindings
    // or directly return a map or list of keys, but you can _get_ a value out of it.
    auto et = component->getContext()->find("elapsedTime");
    ASSERT_FALSE(et.empty());
    ASSERT_TRUE(IsEqual(123, et.object().value()));

    ASSERT_TRUE(bindings.has("elapsedTime"));
    ASSERT_TRUE(IsEqual(et.object().value(), bindings.get("elapsedTime")));

    ASSERT_FALSE(bindings.has("MissingProperty"));
    ASSERT_TRUE(IsEqual("FOO", bindings.opt("MissingProperty", "FOO")));
}