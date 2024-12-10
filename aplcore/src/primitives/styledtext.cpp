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

#include "apl/primitives/styledtext.h"

#include <locale>
#include <codecvt>

#include <tao/pegtl.hpp>

#include "apl/content/content.h"
#include "apl/document/coredocumentcontext.h"
#include "apl/primitives/color.h"
#include "apl/primitives/dimension.h"
#include "apl/primitives/pseudolocalizer.h"
#include "apl/primitives/styledtextstate.h"
#include "apl/primitives/unicode.h"
#include "apl/utils/stringfunctions.h"

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

/**
 * \cond ShowStyledTextGrammar
 */

namespace styledtextgrammar {

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

// Trim spaces around list item
struct li : seq<one<'l','L'>,one<'i','I'>>{};
struct slist : seq<star<space>,one<'<'>,star<space>,li,opt<attributes>,star<space>,one<'>'>,star<space>>{};
struct elist : seq<star<space>,one<'<'>,one<'/'>,star<space>,li,opt<attributes>,star<space>,one<'>'>,star<space>>{};

struct stag : sor<slist,seq<one<'<'>,star<space>,tagname,opt<attributes>,star<space>,one<'>'>>>{};
struct etag : sor<elist,seq<one<'<'>,one<'/'>,star<space>,tagname,star<space>,one<'>'>>>{};
// Start tag and single tag are too similar to do exact match so define line break itself.
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

template<> struct action< li >
{
    template< typename Input >
    static void apply(const Input& in, StyledTextState& state) {
        state.tag(in.string());
    }
};

template<> struct action< slist >
{
    template< typename Input >
    static void apply(const Input& in, StyledTextState& state) {
        state.closeAndCacheAllTags();
    }
};

template<> struct action< stag >
{
    template< typename Input >
    static void apply(const Input& in, StyledTextState& state) {
        state.start();
        // To prevent styles from crossing list items,
        // tags are closed before the list item and re-opened here.
        // Tag ordering is determined by the StyledTextState comparator.
        state.openAllTagsFromCache();
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

} // namespace styledtextgrammar

/**
 * \endcond
 */

StyledText
StyledText::create(const Context& context, const Object& object)
{
    if (object.is<StyledText>())
        return object.get<StyledText>();

    // Some primitive tests don't have document context added
    if (context.documentContext() == nullptr) {
        return {context, object.asString()};
    }

    // Get PseudoLocalization settings
    auto pseudoLocalizationSettings = CoreDocumentContext::cast(context.documentContext())
                                          ->content()
                                          ->getDocumentSettings()
                                          ->getValue("pseudoLocalization");
    // Transform text into its Pseudo-localized version. The method returns the same string if the
    // PseudoLocalization feature is disabled.
    if (pseudoLocalizationSettings.isMap() &&
        pseudoLocalizationSettings.get("enabled").asBoolean()) {
        PseudoLocalizationTextTransformer transformer;
        auto transformedText = transformer.transform(object.asString(), pseudoLocalizationSettings);
        return {context, transformedText};
    }

    return {context, object.asString()};
}

StyledText
StyledText::createRaw(const std::string& raw)
{
    return StyledText(raw);
}

StyledText&
StyledText::operator=(const StyledText& other) {
    if (this == &other)
        return *this;

    mRawText = other.mRawText;
    mText = other.mText;
    mSpans = other.mSpans;
    return *this;
}

StyledText::StyledText(const std::string& raw)
    : mRawText(raw), mText(raw)
{
}

StyledText::StyledText(const Context& context, const std::string& raw) {
    mRawText = raw;
    auto filtered = apl::rtrim(stripControl(raw));

    auto state = StyledTextState(context);
    pegtl::string_input<> in(filtered, "");
    pegtl::parse<styledtextgrammar::styledtext, styledtextgrammar::action>(in, state);

    mText = state.getText();
    mSpans = state.finalize();
}

std::string
StyledText::toDebugString() const
{
    auto result = std::string("StyledText<'" + mRawText);
    result += "' outputText='" + mText;
    result += "' spans=[";
    for (const auto& span : mSpans) {
        result += std::to_string(span.type) + ":" + std::to_string(span.start) + ":" + std::to_string(span.end) + ";";
    }
    result += "]>";
    return result;
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
    const auto& spans = mStyledText.mSpans;
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

size_t
StyledText::Iterator::spanCount() {
    return mStyledText.mSpans.size();
}

std::string
StyledText::ObjectType::asString(const Object::DataHolder& dataHolder) const
{
    return getReferenced<StyledText>(dataHolder).asString();
}

double
StyledText::ObjectType::asNumber(const Object::DataHolder& dataHolder) const
{
    return aplFormattedStringToDouble(getReferenced<StyledText>(dataHolder).asString());
}

int
StyledText::ObjectType::asInt(const Object::DataHolder& dataHolder, int base) const
{
    return sutil::stoi(getReferenced<StyledText>(dataHolder).asString(), nullptr, base);
}

int64_t
StyledText::ObjectType::asInt64(const Object::DataHolder& dataHolder, int base) const
{
    return sutil::stoll(getReferenced<StyledText>(dataHolder).asString(), nullptr, base);
}

Color
StyledText::ObjectType::asColor(const Object::DataHolder& dataHolder, const apl::SessionPtr& session) const
{
    return {session, getReferenced<StyledText>(dataHolder).asString()};
}

Dimension
StyledText::ObjectType::asDimension(const Object::DataHolder& dataHolder, const Context& context) const
{
    return {context, getReferenced<StyledText>(dataHolder).asString()};
}

Dimension
StyledText::ObjectType::asAbsoluteDimension(const Object::DataHolder& dataHolder, const Context& context) const
{
    auto d = Dimension(context, getReferenced<StyledText>(dataHolder).asString());
    return (d.getType() == DimensionType::Absolute ? d : Dimension(DimensionType::Absolute, 0));
}

Dimension
StyledText::ObjectType::asNonAutoDimension(const Object::DataHolder& dataHolder, const Context& context) const
{
    auto d = Dimension(context, getReferenced<StyledText>(dataHolder).asString());
    return (d.getType() == DimensionType::Auto ? Dimension(DimensionType::Absolute, 0) : d);
}
Dimension
StyledText::ObjectType::asNonAutoRelativeDimension(const Object::DataHolder& dataHolder, const Context& context) const
{
    auto d = Dimension(context, getReferenced<StyledText>(dataHolder).asString(), true);
    return (d.getType() == DimensionType::Auto ? Dimension(DimensionType::Relative, 0) : d);
}

std::uint64_t
StyledText::ObjectType::size(const Object::DataHolder& dataHolder) const
{
    return getReferenced<StyledText>(dataHolder).asString().size();
}

size_t
StyledText::ObjectType::hash(const Object::DataHolder& dataHolder) const
{
    return std::hash<std::string>{}(getReferenced<StyledText>(dataHolder).getRawText());
}

} // namespace apl
