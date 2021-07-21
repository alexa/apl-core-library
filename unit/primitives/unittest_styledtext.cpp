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
        ASSERT_EQ(Object::kStyledTextType, styledText.getType());
        spans = std::make_shared<std::vector<StyledText::Span>>(styledText.getStyledText().getSpans());
        ASSERT_EQ(expectedText, styledText.getStyledText().getText());
        ASSERT_EQ(spansCount, spans->size());
    }

    void verifySpan(size_t spanIndex, StyledText::SpanType type, size_t start, size_t end) {
        verifySpan(spanIndex, type, start, end, 0);
    }

    void verifySpan(size_t spanIndex, StyledText::SpanType type, size_t start, size_t end, size_t attributesCount) {
        auto span = spans->at(spanIndex);
        ASSERT_EQ(type, span.type);
        ASSERT_EQ(start, span.start);
        ASSERT_EQ(end, span.end);
        ASSERT_EQ(span.attributes.size(), attributesCount);
    }

    void verifyColorAttribute(
        size_t spanIndex,
        size_t attributeIndex,
        const std::string& attributeValue) {
        auto span = spans->at(spanIndex);
        auto attribute = span.attributes.at(attributeIndex);
        EXPECT_EQ(attribute.name, 0);
        EXPECT_TRUE(attribute.value.isColor());
        EXPECT_EQ(attribute.value.asString(), attributeValue);
    }

    void verifyFontSizeAttribute(
        size_t spanIndex,
        size_t attributeIndex,
        const std::string& attributeValue) {
        auto span = spans->at(spanIndex);
        auto attribute = span.attributes.at(attributeIndex);
        EXPECT_EQ(attribute.name, 1);
        EXPECT_TRUE(attribute.value.isAbsoluteDimension());
        EXPECT_EQ(attribute.value.asString(), attributeValue);
    }

    ContextPtr context;
    Object styledText;

private:
    std::shared_ptr<std::vector<StyledText::Span>> spans;
};

TEST_F(StyledTextTest, Casting)
{
    ASSERT_TRUE(IsEqual("<i>FOO</i>", StyledText::create(*context, "<i>FOO</i>").asString()));
    ASSERT_TRUE(IsEqual(4.5, StyledText::create(*context, "4.5").asNumber()));
    ASSERT_TRUE(IsEqual(4, StyledText::create(*context, "4.3").asInt()));
    ASSERT_TRUE(IsEqual(Color(Color::RED), StyledText::create(*context, "#ff0000").asColor()));

    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Absolute, 10), StyledText::create(*context, "10dp").asDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(), StyledText::create(*context, "auto").asDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Relative, 10), StyledText::create(*context, "10%").asDimension(*context)));

    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Absolute, 5), StyledText::create(*context, "5dp").asAbsoluteDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Absolute, 0), StyledText::create(*context, "auto").asAbsoluteDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Absolute, 0), StyledText::create(*context, "10%").asAbsoluteDimension(*context)));

    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Absolute, 5), StyledText::create(*context, "5dp").asNonAutoDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Absolute, 0), StyledText::create(*context, "auto").asNonAutoDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Relative, 10), StyledText::create(*context, "10%").asNonAutoDimension(*context)));

    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Absolute, 5), StyledText::create(*context, "5dp").asNonAutoRelativeDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Relative, 0), StyledText::create(*context, "auto").asNonAutoRelativeDimension(*context)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Relative, 10), StyledText::create(*context, "10%").asNonAutoRelativeDimension(*context)));

    ASSERT_TRUE(StyledText::create(*context, "").empty());
    ASSERT_FALSE(StyledText::create(*context, "<h2></h2>").empty());

    ASSERT_EQ(0, StyledText::create(*context, "").size());
    ASSERT_EQ(9, StyledText::create(*context, "<h2></h2>").size());
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

TEST_F(StyledTextTest, UnopenedTagNested)
{
    createAndVerifyStyledText(u8"<i>Unopened</i></i> tag.", u8"Unopened tag.", 1);
    verifySpan(0, StyledText::kSpanTypeItalic, 0, 8);
}

TEST_F(StyledTextTest, UnopenedTagDeepNested)
{
    createAndVerifyStyledText(u8"<i><i>Unopened</i></i></i></i></i></i> tag.", u8"Unopened tag.", 2);
    verifySpan(0, StyledText::kSpanTypeItalic, 0, 8);
    verifySpan(1, StyledText::kSpanTypeItalic, 0, 8);
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


TEST_F(StyledTextTest, UnclosedSameTypeTagNested)
{
    createAndVerifyStyledText(u8"Multiple nested <b><b><b><b>very</b></b> unclosed tags.",
    u8"Multiple nested very unclosed tags.", 4);
    verifySpan(0, StyledText::kSpanTypeStrong, 16, 35);
    verifySpan(1, StyledText::kSpanTypeStrong, 16, 35);
    verifySpan(2, StyledText::kSpanTypeStrong, 16, 20);
    verifySpan(3, StyledText::kSpanTypeStrong, 16, 20);
}

TEST_F(StyledTextTest, UnclosedSameTypeTagNestedComplex)
{
    createAndVerifyStyledText(u8"Multiple <b><b>very <u>unclosed<i> tags</b>. And few <tt>unclosed <strike>at the end.",
    u8"Multiple very unclosed tags. And few unclosed at the end.", 8);
    verifySpan(0, StyledText::kSpanTypeStrong, 9, 57);
    verifySpan(1, StyledText::kSpanTypeStrong, 9, 27);
    verifySpan(2, StyledText::kSpanTypeUnderline, 14, 27);
    verifySpan(3, StyledText::kSpanTypeItalic, 22, 27);
    verifySpan(4, StyledText::kSpanTypeItalic, 27, 57);
    verifySpan(5, StyledText::kSpanTypeUnderline, 27, 57);
    verifySpan(6, StyledText::kSpanTypeMonospace, 37, 57);
    verifySpan(7, StyledText::kSpanTypeStrike, 46, 57);
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

    // cat literally walks across the keyboard
    createAndVerifyStyledText(u8"hello<br 3459dfiuwcr9ergh da lia e  =ar -e 89q3 403i4 ''\"<<<<''' << k'asd \" />world", u8"helloworld", 1);
    createAndVerifyStyledText(u8"hello<span color='red' 3459dfiuwcr9ergh da lia e  =ar -e 89q3 403i4 ''\"<<<<''' << k'asd \" >world</span>", u8"helloworld", 1);
    verifySpan(0, StyledText::kSpanTypeSpan, 5, 10, 1);
    verifyColorAttribute(0, 0, "#ff0000ff");

    // span tag with attributes
    createAndVerifyStyledText(u8"Hello <span color='red'>this is an attr</span>", u8"Hello this is an attr", 1);
    verifySpan(0, StyledText::kSpanTypeSpan, 6, 21, 1);
    verifyColorAttribute(0, 0, "#ff0000ff");
    createAndVerifyStyledText(u8"Hello <span fontSize='48dp'>this is an attr</span>", u8"Hello this is an attr", 1);
    verifySpan(0, StyledText::kSpanTypeSpan, 6, 21, 1);
    verifyFontSizeAttribute(0, 0, "48dp");

    // span tag with attribute name with resource binding
    createAndVerifyStyledText(u8"Hello <span fontSize='@testFontSize'>this is an attr</span>", u8"Hello this is an attr", 1);
    verifySpan(0, StyledText::kSpanTypeSpan, 6, 21, 1);
    verifyFontSizeAttribute(0, 0, "10dp");

    // span tag with multiple attributes
    createAndVerifyStyledText(u8"Hello <span color='red' fontSize='48dp'>this is an attr</span>", u8"Hello this is an attr", 1);
    verifySpan(0, StyledText::kSpanTypeSpan, 6, 21, 2);
    verifyColorAttribute(0, 0, "#ff0000ff");
    verifyFontSizeAttribute(0, 1, "48dp");

    // span tag with different kinds of color attributes
    createAndVerifyStyledText(u8"Hello <span color='#edb'>this is an attr</span>", u8"Hello this is an attr", 1);
    verifySpan(0, StyledText::kSpanTypeSpan, 6, 21, 1);
    verifyColorAttribute(0, 0, "#eeddbbff");
    createAndVerifyStyledText(u8"Hello <span color='rgba(blue, 50%)'>this is an attr</span>", u8"Hello this is an attr", 1);
    verifySpan(0, StyledText::kSpanTypeSpan, 6, 21, 1);
    verifyColorAttribute(0, 0, "#0000ff7f");
    createAndVerifyStyledText(u8"Hello <span color='rgb(rgba(green, 50%), 50%)'>this is an attr</span>", u8"Hello this is an attr", 1);
    verifySpan(0, StyledText::kSpanTypeSpan, 6, 21, 1);
    verifyColorAttribute(0, 0, "#0080003f");
    createAndVerifyStyledText(u8"Hello <span color='hsl(0, 100%, 50%)'>this is an attr</span>", u8"Hello this is an attr", 1);
    verifySpan(0, StyledText::kSpanTypeSpan, 6, 21, 1);
    verifyColorAttribute(0, 0, "#ff0000ff");
    createAndVerifyStyledText(u8"Hello <span color='hsla(120, 0, 50%, 25%)'>this is an attr</span>", u8"Hello this is an attr", 1);
    verifySpan(0, StyledText::kSpanTypeSpan, 6, 21, 1);
    verifyColorAttribute(0, 0, "#80808040");

    // span tag with inherit attribute value
    createAndVerifyStyledText(u8"Hello <span color='inherit'>this is an attr</span>", u8"Hello this is an attr", 1);
    verifySpan(0, StyledText::kSpanTypeSpan, 6, 21);

    // span tag with same attributes
    createAndVerifyStyledText(u8"Hello <span color='blue' fontSize='50' color='red' fontSize='7'>this is an attr</span>", u8"Hello this is an attr", 1);
    verifySpan(0, StyledText::kSpanTypeSpan, 6, 21, 2);
    verifyColorAttribute(0, 0, "#0000ffff");
    verifyFontSizeAttribute(0, 1, "50dp");

    // span tag without attributes
    createAndVerifyStyledText(u8"Hello <span>this is an attr</span>", u8"Hello this is an attr", 1);
    verifySpan(0, StyledText::kSpanTypeSpan, 6, 21);

    // span tag with non-supported attributes
    createAndVerifyStyledText(u8"Hello <span foo='bar'>this is an attr</span>", u8"Hello this is an attr", 1);
    verifySpan(0, StyledText::kSpanTypeSpan, 6, 21);
}

TEST_F(StyledTextTest, NobrSimple)
{
    createAndVerifyStyledText(u8"He screamed \"Run <NOBR>faster</nobr>the<noBR>tiger is</NObr>right<nobr/><nobr />behind<nobr>you!!!</nobr>\"",
                              u8"He screamed \"Run fasterthetiger isrightbehindyou!!!\"", 3);
    verifySpan(0, StyledText::kSpanTypeNoBreak, 17, 23);
    verifySpan(1, StyledText::kSpanTypeNoBreak, 26, 34);
    verifySpan(2, StyledText::kSpanTypeNoBreak, 45, 51);
}

TEST_F(StyledTextTest, NobrMerge)
{
    // Only some tags can be merged. For example "<b>te</b><b>xt</b>" can become "<b>text</b>"
    createAndVerifyStyledText(u8"<nobr>This should not</nobr><nobr> merge</nobr> into one big tag",
                              u8"This should not merge into one big tag", 2);
    verifySpan(0, StyledText::kSpanTypeNoBreak, 0, 15);
    verifySpan(1, StyledText::kSpanTypeNoBreak, 15, 21);
}

TEST_F(StyledTextTest, NobrNested)
{
    createAndVerifyStyledText(u8"He screamed \"Run <NOBR><nobr><nobr>faster</nobr></nobr></nobr>the<noBR>tig<nobr>er </nobr>is</NObr>"
                              "right<nobr/><nobr />behind<nobr><nobr>you!</nobr>!!</nobr>\"",
                              u8"He screamed \"Run fasterthetiger isrightbehindyou!!!\"", 7);
    verifySpan(0, StyledText::kSpanTypeNoBreak, 17, 23);
    verifySpan(1, StyledText::kSpanTypeNoBreak, 17, 23);
    verifySpan(2, StyledText::kSpanTypeNoBreak, 17, 23);
    verifySpan(3, StyledText::kSpanTypeNoBreak, 26, 34);
    verifySpan(4, StyledText::kSpanTypeNoBreak, 29, 32);
    verifySpan(5, StyledText::kSpanTypeNoBreak, 45, 51);
    verifySpan(6, StyledText::kSpanTypeNoBreak, 45, 49);
}

TEST_F(StyledTextTest, NobrComplex)
{
    createAndVerifyStyledText(u8"He screamed \"Run <NOBR><nobr><br>fas<br>ter</nobr></nobr><b>the<noBR>tig<nobr>er </nobr>i</b>s</NObr>"
                              "right<nobr/><nobr />behind<nobr><nobr>you!</nobr>!!</nobr>\"",
                              u8"He screamed \"Run fasterthetiger isrightbehindyou!!!\"", 10);
    verifySpan(0, StyledText::kSpanTypeNoBreak, 17, 23);
    verifySpan(1, StyledText::kSpanTypeNoBreak, 17, 23);
    verifySpan(2, StyledText::kSpanTypeLineBreak, 17, 17);
    verifySpan(3, StyledText::kSpanTypeLineBreak, 20, 20);
    verifySpan(4, StyledText::kSpanTypeStrong, 23, 33);
    verifySpan(5, StyledText::kSpanTypeNoBreak, 26, 33);
    verifySpan(6, StyledText::kSpanTypeNoBreak, 29, 32);
    verifySpan(7, StyledText::kSpanTypeNoBreak, 33, 34);
    verifySpan(8, StyledText::kSpanTypeNoBreak, 45, 51);
    verifySpan(9, StyledText::kSpanTypeNoBreak, 45, 49);
}

TEST_F(StyledTextTest, StyledTextIteratorBasic)
{
    createAndVerifyStyledText(u8"He screamed \"<span color='red'>Run</span><u>faster<i>thetigerisbehind</i></u><i>you</i>!!!\"",
            u8"He screamed \"Runfasterthetigerisbehindyou!!!\"", 4);

    StyledText st = styledText.getStyledText();
    StyledText::Iterator it(st);

    EXPECT_EQ(it.next(), StyledText::Iterator::kString);
    EXPECT_EQ(it.getString(), "He screamed \"");

    EXPECT_EQ(it.next(), StyledText::Iterator::kStartSpan);
    EXPECT_EQ(it.getSpanType(), StyledText::kSpanTypeSpan);
    auto attribute = it.getSpanAttributes().at(0);
    EXPECT_EQ(attribute.name, 0);
    EXPECT_TRUE(attribute.value.isColor());
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

    StyledText st = styledText.getStyledText();
    StyledText::Iterator it(st);

    EXPECT_EQ(it.next(), StyledText::Iterator::kEnd);
}

TEST_F(StyledTextTest, StyledTextSpanEquality)
{
    auto st1 = StyledText::create(*context, u8"He screamed <b>\"Runfasterthetigerisbehindyou!!!\"</b>").getStyledText();
    auto st2 = StyledText::create(*context, u8"He screamed <b>\"Runslowerthepuppywantstolickyou\"</b>").getStyledText();
    auto st1Spans = st1.getSpans();
    auto st2Spans = st2.getSpans();

    EXPECT_EQ(st1Spans.size(), st2Spans.size());
    EXPECT_FALSE(st1Spans.empty());
    EXPECT_TRUE(st1Spans[0] == st2Spans[0]);
}

TEST_F(StyledTextTest, StyledTextSpanWithAttributesEquality)
{
    auto st1 = StyledText::create(*context, u8"He screamed <span color='red'>\"Runfasterthetigerisbehindyou!!!\"</span>").getStyledText();
    auto st2 = StyledText::create(*context, u8"He screamed <span color='red'>\"Runslowerthepuppywantstolickyou\"</span>").getStyledText();
    auto st1Spans = st1.getSpans();
    auto st2Spans = st2.getSpans();

    EXPECT_EQ(st1Spans.size(), st2Spans.size());
    EXPECT_FALSE(st1Spans.empty());
    EXPECT_TRUE(st1Spans[0] == st2Spans[0]);
}

TEST_F(StyledTextTest, StyledTextSpanInequality)
{
    auto st1 = StyledText::create(*context, u8"He screamed <b>\"Runfasterthetigerisbehindyou!!!\"</b>").getStyledText();
    auto st2 = StyledText::create(*context, u8"He screamed <b>\"Runslowertheturtleneedstolickyou\"</b>").getStyledText();
    auto st1Spans = st1.getSpans();
    auto st2Spans = st2.getSpans();

    EXPECT_EQ(st1Spans.size(), st2Spans.size());
    EXPECT_FALSE(st1Spans.empty());
    EXPECT_TRUE(st1Spans[0] != st2Spans[0]);
}

TEST_F(StyledTextTest, StyledTextSpanWithAttributesInequality)
{
    auto st1 = StyledText::create(*context, u8"He screamed <span color='red'>\"Runfasterthetigerisbehindyou!!!\"</span>").getStyledText();
    auto st2 = StyledText::create(*context, u8"He screamed <span color='blue'>\"Runslowerthepuppywantstolickyou\"</span>").getStyledText();
    auto st1Spans = st1.getSpans();
    auto st2Spans = st2.getSpans();

    EXPECT_EQ(st1Spans.size(), st2Spans.size());
    EXPECT_FALSE(st1Spans.empty());
    EXPECT_TRUE(st1Spans[0] != st2Spans[0]);
}

TEST_F(StyledTextTest, StyledTextTruthy)
{
    createAndVerifyStyledText(u8"He screamed \"Runfasterthetigerisbehindyou!!!\"",
                              u8"He screamed \"Runfasterthetigerisbehindyou!!!\"", 0);

    StyledText st = styledText.getStyledText();
    EXPECT_TRUE(st.truthy());

    createAndVerifyStyledText(u8"He screamed <b>\"Runfasterthetigerisbehindyou!!!\"</b>",
                              u8"He screamed \"Runfasterthetigerisbehindyou!!!\"", 1);

    st = styledText.getStyledText();
    EXPECT_TRUE(st.truthy());

    createAndVerifyStyledText(u8"",
                              u8"", 0);

    st = styledText.getStyledText();
    EXPECT_FALSE(st.truthy());
}

TEST_F(StyledTextTest, StyledTextIteratorNoTags)
{
    createAndVerifyStyledText(u8"He screamed \"Runfasterthetigerisbehindyou!!!\"",
                              u8"He screamed \"Runfasterthetigerisbehindyou!!!\"", 0);

    StyledText st = styledText.getStyledText();
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
    auto st = styledText.getStyledText();
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

    StyledText st = styledText.getStyledText();
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
