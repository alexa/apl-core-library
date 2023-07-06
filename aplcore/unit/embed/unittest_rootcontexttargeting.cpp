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

#include <memory>

#include "../testeventloop.h"
#include "../embed/testdocumentmanager.h"

#include "apl/dynamicdata.h"

using namespace apl;

class RootContextTargetingTest : public DocumentWrapper {
public:
    RootContextTargetingTest()
        : documentManager(std::make_shared<TestDocumentManager>())
    {
        config->documentManager(std::static_pointer_cast<DocumentManager>(documentManager));
    }

protected:
    std::shared_ptr<TestDocumentManager> documentManager;
};

static const char* DEFAULT_DOC = R"({
  "type": "APL",
  "version": "2023.1",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "top",
      "item": {
        "type": "Host",
        "id": "hostComponent",
        "source": "embeddedDocumentUrl",
        "onLoad": [
          {
            "type": "InsertItem",
            "componentId": "top",
            "item": {
              "type": "Text",
              "id": "hostOnLoadArtifact",
              "value": "hostComponentOnLoad triggered"
            }
          }
        ],
        "onFail": [
          {
            "type": "InsertItem",
            "componentId": "top",
            "item": {
              "type": "Text",
              "id": "hostOnFailArtifact",
              "value": "hostComponentOnFail triggered"
            }
          }
        ]
      }
    }
  }
})";

static const char* EMBEDDED_DEFAULT = R"({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "embeddedTop",
      "items": [
        {
          "type": "Text",
          "id": "embeddedText",
          "value": "Hello, World!"
        },
        {
          "type": "Host",
          "id": "nestedHost",
          "source": "nestedEmbeddedUrl",
          "onLoad": [
            {
              "type": "InsertItem",
              "componentId": "embeddedTop",
              "item": {
                "type": "Text",
                "id": "nestedHostOnLoadArtifact",
                "value": "hostComponentOnLoad triggered"
              }
            }
          ],
          "onFail": [
            {
              "type": "InsertItem",
              "componentId": "embeddedTop",
              "item": {
                "type": "Text",
                "id": "nestedHostOnFailArtifact",
                "value": "hostComponentOnFail triggered"
              }
            }
          ]
        }
      ]
    }
  }
})";

//static const char* EMBEDDED_NESTED = R"({
//  "type": "APL",
//  "version": "2023.2",
//  "mainTemplate": {
//    "item": {
//      "type": "Container",
//      "id": "nestedEmbeddedTop",
//      "items": [
//        {
//          "type": "Text",
//          "id": "nestedEmbeddedText",
//          "value": "Hello, World!"
//        }
//      ]
//    }
//  }
//})";

TEST_F(RootContextTargetingTest, TestFindComponentByIdWithoutDocumentIdForTopLevelComponent)
{
    loadDocument(DEFAULT_DOC);
    auto content = Content::create(EMBEDDED_DEFAULT, makeDefaultSession());
    ASSERT_TRUE(content->isReady());
    documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(root->findComponentById("hostOnLoadArtifact"));
    ASSERT_FALSE(root->findComponentById("hostOnFailArtifact"));

    ASSERT_TRUE(root->findComponentById("hostComponent"));
}

TEST_F(RootContextTargetingTest, TestFindComponentByIdWithoutDocumentIdForEmbeddedComponent)
{
    loadDocument(DEFAULT_DOC);
    auto content = Content::create(EMBEDDED_DEFAULT, makeDefaultSession());
    ASSERT_TRUE(content->isReady());
    documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(root->findComponentById("hostOnLoadArtifact"));
    ASSERT_FALSE(root->findComponentById("hostOnFailArtifact"));

    // It's a public API used by VH
    ASSERT_TRUE(root->findComponentById("embeddedText"));
}

TEST_F(RootContextTargetingTest, TestFindComponentByIdWithDocumentIdForUnregisteredDocumentId)
{
    loadDocument(DEFAULT_DOC);
    auto content = Content::create(EMBEDDED_DEFAULT, makeDefaultSession());
    ASSERT_TRUE(content->isReady());
    auto embed = CoreDocumentContext::cast(documentManager->succeed("embeddedDocumentUrl", content, true));
    ASSERT_TRUE(root->findComponentById("hostOnLoadArtifact"));
    ASSERT_FALSE(root->findComponentById("hostOnFailArtifact"));

    ASSERT_FALSE(embed->findComponentById("hostComponent"));
}

TEST_F(RootContextTargetingTest, TestFindComponentByIdWithDocumentIdForTopLevelComponent)
{
    loadDocument(DEFAULT_DOC);
    auto content = Content::create(EMBEDDED_DEFAULT, makeDefaultSession());
    ASSERT_TRUE(content->isReady());
    auto embed = CoreDocumentContext::cast(documentManager->succeed("embeddedDocumentUrl", content, true));
    ASSERT_TRUE(root->findComponentById("hostOnLoadArtifact"));
    ASSERT_FALSE(root->findComponentById("hostOnFailArtifact"));

    ASSERT_FALSE(embed->findComponentById("hostComponent"));
}

TEST_F(RootContextTargetingTest, TestFindComponentByIdWithDocumentIdForTargetEmbeddedComponent)
{
    loadDocument(DEFAULT_DOC);
    auto content = Content::create(EMBEDDED_DEFAULT, makeDefaultSession());
    ASSERT_TRUE(content->isReady());
    auto embed = CoreDocumentContext::cast(documentManager->succeed("embeddedDocumentUrl", content, true));
    ASSERT_TRUE(root->findComponentById("hostOnLoadArtifact"));
    ASSERT_FALSE(root->findComponentById("hostOnFailArtifact"));

    ASSERT_TRUE(embed->findComponentById("embeddedText"));
}

//TEST_F(RootContextTargetingTest, TestFindComponentByIdWithDocumentIdForNestedEmbeddedComponent)
//{
//    loadDocument(DEFAULT_DOC);
//    auto content = Content::create(EMBEDDED_DEFAULT, makeDefaultSession());
//    ASSERT_TRUE(content->isReady());
//    documentManager->succeed("embeddedDocumentUrl", content, true);
//    ASSERT_TRUE(root->findComponentById("hostOnLoadArtifact"));
//    ASSERT_FALSE(root->findComponentById("hostOnFailArtifact"));
//
//    content = Content::create(EMBEDDED_NESTED, makeDefaultSession());
//    ASSERT_TRUE(content->isReady());
//    documentManager->succeed("nestedEmbeddedUrl", content, true);
//    ASSERT_TRUE(root->findComponentById("nestedHostOnLoadArtifact", "embeddedDocumentUrl"));
//    ASSERT_FALSE(root->findComponentById("nestedHostOnFailArtifact", "embeddedDocumentUrl"));
//
//    ASSERT_FALSE(root->findComponentById("embeddedText", "nestedEmbeddedUrl"));
//    ASSERT_TRUE(root->findComponentById("nestedEmbeddedText", "nestedEmbeddedUrl"));
//}

static const char* HOST_ONLY_DOC = R"({
  "type": "APL",
  "version": "2023.1",
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "item": {
        "type": "Host",
        "id": "hostComponent",
        "source": "embeddedDocumentUrl",
        "onLoad": {
          "type": "SendEvent",
          "sequencer": "SEND_EVENT",
          "arguments": ["LOADED"]
        },
        "onFail": {
          "type": "SendEvent",
          "sequencer": "SEND_EVENT",
          "arguments": ["FAILED"]
        }
      }
    }
  }
})";

static const char* EMBEDDED_DYNAMIC_WITH_ON_MOUNT = R"({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "embeddedText",
      "text": "Hello, World!",
      "onMount": {
        "delay": 1000,
        "sequencer": "EMBEDDED_CHANGE",
        "type": "SetValue",
        "property": "text",
        "value": "Potatoes coming!"
      }
    }
  }
})";

TEST_F(RootContextTargetingTest, TestDirtyEmbeddedDocumentComponent)
{
    loadDocument(HOST_ONLY_DOC);
    auto content = Content::create(EMBEDDED_DYNAMIC_WITH_ON_MOUNT, makeDefaultSession());
    ASSERT_TRUE(content->isReady());
    documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));
    auto rootComp = component->findComponentById("hostComponent", true);
    ASSERT_TRUE(CheckDirty(rootComp, kPropertyNotifyChildrenChanged));
    ASSERT_EQ(1, rootComp->getDisplayedChildCount());

    root->clearDirty();

    auto text = component->findComponentById("embeddedText", true);
    ASSERT_EQ("Hello, World!", text->getCalculated(apl::kPropertyText).asString());

    ASSERT_EQ(1, loop->size());

    advanceTime(1500);
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));

    ASSERT_EQ("Potatoes coming!", text->getCalculated(apl::kPropertyText).asString());

    root->clearDirty();

    ASSERT_FALSE(root->isDirty());
}

TEST_F(RootContextTargetingTest, TestEmbeddedDocumentRemoveCleanup)
{
    loadDocument(HOST_ONLY_DOC);
    auto content = Content::create(EMBEDDED_DYNAMIC_WITH_ON_MOUNT, makeDefaultSession());
    ASSERT_TRUE(content->isReady());
    documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    root->clearDirty();

    auto text = component->findComponentById("embeddedText", true);
    ASSERT_EQ("Hello, World!", text->getCalculated(apl::kPropertyText).asString());

    advanceTime(1500);

    ASSERT_EQ("Potatoes coming!", text->getCalculated(apl::kPropertyText).asString());

    auto actionRef = executeCommand("RemoveItem",
                                   {{"componentId", "hostComponent"}},
                                   false);

    advanceTime(50);

    ASSERT_EQ(0, loop->size());

    ASSERT_FALSE(actionRef->isPending());

    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, component));
}

static const char* EMBEDDED_DYNAMIC_WITH_SEND_EVENT = R"({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "embeddedText",
      "text": "Hello, World!",
      "onMount": {
        "delay": 1000,
        "sequencer": "EMBEDDED_SEND",
        "type": "SendEvent",
        "arguments": ["EMBEDDED"]
      }
    }
  }
})";

TEST_F(RootContextTargetingTest, TestEmbeddedDocumentEvent)
{
    loadDocument(HOST_ONLY_DOC);
    auto content = Content::create(EMBEDDED_DYNAMIC_WITH_SEND_EVENT, makeDefaultSession());
    ASSERT_TRUE(content->isReady());
    documentManager->succeed("embeddedDocumentUrl", content, true);

    advanceTime(1500);

    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));
    ASSERT_TRUE(CheckSendEvent(root, "EMBEDDED"));

    root->clearDirty();
}

TEST_F(RootContextTargetingTest, TestEmbeddedDocumentEventClearOnRemove)
{
    loadDocument(HOST_ONLY_DOC);
    auto content = Content::create(EMBEDDED_DYNAMIC_WITH_SEND_EVENT, makeDefaultSession());
    ASSERT_TRUE(content->isReady());
    documentManager->succeed("embeddedDocumentUrl", content, true);

    advanceTime(1500);

    ASSERT_TRUE(root->hasEvent());
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    // Remove embedded doc
    std::map<std::string, Object> properties = {};
    properties.emplace("componentId", "hostComponent");
    executeCommand("RemoveItem", properties, false);

    advanceTime(500);

    ASSERT_FALSE(root->hasEvent());

    root->clearDirty();
}

TEST_F(RootContextTargetingTest, VerifyUniqueComponentIds)
{
    loadDocument(HOST_ONLY_DOC);
    auto content = Content::create(EMBEDDED_DEFAULT, makeDefaultSession());
    ASSERT_TRUE(content->isReady());
    documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));
    auto rootComp = component->findComponentById("hostComponent", true);
    ASSERT_NE(component->getUniqueId(), rootComp->getUniqueId());
    auto text = component->findComponentById("embeddedText", true);
    ASSERT_NE(component->getUniqueId(), text->getUniqueId());
    ASSERT_NE(component->getUniqueId(), rootComp->getUniqueId());

    // Can search for any UID from RootContext API
    ASSERT_EQ(component.get(), root->findByUniqueId(component->getUniqueId()));
    ASSERT_EQ(text.get(), root->findByUniqueId(text->getUniqueId()));
}

static const char* EMBEDDED_KEY_HANDLER = R"({
  "type": "APL",
  "version": "2023.2",
  "handleKeyUp": [
    {
      "when": "${event.keyboard.code == 'KeyG'}",
      "commands": [
        {
          "type": "SendEvent",
          "arguments": ["GREEN"]
        }
      ]
    }
  ],
  "handleKeyDown": [
    {
      "when": "${event.keyboard.code == 'KeyB'}",
      "commands": [
        {
          "type": "SendEvent",
          "arguments": ["BLUE"]
        }
      ]
    }
  ],
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "id": "testFrame",
      "backgroundColor": "red"
    }
  }
})";

TEST_F(RootContextTargetingTest, FocusedHostDocumentKeyboard)
{
    loadDocument(HOST_ONLY_DOC);
    auto content = Content::create(EMBEDDED_KEY_HANDLER, makeDefaultSession());
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    advanceTime(10);

    root->setFocus(FocusDirection::kFocusDirectionNone, Rect(0,0,10,10), "hostComponent");

    root->popEvent().getActionRef().resolve();
    root->clearPending();
    root->clearDirty();

    // send valid key down
    root->handleKeyboard(kKeyDown, Keyboard("KeyB", "b"));
    // verify down command was executed
    ASSERT_TRUE(CheckSendEvent(root, "BLUE"));

    // send valid key up
    root->handleKeyboard(kKeyUp, Keyboard("KeyG", "g"));
    // verify up command was executed
    ASSERT_TRUE(CheckSendEvent(root, "GREEN"));
}

static const char* HOST_WITH_KEYBOARD_ONLY_DOC = R"({
  "type": "APL",
  "version": "2023.1",
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "item": {
        "type": "Host",
        "id": "hostComponent",
        "source": "embeddedDocumentUrl",
        "onLoad": {
          "type": "SendEvent",
          "sequencer": "SEND_EVENT",
          "arguments": ["LOADED"]
        },
        "onFail": {
          "type": "SendEvent",
          "sequencer": "SEND_EVENT",
          "arguments": ["FAILED"]
        },
        "handleKeyUp": [
          {
            "when": "${event.keyboard.code == 'KeyG'}",
            "commands": [
              {
                "type": "SendEvent",
                "arguments": ["GARBAGE"]
              }
            ]
          }
        ],
        "handleKeyDown": [
          {
            "when": "${event.keyboard.code == 'KeyB'}",
            "commands": [
              {
                "type": "SendEvent",
                "arguments": ["BLUEBERRY"]
              }
            ]
          }
        ]
      }
    }
  }
})";

TEST_F(RootContextTargetingTest, FocusedHostComponentKeyboard)
{
    loadDocument(HOST_WITH_KEYBOARD_ONLY_DOC);
    auto content = Content::create(EMBEDDED_DYNAMIC_WITH_SEND_EVENT, makeDefaultSession());
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    advanceTime(1000);

    ASSERT_TRUE(CheckSendEvent(root, "EMBEDDED"));

    root->setFocus(FocusDirection::kFocusDirectionNone, Rect(0,0,10,10), "hostComponent");

    root->popEvent().getActionRef().resolve();
    root->clearPending();
    root->clearDirty();

    // send valid key down
    root->handleKeyboard(kKeyDown, Keyboard("KeyB", "b"));
    // verify down command was executed
    ASSERT_TRUE(CheckSendEvent(root, "BLUEBERRY"));

    // send valid key up
    root->handleKeyboard(kKeyUp, Keyboard("KeyG", "g"));
    // verify up command was executed
    ASSERT_TRUE(CheckSendEvent(root, "GARBAGE"));
}

static const char* EMBEDDED_KEY_HANDLER_PROPAGATE = R"({
  "type": "APL",
  "version": "2023.2",
  "handleKeyUp": [
    {
      "when": "${event.keyboard.code == 'KeyG'}",
      "propagate": true,
      "commands": [
        {
          "type": "SendEvent",
          "arguments": ["GREEN"]
        }
      ]
    }
  ],
  "handleKeyDown": [
    {
      "when": "${event.keyboard.code == 'KeyB'}",
      "propagate": true,
      "commands": [
        {
          "type": "SendEvent",
          "arguments": ["BLUE"]
        }
      ]
    }
  ],
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "id": "testFrame",
      "backgroundColor": "red"
    }
  }
})";

TEST_F(RootContextTargetingTest, FocusedHostPropagatedKeyboard)
{
    loadDocument(HOST_WITH_KEYBOARD_ONLY_DOC);
    auto content = Content::create(EMBEDDED_KEY_HANDLER_PROPAGATE, makeDefaultSession());
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    advanceTime(1000);

    root->setFocus(FocusDirection::kFocusDirectionNone, Rect(0,0,10,10), "hostComponent");

    root->popEvent().getActionRef().resolve();
    root->clearPending();
    root->clearDirty();

    // send valid key down
    root->handleKeyboard(kKeyDown, Keyboard("KeyB", "b"));
    // verify down command was executed
    ASSERT_TRUE(CheckSendEvent(root, "BLUE"));
    ASSERT_TRUE(CheckSendEvent(root, "BLUEBERRY"));

    // send valid key up
    root->handleKeyboard(kKeyUp, Keyboard("KeyG", "g"));
    // verify up command was executed
    ASSERT_TRUE(CheckSendEvent(root, "GREEN"));
    ASSERT_TRUE(CheckSendEvent(root, "GARBAGE"));
}

static const char* EMBEDDED_KEY_HANDLER_PROPAGATE_DEEPER = R"({
  "type": "APL",
  "version": "2023.2",
  "handleKeyUp": [
    {
      "when": "${event.keyboard.code == 'KeyG'}",
      "propagate": true,
      "commands": [
        {
          "type": "SendEvent",
          "arguments": ["GREEN"]
        }
      ]
    }
  ],
  "handleKeyDown": [
    {
      "when": "${event.keyboard.code == 'KeyB'}",
      "propagate": true,
      "commands": [
        {
          "type": "SendEvent",
          "arguments": ["BLUE"]
        }
      ]
    }
  ],
  "mainTemplate": {
    "items": {
      "type": "TouchWrapper",
      "id": "INTERNALTW",
      "width": "100%",
      "height": "100%",
      "item": {
        "type": "Frame",
        "id": "testFrame",
        "backgroundColor": "red"
      }
    }
  }
})";

TEST_F(RootContextTargetingTest, FocusedHostPropagatedDeeperKeyboard)
{
    loadDocument(HOST_WITH_KEYBOARD_ONLY_DOC);
    auto content = Content::create(EMBEDDED_KEY_HANDLER_PROPAGATE_DEEPER, makeDefaultSession());
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    advanceTime(1000);

    root->setFocus(FocusDirection::kFocusDirectionNone, Rect(0,0,10,10), "INTERNALTW");

    root->popEvent().getActionRef().resolve();
    root->clearPending();
    root->clearDirty();

    // send valid key down
    root->handleKeyboard(kKeyDown, Keyboard("KeyB", "b"));
    // verify down command was executed
    ASSERT_TRUE(CheckSendEvent(root, "BLUE"));
    ASSERT_TRUE(CheckSendEvent(root, "BLUEBERRY"));

    // send valid key up
    root->handleKeyboard(kKeyUp, Keyboard("KeyG", "g"));
    // verify up command was executed
    ASSERT_TRUE(CheckSendEvent(root, "GREEN"));
    ASSERT_TRUE(CheckSendEvent(root, "GARBAGE"));
}
