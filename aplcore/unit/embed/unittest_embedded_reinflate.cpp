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

class EmbeddedReinflateTest : public DocumentWrapper {
public:
    EmbeddedReinflateTest()
        : documentManager(std::make_shared<TestDocumentManager>())
    {
        config->documentManager(std::static_pointer_cast<DocumentManager>(documentManager));
    }

protected:
    std::shared_ptr<TestDocumentManager> documentManager;
};

static const char* HOST_DOC_CONFIG_CHANGE = R"({
  "type": "APL",
  "version": "2023.1",
  "onConfigChange": {
    "type": "SendEvent",
    "sequencer": "SEND_EVENTER",
    "arguments": [
      "${event.height}", "${event.width}", "${event.theme}", "${event.viewportMode}",
      "${event.fontScale}", "${event.screenMode}", "${event.screenReader}",
      "${event.sizeChanged}", "${event.rotated}"
    ]
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "height": "100%",
      "width": "100%",
      "items": [
        {
          "type": "Host",
          "width": "80%",
          "height": "80%",
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
        },
        {
          "type": "Text",
          "id": "hostText",
          "text": "${viewport.theme}",
          "entities": "EMBEDDED"
        }
      ]
    }
  }
})";

static const char* HOST_DOC_REINFLATE_SIMPLE = R"({
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
      "preserve": [ "embeddedDocument" ]
    }
  }
})";

static const char* HOST_DOC_REINFLATE = R"({
  "type": "APL",
  "version": "2023.1",
  "onConfigChange": {
    "type": "Reinflate"
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "height": "100%",
      "width": "100%",
      "items": [
        {
          "type": "Host",
          "width": "100%",
          "height": "100%",
          "id": "hostComponent",
          "entities": "HOST",
          "source": "embeddedDocumentUrl",
          "preserve": [ "embeddedDocument" ],
          "onLoad": [
            {
              "type": "SendEvent",
              "sequencer": "SEND_EVENTER",
              "arguments": ["LOADED"]
            }
          ]
        },
        {
          "type": "Text",
          "id": "hostText",
          "text": "${viewport.theme}",
          "entities": "EMBEDDED"
        }
      ]
    }
  }
})";

static const char* EMBEDDED_DOC_CONFIG_SIMPLE = R"({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "embeddedText",
      "text": "${viewport.theme}",
      "entities": "EMBEDDED"
    }
  }
})";

static const char* EMBEDDED_DOC_CONFIG = R"({
  "type": "APL",
  "version": "2023.2",
  "onConfigChange": {
    "type": "SendEvent",
    "sequencer": "SEND_EVENTER_EMBEDDED",
    "delay": 100,
    "arguments": [
      "${event.height}", "${event.width}", "${event.theme}", "${event.viewportMode}",
      "${event.fontScale}", "${event.screenMode}", "${event.screenReader}",
      "${event.sizeChanged}", "${event.rotated}"
    ]
  },
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "embeddedText",
      "text": "${viewport.theme}",
      "entities": "EMBEDDED"
    }
  }
})";

static const char* EMBEDDED_DOC_REINFLATE = R"({
  "type": "APL",
  "version": "2023.2",
  "onConfigChange": {
    "type": "Reinflate"
  },
  "mainTemplate": {
    "item": {
      "height": "100%",
      "width": "100%",
      "type": "Text",
      "id": "embeddedText",
      "text": "${viewport.theme}",
      "entities": "EMBEDDED"
    }
  }
})";

static const char* HOST_DOC_REINFLATE_NO_PRESERVE = R"({
  "type": "APL",
  "version": "2023.1",
  "onConfigChange": {
    "type": "Reinflate"
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "height": "100%",
      "width": "100%",
      "items": [
        {
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
        },
        {
          "type": "Text",
          "id": "hostText",
          "text": "${viewport.theme}",
          "entities": "EMBEDDED"
        }
      ]
    }
  }
})";

// Simple size config change just triggers resize across the board
TEST_F(EmbeddedReinflateTest, ConfigChangeSize)
{
    metrics.size(400, 400);

    // Host document inflates
    session = std::make_shared<TestSession>();
    loadDocument(HOST_DOC_CONFIG_CHANGE);

    advanceTime(100);

    auto content = Content::create(EMBEDDED_DOC_CONFIG, session);
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed(content);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    auto configChange = ConfigurationChange(500, 500);
    root->configurationChange(configChange);
    ASSERT_TRUE(CheckSendEvent(root, 500, 500, "dark", "hub", 1, "normal", false, true, false));

    advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, 400, 400, "dark", "hub", 1, "normal", false, true, false));
}

// Size change without config change causes one only in Embedded document
TEST_F(EmbeddedReinflateTest, DirectChangeSize)
{
    metrics.size(400, 400);

    // Host document inflates
    session = std::make_shared<TestSession>();
    loadDocument(HOST_DOC_CONFIG_CHANGE);

    advanceTime(100);

    auto content = Content::create(EMBEDDED_DOC_CONFIG, session);
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed(content);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    executeCommands(JsonData(R"([{ "type": "SetValue", "componentId": "hostComponent", "property": "height", "value": 300 }])").moveToObject(), false);
    advanceTime(10);

    ASSERT_FALSE(root->hasEvent());

    advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, 300, 320, "dark", "hub", 1, "normal", false, true, false));
}

// Relevant config change passed over to the embedded doc
TEST_F(EmbeddedReinflateTest, ConfigChangeTheme)
{
    metrics.size(400, 400);

    // Host document inflates
    session = std::make_shared<TestSession>();
    loadDocument(HOST_DOC_CONFIG_CHANGE);

    advanceTime(100);

    auto content = Content::create(EMBEDDED_DOC_CONFIG, session);
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed(content);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    auto hostText = root->findComponentById("hostText");
    ASSERT_EQ("dark", hostText->getCalculated(kPropertyText).asString());
    auto embeddedText = root->findComponentById("embeddedText");
    ASSERT_EQ("dark", embeddedText->getCalculated(kPropertyText).asString());

    auto configChange = ConfigurationChange().theme("light");
    root->configurationChange(configChange);
    ASSERT_TRUE(CheckSendEvent(root, 100, 100, "light", "hub", 1, "normal", false, false, false));

    advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, 100, 100, "light", "hub", 1, "normal", false, false, false));

    hostText = root->findComponentById("hostText");
    ASSERT_EQ("dark", hostText->getCalculated(kPropertyText).asString());
    embeddedText = root->findComponentById("embeddedText");
    ASSERT_EQ("dark", embeddedText->getCalculated(kPropertyText).asString());
}

// Config change may lead to Embedded document reinflate
TEST_F(EmbeddedReinflateTest, ConfigChangeThemeEmbedded)
{
    metrics.size(400, 400);

    // Host document inflates
    session = std::make_shared<TestSession>();
    loadDocument(HOST_DOC_CONFIG_CHANGE);

    advanceTime(100);

    auto content = Content::create(EMBEDDED_DOC_REINFLATE, session);
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed(content);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    auto hostText = root->findComponentById("hostText");
    ASSERT_EQ("dark", hostText->getCalculated(kPropertyText).asString());
    auto embeddedText = root->findComponentById("embeddedText");
    ASSERT_EQ("dark", embeddedText->getCalculated(kPropertyText).asString());

    auto configChange = ConfigurationChange().theme("light");
    root->configurationChange(configChange);
    ASSERT_TRUE(CheckSendEvent(root, 100, 100, "light", "hub", 1, "normal", false, false, false));

    advanceTime(100);

    hostText = root->findComponentById("hostText");
    ASSERT_EQ("dark", hostText->getCalculated(kPropertyText).asString());
    embeddedText = root->findComponentById("embeddedText");
    ASSERT_EQ("light", embeddedText->getCalculated(kPropertyText).asString());
}

// Size change or environment ConfigChange shouldn't lead to reinflate of embedded doc
TEST_F(EmbeddedReinflateTest, ConfigChangeSizeEmbeddedNope)
{
    metrics.size(400, 400);

    // Host document inflates
    session = std::make_shared<TestSession>();
    loadDocument(HOST_DOC_CONFIG_CHANGE);

    advanceTime(100);

    auto content = Content::create(EMBEDDED_DOC_REINFLATE, session);
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed(content);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    auto embed = CoreComponent::cast(root->findComponentById("embeddedText"));
    auto actualBounds = embed->getCalculated(apl::kPropertyBounds).get<Rect>();
    ASSERT_EQ(Rect(0, 0, 320, 320), actualBounds) << "Actual: " << actualBounds.toString();

    auto configChange = ConfigurationChange(500, 500);
    root->configurationChange(configChange);
    ASSERT_TRUE(CheckSendEvent(root, 500, 500, "dark", "hub", 1, "normal", false, true, false));

    advanceTime(100);
    root->clearPending();
    embed = CoreComponent::cast(root->findComponentById("embeddedText"));
    actualBounds = embed->getCalculated(apl::kPropertyBounds).get<Rect>();
    ASSERT_EQ(Rect(0, 0, 400, 400), actualBounds) << "Actual: " << actualBounds.toString();
}

// Size change or environment ConfigChange shouldn't lead to reinflate of embedded doc
TEST_F(EmbeddedReinflateTest, ConfigChangeEnvEmbeddedNope)
{
    metrics.size(400, 400);

    // Host document inflates
    session = std::make_shared<TestSession>();
    loadDocument(HOST_DOC_CONFIG_CHANGE);

    advanceTime(100);

    auto content = Content::create(EMBEDDED_DOC_CONFIG, session);
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed(content);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    auto configChange = ConfigurationChange().environmentValue("someEnvironment", true);
    root->configurationChange(configChange);
    ASSERT_TRUE(CheckSendEvent(root, 100, 100, "dark", "hub", 1, "normal", false, false, false));

    advanceTime(100);
    ASSERT_FALSE(root->hasEvent());
}

// Config change may lead to Embedded document reinflate
TEST_F(EmbeddedReinflateTest, ConfigChangeThemeHostNonResolved)
{
    metrics.size(400, 400);

    // Host document inflates
    session = std::make_shared<TestSession>();
    loadDocument(HOST_DOC_REINFLATE);

    advanceTime(100);

    auto content = Content::create(EMBEDDED_DOC_CONFIG, session);
    ASSERT_TRUE(content->isReady());

    auto hostText = root->findComponentById("hostText");
    ASSERT_EQ("dark", hostText->getCalculated(kPropertyText).asString());

    auto configChange = ConfigurationChange().theme("light");
    root->configurationChange(configChange);
    processReinflate();

    advanceTime(100);

    // Embedded doc shouldn't reinflate
    hostText = root->findComponentById("hostText");
    ASSERT_EQ("light", hostText->getCalculated(kPropertyText).asString());
}

TEST_F(EmbeddedReinflateTest, ConfigChangeThemeHostSimple)
{
    metrics.size(400, 400);

    // Host document inflates
    session = std::make_shared<TestSession>();
    loadDocument(HOST_DOC_REINFLATE_SIMPLE);

    advanceTime(100);

    auto content = Content::create(EMBEDDED_DOC_CONFIG_SIMPLE, session);
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed(content);
    ASSERT_TRUE(embeddedDocumentContext);

    auto embeddedText = root->findComponentById("embeddedText");
    ASSERT_EQ("dark", embeddedText->getCalculated(kPropertyText).asString());

    auto configChange = ConfigurationChange().theme("light");
    root->configurationChange(configChange);
    processReinflate();

    advanceTime(100);

    // Embedded doc shouldn't reinflate
    embeddedText = root->findComponentById("embeddedText");
    ASSERT_EQ("dark", embeddedText->getCalculated(kPropertyText).asString());
}

// Embedded preserved
TEST_F(EmbeddedReinflateTest, ConfigChangeThemeHost)
{
    metrics.size(400, 400);

    // Host document inflates
    session = std::make_shared<TestSession>();
    loadDocument(HOST_DOC_REINFLATE);

    advanceTime(100);

    auto content = Content::create(EMBEDDED_DOC_CONFIG, session);
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed(content);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    auto hostText = root->findComponentById("hostText");
    ASSERT_EQ("dark", hostText->getCalculated(kPropertyText).asString());
    auto embeddedText = root->findComponentById("embeddedText");
    ASSERT_EQ("dark", embeddedText->getCalculated(kPropertyText).asString());

    auto configChange = ConfigurationChange().theme("light");
    root->configurationChange(configChange);
    embeddedDocumentContext = nullptr;
    hostText = nullptr;
    embeddedText = nullptr;
    processReinflate();

    advanceTime(100);

    // Embedded doc shouldn't reinflate
    ASSERT_TRUE(CheckSendEvent(root, 100, 100, "light", "hub", 1, "normal", false, false, false));
    hostText = root->findComponentById("hostText");
    ASSERT_EQ("light", hostText->getCalculated(kPropertyText).asString());
    embeddedText = root->findComponentById("embeddedText");
    ASSERT_EQ("dark", embeddedText->getCalculated(kPropertyText).asString());
}

// Config change may lead to Embedded document reinflate
TEST_F(EmbeddedReinflateTest, ConfigChangeThemeHostAndEmbedded)
{
    metrics.size(400, 400);

    // Host document inflates
    session = std::make_shared<TestSession>();
    loadDocument(HOST_DOC_REINFLATE);

    advanceTime(100);

    auto content = Content::create(EMBEDDED_DOC_REINFLATE, session);
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed(content);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    auto hostText = root->findComponentById("hostText");
    ASSERT_EQ("dark", hostText->getCalculated(kPropertyText).asString());
    auto embeddedText = root->findComponentById("embeddedText");
    ASSERT_EQ("dark", embeddedText->getCalculated(kPropertyText).asString());

    auto configChange = ConfigurationChange().theme("light");
    root->configurationChange(configChange);
    processReinflate();

    advanceTime(100);

    // Embedded doc shouldn't reinflate
    hostText = root->findComponentById("hostText");
    ASSERT_EQ("light", hostText->getCalculated(kPropertyText).asString());
    embeddedText = root->findComponentById("embeddedText");
    ASSERT_EQ("light", embeddedText->getCalculated(kPropertyText).asString());
}

// Config change may lead to Embedded document reinflate
TEST_F(EmbeddedReinflateTest, ConfigChangeThemeHostNoPreserve)
{
    metrics.size(400, 400);

    // Host document inflates
    session = std::make_shared<TestSession>();
    loadDocument(HOST_DOC_REINFLATE_NO_PRESERVE);

    advanceTime(100);

    auto content = Content::create(EMBEDDED_DOC_CONFIG, session);
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed(content);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    auto hostText = root->findComponentById("hostText");
    ASSERT_EQ("dark", hostText->getCalculated(kPropertyText).asString());
    auto embeddedText = root->findComponentById("embeddedText");
    ASSERT_EQ("dark", embeddedText->getCalculated(kPropertyText).asString());

    auto configChange = ConfigurationChange().theme("light");
    root->configurationChange(configChange);

    // TODO: Open question. If runtime holds strong reference - handlers may still run. Stop explicitly?
    embeddedDocumentContext = nullptr;
    processReinflate();

    advanceTime(100);

    // Embedded doc shouldn't reinflate, but will be effectively recreated
    ASSERT_FALSE(root->hasEvent());

    // Replacement requested.
    ASSERT_TRUE(!documentManager->getUnresolvedRequests().empty());
    embeddedDocumentContext = documentManager->succeed(content);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    hostText = root->findComponentById("hostText");
    ASSERT_EQ("light", hostText->getCalculated(kPropertyText).asString());
    embeddedText = root->findComponentById("embeddedText");
    ASSERT_EQ("light", embeddedText->getCalculated(kPropertyText).asString());
}

static const char* HOST_DOC_ENVIRONMENT_PASS = R"({
  "type": "APL",
  "version": "2023.1",
  "mainTemplate": {
    "item": {
      "type": "Host",
      "width": "100%",
      "height": "100%",
      "id": "hostComponent",
      "source": "embeddedDocumentUrl",
      "onLoad": [
        {
          "type": "SendEvent",
          "sequencer": "SEND_EVENTER",
          "arguments": ["LOADED"]
        }
      ],
      "bind": [
        { "name": "BoundEnvPasser", "value": "${environment.magic}" }
      ],
      "environment": {
        "BoundEnv": "${BoundEnvPasser}",
        "Magic": "${environment.magic}",
        "ViewportEnv": "${viewport.mode}",
        "Reason": "${environment.reason}",
        "ScreenMode": "${environment.screenMode}",
        "FontScale": "${environment.fontScale}",
        "ScreenReader": "${environment.screenReader}",
        "DisallowVideo": "${environment.disallowVideo}"
      }
    }
  }
})";

static const char* EMBEDDED_DOC_CONFIG_ENVIRONMENT = R"({
  "type": "APL",
  "version": "2023.2",
  "onConfigChange": [
    {
      "type": "SendEvent",
      "sequencer": "SEND_EVENTER_EMBEDDED",
      "delay": 100,
      "arguments": [
        "${event.environment.BoundEnv}", "${event.environment.DisallowVideo}",
        "${event.environment.FontScale}", "${event.environment.Magic}",
        "${event.environment.Reason}", "${event.environment.ScreenMode}",
        "${event.environment.ScreenReader}", "${event.environment.ViewportEnv}"
      ]
    },
    {
      "type": "Reinflate",
      "sequencer": "REINFLATE_EMBEDDED",
      "delay": 200
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "embeddedText",
      "text": "${environment.BoundEnv} ${environment.DisallowVideo} ${environment.FontScale} ${environment.Magic} ${environment.Reason} ${environment.ScreenMode} ${environment.ScreenReader} ${environment.ViewportEnv}"
    }
  }
})";

TEST_F(EmbeddedReinflateTest, EnvironmentPassing)
{
    metrics.size(400, 400);
    config->setEnvironmentValue("magic", false);

    loadDocument(HOST_DOC_ENVIRONMENT_PASS);

    advanceTime(100);

    auto content = Content::create(EMBEDDED_DOC_CONFIG_ENVIRONMENT, session);
    ASSERT_TRUE(content->isReady());

    // Now request can be answered.
    auto embeddedDocumentContext = documentManager->succeed(content);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    auto textComp = root->findComponentById("embeddedText");
    ASSERT_EQ("false false 1 false initial normal false hub", textComp->getCalculated(apl::kPropertyText).asString());

    auto configChange = ConfigurationChange()
                            .environmentValue("magic", true)
                            .mode(ViewportMode::kViewportModeMobile)
                            .screenMode(RootConfig::ScreenMode::kScreenModeHighContrast)
                            .fontScale(54)
                            .screenReader(true)
                            .disallowVideo(true);
    root->configurationChange(configChange);

    advanceTime(100);
    ASSERT_TRUE(CheckSendEvent(root, false, true, 54.000000, true, "reinflation", 1, true, "mobile"));

    advanceTime(100);
    textComp = root->findComponentById("embeddedText");
    ASSERT_EQ("false true 54 true reinflation 1 true mobile", textComp->getCalculated(apl::kPropertyText).asString());
}