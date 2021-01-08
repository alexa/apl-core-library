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

#include <future>

#include "../testeventloop.h"

using namespace apl;

class StyledTextTest : public ::testing::Test {
protected:
    void createAndVerifyStyledText(const std::string& rawText, const std::string& expectedText, size_t spansCount) {
        styledText = StyledText::create(rawText);
        ASSERT_EQ(Object::kStyledTextType, styledText.getType());
        spans = std::make_shared<std::vector<StyledText::Span>>(styledText.getStyledText().getSpans());
        ASSERT_EQ(expectedText, styledText.getStyledText().getText());
        ASSERT_EQ(spansCount, spans->size());
    }

    void verifySpan(size_t spanIndex, StyledText::SpanType type, size_t start, size_t end) {
        auto span = spans->at(spanIndex);
        ASSERT_EQ(type, span.type);
        ASSERT_EQ(start, span.start);
        ASSERT_EQ(end, span.end);
    }

private:
    Object styledText;
    std::shared_ptr<std::vector<StyledText::Span>> spans;
};

TEST_F(StyledTextTest, Casting)
{
    ASSERT_TRUE(IsEqual("<i>FOO</i>", StyledText::create("<i>FOO</i>").asString()));
    ASSERT_TRUE(IsEqual(4.5, StyledText::create("4.5").asNumber()));
    ASSERT_TRUE(IsEqual(4, StyledText::create("4.3").asInt()));
    ASSERT_TRUE(IsEqual(Color(Color::RED), StyledText::create("#ff0000").asColor()));

    auto context = Context::create(Metrics(), makeDefaultSession());
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Absolute, 10), StyledText::create("10dp").asDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(), StyledText::create("auto").asDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Relative, 10), StyledText::create("10%").asDimension(*context)));

    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Absolute, 5), StyledText::create("5dp").asAbsoluteDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Absolute, 0), StyledText::create("auto").asAbsoluteDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Absolute, 0), StyledText::create("10%").asAbsoluteDimension(*context)));

    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Absolute, 5), StyledText::create("5dp").asNonAutoDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Absolute, 0), StyledText::create("auto").asNonAutoDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Relative, 10), StyledText::create("10%").asNonAutoDimension(*context)));

    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Absolute, 5), StyledText::create("5dp").asNonAutoRelativeDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Relative, 0), StyledText::create("auto").asNonAutoRelativeDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Relative, 10), StyledText::create("10%").asNonAutoRelativeDimension(*context)));

    ASSERT_TRUE(StyledText::create("").empty());
    ASSERT_FALSE(StyledText::create("<h2></h2>").empty());

    ASSERT_EQ(0, StyledText::create("").size());
    ASSERT_EQ(9, StyledText::create("<h2></h2>").size());
}

TEST_F(StyledTextTest, NotStyled)
{
    createAndVerifyStyledText(u8"Simple text.", u8"Simple text.", 0);
}

TEST_F(StyledTextTest, Simple)
{
    createAndVerifyStyledText(u8"Simple <i>styled</i> text.", u8"Simple styled text.", 1);
    verifySpan(0, StyledText::kSpanTypeItalic, 7, 13);
}

TEST_F(StyledTextTest, Multiple)
{
    createAndVerifyStyledText(u8"Simple <i>somewhat</i> <u>styled</u> text.", u8"Simple somewhat styled text.", 2);
    verifySpan(0, StyledText::kSpanTypeItalic, 7, 15);
    verifySpan(1, StyledText::kSpanTypeUnderline, 16, 22);
}

TEST_F(StyledTextTest, LineBreak)
{
    createAndVerifyStyledText(u8"Line <br/>break<br> text.", u8"Linebreaktext.", 2);
    verifySpan(0, StyledText::kSpanTypeLineBreak, 4, 4);
    verifySpan(1, StyledText::kSpanTypeLineBreak, 9, 9);
}

TEST_F(StyledTextTest, EscapeCharacters)
{
    createAndVerifyStyledText(u8"Simple\f text\t should\r not\n break\treally.", u8"Simple text should not break really.", 0);
}

TEST_F(StyledTextTest, Wchar)
{
    createAndVerifyStyledText(u8"\u524D\u9031\n\u672B<i>\u6BD434\u5186</i>80\u92AD",
		u8"\u524D\u9031 \u672B\u6BD434\u518680\u92AD", 1);
    verifySpan(0, StyledText::kSpanTypeItalic, 4, 8);
}

TEST_F(StyledTextTest, Cyrillics) {
    // String just means "Russian language"
    createAndVerifyStyledText(u8"\u0440\0443\u0441\u043a\u0438\u0439 <b>\u044F\u0437\u044B\u043a</b>",
		u8"\u0440\0443\u0441\u043a\u0438\u0439 \u044F\u0437\u044B\u043a", 1);
    verifySpan(0, StyledText::kSpanTypeStrong, 8, 12);
}

TEST_F(StyledTextTest, UnclosedTag)
{
    createAndVerifyStyledText(u8"Unclosed<i> tag.", u8"Unclosed tag.", 1);
    verifySpan(0, StyledText::kSpanTypeItalic, 8, 13);
}

TEST_F(StyledTextTest, UnclosedTagIntersect)
{
    createAndVerifyStyledText(u8"This is<b> bold text,<i> this is bold-italic</b> and </i>plain.",
                              u8"This is bold text, this is bold-italic and plain.", 3);
    verifySpan(0, StyledText::kSpanTypeStrong, 7, 38);
    verifySpan(1, StyledText::kSpanTypeItalic, 18, 38);
    verifySpan(2, StyledText::kSpanTypeItalic, 38, 43);
}

TEST_F(StyledTextTest, UnopenedTag)
{
    createAndVerifyStyledText(u8"Unopened</i> tag.", u8"Unopened tag.", 0);
}

TEST_F(StyledTextTest, UnopenedTagComplex)
{
    createAndVerifyStyledText(u8"<b>Hello, <i>I'm a turtle</sub> who likes lettuce.</i></b>",
                              u8"Hello, I'm a turtle who likes lettuce.", 2);
    verifySpan(0, StyledText::kSpanTypeStrong, 0, 38);
    verifySpan(1, StyledText::kSpanTypeItalic, 7, 38);
}

TEST_F(StyledTextTest, UnclosedTagComplex)
{
    createAndVerifyStyledText(u8"Multiple <b>very <u>unclosed<i> tags</b>. And few <tt>unclosed <strike>at the end.",
                              u8"Multiple very unclosed tags. And few unclosed at the end.", 7);
    verifySpan(0, StyledText::kSpanTypeStrong, 9, 27);
    verifySpan(1, StyledText::kSpanTypeUnderline, 14, 27);
    verifySpan(2, StyledText::kSpanTypeItalic, 22, 27);
    verifySpan(3, StyledText::kSpanTypeItalic, 27, 57);
    verifySpan(4, StyledText::kSpanTypeUnderline, 27, 57);
    verifySpan(5, StyledText::kSpanTypeMonospace, 37, 57);
    verifySpan(6, StyledText::kSpanTypeStrike, 46, 57);
}

TEST_F(StyledTextTest, UnsupportedTag)
{
    createAndVerifyStyledText(u8"Text with <ul><li>unsupported</li></ul> or wrong<b/> tag.",
                              u8"Text with unsupported or wrong tag.", 0);
}

TEST_F(StyledTextTest, SingleChildStyle)
{
    createAndVerifyStyledText(u8"Text <i>with <b>one</b> child</i>.", u8"Text with one child.", 2);
    verifySpan(0, StyledText::kSpanTypeItalic, 5, 19);
    verifySpan(1, StyledText::kSpanTypeStrong, 10, 13);
}

TEST_F(StyledTextTest, SeveralChildStyles)
{
    createAndVerifyStyledText(u8"Text <i>with <b>child</b> and another <u>child</u></i>.",
                              u8"Text with child and another child.", 3);
    verifySpan(0, StyledText::kSpanTypeItalic, 5, 33);
    verifySpan(1, StyledText::kSpanTypeStrong, 10, 15);
    verifySpan(2, StyledText::kSpanTypeUnderline, 28, 33);
}

TEST_F(StyledTextTest, CollapseSpaces)
{
    createAndVerifyStyledText(u8"Text    value.", u8"Text value.", 0);
    createAndVerifyStyledText(u8"     foo     ", u8"foo", 0);
    createAndVerifyStyledText(u8" and<br>this ", u8"andthis", 1);
    verifySpan(0, StyledText::kSpanTypeLineBreak, 3, 3);
    createAndVerifyStyledText(u8" this is a <br> test of whitespace ", u8"this is atest of whitespace", 1);
    verifySpan(0, StyledText::kSpanTypeLineBreak, 9, 9);
}

TEST_F(StyledTextTest, Complex)
{
    createAndVerifyStyledText(u8" Since <i>you</i> are <magic>not</magic> going <u>on a? <b>holiday <em>this</em></b> year! "
                              "Boss,</u><br> <strong>I\f    thought</strong> I\t <strike><tt>should</tt> <sup>give</sup> "
                              "your</strike>\r <sUb>office</suB>\n a <code>holiday;</code> \u524D\u9031\u672B<i>\u6BD434\u518680\u92ad look. ",
                              u8"Since you are not going on a? holiday this year! Boss,I thought I should give your office "
                              "a holiday; \u524D\u9031\u672B\u6BD434\u518680\u92ad look.", 12);
    verifySpan(0, StyledText::kSpanTypeItalic, 6, 9);
    verifySpan(1, StyledText::kSpanTypeUnderline, 24, 54);
    verifySpan(2, StyledText::kSpanTypeStrong, 30, 42);
    verifySpan(3, StyledText::kSpanTypeItalic, 38, 42);
    verifySpan(4, StyledText::kSpanTypeStrong, 54, 63);
    verifySpan(5, StyledText::kSpanTypeLineBreak, 54, 54);
    verifySpan(6, StyledText::kSpanTypeStrike, 66, 82);
    verifySpan(7, StyledText::kSpanTypeMonospace, 66, 72);
    verifySpan(8, StyledText::kSpanTypeSuperscript, 73, 77);
    verifySpan(9, StyledText::kSpanTypeSubscript, 83, 89);
    verifySpan(10, StyledText::kSpanTypeMonospace, 92, 100);
    verifySpan(11, StyledText::kSpanTypeItalic, 104, 117);
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
    verifySpan(0, StyledText::kSpanTypeItalic, 5, 10);
}

TEST_F(StyledTextTest, UppercaseTags)
{
    createAndVerifyStyledText(u8"Simple <I>styled</i> text.", u8"Simple styled text.", 1);
    verifySpan(0, StyledText::kSpanTypeItalic, 7, 13);
}

TEST_F(StyledTextTest, UnneededSpansSimple)
{
    createAndVerifyStyledText(u8"<i></i>", u8"", 0);
    createAndVerifyStyledText(u8"<i><b></b></i>", u8"", 0);
    createAndVerifyStyledText(u8"<i><br></i>", u8"", 1);
    verifySpan(0, StyledText::kSpanTypeLineBreak, 0, 0);
}

TEST_F(StyledTextTest, UnneededSpansCollapse)
{
    createAndVerifyStyledText(u8"<i>span</i><i>calypse</i>", u8"spancalypse", 1);
    verifySpan(0, StyledText::kSpanTypeItalic, 0, 11);
}
TEST_F(StyledTextTest, UnneededSpansCollapseComplex)
{
    createAndVerifyStyledText(u8"<b><i>span</i><i>ca</i></b><i>lypse</i>", u8"spancalypse", 3);
    verifySpan(0, StyledText::kSpanTypeStrong, 0, 6);
    verifySpan(1, StyledText::kSpanTypeItalic, 0, 6);
    verifySpan(2, StyledText::kSpanTypeItalic, 6, 11);
}

TEST_F(StyledTextTest, TagAttribute)
{
    // non-styled tags should be deleted even with attributes
    createAndVerifyStyledText(u8"Hello <break time='1000ms'>this is an attr", u8"Hello this is an attr", 0);
    createAndVerifyStyledText(u8"Hello <break time=\"1000ms\">this is an attr", u8"Hello this is an attr", 0);

    // single attr
    createAndVerifyStyledText(u8"Hello <i foo='bar'>this</i> is an attr", u8"Hello this is an attr", 1);
    verifySpan(0, StyledText::kSpanTypeItalic, 6, 10);

    // multiple attributes
    createAndVerifyStyledText(u8"Hello <i foo='bar' baz = \"biz\" fee='fi' fo='fum'>this</i> is an attr", u8"Hello this is an attr", 1);
    verifySpan(0, StyledText::kSpanTypeItalic, 6, 10);

    // special allowed characters for attribute name and value
    createAndVerifyStyledText(u8"Hello <i _.-..=\"&:--asd;\">this</i> is an attr", u8"Hello this is an attr", 1);
    verifySpan(0, StyledText::kSpanTypeItalic, 6, 10);

    // special allowed characters for break tag's attribute name and value
    createAndVerifyStyledText(u8"Hello <br :.a.2.3=\"&:--asd;\">this</br> is an attr", u8"Hellothis is an attr", 1);
    verifySpan(0, StyledText::kSpanTypeLineBreak, 5, 5);

    // using special start character and all three types of entity references
    createAndVerifyStyledText(u8"Hello <br _foo=\"$:my^ref;\" />this is an <i :attr1='&#xaB23;' :attr2='&#23;' :attr3='&mystringref;'>attr</i>", u8"Hellothis is an attr", 2);
    verifySpan(0, StyledText::kSpanTypeLineBreak, 5, 5);
    verifySpan(1, StyledText::kSpanTypeItalic, 16, 20);

    // Checking for dec entity collisions
    createAndVerifyStyledText(u8"go &#8594; <i attr='&#8594;'>right</i>", u8"go \u2192 right", 1);
    verifySpan(0, StyledText::kSpanTypeItalic, 5, 10);

    createAndVerifyStyledText(u8"hello <i name='value\">world</i>", u8"hello world", 1);
    createAndVerifyStyledText(u8"hello<br name='value\">world", u8"helloworld", 1);
    createAndVerifyStyledText(u8"hello<br name=\"va\"lue\">world", u8"helloworld", 1);
    createAndVerifyStyledText(u8"hello<br name='va'lue'>world", u8"helloworld", 1);
    createAndVerifyStyledText(u8"hello<br +name='value'>world", u8"helloworld", 1);
    createAndVerifyStyledText(u8"hello<br 1+n:a-me='value'>world", u8"helloworld", 1);
    createAndVerifyStyledText(u8"hello<br name='va<lue' >world", u8"helloworld", 1);
    createAndVerifyStyledText(u8"hello<br name='va>lue' >world", u8"helloworld", 1);

    // cat literally walks across the keyboard
    createAndVerifyStyledText(u8"hello<br 3459dfiuwcr9ergh da lia e  =ar -e 89q3 403i4 ''\"<<>><<''' << k'asd \" >world", u8"helloworld", 1);
    createAndVerifyStyledText(u8"hello<br 3459dfiuwcr9ergh da lia e  =ar -e 89q3 403i4 ''\"<<<<''' <>< k'asd \" />world", u8"helloworld", 1);
}

TEST_F(StyledTextTest, WbrSimple)
{
    createAndVerifyStyledText(u8"He screamed \"Run<WBR>faster<Wbr>the<wBr>tiger<wbR>is<wbr>right<wbr/>behind< wbr>you!!!\"",
            u8"He screamed \"Runfasterthetigerisrightbehindyou!!!\"", 7);
    verifySpan(0, StyledText::kSpanTypeWordBreak, 16, 16);
    verifySpan(1, StyledText::kSpanTypeWordBreak, 22, 22);
    verifySpan(2, StyledText::kSpanTypeWordBreak, 25, 25);
    verifySpan(3, StyledText::kSpanTypeWordBreak, 30, 30);
    verifySpan(4, StyledText::kSpanTypeWordBreak, 32, 32);
    verifySpan(5, StyledText::kSpanTypeWordBreak, 37, 37);
    verifySpan(6, StyledText::kSpanTypeWordBreak, 43, 43);
}

TEST_F(StyledTextTest, WbrSpaced)
{
    createAndVerifyStyledText(u8"Should break<wbr> here, and also<wbr> <wbr>twice. Once <wbr><wbr>here though.",
                              u8"Should break here, and also twice. Once here though.", 4);
    verifySpan(0, StyledText::kSpanTypeWordBreak, 12, 12);
    verifySpan(1, StyledText::kSpanTypeWordBreak, 27, 27);
    verifySpan(2, StyledText::kSpanTypeWordBreak, 28, 28);
    verifySpan(3, StyledText::kSpanTypeWordBreak, 40, 40);
}

TEST_F(StyledTextTest, WbrComplex)
{
    createAndVerifyStyledText(u8"He screamed \"Run<wbr><wbr>faster<br>the<i>tiger<wbr>is</i>right<wbr><br>behind<wbr>you!!!\"",
                              u8"He screamed \"Runfasterthetigerisrightbehindyou!!!\"", 7);
    verifySpan(0, StyledText::kSpanTypeWordBreak, 16, 16);
    verifySpan(1, StyledText::kSpanTypeLineBreak, 22, 22);
    verifySpan(2, StyledText::kSpanTypeItalic, 25, 32);
    verifySpan(3, StyledText::kSpanTypeWordBreak, 30, 30);
    verifySpan(4, StyledText::kSpanTypeLineBreak, 37, 37);
    verifySpan(5, StyledText::kSpanTypeWordBreak, 37, 37);
    verifySpan(6, StyledText::kSpanTypeWordBreak, 43, 43);
}

TEST_F(StyledTextTest, WbrCollapse)
{
    createAndVerifyStyledText(u8"He screamed \"Run<wbr><wbr>faster<wbr>the<wbr>tiger<wbr><wbr>is<wbr>right<wbr><wbr>behind<wbr>you!!!\"",
                              u8"He screamed \"Runfasterthetigerisrightbehindyou!!!\"", 7);
    verifySpan(0, StyledText::kSpanTypeWordBreak, 16, 16);
    verifySpan(1, StyledText::kSpanTypeWordBreak, 22, 22);
    verifySpan(2, StyledText::kSpanTypeWordBreak, 25, 25);
    verifySpan(3, StyledText::kSpanTypeWordBreak, 30, 30);
    verifySpan(4, StyledText::kSpanTypeWordBreak, 32, 32);
    verifySpan(5, StyledText::kSpanTypeWordBreak, 37, 37);
    verifySpan(6, StyledText::kSpanTypeWordBreak, 43, 43);
}