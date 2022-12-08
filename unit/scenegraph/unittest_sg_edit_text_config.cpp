/*
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
 *
 */

#include "../testeventloop.h"
#include "test_sg.h"

#include "apl/scenegraph/textpropertiescache.h"

using namespace apl;

class SGEditTextConfig : public ::testing::Test {};

TEST_F(SGEditTextConfig, Basic)
{
    sg::TextPropertiesCache cache;
    auto tp = sg::TextProperties::create(cache,
                                         {"Arial", "Helvetica"},
                                         22.0f,
                                         FontStyle::kFontStyleNormal,
                                         900);

    auto config = sg::EditTextConfig::create(Color::BLUE,
                                             Color::RED,
                                             KeyboardType::kKeyboardTypeEmailAddress,
                                             "en-US",
                                             20,
                                             false,
                                             SubmitKeyType::kSubmitKeyTypeNext,
                                             "a-zA-Z0-9@",
                                             true,
                                             KeyboardBehaviorOnFocus::kBehaviorOnFocusOpenKeyboard,
                                             tp);

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(config->serialize(doc.GetAllocator()), StringToMapObject(R"apl(
        {
            "textColor": "#0000ffff",
            "highlightColor": "#ff0000ff",
            "keyboardType": "emailAddress",
            "keyboardBehaviorOnFocus": "openKeyboard",
            "language": "en-US",
            "maxLength": 20,
            "secureInput": false,
            "selectOnFocus": true,
            "submitKeyType": "next",
            "validCharacters": "a-zA-Z0-9@",
            "textProperties": {
                "fontFamily": ["Arial","Helvetica"],
                "fontSize": 22.0,
                "fontStyle": "normal",
                "fontWeight": 900,
                "letterSpacing": 0.0,
                "lineHeight": 1.25,
                "maxLines": 0,
                "textAlign": "auto",
                "textAlignVertical": "auto"
            }
        }
    )apl")));
}

TEST_F(SGEditTextConfig, ValidateAndStrip)
{
    sg::TextPropertiesCache cache;
    auto tp = sg::TextProperties::create(cache,
                                         {"Arial", "Helvetica"},
                                         22.0f,
                                         FontStyle::kFontStyleNormal,
                                         900);

    auto config = sg::EditTextConfig::create(Color::BLUE,
                                             Color::RED,
                                             KeyboardType::kKeyboardTypeEmailAddress,
                                             "en-US",
                                             10,
                                             false,
                                             SubmitKeyType::kSubmitKeyTypeNext,
                                             "a-zA-Z0-9@",
                                             true,
                                             KeyboardBehaviorOnFocus::kBehaviorOnFocusOpenKeyboard,
                                             tp);

    // Validation
    ASSERT_TRUE(config->validate("abcdeZZ9"));
    ASSERT_FALSE(config->validate("alpha!"));
    ASSERT_FALSE(config->validate("a really long string that is too long"));

    // Strip
    ASSERT_EQ(config->strip("abc"), "abc");
    ASSERT_EQ(config->strip("__ab__c__"), "abc");
    ASSERT_EQ(config->strip("0123456789abcde"), "0123456789");
    ASSERT_EQ(config->strip(u8"ab😀c"), "abc");
}
