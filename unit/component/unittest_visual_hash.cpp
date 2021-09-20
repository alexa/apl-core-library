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

#include "apl/component/textcomponent.h"

using namespace apl;

class VisualHashTest : public DocumentWrapper {};

static const char* BASE_COMPONENT_PROPS = R"({
  "type": "APL",
  "version": "1.8",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "height": 100,
      "width": 100
    }
  }
})";

struct PropTest {
    PropertyKey key;
    Object replaceValue;
    bool affectsVisual;
};

static const std::vector<PropTest> BASIC_TESTS = {
    {kPropertyAccessibilityLabel,      "label",                                                     false},
    {kPropertyChecked,                 true,                                                        false},
    {kPropertyMinHeight,               Dimension(125),                                              true},
    {kPropertyMinWidth,                Dimension(125),                                              true},
    {kPropertyHeight,                  Dimension(200),                                              true},
    {kPropertyWidth,                   Dimension(200),                                              true},
    {kPropertyMaxHeight,               Dimension(150),                                              true},
    {kPropertyMaxWidth,                Dimension(150),                                              true},
    {kPropertyLayoutDirectionAssigned, "RTL",                                                       true},
    {kPropertyOpacity,                 0.5,                                                         true},
    {kPropertyPadding,                 Object(std::make_shared<ObjectArray>(ObjectArray{1,2,3,4})), true},
    {kPropertyShadowColor,             Color(0xFFEEFF),                                             true},
    {kPropertyShadowHorizontalOffset,  Dimension(20),                                               true},
    {kPropertyShadowRadius,            Dimension(20),                                               true},
    {kPropertyShadowVerticalOffset,    Dimension(20),                                               true},
    {kPropertyDisplay,                 "invisible",                                                 false},
    {kPropertyDisabled,                true,                                                        false},
};

TEST_F(VisualHashTest, BaseProperties) {
    loadDocument(BASE_COMPONENT_PROPS);

    for (auto& pt : BASIC_TESTS) {
        component->setProperty(pt.key, pt.replaceValue);
        advanceTime(100);
        auto& dirty = component->getDirty();
        ASSERT_EQ(pt.affectsVisual, dirty.count(kPropertyVisualHash) > 0) << "Property: " << pt.key << " " << pt.affectsVisual;
        root->clearDirty();
    }
}

static const char* EDIT_TEXT_COMPONENT_PROPS = R"({
  "type": "APL",
  "version": "1.8",
  "mainTemplate": {
    "item": {
      "type": "EditText",
      "height": 100,
      "width": 100
    }
  }
})";

static const std::vector<PropTest> ET_TESTS = {
    {kPropertyBorderColor,       Color(0xff00ff), true},
    {kPropertyBorderWidth,       Dimension(10),    true},
    {kPropertyColor,             Color(0xff00ff), true},
    {kPropertyFontFamily,        "kinda-family",  true},
    {kPropertyFontSize,          Dimension(80),   true},
    {kPropertyFontStyle,         "italic",        true},
    {kPropertyFontWeight,        800,             true},
    {kPropertyHighlightColor,    Color(0xff00ff), true},
    {kPropertyHint,              "Hint?",         true},
    {kPropertyHintColor,         Color(0xff00ff), true},
    {kPropertyHintStyle,         "italic",        true},
    {kPropertyHintWeight,        800,             true},
    {kPropertyLang,              "ja-JP",         true},
    {kPropertySecureInput,       true,            false},
    {kPropertyText,              "text",          true},
    {kPropertyBorderStrokeWidth, Dimension(5),    true},
};

TEST_F(VisualHashTest, EditTextProperties) {
    loadDocument(EDIT_TEXT_COMPONENT_PROPS);

    for (auto& pt : ET_TESTS) {
        component->setProperty(pt.key, pt.replaceValue);
        advanceTime(100);
        auto& dirty = component->getDirty();
        ASSERT_EQ(pt.affectsVisual, dirty.count(kPropertyVisualHash) > 0) << "Property: " << pt.key << " " << pt.affectsVisual;
        root->clearDirty();
    }
}

static const char* TEXT_COMPONENT_PROPS = R"({
  "type": "APL",
  "version": "1.8",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "height": 100,
      "width": 100
    }
  }
})";

static const std::vector<PropTest> TEXT_TESTS = {
    {kPropertyColor,             Color(0xff00ff), true},
    {kPropertyFontFamily,        "some-family",   true},
    {kPropertyFontSize,          Dimension(80),   true},
    {kPropertyFontStyle,         "italic",        true},
    {kPropertyFontWeight,        800,             true},
    {kPropertyLang,              "jp-JP",         true},
    {kPropertyText,              "text",          true},
    {kPropertyTextAlignAssigned, "center",        true},
};

TEST_F(VisualHashTest, TextProperties) {
    loadDocument(TEXT_COMPONENT_PROPS);

    for (auto& pt : TEXT_TESTS) {
        component->setProperty(pt.key, pt.replaceValue);
        advanceTime(100);
        auto& dirty = component->getDirty();
        ASSERT_EQ(pt.affectsVisual, dirty.count(kPropertyVisualHash) > 0) << "Property: " << pt.key << " " << pt.affectsVisual;
        root->clearDirty();
    }
}

static const char* FRAME_COMPONENT_PROPS = R"({
  "type": "APL",
  "version": "1.8",
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "height": 100,
      "width": 100
    }
  }
})";

static const std::vector<PropTest> FRAME_TESTS = {
    {kPropertyBackgroundColor,         Color(0xff00ff), true},
    {kPropertyBorderColor,             Color(0xff00ff), true},
    {kPropertyBorderWidth,             Dimension(10),   true},
    {kPropertyBorderRadius,            Dimension(10),   true},
    {kPropertyBorderBottomLeftRadius,  Dimension(5),    true},
    {kPropertyBorderBottomRightRadius, Dimension(5),    true},
    {kPropertyBorderTopLeftRadius,     Dimension(5),    true},
    {kPropertyBorderTopRightRadius,    Dimension(5),    true},
    {kPropertyBorderStrokeWidth,       5,               true},
};

TEST_F(VisualHashTest, FrameProperties) {
    loadDocument(FRAME_COMPONENT_PROPS);

    for (auto& pt : FRAME_TESTS) {
        component->setProperty(pt.key, pt.replaceValue);
        advanceTime(100);
        auto& dirty = component->getDirty();
        ASSERT_EQ(pt.affectsVisual, dirty.count(kPropertyVisualHash) > 0) << "Property: " << pt.key << " " << pt.affectsVisual;
        root->clearDirty();
    }
}

static const char* IMAGE_COMPONENT_PROPS = R"({
  "type": "APL",
  "version": "1.8",
  "mainTemplate": {
    "item": {
      "type": "Image",
      "height": 100,
      "width": 100,
      "overlayGradient": {
        "colorRange": [
          "green",
          "gray"
        ]
      }
    }
  }
})";

static const std::vector<PropTest> IMAGE_TESTS = {
    {kPropertyAlign,           "left",          true},
    {kPropertyBorderRadius,    Dimension(10),   true},
    {kPropertyOverlayColor,    Color(0xff00ff), true},
    {kPropertyOverlayGradient, Object::NULL_OBJECT(), true},
    {kPropertyScale,           "fill",          true},
    {kPropertySource,          "source",        true},
};

TEST_F(VisualHashTest, ImageProperties) {
    loadDocument(IMAGE_COMPONENT_PROPS);

    for (auto& pt : IMAGE_TESTS) {
        component->setProperty(pt.key, pt.replaceValue);
        advanceTime(100);
        auto& dirty = component->getDirty();
        ASSERT_EQ(pt.affectsVisual, dirty.count(kPropertyVisualHash) > 0) << "Property: " << pt.key << " " << pt.affectsVisual;
        root->clearDirty();
    }
}

static const char* VIDEO_COMPONENT_PROPS = R"({
  "type": "APL",
  "version": "1.8",
  "mainTemplate": {
    "item": {
      "type": "Video",
      "height": 100,
      "width": 100
    }
  }
})";

static const std::vector<PropTest> VIDEO_TESTS = {
    {kPropertySource, "IAMARRAY", true},
};

TEST_F(VisualHashTest, VideoProperties) {
    loadDocument(VIDEO_COMPONENT_PROPS);

    for (auto& pt : VIDEO_TESTS) {
        component->setProperty(pt.key, pt.replaceValue);
        advanceTime(100);
        auto& dirty = component->getDirty();
        ASSERT_EQ(pt.affectsVisual, dirty.count(kPropertyVisualHash) > 0) << "Property: " << pt.key << " " << pt.affectsVisual;
        root->clearDirty();
    }
}

static const char* VECTOR_GRAPHIC_PROPS = R"({
  "type": "APL",
  "version": "1.8",
  "graphics": {
    "graphic1": {
      "type": "AVG",
      "version": "1.2",
      "width": 100,
      "height": 50,
      "items": [
        {
          "type": "path",
          "pathData": "M45,88 A43,43,0,0,1,45,2 L105,2 A43,43,0,0,1,105,88 Z",
          "stroke": "#979797",
          "fill": "green",
          "strokeWidth": 2
        }
      ]
    },
    "graphic2": {
      "type": "AVG",
      "version": "1.2",
      "width": 100,
      "height": 50,
      "items": [
        {
          "type": "path",
          "pathData": "M45,88 A43,43,0,0,1,45,2 L105,2 A43,43,0,0,1,105,88 Z",
          "stroke": "yellow",
          "fill": "red",
          "strokeWidth": 7
        }
      ]
    }
  },
  "mainTemplate": {
    "item": {
      "type": "VectorGraphic",
      "height": 100,
      "width": 100,
      "source": "graphic1"
    }
  }
})";

static const std::vector<PropTest> VG_TESTS = {
    {kPropertyAlign,       "bottom",   true},
    {kPropertyScale,       "fill",     true},
    {kPropertySource,      "graphic2", true},
};

TEST_F(VisualHashTest, VectorGraphicsProperties) {
    loadDocument(VECTOR_GRAPHIC_PROPS);

    for (auto& pt : VG_TESTS) {
        component->setProperty(pt.key, pt.replaceValue);
        advanceTime(100);
        auto& dirty = component->getDirty();
        ASSERT_EQ(pt.affectsVisual, dirty.count(kPropertyVisualHash) > 0) << "Property: " << pt.key << " " << pt.affectsVisual;
        root->clearDirty();
    }
}

// TODO: Do same for styled props?

static const char* IDENTITY_FRAMES = R"({
  "type": "APL",
  "version": "1.8",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "height": "100%",
      "width": "100%",
      "items": [
        {
          "type": "Frame",
          "height": 100,
          "width": 100
        },
        {
          "type": "Frame",
          "height": 100,
          "width": 100
        }
      ]
    }
  }
})";

TEST_F(VisualHashTest, HashComparison) {
    loadDocument(IDENTITY_FRAMES);

    auto f1 = component->getCoreChildAt(0);
    auto f2 = component->getCoreChildAt(1);

    ASSERT_EQ(f1->getCalculated(kPropertyVisualHash), f2->getCalculated(kPropertyVisualHash));

    f1->setProperty(kPropertyBackgroundColor, 0x7);
    root->clearPending();
    root->clearDirty();
    ASSERT_NE(f1->getCalculated(kPropertyVisualHash), f2->getCalculated(kPropertyVisualHash));

    f2->setProperty(kPropertyBackgroundColor, 0x7);
    root->clearPending();
    root->clearDirty();
    ASSERT_EQ(f1->getCalculated(kPropertyVisualHash), f2->getCalculated(kPropertyVisualHash));
}

static const char *RTL_FIX_ALIGNMENT = R"(
{
  "type": "APL",
  "version": "1.5",
  "layoutDirection": "RTL",
  "mainTemplate": {
    "items": {
      "type": "Text",
      "text": "Original text"
    }
  }
}
)";

TEST_F(VisualHashTest, HashRemainsStableWhenLayoutAlignmentNeedsFixing)
{
    loadDocument(RTL_FIX_ALIGNMENT);

    ASSERT_EQ("Text", component->name());
    auto textComponent = std::dynamic_pointer_cast<TextComponent>(component);
    auto originalVisualHash = textComponent->getCalculated(kPropertyVisualHash);

    textComponent->setProperty(kPropertyText, "Different text");
    root->clearPending();
    root->clearDirty();
    ASSERT_NE(originalVisualHash, textComponent->getCalculated(kPropertyVisualHash));

    textComponent->setProperty(kPropertyText, "Original text");
    root->clearPending();
    root->clearDirty();
    ASSERT_EQ(originalVisualHash, textComponent->getCalculated(kPropertyVisualHash));
}

class SpyTextMeasure : public TextMeasurement {
public:
    LayoutSize measure(Component *component, float width, MeasureMode widthMode, float height,
                   MeasureMode heightMode) override {
        visualHashes.emplace_back(component->getCalculated(kPropertyVisualHash));
        return LayoutSize({ 90.0, 30.0 });
    }

    float baseline(Component *component, float width, float height) override {
        return 0;
    }

    std::vector<Object> visualHashes;
};

static const char *REMEASURE_TEXT = R"(
{
  "type": "APL",
  "version": "1.8",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "width": "80%",
      "height": "80%",
      "items": {
        "type": "Text",
        "text": "Original text",
        "width": "auto"
      }
    }
  }
}
)";

TEST_F(VisualHashTest, HashRecalculatedBeforeLayoutInTimeForTextMeasurement)
{
    auto spyTextMeasure = std::make_shared<SpyTextMeasure>();
    config->measure(spyTextMeasure);

    loadDocument(REMEASURE_TEXT);
    ASSERT_EQ(1, component->getChildCount());
    auto textComponent = std::dynamic_pointer_cast<TextComponent>(component->getChildAt(0));
    ASSERT_EQ("Text", textComponent->name());

    ASSERT_EQ(1, spyTextMeasure->visualHashes.size());

    // No new measurements are pending
    advanceTime(10);
    clearDirty();
    ASSERT_EQ(1, spyTextMeasure->visualHashes.size());

    // Change in text results in a new measurement
    textComponent->setProperty(kPropertyText, "Different text");
    advanceTime(10);
    clearDirty();
    ASSERT_EQ(2, spyTextMeasure->visualHashes.size());
    
    // Visual hashes should have been updated pre-layout resulting in different values at measure time 
    ASSERT_NE(spyTextMeasure->visualHashes.at(0), spyTextMeasure->visualHashes.at(1));
}
