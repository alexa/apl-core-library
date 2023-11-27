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

#include <string>
#include <vector>

#include "../embed/testdocumentmanager.h"
#include "../testeventloop.h"
#include "apl/common.h"
#include "apl/component/hostcomponent.h"
#include "apl/component/textcomponent.h"
#include "apl/embed/documentmanager.h"
#include "apl/engine/event.h"
#include "apl/primitives/rect.h"

using namespace apl;

static const char* DEFAULT_DOC = R"({
  "type": "APL",
  "version": "2022.3",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "top",
      "item": {
        "type": "Host",
        "id": "hostComponent",
        "source": "embeddedDocumentUrl",
        "EmbeddedParameter": "Hello, World!",
        "onLoad": [
          {
            "type": "InsertItem",
            "componentId": "top",
            "item": {
              "type": "Text",
              "id": "onLoadArtifact",
              "value": "hostComponent::onLoad triggered"
            }
          }
        ],
        "onFail": [
          {
            "type": "InsertItem",
            "componentId": "top",
            "item": {
              "type": "Text",
              "id": "onFailArtifact",
              "value": "hostComponent::onFail triggered"
            }
          }
        ]
      }
    }
  }
})";

static const char* EMBEDDED_DEFAULT = R"({
  "type": "APL",
  "version": "2022.3",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "embeddedTop",
      "item": {
        "type": "Text",
        "id": "embeddedText",
        "value": "Hello, World!"
      }
    }
  },
  "onConfigChange": [
    {
      "type": "SendEvent",
      "sequencer": "EVENTER",
      "arguments": ["EMBEDDED_DEFAULT::onConfigChange triggered"]
    }
  ]
})";

class HostComponentTest : public DocumentWrapper {
public:
    HostComponentTest()
        : documentManager(std::make_shared<TestDocumentManager>())
    {
        config->documentManager(std::static_pointer_cast<DocumentManager>(documentManager));
    }

    void TearDown() override
    {
        host = nullptr;
    }

protected:

    /**
     * Load a valid APL document containing a single Host-type component with id "hostComponent."
     */
    void loadDocument(const char* doc = DEFAULT_DOC)
    {
        DocumentWrapper::loadDocument(doc);
        host = std::static_pointer_cast<HostComponent>(root->findComponentById("hostComponent"));
        ASSERT_TRUE(host);
    }

    /**
     * All requirements of loadDocument, in addition to the following:
     * 1. The APL document must not contain components having ids: "onLoadArtifact," or "onFailArtifact."
     * 2. The APL document must define the Host component to have onLoad and onFail handlers.
     * 3. The Host component onLoad handler must insert a component having id "onLoadArtifact"
     * 4. The Host component onFail handler must insert a component having id "onFailArtifact"
     * 5. The Host component source property must be "embeddedDocumentUrl"
     * Additionally, the embedded APL document must satisfy the following:
     * 1. Valid APL (expected to inflate)
     * 2. Does not declare any mainTemplate parameters
     */
    void
    nominalLoadHostAndEmbedded(const char* hostDoc = DEFAULT_DOC, const char* embedded = EMBEDDED_DEFAULT)
    {
        loadDocument(hostDoc);
        // TODO: remove onLoad/onFail requirements, leaving them only in those tests targeting onLoad/onFail
        ASSERT_FALSE(root->findComponentById("onLoadArtifact"));
        ASSERT_FALSE(root->findComponentById("onFailArtifact"));

        auto content = Content::create(embedded, makeDefaultSession());
        ASSERT_TRUE(content->isReady());
        embeddedDoc = CoreDocumentContext::cast(documentManager->succeed("embeddedDocumentUrl", content, true));

        // Displayed children ensured on the next frame
        advanceTime(10);

        ASSERT_TRUE(root->findComponentById("onLoadArtifact"));
        ASSERT_FALSE(root->findComponentById("onFailArtifact"));
        ASSERT_TRUE(embeddedDoc.lock());
    }

    std::shared_ptr<TestDocumentManager> documentManager;

    std::shared_ptr<HostComponent> host;
    std::weak_ptr<CoreDocumentContext> embeddedDoc;
};

TEST_F(HostComponentTest, ComponentDefaults)
{
    loadDocument();
    ASSERT_EQ(kComponentTypeHost, host->getType());
    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), host->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(documentManager->get("embeddedDocumentUrl").lock());
}

TEST_F(HostComponentTest, AuthorSuppliedDimensions)
{
    loadDocument(R"({
      "type": "APL",
      "version": "2022.3",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "item": {
            "id": "hostComponent",
            "type": "Host",
            "height": "600",
            "width": "800",
            "source": "embeddedDocumentUrl"
          }
        }
      }
    })");

    ASSERT_TRUE(IsEqual(Rect(0,0,800,600), host->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(documentManager->get("embeddedDocumentUrl").lock());
}

TEST_F(HostComponentTest, MissingSourceProperty)
{
    DocumentWrapper::loadDocument(R"({
      "type": "APL",
      "version": "2022.3",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "item": {
            "id": "hostComponent",
            "type": "Host"
          }
        }
      }
    })");

    ASSERT_EQ(session->getCount(), 2);
    ASSERT_TRUE(session->check("Missing required property: source/sources"));
    ASSERT_TRUE(session->check("Unable to inflate component"));
    session->clear();

    auto request = documentManager->get("embeddedDocumentUrl");
    ASSERT_FALSE(request.lock());
    ASSERT_EQ(documentManager->getResolvedRequestCount(), 0);
}

TEST_F(HostComponentTest, TestSuccessAndFailDoNothingAfterRelease)
{
    loadDocument();

    host->release();

    documentManager->succeed("embeddedDocumentUrl", nullptr);

    ASSERT_FALSE(root->findComponentById("onLoadArtifact"));

    documentManager->fail("embeddedDocumentUrl", "Something went wrong");

    ASSERT_FALSE(root->findComponentById("onFailArtifact"));
}

TEST_F(HostComponentTest, TestSuccessAndFailDoNothingAfterDelete)
{
    loadDocument();

    std::weak_ptr<Component> weak;
    {
        host->remove();
        weak = host;
        host = nullptr;
    }

    root->clearPending();
    root->clearDirty();

    // nobody has a reference to "host" anymore
    ASSERT_TRUE(weak.lock() == nullptr);

    documentManager->succeed("embeddedDocumentUrl", nullptr);

    ASSERT_FALSE(root->findComponentById("onLoadArtifact"));

    documentManager->fail("embeddedDocumentUrl", "Something went wrong");

    ASSERT_FALSE(root->findComponentById("onFailArtifact"));
}

TEST_F(HostComponentTest, TestSuccessTriggersOnLoadOnce)
{
    loadDocument();

    const std::string onLoadArtifactId = "onLoadArtifact";
    const std::string hostSourceValue = "embeddedDocumentUrl";

    ASSERT_FALSE(root->findComponentById(onLoadArtifactId));

    auto content = Content::create(EMBEDDED_DEFAULT, makeDefaultSession());
    ASSERT_TRUE(content->isReady());
    documentManager->succeed(hostSourceValue, content);

    auto onLoadArtifact = root->findComponentById(onLoadArtifactId);
    ASSERT_TRUE(onLoadArtifact);

    onLoadArtifact->remove();
    ASSERT_FALSE(root->findComponentById(onLoadArtifactId));

    documentManager->succeed(hostSourceValue, content);
    ASSERT_FALSE(root->findComponentById(onLoadArtifactId));

    documentManager->fail(hostSourceValue, "Something went wrong");
    ASSERT_FALSE(root->findComponentById("onFailArtifact"));
}

TEST_F(HostComponentTest, TestFailTriggersOnFailOnce)
{
    loadDocument();

    const std::string onFailArtifactId = "onFailArtifact";
    const std::string hostSourceValue = "embeddedDocumentUrl";

    ASSERT_FALSE(root->findComponentById(onFailArtifactId));

    documentManager->fail(hostSourceValue, "Failed to resolve Content");

    auto onFailArtifact = root->findComponentById(onFailArtifactId);
    ASSERT_TRUE(onFailArtifact);

    onFailArtifact->remove();
    ASSERT_FALSE(root->findComponentById(onFailArtifactId));

    documentManager->fail(hostSourceValue, "Failed to resolve Content");
    ASSERT_FALSE(root->findComponentById(onFailArtifactId));

    documentManager->succeed(hostSourceValue, nullptr);
    ASSERT_FALSE(root->findComponentById("onLoadArtifact"));
}

TEST_F(HostComponentTest, TestSetSourcePropertyCancelsRequestAndNewRequestSucceeds)
{
    loadDocument();

    const std::string originalSource = "embeddedDocumentUrl";
    const std::string newSource = "newEmbeddedDocumentUrl";
    const std::string onLoadArtifactId = "onLoadArtifact";

    ASSERT_TRUE(documentManager->get(originalSource).lock());

    std::static_pointer_cast<CoreComponent>(host)->setProperty(kPropertySource, newSource);

    ASSERT_FALSE(documentManager->get(originalSource).lock());
    ASSERT_TRUE(documentManager->get(newSource).lock());

    auto content = Content::create(EMBEDDED_DEFAULT, makeDefaultSession());
    documentManager->succeed(originalSource, content, false, nullptr, true);
    ASSERT_FALSE(root->findComponentById(onLoadArtifactId));

    // ...now the old request is gone
    ASSERT_FALSE(documentManager->get(originalSource).lock());
    ASSERT_EQ(documentManager->getResolvedRequestCount(), 1);
    ASSERT_TRUE(documentManager->get(newSource).lock());

    documentManager->succeed(newSource, content, false, nullptr, true);
    ASSERT_EQ(documentManager->getResolvedRequestCount(), 2);
    ASSERT_TRUE(root->findComponentById(onLoadArtifactId));
}

TEST_F(HostComponentTest, TestSetSourcePropertyCancelsRequestAndNewRequestFails)
{
    loadDocument();

    const std::string originalSource = "embeddedDocumentUrl";
    const std::string newSource = "newEmbeddedDocumentUrl";
    const std::string onFailArtifactId = "onFailArtifact";

    ASSERT_TRUE(documentManager->get(originalSource).lock());

    std::static_pointer_cast<CoreComponent>(host)->setProperty(kPropertySource, newSource);

    ASSERT_FALSE(documentManager->get(originalSource).lock());
    ASSERT_TRUE(documentManager->get(newSource).lock());

    documentManager->fail(originalSource, "Something went wrong", true);
    ASSERT_FALSE(root->findComponentById(onFailArtifactId));

    // ...now the old request is gone
    ASSERT_FALSE(documentManager->get(originalSource).lock());
    ASSERT_EQ(documentManager->getResolvedRequestCount(), 1);
    ASSERT_TRUE(documentManager->get(newSource).lock());

    documentManager->fail(newSource, "Something went wrong", true);
    ASSERT_EQ(documentManager->getResolvedRequestCount(), 2);
    ASSERT_TRUE(root->findComponentById(onFailArtifactId));
}

TEST_F(HostComponentTest, TestResolvedContentWithPendingParameterSuccess)
{
    auto content = Content::create(
        R"({
          "type": "APL",
          "version": "2022.3",
          "mainTemplate": {
            "parameters": "EmbeddedParameter",
            "item": {
              "type": "Container",
              "item": {
                "type": "Text",
                "id": "embeddedText",
                "text": "${EmbeddedParameter}"
              }
            }
          }
        })",
        makeDefaultSession()
    );

    std::set<std::string> pendingParameters = content->getPendingParameters();
    ASSERT_EQ(pendingParameters.size(), 1);
    ASSERT_NE(pendingParameters.find("EmbeddedParameter"), pendingParameters.end());

    loadDocument(DEFAULT_DOC);
    ASSERT_TRUE(host);
    ASSERT_FALSE(root->findComponentById("onLoadArtifact"));
    ASSERT_FALSE(root->findComponentById("onFailArtifact"));
    auto embeddedDoc = documentManager->succeed("embeddedDocumentUrl", content, true);

    ASSERT_TRUE(root->findComponentById("onLoadArtifact"));
    ASSERT_FALSE(root->findComponentById("onFailArtifact"));
    ASSERT_EQ(
        std::static_pointer_cast<TextComponent>(
            CoreDocumentContext::cast(embeddedDoc)->findComponentById("embeddedText"))->getValue(),
        "Hello, World!");
}

TEST_F(HostComponentTest, TestResolvedContentWithMissingParameterBecomesNull)
{
    auto content = Content::create(
        R"({
          "type": "APL",
          "version": "2022.3",
          "mainTemplate": {
            "parameters": [
              "EmbeddedParameter",
              "MissingParameter"
            ],
            "item": {
              "type": "Container",
              "item": {
                "type": "Text",
                "id": "embeddedText",
                "text": "${EmbeddedParameter} - ${MissingParameter}"
              }
            }
          }
        })",
        makeDefaultSession()
    );

    std::set<std::string> pendingParameters = content->getPendingParameters();
    ASSERT_EQ(pendingParameters.size(), 2);
    ASSERT_NE(pendingParameters.find("EmbeddedParameter"), pendingParameters.end());
    ASSERT_NE(pendingParameters.find("MissingParameter"), pendingParameters.end());
    ASSERT_FALSE(content->isReady());

    loadDocument();
    ASSERT_FALSE(root->findComponentById("onLoadArtifact"));
    ASSERT_FALSE(root->findComponentById("onFailArtifact"));

    auto embeddedDoc = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(content->isReady());

    ASSERT_TRUE(root->findComponentById("onLoadArtifact"));
    ASSERT_FALSE(root->findComponentById("onFailArtifact"));
    auto embeddedTextComponent = CoreDocumentContext::cast(embeddedDoc)->findComponentById("embeddedText");
    ASSERT_EQ("Hello, World! - ", embeddedTextComponent->getCalculated(kPropertyText).asString());
}

TEST_F(HostComponentTest, TestFindComponentByIdTraversingHostForHostById)
{
    nominalLoadHostAndEmbedded();
    ASSERT_EQ(host->findComponentById(host->getId(), true), host);
}

TEST_F(HostComponentTest, TestFindComponentByIdTraversingHostForHostByUId)
{
    nominalLoadHostAndEmbedded();
    ASSERT_EQ(host->findComponentById(host->getUniqueId(), true), host);
}

TEST_F(HostComponentTest, TestFindComponentByIdTraversingHostForEmpty)
{
    nominalLoadHostAndEmbedded();
    ASSERT_EQ(host->findComponentById("", true), nullptr);
}

TEST_F(HostComponentTest, TestFindComponentByIdTraversingHostForHostChild)
{
    nominalLoadHostAndEmbedded();

    auto child = host->getChildAt(0);
    ASSERT_TRUE(child);
    auto targetId = child->getId();
    ASSERT_EQ(targetId, "embeddedTop");
    ASSERT_EQ(host->findComponentById(targetId, true), child);
}

TEST_F(HostComponentTest, TestFindComponentByIdNotTraversingHostForHostById)
{
    nominalLoadHostAndEmbedded();

    auto targetId = host->getId();
    ASSERT_EQ(host->findComponentById(targetId, false), host);
}

TEST_F(HostComponentTest, TestFindComponentByIdNotTraversingHostForHostByUId)
{
    nominalLoadHostAndEmbedded();

    auto targetId = host->getUniqueId();
    ASSERT_EQ(host->findComponentById(targetId, false), host);
}

TEST_F(HostComponentTest, TestFindComponentByIdNotTraversingHostForEmpty)
{
    nominalLoadHostAndEmbedded();

    ASSERT_EQ(host->findComponentById("", false), nullptr);
}

TEST_F(HostComponentTest, TestFindComponentByIdNotTraversingHostForHostChild)
{
    nominalLoadHostAndEmbedded();

    auto child = host->getChildAt(0);
    ASSERT_TRUE(child);
    auto targetId = child->getId();
    ASSERT_EQ(targetId, "embeddedTop");
    ASSERT_EQ(host->findComponentById(targetId, false), nullptr);
}

TEST_F(HostComponentTest, TestEmbedRequestSuccessWithInflationFailure)
{
    // The following APL will fail to inflate because the "mainTemplate" layout
    // is not a JSON Object, but is rather just a JSON string.
    auto content = Content::create(
        R"({
          "type": "APL",
          "version": "2022.3",
          "mainTemplate": "notAnObject"
        })",
        makeDefaultSession()
    );

    loadDocument();
    ASSERT_TRUE(content->isReady());
    ASSERT_FALSE(root->findComponentById("onFailArtifact"));
    ASSERT_FALSE(documentManager->succeed("embeddedDocumentUrl", content));
    ASSERT_TRUE(root->findComponentById("onFailArtifact"));
}

TEST_F(HostComponentTest, TestGetChildCountWithEmbedded)
{
    nominalLoadHostAndEmbedded();
    ASSERT_EQ(host->getChildCount(), 1);
}

TEST_F(HostComponentTest, TestGetChildCountWithoutEmbedded)
{
    loadDocument();
    ASSERT_EQ(host->getChildCount(), 0);
}

TEST_F(HostComponentTest, TestValidGetChildAtWithEmbedded)
{
    nominalLoadHostAndEmbedded();
    auto child = host->getChildAt(0);
    ASSERT_EQ(child->getId(), "embeddedTop");
}

TEST_F(HostComponentTest, TestGetDisplayedChildCountWithEmbedded)
{
    nominalLoadHostAndEmbedded();
    ASSERT_EQ(host->getDisplayedChildCount(), 1);
}

TEST_F(HostComponentTest, TestGetDisplayedChildCountWithoutEmbedded)
{
    loadDocument();
    ASSERT_EQ(host->getDisplayedChildCount(), 0);
}

TEST_F(HostComponentTest, TestGetDisplayedChildAtWithEmbedded)
{
    nominalLoadHostAndEmbedded();
    auto child = host->getDisplayedChildAt(0);
    ASSERT_EQ(child->getId(), "embeddedTop");
}

TEST_F(HostComponentTest, TestHostSizeChangeSendsConfigurationChangeToEmbedded)
{
    nominalLoadHostAndEmbedded();
    ASSERT_TRUE(root->isDirty());

    auto hostInitialBounds = host->getProperty(kPropertyInnerBounds).get<Rect>();
    auto embeddedTop = CoreComponent::cast(embeddedDoc.lock()->findComponentById("embeddedTop"));
    auto embeddedTopInitialBounds = embeddedTop->getProperty(kPropertyBounds).get<Rect>();

    rapidjson::Document doc;
    doc.Parse(R"([{
      "type": "SetValue",
      "componentId": "hostComponent",
      "property": "width",
      "value": 50
    }])");
    root->topDocument()->executeCommands(doc, false);

    ASSERT_TRUE(CheckSendEvent(root, "EMBEDDED_DEFAULT::onConfigChange triggered"));

    ASSERT_TRUE(root->isDirty());
    auto hostNewBounds = host->getProperty(kPropertyInnerBounds).get<Rect>();
    ASSERT_NE(hostInitialBounds, hostNewBounds);
    auto embeddedNewBounds = embeddedTop->getProperty(kPropertyBounds).get<Rect>();
    ASSERT_NE(embeddedTopInitialBounds, embeddedNewBounds);
    ASSERT_EQ(hostNewBounds, embeddedNewBounds);
}

static const char* HOST_ENVIRONMENT_ENV_DISALLOW_TRUE = R"({
  "type": "APL",
  "version": "2022.3",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "top",
      "width": 200,
      "height": 200,
      "item": {
        "type": "Host",
        "id": "hostComponent",
        "source": "embeddedDocumentUrl",
        "environment": {
          "disallowEditText": true,
          "disallowVideo": true
        }
      }
    }
  }
})";

static const char* HOST_ENVIRONMENT_ENV_DISALLOW_FALSE = R"({
  "type": "APL",
  "version": "2022.3",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "top",
      "width": 200,
      "height": 200,
      "item": {
        "type": "Host",
        "id": "hostComponent",
        "source": "embeddedDocumentUrl",
        "environment": {
          "disallowEditText": false,
          "disallowVideo": false
        }
      }
    }
  }
})";

static const char* EDIT_TEXT_EMBEDDED = R"({
  "type": "APL",
  "version": "2022.3",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": "100%",
      "height": "100%",
      "id": "embeddedTop",
      "item": {
        "type": "EditText",
        "width": "100%",
        "height": "100%",
        "id": "embeddedEditText"
      }
    }
  }
})";

static const char* VIDEO_EMBEDDED = R"({
  "type": "APL",
  "version": "2022.3",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": "100%",
      "height": "100%",
      "id": "embeddedTop",
      "item": {
        "type": "Video",
        "width": "100%",
        "height": "100%",
        "id": "embeddedVideo"
      }
    }
  }
})";

TEST_F(HostComponentTest, EmbeddedEditTextNotDisplayedWhenEmbeddedDisallowEditTextTrue) {
    config->set(RootProperty::kDisallowEditText, false);
    loadDocument(HOST_ENVIRONMENT_ENV_DISALLOW_TRUE);

    auto content = Content::create(EDIT_TEXT_EMBEDDED, makeDefaultSession());
    ASSERT_TRUE(content->isReady());
    auto embeddedDoc = documentManager->succeed("embeddedDocumentUrl", content, true);

    advanceTime(10);

    ASSERT_EQ(host->getChildCount(), 1);
    auto c = host->getChildAt(0);
    ASSERT_EQ(kComponentTypeContainer, c->getType());
    // Component not displayed
    ASSERT_EQ(1, c->getChildCount());
    ASSERT_EQ(kComponentTypeEditText, c->getChildAt(0)->getType());
    ASSERT_EQ(0, c->getDisplayedChildCount());
}

TEST_F(HostComponentTest, EmbeddedEditTextDisplayedWhenEmbeddedDisallowEditTextFalse) {
    config->set(RootProperty::kDisallowEditText, false);
    loadDocument(HOST_ENVIRONMENT_ENV_DISALLOW_FALSE);

    auto content = Content::create(EDIT_TEXT_EMBEDDED, makeDefaultSession());
    ASSERT_TRUE(content->isReady());
    auto embeddedDoc = documentManager->succeed("embeddedDocumentUrl", content, true);

    advanceTime(10);

    ASSERT_EQ(host->getChildCount(), 1);
    auto c = host->getChildAt(0);
    ASSERT_EQ(kComponentTypeContainer, c->getType());
    // Component displayed
    ASSERT_EQ(1, c->getChildCount());
    ASSERT_EQ(kComponentTypeEditText, c->getChildAt(0)->getType());
    ASSERT_EQ(1, c->getDisplayedChildCount());
}

TEST_F(HostComponentTest, EmbeddedVideoNotDisplayedWhenEmbeddedDisallowVideoTrue) {
    config->set(RootProperty::kDisallowVideo, false);
    loadDocument(HOST_ENVIRONMENT_ENV_DISALLOW_TRUE);

    auto content = Content::create(VIDEO_EMBEDDED, makeDefaultSession());
    ASSERT_TRUE(content->isReady());
    auto embeddedDoc = documentManager->succeed("embeddedDocumentUrl", content, true);

    advanceTime(10);

    ASSERT_EQ(host->getChildCount(), 1);
    auto c = host->getChildAt(0);
    ASSERT_EQ(kComponentTypeContainer, c->getType());
    // Component not displayed
    ASSERT_EQ(1, c->getChildCount());
    ASSERT_EQ(kComponentTypeVideo, c->getChildAt(0)->getType());
    ASSERT_EQ(0, c->getDisplayedChildCount());
}

TEST_F(HostComponentTest, EmbeddedVideoDisplayedWhenEmbeddedDisallowVideoFalse) {
    config->set(RootProperty::kDisallowVideo, false);
    loadDocument(HOST_ENVIRONMENT_ENV_DISALLOW_FALSE);

    auto content = Content::create(VIDEO_EMBEDDED, makeDefaultSession());
    ASSERT_TRUE(content->isReady());
    auto embeddedDoc = documentManager->succeed("embeddedDocumentUrl", content, true);

    advanceTime(10);

    ASSERT_EQ(host->getChildCount(), 1);
    auto c = host->getChildAt(0);
    ASSERT_EQ(kComponentTypeContainer, c->getType());
    // Component displayed
    ASSERT_EQ(1, c->getChildCount());
    ASSERT_EQ(kComponentTypeVideo, c->getChildAt(0)->getType());
    ASSERT_EQ(1, c->getDisplayedChildCount());
}

static const char* EXPLICIT_PARAMETER_HOST = R"({
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "item": {
        "type": "Host",
        "id": "hostComponent",
        "source": "embeddedDocumentUrl",
        "parameters": {
          "ExplicitParameter": "Hello, World!"
        },
        "onLoad": [
          {
            "type": "SendEvent",
            "sequencer": "LOAD_SEQUENCER",
            "arguments": "Loaded"
          }
        ],
        "onFail": [
          {
            "type": "SendEvent",
            "sequencer": "FAIL_SEQUENCER",
            "arguments": "Failed"
          }
        ]
      }
    }
  }
})";

static const char* EXPLICIT_PARAMETER_EMBEDDED = R"({
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "parameters": "ExplicitParameter",
    "item": {
      "type": "Container",
      "item": {
        "type": "Text",
        "id": "embeddedText",
        "text": "${ExplicitParameter}"
      }
    }
  }
})";

TEST_F(HostComponentTest, TestExplicitParameterPassing)
{
    loadDocument(EXPLICIT_PARAMETER_HOST);
    ASSERT_TRUE(host);
    ASSERT_FALSE(CheckSendEvent(root, "Loaded"));
    ASSERT_FALSE(CheckSendEvent(root, "Failed"));

    auto content = Content::create(EXPLICIT_PARAMETER_EMBEDDED, makeDefaultSession());

    std::set<std::string> pendingParameters = content->getPendingParameters();
    ASSERT_EQ(pendingParameters.size(), 1);
    ASSERT_NE(pendingParameters.find("ExplicitParameter"), pendingParameters.end());

    auto embeddedDoc = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(CheckSendEvent(root, "Loaded"));
    ASSERT_FALSE(CheckSendEvent(root, "Failed"));

    auto embeddedTextComponent = CoreDocumentContext::cast(embeddedDoc)->findComponentById("embeddedText");
    ASSERT_EQ("Hello, World!", embeddedTextComponent->getCalculated(kPropertyText).asString());

    // Verify plural version of parameter is chosen in DOM serialization
    rapidjson::Document doc;
    auto json = root->serializeDOM(true, doc.GetAllocator());
    auto& hostJson = json["children"][0];
    auto& param = hostJson["parameters"]["ExplicitParameter"];
    ASSERT_EQ(std::string("Hello, World!"), param.GetString());
}

static const char* EXPLICIT_PARAMETER_HOST_WITH_PLURAL_AND_SINGULAR = R"({
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "item": {
        "type": "Host",
        "id": "hostComponent",
        "source": "embeddedDocumentUrl",
        "parameter": {
          "SingularOnly": "One",
          "Both": "SingularWins"
        },
        "parameters": {
          "PluralOnly": "Many",
          "Both": "PluralWins"
        }
      }
    }
  }
})";

static const char* EXPLICIT_PARAMETER_EMBEDDED_WITH_PLURAL_AND_SINGULAR = R"({
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "parameters": [
      "SingularOnly",
      "PluralOnly",
      "Both"
    ],
    "item": {
      "type": "Container",
      "item": {
        "type": "Text",
        "id": "embeddedText",
        "text": "${SingularOnly} - ${PluralOnly} - ${Both}"
      }
    }
  }
})";

TEST_F(HostComponentTest, TestPluralVariantPreferred)
{
    loadDocument(EXPLICIT_PARAMETER_HOST_WITH_PLURAL_AND_SINGULAR);
    ASSERT_TRUE(host);

    auto content = Content::create(EXPLICIT_PARAMETER_EMBEDDED_WITH_PLURAL_AND_SINGULAR, makeDefaultSession());

    std::set<std::string> pendingParameters = content->getPendingParameters();
    ASSERT_EQ(pendingParameters.size(), 3);
    ASSERT_NE(pendingParameters.find("SingularOnly"), pendingParameters.end());
    ASSERT_NE(pendingParameters.find("PluralOnly"), pendingParameters.end());
    ASSERT_NE(pendingParameters.find("Both"), pendingParameters.end());

    auto embeddedDoc = documentManager->succeed("embeddedDocumentUrl", content, true);

    auto embeddedTextComponent = CoreDocumentContext::cast(embeddedDoc)->findComponentById("embeddedText");
    ASSERT_EQ(" - Many - PluralWins", embeddedTextComponent->getCalculated(kPropertyText).asString());

    // Verify plural version of parameter is chosen in DOM serialization
    rapidjson::Document doc;
    auto json = root->serializeDOM(true, doc.GetAllocator());
    auto& hostJson = json["children"][0];
    ASSERT_FALSE(hostJson.HasMember("parameter"));
    ASSERT_TRUE(hostJson.HasMember("parameters"));
    ASSERT_EQ(std::string("Many"), hostJson["parameters"]["PluralOnly"].GetString());
    ASSERT_EQ(std::string("PluralWins"), hostJson["parameters"]["Both"].GetString());
    ASSERT_FALSE(hostJson["parameters"].HasMember("SinglarOnly"));
}

static const char* EXPLICIT_PARAMETER_HOST_WITH_SINGULAR = R"({
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "item": {
        "type": "Host",
        "id": "hostComponent",
        "source": "embeddedDocumentUrl",
        "parameter": {
          "Singular": "One"
        }
      }
    }
  }
})";

static const char* EXPLICIT_PARAMETER_EMBEDDED_WITH_SINGULAR = R"({
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "parameters": [
      "Singular"
    ],
    "item": {
      "type": "Container",
      "item": {
        "type": "Text",
        "id": "embeddedText",
        "text": "${Singular}"
      }
    }
  }
})";

TEST_F(HostComponentTest, TestSingularVariantWorks)
{
    loadDocument(EXPLICIT_PARAMETER_HOST_WITH_SINGULAR);
    ASSERT_TRUE(host);

    auto content = Content::create(EXPLICIT_PARAMETER_EMBEDDED_WITH_SINGULAR, makeDefaultSession());

    std::set<std::string> pendingParameters = content->getPendingParameters();
    ASSERT_EQ(pendingParameters.size(), 1);
    ASSERT_NE(pendingParameters.find("Singular"), pendingParameters.end());

    auto embeddedDoc = documentManager->succeed("embeddedDocumentUrl", content, true);

    auto embeddedTextComponent = CoreDocumentContext::cast(embeddedDoc)->findComponentById("embeddedText");
    ASSERT_EQ("One", embeddedTextComponent->getCalculated(kPropertyText).asString());

    // Verify plural version of parameter is chosen in DOM serialization
    rapidjson::Document doc;
    auto json = root->serializeDOM(true, doc.GetAllocator());
    auto& hostJson = json["children"][0];
    auto& param = hostJson["parameters"]["Singular"];
    ASSERT_EQ(std::string("One"), param.GetString());
}

static const char* EXPLICIT_AND_IMPLICIT_PARAMETER_HOST = R"({
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "item": {
        "type": "Host",
        "id": "hostComponent",
        "source": "embeddedDocumentUrl",
        "ImplicitParameter": "Implicit value",
        "parameters": {
          "ExplicitParameter": "Explicit value"
        },
        "onLoad": [
          {
            "type": "SendEvent",
            "sequencer": "LOAD_SEQUENCER",
            "arguments": "Loaded"
          }
        ],
        "onFail": [
          {
            "type": "SendEvent",
            "sequencer": "FAIL_SEQUENCER",
            "arguments": "Failed"
          }
        ]
      }
    }
  }
})";

static const char* EXPLICIT_AND_IMPLICIT_PARAMETER_EMBEDDED = R"({
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "parameters": [
      "ExplicitParameter",
      "ImplicitParameter"
    ],
    "item": {
      "type": "Container",
      "item": {
        "type": "Text",
        "id": "embeddedText",
        "text": "${ExplicitParameter} - ${ImplicitParameter}"
      }
    }
  }
})";

TEST_F(HostComponentTest, TestDisallowImplicitParametersWhenUsingExplicitParameters)
{
    loadDocument(EXPLICIT_AND_IMPLICIT_PARAMETER_HOST);
    ASSERT_TRUE(host);
    ASSERT_FALSE(CheckSendEvent(root, "Loaded"));
    ASSERT_FALSE(CheckSendEvent(root, "Failed"));

    auto content = Content::create(EXPLICIT_AND_IMPLICIT_PARAMETER_EMBEDDED, makeDefaultSession());

    std::set<std::string> pendingParameters = content->getPendingParameters();
    ASSERT_EQ(pendingParameters.size(), 2);
    ASSERT_NE(pendingParameters.find("ExplicitParameter"), pendingParameters.end());
    ASSERT_NE(pendingParameters.find("ImplicitParameter"), pendingParameters.end());

    auto embeddedDoc = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(CheckSendEvent(root, "Loaded"));
    ASSERT_FALSE(CheckSendEvent(root, "Failed"));

    auto embeddedTextComponent = CoreDocumentContext::cast(embeddedDoc)->findComponentById("embeddedText");
    ASSERT_EQ("Explicit value - ", embeddedTextComponent->getCalculated(kPropertyText).asString());
}

static const char* IMPLICIT_INTRINSIC_PROPERTY_PARAMETER_HOST = R"({
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "item": {
        "type": "Host",
        "id": "hostComponent",
        "source": "embeddedDocumentUrl",
        "ImplicitParameter": "Implicit value",
        "speech": "URL",
        "onLoad": [
          {
            "type": "SendEvent",
            "sequencer": "LOAD_SEQUENCER",
            "arguments": "Loaded"
          }
        ],
        "onFail": [
          {
            "type": "SendEvent",
            "sequencer": "FAIL_SEQUENCER",
            "arguments": "Failed"
          }
        ]
      }
    }
  }
})";

static const char* IMPLICIT_INTRINSIC_PROPERTY_PARAMETER_EMBEDDED = R"({
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "parameters": [
      "ImplicitParameter",
      "speech"
    ],
    "item": {
      "type": "Container",
      "item": {
        "type": "Text",
        "id": "embeddedText",
        "text": "${ImplicitParameter} - ${speech}"
      }
    }
  }
})";

TEST_F(HostComponentTest, TestDisallowReadingIntrinsicPropertiesAsImplicitParameters)
{
    loadDocument(IMPLICIT_INTRINSIC_PROPERTY_PARAMETER_HOST);
    ASSERT_TRUE(host);
    ASSERT_FALSE(CheckSendEvent(root, "Loaded"));
    ASSERT_FALSE(CheckSendEvent(root, "Failed"));

    auto content = Content::create(IMPLICIT_INTRINSIC_PROPERTY_PARAMETER_EMBEDDED, makeDefaultSession());

    std::set<std::string> pendingParameters = content->getPendingParameters();
    ASSERT_EQ(pendingParameters.size(), 2);
    ASSERT_NE(pendingParameters.find("ImplicitParameter"), pendingParameters.end());
    ASSERT_NE(pendingParameters.find("speech"), pendingParameters.end());

    auto embeddedDoc = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(CheckSendEvent(root, "Loaded"));
    ASSERT_FALSE(CheckSendEvent(root, "Failed"));

    auto embeddedTextComponent = CoreDocumentContext::cast(embeddedDoc)->findComponentById("embeddedText");
    ASSERT_EQ("Implicit value - ", embeddedTextComponent->getCalculated(kPropertyText).asString());
}

TEST_F(HostComponentTest, ExperimentalFeaturesCopiedCorrectly) {
    nominalLoadHostAndEmbedded();
    auto child = host->getChildAt(0);
    auto hostExperimentalFeatures = host->getContext()->getRootConfig().getExperimentalFeatures();
    auto childExperimentalFeatures = child->getContext()->getRootConfig().getExperimentalFeatures();
    ASSERT_EQ(hostExperimentalFeatures, childExperimentalFeatures);
}

