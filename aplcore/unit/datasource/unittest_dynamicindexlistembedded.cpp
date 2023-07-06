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

#include "./dynamicindexlisttest.h"
#include "../embed/testdocumentmanager.h"

#include "apl/dynamicdata.h"

using namespace apl;

class DynamicIndexListEmbeddedTest : public DynamicIndexListTest {
public:
    DynamicIndexListEmbeddedTest() {
        documentManager = std::make_shared<TestDocumentManager>();
        config->documentManager(std::static_pointer_cast<DocumentManager>(documentManager));

        auto cnf = DynamicIndexListConfiguration()
                       .setCacheChunkSize(5)
                       .setListUpdateBufferSize(5)
                       .setFetchRetries(0)
                       .setFetchTimeout(100)
                       .setCacheExpiryTimeout(500);

        eds = std::make_shared<DynamicIndexListDataSourceProvider>(cnf);
        documentConfig = DocumentConfig::create();
        documentConfig->dataSourceProvider(eds);
    }

    std::shared_ptr<TestDocumentManager> documentManager;
    std::shared_ptr<DocumentConfig> documentConfig;
    std::shared_ptr<DynamicIndexListDataSourceProvider> eds;
};

static const char *EMBEDDED_DOC = R"({
  "type": "APL",
  "version": "2023.2",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [ "dynamicSource" ],
    "item": {
      "type": "Container",
      "id": "EmbeddedExpandable",
      "height": "100%",
      "width": "100%",
      "data": "${dynamicSource}",
      "item": {
        "type": "Text",
        "width": 100,
        "height": 100,
        "text": "${data}"
      }
    }
  }
})";

static const char* HOST_DOC = R"({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "parameters": [ "dynamicSource" ],
    "item": {
      "type": "Container",
      "width": "100%",
      "height": "100%",
      "items": [
        {
          "type": "Host",
          "width": "50%",
          "height": "50%",
          "source": "embeddedDocumentUrl"
        },
        {
          "type": "Container",
          "id": "HostExpandable",
          "width": "50%",
          "height": "50%",
          "item": {
            "type": "Text",
            "width": 100,
            "height": 100,
            "text": "${data}"
          },
          "data": "${dynamicSource}"
        }
      ]
    }
  }
})";

static const char *STATIC_DATA_1 = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "list1",
    "startIndex": 0,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 3,
    "items": [ 0, 1, 2 ]
  }
})";

static const char *STATIC_DATA_1_EMBED = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "list1",
    "startIndex": 0,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 2,
    "items": [ 5, 6 ]
  }
})";

static const char *STATIC_DATA_2 = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "list2",
    "startIndex": 0,
    "minimumInclusiveIndex": 0,
    "maximumExclusiveIndex": 3,
    "items": [ 5, 6, 7 ]
  }
})";

TEST_F(DynamicIndexListEmbeddedTest, SimpleStaticCase) {
    loadDocument(HOST_DOC, STATIC_DATA_1);
    auto content = Content::create(EMBEDDED_DOC, session);

    rawData = std::make_unique<JsonData>(STATIC_DATA_2);
    ASSERT_TRUE(rawData->get().IsObject());
    for (auto it = rawData->get().MemberBegin(); it != rawData->get().MemberEnd(); it++) {
        content->addData(it->name.GetString(), it->value);
    }

    // Load any packages if required and check if ready.
    ASSERT_TRUE(content->isReady());

    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true, documentConfig);
    ASSERT_TRUE(embeddedDocumentContext);

    auto hostParent = root->findComponentById("HostExpandable");
    ASSERT_EQ(3, hostParent->getChildCount());
    auto embeddedParent = root->findComponentById("EmbeddedExpandable");
    ASSERT_EQ(3, embeddedParent->getChildCount());
}

TEST_F(DynamicIndexListEmbeddedTest, SameListId) {
    loadDocument(HOST_DOC, STATIC_DATA_1);
    auto content = Content::create(EMBEDDED_DOC, session);

    rawData = std::make_unique<JsonData>(STATIC_DATA_1_EMBED);
    ASSERT_TRUE(rawData->get().IsObject());
    for (auto it = rawData->get().MemberBegin(); it != rawData->get().MemberEnd(); it++) {
        content->addData(it->name.GetString(), it->value);
    }

    // Load any packages if required and check if ready.
    ASSERT_TRUE(content->isReady());

    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true, documentConfig);
    ASSERT_TRUE(embeddedDocumentContext);

    auto hostParent = root->findComponentById("HostExpandable");
    ASSERT_EQ(3, hostParent->getChildCount());

    // Reuse is not allowed due to sandboxing
    auto embeddedParent = root->findComponentById("EmbeddedExpandable");
    ASSERT_EQ(2, embeddedParent->getChildCount());


    auto cmd = JsonData(R"apl([{"type": "Reinflate"}])apl");
    ASSERT_TRUE(cmd);

    embeddedDocumentContext->executeCommands(cmd.get(), true);
    advanceTime(1500);

    // Reinflate should not pick-up parent's data
    embeddedParent = root->findComponentById("EmbeddedExpandable");
    ASSERT_EQ(2, embeddedParent->getChildCount());
}

static const char* HOST_PASS_PARAMETER_DOC = R"({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "parameters": [ "dynamicSource" ],
    "item": {
      "type": "Container",
      "width": "100%",
      "height": "100%",
      "items": [
        {
          "type": "Host",
          "width": "50%",
          "height": "50%",
          "source": "embeddedDocumentUrl",
          "EmbeddedDynamicSource": "${dynamicSource}"
        },
        {
          "type": "Container",
          "id": "HostExpandable",
          "width": "50%",
          "height": "50%",
          "item": {
            "type": "Text",
            "width": 100,
            "height": 100,
            "text": "${data}"
          },
          "data": "${dynamicSource}"
        }
      ]
    }
  }
})";

static const char *EMBEDDED_AS_PARAMETER_DOC = R"({
  "type": "APL",
  "version": "2023.2",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [ "EmbeddedDynamicSource" ],
    "item": {
      "type": "Container",
      "id": "EmbeddedExpandable",
      "height": "100%",
      "width": "100%",
      "data": "${EmbeddedDynamicSource}",
      "item": {
        "type": "Text",
        "width": 100,
        "height": 100,
        "text": "${data}"
      }
    }
  }
})";

TEST_F(DynamicIndexListEmbeddedTest, PassedAsParameter) {
    loadDocument(HOST_PASS_PARAMETER_DOC, STATIC_DATA_1);
    auto content = Content::create(EMBEDDED_AS_PARAMETER_DOC, session);

    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);

    auto hostParent = root->findComponentById("HostExpandable");
    ASSERT_EQ(3, hostParent->getChildCount());
    auto embeddedParent = root->findComponentById("EmbeddedExpandable");
    ASSERT_EQ(3, embeddedParent->getChildCount());
}

static const char *WRONG_MISSING_FIELDS_DATA = R"({
  "dynamicSource": {
    "type": "dynamicIndexList",
    "listId": "listWrong",
    "minimumInclusiveIndex": 15,
    "maximumExclusiveIndex": 20,
    "items": [ 10, 11, 12, 13, 14 ]
  }
})";

TEST_F(DynamicIndexListEmbeddedTest, EmbeddedDocErrors) {
    loadDocument(HOST_DOC, STATIC_DATA_1);
    auto content = Content::create(EMBEDDED_DOC, session);

    rawData = std::make_unique<JsonData>(WRONG_MISSING_FIELDS_DATA);
    ASSERT_TRUE(rawData->get().IsObject());
    for (auto it = rawData->get().MemberBegin(); it != rawData->get().MemberEnd(); it++) {
        content->addData(it->name.GetString(), it->value);
    }

    // Load any packages if required and check if ready.
    ASSERT_TRUE(content->isReady());

    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true, documentConfig);
    ASSERT_TRUE(embeddedDocumentContext);

    ASSERT_TRUE(session->checkAndClear());

    auto errors = eds->getPendingErrors();
    ASSERT_TRUE(errors.size() == 1);
}

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

static const char *DATA = R"({
  "type": "dynamicIndexList",
  "listId": "vQdpOESlok",
  "startIndex": 0,
  "minimumInclusiveIndex": 0,
  "maximumExclusiveIndex": 1,
  "items": []
})";

static const char *EMBEDDED_DYNAMIC_LIST = R"({
  "type": "APL",
  "version": "2023.2",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "dynamicSource"
    ],
    "item": {
      "type": "Sequence",
      "id": "sequence",
      "height": 300,
      "data": "${dynamicSource}",
      "items": {
        "type": "Text",
        "id": "id${data}",
        "width": 100,
        "height": 100,
        "text": "${data}"
      }
    }
  }
})";

TEST_F(DynamicIndexListEmbeddedTest, DynamicIndexListRequestsTagged)
{
    loadDocument(HOST_ONLY_DOC);
    auto content = Content::create(EMBEDDED_DYNAMIC_LIST, makeDefaultSession());
    content->addData("dynamicSource", DATA);
    ASSERT_TRUE(content->isReady());
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true, documentConfig);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    advanceTime(10);

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeDataSourceFetchRequest, event.getType());
    ASSERT_EQ(embeddedDocumentContext, event.getDocument());
}

TEST_F(DynamicIndexListEmbeddedTest, NotAvailableForEmbedded) {
    loadDocument(HOST_DOC, STATIC_DATA_1);
    auto content = Content::create(EMBEDDED_DOC, session);

    rawData = std::make_unique<JsonData>(STATIC_DATA_2);
    ASSERT_TRUE(rawData->get().IsObject());
    for (auto it = rawData->get().MemberBegin(); it != rawData->get().MemberEnd(); it++) {
        content->addData(it->name.GetString(), it->value);
    }

    // Load any packages if required and check if ready.
    ASSERT_TRUE(content->isReady());

    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);

    auto hostParent = root->findComponentById("HostExpandable");
    ASSERT_EQ(3, hostParent->getChildCount());
    auto embeddedParent = root->findComponentById("EmbeddedExpandable");
    ASSERT_EQ(1, embeddedParent->getChildCount());
}