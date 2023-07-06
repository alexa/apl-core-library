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
    "type": "Log"
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

const static char *PARENT_VC = R"({
  "children": [
    {
      "entities": [
        "HOST"
      ],
      "tags": {
        "focused": false
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
          "focused": false
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

    auto embeddedDocumentContext1 = documentManager->succeed("embeddedDocumentUrl", content, true, std::make_shared<DocumentConfig>(), true);
    ASSERT_TRUE(embeddedDocumentContext1);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED1"));

    auto embeddedDocumentContext2 = documentManager->succeed("embeddedDocumentUrl", content, true, std::make_shared<DocumentConfig>(), true);
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

    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true, std::make_shared<DocumentConfig>(), true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));
}
