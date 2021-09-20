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
#include "apl/component/edittextcomponent.h"
#include "apl/engine/event.h"
#include "apl/primitives/object.h"

using namespace apl;

class EditTextComponentTest : public DocumentWrapper {};

static const char* DEFAULT_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "EditText"
    }
  }
})";

/**
 * Test that the defaults are as expected when no values are set.
 */
TEST_F(EditTextComponentTest, ComponentDefaults) {
    loadDocument(DEFAULT_DOC);

    auto et = root->topComponent();
    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeEditText, et->getType());

    ASSERT_TRUE(IsEqual(Color(Color::TRANSPARENT), et->getCalculated(kPropertyBorderColor)));
    // when not set kPropertyBorderStrokeWidth is initialized from kPropertyBorderWidth
    ASSERT_TRUE(IsEqual(et->getCalculated(kPropertyBorderWidth), et->getCalculated(kPropertyBorderStrokeWidth)));
    ASSERT_TRUE(IsEqual(Dimension(0), et->getCalculated(kPropertyBorderWidth)));
    // kPropertyDrawnBorderWidth is calculated from kPropertyBorderStrokeWidth (inputOnly) and (kPropertyBorderWidth)
    ASSERT_TRUE(IsEqual(Dimension(0), et->getCalculated(kPropertyDrawnBorderWidth)));
    ASSERT_TRUE(IsEqual(Color(0xfafafaff), et->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual("sans-serif", et->getCalculated(kPropertyFontFamily)));
    ASSERT_TRUE(IsEqual(Dimension(40), et->getCalculated(kPropertyFontSize)));
    ASSERT_TRUE(IsEqual(kFontStyleNormal, et->getCalculated(kPropertyFontStyle)));
    ASSERT_TRUE(IsEqual(400, et->getCalculated(kPropertyFontWeight)));
    ASSERT_TRUE(IsEqual(Color(0x00caff4d), et->getCalculated(kPropertyHighlightColor)));
    ASSERT_TRUE(IsEqual("", et->getCalculated(kPropertyHint)));
    ASSERT_TRUE(IsEqual(Color(0xfafafaff), et->getCalculated(kPropertyHintColor)));
    ASSERT_TRUE(IsEqual(kFontStyleNormal, et->getCalculated(kPropertyHintStyle)));
    ASSERT_TRUE(IsEqual(400, et->getCalculated(kPropertyHintWeight)));
    ASSERT_TRUE(IsEqual(kKeyboardTypeNormal, et->getCalculated(kPropertyKeyboardType)));
    ASSERT_TRUE(IsEqual(0, et->getCalculated(kPropertyMaxLength)));
    ASSERT_TRUE(IsEqual(Object::EMPTY_ARRAY(), et->getCalculated(kPropertyOnSubmit)));
    ASSERT_TRUE(IsEqual(Object::EMPTY_ARRAY(), et->getCalculated(kPropertyOnTextChange)));
    ASSERT_TRUE(IsEqual(false, et->getCalculated(kPropertySecureInput)));
    ASSERT_TRUE(IsEqual(false, et->getCalculated(kPropertySelectOnFocus)));
    ASSERT_TRUE(IsEqual(8, et->getCalculated(kPropertySize)));
    ASSERT_TRUE(IsEqual(kSubmitKeyTypeDone, et->getCalculated(kPropertySubmitKeyType)));
    ASSERT_TRUE(IsEqual("", et->getCalculated(kPropertyText)));
    ASSERT_TRUE(IsEqual("", et->getCalculated(kPropertyValidCharacters)));
    ASSERT_TRUE(IsEqual("", et->getCalculated(kPropertyLang)));

    // Should not have scrollable moves
    ASSERT_FALSE(component->allowForward());
    ASSERT_FALSE(component->allowBackwards());

    ASSERT_TRUE(IsEqual(kBehaviorOnFocusSystemDefault, et->getCalculated(kPropertyKeyboardBehaviorOnFocus)));
}


static const char* THEMED_DEFAULT_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "theme": "light",
  "mainTemplate": {
    "item": {
      "type": "EditText"
    }
  }
})";

/**
 * Test that the defaults are as expected when no values are set.
 */
TEST_F(EditTextComponentTest, ComponentThemedDefaults) {
    loadDocument(THEMED_DEFAULT_DOC);

    auto et = root->topComponent();
    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeEditText, et->getType());

    ASSERT_TRUE(IsEqual(Color(0x1e2222ff), et->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual(Color(0x0070ba4d), et->getCalculated(kPropertyHighlightColor)));
    ASSERT_TRUE(IsEqual(Color(0x1e2222ff), et->getCalculated(kPropertyHintColor)));
}

static const char* LANG_DEFAULT_DOC = R"({
  "type": "APL",
  "version": "1.7",
  "lang": "en-US",
  "mainTemplate": {
    "item": {
      "type": "EditText"
    }
  }
})";

/**
 * Enforce that the value is "" if rootconfig and doc level lang properties are not set
 */
TEST_F(EditTextComponentTest, ComponentLangDefaults) {
    loadDocument(THEMED_DEFAULT_DOC);

    auto et = root->topComponent();
    ASSERT_EQ("", et->getCalculated(kPropertyLang).asString());
}

/**
 * Enforce that we are shadowing the rootconfig value if doc level lang property is not set
 */
TEST_F(EditTextComponentTest, ComponentLangDefaultsRootConfig) {
    config->set(RootProperty::kLang, "en-US");
    loadDocument(THEMED_DEFAULT_DOC);

    auto et = root->topComponent();
    ASSERT_EQ("en-US", et->getCalculated(kPropertyLang).asString());
}

/**
 * Enforce that we are shadowing the doc level lang property
 */
TEST_F(EditTextComponentTest, ComponentLangDefaultsDocumentLevel) {
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
      "type": "EditText"
    }
  }
})";

/**
 * Check the lang property is set and dynamic
 */
TEST_F(EditTextComponentTest, ComponentTextLangDefaults) {
    loadDocument(LANG_TEXT_DEFAULT_DOC);

    auto et = std::dynamic_pointer_cast<CoreComponent>(root->topComponent());
    ASSERT_EQ("en-US", et->getCalculated(kPropertyLang).asString());

    et->setProperty(kPropertyLang, "ja-jp");

    ASSERT_EQ("ja-jp", et->getCalculated(kPropertyLang).asString());
}


static const char* NON_DEFAULT_DOC = R"({
  "type": "APL",
  "version": "1.r",
  "mainTemplate": {
    "item": {
      "type": "EditText",
      "borderColor": "blue",
      "borderStrokeWidth": 20,
      "borderWidth": 30,
      "color": "yellow",
      "fontFamily": "ember",
      "fontSize": 24,
      "fontStyle": "italic",
      "fontWeight": 600,
      "hint": "hint",
      "highlightColor": "green",
      "hintColor": "gray",
      "hintStyle": "italic",
      "hintWeight": 500,
      "keyboardType": "numberPad",
      "maxLength": 4,
      "onSubmit": [
        {
          "type": "SetValue",
          "componentId": "myEditText",
          "property": "color",
          "value": "blue"
        }
      ],
      "onTextChange": [
        {
          "type": "SetValue",
          "componentId": "myEditText",
          "property": "color",
          "value": "red"
        }
      ],
      "secureInput": true,
      "selectOnFocus": true,
      "size": 4,
      "submitKeyType": "go",
      "text": "1234",
      "validCharacters": "0-9"
    }
  }
})";

/**
 * Test the setting of all properties to non default values.
 */
TEST_F(EditTextComponentTest, NonDefaults) {

    loadDocument(NON_DEFAULT_DOC);

    auto et = root->topComponent();
    ASSERT_TRUE(et);
    ASSERT_EQ(kComponentTypeEditText, et->getType());

    ASSERT_TRUE(IsEqual(Color(Color::BLUE), et->getCalculated(kPropertyBorderColor)));
    ASSERT_TRUE(IsEqual(Dimension(20), et->getCalculated(kPropertyBorderStrokeWidth)));
    ASSERT_TRUE(IsEqual(Dimension(30), et->getCalculated(kPropertyBorderWidth)));
    // kPropertyDrawnBorderWidth is calculated from kPropertyBorderStrokeWidth (inputOnly) and (kPropertyBorderWidth)
    // it is the minimum of the two
    ASSERT_TRUE(IsEqual(et->getCalculated(kPropertyBorderStrokeWidth), et->getCalculated(kPropertyDrawnBorderWidth)));
    ASSERT_TRUE(IsEqual(Color(Color::YELLOW), et->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual("ember", et->getCalculated(kPropertyFontFamily)));
    ASSERT_TRUE(IsEqual(Dimension(24), et->getCalculated(kPropertyFontSize)));
    ASSERT_TRUE(IsEqual(kFontStyleItalic, et->getCalculated(kPropertyFontStyle)));
    ASSERT_TRUE(IsEqual(600, et->getCalculated(kPropertyFontWeight)));
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), et->getCalculated(kPropertyHighlightColor)));
    ASSERT_TRUE(IsEqual("hint", et->getCalculated(kPropertyHint)));
    ASSERT_TRUE(IsEqual(Color(Color::GRAY), et->getCalculated(kPropertyHintColor)));
    ASSERT_TRUE(IsEqual(kFontStyleItalic, et->getCalculated(kPropertyHintStyle)));
    ASSERT_TRUE(IsEqual(500, et->getCalculated(kPropertyHintWeight)));
    ASSERT_TRUE(IsEqual(kKeyboardTypeNumberPad, et->getCalculated(kPropertyKeyboardType)));
    ASSERT_TRUE(IsEqual(4, et->getCalculated(kPropertyMaxLength)));
    auto submit = et->getCalculated(kPropertyOnSubmit);
    ASSERT_EQ(Object::kArrayType, submit.getType());
    ASSERT_EQ(1, submit.getArray().size());
    auto change = et->getCalculated(kPropertyOnTextChange);
    ASSERT_EQ(Object::kArrayType, submit.getType());
    ASSERT_EQ(1, change.getArray().size());
    ASSERT_TRUE(IsEqual(true, et->getCalculated(kPropertySecureInput)));
    ASSERT_TRUE(IsEqual(true, et->getCalculated(kPropertySelectOnFocus)));
    ASSERT_TRUE(IsEqual(4, et->getCalculated(kPropertySize)));
    ASSERT_TRUE(IsEqual(kSubmitKeyTypeGo, et->getCalculated(kPropertySubmitKeyType)));
    ASSERT_TRUE(IsEqual("1234", et->getCalculated(kPropertyText)));
    ASSERT_TRUE(IsEqual("0-9", et->getCalculated(kPropertyValidCharacters)));
    ASSERT_TRUE(IsEqual(kBehaviorOnFocusSystemDefault, et->getCalculated(kPropertyKeyboardBehaviorOnFocus)));
}

static const char* VALID_CHARACTER_RANGES_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "EditText",
      "validCharacters": "0-9a-yA-Y:-@"
    }
  }
})";

/**
 * Test the isCharacterValid method, valid ranges
 */
TEST_F(EditTextComponentTest, ValidCharacterRanges) {

    loadDocument(VALID_CHARACTER_RANGES_DOC);
    EditTextComponent* pEditText = dynamic_cast<EditTextComponent*>(root->topComponent().get());
    ASSERT_TRUE(pEditText);
    ASSERT_EQ(kComponentTypeEditText, pEditText->getType());

    ASSERT_TRUE(pEditText->isCharacterValid(L'0'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'9'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'A'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'Y'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'a'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'y'));
    ASSERT_FALSE(pEditText->isCharacterValid(L'-'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'@'));
    ASSERT_TRUE(pEditText->isCharacterValid(L':'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'?'));
    ASSERT_FALSE(pEditText->isCharacterValid(L'z'));
    ASSERT_FALSE(pEditText->isCharacterValid(L'Z'));
    ASSERT_FALSE(pEditText->isCharacterValid(L'{'));
    ASSERT_FALSE(pEditText->isCharacterValid(L'\u2192'));
}

static const char* VALID_CHARACTER_RANGES_UNICODE_DOC =
        u8"{"
        u8"  \"type\": \"APL\","
        u8"  \"version\": \"1.4\","
        u8"  \"theme\": \"light\","
        u8"  \"mainTemplate\": {"
        u8"    \"item\": {"
        u8"      \"type\": \"EditText\","
        u8"      \"validCharacters\": \"\u2192-\u2195\""
        u8"    }"
        u8"  }"
        u8"}";

/**
 * Test the isCharacterValid method, valid ranges
 */
TEST_F(EditTextComponentTest, ValidCharacterRangesUnicode) {

    loadDocument(VALID_CHARACTER_RANGES_UNICODE_DOC);
    EditTextComponent* pEditText = dynamic_cast<EditTextComponent*>(root->topComponent().get());
    ASSERT_TRUE(pEditText);
    ASSERT_EQ(kComponentTypeEditText, pEditText->getType());

    ASSERT_TRUE(pEditText->isCharacterValid(L'\u2192'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'\u2193'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'\u2195'));
    ASSERT_FALSE(pEditText->isCharacterValid(L'\u2196'));
}

static const char* EMPTY_CHARACTER_RANGES_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "theme": "light",
  "mainTemplate": {
    "item": {
      "type": "EditText"
    }
  }
})";

TEST_F(EditTextComponentTest, EmptyCharacterRanges) {

    loadDocument(EMPTY_CHARACTER_RANGES_DOC);
    EditTextComponent* pEditText = dynamic_cast<EditTextComponent*>(root->topComponent().get());
    ASSERT_TRUE(pEditText);
    ASSERT_EQ(kComponentTypeEditText, pEditText->getType());

    // everything should be valid
    ASSERT_TRUE(pEditText->isCharacterValid(L'\u2192'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'-'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'A'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'0'));

    session->clear();
}

static const char* INVALID_CHARACTER_RANGES_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "theme": "light",
  "mainTemplate": {
    "item": {
      "type": "EditText",
      "validCharacters": "Q--"
    }
  }
})";

TEST_F(EditTextComponentTest, InvalidCharacterRanges) {

    loadDocument(INVALID_CHARACTER_RANGES_DOC);
    EditTextComponent* pEditText = dynamic_cast<EditTextComponent*>(root->topComponent().get());
    ASSERT_TRUE(pEditText);
    ASSERT_EQ(kComponentTypeEditText, pEditText->getType());

    // everything should be valid
    ASSERT_TRUE(pEditText->isCharacterValid(L'\u2192'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'-'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'A'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'0'));

    session->clear();
}

static const char* INVALID_DASH_CHARACTER_RANGES_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "EditText",
      "validCharacters": "0-9a-yA-Y--@"
    }
  }
})";

TEST_F(EditTextComponentTest, InvalidDashCharacterRanges) {

    loadDocument(INVALID_DASH_CHARACTER_RANGES_DOC);
    EditTextComponent* pEditText = dynamic_cast<EditTextComponent*>(root->topComponent().get());
    ASSERT_TRUE(pEditText);
    ASSERT_EQ(kComponentTypeEditText, pEditText->getType());

    // everything should be valid
    ASSERT_TRUE(pEditText->isCharacterValid(L'\u2192'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'-'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'A'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'0'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'}'));

    session->clear();
}

static const char* AMOUNT_CHARACTER_RANGES_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "EditText",
      "validCharacters": "0-9."
    }
  }
})";

TEST_F(EditTextComponentTest, AmountCharacterRanges) {

    loadDocument(AMOUNT_CHARACTER_RANGES_DOC);
    EditTextComponent* pEditText = dynamic_cast<EditTextComponent*>(root->topComponent().get());
    ASSERT_TRUE(pEditText);
    ASSERT_EQ(kComponentTypeEditText, pEditText->getType());

    ASSERT_TRUE(pEditText->isCharacterValid(L'0'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'5'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'7'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'9'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'.'));
    ASSERT_FALSE(pEditText->isCharacterValid(L'A'));
    ASSERT_FALSE(pEditText->isCharacterValid(L'@'));
    ASSERT_FALSE(pEditText->isCharacterValid(L'-'));
    ASSERT_FALSE(pEditText->isCharacterValid(L'\u2192'));
}

static const char* EMAIL_CHARACTER_RANGES_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "EditText",
      "validCharacters": "-+a-zA-Z0-9_@."
    }
  }
})";

TEST_F(EditTextComponentTest, EmailCharacterRanges) {

    loadDocument(EMAIL_CHARACTER_RANGES_DOC);
    EditTextComponent* pEditText = dynamic_cast<EditTextComponent*>(root->topComponent().get());
    ASSERT_TRUE(pEditText);
    ASSERT_EQ(kComponentTypeEditText, pEditText->getType());

    ASSERT_TRUE(pEditText->isCharacterValid(L'-'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'+'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'a'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'p'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'z'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'A'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'P'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'Z'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'0'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'5'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'7'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'9'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'_'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'@'));
    ASSERT_TRUE(pEditText->isCharacterValid(L'.'));
    ASSERT_FALSE(pEditText->isCharacterValid(L':'));
    ASSERT_FALSE(pEditText->isCharacterValid(L'\u2192'));
}

static const char* INVALID_DIMENSIONS_DOC = R"({
  "type": "APL",
  "version": "1.r",
  "mainTemplate": {
    "item": {
      "type": "EditText",
      "borderStrokeWidth": -20,
      "borderWidth": -30,
      "size": -44
    }
  }
})";

/**
 * Test the setting of all properties to non default values.
 */
TEST_F(EditTextComponentTest, InvalidDimensions) {

    loadDocument(INVALID_DIMENSIONS_DOC);

    auto et = root->topComponent();
    ASSERT_TRUE(et);
    ASSERT_EQ(kComponentTypeEditText, et->getType());

    ASSERT_TRUE(IsEqual(Dimension(0), et->getCalculated(kPropertyBorderStrokeWidth)));
    ASSERT_TRUE(IsEqual(Dimension(0), et->getCalculated(kPropertyBorderWidth)));
    // kPropertyDrawnBorderWidth is calculated from kPropertyBorderStrokeWidth (inputOnly) and (kPropertyBorderWidth)
    // it is the minimum of the two
    ASSERT_TRUE(IsEqual(Dimension(0), et->getCalculated(kPropertyDrawnBorderWidth)));
    ASSERT_TRUE(IsEqual(1, et->getCalculated(kPropertySize)));
}


static const char* BORDER_STROKE_CLAMP_DOC = R"({
  "type": "APL",
  "version": "1.r",
  "mainTemplate": {
    "item": {
      "type": "EditText",
      "id": "myEditText",
      "borderStrokeWidth": 64,
      "borderWidth": 30
    }
  }
})";

static const char * SET_VALUE_STROKEWIDTH_COMMAND = R"([
  {
    "type": "SetValue",
    "componentId": "myEditText",
    "property": "borderStrokeWidth",
    "value": "17"
  }
])";

/**
 * Test the setting of all properties to non default values.
 */
TEST_F(EditTextComponentTest, ClampDrawnBorder) {

    loadDocument(BORDER_STROKE_CLAMP_DOC);

    auto et = root->topComponent();
    ASSERT_TRUE(et);
    ASSERT_EQ(kComponentTypeEditText, et->getType());

    ASSERT_TRUE(IsEqual(Dimension(30), et->getCalculated(kPropertyBorderWidth)));
    ASSERT_TRUE(IsEqual(Dimension(64), et->getCalculated(kPropertyBorderStrokeWidth)));
    // kPropertyDrawnBorderWidth is calculated from kPropertyBorderStrokeWidth (inputOnly) and (kPropertyBorderWidth)
    // and is clamped to kPropertyBorderWidth
    ASSERT_TRUE(IsEqual(Dimension(30), et->getCalculated(kPropertyDrawnBorderWidth)));

    // execute command to set kPropertyBorderStrokeWidth within border bounds,
    // the drawn border should update
    auto doc = rapidjson::Document();
    doc.Parse(SET_VALUE_STROKEWIDTH_COMMAND);
    auto action = root->executeCommands(apl::Object(std::move(doc)), false);
    ASSERT_TRUE(IsEqual(Dimension(17), et->getCalculated(kPropertyBorderStrokeWidth)));
    ASSERT_TRUE(IsEqual(Dimension(17), et->getCalculated(kPropertyDrawnBorderWidth)));
}

static const char* HANDLERS_DOC = R"({
  "type": "APL",
  "version": "1.r",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "items": [
        {
          "type": "EditText",
          "id": "myEditText",
          "text": "hello",
          "onSubmit": [
            {
              "type": "SetValue",
              "componentId": "myEditText",
              "property": "color",
              "value": "blue"
            },
            {
              "type": "SetValue",
              "componentId": "myResult",
              "property": "text",
              "value": "${event.source.handler}:${event.source.value}"
            }
          ],
          "onTextChange": [
            {
              "type": "SetValue",
              "componentId": "myEditText",
              "property": "color",
              "value": "red"
            },
            {
              "type": "SetValue",
              "componentId": "myResult",
              "property": "text",
              "value": "${event.source.handler}:${event.source.value}"
            }
          ]
        },
        {
          "type": "Text",
          "id": "myResult"
        }
      ]
    }
  }
})";

/**
 * Test the event handlers for onSubmit and onTextChange
 */
TEST_F(EditTextComponentTest, Handlers) {
    loadDocument(HANDLERS_DOC);

    auto top = root->topComponent();
    ASSERT_TRUE(top);
    auto et = top->findComponentById("myEditText");
    ASSERT_TRUE(et);
    ASSERT_EQ(kComponentTypeEditText, et->getType());
    auto result = top->findComponentById("myResult");
    ASSERT_TRUE(result);
    ASSERT_EQ((kComponentTypeText), result->getType());

    // press the submit button and advance time
    et->update(kUpdateSubmit);
    loop->advanceToEnd();

    CheckDirty(root, et, result);
    CheckDirty(et, kPropertyColor);
    CheckDirty(result, kPropertyText);
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), et->getCalculated(kPropertyColor)));
    auto resultTxt = result->getCalculated(kPropertyText);
    ASSERT_TRUE(resultTxt.isStyledText());
    ASSERT_TRUE(IsEqual("Submit:hello", resultTxt.getStyledText().getRawText()));
    root->clearDirty();

    et->update(kUpdateTextChange, "goodbye");
    loop->advanceToEnd();

    CheckDirty(root, et, result);
    CheckDirty(et, kPropertyText, kPropertyColor);
    CheckDirty(result, kPropertyText);
    ASSERT_TRUE(IsEqual("goodbye", et->getCalculated(kPropertyText)));
    ASSERT_TRUE(IsEqual(Color(Color::RED), et->getCalculated(kPropertyColor)));
    resultTxt = result->getCalculated(kPropertyText);
    ASSERT_TRUE(resultTxt.isStyledText());
    ASSERT_TRUE(IsEqual("TextChange:goodbye", resultTxt.getStyledText().getRawText()));
    root->clearDirty();
}


static const char* STYLED_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "styles": {
    "myStyle": {
      "values": [
        {
          "borderColor": "blue",
          "borderStrokeWidth": 20,
          "borderWidth": 30,
          "color": "yellow",
          "fontFamily": "ember",
          "fontSize": 24,
          "fontStyle": "italic",
          "fontWeight": 600,
          "hint": "hint",
          "highlightColor": "green",
          "hintColor": "gray",
          "hintStyle": "italic",
          "hintWeight": 500,
          "keyboardType": "numberPad",
          "maxLength": 4,
          "secureInput": true,
          "selectOnFocus": true,
          "size": 4,
          "submitKeyType": "go",
          "text": "1234",
          "validCharacters": "0-9"
        }
      ]
    }
  },
  "mainTemplate": {
    "item": {
      "type": "EditText",
      "style": "myStyle"
    }
  }
})";

/**
 * Verify styled properties can be set via style, and non-styled properties cannot be set via style
 */
TEST_F(EditTextComponentTest, Styled) {
    loadDocument(STYLED_DOC);

    auto et = root->topComponent();
    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeEditText, et->getType());

    // These are styled
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), et->getCalculated(kPropertyBorderColor)));
    // kPropertyDrawnBorderWidth is calculated from kPropertyBorderStrokeWidth (inputOnly) and (kPropertyBorderWidth)
    ASSERT_TRUE(IsEqual(Dimension(20), et->getCalculated(kPropertyDrawnBorderWidth)));
    ASSERT_TRUE(IsEqual(Dimension(30), et->getCalculated(kPropertyBorderWidth)));
    ASSERT_TRUE(IsEqual(Color(Color::YELLOW), et->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual("ember", et->getCalculated(kPropertyFontFamily)));
    ASSERT_TRUE(IsEqual(Dimension(24), et->getCalculated(kPropertyFontSize)));
    ASSERT_TRUE(IsEqual(kFontStyleItalic, et->getCalculated(kPropertyFontStyle)));
    ASSERT_TRUE(IsEqual(600, et->getCalculated(kPropertyFontWeight)));
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), et->getCalculated(kPropertyHighlightColor)));
    ASSERT_TRUE(IsEqual("hint", et->getCalculated(kPropertyHint)));
    ASSERT_TRUE(IsEqual(Color(Color::GRAY), et->getCalculated(kPropertyHintColor)));
    ASSERT_TRUE(IsEqual(kFontStyleItalic, et->getCalculated(kPropertyHintStyle)));
    ASSERT_TRUE(IsEqual(500, et->getCalculated(kPropertyHintWeight)));
    ASSERT_TRUE(IsEqual(kKeyboardTypeNumberPad, et->getCalculated(kPropertyKeyboardType)));
    ASSERT_TRUE(IsEqual(4, et->getCalculated(kPropertyMaxLength)));
    auto submit = et->getCalculated(kPropertyOnSubmit);
    ASSERT_TRUE(IsEqual(true, et->getCalculated(kPropertySecureInput)));
    ASSERT_TRUE(IsEqual(true, et->getCalculated(kPropertySelectOnFocus)));
    ASSERT_TRUE(IsEqual(4, et->getCalculated(kPropertySize)));
    ASSERT_TRUE(IsEqual(kSubmitKeyTypeGo, et->getCalculated(kPropertySubmitKeyType)));
    ASSERT_TRUE(IsEqual("0-9", et->getCalculated(kPropertyValidCharacters)));

    // Style should be ignored
    ASSERT_FALSE(IsEqual("1234", et->getCalculated(kPropertyText)));
}

class DummyTextMeasure : public TextMeasurement {
public:
    LayoutSize measure(Component *component, float width, MeasureMode widthMode, float height,
                       MeasureMode heightMode) override {
        int size = component->getCalculated(kPropertySize).asInt();
        int componentWidth = component->getCalculated(kPropertyWidth).asInt();
        int componentHeight = component->getCalculated(kPropertyHeight).asInt();

        float resultingWidth = size > 0 ? size*20 : componentWidth;
        float resultingHeight = componentHeight > 0 ? componentHeight : 120;

        return LayoutSize({ resultingWidth, resultingHeight});
    }

    float baseline(Component *component, float width, float height) override {
        return 0;
    }
};

static const char* EDITTEXT_MEASUREMENT_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "borderWidth": 2,
      "item": {
        "type": "EditText",
        "text": "Hello",
        "size": 3,
        "color": "#000000"
      }
    }
  }
})";

/**
 * Test text measurement for EditText component
 */
TEST_F(EditTextComponentTest, EditTextMeasurement) {

    // Load the main document
    auto content = Content::create(EDITTEXT_MEASUREMENT_DOC, makeDefaultSession());
    ASSERT_TRUE(content);

    // Inflate the document
    auto metrics = Metrics().size(800,800).dpi(320);
    auto measure = std::make_shared<DummyTextMeasure>();
    RootConfig rootConfig = RootConfig().measure(measure);
    auto root = RootContext::create( metrics, content, rootConfig );
    ASSERT_TRUE(root);

    // Check the layout
    auto top = root->topComponent().get();
    ASSERT_EQ(Rect(0, 0, 400, 400), top->getCalculated(kPropertyBounds).getRect());
    auto editText = top->getChildAt(0);
    ASSERT_EQ(Rect(2, 2, 60, 120), editText->getCalculated(kPropertyBounds).getRect());
}

/**
 * Test that when update of text done - component marked as dirty.
 */
TEST_F(EditTextComponentTest, UpdateMarksDirty) {
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureMarkEditTextDirtyOnUpdate);
    loadDocument(DEFAULT_DOC);

    auto et = root->topComponent();
    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeEditText, et->getType());

    et->update(kUpdateTextChange, "test");
    ASSERT_TRUE(CheckDirty(et, kPropertyText, kPropertyVisualHash));
}

static const char* OPEN_KEYBOARD_EVENT_DOC = R"(
{
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "items": [
        {
          "type": "TouchWrapper",
          "id": "btn",
          "item": {
            "type": "Text",
            "text": "Edit"
          },
          "onPress":[
            {
              "type": "SetFocus",
              "componentId": "stickyNote"
            }
          ]
        },
        {
          "type": "EditText",
          "id": "stickyNote",
          "size": 10,
          "selectOnFocus": false,
          "-keyboardBehaviorOnFocus": "openKeyboard",
          "text": "MyText"
        }
      ]
    }
  }
}
)";

/**
 * Verify OpenKeyboard type event is generated at a time of setting focus on edittext component
 */
TEST_F(EditTextComponentTest, OpenKeyboardEventOnFocus) {

    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureRequestKeyboard);
    loadDocument(OPEN_KEYBOARD_EVENT_DOC);

    performClick(0, 0);
    loop->advanceToEnd();

    auto edittext = component->findComponentById("stickyNote");
    ASSERT_TRUE(edittext);
    ASSERT_EQ(kComponentTypeEditText, edittext->getType());
    ASSERT_TRUE(IsEqual(kBehaviorOnFocusOpenKeyboard, edittext->getCalculated(kPropertyKeyboardBehaviorOnFocus)));

    auto hasEvent = root->hasEvent();
    ASSERT_TRUE(hasEvent);
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeOpenKeyboard, event.getType());
    ASSERT_EQ(edittext, event.getComponent());

    hasEvent = root->hasEvent();
    ASSERT_TRUE(hasEvent);
    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(edittext, event.getComponent());
}
