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

#ifndef _APL_STYLED_TEXT_STATE_H
#define _APL_STYLED_TEXT_STATE_H

#include "apl/engine/context.h"
#include "apl/primitives/styledtext.h"

#include <map>
#include <stack>
#include <string>

namespace apl {

/**
 * Builder to construct StyledText during PEGTL parsing.
 */
class StyledTextState {
public:
    /**
      * @param context The data-binding context.
      */
    StyledTextState(const Context& context) : mContext(context) {};

    /**
     * Append text to raw text "container".
     * @param val text to append.
     */
    void append(const std::string& val);

    /**
     * Add space. Avoids wchar processing for spaces because " " is already utf-8 encoded.
     */
    void space();

    /**
     * @param attributeName style attributeName.
     */
    void attributeName(const std::string& attributeName);

    /**
     * @param attributeValue style attributeName.
     */
    void attributeValue(const std::string& attributeValue);

    /**
     * @param tag style tag.
     */
    void tag(const std::string& tag);

    /**
     * Start style span on current text position.
     */
    void start();

    /**
     * Determines if a current span can be merged with the previous one.
     *
     * The implementation checks for a whitespace between spans to avoid colluding undesired situations.
     * @param previousSpan previous read span
     * @param currentSpanType current span type
     * @param currentPosition current position in the mText attribute
     * @return true if the current span the previous span can be merged
     */
    bool canMergeSpans(const StyledText::Span& previousSpan,
                       const StyledText::SpanType& currentSpanType,
                       size_t currentPosition) const;

    /**
     * End style span on current text position. In case if tag was not opened it will close current one and move up to "parent". This implementation effectively replicates html behavior.
     * @param tag style tag.
     */
    void end();

    /**
     * Record non-parameterized tag, for example line break.
     * @param type style type.
     */
    void single(StyledText::SpanType type);

    /**
     * Record non-parameterized tag, for example line break.
     * @param type style type.
     */
    void single();

    /**
     * Checks if any unhandled opened tags left and closes them.
     * @return Vector of style spans.
     */
    std::vector<StyledText::Span> finalize();

    /**
     * @return text.
     */
    std::string& getText() { return mText; }

private:
    const Context& mContext;
    std::stack<StyledText::Span> mBuildStack;
    std::map<StyledText::SpanType, size_t> mOpenedSpans;
    std::vector<StyledText::Span> mSpans;
    std::string mText;
    std::map<StyledText::SpanAttributeName, Object> mCurrentAttributeMap;
    std::string mCurrentAttributeName;
    /**
     *  All span offsets are codepoint offsets. Note that the number of bytes per codepoint depends on the string encoding used.
     */
    size_t mPosition = 0;
    std::string mCurrentTag;
    /**
     * Keeps an internal state if the last element inserted was a whitespace and propagates it ignoring tags, this is done to avoid the following scenario, {word}{ws}{stag}{ws}{word}{etag}
     */
    bool mProcessingWhiteSpace = false;
    /**
     * Internal state of the span collapse, this is not on position or span type class but
     * on the syntactical element. Without tracking the state is impossible to
     * handle whitespaces around collapsing spans.
     */
    bool mAllowMerge = false;

    struct {
        bool operator()(const StyledText::Span& a, const StyledText::Span& b) const {
            if (a.start < b.start) {
                return true;
            }
            else if (a.start > b.start) {
                return false;
            }

            if (a.end > b.end) {
                return true;
            }
            else if (a.end < b.end) {
                return false;
            }

            return (a.type < b.type);
        }
    } spanComparator;

    void emplaceAttribute(const std::string& value);
};

} // namespace apl

#endif // APL_STYLEDTEXTSTATE_H
