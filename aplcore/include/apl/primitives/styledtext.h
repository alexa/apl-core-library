#include <utility>

/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef _APL_STYLED_TEXT_H
#define _APL_STYLED_TEXT_H

#include <vector>
#include <stack>

namespace apl {

class Object;
class Context;

/**
 * Represents styled text.
 * Object contains original string, string with styles and not-allowed characters stripped and list of style
 * Spans with type and character index ranges. Whole idea behind this representation is to have intermediate text that
 * will have styles processed and sanitized same way for all view hosts while being easily applicable regardless target
 * styled text form.
 */
class StyledText {
public:
    /**
     * Limited set of APL supported styles.
     */
    enum SpanType {
        kSpanTypeLineBreak = 0,
        kSpanTypeStrong = 1,
        kSpanTypeItalic = 2,
        kSpanTypeStrike = 3,
        kSpanTypeUnderline = 4,
        kSpanTypeMonospace = 5,
        kSpanTypeSuperscript = 6,
        kSpanTypeSubscript = 7
    };

    /**
     * Representation of text style span.
     */
    struct Span {
        /**
         * @param start style span starting index.
         * @param type style type.
         */
        Span(size_t start, SpanType type) : type(type), start(start), end(start) {}

        /**
         * Copy constructor for emplace to vector<const Span> to work with clang.
         * @param obj object to copy.
         */
        Span(const Span& obj) : type(obj.type), start(obj.start), end(obj.end) {};

        /**
         * Type of span.
         */
        SpanType type;

        /**
         * Span start index.
         */
        size_t start;

        /**
         * Span end index.
         */
        size_t end;
    };

    /**
     * Build StyledText from object.
     * @param object The source of the text.
     * @return An object containing a StyledText or null.
     */
    static Object create(const Object& object);

    /**
     * Empty styled text object. Useful as default value.
     * @return empty StyledText.
     */
    static Object EMPTY();

    /**
     * @return Raw text filtered of not-allowed characters and styles.
     */
    const std::string getText() const { return mText; }

    /**
     * @return Raw original text.
     */
    const std::string getRawText() const { return mRawText; }

    /**
     * @return Vector of style spans.
     */
    const std::vector<Span>& getSpans() const { return mSpans; }

    const std::string asString() const {
        return getRawText();
    }

    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

    bool operator==(const StyledText& rhs) const { return mRawText == rhs.mRawText; }

private:
    StyledText(std::string&& rawText, std::string text, std::vector<Span>&& spans) :
            mRawText(std::move(rawText)),
            mText(std::move(text)),
            mSpans(std::move(spans))
    {}

    StyledText() {}

    std::string mRawText;
    std::string mText;
    std::vector<Span> mSpans;
};

} // namespace apl

#endif // _APL_STYLED_TEXT_H
