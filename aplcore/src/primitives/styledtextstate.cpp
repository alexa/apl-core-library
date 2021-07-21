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

#include "apl/engine/evaluate.h"
#include "apl/primitives/styledtextstate.h"

#include <codecvt>
#include <locale>
#include <set>

namespace apl {

const std::string INHERIT_ATTRIBUTE_VALUE = "inherit";

const std::map<std::string, StyledText::SpanAttributeName> sSpanAttributeMap = {
    {"color", StyledText::kSpanAttributeNameColor},
    {"fontSize", StyledText::kSpanAttributeNameFontSize},
};

const std::map<std::string, StyledText::SpanType> sTextSpanTypeMap = {
    {"br", StyledText::kSpanTypeLineBreak},
    {"strong", StyledText::kSpanTypeStrong},
    {"b", StyledText::kSpanTypeStrong},
    {"em", StyledText::kSpanTypeItalic},
    {"i", StyledText::kSpanTypeItalic},
    {"strike", StyledText::kSpanTypeStrike},
    {"u", StyledText::kSpanTypeUnderline},
    {"tt", StyledText::kSpanTypeMonospace},
    {"code", StyledText::kSpanTypeMonospace},
    {"sup", StyledText::kSpanTypeSuperscript},
    {"sub", StyledText::kSpanTypeSubscript},
    {"nobr", StyledText::kSpanTypeNoBreak},
    {"span", StyledText::kSpanTypeSpan},
};

const std::set<std::string> sAttributableTags = {
    "span",
};

// Only some tags can be merged. For example "<b>te</b><b>xt</b>" can become "<b>text</b>"
const std::set<StyledText::SpanType> sMergeableTags = {
    StyledText::kSpanTypeLineBreak,
    StyledText::kSpanTypeStrong,
    StyledText::kSpanTypeItalic,
    StyledText::kSpanTypeStrike,
    StyledText::kSpanTypeUnderline,
    StyledText::kSpanTypeMonospace,
    StyledText::kSpanTypeSuperscript,
    StyledText::kSpanTypeSubscript
};

/**
 * Character converter to be used for multi-byte sized characters.
 * Static variables are contained within a single compilation unit (file)
 */
#ifdef APL_CORE_UWP
// avoids a runtime crash on UWP
// see:  https://tinyurl.com/ra2coob
static std::wstring_convert<std::codecvt_utf8<wchar_t>> wchar_converter;
#else
static std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> wchar_converter;
#endif

/**
 * Internal utility to convert string to lowercase.
 * Not using transform as it does it inplace.
 * Applicable only to latin characters.
 * @param str string to process.
 * @return lowercase version of str.
 */
static inline std::string tolower(const std::string& str) {
    std::string output = str;

    std::transform(output.begin(), output.end(), output.begin(), [](unsigned char ch) {
      return std::tolower(ch);
    });

    return output;
}

void
StyledTextState::append(const std::string& val) {
    wchar_converter.to_bytes(wchar_converter.from_bytes(val.c_str()));

    // Get real number of characters.
    mPosition += wchar_converter.converted();
    mText = mText.append(val);

    // After appending any raw value we disable whitespace collapse
    mProcessingWhiteSpace = false;
}

void
StyledTextState::space() {
    // Skip if we are collapsing whitespaces
    if (mProcessingWhiteSpace) {
        mAllowMerge = false;
        return;
    }

    mPosition += 1;
    mText = mText.append(" ");

    // If we are not collapsing whitespaces, enable it
    mProcessingWhiteSpace = true;
    mAllowMerge = false;
}

void
StyledTextState::attributeName(const std::string& attributeName) {
    mCurrentAttributeName = attributeName;
}

void
StyledTextState::attributeValue(const std::string& attributeValue) {
    if (mCurrentAttributeName.empty()) {
        return;
    }

    // Skip if current tag can't be attributed
    if (!sAttributableTags.count(mCurrentTag)) {
        mCurrentAttributeName = "";
        return;
    }

    emplaceAttribute(attributeValue);
}

void
StyledTextState::tag(const std::string& tag) {
    mCurrentTag = tolower(tag);
}

void
StyledTextState::start() {
    auto typeIt = sTextSpanTypeMap.find(mCurrentTag);
    if (typeIt == sTextSpanTypeMap.end()) {
        return;
    }

    auto type = typeIt->second;
    size_t start = mPosition;
    // If type same as previously closed tag - merge it instead of creating new one.
    // TODO: Not ideal but simple enough. Nested merging to be added if we see problems with viewhost performance.
    if (!mSpans.empty()) {
        auto previous = mSpans.back();
        if (canMergeSpans(previous, type, mPosition)) {
            start = previous.start;
            mSpans.pop_back();
        }
    }

    mOpenedSpans[type] += 1;
    mBuildStack.push(StyledText::Span(start, type, mCurrentAttributeMap));

    mCurrentAttributeMap.clear();

    // Do not allow merging after a start tag, only after and end tag
    mAllowMerge = false;
}

bool
StyledTextState::canMergeSpans(const StyledText::Span& previousSpan,
                               const StyledText::SpanType& currentSpanType,
                               size_t currentPosition) const {
    // <b>hello</b> <b>world</b> is 2 spans, raw text is "hello world"
    // <b> hello </b> <b>world</b> is 2 spans, raw text is "hello wold"
    // Both represents the same string "hello world" but because of the whitespace merge
    // that does not increment the current position on the rawOutputText, we may end up on a situation where start and end are the same position and the end() method will try to merge both producing inconsistencies.
    if (!mAllowMerge)
        return false;
    return sMergeableTags.find(currentSpanType) != sMergeableTags.end() &&
           previousSpan.type == currentSpanType && previousSpan.end == currentPosition;
}

void
StyledTextState::end() {
    auto typeIt = sTextSpanTypeMap.find(mCurrentTag);
    if (typeIt == sTextSpanTypeMap.end()) {
        return;
    }

    auto type = typeIt->second;

    if (mOpenedSpans[type] == 0) {
        // No opened tag available
        return;
    }

    auto span = mBuildStack.top();
    mBuildStack.pop();

    // If start == end then span is unnecessary so don't record it
    if (span.start != mPosition) {
        span.end = mPosition;
        mSpans.emplace_back(span);
    }

    // If not equal then likely tag is unclosed so close it (above) and try again. This will split intersecting/wrongly closed tags to separate spans in order to simplify view-host processing.
    if (span.type != type) {
        end();
        mBuildStack.push(StyledText::Span(mPosition, span.type));
    }
    else {
        mOpenedSpans[type] -= 1;
    }

    mAllowMerge = true;
}

void
StyledTextState::single(StyledText::SpanType type) {
    mSpans.emplace_back(StyledText::Span(mPosition, type));
}

void
StyledTextState::single() {
    auto typeIt = sTextSpanTypeMap.find(mCurrentTag);
    if (typeIt == sTextSpanTypeMap.end()) {
        return;
    }

    auto type = typeIt->second;
    if (StyledText::kSpanTypeLineBreak == type) {
        // Handle as single tag.
        single(type);
    }
}

std::vector<StyledText::Span>
StyledTextState::finalize() {
    std::vector<StyledText::Span> output;
    while (!mBuildStack.empty()) {
        auto span = mBuildStack.top();
        mBuildStack.pop();
        span.end = mPosition;
        mSpans.emplace_back(span);
    }

    std::sort(mSpans.begin(), mSpans.end(), spanComparator);
    for (auto s : mSpans) {
        output.emplace_back(s);
    }

    return output;
}

void
StyledTextState::emplaceAttribute(const std::string& value) {
    // Skip if this attribute name is not supported in APL
    auto nameIt = sSpanAttributeMap.find(mCurrentAttributeName);
    if (nameIt == sSpanAttributeMap.end()) {
        mCurrentAttributeName = "";
        return;
    }

    auto valueObject = evaluate(mContext, value);
    if (valueObject == INHERIT_ATTRIBUTE_VALUE) {
        mCurrentAttributeName  = "";
        return;
    }

    auto name = nameIt->second;
    switch (name) {
        case StyledText::kSpanAttributeNameColor:
            mCurrentAttributeMap.emplace(name, valueObject.asColor(mContext));
            break;
        case StyledText::kSpanAttributeNameFontSize:
            mCurrentAttributeMap.emplace(name, valueObject.asAbsoluteDimension(mContext));
            break;
    }

    mCurrentAttributeName  = "";
}

} // namespace apl
