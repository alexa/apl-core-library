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

#include <set>
#include <locale>
#include <codecvt>

#include <tao/pegtl.hpp>

#include "apl/primitives/object.h"
#include "apl/primitives/unicode.h"
#include "apl/primitives/styledtext.h"
#include "apl/primitives/styledtextstate.h"


namespace apl {

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;
using namespace pegtl;

const std::map<std::string, std::string> sTextSpecialEntity = {
        {"&amp;", "&"},
        {"&lt;", "<"},
        {"&gt;", ">"},
};

/**
 * Character converter to be used for multi-byte sized characters.
 */
#ifdef APL_CORE_UWP
// avoids a runtime crash on UWP
// see:  https://tinyurl.com/ra2coob
static std::wstring_convert<std::codecvt_utf8<wchar_t>> wchar_converter;
#else
static std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> wchar_converter;
#endif

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

static inline std::string rtrim(const std::string &str) {
    std::string output(str);
    output.erase(std::find_if(output.rbegin(), output.rend(), [](unsigned char ch) {
                     return !std::isspace(ch);
                 }).base(),
                 output.end());
    return output;
}

struct quote : sor<one<'"'>, one<'\''>> {};
struct attributename : plus<alnum>{};
struct attributevalue : plus<not_one<'"', '\''>>{};
struct attribute : seq<attributename,star<space>,one<'='>,star<space>,quote,attributevalue,quote,star<space>>{};
struct attributes : seq<plus<space>,star<attribute>,until<at<sor<one<'>'>,string<'/','>'>>>>>{};
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
struct stag : seq<one<'<'>,star<space>,tagname,opt<attributes>,star<space>,one<'>'>>{};
struct etag : seq<one<'<'>,one<'/'>,star<space>,tagname,star<space>,one<'>'>>{};
// Start tag and single tag is too similar to do exact match so define line break itself.
struct br : seq<star<space>,one<'<'>,star<space>,seq<one<'b','B'>,one<'r','R'>>,opt<attributes>,star<space>,opt<one<'/'>>,one<'>'>,star<space>>{};
struct utag : seq<one<'<'>,star<space>,tagname,opt<attributes>,star<space>,one<'/'>,one<'>'>>{};
struct element : sor<br, utag, stag, etag, specialentity, markdownchar, word, ws>{};
// Trim starting spaces here as it's easy.
struct styledtext : seq<star<space>,star<element>,eof>{};

template<typename Rule>
struct action : nothing< Rule >
{
};

template<> struct action< attributename >
{
    template< typename Input >
    static void apply(const Input& in, StyledTextState& state) {
        state.attributeName(in.string());
    }
};

template<> struct action< attributevalue >
{
    template< typename Input >
    static void apply(const Input& in, StyledTextState& state) {
        state.attributeValue(in.string());
    }
};

template<> struct action< tagname >
{
    template< typename Input >
    static void apply(const Input& in, StyledTextState& state) {
        state.tag(in.string());
    }
};

template<> struct action< stag >
{
    template< typename Input >
    static void apply(const Input& in, StyledTextState& state) {
        state.start();
    }
};

template<> struct action< etag >
{
    template< typename Input >
    static void apply(const Input& in, StyledTextState& state) {
        state.end();
    }
};

template<> struct action< utag >
{
    template< typename Input >
    static void apply(const Input& in, StyledTextState& state) {
        state.single();
    }
};

template<> struct action< br >
{
    template< typename Input >
    static void apply(const Input& in, StyledTextState& state) {
        state.single(StyledText::kSpanTypeLineBreak);
    }
};

template<> struct action< word >
{
    template< typename Input >
    static void apply(const Input& in, StyledTextState& state) {
        state.append(in.string());
    }
};

template<> struct action< ws >
{
    template< typename Input >
    static void apply(const Input& in, StyledTextState& state) {
        state.space();
    }
};

template<> struct action< markdownchar >
{
    template< typename Input >
    static void apply(const Input& in, StyledTextState& state) {
        // TODO: "Best effort" suggests that we allow that, though spec says: Markup characters in text must be replaced
        //  with character entity references. Do we want to log it or restrict it in any way?
        state.append(in.string());
    }
};

template<> struct action< stringentity >
{
    template< typename Input >
    static void apply(const Input& in, StyledTextState& state) {
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

template<> struct action< decentity >
{
    template< typename Input >
    static void apply(const Input& in, StyledTextState& state) {
        // strip leading &# and trailing ;
        std::string numstr = in.string().substr(2, in.string().length() - 3);
        unsigned long wc = std::stoul(numstr);
        state.append(wchar_converter.to_bytes(wc));
    }
};

template<> struct action< hexentity >
{
    template< typename Input >
    static void apply(const Input& in, StyledTextState& state) {
        // strip leading &#x and trailing ;
        std::string numstr = in.string().substr(3, in.string().length() - 4);
        unsigned long wc = std::stoul(numstr, 0, 16);
        state.append(wchar_converter.to_bytes(wc));
    }
};

Object
StyledText::create(const Context& context, const Object& object) {
    if (object.isStyledText())
        return object;

    return Object(StyledText(context, object.asString()));
}

StyledText::StyledText(const Context& context, const std::string& raw) {
    mRawText = raw;
    auto filtered = rtrim(stripControl(raw));

    auto state = StyledTextState(context);
    pegtl::string_input<> in(filtered, "");
    pegtl::parse<styledtext, action>(in, state);

    mText = state.getText();
    mSpans = state.finalize();
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
        Value attributes(rapidjson::kArrayType);
        for (auto &a : s.attributes) {
            Value attribute(rapidjson::kArrayType);
            attribute.PushBack(a.name, allocator);
            attribute.PushBack(a.value.serialize(allocator), allocator);
            attributes.PushBack(attribute, allocator);
        }
        span.PushBack(attributes, allocator);
        spans.PushBack(span, allocator);
    }
    v.AddMember("spans", spans, allocator);
    return v;
}

StyledText::Iterator::Iterator(const StyledText& styledText)
    : mStyledText(styledText),
        codePointCount(utf8StringLength(styledText.getText()))
{}

StyledText::SpanType
StyledText::Iterator::getSpanType() const {
    if (mSpanType == START) {
        LOG(LogLevel::kError) << "StyledText::Iterator::getSpanType() called before mSpanType was set";
        assert(mSpanType != START);
    }
    return mSpanType;
};

StyledText::Iterator::TokenType
StyledText::Iterator::next() {
    const auto& spans = mStyledText.getSpans();
    size_t nextStartSpanPosition = mSpanIndex < spans.size() ? spans[mSpanIndex].start :  std::numeric_limits<int>::max();
    size_t nextEndSpanPosition = mStack.empty() ? std::numeric_limits<int>::max() : mStack.top()->end;
    size_t next = std::min({nextStartSpanPosition, nextEndSpanPosition, codePointCount});

    if (mCurrentStrPos < next) {
        mString = utf8StringSlice(mStyledText.getText(), mCurrentStrPos, next);
        mCurrentStrPos = next;
        return kString;
    }

    if (nextEndSpanPosition == mCurrentStrPos) {
        mSpanType = mStack.top()->type;
        mStack.pop();
        return kEndSpan;
    }

    if (nextStartSpanPosition == mCurrentStrPos) {
        mSpanType = spans[mSpanIndex].type;
        mSpanAttributes = spans[mSpanIndex].attributes;
        mStack.push(&spans[mSpanIndex++]);
        return kStartSpan;
    }

    return kEnd;
}

} // namespace apl
