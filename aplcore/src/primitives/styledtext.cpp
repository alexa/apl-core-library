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

#include <regex>
#include <vector>
#include <stack>
#include <set>
#include <locale>
#include <codecvt>

#include <pegtl.hh>

#include "apl/primitives/object.h"
#include "apl/primitives/styledtext.h"


namespace apl {

using namespace pegtl;

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
};

const std::map<std::string, std::string> sTextSpecialEntity = {
        {"&amp;", "&"},
        {"&lt;", "<"},
        {"&gt;", ">"},
};

/**
 * Character converter to be used for multi-byte sized characters.
 */
static std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> wchar_converter;

/**
 * Internal utility to identify control characters.
 * @param c input character.
 * @return true if control, false otherwise.
 */
static inline bool controlChar(char c)
{
    return static_cast<int>(c) > 0 && static_cast<int>(c) < 32;
}

/**
 * Internal utility to strip control characters from string.
 * @param str string to process.
 * @return string without control characters.
 */
static inline std::string stripControl(const std::string& str)
{
    std::string output;

    for(char c : str) {
        if(!controlChar(c)) output += c;
        else output += ' ';
    }

    return output;
}

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

static inline std::string rtrim(const std::string &str) {
    std::string output(str);
    output.erase(std::find_if(output.rbegin(), output.rend(), [](unsigned char ch) {
                     return !std::isspace(ch);
                 }).base(),
                 output.end());
    return output;
}

/**
 * Internal builder to construct StyledText during PEGTL parsing.
 */
class StyledTextState {
public:
    StyledTextState() = default;

    /**
     * Append text to raw text "container".
     * @param val text to append.
     */
    void append( const std::string& val ) {
        wchar_converter.to_bytes(wchar_converter.from_bytes(val.c_str()));

        // Get real number of characters.
        mPosition += wchar_converter.converted();
        mText = mText.append(val);
    }

    /**
     * Add space. Avoids wchar processing for spaces.
     */
    void space() {
        mPosition+=1;
        mText = mText.append(" ");
    }

    /**
     * Start style span on current text position.
     * @param tag style tag.
     */
    void start(const std::string& tag) {
        auto typeIt = sTextSpanTypeMap.find(tolower(tag));
        if(typeIt == sTextSpanTypeMap.end()) {
            return;
        }

        auto type = typeIt->second;
        if(StyledText::kSpanTypeLineBreak == type) {
            // Handle as single tag.
            single(type);
            return;
        }

        size_t start = mPosition;
        // If type same as previously closed tag - merge it instead of creating new one.
        // TODO: Not ideal but simple enough. Nested merging to be added if we see problems with viewhost performance.
        if(!mSpans.empty()) {
            auto previous = mSpans.back();
            if (previous.type == type && previous.end == mPosition) {
                start = previous.start;
                mSpans.pop_back();
            }
        }

        mOpenedSpans.insert(type);
        mBuildStack.push(StyledText::Span(start, type));
    }

    /**
     * End style span on current text position. In case if tag was not opened it will close current one and move up to
     * "parent". This implementation effectively replicates html behavior.
     * @param tag style tag.
     */
    void end(const std::string& tag) {
        auto typeIt = sTextSpanTypeMap.find(tolower(tag));
        if(typeIt == sTextSpanTypeMap.end()) {
            return;
        }

        auto type = typeIt->second;

        if(mOpenedSpans.find(type) == mOpenedSpans.end()) {
            // No opened tag available
            return;
        }

        if(StyledText::kSpanTypeLineBreak == type) {
            // Handle as single tag.
            single(type);
            return;
        }

        auto& span = mBuildStack.top();
        mBuildStack.pop();

        // If start == end then span is unnecessary so don't record it
        if(span.start != mPosition) {
            span.end = mPosition;
            mSpans.emplace_back(span);
        }

        // If not equal then likely tag is unclosed so close it (above) and try again. This will split
        // intersecting/wrongly closed tags to separate spans in order to simplify view-host processing.
        if(span.type != type) {
            end(tag);
            mBuildStack.push(StyledText::Span(mPosition, span.type));
        } else {
            mOpenedSpans.erase(type);
        }
    }

    /**
     * Record non-parameterized tag, for example line break.
     * @param type style type.
     */
    void single(StyledText::SpanType type) {
        mSpans.emplace_back(StyledText::Span(mPosition, type));
    }

    /**
     * Checks if any unhandled opened tags left and closes them.
     * @return Vector of style spans.
     */
    std::vector<StyledText::Span> finalize() {
        std::vector<StyledText::Span> output;
        while(!mBuildStack.empty()) {
            auto span = mBuildStack.top();
            mBuildStack.pop();
            span.end = mPosition;
            mSpans.emplace_back(span);
        }

        std::sort(mSpans.begin(), mSpans.end(), spanComparator);
        for(auto s : mSpans) {
            output.emplace_back(s);
        }

        return output;
    }

    /**
     * @return text.
     */
    std::string& getText() {return mText;}

private:
    std::stack<StyledText::Span> mBuildStack;
    std::set<StyledText::SpanType> mOpenedSpans;
    std::vector<StyledText::Span> mSpans;
    std::string mText;
    size_t mPosition = 0;

    struct {
        bool operator()(const StyledText::Span& a, const StyledText::Span& b) const {
            if(a.start < b.start) {
                return true;
            } else if(a.start > b.start) {
                return false;
            }

            if(a.end > b.end) {
                return true;
            } else if(a.end < b.end) {
                return false;
            }

            return (a.type < b.type);
        }
    } spanComparator;
};

struct quote : sor<one<'"'>, one<'\''>> {};
struct attributes :
        seq<plus<space>, until<
            seq<
                quote, star<space>,
                at<sor<one<'>'>, string<'/','>'>>
                >>>> {};

struct markdownchar : one<'<','>','&'>{};
struct stringentity : seq<one<'&'>, plus<alpha>, one<';'>>{};
struct decnum : plus<digit>{};
struct decentity : seq<one<'&'>, one<'#'>, decnum, one<';'>>{};
struct hexnum : plus<xdigit>{};
struct hexentity : seq<one<'&'>, one<'#'>, one<'x'>, hexnum, one<';'>>{};
struct specialentity : sor<hexentity,decentity,stringentity>{};
struct symbol : not_one<'<','>','&',' '>{}; // Exclude markdown and whitespace collapsing.
struct word : plus<symbol>{};
struct ws : plus<space>{};
struct tagname : plus<alnum>{};
struct stagname : tagname{};
struct stag : seq<one<'<'>,opt<star<space>>,stagname,opt<attributes>,opt<star<space>>, one<'>'>>{};
struct etagname : tagname{};
struct etag : seq<one<'<'>,one<'/'>,opt<star<space>>,etagname,opt<star<space>>,one<'>'>>{};
// Start tag and single tag is too similar to do exact match so define line break itself.
struct br : seq<star<space>,one<'<'>,opt<star<space>>,seq<one<'b','B'>,one<'r','R'>>,opt<attributes>,opt<star<space>>,opt<one<'/'>>,one<'>'>,star<space>>{};
struct utag : seq<one<'<'>,opt<star<space>>,tagname,opt<attributes>,opt<star<space>>,one<'/'>,one<'>'>>{};
struct element : sor<br, utag, stag, etag, specialentity, markdownchar, word, ws>{};
// Trim starting spaces here as it's easy.
struct styledtext : seq<star<space>,star<element>,eof>{};

template<typename Rule>
struct action : nothing< Rule >
{
};

template<> struct action< stagname >
{
    static void apply(const input& in, StyledTextState& state) {
        state.start(in.string());
    }
};

template<> struct action< etagname >
{
    static void apply(const input& in, StyledTextState& state) {
        state.end(in.string());
    }
};

template<> struct action< br >
{
    static void apply(const input& in, StyledTextState& state) {
        state.single(StyledText::kSpanTypeLineBreak);
    }
};

template<> struct action< word >
{
    static void apply(const input& in, StyledTextState& state) {
        state.append(in.string());
    }
};

template<> struct action< ws >
{
    static void apply(const input& in, StyledTextState& state) {
        state.space();
    }
};

template<> struct action< markdownchar >
{
    static void apply(const input& in, StyledTextState& state) {
        // TODO: "Best effort" suggests that we allow that, though spec says: Markup characters in text must be replaced
        // with character entity references. Do we want to log it or restrict it in any way?
        state.append(in.string());
    }
};

template<> struct action< stringentity >
{
    static void apply(const input& in, StyledTextState& state) {
        auto str =  in.string();
        auto entity = sTextSpecialEntity.find(str);
        if(entity != sTextSpecialEntity.end()) {
            state.append(entity->second);
        } else {
            // Leave unparsed - aligned with HTML.
            state.append(str);
        }
    }
};

template<> struct action< decnum >
{
    static void apply(const input& in, StyledTextState& state) {
        unsigned long wc = std::stoul(in.string());
        state.append(wchar_converter.to_bytes(wc));
    }
};

template<> struct action< hexnum >
{
    static void apply(const input& in, StyledTextState& state) {
        unsigned long wc = std::stoul(in.string(), 0, 16);
        state.append(wchar_converter.to_bytes(wc));
    }
};

Object
StyledText::create(const Object& object) {
    if (object.isStyledText())
        return object;

    auto str = object.asString();
    auto filtered = rtrim(stripControl(str));

    data_parser parser(filtered, "TextMarkdown");
    StyledTextState state;
    parser.parse<styledtext, action>(state);

    return Object(StyledText(std::move(str), state.getText(), std::move(state.finalize())));
}

Object StyledText::EMPTY() {
    static Object* obj = new Object(StyledText());
    return *obj;
}

rapidjson::Value
StyledText::serialize(rapidjson::Document::AllocatorType& allocator) const {
    using rapidjson::Value;
    Value v(rapidjson::kObjectType);
    v.AddMember("text", Value(mText.c_str(), allocator).Move(), allocator);

    Value spans(rapidjson::kArrayType);
    for (const auto& s : mSpans) {
        Value span(rapidjson::kArrayType);
        span.PushBack(s.type, allocator);
        span.PushBack(static_cast<unsigned>(s.start), allocator);
        span.PushBack(static_cast<unsigned>(s.end), allocator);
        spans.PushBack(span, allocator);
    }
    v.AddMember("spans", spans, allocator);
    return v;
}

} // namespace apl