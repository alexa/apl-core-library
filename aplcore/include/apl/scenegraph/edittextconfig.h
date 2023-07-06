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

#ifndef _APL_EDIT_TEXT_CONFIG_H
#define _APL_EDIT_TEXT_CONFIG_H

#include <rapidjson/document.h>

#include "apl/component/componentproperties.h"
#include "apl/primitives/color.h"
#include "apl/scenegraph/common.h"
#include "apl/utils/userdata.h"

namespace apl {
namespace sg {

/**
 * Settings which control how an edit text control behaves and is displayed
 */
class EditTextConfig : public UserData<EditTextConfig> {
public:
    static EditTextConfigPtr create(Color textColor,
                                    Color highlightColor,
                                    KeyboardType keyboardType,
                                    unsigned int maxLength,
                                    bool secureInput,
                                    SubmitKeyType submitKeyType,
                                    const std::string& validCharacters,
                                    bool selectOnFocus,
                                    KeyboardBehaviorOnFocus keyboardBehaviorOnFocus,
                                    const TextPropertiesPtr& textProperties);

    EditTextConfig() = default;

    Color textColor() const { return mTextColor; }
    Color highlightColor() const { return mHighlightColor; }
    KeyboardType keyboardType() const { return mKeyboardType; }
    bool secureInput() const { return mSecureInput; }
    SubmitKeyType submitKeyType() const { return mSubmitKeyType; }
    bool selectOnFocus() const { return mSelectOnFocus; }
    KeyboardBehaviorOnFocus keyboardBehaviorOnFocus() const { return mKeyboardBehaviorOnFocus; }
    const TextPropertiesPtr& textProperties() const { return mTextProperties; }

    /**
     * Validate a text string against the maximum length and validCharacters properties.
     * @param text The input text string.
     * @return True if this is a valid text string; false if it does not pass.
     */
    bool validate(const std::string& text) const;

    /**
     * Given an input string, strip characters that don't pass validation for maximum length
     * and the valid characters allowed.
     * @param text The input text string.
     * @return The stripped text string.
     */
    std::string strip(const std::string& text);

    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

private:
    Color mTextColor;
    Color mHighlightColor;
    KeyboardType mKeyboardType;
    unsigned int mMaxLength;
    SubmitKeyType mSubmitKeyType;
    std::string mValidCharacters;
    TextPropertiesPtr mTextProperties;
    KeyboardBehaviorOnFocus mKeyboardBehaviorOnFocus;
    unsigned int mSecureInput : 1;
    unsigned int mSelectOnFocus : 1;
};

} // namespace sg
} // namespace apl

#endif // _APL_EDIT_TEXT_CONFIG_H
