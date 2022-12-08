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

#include <future>

#include "../testeventloop.h"

using namespace apl;

class StyledTextTest : public ::testing::Test {
protected:
    StyledTextTest() {
        context = Context::createTestContext(Metrics(), makeDefaultSession());
        context->putConstant("@testFontSize", Dimension(DimensionType::Absolute, 10));
    }

    void createAndVerifyStyledText(const std::string& rawText, const std::string& expectedText, size_t spansCount) {
        styledText = StyledText::create(*context, rawText);
        ASSERT_TRUE(styledText.is<StyledText>());
        auto iterator = StyledText::Iterator(styledText.get<StyledText>());
        ASSERT_EQ(expectedText, styledText.get<StyledText>().getText());
        ASSERT_EQ(spansCount, iterator.spanCount());
    }

    ::testing::AssertionResult
    verifyText(StyledText::Iterator& it, const std::string& text) {
        auto token = it.next();
        if (token != StyledText::Iterator::kString) {
            return ::testing::AssertionFailure() << "Mismatching token=" << token << ", expected=" << StyledText::Iterator::kString;
        }

        if (text != it.getString()) {
            return ::testing::AssertionFailure() << "Mismatching text=" << it.getString() << ", expected=" << text;
        }

        return ::testing::AssertionSuccess();
    }

    ::testing::AssertionResult
    verifySpanStart(StyledText::Iterator& it, StyledText::SpanType type) {
        auto token = it.next();
        if (token != StyledText::Iterator::kStartSpan) {
            return ::testing::AssertionFailure() << "Mismatching token=" << token << ", expected=" << StyledText::Iterator::kStartSpan;
        }

        if (type != it.getSpanType()) {
            return ::testing::AssertionFailure() << "Mismatching type=" << it.getSpanType() << ", expected=" << type;
        }

        return ::testing::AssertionSuccess();
    }

    ::testing::AssertionResult
    verifySpanEnd(StyledText::Iterator& it, StyledText::SpanType type) {
        auto token = it.next();
        if (token != StyledText::Iterator::kEndSpan) {
            return ::testing::AssertionFailure() << "Mismatching token=" << token << ", expected=" << StyledText::Iterator::kEndSpan;
        }

        if (type != it.getSpanType()) {
            return ::testing::AssertionFailure() << "Mismatching type=" << it.getSpanType() << ", expected=" << type;
        }

        return ::testing::AssertionSuccess();
    }

    ::testing::AssertionResult
    verifyColorAttribute(StyledText::Iterator& it, size_t attributeIndex, const std::string& attributeValue) {
        auto attribute = it.getSpanAttributes().at(attributeIndex);
        if (attribute.name != 0) return ::testing::AssertionFailure() << "Wrong attribute name.";
        if (!attribute.value.is<Color>()) return ::testing::AssertionFailure() << "Not a color.";
        if (attribute.value.asString() != attributeValue) return ::testing::AssertionFailure() << "Wron value.";

        return ::testing::AssertionSuccess();
    }

    ::testing::AssertionResult
    verifyFontSizeAttribute(StyledText::Iterator& it, size_t attributeIndex, const std::string& attributeValue) {
        auto attribute = it.getSpanAttributes().at(attributeIndex);
        if (attribute.name != 1) return ::testing::AssertionFailure() << "Wrong attribute name.";
        if (!attribute.value.isAbsoluteDimension()) return ::testing::AssertionFailure() << "Not a dimension.";
        if (attribute.value.asString() != attributeValue) return ::testing::AssertionFailure() << "Wron value.";

        return ::testing::AssertionSuccess();
    }

    ContextPtr context;
    Object styledText;
};

TEST_F(StyledTextTest, Casting)
{
    ASSERT_TRUE(IsEqual("<i>FOO</i>", StyledText::create(*context, "<i>FOO</i>").asString()));

    ASSERT_TRUE(IsEqual(4.5, Object(StyledText::create(*context, "4.5")).asNumber()));
    ASSERT_TRUE(IsEqual(4, Object(StyledText::create(*context, "4.3")).asInt()));
    ASSERT_TRUE(IsEqual(Color(Color::RED), Object(StyledText::create(*context, "#ff0000")).asColor(*context)));

    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Absolute, 10),
                        Object(StyledText::create(*context, "10dp")).asDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(),
                        Object(StyledText::create(*context, "auto")).asDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Relative, 10),
                        Object(StyledText::create(*context, "10%")).asDimension(*context)));

    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Absolute, 5),
                        Object(StyledText::create(*context, "5dp")).asAbsoluteDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Absolute, 0),
                        Object(StyledText::create(*context, "auto")).asAbsoluteDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Absolute, 0),
                        Object(StyledText::create(*context, "10%")).asAbsoluteDimension(*context)));

    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Absolute, 5),
                        Object(StyledText::create(*context, "5dp")).asNonAutoDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Absolute, 0),
                        Object(StyledText::create(*context, "auto")).asNonAutoDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Relative, 10),
                        Object(StyledText::create(*context, "10%")).asNonAutoDimension(*context)));

    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Absolute, 5),
                        Object(StyledText::create(*context, "5dp")).asNonAutoRelativeDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Relative, 0),
                        Object(StyledText::create(*context, "auto")).asNonAutoRelativeDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Relative, 10),
                        Object(StyledText::create(*context, "10%")).asNonAutoRelativeDimension(*context)));

    ASSERT_TRUE(StyledText::create(*context, "").empty());
    ASSERT_FALSE(StyledText::create(*context, "<h2></h2>").empty());

    ASSERT_EQ(0, Object(StyledText::create(*context, "")).size());
    ASSERT_EQ(9, Object(StyledText::create(*context, "<h2></h2>")).size());
}

TEST_F(StyledTextTest, NotStyled)
{
    createAndVerifyStyledText(u8"Simple text.", u8"Simple text.", 0);
}

TEST_F(StyledTextTest, Simple)
{
    createAndVerifyStyledText(u8"Simple <i>styled</i> text.", u8"Simple styled text.", 1);
    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator, "Simple "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, "styled"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, " text."));
}

TEST_F(StyledTextTest, Multiple)
{
    createAndVerifyStyledText(u8"Simple <i>somewhat</i> <u>styled</u> text.", u8"Simple somewhat styled text.", 2);
    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator, "Simple "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, "somewhat"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, " "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeUnderline));
    ASSERT_TRUE(verifyText(iterator, "styled"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeUnderline));
    ASSERT_TRUE(verifyText(iterator, " text."));
}

TEST_F(StyledTextTest, LineBreak)
{
    createAndVerifyStyledText(u8"Line <br/>break<br> text.", u8"Linebreaktext.", 2);
    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator, "Line"));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeLineBreak));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeLineBreak));
    ASSERT_TRUE(verifyText(iterator, "break"));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeLineBreak));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeLineBreak));
    ASSERT_TRUE(verifyText(iterator, "text."));
}

TEST_F(StyledTextTest, EscapeCharacters)
{
    createAndVerifyStyledText(u8"Simple\f text\t should\r not\n break\treally.", u8"Simple text should not break really.", 0);
}

TEST_F(StyledTextTest, Wchar)
{
    createAndVerifyStyledText(u8"\u524D\u9031\n\u672B<i>\u6BD434\u5186</i>80\u92AD",
		u8"\u524D\u9031 \u672B\u6BD434\u518680\u92AD", 1);
    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator, u8"\u524D\u9031 \u672B"));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, u8"\u6BD434\u5186"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, u8"80\u92AD"));
}

TEST_F(StyledTextTest, Cyrillics) {
    // String just means "Russian language"
    createAndVerifyStyledText(u8"\u0440\0443\u0441\u043a\u0438\u0439 <b>\u044F\u0437\u044B\u043a</b>",
		u8"\u0440\0443\u0441\u043a\u0438\u0439 \u044F\u0437\u044B\u043a", 1);

    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator, u8"\u0440\0443\u0441\u043a\u0438\u0439 "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifyText(iterator, u8"\u044F\u0437\u044B\u043a"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeStrong));
}

TEST_F(StyledTextTest, UnclosedTag)
{
    createAndVerifyStyledText(u8"Unclosed<i> tag.", u8"Unclosed tag.", 1);
    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator, "Unclosed"));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, " tag."));
}

TEST_F(StyledTextTest, UnclosedTagIntersect)
{
    createAndVerifyStyledText(u8"This is<b> bold text,<i> this is bold-italic</b> and </i>plain.",
                              u8"This is bold text, this is bold-italic and plain.", 3);

    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator, "This is"));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifyText(iterator, " bold text,"));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, " this is bold-italic"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, " and "));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, "plain."));
}

TEST_F(StyledTextTest, UnopenedTag)
{
    createAndVerifyStyledText(u8"Unopened</i> tag.", u8"Unopened tag.", 0);
}

TEST_F(StyledTextTest, UnopenedTagComplex)
{
    createAndVerifyStyledText(u8"<b>Hello, <i>I'm a turtle</sub> who likes lettuce.</i></b>",
                              u8"Hello, I'm a turtle who likes lettuce.", 2);

    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifyText(iterator, "Hello, "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, "I'm a turtle who likes lettuce."));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeStrong));
}

TEST_F(StyledTextTest, UnopenedTagNested)
{
    createAndVerifyStyledText(u8"<i>Unopened</i></i> tag.", u8"Unopened tag.", 1);

    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, "Unopened"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, " tag."));
}

TEST_F(StyledTextTest, UnopenedTagDeepNested)
{
    createAndVerifyStyledText(u8"<i><i>Unopened</i></i></i></i></i></i> tag.", u8"Unopened tag.", 2);

    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, "Unopened"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, " tag."));
}

TEST_F(StyledTextTest, UnclosedTagComplex)
{
    createAndVerifyStyledText(u8"Multiple <b>very <u>unclosed<i> tags</b>. And few <tt>unclosed <strike>at the end.",
                              u8"Multiple very unclosed tags. And few unclosed at the end.", 7);

    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator, "Multiple "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifyText(iterator, "very "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeUnderline));
    ASSERT_TRUE(verifyText(iterator, "unclosed"));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, " tags"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeUnderline));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeUnderline));
    ASSERT_TRUE(verifyText(iterator, ". And few "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeMonospace));
    ASSERT_TRUE(verifyText(iterator, "unclosed "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeStrike));
    ASSERT_TRUE(verifyText(iterator, "at the end."));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeStrike));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeMonospace));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeUnderline));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
}


TEST_F(StyledTextTest, UnclosedSameTypeTagNested)
{
    createAndVerifyStyledText(u8"Multiple nested <b><b><b><b>very</b></b> unclosed tags.",
    u8"Multiple nested very unclosed tags.", 4);

    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator, "Multiple nested "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifyText(iterator, "very"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifyText(iterator, " unclosed tags."));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeStrong));
}

TEST_F(StyledTextTest, UnclosedSameTypeTagNestedComplex)
{
    createAndVerifyStyledText(u8"Multiple <b><b>very <u>unclosed<i> tags</b>. And few <tt>unclosed <strike>at the end.",
    u8"Multiple very unclosed tags. And few unclosed at the end.", 8);

    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator, "Multiple "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifyText(iterator, "very "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeUnderline));
    ASSERT_TRUE(verifyText(iterator, "unclosed"));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, " tags"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeUnderline));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeUnderline));
    ASSERT_TRUE(verifyText(iterator, ". And few "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeMonospace));
    ASSERT_TRUE(verifyText(iterator, "unclosed "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeStrike));
    ASSERT_TRUE(verifyText(iterator, "at the end."));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeStrike));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeMonospace));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeUnderline));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeStrong));
}

TEST_F(StyledTextTest, UnsupportedTag)
{
    createAndVerifyStyledText(u8"Text with <ul><li>unsupported</li></ul> or wrong<b/> tag.",
                              u8"Text with unsupported or wrong tag.", 0);
}

TEST_F(StyledTextTest, SingleChildStyle)
{
    createAndVerifyStyledText(u8"Text <i>with <b>one</b> child</i>.", u8"Text with one child.", 2);

    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator, "Text "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, "with "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifyText(iterator, "one"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifyText(iterator, " child"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, "."));
}

TEST_F(StyledTextTest, SeveralChildStyles)
{
    createAndVerifyStyledText(u8"Text <i>with <b>child</b> and another <u>child</u></i>.",
                              u8"Text with child and another child.", 3);

    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator, "Text "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, "with "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifyText(iterator, "child"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifyText(iterator, " and another "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeUnderline));
    ASSERT_TRUE(verifyText(iterator, "child"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeUnderline));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, "."));
}

TEST_F(StyledTextTest, CollapseSpaces)
{
    createAndVerifyStyledText(u8"Text    value.", u8"Text value.", 0);
    createAndVerifyStyledText(u8"     foo     ", u8"foo", 0);
    createAndVerifyStyledText(u8" and<br>this ", u8"andthis", 1);
    createAndVerifyStyledText(u8" this is a <br> test of whitespace ", u8"this is atest of whitespace", 1);
}

TEST_F(StyledTextTest, Complex)
{
    createAndVerifyStyledText(u8" Since <i>you</i> are <magic>not</magic> going <u>on a? <b>holiday <em>this</em></b> year! "
                              "Boss,</u><br> <strong>I\f    thought</strong> I\t <strike><tt>should</tt> <sup>give</sup> "
                              "your</strike>\r <sUb>office</suB>\n a <code>holiday;</code> \u524D\u9031\u672B<i>\u6BD434\u518680\u92ad look. ",
                              u8"Since you are not going on a? holiday this year! Boss,I thought I should give your office "
                              "a holiday; \u524D\u9031\u672B\u6BD434\u518680\u92ad look.", 12);

    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator, "Since "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, "you"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, " are not going "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeUnderline));
    ASSERT_TRUE(verifyText(iterator, "on a? "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifyText(iterator, "holiday "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, "this"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifyText(iterator, " year! Boss,"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeUnderline));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeLineBreak));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeLineBreak));
    ASSERT_TRUE(verifyText(iterator, "I thought"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifyText(iterator, " I "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeStrike));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeMonospace));
    ASSERT_TRUE(verifyText(iterator, "should"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeMonospace));
    ASSERT_TRUE(verifyText(iterator, " "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeSuperscript));
    ASSERT_TRUE(verifyText(iterator, "give"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeSuperscript));
    ASSERT_TRUE(verifyText(iterator, " your"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeStrike));
    ASSERT_TRUE(verifyText(iterator, " "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeSubscript));
    ASSERT_TRUE(verifyText(iterator, "office"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeSubscript));
    ASSERT_TRUE(verifyText(iterator, " a "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeMonospace));
    ASSERT_TRUE(verifyText(iterator, "holiday;"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeMonospace));
    ASSERT_TRUE(verifyText(iterator, " 前週末"));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, "比34円80銭 look."));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
}

TEST_F(StyledTextTest, WithMarkdownCharacters)
{
    createAndVerifyStyledText(u8"1/2. This is true: -1 < 0.", u8"1/2. This is true: -1 < 0.", 0);
}

TEST_F(StyledTextTest, SpecialEntity)
{
    createAndVerifyStyledText(u8"1&lt;2, also 1&#60;2 and 1&#x3C;2", u8"1<2, also 1<2 and 1<2", 0);
}

TEST_F(StyledTextTest, IncompleteEntities) {
    createAndVerifyStyledText(u8"&#x1f607", u8"&#x1f607", 0);
    createAndVerifyStyledText(u8"&#128519", u8"&#128519", 0);
}

TEST_F(StyledTextTest, LongSpecialEntity)
{
    createAndVerifyStyledText(u8"go &#8594; <i>right</i>", u8"go \u2192 right", 1);

    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator, u8"go \u2192 "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, "right"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
}

TEST_F(StyledTextTest, UppercaseTags)
{
    createAndVerifyStyledText(u8"Simple <I>styled</i> text.", u8"Simple styled text.", 1);

    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator, "Simple "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, "styled"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, " text."));
}

TEST_F(StyledTextTest, UnneededSpansSimple)
{
    createAndVerifyStyledText(u8"<i></i>", u8"", 0);
    createAndVerifyStyledText(u8"<i><b></b></i>", u8"", 0);
    createAndVerifyStyledText(u8"<i><br></i>", u8"", 1);

    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeLineBreak));
}

TEST_F(StyledTextTest, UnneededSpansCollapse)
{
    createAndVerifyStyledText(u8"<i>span</i><i>calypse</i>", u8"spancalypse", 1);

    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, "spancalypse"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
}
TEST_F(StyledTextTest, UnneededSpansCollapseComplex)
{
    createAndVerifyStyledText(u8"<b><i>span</i><i>ca</i></b><i>lypse</i>", u8"spancalypse", 3);

    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, "spanca"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, "lypse"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
}

TEST_F(StyledTextTest, TagAttribute)
{
    // non-styled tags should be deleted even with attributes
    createAndVerifyStyledText(u8"Hello <break time='1000ms'>this is an attr", u8"Hello this is an attr", 0);
    createAndVerifyStyledText(u8"Hello <break time=\"1000ms\">this is an attr", u8"Hello this is an attr", 0);

    // single attr
    createAndVerifyStyledText(u8"Hello <i foo='bar'>this</i> is an attr", u8"Hello this is an attr", 1);
    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator, "Hello "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeItalic));
    ASSERT_EQ(0, iterator.getSpanAttributes().size());
    ASSERT_TRUE(verifyText(iterator, "this"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator, " is an attr"));

    // multiple attributes
    createAndVerifyStyledText(u8"Hello <i foo='bar' baz = \"biz\" fee='fi' fo='fum'>this</i> is an attr", u8"Hello this is an attr", 1);
    auto iterator2 = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator2, "Hello "));
    ASSERT_TRUE(verifySpanStart(iterator2, StyledText::kSpanTypeItalic));
    ASSERT_EQ(0, iterator2.getSpanAttributes().size());
    ASSERT_TRUE(verifyText(iterator2, "this"));
    ASSERT_TRUE(verifySpanEnd(iterator2, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator2, " is an attr"));

    // special allowed characters for attribute name and value
    createAndVerifyStyledText(u8"Hello <i _.-..=\"&:--asd;\">this</i> is an attr", u8"Hello this is an attr", 1);
    auto iterator3 = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator3, "Hello "));
    ASSERT_TRUE(verifySpanStart(iterator3, StyledText::kSpanTypeItalic));
    ASSERT_EQ(0, iterator.getSpanAttributes().size());
    ASSERT_TRUE(verifyText(iterator3, "this"));
    ASSERT_TRUE(verifySpanEnd(iterator3, StyledText::kSpanTypeItalic));
    ASSERT_TRUE(verifyText(iterator3, " is an attr"));

    // special allowed characters for break tag's attribute name and value
    createAndVerifyStyledText(u8"Hello <br :.a.2.3=\"&:--asd;\">this</br> is an attr", u8"Hellothis is an attr", 1);
    auto iterator4 = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator4, "Hello"));
    ASSERT_TRUE(verifySpanStart(iterator4, StyledText::kSpanTypeLineBreak));
    ASSERT_EQ(0, iterator4.getSpanAttributes().size());
    ASSERT_TRUE(verifySpanEnd(iterator4, StyledText::kSpanTypeLineBreak));
    ASSERT_TRUE(verifyText(iterator4, "this is an attr"));

    // using special start character and all three types of entity references
    createAndVerifyStyledText(u8"Hello <br _foo=\"$:my^ref;\" />this is an <i :attr1='&#xaB23;' :attr2='&#23;' :attr3='&mystringref;'>attr</i>", u8"Hellothis is an attr", 2);
    auto iterator5 = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator5, "Hello"));
    ASSERT_TRUE(verifySpanStart(iterator5, StyledText::kSpanTypeLineBreak));
    ASSERT_EQ(0, iterator5.getSpanAttributes().size());
    ASSERT_TRUE(verifySpanEnd(iterator5, StyledText::kSpanTypeLineBreak));
    ASSERT_TRUE(verifyText(iterator5, "this is an "));
    ASSERT_TRUE(verifySpanStart(iterator5, StyledText::kSpanTypeItalic));
    ASSERT_EQ(0, iterator5.getSpanAttributes().size());
    ASSERT_TRUE(verifyText(iterator5, "attr"));
    ASSERT_TRUE(verifySpanEnd(iterator5, StyledText::kSpanTypeItalic));

    // Checking for dec entity collisions
    createAndVerifyStyledText(u8"go &#8594; <i attr='&#8594;'>right</i>", u8"go \u2192 right", 1);
    auto iterator6 = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator6, u8"go \u2192 "));
    ASSERT_TRUE(verifySpanStart(iterator6, StyledText::kSpanTypeItalic));
    ASSERT_EQ(0, iterator6.getSpanAttributes().size());
    ASSERT_TRUE(verifyText(iterator6, "right"));
    ASSERT_TRUE(verifySpanEnd(iterator6, StyledText::kSpanTypeItalic));

    createAndVerifyStyledText(u8"hello <i name='value\">world</i>", u8"hello world", 1);
    createAndVerifyStyledText(u8"hello<br name='value\">world", u8"helloworld", 1);
    createAndVerifyStyledText(u8"hello<br name=\"va\"lue\">world", u8"helloworld", 1);
    createAndVerifyStyledText(u8"hello<br name='va'lue'>world", u8"helloworld", 1);
    createAndVerifyStyledText(u8"hello<br +name='value'>world", u8"helloworld", 1);
    createAndVerifyStyledText(u8"hello<br 1+n:a-me='value'>world", u8"helloworld", 1);
    createAndVerifyStyledText(u8"hello<br name='va<lue' >world", u8"helloworld", 1);

    // cat literally walks across the keyboard
    createAndVerifyStyledText(u8"hello<br 3459dfiuwcr9ergh da lia e  =ar -e 89q3 403i4 ''\"<<<<''' << k'asd \" />world", u8"helloworld", 1);

    createAndVerifyStyledText(u8"hello<span color='red' 3459dfiuwcr9ergh da lia e  =ar -e 89q3 403i4 ''\"<<<<''' << k'asd \" >world</span>", u8"helloworld", 1);
    auto iterator7 = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator7, "hello"));
    ASSERT_TRUE(verifySpanStart(iterator7, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyText(iterator7, "world"));
    ASSERT_TRUE(verifySpanEnd(iterator7, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyColorAttribute(iterator7, 0, "#ff0000ff"));

    // span tag with attributes
    createAndVerifyStyledText(u8"Hello <span color='red'>this is an attr</span>", u8"Hello this is an attr", 1);
    auto iterator8 = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator8, "Hello "));
    ASSERT_TRUE(verifySpanStart(iterator8, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyText(iterator8, "this is an attr"));
    ASSERT_TRUE(verifySpanEnd(iterator8, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyColorAttribute(iterator8, 0, "#ff0000ff"));

    createAndVerifyStyledText(u8"Hello <span fontSize='48dp'>this is an attr</span>", u8"Hello this is an attr", 1);
    auto iterator9 = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator9, "Hello "));
    ASSERT_TRUE(verifySpanStart(iterator9, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyText(iterator9, "this is an attr"));
    ASSERT_TRUE(verifySpanEnd(iterator9, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyFontSizeAttribute(iterator9, 0, "48dp"));

    // span tag with attribute name with resource binding
    createAndVerifyStyledText(u8"Hello <span fontSize='@testFontSize'>this is an attr</span>", u8"Hello this is an attr", 1);
    auto iterator10 = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator10, "Hello "));
    ASSERT_TRUE(verifySpanStart(iterator10, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyText(iterator10, "this is an attr"));
    ASSERT_TRUE(verifySpanEnd(iterator10, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyFontSizeAttribute(iterator10, 0, "10dp"));

    // span tag with multiple attributes
    createAndVerifyStyledText(u8"Hello <span color='red' fontSize='48dp'>this is an attr</span>", u8"Hello this is an attr", 1);
    auto iterator11 = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator11, "Hello "));
    ASSERT_TRUE(verifySpanStart(iterator11, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyText(iterator11, "this is an attr"));
    ASSERT_TRUE(verifySpanEnd(iterator11, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyColorAttribute(iterator11, 0, "#ff0000ff"));
    ASSERT_TRUE(verifyFontSizeAttribute(iterator11, 1, "48dp"));

    // span tag with different kinds of color attributes
    createAndVerifyStyledText(u8"Hello <span color='#edb'>this is an attr</span>", u8"Hello this is an attr", 1);
    auto iterator12 = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator12, "Hello "));
    ASSERT_TRUE(verifySpanStart(iterator12, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyText(iterator12, "this is an attr"));
    ASSERT_TRUE(verifySpanEnd(iterator12, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyColorAttribute(iterator12, 0, "#eeddbbff"));

    createAndVerifyStyledText(u8"Hello <span color='rgba(blue, 50%)'>this is an attr</span>", u8"Hello this is an attr", 1);
    auto iterator13 = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator13, "Hello "));
    ASSERT_TRUE(verifySpanStart(iterator13, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyText(iterator13, "this is an attr"));
    ASSERT_TRUE(verifySpanEnd(iterator13, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyColorAttribute(iterator13, 0, "#0000ff7f"));

    createAndVerifyStyledText(u8"Hello <span color='rgb(rgba(green, 50%), 50%)'>this is an attr</span>", u8"Hello this is an attr", 1);
    auto iterator14 = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator14, "Hello "));
    ASSERT_TRUE(verifySpanStart(iterator14, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyText(iterator14, "this is an attr"));
    ASSERT_TRUE(verifySpanEnd(iterator14, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyColorAttribute(iterator14, 0, "#0080003f"));

    createAndVerifyStyledText(u8"Hello <span color='hsl(0, 100%, 50%)'>this is an attr</span>", u8"Hello this is an attr", 1);
    auto iterator15 = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator15, "Hello "));
    ASSERT_TRUE(verifySpanStart(iterator15, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyText(iterator15, "this is an attr"));
    ASSERT_TRUE(verifySpanEnd(iterator15, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyColorAttribute(iterator15, 0, "#ff0000ff"));

    createAndVerifyStyledText(u8"Hello <span color='hsla(120, 0, 50%, 25%)'>this is an attr</span>", u8"Hello this is an attr", 1);
    auto iterator16 = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator16, "Hello "));
    ASSERT_TRUE(verifySpanStart(iterator16, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyText(iterator16, "this is an attr"));
    ASSERT_TRUE(verifySpanEnd(iterator16, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyColorAttribute(iterator16, 0, "#80808040"));

    // span tag with inherit attribute value
    createAndVerifyStyledText(u8"Hello <span color='inherit'>this is an attr</span>", u8"Hello this is an attr", 1);
    auto iterator17 = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator17, "Hello "));
    ASSERT_TRUE(verifySpanStart(iterator17, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyText(iterator17, "this is an attr"));
    ASSERT_TRUE(verifySpanEnd(iterator17, StyledText::kSpanTypeSpan));

    // span tag with same attributes
    createAndVerifyStyledText(u8"Hello <span color='blue' fontSize='50' color='red' fontSize='7'>this is an attr</span>", u8"Hello this is an attr", 1);
    auto iterator18 = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator18, "Hello "));
    ASSERT_TRUE(verifySpanStart(iterator18, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyText(iterator18, "this is an attr"));
    ASSERT_TRUE(verifySpanEnd(iterator18, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyColorAttribute(iterator18, 0, "#0000ffff"));
    ASSERT_TRUE(verifyFontSizeAttribute(iterator18, 1, "50dp"));

    // span tag without attributes
    createAndVerifyStyledText(u8"Hello <span>this is an attr</span>", u8"Hello this is an attr", 1);
    auto iterator19 = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator19, "Hello "));
    ASSERT_TRUE(verifySpanStart(iterator19, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyText(iterator19, "this is an attr"));
    ASSERT_TRUE(verifySpanEnd(iterator19, StyledText::kSpanTypeSpan));

    // span tag with non-supported attributes
    createAndVerifyStyledText(u8"Hello <span foo='bar'>this is an attr</span>", u8"Hello this is an attr", 1);
    auto iterator20 = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator20, "Hello "));
    ASSERT_TRUE(verifySpanStart(iterator20, StyledText::kSpanTypeSpan));
    ASSERT_TRUE(verifyText(iterator20, "this is an attr"));
    ASSERT_TRUE(verifySpanEnd(iterator20, StyledText::kSpanTypeSpan));
}

TEST_F(StyledTextTest, NobrSimple)
{
    createAndVerifyStyledText(u8"He screamed \"Run <NOBR>faster</nobr>the<noBR>tiger is</NObr>right<nobr/><nobr />behind<nobr>you!!!</nobr>\"",
                              u8"He screamed \"Run fasterthetiger isrightbehindyou!!!\"", 3);
    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator, u8"He screamed \"Run "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "faster"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "the"));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "tiger is"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "rightbehind"));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "you!!!"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "\""));
}

TEST_F(StyledTextTest, NobrMerge)
{
    // Only some tags can be merged. For example "<b>te</b><b>xt</b>" can become "<b>text</b>"
    createAndVerifyStyledText(u8"<nobr>This should not</nobr><nobr> merge</nobr> into one big tag",
                              u8"This should not merge into one big tag", 2);
    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "This should not"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, " merge"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, " into one big tag"));
}

TEST_F(StyledTextTest, NobrNested)
{
    createAndVerifyStyledText(u8"He screamed \"Run <NOBR><nobr><nobr>faster</nobr></nobr></nobr>the<noBR>tig<nobr>er </nobr>is</NObr>"
                              "right<nobr/><nobr />behind<nobr><nobr>you!</nobr>!!</nobr>\"",
                              u8"He screamed \"Run fasterthetiger isrightbehindyou!!!\"", 7);
    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator, "He screamed \"Run "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "faster"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "the"));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "tig"));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "er "));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "is"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "rightbehind"));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "you!"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "!!"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "\""));
}

TEST_F(StyledTextTest, NobrComplex)
{
    createAndVerifyStyledText(u8"He screamed \"Run <NOBR><nobr><br>fas<br>ter</nobr></nobr><b>the<noBR>tig<nobr>er </nobr>i</b>s</NObr>"
                              "right<nobr/><nobr />behind<nobr><nobr>you!</nobr>!!</nobr>\"",
                              u8"He screamed \"Run fasterthetiger isrightbehindyou!!!\"", 10);
    auto iterator = StyledText::Iterator(styledText.get<StyledText>());
    ASSERT_TRUE(verifyText(iterator, u8"He screamed \"Run "));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeLineBreak));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeLineBreak));
    ASSERT_TRUE(verifyText(iterator, "fas"));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeLineBreak));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeLineBreak));
    ASSERT_TRUE(verifyText(iterator, "ter"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifyText(iterator, "the"));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "tig"));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "er "));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "i"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeStrong));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "s"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "rightbehind"));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifySpanStart(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "you!"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "!!"));
    ASSERT_TRUE(verifySpanEnd(iterator, StyledText::kSpanTypeNoBreak));
    ASSERT_TRUE(verifyText(iterator, "\""));
}

TEST_F(StyledTextTest, StyledTextIteratorBasic)
{
    createAndVerifyStyledText(u8"He screamed \"<span color='red'>Run</span><u>faster<i>thetigerisbehind</i></u><i>you</i>!!!\"",
            u8"He screamed \"Runfasterthetigerisbehindyou!!!\"", 4);

    StyledText st = styledText.get<StyledText>();
    StyledText::Iterator it(st);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), "He screamed \"");

    EXPECT_EQ(it.next(), StyledText::Iterator::kStartSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeSpan);
    auto attribute = it.getSpanAttributes().at(0);
    EXPECT_EQ(attribute.name, 0);
    EXPECT_TRUE(attribute.value.is<Color>());
    EXPECT_EQ(attribute.value.asString(), "#ff0000ff");

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), "Run");

    EXPECT_EQ(it.next(), StyledText::Iterator::kEndSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeSpan);

    EXPECT_EQ(it.next(), StyledText::Iterator::kStartSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeUnderline);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), "faster");

    EXPECT_EQ(it.next(), StyledText::Iterator::kStartSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeItalic);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), "thetigerisbehind");

    EXPECT_EQ(it.next(), StyledText::Iterator::kEndSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeItalic);

    EXPECT_EQ(it.next(), StyledText::Iterator::kEndSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeUnderline);

    EXPECT_EQ(it.next(), StyledText::Iterator::kStartSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeItalic);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), "you");

    EXPECT_EQ(it.next(), StyledText::Iterator::kEndSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeItalic);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), "!!!\"");

    EXPECT_EQ(it.next(), StyledText::Iterator::kEnd);
}

TEST_F(StyledTextTest, StyledTextIteratorEmpty)
{
    createAndVerifyStyledText("", "", 0);

    StyledText st = styledText.get<StyledText>();
    StyledText::Iterator it(st);

    EXPECT_EQ(it.next(), StyledText::Iterator::kEnd);
}

TEST_F(StyledTextTest, StyledTextSpanEquality)
{
    auto st1 = StyledText::create(*context, u8"He screamed <b>\"Runfasterthetigerisbehindyou!!!\"</b>");
    auto st2 = StyledText::create(*context, u8"He screamed <b>\"Runslowerthepuppywantstolickyou\"</b>");
    auto st1Spans = StyledText::Iterator(st1);
    auto st2Spans = StyledText::Iterator(st2);

    EXPECT_EQ(st1Spans.spanCount(), st2Spans.spanCount());
    st1Spans.next();
    st2Spans.next();
    EXPECT_EQ(st1Spans.getString(), st2Spans.getString());
    st1Spans.next();
    st2Spans.next();
    EXPECT_EQ(st1Spans.getSpanType(), st2Spans.getSpanType());
    st1Spans.next();
    st2Spans.next();
    EXPECT_NE(st1Spans.getString(), st2Spans.getString());
    st1Spans.next();
    st2Spans.next();
    EXPECT_EQ(st1Spans.getSpanType(), st2Spans.getSpanType());
}

TEST_F(StyledTextTest, StyledTextSpanWithAttributesEquality)
{
    auto st1 = StyledText::create(*context, u8"He screamed <span color='red'>\"Runfasterthetigerisbehindyou!!!\"</span>");
    auto st2 = StyledText::create(*context, u8"He screamed <span color='red'>\"Runslowerthepuppywantstolickyou\"</span>");
    auto st1Spans = StyledText::Iterator(st1);
    auto st2Spans = StyledText::Iterator(st2);

    EXPECT_EQ(st1Spans.spanCount(), st2Spans.spanCount());
    st1Spans.next();
    st2Spans.next();
    EXPECT_EQ(st1Spans.getString(), st2Spans.getString());
    st1Spans.next();
    st2Spans.next();
    EXPECT_EQ(st1Spans.getSpanType(), st2Spans.getSpanType());
    EXPECT_EQ(st1Spans.getSpanAttributes(), st2Spans.getSpanAttributes());
    st1Spans.next();
    st2Spans.next();
    EXPECT_NE(st1Spans.getString(), st2Spans.getString());
    st1Spans.next();
    st2Spans.next();
    EXPECT_EQ(st1Spans.getSpanType(), st2Spans.getSpanType());

}

TEST_F(StyledTextTest, StyledTextSpanInequality)
{
    auto st1 = StyledText::create(*context, u8"He screamed <b>\"Runfasterthetigerisbehindyou!!!\"</b>");
    auto st2 = StyledText::create(*context, u8"He screamed <b>\"Runslowertheturtleneedstolickyou\"</b>");
    auto st1Spans = StyledText::Iterator(st1);
    auto st2Spans = StyledText::Iterator(st2);

    EXPECT_EQ(st1Spans.spanCount(), st2Spans.spanCount());
    st1Spans.next();
    st2Spans.next();
    EXPECT_EQ(st1Spans.getString(), st2Spans.getString());
    st1Spans.next();
    st2Spans.next();
    EXPECT_EQ(st1Spans.getSpanType(), st2Spans.getSpanType());
    st1Spans.next();
    st2Spans.next();
    EXPECT_NE(st1Spans.getString(), st2Spans.getString());
    st1Spans.next();
    st2Spans.next();
    EXPECT_EQ(st1Spans.getSpanType(), st2Spans.getSpanType());

}

TEST_F(StyledTextTest, StyledTextSpanWithAttributesInequality)
{
    auto st1 = StyledText::create(*context, u8"He screamed <span color='red'>\"Runfasterthetigerisbehindyou!!!\"</span>");
    auto st2 = StyledText::create(*context, u8"He screamed <span color='blue'>\"Runslowerthepuppywantstolickyou\"</span>");
    auto st1Spans = StyledText::Iterator(st1);
    auto st2Spans = StyledText::Iterator(st2);

    EXPECT_EQ(st1Spans.spanCount(), st2Spans.spanCount());
    st1Spans.next();
    st2Spans.next();
    EXPECT_EQ(st1Spans.getString(), st2Spans.getString());
    st1Spans.next();
    st2Spans.next();
    EXPECT_EQ(st1Spans.getSpanType(), st2Spans.getSpanType());
    EXPECT_NE(st1Spans.getSpanAttributes(), st2Spans.getSpanAttributes());
    st1Spans.next();
    st2Spans.next();
    EXPECT_NE(st1Spans.getString(), st2Spans.getString());
    st1Spans.next();
    st2Spans.next();
    EXPECT_EQ(st1Spans.getSpanType(), st2Spans.getSpanType());

}

TEST_F(StyledTextTest, StyledTextTruthy)
{
    createAndVerifyStyledText(u8"He screamed \"Runfasterthetigerisbehindyou!!!\"",
                              u8"He screamed \"Runfasterthetigerisbehindyou!!!\"", 0);

    StyledText st = styledText.get<StyledText>();
    EXPECT_TRUE(st.truthy());

    createAndVerifyStyledText(u8"He screamed <b>\"Runfasterthetigerisbehindyou!!!\"</b>",
                              u8"He screamed \"Runfasterthetigerisbehindyou!!!\"", 1);

    st = styledText.get<StyledText>();
    EXPECT_TRUE(st.truthy());

    createAndVerifyStyledText(u8"",
                              u8"", 0);

    st = styledText.get<StyledText>();
    EXPECT_FALSE(st.truthy());
}

TEST_F(StyledTextTest, StyledTextIteratorNoTags)
{
    createAndVerifyStyledText(u8"He screamed \"Runfasterthetigerisbehindyou!!!\"",
                              u8"He screamed \"Runfasterthetigerisbehindyou!!!\"", 0);

    StyledText st = styledText.get<StyledText>();
    StyledText::Iterator it(st);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), u8"He screamed \"Runfasterthetigerisbehindyou!!!\"");

    EXPECT_EQ(it.next(), StyledText::Iterator::kEnd);
}

TEST_F(StyledTextTest, CollapseWhiteSpaceSurroundingSpans)
{
    createAndVerifyStyledText(u8"Example 1:<b>hello</b> <b>world</b>",
                              u8"Example 1:hello world", 2);
    createAndVerifyStyledText(u8"Example 2: <b> hola</b> <b>mundo</b>",
                              u8"Example 2: hola mundo", 2);
    createAndVerifyStyledText(u8"Example 3:<b> hallo </b> <b>welt</b>",
                              u8"Example 3: hallo welt", 2);
    createAndVerifyStyledText(u8"Example 4: <b> ciao   </b> <b>    mondo </b>",
                              u8"Example 4: ciao mondo ", 2);
    createAndVerifyStyledText(u8"Example 5:<i> bonjour </i> <i>le monde</i>",
                              u8"Example 5: bonjour le monde", 2);
    createAndVerifyStyledText(u8"Example 6: hello   <b/>world",
                              u8"Example 6: hello world", 0);
    createAndVerifyStyledText(u8"Example 7:<u> hello </u> <u>underline</u>",
                              u8"Example 7: hello underline", 2);
    createAndVerifyStyledText(u8"Example 8: <b>hello </b><b>merge</b>",
                              u8"Example 8: hello merge", 1);
    createAndVerifyStyledText(u8"Example 9: <b>hello </b><b> merge</b>",
                              u8"Example 9: hello merge", 1);
    createAndVerifyStyledText(u8"Example 10: <b>hello </b> <i><b> potato</b></i>",
                              u8"Example 10: hello potato", 3);
}

TEST_F(StyledTextTest, SpanMultipleBreaklines)
{
    createAndVerifyStyledText(u8"Example 1:<b>hello</b> <b>world</b><br>Example 2: <b> hola</b> <b>mundo</b><br>Example 3:<b> hallo </b> <b>welt</b><br>Example 4: <b> ciao   </b> <b>    mondo</b>",
                              u8"Example 1:hello worldExample 2: hola mundoExample 3: hallo weltExample 4: ciao mondo", 11 /* 2 per line + 3 break lines 2 * 4 + 3 = 11*/);
    auto st = styledText.get<StyledText>();
    StyledText::Iterator it(st);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), u8"Example 1:");

    EXPECT_EQ(it.next(), StyledText::Iterator::kStartSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeStrong);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), u8"hello");

    EXPECT_EQ(it.next(), StyledText::Iterator::kEndSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeStrong);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), u8" ");

    EXPECT_EQ(it.next(), StyledText::Iterator::kStartSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeStrong);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), u8"world");

    EXPECT_EQ(it.next(), StyledText::Iterator::kEndSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeStrong);

    EXPECT_EQ(it.next(), StyledText::Iterator::kStartSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeLineBreak);

    EXPECT_EQ(it.next(), StyledText::Iterator::kEndSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeLineBreak);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), u8"Example 2: ");

    EXPECT_EQ(it.next(), StyledText::Iterator::kStartSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeStrong);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), u8"hola");

    EXPECT_EQ(it.next(), StyledText::Iterator::kEndSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeStrong);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), u8" ");

    EXPECT_EQ(it.next(), StyledText::Iterator::kStartSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeStrong);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), u8"mundo");

    EXPECT_EQ(it.next(), StyledText::Iterator::kEndSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeStrong);

    EXPECT_EQ(it.next(), StyledText::Iterator::kStartSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeLineBreak);

    EXPECT_EQ(it.next(), StyledText::Iterator::kEndSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeLineBreak);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), u8"Example 3:");

    EXPECT_EQ(it.next(), StyledText::Iterator::kStartSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeStrong);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), u8" hallo ");

    EXPECT_EQ(it.next(), StyledText::Iterator::kEndSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeStrong);

    EXPECT_EQ(it.next(), StyledText::Iterator::kStartSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeStrong);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), u8"welt");

    EXPECT_EQ(it.next(), StyledText::Iterator::kEndSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeStrong);

    EXPECT_EQ(it.next(), StyledText::Iterator::kStartSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeLineBreak);

    EXPECT_EQ(it.next(), StyledText::Iterator::kEndSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeLineBreak);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), u8"Example 4: ");

    EXPECT_EQ(it.next(), StyledText::Iterator::kStartSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeStrong);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), u8"ciao ");

    EXPECT_EQ(it.next(), StyledText::Iterator::kEndSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeStrong);

    EXPECT_EQ(it.next(), StyledText::Iterator::kStartSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeStrong);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), u8"mondo");

    EXPECT_EQ(it.next(), StyledText::Iterator::kEndSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeStrong);
}

TEST_F(StyledTextTest, SpanTransitionUnicodes)
{
    createAndVerifyStyledText(u8"\u524D\u9031\n\u672B<i>\u6BD434\u5186</i>80\u92AD<br>",
                              u8"\u524D\u9031 \u672B\u6BD434\u518680\u92AD", 2);

    StyledText st = styledText.get<StyledText>();
    StyledText::Iterator it(st);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), u8"\u524D\u9031 \u672B");

    EXPECT_EQ(it.next(), StyledText::Iterator::kStartSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeItalic);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), u8"\u6BD434\u5186");

    EXPECT_EQ(it.next(), StyledText::Iterator::kEndSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeItalic);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), u8"80\u92AD");

    EXPECT_EQ(it.next(), StyledText::Iterator::kStartSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeLineBreak);

    EXPECT_EQ(it.next(), StyledText::Iterator::kEndSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeLineBreak);

    EXPECT_EQ(it.next(), StyledText::Iterator::kEnd);
}
