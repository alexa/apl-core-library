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
#include "apl/engine/event.h"
#include "apl/primitives/object.h"

using namespace apl;

class TextComponentTest : public DocumentWrapper {};

static const char* NOLANG_DEFAULT_DOC = R"({
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "item": {
      "type": "Text"
    }
  }
})";

static const char* LANG_DEFAULT_DOC = R"({
  "type": "APL",
  "version": "1.7",
  "lang": "en-US",
  "mainTemplate": {
    "item": {
      "type": "Text"
    }
  }
})";

/**
 * Verify that we are shadowing the rootconfig value if doc level layoutDirection property is not set
 */
TEST_F(TextComponentTest, ComponentLayoutDirectionDefaultsRootConfig) {
    config->set(RootProperty::kLayoutDirection, "RTL");
    loadDocument(NOLANG_DEFAULT_DOC);

    auto et = root->topComponent();
    ASSERT_EQ(kLayoutDirectionRTL, et->getCalculated(kPropertyLayoutDirection).asInt());
}

/**
 * Verify that the value is "" if rootconfig and doc level lang properties are not set
 */
TEST_F(TextComponentTest, ComponentLangDefaults) {
    loadDocument(NOLANG_DEFAULT_DOC);

    auto et = root->topComponent();
    ASSERT_EQ("", et->getCalculated(kPropertyLang).asString());
}

/**
 * Verify that we are shadowing the rootconfig value if doc level lang property is not set
 */
TEST_F(TextComponentTest, ComponentLangDefaultsRootConfig) {
    config->set(RootProperty::kLang, "en-US");
    loadDocument(NOLANG_DEFAULT_DOC);

    auto et = root->topComponent();
    ASSERT_EQ("en-US", et->getCalculated(kPropertyLang).asString());
}

/**
 * Verify that we are shadowing the doc level lang property
 */
TEST_F(TextComponentTest, ComponentLangDefaultsDocumentLevel) {
    loadDocument(LANG_DEFAULT_DOC);

    auto et = root->topComponent();
    ASSERT_EQ("en-US", et->getCalculated(kPropertyLang).asString());
}

static const char* LANG_TEXT_DEFAULT_DOC = R"({
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "item": {
      "lang": "en-US",
      "type": "Text"
    }
  }
})";

/**
 * Check the lang property is set and dynamic
 */
TEST_F(TextComponentTest, ComponentTextLangDefaults) {
    loadDocument(LANG_TEXT_DEFAULT_DOC);

    auto et = std::dynamic_pointer_cast<CoreComponent>(root->topComponent());
    ASSERT_EQ("en-US", et->getCalculated(kPropertyLang).asString());

    et->setProperty(kPropertyLang, "ja-jp");

    ASSERT_EQ("ja-jp", et->getCalculated(kPropertyLang).asString());
}

static const char* TEXT_ALIGN_DEFAULT = R"({
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "items": [
        {
          "textAlign": "auto",
          "type": "Text"
        },
        {
          "textAlign": "left",
          "type": "Text"
        },
        {
          "textAlign": "right",
          "type": "Text"
        },
        {
          "textAlign": "center",
          "type": "Text"
        },
        {
          "textAlign": "start",
          "type": "Text"
        },
        {
          "textAlign": "end",
          "type": "Text"
        }
      ]
    }
  }
}
)";

/**
 * Check the text align property parses correctly and responds to layout direction change
 */
TEST_F(TextComponentTest, TextAlignParseCheck) {
    loadDocument(TEXT_ALIGN_DEFAULT);

    auto et = std::dynamic_pointer_cast<CoreComponent>(root->topComponent());

    EXPECT_EQ(kTextAlignAuto,   et->getCoreChildAt(0)->getCalculated(kPropertyTextAlign).asInt());
    EXPECT_EQ(kTextAlignAuto,   et->getCoreChildAt(0)->getCalculated(kPropertyTextAlignAssigned).asInt());
    EXPECT_EQ(kTextAlignLeft,   et->getCoreChildAt(1)->getCalculated(kPropertyTextAlign).asInt());
    EXPECT_EQ(kTextAlignLeft,   et->getCoreChildAt(1)->getCalculated(kPropertyTextAlignAssigned).asInt());
    EXPECT_EQ(kTextAlignRight,  et->getCoreChildAt(2)->getCalculated(kPropertyTextAlign).asInt());
    EXPECT_EQ(kTextAlignRight,  et->getCoreChildAt(2)->getCalculated(kPropertyTextAlignAssigned).asInt());
    EXPECT_EQ(kTextAlignCenter, et->getCoreChildAt(3)->getCalculated(kPropertyTextAlign).asInt());
    EXPECT_EQ(kTextAlignCenter, et->getCoreChildAt(3)->getCalculated(kPropertyTextAlignAssigned).asInt());
    EXPECT_EQ(kTextAlignLeft,   et->getCoreChildAt(4)->getCalculated(kPropertyTextAlign).asInt());
    EXPECT_EQ(kTextAlignStart,  et->getCoreChildAt(4)->getCalculated(kPropertyTextAlignAssigned).asInt());
    EXPECT_EQ(kTextAlignRight,  et->getCoreChildAt(5)->getCalculated(kPropertyTextAlign).asInt());
    EXPECT_EQ(kTextAlignEnd,    et->getCoreChildAt(5)->getCalculated(kPropertyTextAlignAssigned).asInt());

    et->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending(); // force layout changes

    EXPECT_EQ(kTextAlignAuto,   et->getCoreChildAt(0)->getCalculated(kPropertyTextAlign).asInt());
    EXPECT_EQ(kTextAlignAuto,   et->getCoreChildAt(0)->getCalculated(kPropertyTextAlignAssigned).asInt());
    EXPECT_EQ(kTextAlignLeft,   et->getCoreChildAt(1)->getCalculated(kPropertyTextAlign).asInt());
    EXPECT_EQ(kTextAlignLeft,   et->getCoreChildAt(1)->getCalculated(kPropertyTextAlignAssigned).asInt());
    EXPECT_EQ(kTextAlignRight,  et->getCoreChildAt(2)->getCalculated(kPropertyTextAlign).asInt());
    EXPECT_EQ(kTextAlignRight,  et->getCoreChildAt(2)->getCalculated(kPropertyTextAlignAssigned).asInt());
    EXPECT_EQ(kTextAlignCenter, et->getCoreChildAt(3)->getCalculated(kPropertyTextAlign).asInt());
    EXPECT_EQ(kTextAlignCenter, et->getCoreChildAt(3)->getCalculated(kPropertyTextAlignAssigned).asInt());
    EXPECT_EQ(kTextAlignRight,  et->getCoreChildAt(4)->getCalculated(kPropertyTextAlign).asInt());
    EXPECT_EQ(kTextAlignStart,  et->getCoreChildAt(4)->getCalculated(kPropertyTextAlignAssigned).asInt());
    EXPECT_EQ(kTextAlignLeft,   et->getCoreChildAt(5)->getCalculated(kPropertyTextAlign).asInt());
    EXPECT_EQ(kTextAlignEnd,    et->getCoreChildAt(5)->getCalculated(kPropertyTextAlignAssigned).asInt());
}

static const char* TEXT_ALIGN_DEFAULT_RTL = R"({
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "layoutDirection": "RTL",
      "items": [
        {
          "textAlign": "start",
          "type": "Text"
        },
        {
          "textAlign": "end",
          "type": "Text"
        }
      ]
    }
  }
}
)";

/**
 * Check that assign property works with RTL layout
 */
TEST_F(TextComponentTest, TextAlignParseCheckRTL) {
    loadDocument(TEXT_ALIGN_DEFAULT_RTL);

    auto et = std::dynamic_pointer_cast<CoreComponent>(root->topComponent());

    EXPECT_EQ(kTextAlignRight, et->getCoreChildAt(0)->getCalculated(kPropertyTextAlign).asInt());
    EXPECT_EQ(kTextAlignStart, et->getCoreChildAt(0)->getCalculated(kPropertyTextAlignAssigned).asInt());
    EXPECT_EQ(kTextAlignLeft,  et->getCoreChildAt(1)->getCalculated(kPropertyTextAlign).asInt());
    EXPECT_EQ(kTextAlignEnd,   et->getCoreChildAt(1)->getCalculated(kPropertyTextAlignAssigned).asInt());

    et->setProperty(kPropertyLayoutDirectionAssigned, "LTR");
    root->clearPending(); // force layout changes

    EXPECT_EQ(kTextAlignLeft,   et->getCoreChildAt(0)->getCalculated(kPropertyTextAlign).asInt());
    EXPECT_EQ(kTextAlignStart,  et->getCoreChildAt(0)->getCalculated(kPropertyTextAlignAssigned).asInt());
    EXPECT_EQ(kTextAlignRight,  et->getCoreChildAt(1)->getCalculated(kPropertyTextAlign).asInt());
    EXPECT_EQ(kTextAlignEnd,    et->getCoreChildAt(1)->getCalculated(kPropertyTextAlignAssigned).asInt());
}

/**
 * Check dynamic changing
 */
TEST_F(TextComponentTest, TextAlignDynCheckRTL) {
    loadDocument(TEXT_ALIGN_DEFAULT_RTL);

    auto et = std::dynamic_pointer_cast<CoreComponent>(root->topComponent());

    EXPECT_EQ(kTextAlignRight, et->getCoreChildAt(0)->getCalculated(kPropertyTextAlign).asInt());
    EXPECT_EQ(kTextAlignStart, et->getCoreChildAt(0)->getCalculated(kPropertyTextAlignAssigned).asInt());
    EXPECT_EQ(kTextAlignLeft,  et->getCoreChildAt(1)->getCalculated(kPropertyTextAlign).asInt());
    EXPECT_EQ(kTextAlignEnd,   et->getCoreChildAt(1)->getCalculated(kPropertyTextAlignAssigned).asInt());

    et->getCoreChildAt(0)->setProperty(kPropertyTextAlignAssigned, "end");
    et->getCoreChildAt(1)->setProperty(kPropertyTextAlignAssigned, "start");
    root->clearPending(); // force layout changes

    EXPECT_EQ(kTextAlignLeft,  et->getCoreChildAt(0)->getCalculated(kPropertyTextAlign).asInt());
    EXPECT_EQ(kTextAlignEnd,   et->getCoreChildAt(0)->getCalculated(kPropertyTextAlignAssigned).asInt());
    EXPECT_EQ(kTextAlignRight, et->getCoreChildAt(1)->getCalculated(kPropertyTextAlign).asInt());
    EXPECT_EQ(kTextAlignStart, et->getCoreChildAt(1)->getCalculated(kPropertyTextAlignAssigned).asInt());
}

/**
 * Check dirty flag is set correclty
 */
TEST_F(TextComponentTest, TextAlignDirtyFlag) {
    loadDocument(TEXT_ALIGN_DEFAULT_RTL);

    auto et = std::dynamic_pointer_cast<CoreComponent>(root->topComponent());

    EXPECT_TRUE(CheckDirty(root));

    // Basic check
    et->getCoreChildAt(0)->setProperty(kPropertyTextAlignAssigned, "left");
    et->getCoreChildAt(1)->setProperty(kPropertyTextAlignAssigned, "right");

    EXPECT_TRUE(CheckDirty(et->getCoreChildAt(0), kPropertyTextAlign));
    EXPECT_TRUE(CheckDirty(et->getCoreChildAt(1), kPropertyTextAlign));

    root->clearPending(); // force layout changes

    EXPECT_TRUE(CheckDirty(et->getCoreChildAt(0)));
    EXPECT_TRUE(CheckDirty(et->getCoreChildAt(1)));

    // layout change WITHOUT start/end text align should not change text align

    et->setProperty(kPropertyLayoutDirectionAssigned, "LTR");
    root->clearPending(); // force layout changes

    EXPECT_TRUE(CheckDirty(et->getCoreChildAt(0), kPropertyLayoutDirection));
    EXPECT_TRUE(CheckDirty(et->getCoreChildAt(1), kPropertyLayoutDirection));

    // start and end is the same as left and right in this layout direction so this shouldn't set the dirty flag

    et->getCoreChildAt(0)->setProperty(kPropertyTextAlignAssigned, "start");
    et->getCoreChildAt(1)->setProperty(kPropertyTextAlignAssigned, "end");

    EXPECT_TRUE(CheckDirty(et->getCoreChildAt(0)));
    EXPECT_TRUE(CheckDirty(et->getCoreChildAt(1)));

    et->getCoreChildAt(0)->setProperty(kPropertyTextAlignAssigned, "end");
    et->getCoreChildAt(1)->setProperty(kPropertyTextAlignAssigned, "start");

    EXPECT_TRUE(CheckDirty(et->getCoreChildAt(0), kPropertyTextAlign));
    EXPECT_TRUE(CheckDirty(et->getCoreChildAt(1), kPropertyTextAlign));

    root->clearPending(); // force layout changes

    // layout change WITH start/end text align should change text align

    et->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending(); // force layout changes

    EXPECT_TRUE(CheckDirty(et->getCoreChildAt(0), kPropertyTextAlign, kPropertyLayoutDirection));
    EXPECT_TRUE(CheckDirty(et->getCoreChildAt(1), kPropertyTextAlign, kPropertyLayoutDirection));
}

const char *AUTO_SEQUENCED_TEXT = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "items": {
        "type": "Sequence",
        "direction": "horizontal",
        "width": "100%",
        "height": "auto",
        "items": {
          "type": "Text",
          "text": "${data}"
        },
        "data": "${Array.range(5)}"
      }
    }
  }
})";

TEST_F(TextComponentTest, AutoSequencedText)
{
    auto ctm = std::make_shared<CountingTextMeasurement>();
    config->measure(ctm);
    loadDocument(AUTO_SEQUENCED_TEXT);
    advanceTime(10);


    // Should have not more than 5 measurements == number of text fields.
    ASSERT_EQ(5, ctm->measures);
    ASSERT_EQ(0, ctm->baselines);
}