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
#include "apl/command/displaystatechangecommand.h"

using namespace apl;

class DisplayStateTest : public DocumentWrapper {};

static const char *DOC_WITH_DISPLAY_STATE_CHANGE_HANDLER = R"apl(
{
  "type": "APL",
  "version": "1.8",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "text": "Hello there"
    }
  },
  "onDisplayStateChange": [
    {
      "type": "SendEvent",
      "sequencer": "DUMMY",
      "arguments": [
        "${event.source.type}",
        "${event.source.handler}",
        "${event.displayState}"
      ]
    }
  ]
}
)apl";

TEST_F(DisplayStateTest, GlobalDataBindingAndChangeEventHandling)
{
    loadDocument(DOC_WITH_DISPLAY_STATE_CHANGE_HANDLER);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("foreground", evaluate(*context, "${displayState}")));

    root->updateDisplayState(kDisplayStateBackground);
    ASSERT_TRUE(CheckSendEvent(root, "Document", "DisplayStateChange", "background"));
    ASSERT_TRUE(IsEqual("background", evaluate(*context, "${displayState}")));

    root->updateDisplayState(kDisplayStateHidden);
    ASSERT_TRUE(CheckSendEvent(root, "Document", "DisplayStateChange", "hidden"));
    ASSERT_TRUE(IsEqual("hidden", evaluate(*context, "${displayState}")));
}

TEST_F(DisplayStateTest, DoNotSendEventIfDisplayStateDoNotChange)
{
    loadDocument(DOC_WITH_DISPLAY_STATE_CHANGE_HANDLER);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("foreground", evaluate(*context, "${displayState}")));

    // No-op
    root->updateDisplayState(kDisplayStateForeground);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(IsEqual("foreground", evaluate(*context, "${displayState}")));
}

TEST_F(DisplayStateTest, ViewHostCanSetInitialDisplayStateViaRootConfig)
{
    config->set(RootProperty::kInitialDisplayState, kDisplayStateBackground);
    
    loadDocument(DOC_WITH_DISPLAY_STATE_CHANGE_HANDLER);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("background", evaluate(*context, "${displayState}")));
}

TEST_F(DisplayStateTest, HandlesInvalidDisplayStatesFromViewHost)
{
    ASSERT_EQ(kDisplayStateForeground, config->getProperty(RootProperty::kInitialDisplayState).getInteger());

    // valid initial state accepted
    config->set(RootProperty::kInitialDisplayState, kDisplayStateBackground);
    ASSERT_EQ(kDisplayStateBackground, config->getProperty(RootProperty::kInitialDisplayState).getInteger());

    // invalid initial state reverts to default
    config->set(RootProperty::kInitialDisplayState, -1);
    ASSERT_EQ(kDisplayStateForeground, config->getProperty(RootProperty::kInitialDisplayState).getInteger());

    // another valid initial state accepted
    config->set(RootProperty::kInitialDisplayState, kDisplayStateHidden);
    ASSERT_EQ(kDisplayStateHidden, config->getProperty(RootProperty::kInitialDisplayState).getInteger());

    loadDocument(DOC_WITH_DISPLAY_STATE_CHANGE_HANDLER);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("hidden", evaluate(*context, "${displayState}")));

    // invalid state change ignored (not reverting to default)
    root->updateDisplayState(static_cast<DisplayState>(-1));
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(IsEqual("hidden", evaluate(*context, "${displayState}")));
}

static const char *PLAIN_DOC = R"apl(
{
  "type": "APL",
  "version": "1.8",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "text": "Hello there"
    }
  }
}
)apl";

TEST_F(DisplayStateTest, DisplayStateChangesWithoutHandlerWork)
{
    loadDocument(PLAIN_DOC);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("foreground", evaluate(*context, "${displayState}")));

    root->updateDisplayState(kDisplayStateBackground);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(IsEqual("background", evaluate(*context, "${displayState}")));
}

TEST_F(DisplayStateTest, DisplayStateChangeCommandHasExpectedName)
{
    auto command = DisplayStateChangeCommand::create(root, ObjectMap());
    ASSERT_TRUE(IsEqual("DisplayStateChangeCommand", command->name()));
}
