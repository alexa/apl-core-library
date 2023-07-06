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

#include "apl/scenegraph/textproperties.h"
#include "apl/scenegraph/textpropertiescache.h"
#include "apl/utils/hash.h"

namespace apl {
namespace sg {

TextPropertiesPtr
TextProperties::create(TextPropertiesCache& cache,
                       std::vector<std::string>&& fontFamily,
                       float fontSize,
                       FontStyle fontStyle,
                       const std::string& language,
                       int fontWeight,
                       float letterSpacing,
                       float lineHeight,
                       int maxLines,
                       TextAlign textAlign,
                       TextAlignVertical textAlignVertical)
{
    size_t hash = 0;
    for (const auto& m : fontFamily)
        hashCombine(hash, m);
    hashCombine(hash, fontSize);
    hashCombine(hash, static_cast<int>(fontStyle));
    hashCombine(hash, language);
    hashCombine(hash, fontWeight);
    hashCombine(hash, letterSpacing);
    hashCombine(hash, lineHeight);
    hashCombine(hash, maxLines);
    hashCombine(hash, static_cast<int>(textAlign));
    hashCombine(hash, static_cast<int>(textAlignVertical));

    auto properties = cache.find(hash);

    // Let's be paranoid and check for a collision
    if (properties) {
        auto *p = properties.get();

        if (p->fontFamily() != fontFamily ||
            p->fontSize() != fontSize ||
            p->fontStyle() != fontStyle ||
            p->language() != language ||
            p->fontWeight() != fontWeight ||
            p->letterSpacing() != letterSpacing ||
            p->lineHeight() != lineHeight ||
            p->maxLines() != maxLines ||
            p->textAlign() != textAlign ||
            p->textAlignVertical() != textAlignVertical)
            properties = nullptr;
    }

    // We need to create a new properties object and add it to the cache
    if (!properties) {
        properties = std::make_shared<TextProperties>();
        auto* p = properties.get();

        p->mFontFamily = std::move(fontFamily);
        p->mFontSize = fontSize;
        p->mFontStyle = fontStyle;
        p->mLanguage = language;
        p->mFontWeight = fontWeight;
        p->mLetterSpacing = letterSpacing;
        p->mLineHeight = lineHeight;
        p->mMaxLines = maxLines;
        p->mTextAlign = textAlign;
        p->mTextAlignVertical = textAlignVertical;

        cache.insert(hash, properties);
    }

    return properties;
}

bool
operator==(const TextProperties& lhs, const TextProperties& rhs)
{
    return lhs.mFontFamily == rhs.mFontFamily &&
           lhs.mFontSize == rhs.mFontSize &&
           lhs.mFontStyle == rhs.mFontStyle &&
           lhs.mLanguage == rhs.mLanguage &&
           lhs.mFontWeight == rhs.mFontWeight &&
           lhs.mLetterSpacing == rhs.mLetterSpacing &&
           lhs.mLineHeight == rhs.mLineHeight &&
           lhs.mMaxLines == rhs.mMaxLines &&
           lhs.mTextAlign == rhs.mTextAlign &&
           lhs.mTextAlignVertical == rhs.mTextAlignVertical;
}

bool
operator!=(const TextProperties& lhs, const TextProperties& rhs)
{
    return !(lhs == rhs);
}

rapidjson::Value
TextProperties::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    auto result = rapidjson::Value(rapidjson::kObjectType);
    auto fonts = rapidjson::Value(rapidjson::kArrayType);
    for (const auto& m : mFontFamily)
        fonts.PushBack(rapidjson::Value(m.c_str(), allocator), allocator);
    result.AddMember("fontFamily", fonts, allocator);
    result.AddMember("fontSize", mFontSize, allocator);
    result.AddMember("fontStyle", rapidjson::Value(sFontStyleMap.at(mFontStyle).c_str(), allocator), allocator);
    result.AddMember("lang", rapidjson::Value(mLanguage.c_str(), allocator), allocator);
    result.AddMember("fontWeight", mFontWeight, allocator);
    result.AddMember("letterSpacing", mLetterSpacing, allocator);
    result.AddMember("lineHeight", mLineHeight, allocator);
    result.AddMember("maxLines", mMaxLines, allocator);
    result.AddMember("textAlign", rapidjson::Value(sTextAlignMap.at(mTextAlign).c_str(), allocator), allocator);
    result.AddMember("textAlignVertical", rapidjson::Value(sTextAlignVerticalMap.at(mTextAlignVertical).c_str(), allocator), allocator);
    return result;
}

} // namespace sg
} // namespace apl
