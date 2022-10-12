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

#include "apl/scenegraph/edittextconfig.h"
#include "apl/scenegraph/textproperties.h"
#include "apl/primitives/unicode.h"

namespace apl {
namespace sg {

EditTextConfigPtr
EditTextConfig::create(Color textColor,
                        Color highlightColor,
                        KeyboardType keyboardType,
                        const std::string& language,
                        unsigned int maxLength,
                        bool secureInput,
                        SubmitKeyType submitKeyType,
                        const std::string& validCharacters,
                        bool selectOnFocus,
                        KeyboardBehaviorOnFocus keyboardBehaviorOnFocus,
                        const TextPropertiesPtr& textProperties)
{
    auto result = std::make_shared<EditTextConfig>();

    auto* ptr = result.get();
    ptr->mTextColor = textColor;
    ptr->mHighlightColor = highlightColor;
    ptr->mKeyboardType = keyboardType;
    ptr->mLanguage = language;
    ptr->mMaxLength = maxLength;
    ptr->mSecureInput = secureInput;
    ptr->mSubmitKeyType = submitKeyType;
    ptr->mValidCharacters = validCharacters;
    ptr->mSelectOnFocus = selectOnFocus;
    ptr->mKeyboardBehaviorOnFocus = keyboardBehaviorOnFocus;
    ptr->mTextProperties = textProperties;

    return result;
}

bool
EditTextConfig::validate(const std::string& text) const
{
    auto len = utf8StringLength(text);
    if (len == -1 || (mMaxLength > 0 && len > mMaxLength))
        return false;

    return utf8ValidCharacters(text, mValidCharacters);
}

std::string
EditTextConfig::strip(const std::string& text)
{
    return utf8StripInvalidAndTrim(text, mValidCharacters, mMaxLength);
}

rapidjson::Value
EditTextConfig::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = rapidjson::Value(rapidjson::kObjectType);

    result.AddMember("textColor", mTextColor.serialize(allocator), allocator);
    result.AddMember("highlightColor", mHighlightColor.serialize(allocator), allocator);
    result.AddMember("keyboardType",
                     rapidjson::Value(sKeyboardTypeMap.at(mKeyboardType).c_str(), allocator),
                     allocator);
    result.AddMember(
        "keyboardBehaviorOnFocus",
        rapidjson::Value(sKeyboardBehaviorOnFocusMap.at(mKeyboardBehaviorOnFocus).c_str(),
                         allocator),
        allocator);
    result.AddMember("language", rapidjson::Value(mLanguage.c_str(), allocator), allocator);
    result.AddMember("maxLength", mMaxLength, allocator);
    result.AddMember("secureInput", static_cast<bool>(mSecureInput), allocator);
    result.AddMember("selectOnFocus", static_cast<bool>(mSelectOnFocus), allocator);
    result.AddMember("submitKeyType",
                     rapidjson::Value(sSubmitKeyTypeMap.at(mSubmitKeyType).c_str(), allocator),
                     allocator);
    result.AddMember("validCharacters", rapidjson::Value(mValidCharacters.c_str(), allocator),
                     allocator);
    result.AddMember("textProperties", mTextProperties->serialize(allocator), allocator);

    return result;
}

} // namespace sg
} // namespace apl
