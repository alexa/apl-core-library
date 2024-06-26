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
#include "../content/testpackagemanager.h"
#include "../embed/testdocumentmanager.h"

using namespace apl;

class EmbeddedImportPackageTest : public DocumentWrapper {
public:
    EmbeddedImportPackageTest()
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
            "type": "ImportPackage",
            "sequencer": "ImportSequencer",
            "name": "packageName",
            "version": "1.0",
            "source": "sourceUri",
            "onLoad": [
              {
                "type": "InsertItem",
                "componentId": "top",
                "item": {
                  "type": "Text",
                  "text": "${@testStringImport}"
                }
              }
            ]
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
      "text": "${@testStringImport}",
      "entities": "EMBEDDED"
    }
  }
})";

static const char *PACKAGE_JSON = R"(
{
  "type": "APL",
  "version": "2023.3",
  "description": "test package definition",
  "resources": [
    {
      "string": {
        "testStringImport": "wow, nice string"
      }
    }
  ],
  "import": [],
  "layouts": {}
}
)";

TEST_F(EmbeddedImportPackageTest, HostImportPackagedNotAddedToChildContext) {
    auto testPackageManager = std::make_shared<TestPackageManager>();
    testPackageManager->putPackage("packageName:1.0", PACKAGE_JSON);

    config->packageManager(testPackageManager);

    createContent(HOST_DOC, "{}", true);
    content->load([]{}, []{});
    inflate();
    ASSERT_TRUE(root);
    rootDocument = root->topDocument();
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

    loop->advanceToEnd();

    auto top = root->topComponent();
    ASSERT_EQ("wow, nice string", top->getChildAt(1)->getCalculated(apl::kPropertyText).asString());

    // Verifies the embedded text can't read the resources from parent context
    auto embeddedText = top->getChildAt(0)->getChildAt(0);
    ASSERT_EQ("", embeddedText->getCalculated(apl::kPropertyText).asString());
}

static const char* HOST_DOC_NO_REQUEST = R"({
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
      "item": [
        {
          "type": "Host",
          "width": "100",
          "height": "100",
          "id": "hostComponent",
          "entities": "HOST",
          "source": "embeddedDocumentUrl"
        },
        {
          "type": "TouchWrapper",
          "width": "100",
          "height": "100",
          "onPress": {
            "type": "InsertItem",
            "componentId": ":root",
            "item": {
              "type": "Text",
              "text": "${@testStringImport}"
            }
          }
        }
      ]
    }
  }
})";

static const char* EMBEDDED_DOC_REQUEST = R"({
  "type": "APL",
  "version": "2023.2",
  "onMount": [
    {
      "type": "ImportPackage",
      "sequencer": "ImportSequencer",
      "name": "packageName",
      "version": "1.0",
      "source": "sourceUri",
      "onLoad": [
        {
          "type": "InsertItem",
          "componentId": ":root",
          "item": {
            "type": "Text",
            "text": "${@testStringImport}"
          }
        }
      ]
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Container"
    }
  }
})";

TEST_F(EmbeddedImportPackageTest, ChildImportPackageNotAddedToHost) {
    auto testPackageManager = std::make_shared<TestPackageManager>();
    testPackageManager->putPackage("packageName:1.0", PACKAGE_JSON);

    config->packageManager(testPackageManager);

    createContent(HOST_DOC_NO_REQUEST, "{}", true);
    content->load([]{}, []{});
    inflate();
    ASSERT_TRUE(root);
    rootDocument = root->topDocument();
    // While it inflates embedded document requested.
    auto requestWeak = documentManager->get("embeddedDocumentUrl");
    ASSERT_TRUE(requestWeak.lock());
    auto request = requestWeak.lock();
    ASSERT_EQ(request->getUrlRequest().getUrl(), "embeddedDocumentUrl");

    // When document retrieved - create content with new session (console session management is up
    // to runtime/viewhost)
    auto embeddedSession = std::make_shared<TestSession>();
    auto content = Content::create(EMBEDDED_DOC_REQUEST, embeddedSession);
    // Load any packages if required and check if ready.
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(embeddedDocumentContext);
    loop->advanceToEnd();

    auto top = root->topComponent();

    // Verifies the embedded text cant read the resources from new package
    auto embeddedText = top->getChildAt(0)->getChildAt(0)->getChildAt(0);
    ASSERT_EQ("wow, nice string", embeddedText->getCalculated(apl::kPropertyText).asString());

    // Verifies the host document can't read the resources of embedded package
    performTap(1, 101);
    ASSERT_EQ("", top->getChildAt(2)->getCalculated(apl::kPropertyText).asString());
}