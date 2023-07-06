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
#include "../audio/testaudioplayerfactory.h"
#include "../embed/testdocumentmanager.h"
#include "../media/testmediaplayerfactory.h"

using namespace apl;

class DocumentCreateTest : public DocumentWrapper {
public:
    DocumentCreateTest()
        : documentManager(std::make_shared<TestDocumentManager>())
    {
        config->documentManager(std::static_pointer_cast<DocumentManager>(documentManager));
    }

protected:
    std::shared_ptr<TestDocumentManager> documentManager;
};

static const char* DEFAULT_DOC = R"({
  "type": "APL",
  "version": "2022.3",
  "environment": {
    "lang": "en-UK",
    "layoutDirection": "RTL"
  },
  "theme": "light",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "top",
      "item": {
        "type": "Host",
        "id": "hostComponent",
        "height": 125.0,
        "width": 250.0,
        "source": "embeddedDocumentUrl",
        "onLoad": [
          {
            "type": "InsertItem",
            "componentId": "top",
            "item": {
              "type": "Text",
              "id": "hostOnLoadArtifact"
            }
          }
        ],
        "onFail": [
          {
            "type": "InsertItem",
            "componentId": "top",
            "item": {
              "type": "Text",
              "id": "hostOnFailArtifact"
            }
          }
        ]
      }
    }
  }
})";

static const char* EFFECTIVE_OVERRIDES_DOC = R"({
  "type": "APL",
  "version": "2022.3",
  "environment": {
    "lang": "en-UK",
    "layoutDirection": "LTR"
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "top",
      "item": {
        "type": "Host",
        "id": "hostComponent",
        "environment": {
          "allowOpenURL": false,
          "disallowDialog": true,
          "disallowEditText": true,
          "disallowVideo": true,
          "lang": "en-IN",
          "layoutDirection": "RTL"
        },
        "source": "embeddedDocumentUrl",
        "onLoad": [
          {
            "type": "InsertItem",
            "componentId": "top",
            "item": {
              "type": "Text",
              "id": "hostOnLoadArtifact"
            }
          }
        ],
        "onFail": [
          {
            "type": "InsertItem",
            "componentId": "top",
            "item": {
              "type": "Text",
              "id": "hostOnFailArtifact"
            }
          }
        ]
      }
    }
  }
})";

static const char* INEFFECTIVE_OVERRIDES_DOC = R"({
  "type": "APL",
  "version": "2022.3",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "top",
      "item": {
        "type": "Host",
        "id": "hostComponent",
        "environment": {
          "allowOpenURL": true,
          "disallowDialog": false,
          "disallowEditText": false,
          "disallowVideo": false
        },
        "source": "embeddedDocumentUrl",
        "onLoad": [
          {
            "type": "InsertItem",
            "componentId": "top",
            "item": {
              "type": "Text",
              "id": "hostOnLoadArtifact"
            }
          }
        ],
        "onFail": [
          {
            "type": "InsertItem",
            "componentId": "top",
            "item": {
              "type": "Text",
              "id": "hostOnFailArtifact"
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
      "items": [
        {
          "type": "EditText",
          "id": "embeddedEditText",
          "onSubmit": [
            {
              "type": "SendEvent"
            }
          ]
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

TEST_F(DocumentCreateTest, TestEnvironmentCreationWithEffectiveOverrides)
{
    config->set(RootProperty::kAllowOpenUrl, true);
    config->set(RootProperty::kDisallowDialog, false);
    config->set(RootProperty::kDisallowEditText, false);
    config->set(RootProperty::kDisallowVideo, false);

    loadDocument(EFFECTIVE_OVERRIDES_DOC);

    // Inflate and verify the embedded document
    auto content = Content::create(EMBEDDED_DEFAULT, makeDefaultSession());
    ASSERT_TRUE(content->isReady());
    auto embeddedDoc = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(root->findComponentById("hostOnLoadArtifact"));
    ASSERT_FALSE(root->findComponentById("hostOnFailArtifact"));

    auto embeddedTop = CoreComponent::cast(CoreDocumentContext::cast(embeddedDoc)->topComponent());

    auto embeddedConfig = embeddedTop->getRootConfig();
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kAllowOpenUrl), false);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kDisallowDialog), true);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kDisallowEditText), true);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kDisallowVideo), true);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kLang), "en-IN");
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kLayoutDirection), LayoutDirection::kLayoutDirectionRTL);
}

TEST_F(DocumentCreateTest, TestEnvironmentCreationWithIneffectiveOverrides)
{
    config->set(RootProperty::kAllowOpenUrl, false);
    config->set(RootProperty::kDisallowDialog, true);
    config->set(RootProperty::kDisallowEditText, true);
    config->set(RootProperty::kDisallowVideo, true);

    loadDocument(INEFFECTIVE_OVERRIDES_DOC);

    // Inflate and verify the embedded document
    auto content = Content::create(EMBEDDED_DEFAULT, makeDefaultSession());
    ASSERT_TRUE(content->isReady());
    auto embeddedDoc = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(root->findComponentById("hostOnLoadArtifact"));
    ASSERT_FALSE(root->findComponentById("hostOnFailArtifact"));

    auto embeddedTop = CoreComponent::cast(CoreDocumentContext::cast(embeddedDoc)->topComponent());

    auto embeddedConfig = embeddedTop->getRootConfig();
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kAllowOpenUrl), false);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kDisallowDialog), true);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kDisallowEditText), true);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kDisallowVideo), true);
}

TEST_F(DocumentCreateTest, TestRootConfigCreation)
{
    auto dpi = Metrics::CORE_DPI;
    auto mode = ViewportMode::kViewportModeHub;
    auto shape = ScreenShape::RECTANGLE;

    metrics.dpi(dpi)
        .mode(mode)
        .shape(shape);

    auto audioPlayerFactory = std::make_shared<TestAudioPlayerFactory>(config->getTimeManager());
    config->audioPlayerFactory(audioPlayerFactory);
    auto mediaPlayerFactory = std::make_shared<TestMediaPlayerFactory>();
    config->mediaPlayerFactory(mediaPlayerFactory);
    config->set(RootProperty::kInitialDisplayState, DisplayState::kDisplayStateBackground);
    config->set(RootProperty::kAgentName, "unittest");
    config->set(RootProperty::kAgentVersion, "90210");
    config->set(RootProperty::kAnimationQuality, "slow");
    config->set(RootProperty::kReportedVersion, "2023.1");
    config->set(RootProperty::kFontScale, 1.5);
    config->set(RootProperty::kScreenMode, "high-contrast");
    config->set(RootProperty::kScreenReader, true);
    config->set(RootProperty::kDoublePressTimeout, 350);
    config->set(RootProperty::kLongPressTimeout, 450);
    config->set(RootProperty::kMinimumFlingVelocity, 45);
    config->set(RootProperty::kPressedDuration, 60);
    config->set(RootProperty::kTapOrScrollTimeout, 99);
    config->set(RootProperty::kMaximumTapVelocity, 555);
    config->set(RootProperty::kAllowOpenUrl, true);
    config->set(RootProperty::kDisallowDialog, false);
    config->set(RootProperty::kDisallowEditText, false);
    config->set(RootProperty::kDisallowVideo, false);
    config->set(RootProperty::kUTCTime, 12345678);
    config->set(RootProperty::kLocalTimeAdjustment, 4000);

    loadDocument(DEFAULT_DOC);

    // Inflate and verify the embedded document
    auto content = Content::create(EMBEDDED_DEFAULT, makeDefaultSession());
    ASSERT_TRUE(content->isReady());
    auto embeddedDoc = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(root->findComponentById("hostOnLoadArtifact"));
    ASSERT_FALSE(root->findComponentById("hostOnFailArtifact"));

    auto embeddedTop = CoreComponent::cast(CoreDocumentContext::cast(embeddedDoc)->topComponent());

    auto embeddedConfig = embeddedTop->getRootConfig();
    auto embeddedContext = embeddedTop->getContext();
    auto embeddedViewport = embeddedContext->opt("viewport");

    // Not copied from host document
    ASSERT_EQ(embeddedContext->opt("elapsedTime"), 0);
    ASSERT_EQ(embeddedContext->opt("environment").get("reason"), "initial");

    // Copied from host document
    ASSERT_EQ(embeddedViewport.get("dpi"), dpi);
    ASSERT_EQ(embeddedViewport.get("shape"), "rectangle");
    ASSERT_EQ(embeddedViewport.get("mode"), "hub");
    ASSERT_EQ(embeddedConfig.getDocumentManager(), documentManager);
    ASSERT_EQ(embeddedConfig.getAudioPlayerFactory(), audioPlayerFactory);
    ASSERT_EQ(embeddedConfig.getMediaPlayerFactory(), mediaPlayerFactory);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kUTCTime), 12345678);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kLocalTimeAdjustment), 4000);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kAgentName), "unittest");
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kAgentVersion), "90210");
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kFontScale), 1.5);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kScreenMode), RootConfig::ScreenMode::kScreenModeHighContrast);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kScreenReader), true);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kInitialDisplayState), DisplayState::kDisplayStateBackground);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kAnimationQuality), RootConfig::AnimationQuality::kAnimationQualitySlow);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kReportedVersion), "2023.1");
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kDoublePressTimeout), 350);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kLongPressTimeout), 450);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kMinimumFlingVelocity), 45);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kPressedDuration), 60);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kTapOrScrollTimeout), 99);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kMaximumTapVelocity), 555);

    // TODO: verify extension ?

    // Can be overridden by the top-level document, so Metrics/RootConfig is not the authority
    ASSERT_EQ(embeddedViewport.get("theme"), "light");
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kLang), "en-UK");
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kLayoutDirection), LayoutDirection::kLayoutDirectionRTL);

    // Set by the Host Component
    ASSERT_EQ(embeddedViewport.get("height"), 125.0);
    ASSERT_EQ(embeddedViewport.get("width"), 250.0);

    // Can be overridden by the Host Component, but aren't in this test
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kAllowOpenUrl), true);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kDisallowDialog), false);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kDisallowEditText), false);
    ASSERT_EQ(embeddedConfig.getProperty(RootProperty::kDisallowVideo), false);
}

TEST_F(DocumentCreateTest, TestEventManagerPassedThrough)
{
    loadDocument(DEFAULT_DOC);

    // Inflate and verify the embedded document
    auto content = Content::create(EMBEDDED_DEFAULT, makeDefaultSession());
    ASSERT_TRUE(content->isReady());
    auto embeddedDoc = documentManager->succeed("embeddedDocumentUrl", content, true);
    ASSERT_TRUE(root->findComponentById("hostOnLoadArtifact"));
    ASSERT_FALSE(root->findComponentById("hostOnFailArtifact"));

    auto editText = std::static_pointer_cast<CoreComponent>(CoreDocumentContext::cast(embeddedDoc)->findComponentById("embeddedEditText"));
    ASSERT_FALSE(editText->getRootConfig().getProperty(RootProperty::kDisallowEditText).asBoolean());

    // Verifying the Event published via the embedded EditText implicitly verifies the embedded doc
    // has the same EventManager as the host doc
    ASSERT_FALSE(root->hasEvent());
    editText->update(kUpdateSubmit, 0.0);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(event.getType(), kEventTypeSendEvent);
    ASSERT_EQ(event.getValue(apl::kEventPropertySource).get("id"), editText->getId());
}
