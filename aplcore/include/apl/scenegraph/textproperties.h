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

#ifndef _APL_SG_TEXT_PROPERTIES_H
#define _APL_SG_TEXT_PROPERTIES_H

#include <rapidjson/document.h>

#include "apl/scenegraph/common.h"
#include "apl/component/componentproperties.h"
#include "apl/component/textmeasurement.h"
#include "apl/utils/userdata.h"

namespace apl {
namespace sg {

/**
 * Properties needed to layout a TextComponent
 */
class TextProperties : public UserData<TextProperties> {
public:
    static TextPropertiesPtr create(TextPropertiesCache& cache,
                                    std::vector<std::string>&& fontFamily,
                                    float fontSize,
                                    FontStyle fontStyle,
                                    const std::string& language,
                                    int fontWeight,
                                    float letterSpacing = 0,
                                    float lineHeight = 1.25f,
                                    int maxLines = 0,
                                    TextAlign textAlign = kTextAlignAuto,
                                    TextAlignVertical textAlignVertical = kTextAlignVerticalAuto);

    TextProperties() = default;

    const std::vector<std::string>& fontFamily() const { return mFontFamily; }
    float fontSize() const { return mFontSize; }
    FontStyle fontStyle() const { return mFontStyle; }
    std::string language() const { return mLanguage; }
    int fontWeight() const { return mFontWeight; }
    float letterSpacing() const { return mLetterSpacing; }

    float lineHeight() const { return mLineHeight; }
    int maxLines() const { return mMaxLines; }
    TextAlign textAlign() const { return mTextAlign; }
    TextAlignVertical textAlignVertical() const { return mTextAlignVertical; }

    friend bool operator==(const TextProperties& lhs, const TextProperties& rhs);
    friend bool operator!=(const TextProperties& lhs, const TextProperties& rhs);

    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

private:
    std::vector<std::string> mFontFamily;
    float mFontSize;
    FontStyle mFontStyle;
    std::string mLanguage;
    int mFontWeight;
    float mLetterSpacing;

    // These properties are not used in GraphicElementText
    float mLineHeight;
    int mMaxLines;
    TextAlign mTextAlign;
    TextAlignVertical mTextAlignVertical;
};


} // namespace sg
} // namespace apl

#endif // _APL_SG_TEXT_PROPERTIES_H
