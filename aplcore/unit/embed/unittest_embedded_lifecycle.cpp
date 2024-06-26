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

using namespace apl;

class EmbeddedLifecycleTest : public DocumentWrapper {
public:
    EmbeddedLifecycleTest()
        : documentManager(std::make_shared<TestDocumentManager>())
    {
        config->documentManager(std::static_pointer_cast<DocumentManager>(documentManager));
    }

protected:
    std::shared_ptr<TestDocumentManager> documentManager;
};

static const char* HOST_DOC = R"({
  "type": "APL",
  "version": "2023.1",
  "onConfigChange": {
    "type": "Reinflate"
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "entities": "ROOT",
      "id": "top",
      "item": {
        "type": "Host",
        "width": "100%",
        "height": "100%",
        "id": "hostComponent",
        "entities": "HOST",
        "source": "embeddedDocumentUrl",
        "onLoad": [
          {
            "type": "SendEvent",
            "sequencer": "SEND_EVENTER",
            "arguments": ["LOADED"]
          }
        ],
        "onFail": [
          {
            "type": "InsertItem",
            "sequencer": "SEND_EVENTER",
            "arguments": ["FAILED"]
          }
        ]
      }
    }
  }
})";

static const char* EMBEDDED_DOC = R"({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "embeddedText",
      "text": "Hello, World!",
      "entities": "EMBEDDED"
    }
  }
})";

static const char* PSEUDO_LOG_COMMAND = R"apl([
  {
    "type": "PseudoLog"
  }
])apl";

TEST_F(EmbeddedLifecycleTest, SimpleLoad)
{
    // Host document inflates
    session = std::make_shared<TestSession>();
    loadDocument(HOST_DOC);

    // While it inflates embedded document requested.
    auto requestWeak = documentManager->get("embeddedDocumentUrl");
    ASSERT_TRUE(requestWeak.lock());
    auto request = requestWeak.lock();
    ASSERT_EQ(request->getUrlRequest().getUrl(), "embeddedDocumentUrl");

    // When document retrieved - create content with new session (console session management is up
    // to runtime/viewhost)
    auto embeddedSession = std::make_shared<TestSession>();
    auto content = Content::create(EMBEDDED_DOC, embeddedSession);
    // Load any packages if required and check if ready.
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    // Document causes update in DOM
    auto host = component->getCoreChildAt(0);
    ASSERT_EQ(1, host->getChildCount());
    auto update = std::vector<ObjectMap>{
        ObjectMap{{"index", 0}, {"uid", host->getCoreChildAt(0)->getUniqueId()}, {"action", "insert"}}
    };
    ASSERT_TRUE(CheckUpdatedChildrenNotification(root, host, update));
    ASSERT_TRUE(CheckChildLaidOut(host, 0, true));

    // Required by VH code in order to do dynamic dom changes.
    ASSERT_EQ(host, CoreDocumentContext::cast(embeddedDocumentContext)->topComponent()->getParent());

    // We can send commands to the root doc
    auto cmd = JsonData(PSEUDO_LOG_COMMAND);
    ASSERT_TRUE(cmd);

    ASSERT_TRUE(session->getCount() == 0);
    rootDocument->executeCommands(cmd.get(), false);
    ASSERT_TRUE(session->checkAndClear());

    ASSERT_TRUE(embeddedSession->getCount() == 0);
    embeddedDocumentContext->executeCommands(cmd.get(), false);
    ASSERT_TRUE(embeddedSession->checkAndClear());
}

TEST_F(EmbeddedLifecycleTest, DoubleResolve)
{
    // Host document inflates
    session = std::make_shared<TestSession>();
    loadDocument(HOST_DOC);

    // When document retrieved - create content with new session (console session management is up
    // to runtime/viewhost)
    auto embeddedSession = std::make_shared<TestSession>();
    auto content = Content::create(EMBEDDED_DOC, embeddedSession);
    // Load any packages if required and check if ready.
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext1 = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext1);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    // And again
    ASSERT_FALSE(documentManager->succeed("embeddedDocumentUrl", content, true));
}

static const char* EMBEDDED_DEEPER_DOC = R"({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "height": "100%",
      "width": "100%",
      "items": [
        {
          "type": "Text",
          "id": "embeddedText1",
          "text": "Hello, World!"
        },
        {
          "type": "Text",
          "height": 200,
          "width": 200,
          "id": "embeddedText2",
          "text": "Hello, World!"
        }
      ]
    }
  }
})";

TEST_F(EmbeddedLifecycleTest, EmbeddedTextMeasurement)
{
    // Host document inflates
    loadDocument(HOST_DOC);

    // While it inflates embedded document requested.
    auto requestWeak = documentManager->get("embeddedDocumentUrl");
    ASSERT_TRUE(requestWeak.lock());
    auto request = requestWeak.lock();
    ASSERT_EQ(request->getUrlRequest().getUrl(), "embeddedDocumentUrl");

    // When document retrieved - create content with new session (console session management is up
    // to runtime/viewhost)
    auto content = Content::create(EMBEDDED_DEEPER_DOC, session);
    // Load any packages if required and check if ready.
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    // Document causes update in DOM
    auto host = component->getCoreChildAt(0);
    ASSERT_TRUE(host);

    auto text1 = root->findComponentById("embeddedText1");
    ASSERT_TRUE(text1);
    ASSERT_EQ(Rect(0,0,1024,10), text1->getCalculated(kPropertyBounds).get<Rect>());

    auto text2 = root->findComponentById("embeddedText2");
    ASSERT_TRUE(text2);
    ASSERT_EQ(Rect(0,10,200,200), text2->getCalculated(kPropertyBounds).get<Rect>());
}

static const char* EMBEDDED_PAGER_DOC = R"({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Pager",
      "height": "100%",
      "width": "100%",
      "items": [
        {
          "type": "Text",
          "id": "embeddedText1",
          "text": "Hello, World!"
        },
        {
          "type": "Text",
          "height": 200,
          "width": 200,
          "id": "embeddedText2",
          "text": "Hello, World!"
        }
      ]
    }
  }
})";

TEST_F(EmbeddedLifecycleTest, EmbeddedTextPager)
{
    // Host document inflates
    loadDocument(HOST_DOC);

    // While it inflates embedded document requested.
    auto requestWeak = documentManager->get("embeddedDocumentUrl");
    ASSERT_TRUE(requestWeak.lock());
    auto request = requestWeak.lock();
    ASSERT_EQ(request->getUrlRequest().getUrl(), "embeddedDocumentUrl");

    // When document retrieved - create content with new session (console session management is up
    // to runtime/viewhost)
    auto content = Content::create(EMBEDDED_PAGER_DOC, session);
    // Load any packages if required and check if ready.
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    // Document causes update in DOM
    auto host = component->getCoreChildAt(0);
    ASSERT_TRUE(host);

    auto text1 = root->findComponentById("embeddedText1");
    ASSERT_TRUE(text1);
    ASSERT_EQ(Rect(0,0,1024,800), text1->getCalculated(kPropertyBounds).get<Rect>());

    auto text2 = root->findComponentById("embeddedText2");
    ASSERT_TRUE(text2);
    ASSERT_EQ(Rect(0,0,1024,800), text2->getCalculated(kPropertyBounds).get<Rect>());
}

static const char* EMBEDDED_SEND_EVENT_MOUNT_DOC = R"({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "embeddedText1",
      "text": "Hello, World!",
      "onMount": {
        "type": "SendEvent",
        "delay": 1000,
        "sequencer": "COMPONENT_MOUNT",
        "arguments": ["EMBEDDED_COMPONENT"]
      }
    }
  },
  "onMount": {
    "type": "SendEvent",
    "delay": 500,
    "sequencer": "DOCUMENT_MOUNT",
    "arguments": ["DOCUMENT"]
  }
})";

TEST_F(EmbeddedLifecycleTest, EmbeddedSendEventTagging)
{
    // Host document inflates
    loadDocument(HOST_DOC);

    // While it inflates embedded document requested.
    auto requestWeak = documentManager->get("embeddedDocumentUrl");
    ASSERT_TRUE(requestWeak.lock());
    auto request = requestWeak.lock();
    ASSERT_EQ(request->getUrlRequest().getUrl(), "embeddedDocumentUrl");

    // When document retrieved - create content with new session (console session management is up
    // to runtime/viewhost)
    auto content = Content::create(EMBEDDED_SEND_EVENT_MOUNT_DOC, session);
    // Load any packages if required and check if ready.
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);

    // Check that first SendEvent is load success and tagged by host doc
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(event.getType(), kEventTypeSendEvent);
    ASSERT_EQ(event.getValue(apl::kEventPropertyArguments).at(0).getString(), "LOADED");
    ASSERT_EQ(rootDocument, event.getDocument());

    advanceTime(500);

    // Embedded doc fires it's onmount
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(event.getType(), kEventTypeSendEvent);
    ASSERT_EQ(event.getValue(apl::kEventPropertyArguments).at(0).getString(), "DOCUMENT");
    ASSERT_EQ(embeddedDocumentContext, event.getDocument());

    advanceTime(1000);

    // Embedded doc component fires it's onmount
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(event.getType(), kEventTypeSendEvent);
    ASSERT_EQ(event.getValue(apl::kEventPropertyArguments).at(0).getString(), "EMBEDDED_COMPONENT");
    ASSERT_EQ(embeddedDocumentContext, event.getDocument());
}

static const char* EMBEDDED_OPEN_URL_MOUNT_DOC = R"({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "embeddedText1",
      "text": "Hello, World!"
    }
  },
  "onMount": {
    "type": "OpenURL",
    "delay": 500,
    "source": "SOURCE"
  }
})";

TEST_F(EmbeddedLifecycleTest, EmbeddedOpenUrlTagging)
{
    config->set(RootProperty::kAllowOpenUrl, true);
    // Host document inflates
    loadDocument(HOST_DOC);

    // While it inflates embedded document requested.
    auto requestWeak = documentManager->get("embeddedDocumentUrl");
    ASSERT_TRUE(requestWeak.lock());
    auto request = requestWeak.lock();
    ASSERT_EQ(request->getUrlRequest().getUrl(), "embeddedDocumentUrl");

    // When document retrieved - create content with new session (console session management is up
    // to runtime/viewhost)
    auto content = Content::create(EMBEDDED_OPEN_URL_MOUNT_DOC, session);
    // Load any packages if required and check if ready.
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);

    // Check that first SendEvent is load success and tagged by host doc
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(event.getType(), kEventTypeSendEvent);
    ASSERT_EQ(event.getValue(apl::kEventPropertyArguments).at(0).getString(), "LOADED");
    ASSERT_EQ(rootDocument, event.getDocument());

    advanceTime(500);

    // Embedded doc fires it's onmount
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(event.getType(), kEventTypeOpenURL);
    ASSERT_EQ(event.getValue(apl::kEventPropertySource).getString(), "SOURCE");
    ASSERT_EQ(embeddedDocumentContext, event.getDocument());
}

TEST_F(EmbeddedLifecycleTest, Finish) {
    loadDocument(HOST_DOC);

    auto content = Content::create(EMBEDDED_DOC, session);
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    root->clearDirty();

    // We can send commands to the root doc
    auto cmd = JsonData(R"([{"type": "Finish"}])");
    ASSERT_TRUE(cmd);

    // Finish ignored by embedded doc
    embeddedDocumentContext->executeCommands(cmd.get(), false);
    ASSERT_FALSE(root->hasEvent());

    // But not by host
    rootDocument->executeCommands(cmd.get(), false);
    ASSERT_TRUE(root->hasEvent());
    ASSERT_EQ(kEventTypeFinish, root->popEvent().getType());
}

TEST_F(EmbeddedLifecycleTest, EmbeddedDocCommandCancelExecution) {
    loadDocument(HOST_DOC);

    auto content = Content::create(EMBEDDED_DOC, session);
    ASSERT_TRUE(content->isReady());

    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    root->clearDirty();

    auto cmd = JsonData(R"([{
      "type": "AnimateItem",
      "componentId": "embeddedText",
      "duration": "3000",
      "easing": "linear",
      "value": [
        {
          "property": "opacity",
          "to": "0.0"
        }
      ]
    }])");
    ASSERT_TRUE(cmd);

    auto command = embeddedDocumentContext->executeCommands(cmd.get(), false);
    root->cancelExecution();
    ASSERT_TRUE(command->isTerminated());
}

const static char *PARENT_VC = R"({
  "children": [
    {
      "entities": [
        "HOST"
      ],
      "tags": {
        "focused": false,
        "embedded": {
          "attached": false,
          "source": "embeddedDocumentUrl"
        }
      },
      "id": "hostComponent",
      "uid": "HOSTID",
      "position": "1024x800+0+0:0",
      "type": "empty"
    }
  ],
  "entities": [
    "ROOT"
  ],
  "tags": {
    "viewport": {}
  },
  "id": "top",
  "uid": "ROOTID",
  "position": "1024x800+0+0:0",
  "type": "empty"
})";

const static char *EMBEDDED_VC = R"({
  "entities": [
    "EMBEDDED"
  ],
  "tags": {
    "viewport": {}
  },
  "id": "embeddedText",
  "uid": "EMBEDDEDID",
  "position": "1024x800+0+0:0",
  "type": "text"
})";

TEST_F(EmbeddedLifecycleTest, VisualContextDetached)
{
    loadDocument(HOST_DOC);

    auto content = Content::create(EMBEDDED_DOC, session);
    ASSERT_TRUE(content->isReady());

    // Host and embedded documents have different origin.
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, false);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    root->clearDirty();

    rapidjson::Document document(rapidjson::kObjectType);
    auto rootVisualContextDocument = rootDocument->serializeVisualContext(document.GetAllocator());

    // Load the JSON result above
    auto parentVC = std::string(PARENT_VC);
    parentVC = std::regex_replace(parentVC, std::regex("ROOTID"), component->getUniqueId());
    parentVC = std::regex_replace(parentVC, std::regex("HOSTID"), component->getChildAt(0)->getUniqueId());

    rapidjson::Document result;
    rapidjson::ParseResult ok = result.Parse(parentVC.c_str());
    ASSERT_TRUE(ok);

    ASSERT_TRUE(rootVisualContextDocument == result);

    auto embeddedContextDocument = embeddedDocumentContext->serializeVisualContext(document.GetAllocator());

    // Load the JSON result above
    auto embeddedVc = std::string(EMBEDDED_VC);
    embeddedVc = std::regex_replace(embeddedVc, std::regex("EMBEDDEDID"), component->getChildAt(0)->getChildAt(0)->getUniqueId());

    ok = result.Parse(embeddedVc.c_str());
    ASSERT_TRUE(ok);

    ASSERT_TRUE(embeddedContextDocument == result);
}

const static char *FULL_VC = R"({
  "children":
    [
      {
        "children": [
          {
            "entities": [
              "EMBEDDED"
            ],
            "id": "embeddedText",
            "uid": "EMBEDDEDID",
            "position": "1024x800+0+0:0",
            "type": "text"
          }
        ],
        "entities": [
          "HOST"
        ],
        "tags": {
          "focused": false,
          "embedded": {
            "attached": true,
            "source": "embeddedDocumentUrl"
          }
        },
        "id": "hostComponent",
        "uid": "HOSTID",
        "position": "1024x800+0+0:0",
        "type": "text"
      }
    ],
    "entities": [
      "ROOT"
    ],
    "tags": {
    "viewport": {}
  },
  "id": "top",
  "uid": "ROOTID",
  "position": "1024x800+0+0:0",
  "type": "text"
})";

TEST_F(EmbeddedLifecycleTest, VisualContextAttached)
{
    loadDocument(HOST_DOC);

    // While it inflates embedded document requested.
    auto requestWeak = documentManager->get("embeddedDocumentUrl");
    ASSERT_TRUE(requestWeak.lock());
    auto request = requestWeak.lock();
    ASSERT_EQ(request->getUrlRequest().getUrl(), "embeddedDocumentUrl");

    auto content = Content::create(EMBEDDED_DOC, session);
    ASSERT_TRUE(content->isReady());

    // Check that origin is host document
    ASSERT_EQ(rootDocument, request->getOrigin());

    // Host and embedded documents have same origin.
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    root->clearDirty();

    rapidjson::Document document(rapidjson::kObjectType);
    auto foolVisualContextDocument = rootDocument->serializeVisualContext(document.GetAllocator());

    auto fullVc = std::string(FULL_VC);
    fullVc = std::regex_replace(fullVc, std::regex("ROOTID"), component->getUniqueId());
    fullVc = std::regex_replace(fullVc, std::regex("HOSTID"), component->getChildAt(0)->getUniqueId());
    fullVc = std::regex_replace(fullVc, std::regex("EMBEDDEDID"), component->getChildAt(0)->getChildAt(0)->getUniqueId());

    rapidjson::Document result;
    rapidjson::ParseResult ok = result.Parse(fullVc.c_str());
    ASSERT_TRUE(ok);

    ASSERT_TRUE(foolVisualContextDocument == result);
}

static const char* EMBEDDED_DOC_TIMED = R"({
  "type": "APL",
  "version": "2023.2",
  "onConfigChange": {
    "type": "Reinflate"
  },
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "embeddedText",
      "text": "${utcTime}"
    }
  }
})";

TEST_F(EmbeddedLifecycleTest, TimeUpdatesPropagation)
{
    // Host document inflates
    loadDocument(HOST_DOC);

    auto content = Content::create(EMBEDDED_DOC_TIMED, session);

    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    auto text = root->findComponentById("embeddedText");
    ASSERT_EQ("0", text->getCalculated(apl::kPropertyText).asString());

    advanceTime(100);

    ASSERT_EQ("100", text->getCalculated(apl::kPropertyText).asString());
}

static const char* HOST_DOC_DOUBLE = R"({
  "type": "APL",
  "version": "2023.1",
  "onConfigChange": {
    "type": "Reinflate"
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "entities": "ROOT",
      "id": "top",
      "items": [
        {
          "type": "Host",
          "width": "50%",
          "height": "50%",
          "id": "hostComponent1",
          "entities": "HOST",
          "source": "embeddedDocumentUrl",
          "onLoad": [
            {
              "type": "SendEvent",
              "sequencer": "SEND_EVENTER",
              "arguments": ["LOADED1"]
            }
          ]
        },
        {
          "type": "Host",
          "width": "50%",
          "height": "50%",
          "id": "hostComponent2",
          "entities": "HOST",
          "source": "embeddedDocumentUrl",
          "onLoad": [
            {
              "type": "SendEvent",
              "sequencer": "SEND_EVENTER",
              "arguments": ["LOADED2"]
            }
          ]
        }
      ]
    }
  }
})";

// Content should be reusable, even behind the same source.
TEST_F(EmbeddedLifecycleTest, ContentAndSourceReuse)
{
    // Host document inflates
    loadDocument(HOST_DOC_DOUBLE);

    auto content = Content::create(EMBEDDED_DOC, session);
    ASSERT_TRUE(content->isReady());

    auto embeddedDocumentContext1 = documentManager->succeed("embeddedDocumentUrl", content, true, DocumentConfig::create(), true);
    ASSERT_TRUE(embeddedDocumentContext1);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED1"));

    auto embeddedDocumentContext2 = documentManager->succeed("embeddedDocumentUrl", content, true, DocumentConfig::create(), true);
    ASSERT_TRUE(embeddedDocumentContext2);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED2"));
}

static const char* SINGLE_HOST_DOC = R"({
  "type": "APL",
  "version": "2023.1",
  "onConfigChange": {
    "type": "Reinflate"
  },
  "mainTemplate": {
    "item": {
      "type": "Host",
      "width": "100%",
      "height": "100%",
      "id": "hostComponent",
      "entities": "HOST",
      "source": "embeddedDocumentUrl",
      "onLoad": [
        {
          "type": "SendEvent",
          "sequencer": "SEND_EVENTER",
          "arguments": ["LOADED"]
        }
      ]
    }
  }
})";

TEST_F(EmbeddedLifecycleTest, SingleHost)
{
    // Host document inflates
    loadDocument(SINGLE_HOST_DOC);

    auto content = Content::create(EMBEDDED_DOC, session);
    ASSERT_TRUE(content->isReady());

    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true, DocumentConfig::create(), true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));
}

TEST_F(EmbeddedLifecycleTest, ChangeSourceAfterDocumentLoaded)
{
    loadDocument(HOST_DOC);

    // Host component has no children at the beginning
    auto host = component->getCoreChildAt(0);
    ASSERT_EQ(0, host->getChildCount());

    auto content = Content::create(EMBEDDED_DOC, session);
    ASSERT_TRUE(content->isReady());

    auto embeddedDocument = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocument);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    // Now there is one child (due to embedded document Text component)
    ASSERT_EQ(1, host->getChildCount());

    auto text = root->findComponentById("embeddedText");
    ASSERT_EQ("Hello, World!", text->getCalculated(kPropertyText).asString());

    // Change the source to something else
    auto action = executeCommand(
        rootDocument,
        "SetValue", {
                        {"componentId", "hostComponent"},
                        {"property", "source"},
                        {"value", "anotherEmbeddedDocumentUrl"}
                    }, false);

    // Back to no children (Host is empty)
    ASSERT_EQ(0, host->getChildCount());

    auto embeddedDocument2 = documentManager->succeed("anotherEmbeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocument2);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    // Again there is one child (due to embedded document Text component)
    ASSERT_EQ(1, host->getChildCount());
}

TEST_F(EmbeddedLifecycleTest, ChangeSourceBeforeDocumentLoaded) {
    loadDocument(HOST_DOC);

    // Host component has no children at the beginning
    auto host = component->getCoreChildAt(0);
    ASSERT_EQ(0, host->getChildCount());

    // Change the source to something else
    auto action = executeCommand(rootDocument, "SetValue",
                                 {{"componentId", "hostComponent"},
                                  {"property", "source"},
                                  {"value", "anotherEmbeddedDocumentUrl"}},
                                 false);

    auto content = Content::create(EMBEDDED_DOC, session);
    ASSERT_TRUE(content->isReady());

    // Original request is no longer needed
    auto embeddedDocument = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_FALSE(embeddedDocument);
    ASSERT_FALSE(CheckSendEvent(root, "LOADED"));

    // Still no children
    ASSERT_EQ(0, host->getChildCount());
    auto text = root->findComponentById("embeddedText");
    ASSERT_FALSE(text);

    auto embeddedDocument2 = documentManager->succeed("anotherEmbeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocument2);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    // Now there's one child (due to embedded document Text component)
    ASSERT_EQ(1, host->getChildCount());
    text = root->findComponentById("embeddedText");
    ASSERT_TRUE(text);
    ASSERT_EQ("Hello, World!", text->getCalculated(kPropertyText).asString());
}

static const char* CUSTOM_EMBEDDED_ENV = R"({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "embeddedText",
      "text": "${environment.magic}"
    }
  }
})";

TEST_F(EmbeddedLifecycleTest, CustomEnv)
{
    // Host document inflates
    loadDocument(SINGLE_HOST_DOC);

    auto content = Content::create(CUSTOM_EMBEDDED_ENV, session);
    ASSERT_TRUE(content->isReady());

    auto documentConfig = DocumentConfig::create();
    documentConfig->setEnvironmentValue("magic", "Very magic.");

    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true, documentConfig, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    ASSERT_EQ("Very magic.", root->findComponentById("embeddedText")->getCalculated(apl::kPropertyText).asString());
}

static const char* HOST_DOC_AUTO = R"({
  "type": "APL",
  "version": "2023.1",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": "100%",
      "height": "100%",
      "item": {
        "type": "Host",
        "width": "auto",
        "height": "auto",
        "id": "hostComponent",
        "entities": "HOST",
        "source": "embeddedDocumentUrl",
        "onLoad": [
          {
            "type": "SendEvent",
            "sequencer": "SEND_EVENTER",
            "arguments": ["LOADED"]
          }
        ]
      }
    }
  }
})";

static const char* FIXED_EMBEDDED_DOC = R"({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "width": 300,
      "height": 300,
      "id": "embeddedText",
      "text": "Hello, World!",
      "entities": "EMBEDDED"
    }
  }
})";

TEST_F(EmbeddedLifecycleTest, AutoSizedEmbedded)
{
    loadDocument(HOST_DOC_AUTO);

    // While it inflates embedded document requested.
    auto requestWeak = documentManager->get("embeddedDocumentUrl");
    ASSERT_TRUE(requestWeak.lock());
    auto request = requestWeak.lock();
    ASSERT_EQ(request->getUrlRequest().getUrl(), "embeddedDocumentUrl");

    // When document retrieved - create content with new session (console session management is up
    // to runtime/viewhost)
    auto embeddedSession = std::make_shared<TestSession>();
    auto content = Content::create(FIXED_EMBEDDED_DOC, embeddedSession);
    // Load any packages if required and check if ready.
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    ASSERT_TRUE(CheckComponent(root->findComponentById("embeddedText"), 300, 300));
    ASSERT_TRUE(CheckComponent(root->findComponentById("hostComponent"), 300, 300));
    ASSERT_TRUE(CheckComponent(component, 1024, 800));
    ASSERT_TRUE(CheckViewport(root, 1024, 800));

    // Change size directly
    auto action = executeCommand(
        embeddedDocumentContext,
        "SetValue", {
                        {"componentId", "embeddedText"},
                        {"property", "width"},
                        {"value", 200}
                    }, false);

    advanceTime(100);

    ASSERT_TRUE(CheckComponent(root->findComponentById("embeddedText"), 200, 300));
    ASSERT_TRUE(CheckComponent(root->findComponentById("hostComponent"), 200, 300));
    ASSERT_TRUE(CheckComponent(component, 1024, 800));
    ASSERT_TRUE(CheckViewport(root, 1024, 800));
}

static const char* AUTO_EMBEDDED_DOC = R"({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "width": "auto",
      "height": "auto",
      "id": "embeddedText",
      "text": "Hello, World!"
    }
  }
})";

TEST_F(EmbeddedLifecycleTest, AutoSizedAutoEmbedded)
{
    loadDocument(HOST_DOC_AUTO);

    // While it inflates embedded document requested.
    auto requestWeak = documentManager->get("embeddedDocumentUrl");
    ASSERT_TRUE(requestWeak.lock());
    auto request = requestWeak.lock();
    ASSERT_EQ(request->getUrlRequest().getUrl(), "embeddedDocumentUrl");

    // When document retrieved - create content with new session (console session management is up
    // to runtime/viewhost)
    auto embeddedSession = std::make_shared<TestSession>();
    auto content = Content::create(AUTO_EMBEDDED_DOC, embeddedSession);
    // Load any packages if required and check if ready.
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    ASSERT_TRUE(CheckComponent(root->findComponentById("embeddedText"), 130, 10));
    ASSERT_TRUE(CheckComponent(root->findComponentById("hostComponent"), 130, 10));
    ASSERT_TRUE(CheckComponent(component, 1024, 800));
    ASSERT_TRUE(CheckViewport(root, 1024, 800));


    executeCommand(
        embeddedDocumentContext,
        "SetValue", {
                        {"componentId", "embeddedText"},
                        {"property", "text"},
                        {"value", "Hello, World! Maybe, not sure yet."}
                    }, false);

    advanceTime(100);

    ASSERT_TRUE(CheckComponent(root->findComponentById("embeddedText"), 340, 10));
    ASSERT_TRUE(CheckComponent(root->findComponentById("hostComponent"), 340, 10));
    ASSERT_TRUE(CheckComponent(component, 1024, 800));
    ASSERT_TRUE(CheckViewport(root, 1024, 800));
}

static const char* HOST_DOC_AUTO_MINMAX_WIDTH = R"({
  "type": "APL",
  "version": "2023.1",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": "100%",
      "height": "100%",
      "item": {
        "type": "Host",
        "width": "auto",
        "minWidth": 100,
        "maxWidth": 200,
        "height": "auto",
        "id": "hostComponent",
        "entities": "HOST",
        "source": "embeddedDocumentUrl",
        "onLoad": [
          {
            "type": "SendEvent",
            "sequencer": "SEND_EVENTER",
            "arguments": ["LOADED"]
          }
        ]
      }
    }
  }
})";

TEST_F(EmbeddedLifecycleTest, AutoSizedEmbeddedMinMaxWidth)
{
    loadDocument(HOST_DOC_AUTO_MINMAX_WIDTH);

    // While it inflates embedded document requested.
    auto requestWeak = documentManager->get("embeddedDocumentUrl");
    ASSERT_TRUE(requestWeak.lock());
    auto request = requestWeak.lock();
    ASSERT_EQ(request->getUrlRequest().getUrl(), "embeddedDocumentUrl");

    // When document retrieved - create content with new session (console session management is up
    // to runtime/viewhost)
    auto embeddedSession = std::make_shared<TestSession>();
    auto content = Content::create(AUTO_EMBEDDED_DOC, embeddedSession);
    // Load any packages if required and check if ready.
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    ASSERT_TRUE(CheckComponent(root->findComponentById("embeddedText"), 130, 10));
    ASSERT_TRUE(CheckComponent(root->findComponentById("hostComponent"), 130, 10));
    ASSERT_TRUE(CheckComponent(component, 1024, 800));
    ASSERT_TRUE(CheckViewport(root, 1024, 800));


    executeCommand(
        embeddedDocumentContext,
        "SetValue", {
                        {"componentId", "embeddedText"},
                        {"property", "text"},
                        {"value", "Hello"}
                    }, false);

    advanceTime(100);

    ASSERT_TRUE(CheckComponent(root->findComponentById("embeddedText"), 100, 10));
    ASSERT_TRUE(CheckComponent(root->findComponentById("hostComponent"), 100, 10));
    ASSERT_TRUE(CheckComponent(component, 1024, 800));
    ASSERT_TRUE(CheckViewport(root, 1024, 800));


    executeCommand(
        embeddedDocumentContext,
        "SetValue", {
                        {"componentId", "embeddedText"},
                        {"property", "text"},
                        {"value", "Hello, World! Maybe, not sure yet."}
                    }, false);

    advanceTime(100);

    ASSERT_TRUE(CheckComponent(root->findComponentById("embeddedText"), 200, 20));
    ASSERT_TRUE(CheckComponent(root->findComponentById("hostComponent"), 200, 20));
    ASSERT_TRUE(CheckComponent(component, 1024, 800));
    ASSERT_TRUE(CheckViewport(root, 1024, 800));
}

static const char* HOST_DOC_AUTO_MINMAX_HEIGHT = R"({
  "type": "APL",
  "version": "2023.1",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": "100%",
      "height": "100%",
      "item": {
        "type": "Host",
        "width": 50,
        "height": "auto",
        "minHeight": 20,
        "maxHeight": 60,
        "id": "hostComponent",
        "entities": "HOST",
        "source": "embeddedDocumentUrl",
        "onLoad": [
          {
            "type": "SendEvent",
            "sequencer": "SEND_EVENTER",
            "arguments": ["LOADED"]
          }
        ]
      }
    }
  }
})";

TEST_F(EmbeddedLifecycleTest, AutoSizedEmbeddedMinMaxHeight)
{
    loadDocument(HOST_DOC_AUTO_MINMAX_HEIGHT);

    // While it inflates embedded document requested.
    auto requestWeak = documentManager->get("embeddedDocumentUrl");
    ASSERT_TRUE(requestWeak.lock());
    auto request = requestWeak.lock();
    ASSERT_EQ(request->getUrlRequest().getUrl(), "embeddedDocumentUrl");

    // When document retrieved - create content with new session (console session management is up
    // to runtime/viewhost)
    auto embeddedSession = std::make_shared<TestSession>();
    auto content = Content::create(AUTO_EMBEDDED_DOC, embeddedSession);
    // Load any packages if required and check if ready.
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    ASSERT_TRUE(CheckComponent(root->findComponentById("embeddedText"), 50, 30));
    ASSERT_TRUE(CheckComponent(root->findComponentById("hostComponent"), 50, 30));
    ASSERT_TRUE(CheckComponent(component, 1024, 800));
    ASSERT_TRUE(CheckViewport(root, 1024, 800));


    executeCommand(
        embeddedDocumentContext,
        "SetValue", {
                        {"componentId", "embeddedText"},
                        {"property", "text"},
                        {"value", "Hello"}
                    }, false);

    advanceTime(100);

    ASSERT_TRUE(CheckComponent(root->findComponentById("embeddedText"), 50, 20));
    ASSERT_TRUE(CheckComponent(root->findComponentById("hostComponent"), 50, 20));
    ASSERT_TRUE(CheckComponent(component, 1024, 800));
    ASSERT_TRUE(CheckViewport(root, 1024, 800));


    executeCommand(
        embeddedDocumentContext,
        "SetValue", {
                        {"componentId", "embeddedText"},
                        {"property", "text"},
                        {"value", "Hello, World! Maybe, not sure yet."}
                    }, false);

    advanceTime(100);

    ASSERT_TRUE(CheckComponent(root->findComponentById("embeddedText"), 50, 60));
    ASSERT_TRUE(CheckComponent(root->findComponentById("hostComponent"), 50, 60));
    ASSERT_TRUE(CheckComponent(component, 1024, 800));
    ASSERT_TRUE(CheckViewport(root, 1024, 800));
}

static const char* HOST_DOC_AUTO_MINMAX = R"({
  "type": "APL",
  "version": "2023.1",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": "100%",
      "height": "100%",
      "item": {
        "type": "Host",
        "width": "auto",
        "height": "auto",
        "minWidth": 60,
        "maxWidth": 150,
        "minHeight": 20,
        "maxHeight": 25,
        "id": "hostComponent",
        "entities": "HOST",
        "source": "embeddedDocumentUrl",
        "onLoad": [
          {
            "type": "SendEvent",
            "sequencer": "SEND_EVENTER",
            "arguments": ["LOADED"]
          }
        ]
      }
    }
  }
})";

TEST_F(EmbeddedLifecycleTest, AutoSizedEmbeddedMinMax)
{
    loadDocument(HOST_DOC_AUTO_MINMAX);

    // While it inflates embedded document requested.
    auto requestWeak = documentManager->get("embeddedDocumentUrl");
    ASSERT_TRUE(requestWeak.lock());
    auto request = requestWeak.lock();
    ASSERT_EQ(request->getUrlRequest().getUrl(), "embeddedDocumentUrl");

    // When document retrieved - create content with new session (console session management is up
    // to runtime/viewhost)
    auto embeddedSession = std::make_shared<TestSession>();
    auto content = Content::create(AUTO_EMBEDDED_DOC, embeddedSession);
    // Load any packages if required and check if ready.
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    ASSERT_TRUE(CheckComponent(root->findComponentById("embeddedText"), 130, 20));
    ASSERT_TRUE(CheckComponent(root->findComponentById("hostComponent"), 130, 20));
    ASSERT_TRUE(CheckComponent(component, 1024, 800));
    ASSERT_TRUE(CheckViewport(root, 1024, 800));


    executeCommand(
        embeddedDocumentContext,
        "SetValue", {
                        {"componentId", "embeddedText"},
                        {"property", "text"},
                        {"value", "Hello"}
                    }, false);

    advanceTime(100);

    ASSERT_TRUE(CheckComponent(root->findComponentById("embeddedText"), 60, 20));
    ASSERT_TRUE(CheckComponent(root->findComponentById("hostComponent"), 60, 20));
    ASSERT_TRUE(CheckComponent(component, 1024, 800));
    ASSERT_TRUE(CheckViewport(root, 1024, 800));


    executeCommand(
        embeddedDocumentContext,
        "SetValue", {
                        {"componentId", "embeddedText"},
                        {"property", "text"},
                        {"value", "Hello, World! Maybe, not sure yet."}
                    }, false);

    advanceTime(100);

    ASSERT_TRUE(CheckComponent(root->findComponentById("embeddedText"), 150, 25));
    ASSERT_TRUE(CheckComponent(root->findComponentById("hostComponent"), 150, 25));
    ASSERT_TRUE(CheckComponent(component, 1024, 800));
    ASSERT_TRUE(CheckViewport(root, 1024, 800));
}

static const char* HOST_AUTO_DOC_AUTO = R"({
  "type": "APL",
  "version": "2023.1",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": "auto",
      "height": "auto",
      "item": {
        "type": "Host",
        "width": "auto",
        "height": "auto",
        "id": "hostComponent",
        "entities": "HOST",
        "source": "embeddedDocumentUrl",
        "onLoad": [
          {
            "type": "SendEvent",
            "sequencer": "SEND_EVENTER",
            "arguments": ["LOADED"]
          }
        ]
      }
    }
  }
})";

TEST_F(EmbeddedLifecycleTest, AutoSizedAutoEmbeddedAutoHost)
{
    metrics = Metrics().size(100, 100).minAndMaxHeight(50, 100).minAndMaxWidth(100, 500);
    loadDocument(HOST_AUTO_DOC_AUTO);

    // While it inflates embedded document requested.
    auto requestWeak = documentManager->get("embeddedDocumentUrl");
    ASSERT_TRUE(requestWeak.lock());
    auto request = requestWeak.lock();
    ASSERT_EQ(request->getUrlRequest().getUrl(), "embeddedDocumentUrl");

    // When document retrieved - create content with new session (console session management is up
    // to runtime/viewhost)
    auto embeddedSession = std::make_shared<TestSession>();
    auto content = Content::create(AUTO_EMBEDDED_DOC, embeddedSession);
    // Load any packages if required and check if ready.
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    ASSERT_TRUE(CheckComponent(root->findComponentById("embeddedText"), 130, 10));
    ASSERT_TRUE(CheckComponent(root->findComponentById("hostComponent"), 130, 10));
    ASSERT_TRUE(CheckComponent(component, 130, 50));
    ASSERT_TRUE(CheckViewport(root, 130, 50));


    executeCommand(
        embeddedDocumentContext,
        "SetValue", {
                        {"componentId", "embeddedText"},
                        {"property", "text"},
                        {"value", "Hello, World! Maybe, not sure yet."}
                    }, false);

    advanceTime(100);

    ASSERT_TRUE(CheckComponent(root->findComponentById("embeddedText"), 340, 10));
    ASSERT_TRUE(CheckComponent(root->findComponentById("hostComponent"), 340, 10));
    ASSERT_TRUE(CheckComponent(component, 340, 50));
    ASSERT_TRUE(CheckViewport(root, 340, 50));
}

static const char* SCROLLABLE_MULTI_HOST = R"({
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "width": "auto",
      "height": 100,
      "data": [
        "Hello first time.",
        "Hello very second time. For real. Not kidding now.",
        "Hello third time time.",
        "Bye now"
      ],
      "item": {
        "type": "Host",
        "width": "auto",
        "height": "auto",
        "entities": "HOST",
        "minWidth": 100,
        "maxWidth": 200,
        "source": "embeddedDocumentUrl${index}",
        "Input": "${data}",
        "onLoad": [
          {
            "type": "SendEvent",
            "sequencer": "SEND_EVENTER",
            "arguments": ["LOADED"]
          }
        ]
      }
    }
  }
})";

static const char* PARAMETERIZED_EMBEDDED_TEXT = R"({
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "parameters": [ "Input" ],
    "item": {
      "type": "Text",
      "width": "auto",
      "height": "auto",
      "text": "${Input}"
    }
  }
})";

TEST_F(EmbeddedLifecycleTest, ComplexScrollable)
{
    metrics = Metrics().size(100, 100).minAndMaxHeight(50, 200).minAndMaxWidth(50, 500);
    loadDocument(SCROLLABLE_MULTI_HOST);

    // When document retrieved - create content with new session (console session management is up
    // to runtime/viewhost)
    auto embeddedSession = std::make_shared<TestSession>();
    auto embeddedContent = Content::create(PARAMETERIZED_EMBEDDED_TEXT, embeddedSession);

    auto requestWeak = documentManager->get("embeddedDocumentUrl0");
    ASSERT_TRUE(requestWeak.lock());
    auto request = requestWeak.lock();
    ASSERT_EQ(request->getUrlRequest().getUrl(), "embeddedDocumentUrl0");

    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl0", embeddedContent, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));


    requestWeak = documentManager->get("embeddedDocumentUrl1");
    ASSERT_TRUE(requestWeak.lock());
    request = requestWeak.lock();
    ASSERT_EQ(request->getUrlRequest().getUrl(), "embeddedDocumentUrl1");

    embeddedContent = Content::create(PARAMETERIZED_EMBEDDED_TEXT, embeddedSession);
    embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl1", embeddedContent, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));


    requestWeak = documentManager->get("embeddedDocumentUrl2");
    ASSERT_TRUE(requestWeak.lock());
    request = requestWeak.lock();
    ASSERT_EQ(request->getUrlRequest().getUrl(), "embeddedDocumentUrl2");

    embeddedContent = Content::create(PARAMETERIZED_EMBEDDED_TEXT, embeddedSession);
    embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl2", embeddedContent, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));


    requestWeak = documentManager->get("embeddedDocumentUrl3");
    ASSERT_TRUE(requestWeak.lock());
    request = requestWeak.lock();
    ASSERT_EQ(request->getUrlRequest().getUrl(), "embeddedDocumentUrl3");

    embeddedContent = Content::create(PARAMETERIZED_EMBEDDED_TEXT, embeddedSession);
    embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl3", embeddedContent, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));


    ASSERT_TRUE(CheckComponent(component, 200, 100));
    ASSERT_TRUE(CheckViewport(root, 200, 100));

    ASSERT_TRUE(CheckComponent(component->getCoreChildAt(0)->getCoreChildAt(0), 170, 10));
    ASSERT_TRUE(CheckComponent(component->getCoreChildAt(1)->getCoreChildAt(0), 200, 30));
    ASSERT_TRUE(CheckComponent(component->getCoreChildAt(2)->getCoreChildAt(0), 200, 20));
    ASSERT_TRUE(CheckComponent(component->getCoreChildAt(3)->getCoreChildAt(0), 100, 10));
}

static const char* HOST_WITH_PARAMETERS = R"({
  "type": "APL",
  "version": "2024.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "item": {
        "type": "Host",
        "width": "100%",
        "height": "100%",
        "id": "hostComponent",
        "source": "embeddedDocumentUrl",
        "parameters": {
          "ResolveMeFromHost": "World"
        },
        "onLoad": [
          {
            "type": "SendEvent",
            "sequencer": "SEND_EVENTER",
            "arguments": ["LOADED"]
          }
        ],
        "onFail": [
          {
            "type": "SendEvent",
            "sequencer": "SEND_EVENTER",
            "arguments": ["FAILED"]
          }
        ]
      }
    }
  }
})";

static const char* EMBEDDED_WITH_PARAMETERS = R"({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "parameters": [
      "ResolveMeFromRuntime",
      "ResolveMeFromHost",
      "IAmUnusedYouKnow"
    ],
    "item": {
      "type": "Text",
      "id": "embeddedText",
      "text": "${ResolveMeFromRuntime}, ${ResolveMeFromHost}${IAmUnusedYouKnow}"
    }
  }
})";

TEST_F(EmbeddedLifecycleTest, ParameterResolution)
{
    session = std::make_shared<TestSession>();
    loadDocument(HOST_WITH_PARAMETERS);

    auto requestWeak = documentManager->get("embeddedDocumentUrl");
    ASSERT_TRUE(requestWeak.lock());
    auto request = requestWeak.lock();
    ASSERT_EQ(request->getUrlRequest().getUrl(), "embeddedDocumentUrl");

    auto embeddedSession = std::make_shared<TestSession>();
    auto content = Content::create(EMBEDDED_WITH_PARAMETERS, embeddedSession);
    // Resolve what we have
    content->addObjectData("ResolveMeFromRuntime", "Hello");
    // Still needs more
    ASSERT_FALSE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    auto embeddedText = root->findComponentById("embeddedText");
    ASSERT_EQ("Hello, World", embeddedText->getCalculated(kPropertyText).asString());
}