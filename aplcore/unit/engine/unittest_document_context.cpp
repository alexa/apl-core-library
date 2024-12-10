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

class DocumentContextTest : public DocumentWrapper {};

static const char *DOCUMENT_EXECUTION_CONTEXT = R"({
  "type": "APL",
  "version": "2024.3",
  "extensions": [
    {
      "name": "T",
      "uri": "aplext:Test"
    }
  ],
  "handleKeyDown": [
    {
      "when": "${event.keyboard.code == 'ArrowDown'}",
      "commands": [
        {
          "type": "SendEvent",
          "sequencer": "KEY_PRESSER",
          "arguments": [ "KEY_DOWN", "${MyGlobalData}" ]
        }
      ]
    }
  ],
  "handleKeyUp": [
    {
      "when": "${event.keyboard.code == 'ArrowDown'}",
      "commands": [
        {
          "type": "SendEvent",
          "sequencer": "KEY_PRESSER",
          "arguments": [ "KEY_UP", "${MyGlobalData}" ]
        }
      ]
    }
  ],
  "handleTick": [
    {
      "minimumDelay": 15000,
      "commands": [
        {
          "type": "SendEvent",
          "sequencer": "TICKER",
          "arguments": [ "TICK", "${MyGlobalData}" ]
        }
      ]
    }
  ],
  "onConfigChange": {
    "type": "SendEvent",
    "sequencer": "SEQUENTOR",
    "arguments": [ "CONFIG_CHANGE", "${MyGlobalData}" ]
  },
  "onDisplayStateChange": {
    "type": "SendEvent",
    "sequencer": "SEQUENTOR",
    "arguments": [ "DISPLAY_STATE_CHANGE", "${MyGlobalData}" ]
  },
  "onMount": {
    "type": "SendEvent",
    "sequencer": "SEQUENTOR",
    "arguments": [ "MOUNT", "${MyGlobalData}" ]
  },
  "mainTemplate": {
    "parameters": [ "MyGlobalData" ],
    "items": {
      "type": "Frame"
    }
  },
  "T:onExtensionHandler": {
    "type": "SendEvent",
    "sequencer": "SEQUENTOR",
    "arguments": [ "EXTENSION_HANDLER", "${MyGlobalData}" ]
  }
})";

static const char *DOCUMENT_EXECUTION_CONTEXT_DATA = R"({
  "MyGlobalData": "TEST"
})";

TEST_F(DocumentContextTest, ParametersExposure) {
    config->registerExtensionEventHandler(ExtensionEventHandler{"aplext:Test", "onExtensionHandler"});

    loadDocument(DOCUMENT_EXECUTION_CONTEXT, DOCUMENT_EXECUTION_CONTEXT_DATA);

    ASSERT_TRUE(CheckSendEvent(root, "MOUNT", "TEST"));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    ASSERT_TRUE(CheckSendEvent(root, "KEY_DOWN", "TEST"));

    root->handleKeyboard(kKeyUp, Keyboard::ARROW_DOWN_KEY());
    ASSERT_TRUE(CheckSendEvent(root, "KEY_UP", "TEST"));

    root->updateDisplayState(apl::kDisplayStateBackground);
    ASSERT_TRUE(CheckSendEvent(root, "DISPLAY_STATE_CHANGE", "TEST"));
    root->updateDisplayState(apl::kDisplayStateForeground);
    ASSERT_TRUE(CheckSendEvent(root, "DISPLAY_STATE_CHANGE", "TEST"));

    advanceTime(15000);
    ASSERT_TRUE(CheckSendEvent(root, "TICK", "TEST"));

    configChange(ConfigurationChange().theme("brisk"));
    ASSERT_TRUE(CheckSendEvent(root, "CONFIG_CHANGE", "TEST"));

    root->invokeExtensionEventHandler("aplext:Test", "onExtensionHandler", {}, false);
    ASSERT_TRUE(CheckSendEvent(root, "EXTENSION_HANDLER", "TEST"));
}