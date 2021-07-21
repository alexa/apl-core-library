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

#include <iostream>
#include <sstream>

#include "rapidjson/document.h"
#include "gtest/gtest.h"

#include "apl/engine/evaluate.h"
#include "apl/content/metrics.h"
#include "apl/component/component.h"
#include "apl/engine/builder.h"
#include "apl/primitives/transform.h"
#include "apl/utils/streamer.h"

#include "../testeventloop.h"

using namespace apl;

class BuilderTest : public DocumentWrapper {};

static const char *TEST_MULTIPLE_STATES =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"styles\": {"
    "    \"testStyle\": {"
    "      \"values\": ["
    "        {"
    "          \"when\": \"${state.pressed}\","
    "          \"color\": \"blue\","
    "          \"opacity\": 0.25"
    "        },"
    "        {"
    "          \"when\": \"${state.karaoke}\","
    "          \"color\": \"green\","
    "          \"opacity\": 0.5"
    "        },"
    "        {"
    "          \"when\": \"${state.karaokeTarget}\","
    "          \"color\": \"olive\","
    "          \"opacity\": 0.5"
    "        },"
    "        {"
    "          \"when\": \"${state.pressed && state.karaoke}\","
    "          \"color\": \"red\","
    "          \"opacity\": 0.75"
    "        }"
    "      ]"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"style\": \"testStyle\""
    "    }"
    "  }"
    "}";

TEST_F(BuilderTest, StatesOnOff)
{
    loadDocument(TEST_MULTIPLE_STATES);

    ASSERT_EQ(Object(1.0), component->getCalculated(kPropertyOpacity));
    ASSERT_TRUE(IsEqual(config->getDefaultFontColor("dark"), component->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual(config->getDefaultFontColor("dark"), component->getCalculated(kPropertyColorKaraokeTarget)));

    component->setState(kStatePressed, true);
    ASSERT_EQ(Object(0.25), component->getCalculated(kPropertyOpacity));
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), component->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), component->getCalculated(kPropertyColorKaraokeTarget)));

    component->setState(kStateKaraoke, true);
    ASSERT_EQ(Object(0.75), component->getCalculated(kPropertyOpacity));
    ASSERT_TRUE(IsEqual(Color(Color::RED), component->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual(Color(Color::RED), component->getCalculated(kPropertyColorKaraokeTarget)));

    component->setState(kStatePressed, false);
    ASSERT_EQ(Object(0.5), component->getCalculated(kPropertyOpacity));
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), component->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual(Color(Color::OLIVE), component->getCalculated(kPropertyColorKaraokeTarget)));

    component->setState(kStateKaraoke, false);
    ASSERT_EQ(Object(1.0), component->getCalculated(kPropertyOpacity));
    ASSERT_TRUE(IsEqual(config->getDefaultFontColor("dark"), component->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual(config->getDefaultFontColor("dark"), component->getCalculated(kPropertyColorKaraokeTarget)));

    clearDirty();
}

static const char *DATA = "{"
    "\"title\": \"Pecan Pie V\""
    "}";

static const char *SIMPLE_IMAGE = "{"
"  \"type\": \"APL\","
"  \"version\": \"1.0\","
"  \"theme\": \"dark\","
"  \"mainTemplate\": {"
"    \"parameters\": ["
"      \"payload\""
"    ],"
"    \"item\": {"
"      \"id\": \"abc\","
"      \"type\": \"Image\""
"    }"
"  }"
"}";


TEST_F(BuilderTest, SimpleImage)
{
    loadDocument(SIMPLE_IMAGE, DATA);
    auto map = component->getCalculated();

    ASSERT_EQ(kComponentTypeImage, component->getType());

    // ID tests
    ASSERT_EQ(Object("abc"), component->getId());
    ASSERT_EQ(component, context->findComponentById(component->getUniqueId()));
    ASSERT_EQ(component, context->findComponentById("abc"));
    ASSERT_FALSE(context->findComponentById("foo"));

    // Standard properties
    ASSERT_EQ(Object(""), component->getCalculated(kPropertyAccessibilityLabel));
    ASSERT_EQ(Object::EMPTY_ARRAY(), component->getCalculated(kPropertyAccessibilityActions));
    ASSERT_EQ(Object::FALSE_OBJECT(), component->getCalculated(kPropertyChecked));
    ASSERT_EQ(Object(""), component->getCalculated(kPropertyDescription));
    ASSERT_EQ(Object::FALSE_OBJECT(), component->getCalculated(kPropertyDisabled));
    ASSERT_EQ(kDisplayNormal, component->getCalculated(kPropertyDisplay).getInteger());
    ASSERT_EQ(Object(Dimension(100)), component->getCalculated(kPropertyHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxWidth));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyMinHeight));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyMinWidth));
    ASSERT_EQ(1.0, component->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingBottom));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingLeft));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingRight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingTop));
    ASSERT_EQ(Object(ObjectArray{}), component->getCalculated(kPropertyPadding));
    ASSERT_EQ(kRoleNone, component->getCalculated(kPropertyRole).getInteger());
    ASSERT_EQ(Object(Color(Color::TRANSPARENT)), component->getCalculated(kPropertyShadowColor));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyShadowHorizontalOffset));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyShadowRadius));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyShadowVerticalOffset));
    ASSERT_EQ(Object::IDENTITY_2D(), component->getCalculated(kPropertyTransform));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyTransformAssigned));
    ASSERT_EQ(Object(Dimension(100)), component->getCalculated(kPropertyWidth));
    ASSERT_EQ(Object::TRUE_OBJECT(), component->getCalculated(kPropertyLaidOut));

    // Image-specific properties
    ASSERT_EQ(kImageAlignCenter, component->getCalculated(kPropertyAlign).getInteger());
    ASSERT_EQ(kImageScaleBestFit, component->getCalculated(kPropertyScale).getInteger());
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyBorderRadius));
    ASSERT_EQ(0x00000000, component->getCalculated(kPropertyOverlayColor).getColor());
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyOverlayGradient));
    ASSERT_EQ("", component->getCalculated(kPropertySource).getString());
    ASSERT_EQ(0, component->getCalculated(kPropertyFilters).size());

    ASSERT_TRUE(CheckState(component));
}


static const char *FULL_IMAGE = "{"
"  \"type\": \"APL\","
"  \"version\": \"1.0\","
"  \"theme\": \"dark\","
"  \"mainTemplate\": {"
"    \"parameters\": ["
"      \"payload\""
"    ],"
"    \"item\": {"
"      \"type\": \"Image\","
"      \"accessibilityLabel\": \"Foo bar!\","
"      \"checked\": true,"
"      \"description\": \"My Image\","
"      \"disabled\": true,"
"      \"display\": \"invisible\","
"      \"height\": 200,"
"      \"width\": \"50vw\","
"      \"minHeight\": 10,"
"      \"minWidth\": 20,"
"      \"maxHeight\": \"100vh\","
"      \"maxWidth\": \"100vw\","
"      \"opacity\": 0.5,"
"      \"paddingBottom\": 1,"
"      \"paddingLeft\": 2,"
"      \"paddingRight\": \"3dp\","
"      \"paddingTop\": 4,"
"      \"role\": \"image\","
"      \"align\": \"bottom-right\","
"      \"scale\": \"fill\","
"      \"borderRadius\": \"10dp\","
"      \"overlayColor\": \"red\","
"      \"overlayGradient\": {"
"        \"colorRange\": ["
"          \"blue\","
"          \"red\""
"        ]"
"      },"
"      \"shadowColor\": \"green\","
"      \"shadowHorizontalOffset\": \"50vw\","
"      \"shadowRadius\": 5,"
"      \"shadowVerticalOffset\": \"20dp\","
"      \"source\": \"http://foo.com/bar.png\","
"      \"transform\": [{\"translateX\": 10}],"
"      \"filters\": {\"type\": \"Blur\", \"radius\": 22},"
"      \"random\": \"ERROR\""
"    }"
"  }"
"}";

TEST_F(BuilderTest, FullImage)
{
    loadDocument(FULL_IMAGE, DATA);

    auto map = component->getCalculated();

    // Standard properties
    ASSERT_EQ("Foo bar!", component->getCalculated(kPropertyAccessibilityLabel).getString());
    ASSERT_EQ(Object::EMPTY_ARRAY(), component->getCalculated(kPropertyAccessibilityActions));
    ASSERT_EQ(Object::TRUE_OBJECT(), component->getCalculated(kPropertyChecked));
    ASSERT_EQ(Object("My Image"), component->getCalculated(kPropertyDescription));
    ASSERT_EQ(Object::TRUE_OBJECT(), component->getCalculated(kPropertyDisabled));
    ASSERT_EQ(kDisplayInvisible, component->getCalculated(kPropertyDisplay).getInteger());
    ASSERT_EQ(Object(Dimension(200)), component->getCalculated(kPropertyHeight));
    ASSERT_EQ(Object(Dimension(800)), component->getCalculated(kPropertyMaxHeight));
    ASSERT_EQ(Object(Dimension(1024)), component->getCalculated(kPropertyMaxWidth));
    ASSERT_EQ(Object(Dimension(10)), component->getCalculated(kPropertyMinHeight));
    ASSERT_EQ(Object(Dimension(20)), component->getCalculated(kPropertyMinWidth));
    ASSERT_EQ(0.5, component->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_EQ(Object(Dimension(1)), component->getCalculated(kPropertyPaddingBottom));
    ASSERT_EQ(Object(Dimension(2)), component->getCalculated(kPropertyPaddingLeft));
    ASSERT_EQ(Object(Dimension(3)), component->getCalculated(kPropertyPaddingRight));
    ASSERT_EQ(Object(Dimension(4)), component->getCalculated(kPropertyPaddingTop));
    ASSERT_EQ(kRoleImage, component->getCalculated(kPropertyRole).getInteger());
    ASSERT_EQ(Object(Color(Color::GREEN)), component->getCalculated(kPropertyShadowColor));
    ASSERT_TRUE(IsEqual(Dimension(metrics.getWidth() / 2), component->getCalculated(kPropertyShadowHorizontalOffset)));
    ASSERT_EQ(Object(Dimension(5)), component->getCalculated(kPropertyShadowRadius));
    ASSERT_EQ(Object(Dimension(20)), component->getCalculated(kPropertyShadowVerticalOffset));
    ASSERT_EQ(Object(Dimension(512)), component->getCalculated(kPropertyWidth));
    ASSERT_EQ(Object::TRUE_OBJECT(), component->getCalculated(kPropertyLaidOut));

    // Transforms are tricky
    auto transform = component->getCalculated(kPropertyTransformAssigned);
    ASSERT_TRUE(transform.isTransform());
    ASSERT_EQ(Point(20,4), transform.getTransformation()->get(10,10) * Point(10,4));
    ASSERT_EQ(Object(Transform2D::translateX(10)), component->getCalculated(kPropertyTransform));

    // Image-specific properties
    ASSERT_EQ(kImageAlignBottomRight, component->getCalculated(kPropertyAlign).getInteger());
    ASSERT_EQ(kImageScaleFill, component->getCalculated(kPropertyScale).getInteger());
    ASSERT_EQ(Object(Dimension(10)), component->getCalculated(kPropertyBorderRadius));
    ASSERT_EQ(0xff0000ff, component->getCalculated(kPropertyOverlayColor).getColor());
    ASSERT_EQ("http://foo.com/bar.png", component->getCalculated(kPropertySource).getString());

    auto grad = component->getCalculated(kPropertyOverlayGradient);
    ASSERT_TRUE(grad.isGradient());
    ASSERT_EQ(Gradient::LINEAR, grad.getGradient().getType());
    ASSERT_EQ(Object(Color(0x0000ffff)), grad.getGradient().getColorRange().at(0));

    auto filters = component->getCalculated(kPropertyFilters);
    ASSERT_EQ(1, filters.size());
    ASSERT_EQ(kFilterTypeBlur, filters.at(0).getFilter().getType());
    ASSERT_EQ(Object(Dimension(22)), filters.at(0).getFilter().getValue(kFilterPropertyRadius));

    ASSERT_TRUE(CheckState(component, kStateChecked, kStateDisabled));
}

static const char *GRADIENT_IN_RESOURCE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"resources\": ["
    "    {"
    "      \"gradients\": {"
    "        \"myGrad\": {"
    "          \"colorRange\": ["
    "            \"blue\","
    "            \"green\","
    "            \"red\""
    "          ]"
    "        }"
    "      }"
    "    }"
    "  ],"
    "  \"mainTemplate\": {"
    "    \"parameters\": ["
    "      \"payload\""
    "    ],"
    "    \"item\": {"
    "      \"type\": \"Image\","
    "      \"overlayGradient\": \"@myGrad\","
    "      \"source\": \"http://foo.com/bar.png\""
    "    }"
    "  }"
    "}";

TEST_F(BuilderTest, GradientInResource)
{
    loadDocument(GRADIENT_IN_RESOURCE, DATA);
    auto grad = component->getCalculated(kPropertyOverlayGradient);
    ASSERT_TRUE(grad.isGradient());
    ASSERT_EQ(Gradient::LINEAR, grad.getGradient().getType());
    ASSERT_EQ(Object(Color(0x0000ffff)), grad.getGradient().getColorRange().at(0));
}

static const char *SIMPLE_TEXT = "{"
"  \"type\": \"APL\","
"  \"version\": \"1.0\","
"  \"theme\": \"dark\","
"  \"mainTemplate\": {"
"    \"parameters\": ["
"      \"payload\""
"    ],"
"    \"item\": {"
"      \"type\": \"Text\""
"    }"
"  }"
"}";


TEST_F(BuilderTest, SimpleText)
{
    loadDocument(SIMPLE_TEXT, DATA);

    auto map = component->getCalculated();
    ASSERT_EQ(kComponentTypeText, component->getType());

    // Standard properties
    ASSERT_EQ("", component->getCalculated(kPropertyAccessibilityLabel).getString());
    ASSERT_EQ(Object::EMPTY_ARRAY(), component->getCalculated(kPropertyAccessibilityActions));
    ASSERT_EQ(Object::FALSE_OBJECT(), component->getCalculated(kPropertyDisabled));
    ASSERT_EQ(Object(Dimension()), component->getCalculated(kPropertyHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxWidth));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyMinHeight));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyMinWidth));
    ASSERT_EQ(1.0, component->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingBottom));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingLeft));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingRight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingTop));
    ASSERT_EQ(Object(ObjectArray{}), component->getCalculated(kPropertyPadding));
    ASSERT_EQ(kRoleNone, component->getCalculated(kPropertyRole).getInteger());
    ASSERT_EQ(Object::IDENTITY_2D(), component->getCalculated(kPropertyTransform));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyTransformAssigned));
    ASSERT_EQ(Object(Dimension()), component->getCalculated(kPropertyWidth));
    ASSERT_EQ(Object::TRUE_OBJECT(), component->getCalculated(kPropertyLaidOut));

    // Text-specific properties
    ASSERT_EQ(0xfafafaff, component->getCalculated(kPropertyColor).getColor());
    ASSERT_EQ("sans-serif", component->getCalculated(kPropertyFontFamily).getString());
    ASSERT_EQ(Object(Dimension(40)), component->getCalculated(kPropertyFontSize));
    ASSERT_EQ(kFontStyleNormal, component->getCalculated(kPropertyFontStyle).getInteger());
    ASSERT_EQ(400, component->getCalculated(kPropertyFontWeight).getInteger());
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyLetterSpacing));
    ASSERT_EQ(1.25, component->getCalculated(kPropertyLineHeight).getDouble());
    ASSERT_EQ(0, component->getCalculated(kPropertyMaxLines).getInteger());
    ASSERT_EQ("", component->getCalculated(kPropertyText).asString());
    ASSERT_EQ(kTextAlignAuto, component->getCalculated(kPropertyTextAlign).getInteger());
    ASSERT_EQ(kTextAlignVerticalAuto, component->getCalculated(kPropertyTextAlignVertical).getInteger());
}

static const char *FULL_TEXT = "{"
"  \"type\": \"APL\","
"  \"version\": \"1.0\","
"  \"theme\": \"dark\","
"  \"mainTemplate\": {"
"    \"parameters\": ["
"      \"payload\""
"    ],"
"    \"item\": {"
"      \"type\": \"Text\","
"      \"accessibilityLabel\": \"Happy Text\","
"      \"height\": \"50vh\","
"      \"width\": \"50%\","
"      \"maxHeight\": \"100vh\","
"      \"maxWidth\": \"100vw\","
"      \"minHeight\": \"10%\","
"      \"minWidth\": \"25vw\","
"      \"opacity\": 0.5,"
"      \"paddingBottom\": 2,"
"      \"paddingLeft\": 4,"
"      \"paddingRight\": 6,"
"      \"paddingTop\": 10,"
"      \"role\": \"text\","
"      \"color\": \"blue\","
"      \"fontFamily\": \"Bookerly\","
"      \"fontSize\": \"20dp\","
"      \"fontStyle\": \"italic\","
"      \"fontWeight\": 800,"
"      \"letterSpacing\": \"2dp\","
"      \"lineHeight\": 1.5,"
"      \"maxLines\": 10,"
"      \"text\": \"Once more unto the breach, dear friends, once more;\","
"      \"textAlign\": \"right\","
"      \"transform\": [{\"translateY\": 10}],"
"      \"textAlignVertical\": \"bottom\""
"    }"
"  }"
"}";


TEST_F(BuilderTest, FullText)
{
    loadDocument(FULL_TEXT, DATA);

    auto map = component->getCalculated();

    // Standard properties
    ASSERT_EQ("Happy Text", component->getCalculated(kPropertyAccessibilityLabel).getString());
    ASSERT_EQ(Object::EMPTY_ARRAY(), component->getCalculated(kPropertyAccessibilityActions));
    ASSERT_EQ(Object::FALSE_OBJECT(), component->getCalculated(kPropertyDisabled));
    ASSERT_EQ(Object(Dimension(400)), component->getCalculated(kPropertyHeight));
    ASSERT_EQ(Object(Dimension(800)), component->getCalculated(kPropertyMaxHeight));
    ASSERT_EQ(Object(Dimension(1024)), component->getCalculated(kPropertyMaxWidth));
    ASSERT_EQ(Object(Dimension(DimensionType::Relative, 10)), component->getCalculated(kPropertyMinHeight));
    ASSERT_EQ(Object(Dimension(256)), component->getCalculated(kPropertyMinWidth));
    ASSERT_EQ(0.5, component->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_EQ(Object(Dimension(2)), component->getCalculated(kPropertyPaddingBottom));
    ASSERT_EQ(Object(Dimension(4)), component->getCalculated(kPropertyPaddingLeft));
    ASSERT_EQ(Object(Dimension(6)), component->getCalculated(kPropertyPaddingRight));
    ASSERT_EQ(Object(Dimension(10)), component->getCalculated(kPropertyPaddingTop));
    ASSERT_EQ(kRoleText, component->getCalculated(kPropertyRole).getInteger());
    ASSERT_EQ(Object(Dimension(DimensionType::Relative, 50)), component->getCalculated(kPropertyWidth));
    ASSERT_EQ(Object(Transform2D::translateY(10)), component->getCalculated(kPropertyTransform));
    ASSERT_EQ(Object::TRUE_OBJECT(), component->getCalculated(kPropertyLaidOut));

    // Text-specific properties
    ASSERT_EQ(0x0000ffff, component->getCalculated(kPropertyColor).getColor());
    ASSERT_EQ("Bookerly", component->getCalculated(kPropertyFontFamily).getString());
    ASSERT_EQ(Object(Dimension(20)), component->getCalculated(kPropertyFontSize));
    ASSERT_EQ(kFontStyleItalic, component->getCalculated(kPropertyFontStyle).getInteger());
    ASSERT_EQ(800, component->getCalculated(kPropertyFontWeight).getInteger());
    ASSERT_EQ(Object(Dimension(2)), component->getCalculated(kPropertyLetterSpacing));
    ASSERT_EQ(1.5, component->getCalculated(kPropertyLineHeight).getDouble());
    ASSERT_EQ(10, component->getCalculated(kPropertyMaxLines).getInteger());
    ASSERT_EQ("Once more unto the breach, dear friends, once more;", component->getCalculated(kPropertyText).asString());
    ASSERT_EQ(kTextAlignRight, component->getCalculated(kPropertyTextAlign).getInteger());
    ASSERT_EQ(kTextAlignVerticalBottom, component->getCalculated(kPropertyTextAlignVertical).getInteger());
}


static const char *SIMPLE_CONTAINER = "{"
"  \"type\": \"APL\","
"  \"version\": \"1.0\","
"  \"theme\": \"dark\","
"  \"mainTemplate\": {"
"    \"parameters\": ["
"      \"payload\""
"    ],"
"    \"item\": {"
"      \"type\": \"Container\","
"      \"item\": {"
"        \"type\": \"Text\""
"      }"
"    }"
"  }"
"}";

TEST_F(BuilderTest, SimpleContainer)
{
    loadDocument(SIMPLE_CONTAINER, DATA);

    auto map = component->getCalculated();
    ASSERT_EQ(kComponentTypeContainer, component->getType());

    // Standard properties
    ASSERT_EQ("", component->getCalculated(kPropertyAccessibilityLabel).getString());
    ASSERT_EQ(Object::EMPTY_ARRAY(), component->getCalculated(kPropertyAccessibilityActions));
    ASSERT_EQ(Object::FALSE_OBJECT(), component->getCalculated(kPropertyDisabled));
    ASSERT_EQ(Object(Dimension()), component->getCalculated(kPropertyHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxWidth));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyMinHeight));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyMinWidth));
    ASSERT_EQ(1.0, component->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingBottom));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingLeft));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingRight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingTop));
    ASSERT_EQ(Object(ObjectArray{}), component->getCalculated(kPropertyPadding));
    ASSERT_EQ(kRoleNone, component->getCalculated(kPropertyRole).getInteger());
    ASSERT_EQ(Object(Dimension()), component->getCalculated(kPropertyWidth));
    ASSERT_EQ(Object::TRUE_OBJECT(), component->getCalculated(kPropertyLaidOut));

    // Container-specific properties
    ASSERT_EQ(kFlexboxAlignStretch, component->getCalculated(kPropertyAlignItems).getInteger());
    ASSERT_EQ(kContainerDirectionColumn, component->getCalculated(kPropertyDirection).getInteger());
    ASSERT_EQ(kFlexboxJustifyContentStart, component->getCalculated(kPropertyJustifyContent).getInteger());
    ASSERT_FALSE(component->getCalculated(kPropertyNumbered).getBoolean());

    // Children
    ASSERT_EQ(1, component->getChildCount());
    auto text = component->getChildAt(0)->getCalculated();

    // The child has relative positioning
    ASSERT_EQ(kFlexboxAlignAuto, text["alignSelf"].getInteger());
    ASSERT_EQ(Object::AUTO_OBJECT(), text["bottom"]);
    ASSERT_EQ(0, text["grow"].getDouble());
    ASSERT_EQ(Object::AUTO_OBJECT(), text["left"]);
    ASSERT_EQ(kNumberingNormal, text["numbering"].getInteger());
    ASSERT_EQ(kPositionRelative, text["position"].getInteger());
    ASSERT_EQ(Object::AUTO_OBJECT(), text["right"]);
    ASSERT_EQ(0, text["shrink"].getDouble());
    ASSERT_EQ(Object(Dimension(0)), text["spacing"]);
    ASSERT_EQ(Object::AUTO_OBJECT(), text["top"]);
}

static const char *FULL_CONTAINER = "{"
"  \"type\": \"APL\","
"  \"version\": \"1.0\","
"  \"theme\": \"dark\","
"  \"mainTemplate\": {"
"    \"parameters\": ["
"      \"payload\""
"    ],"
"    \"item\": {"
"      \"type\": \"Container\","
"      \"accessibilityLabel\": \"Happy Text\","
"      \"height\": \"50vh\","
"      \"width\": \"50%\","
"      \"maxHeight\": \"100vh\","
"      \"maxWidth\": \"100vw\","
"      \"minHeight\": \"10%\","
"      \"minWidth\": \"25vw\","
"      \"opacity\": 0.5,"
"      \"paddingBottom\": 2,"
"      \"paddingLeft\": 4,"
"      \"paddingRight\": 6,"
"      \"paddingTop\": 10,"
"      \"role\": \"alert\","
"      \"alignItems\": \"end\","
"      \"justifyContent\": \"center\","
"      \"direction\": \"row\","
"      \"numbered\": true,"
"      \"firstItem\": {"
"        \"type\": \"Text\","
"        \"text\": \"First\""
"      },"
"      \"items\": ["
"        {"
"          \"type\": \"Text\","
"          \"text\": \"Turtle\","
"          \"position\": \"absolute\","
"          \"top\": 10,"
"          \"bottom\": 10,"
"          \"left\": 20,"
"          \"right\": 30"
"        },"
"        {"
"          \"type\": \"Image\","
"          \"source\": \"my_little_picture\","
"          \"grow\": 1,"
"          \"shrink\": 2,"
"          \"left\": 10,"
"          \"spacing\": 20,"
"          \"numbering\": \"skip\","
"          \"alignSelf\": \"baseline\""
"        }"
"      ],"
"      \"lastItem\": {"
"        \"type\": \"Text\","
"        \"text\": \"Last\""
"      }"
"    }"
"  }"
"}";

TEST_F(BuilderTest, FullContainer)
{
    loadDocument(FULL_CONTAINER, DATA);

    auto map = component->getCalculated();

    // Standard properties
    ASSERT_EQ("Happy Text", component->getCalculated(kPropertyAccessibilityLabel).getString());
    ASSERT_EQ(Object::EMPTY_ARRAY(), component->getCalculated(kPropertyAccessibilityActions));
    ASSERT_EQ(Object::FALSE_OBJECT(), component->getCalculated(kPropertyDisabled));
    ASSERT_EQ(Object(Dimension(400)), component->getCalculated(kPropertyHeight));
    ASSERT_EQ(Object(Dimension(800)), component->getCalculated(kPropertyMaxHeight));
    ASSERT_EQ(Object(Dimension(1024)), component->getCalculated(kPropertyMaxWidth));
    ASSERT_EQ(Object(Dimension(DimensionType::Relative, 10)), component->getCalculated(kPropertyMinHeight));
    ASSERT_EQ(Object(Dimension(256)), component->getCalculated(kPropertyMinWidth));
    ASSERT_EQ(0.5, component->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_EQ(Object(Dimension(2)), component->getCalculated(kPropertyPaddingBottom));
    ASSERT_EQ(Object(Dimension(4)), component->getCalculated(kPropertyPaddingLeft));
    ASSERT_EQ(Object(Dimension(6)), component->getCalculated(kPropertyPaddingRight));
    ASSERT_EQ(Object(Dimension(10)), component->getCalculated(kPropertyPaddingTop));
    ASSERT_EQ(kRoleAlert, component->getCalculated(kPropertyRole).getInteger());
    ASSERT_EQ(Object(Dimension(DimensionType::Relative, 50)), component->getCalculated(kPropertyWidth));
    ASSERT_EQ(Object::TRUE_OBJECT(), component->getCalculated(kPropertyLaidOut));

    // Container-specific properties
    ASSERT_EQ(kFlexboxAlignEnd, component->getCalculated(kPropertyAlignItems).getInteger());
    ASSERT_EQ(kContainerDirectionRow, component->getCalculated(kPropertyDirection).getInteger());
    ASSERT_EQ(kFlexboxJustifyContentCenter, component->getCalculated(kPropertyJustifyContent).getInteger());
    ASSERT_TRUE(component->getCalculated(kPropertyNumbered).getBoolean());

    // Children
    ASSERT_EQ(4, component->getChildCount());

    // First item
    ASSERT_EQ("First", component->getChildAt(0)->getCalculated(kPropertyText).asString());

    // Second item (Absolute positioning)
    auto child = component->getChildAt(1)->getCalculated();;
    ASSERT_EQ(kFlexboxAlignAuto, child["alignSelf"].getInteger());
    ASSERT_EQ(Object(Dimension(10)), child["bottom"]);
    ASSERT_EQ(0, child["grow"].getInteger());
    ASSERT_EQ(Object(Dimension(20)), child["left"]);
    ASSERT_EQ(kNumberingNormal, child["numbering"].getInteger());
    ASSERT_EQ(kPositionAbsolute, child["position"].getInteger());
    ASSERT_EQ(Object(Dimension(30)), child["right"]);
    ASSERT_EQ(0, child["shrink"].getInteger());
    ASSERT_EQ(Object(Dimension(0)), child["spacing"]);
    ASSERT_EQ(Object(Dimension(10)), child["top"]);

    // Third item (Relative positioning)
    child = component->getChildAt(2)->getCalculated();
    ASSERT_EQ(kFlexboxAlignBaseline, child["alignSelf"].getInteger());
    ASSERT_EQ(Object::AUTO_OBJECT(), child["bottom"]);
    ASSERT_EQ(1, child["grow"].getDouble());
    ASSERT_EQ(Object(Dimension(10)), child["left"]);
    ASSERT_EQ(kNumberingSkip, child["numbering"].getInteger());
    ASSERT_EQ(kPositionRelative, child["position"].getInteger());
    ASSERT_EQ(Object::AUTO_OBJECT(), child["right"]);
    ASSERT_EQ(2, child["shrink"].getDouble());
    ASSERT_EQ(Object(Dimension(20)), child["spacing"]);
    ASSERT_EQ(Object::AUTO_OBJECT(), child["top"]);

    // Fourth item
    ASSERT_EQ("Last", component->getChildAt(3)->getCalculated(kPropertyText).asString());
}

static const char *START_END_OFFSETS_BASIC = R"apl(
{
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "width": "500",
      "height": "500",
      "items": {
        "layoutDirection": "RTL",
        "id": "textComp",
        "type": "Frame",
        "width": "100",
        "height": "100",
        "start": 100,
        "position": "relative",
        "backgroundColor": "blue"
      }
    }
  }
}
)apl";

TEST_F(BuilderTest, BasicStartOffset)
{
    loadDocument(START_END_OFFSETS_BASIC);

    auto text = component->getCoreChildAt(0);
    text->setProperty(kPropertyStart, 100);
    root->clearPending(); // Force layout

    auto childBounds = text->getCalculated(kPropertyBounds);
    ASSERT_TRUE(expectBounds(text, 0, 100, 100, 200));
}

TEST_F(BuilderTest, BasicEndOffset)
{
    loadDocument(START_END_OFFSETS_BASIC);

    auto text = component->getCoreChildAt(0);
    text->setProperty(kPropertyEnd, 100);
    root->clearPending(); // Force layout

    auto childBounds = text->getCalculated(kPropertyBounds);
    ASSERT_TRUE(expectBounds(text, 0, 100, 100, 200));
}

TEST_F(BuilderTest, BasicStartOverrideOffset)
{
    loadDocument(START_END_OFFSETS_BASIC);

    auto text = component->getCoreChildAt(0);
    text->setProperty(kPropertyLeft, 200);
    text->setProperty(kPropertyStart, 100);
    root->clearPending(); // Force layout

    auto childBounds = text->getCalculated(kPropertyBounds);
    ASSERT_TRUE(expectBounds(text, 0, 100, 100, 200));
}

TEST_F(BuilderTest, BasicEndOverrideOffset)
{
    loadDocument(START_END_OFFSETS_BASIC);

    auto text = component->getCoreChildAt(0);
    text->setProperty(kPropertyRight, 200);
    text->setProperty(kPropertyEnd, 100);
    root->clearPending(); // Force layout

    auto childBounds = text->getCalculated(kPropertyBounds);
    ASSERT_TRUE(expectBounds(text, 0, 100, 100, 200));
}

TEST_F(BuilderTest, CombinedStartEndOffset)
{
    loadDocument(START_END_OFFSETS_BASIC);

    auto text = component->getCoreChildAt(0);
    text->setProperty(kPropertyStart, 200);
    text->setProperty(kPropertyEnd, 100);
    root->clearPending(); // Force layout

    auto childBounds = text->getCalculated(kPropertyBounds);
    ASSERT_TRUE(expectBounds(text, 0, 200, 100, 300));
}

TEST_F(BuilderTest, CombinedStartEndWithOverrideOffset)
{
    loadDocument(START_END_OFFSETS_BASIC);

    auto text = component->getCoreChildAt(0);
    text->setProperty(kPropertyLeft, 400);
    text->setProperty(kPropertyRight, 300);
    text->setProperty(kPropertyStart, 200);
    text->setProperty(kPropertyEnd, 100);
    root->clearPending(); // Force layout

    auto childBounds = text->getCalculated(kPropertyBounds);
    ASSERT_TRUE(expectBounds(text, 0, 200, 100, 300));
}

TEST_F(BuilderTest, BasicStartOffsetRTL)
{
    loadDocument(START_END_OFFSETS_BASIC);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");

    auto text = component->getCoreChildAt(0);
    text->setProperty(kPropertyStart, 100);
    root->clearPending(); // Force layout

    auto childBounds = text->getCalculated(kPropertyBounds);
    ASSERT_TRUE(expectBounds(text, 0, 300, 100, 400));
}

TEST_F(BuilderTest, BasicEndOffsetRTL)
{
    loadDocument(START_END_OFFSETS_BASIC);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");

    auto text = component->getCoreChildAt(0);
    text->setProperty(kPropertyEnd, 100);
    root->clearPending(); // Force layout

    auto childBounds = text->getCalculated(kPropertyBounds);
    ASSERT_TRUE(expectBounds(text, 0, 300, 100, 400));
}

TEST_F(BuilderTest, BasicStartOverrideOffsetRTL)
{
    loadDocument(START_END_OFFSETS_BASIC);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending(); // Force layout

    auto text = component->getCoreChildAt(0);
    text->setProperty(kPropertyRight, 200);
    text->setProperty(kPropertyStart, 100);
    root->clearPending(); // Force layout

    auto childBounds = text->getCalculated(kPropertyBounds);
    ASSERT_TRUE(expectBounds(text, 0, 300, 100, 400));
}

TEST_F(BuilderTest, BasicEndOverrideOffsetRTL)
{
    loadDocument(START_END_OFFSETS_BASIC);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");

    auto text = component->getCoreChildAt(0);
    text->setProperty(kPropertyLeft, 200);
    text->setProperty(kPropertyEnd, 100);
    root->clearPending(); // Force layout

    auto childBounds = text->getCalculated(kPropertyBounds);
    ASSERT_TRUE(expectBounds(text, 0, 300, 100, 400));
}

TEST_F(BuilderTest, CombinedStartEndOffsetRTL)
{
    loadDocument(START_END_OFFSETS_BASIC);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");

    auto text = component->getCoreChildAt(0);
    text->setProperty(kPropertyStart, 200);
    text->setProperty(kPropertyEnd, 100);
    root->clearPending(); // Force layout

    auto childBounds = text->getCalculated(kPropertyBounds);
    ASSERT_TRUE(expectBounds(text, 0, 200, 100, 300));
}

TEST_F(BuilderTest, CombinedStartEndWithOverrideOffsetRTL)
{
    loadDocument(START_END_OFFSETS_BASIC);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");

    auto text = component->getCoreChildAt(0);
    text->setProperty(kPropertyLeft, 400);
    text->setProperty(kPropertyRight, 300);
    text->setProperty(kPropertyStart, 200);
    text->setProperty(kPropertyEnd, 100);
    root->clearPending(); // Force layout

    auto childBounds = text->getCalculated(kPropertyBounds);
    ASSERT_TRUE(expectBounds(text, 0, 200, 100, 300));
}

TEST_F(BuilderTest, CombinedStartEndWithOverrideOffsetWithDirectionChangeRTL)
{
    loadDocument(START_END_OFFSETS_BASIC);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");

    auto text = component->getCoreChildAt(0);
    text->setProperty(kPropertyLeft, 400);
    text->setProperty(kPropertyRight, 300);
    text->setProperty(kPropertyStart, 200);
    text->setProperty(kPropertyEnd, 100);
    root->clearPending(); // Force layout

    auto childBounds = text->getCalculated(kPropertyBounds);
    ASSERT_TRUE(expectBounds(text, 0, 200, 100, 300));

    component->setProperty(kPropertyLayoutDirectionAssigned, "LTR");
    root->clearPending(); // Force layout

    childBounds = text->getCalculated(kPropertyBounds);
    ASSERT_TRUE(expectBounds(text, 0, 200, 100, 300));
}

static const char *RELATIVE_POSITION =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"Container\","
        "      \"width\": \"100%\","
        "      \"height\": \"100%\","
        "      \"items\": {"
        "        \"type\": \"Text\","
        "        \"left\": \"25%\","
        "        \"top\": \"25%\","
        "        \"bottom\": \"25%\","
        "        \"right\": \"25%\","
        "        \"position\": \"absolute\""
        "      }"
        "    }"
        "  }"
        "}";

TEST_F(BuilderTest, RelativePosition)
{
    loadDocument(RELATIVE_POSITION);

    ASSERT_TRUE(component);
    auto bounds = component->getCalculated(kPropertyBounds);
    auto width = metrics.getWidth();
    auto height = metrics.getHeight();
    ASSERT_TRUE(IsEqual(bounds, Rect(0, 0, width, height)));

    ASSERT_EQ(1, component->getChildCount());
    auto text = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Relative, 25), text->getCalculated(kPropertyLeft)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Relative, 25), text->getCalculated(kPropertyTop)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Relative, 25), text->getCalculated(kPropertyRight)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Relative, 25), text->getCalculated(kPropertyBottom)));

    auto childBounds = text->getCalculated(kPropertyBounds);
    ASSERT_TRUE(IsEqual(childBounds, Rect(width / 4, height / 4, width / 2, height / 2)));
}

static const char *RELATIVE_POSITION_2 =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"Container\","
        "      \"width\": \"100%\","
        "      \"height\": \"100%\","
        "      \"items\": {"
        "        \"type\": \"Text\","
        "        \"left\": \"25%\","
        "        \"top\": \"25%\","
        "        \"width\": \"25%\","
        "        \"height\": \"25%\","
        "        \"position\": \"absolute\""
        "      }"
        "    }"
        "  }"
        "}";

TEST_F(BuilderTest, RelativePosition2)
{
    loadDocument(RELATIVE_POSITION_2);

    ASSERT_TRUE(component);
    auto bounds = component->getCalculated(kPropertyBounds);
    auto width = metrics.getWidth();
    auto height = metrics.getHeight();
    ASSERT_TRUE(IsEqual(bounds, Rect(0, 0, width, height)));

    ASSERT_EQ(1, component->getChildCount());
    auto text = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Relative, 25), text->getCalculated(kPropertyLeft)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Relative, 25), text->getCalculated(kPropertyTop)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Relative, 25), text->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Dimension(DimensionType::Relative, 25), text->getCalculated(kPropertyHeight)));

    auto childBounds = text->getCalculated(kPropertyBounds);
    ASSERT_TRUE(IsEqual(childBounds, Rect(width / 4, height / 4, width / 4, height / 4)));
}


const char *DATA_CONTAINER ="{"
"  \"type\": \"APL\","
"  \"version\": \"1.0\","
"  \"mainTemplate\": {"
"    \"parameters\": ["
"      \"payload\""
"    ],"
"    \"item\": {"
"      \"type\": \"Container\","
"      \"data\": ["
"        \"a\","
"        \"b\","
"        \"c\","
"        \"d\","
"        \"e\""
"      ],"
"      \"items\": ["
"        {"
"          \"type\": \"Text\","
"          \"text\": \"Item ${data} index=${index}\""
"        }"
"      ]"
"    }"
"  }"
"}";


TEST_F(BuilderTest, DataContainer)
{
    loadDocument(DATA_CONTAINER, DATA);

    auto map = component->getCalculated();

    // Children
    ASSERT_EQ(5, component->getChildCount());

    // First item
    for (int i = 0 ; i < 5 ; i++ ) {
        auto child = component->getChildAt(i);
        std::stringstream ss;
        ss << "Item " << (char) ('a'+i) << " index=" << i;
        ASSERT_EQ(ss.str(), child->getCalculated(kPropertyText).asString());
    }
}

const char *DATA_CONTAINER_2 = "{"
"  \"type\": \"APL\","
"  \"version\": \"1.0\","
"  \"mainTemplate\": {"
"    \"parameters\": ["
"      \"payload\""
"    ],"
"    \"item\": {"
"      \"type\": \"Container\","
"      \"data\": \"${payload.elements}\","
"      \"items\": ["
"        {"
"          \"type\": \"Text\","
"          \"text\": \"Item ${data} index=${index}\""
"        }"
"      ]"
"    }"
"  }"
"}";

const char *DATA_CONTAINER_2_DATA = "{"
"  \"elements\": ["
"    \"A\","
"    \"B\","
"    \"C\","
"    \"D\","
"    \"E\","
"    \"F\""
"  ]"
"}";

TEST_F(BuilderTest, DataContainer2)
{
    loadDocument(DATA_CONTAINER_2, DATA_CONTAINER_2_DATA);

    auto map = component->getCalculated();

    // Children
    ASSERT_EQ(6, component->getChildCount());

    // First item
    for (int i = 0 ; i < 6 ; i++ ) {
        auto child = component->getChildAt(i);
        std::stringstream ss;
        ss << "Item " << (char) ('A'+i) << " index=" << i;
        ASSERT_EQ(ss.str(), child->getCalculated(kPropertyText).asString());
    }
}

static const char *DATA_CONTAINER_DEEP_EVALUATION =
    "{"
    "  \"elements\": ["
    "    \"${viewport.width}\","
    "    \"${viewport.height}\""
    "  ]"
    "}";

TEST_F(BuilderTest, DataContainerDeepEvaluation)
{
    loadDocument(DATA_CONTAINER_2, DATA_CONTAINER_DEEP_EVALUATION);
    ASSERT_EQ(2, component->getChildCount());

    auto width = std::to_string(static_cast<int>(metrics.getWidth()));
    auto height = std::to_string(static_cast<int>(metrics.getHeight()));

    auto child = component->getChildAt(0);
    ASSERT_EQ(std::string("Item "+width+" index=0"), child->getCalculated(kPropertyText).asString());

    child = component->getChildAt(1);
    ASSERT_EQ(std::string("Item "+height+" index=1"), child->getCalculated(kPropertyText).asString());
}


const char *SIMPLE_SCROLL_VIEW = "{"
"  \"type\": \"APL\","
"  \"version\": \"1.0\","
"  \"mainTemplate\": {"
"    \"parameters\": ["
"      \"payload\""
"    ],"
"    \"item\": {"
"      \"type\": \"ScrollView\","
"      \"items\": ["
"        {"
"          \"type\": \"Text\""
"        },"
"        {"
"          \"type\": \"Text\""
"        }"
"      ]"
"    }"
"  }"
"}";


TEST_F(BuilderTest, SimpleScrollView)
{
    loadDocument(SIMPLE_SCROLL_VIEW, DATA);

    auto map = component->getCalculated();
    ASSERT_EQ(kComponentTypeScrollView, component->getType());

    // Standard properties
    ASSERT_EQ("", component->getCalculated(kPropertyAccessibilityLabel).getString());
    ASSERT_EQ(Object::EMPTY_ARRAY(), component->getCalculated(kPropertyAccessibilityActions));
    ASSERT_EQ(Object::FALSE_OBJECT(), component->getCalculated(kPropertyDisabled));
    ASSERT_EQ(Object(Dimension(100)), component->getCalculated(kPropertyHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxWidth));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyMinHeight));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyMinWidth));
    ASSERT_EQ(1.0, component->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingBottom));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingLeft));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingRight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingTop));
    ASSERT_EQ(Object(ObjectArray{}), component->getCalculated(kPropertyPadding));
    ASSERT_EQ(kRoleNone, component->getCalculated(kPropertyRole).getInteger());
    ASSERT_EQ(Object(Dimension()), component->getCalculated(kPropertyWidth));
    ASSERT_EQ(Object::TRUE_OBJECT(), component->getCalculated(kPropertyLaidOut));

    // Children
    ASSERT_EQ(1, component->getChildCount());
}

const char *SIMPLE_TOUCH_WRAPPER = "{"
"  \"type\": \"APL\","
"  \"version\": \"1.0\","
"  \"mainTemplate\": {"
"    \"parameters\": ["
"      \"payload\""
"    ],"
"    \"item\": {"
"      \"type\": \"TouchWrapper\","
"      \"items\": ["
"        {"
"          \"type\": \"Text\""
"        },"
"        {"
"          \"type\": \"Text\""
"        }"
"      ],"
"      \"onPress\": ["
"       {"
"          \"type\": \"PlayMedia\","
"          \"componentId\": \"myVideoPlayer\","
"          \"source\": \"URL\","
"          \"audioTrack\": \"background\""
"       },"
"       {"
"          \"type\": \"SendEvent\","
"          \"description\": \"This will execute immediately\","
"          \"arguments\": [\"Media has started, but hasn't stopped yet\"]"
"       }"
"      ]"
"    }"
"  }"
"}";

TEST_F(BuilderTest, SimpleTouchWrapper)
{
    loadDocument(SIMPLE_TOUCH_WRAPPER, DATA);

    auto map = component->getCalculated();
    ASSERT_EQ(kComponentTypeTouchWrapper, component->getType());

    // Standard properties
    ASSERT_EQ("", component->getCalculated(kPropertyAccessibilityLabel).getString());
    ASSERT_EQ(Object::EMPTY_ARRAY(), component->getCalculated(kPropertyAccessibilityActions));
    ASSERT_EQ(Object::FALSE_OBJECT(), component->getCalculated(kPropertyDisabled));
    ASSERT_EQ(Object(Dimension()), component->getCalculated(kPropertyHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxWidth));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyMinHeight));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyMinWidth));
    ASSERT_EQ(1.0, component->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingBottom));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingLeft));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingRight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingTop));
    ASSERT_EQ(Object(ObjectArray{}), component->getCalculated(kPropertyPadding));
    ASSERT_EQ(kRoleNone, component->getCalculated(kPropertyRole).getInteger());
    ASSERT_EQ(Object(Dimension()), component->getCalculated(kPropertyWidth));
    ASSERT_EQ(Object::TRUE_OBJECT(), component->getCalculated(kPropertyLaidOut));

    // TouchWrapper properties
    auto commands = component->getCalculated(kPropertyOnPress);
    ASSERT_TRUE(commands.isArray());
    ASSERT_EQ(2, commands.size());

    // Children
    ASSERT_EQ(1, component->getChildCount());
}


const char *NUMBER_TEST = "{"
                          "  \"type\": \"APL\","
                          "  \"version\": \"1.0\","
                          "  \"mainTemplate\": {"
                          "    \"parameters\": ["
                          "      \"payload\""
                          "    ],"
                          "    \"item\": {"
                          "      \"type\": \"Container\","
                          "      \"numbered\": true,"
                          "      \"firstItem\": {"
                          "        \"type\": \"Text\","
                          "        \"text\": \"First\""
                          "      },"
                          "      \"lastItem\": {"
                          "        \"type\": \"Text\","
                          "        \"text\": \"Last\""
                          "      },"
                          "      \"items\": ["
                          "        {"
                          "          \"type\": \"Text\","
                          "          \"text\": \"A ${index}-${ordinal}-${length}\","
                          "          \"spacing\": \"${index + 10}\""
                          "        },"
                          "        {"
                          "          \"type\": \"Text\","
                          "          \"text\": \"B ${index}-${ordinal}-${length}\","
                          "          \"numbering\": \"skip\""
                          "        },"
                          "        {"
                          "          \"type\": \"Text\","
                          "          \"text\": \"C ${index}-${ordinal}-${length}\""
                          "        },"
                          "        {"
                          "          \"when\": \"${index == 10}\","
                          "          \"type\": \"Text\","
                          "          \"text\": \"D ${index}-${ordinal}-${length}\""
                          "        },"
                          "        {"
                          "          \"type\": \"Text\","
                          "          \"text\": \"E ${index}-${ordinal}-${length}\""
                          "        },"
                          "        {"
                          "          \"type\": \"Text\","
                          "          \"text\": \"F ${index}-${ordinal}-${length}\","
                          "          \"numbering\": \"reset\""
                          "        },"
                          "        {"
                          "          \"type\": \"Text\","
                          "          \"text\": \"G ${index}-${ordinal}-${length}\""
                          "        }"
                          "      ]"
                          "    }"
                          "  }"
                          "}";

TEST_F(BuilderTest, NumberingItems)
{
    loadDocument(NUMBER_TEST, DATA);

    auto map = component->getCalculated();

    ASSERT_EQ(8, component->getChildCount());
    ASSERT_EQ(Object(Dimension(10)), component->getChildAt(1)->getCalculated(kPropertySpacing));

    ASSERT_EQ(Object("First"), component->getChildAt(0)->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object("A 0-1-7"), component->getChildAt(1)->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object("B 1-2-7"), component->getChildAt(2)->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object("C 2-2-7"), component->getChildAt(3)->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object("E 3-3-7"), component->getChildAt(4)->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object("F 4-4-7"), component->getChildAt(5)->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object("G 5-1-7"), component->getChildAt(6)->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object("Last"), component->getChildAt(7)->getCalculated(kPropertyText).asString());
}

const char *NUMBER_TEST_2 = "{"
                            "  \"type\": \"APL\","
                            "  \"version\": \"1.0\","
                            "  \"mainTemplate\": {"
                            "    \"parameters\": ["
                            "      \"payload\""
                            "    ],"
                            "    \"item\": {"
                            "      \"type\": \"Container\","
                            "      \"numbered\": true,"
                            "      \"data\": ["
                            "        \"One\","
                            "        \"Two\","
                            "        \"Three\","
                            "        \"Four\","
                            "        \"Five\""
                            "      ],"
                            "      \"items\": ["
                            "        {"
                            "          \"when\": \"${data == 'Two'}\","
                            "          \"type\": \"Text\","
                            "          \"text\": \"A ${index}-${ordinal}-${length}\","
                            "          \"numbering\": \"reset\""
                            "        },"
                            "        {"
                            "          \"when\": \"${data == 'Four'}\","
                            "          \"type\": \"Text\","
                            "          \"text\": \"B ${index}-${ordinal}-${length}\","
                            "          \"numbering\": \"skip\""
                            "        },"
                            "        {"
                            "          \"type\": \"Text\","
                            "          \"text\": \"C ${index}-${ordinal}-${length}\""
                            "        }"
                            "      ]"
                            "    }"
                            "  }"
                            "}";

TEST_F(BuilderTest, NumberingDataItems)
{
    loadDocument(NUMBER_TEST_2, DATA);
    auto map = component->getCalculated();

    ASSERT_EQ(5, component->getChildCount());
    ASSERT_EQ(Object("C 0-1-5"), component->getChildAt(0)->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object("A 1-2-5"), component->getChildAt(1)->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object("C 2-1-5"), component->getChildAt(2)->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object("B 3-2-5"), component->getChildAt(3)->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object("C 4-2-5"), component->getChildAt(4)->getCalculated(kPropertyText).asString());
}

static const char *SIMPLE_VIDEO = "{"
                                  "  \"type\": \"APL\","
                                  "  \"version\": \"1.0\","
                                  "  \"theme\": \"dark\","
                                  "  \"mainTemplate\": {"
                                  "    \"parameters\": ["
                                  "      \"payload\""
                                  "    ],"
                                  "    \"item\": {"
                                  "      \"id\": \"abc\","
                                  "      \"type\": \"Video\""
                                  "    }"
                                  "  }"
                                  "}";

TEST_F(BuilderTest, SimpleVideo)
{
    loadDocument(SIMPLE_VIDEO, DATA);
    auto map = component->getCalculated();

    ASSERT_EQ(kComponentTypeVideo, component->getType());

    // ID tests
    ASSERT_EQ(Object("abc"), component->getId());
    ASSERT_EQ(component, context->findComponentById(component->getUniqueId()));
    ASSERT_EQ(component, context->findComponentById("abc"));
    ASSERT_FALSE(context->findComponentById("foo"));

    // Standard properties
    ASSERT_EQ("", component->getCalculated(kPropertyAccessibilityLabel).getString());
    ASSERT_EQ(Object::EMPTY_ARRAY(), component->getCalculated(kPropertyAccessibilityActions));
    ASSERT_EQ(Object::FALSE_OBJECT(), component->getCalculated(kPropertyDisabled));
    ASSERT_EQ(Object(Dimension(100)), component->getCalculated(kPropertyHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxWidth));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyMinHeight));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyMinWidth));
    ASSERT_EQ(1.0, component->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingBottom));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingLeft));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingRight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingTop));
    ASSERT_EQ(Object(ObjectArray{}), component->getCalculated(kPropertyPadding));
    ASSERT_EQ(kRoleNone, component->getCalculated(kPropertyRole).getInteger());
    ASSERT_EQ(Object(Dimension(100)), component->getCalculated(kPropertyWidth));
    ASSERT_EQ(Object::TRUE_OBJECT(), component->getCalculated(kPropertyLaidOut));

    // Video-specific properties
    ASSERT_EQ(kVideoScaleBestFit, component->getCalculated(kPropertyScale).getInteger());
    ASSERT_EQ(Object::EMPTY_ARRAY(), component->getCalculated(kPropertySource));
    ASSERT_EQ(kAudioTrackForeground, component->getCalculated(kPropertyAudioTrack).getInteger());
    ASSERT_EQ(Object::EMPTY_ARRAY(), component->getCalculated(kPropertyOnEnd));
    ASSERT_EQ(Object::EMPTY_ARRAY(), component->getCalculated(kPropertyOnPause));
    ASSERT_EQ(Object::EMPTY_ARRAY(), component->getCalculated(kPropertyOnPlay));
    ASSERT_EQ(Object::EMPTY_ARRAY(), component->getCalculated(kPropertyOnTrackUpdate));
    ASSERT_EQ(false, component->getCalculated(kPropertyAutoplay).getBoolean());
}

static const char *OLD_AUTO_PLAY_VIDEO =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Video\","
    "      \"autoplay\": \"false\""
    "    }"
    "  }"
    "}";

/*
 * For backward compatibility with 1.0, the "autoplay" property treats the string "false" as
 * evaluating to false.
 */
TEST_F(BuilderTest, OldAutoPlayVideo)
{
    loadDocument(OLD_AUTO_PLAY_VIDEO);
    ASSERT_EQ(Object::FALSE_OBJECT(), component->getCalculated(kPropertyAutoplay));
}

static const char *NEW_AUTO_PLAY_VIDEO =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Video\","
    "      \"autoplay\": \"false\""
    "    }"
    "  }"
    "}";

/**
 * With the release of 1.1, we evaluate the "autoplay" property in the documented manner,
 * where any non-empty string will evaluate to true.
 */
TEST_F(BuilderTest, NewAutoPlayVideo)
{
    loadDocument(NEW_AUTO_PLAY_VIDEO);
    ASSERT_EQ(Object::TRUE_OBJECT(), component->getCalculated(kPropertyAutoplay));
}

static const char *FULL_VIDEO = "{"
                                  "  \"type\": \"APL\","
                                  "  \"version\": \"1.0\","
                                  "  \"theme\": \"dark\","
                                  "  \"mainTemplate\": {"
                                  "    \"parameters\": ["
                                  "      \"payload\""
                                  "    ],"
                                  "    \"item\": {"
                                  "      \"id\": \"abc\","
                                  "      \"type\": \"Video\","
                                  "      \"audioTrack\": \"background\","
                                  "      \"autoplay\": \"true\","
                                  "      \"scale\": \"best-fill\","
                                  "      \"source\": [ "
                                  "        \"URL1\","
                                  "        { \"url\": \"URL2\" },"
                                  "        { "
                                  "          \"description\": \"Sample video.\","
                                  "          \"duration\": 1000,"
                                  "          \"url\": \"URL3\","
                                  "          \"repeatCount\": 2,"
                                  "          \"entity\": [ \"Entity.\" ],"
                                  "          \"offset\": 100"
                                  "        }"
                                  "      ],"
                                  "      \"onEnd\": ["
                                  "       {"
                                  "          \"type\": \"PlayMedia\""
                                  "       }"
                                  "      ],"
                                  "      \"onPause\": ["
                                  "       {"
                                  "          \"type\": \"PlayMedia\""
                                  "       },"
                                  "       {"
                                  "          \"type\": \"SendEvent\""
                                  "       }"
                                  "      ],"
                                  "      \"onPlay\": ["
                                  "       {"
                                  "          \"type\": \"PlayMedia\""
                                  "       },"
                                  "       {"
                                  "          \"type\": \"SetValue\""
                                  "       },"
                                  "       {"
                                  "          \"type\": \"SendEvent\""
                                  "       }"
                                  "      ],"
                                  "      \"onTrackUpdate\": ["
                                  "       {"
                                  "          \"type\": \"PlayMedia\""
                                  "       },"
                                  "       {"
                                  "          \"type\": \"SetValue\""
                                  "       },"
                                  "       {"
                                  "          \"type\": \"SetPage\""
                                  "       },"
                                  "       {"
                                  "          \"type\": \"SendEvent\""
                                  "       }"
                                  "      ]"
                                  "    }"
                                  "  }"
                                  "}";

TEST_F(BuilderTest, FullVideo)
{
    loadDocument(FULL_VIDEO, DATA);
    auto map = component->getCalculated();

    ASSERT_EQ(kComponentTypeVideo, component->getType());

    // ID tests
    ASSERT_EQ(Object("abc"), component->getId());
    ASSERT_EQ(component, context->findComponentById(component->getUniqueId()));
    ASSERT_EQ(component, context->findComponentById("abc"));
    ASSERT_FALSE(context->findComponentById("foo"));

    // Standard properties
    ASSERT_EQ("", map.get(kPropertyAccessibilityLabel).getString());
    ASSERT_EQ(Object::EMPTY_ARRAY(), component->getCalculated(kPropertyAccessibilityActions));
    ASSERT_EQ(Object::FALSE_OBJECT(), map.get(kPropertyDisabled));
    ASSERT_EQ(Object(Dimension(100)), map.get(kPropertyHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), map.get(kPropertyMaxHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), map.get(kPropertyMaxWidth));
    ASSERT_EQ(Object(Dimension(0)), map.get(kPropertyMinHeight));
    ASSERT_EQ(Object(Dimension(0)), map.get(kPropertyMinWidth));
    ASSERT_EQ(1.0, map.get(kPropertyOpacity).getDouble());
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingBottom));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingLeft));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingRight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyPaddingTop));
    ASSERT_EQ(Object(ObjectArray{}), component->getCalculated(kPropertyPadding));
    ASSERT_EQ(kRoleNone, component->getCalculated(kPropertyRole).getInteger());
    ASSERT_EQ(Object(Dimension(100)), map.get(kPropertyWidth));
    ASSERT_EQ(Object::TRUE_OBJECT(), component->getCalculated(kPropertyLaidOut));

    ASSERT_EQ(kVideoScaleBestFill, map.get(kPropertyScale).getInteger());
    ASSERT_EQ(kAudioTrackBackground, map.get(kPropertyAudioTrack).getInteger());
    ASSERT_EQ(1, map.get(kPropertyOnEnd).size());
    ASSERT_EQ(2, map.get(kPropertyOnPause).size());
    ASSERT_EQ(3, map.get(kPropertyOnPlay).size());
    ASSERT_EQ(4, map.get(kPropertyOnTrackUpdate).size());
    ASSERT_EQ(true, map.get(kPropertyAutoplay).getBoolean());

    ASSERT_EQ(3, map.get(kPropertySource).size());
    auto source1 = map.get(kPropertySource).at(0).getMediaSource();
    ASSERT_EQ("", source1.getDescription());
    ASSERT_EQ(0, source1.getDuration());
    ASSERT_EQ("URL1", source1.getUrl());
    ASSERT_EQ(0, source1.getRepeatCount());
    ASSERT_TRUE(source1.getEntities().empty());
    ASSERT_EQ(0, source1.getOffset());

    auto source2 = map.get(kPropertySource).at(1).getMediaSource();
    ASSERT_EQ("", source2.getDescription());
    ASSERT_EQ(0, source2.getDuration());
    ASSERT_EQ("URL2", source2.getUrl());
    ASSERT_EQ(0, source2.getRepeatCount());
    ASSERT_TRUE(source2.getEntities().empty());
    ASSERT_EQ(0, source2.getOffset());

    auto source3 = map.get(kPropertySource).at(2).getMediaSource();
    ASSERT_EQ("Sample video.", source3.getDescription());
    ASSERT_EQ(1000, source3.getDuration());
    ASSERT_EQ("URL3", source3.getUrl());
    ASSERT_EQ(2, source3.getRepeatCount());
    ASSERT_EQ(1, source3.getEntities().size());
    ASSERT_EQ(100, source3.getOffset());
}

static const char *MEDIA_SOURCE = "{"
                                "  \"type\": \"APL\","
                                "  \"version\": \"1.1\","
                                "  \"mainTemplate\": {"
                                "    \"item\": "
                                "    {"
                                "      \"type\": \"Container\","
                                "      \"items\":"
                                "      ["
                                "        {"
                                "          \"type\": \"Video\""
                                "        },"
                                "        {"
                                "          \"type\": \"Video\","
                                "          \"source\": \"URL1\""
                                "        },"
                                "        {"
                                "          \"type\": \"Video\","
                                "          \"source\":"
                                "          {"
                                "            \"description\": \"Sample video.\","
                                "            \"duration\": 1000,"
                                "            \"url\": \"URL1\","
                                "            \"repeatCount\": 2,"
                                "            \"entity\": [ \"Entity.\" ],"
                                "            \"offset\": 100"
                                "          }"
                                "        },"
                                "        {"
                                "          \"type\": \"Video\","
                                "          \"source\": [ \"URL1\", { \"url\": \"URL2\" } ]"
                                "        }"
                                "      ]"
                                "    }"
                                "  }"
                                "}";

TEST_F(BuilderTest, MediaSource)
{
    loadDocument(MEDIA_SOURCE);

    ASSERT_EQ(kComponentTypeContainer, component->getType());
    ASSERT_EQ(4, component->getChildCount());

    auto video0 = component->getCoreChildAt(0);
    auto video1 = component->getCoreChildAt(1);
    auto video2 = component->getCoreChildAt(2);
    auto video3 = component->getCoreChildAt(3);

    ASSERT_EQ(kComponentTypeVideo, video0->getType());
    ASSERT_EQ(kComponentTypeVideo, video1->getType());
    ASSERT_EQ(kComponentTypeVideo, video2->getType());
    ASSERT_EQ(kComponentTypeVideo, video3->getType());

    auto sources = video0->getCalculated(kPropertySource);
    ASSERT_TRUE(sources.isArray());
    ASSERT_TRUE(sources.empty());

    sources = video1->getCalculated(kPropertySource);
    ASSERT_TRUE(sources.isArray());
    ASSERT_EQ(1, sources.size());
    auto source = sources.at(0).getMediaSource();
    ASSERT_EQ("", source.getDescription());
    ASSERT_EQ(0, source.getDuration());
    ASSERT_EQ("URL1", source.getUrl());
    ASSERT_EQ(0, source.getRepeatCount());
    ASSERT_TRUE(source.getEntities().empty());
    ASSERT_EQ(0, source.getOffset());

    sources = video2->getCalculated(kPropertySource);
    ASSERT_TRUE(sources.isArray());
    ASSERT_EQ(1, sources.size());
    source = sources.at(0).getMediaSource();
    ASSERT_EQ("Sample video.", source.getDescription());
    ASSERT_EQ(1000, source.getDuration());
    ASSERT_EQ("URL1", source.getUrl());
    ASSERT_EQ(2, source.getRepeatCount());
    ASSERT_EQ(1, source.getEntities().size());
    ASSERT_EQ(100, source.getOffset());

    sources = video3->getCalculated(kPropertySource);
    ASSERT_TRUE(sources.isArray());
    ASSERT_EQ(2, sources.size());
    source = sources.at(0).getMediaSource();
    ASSERT_EQ("", source.getDescription());
    ASSERT_EQ(0, source.getDuration());
    ASSERT_EQ("URL1", source.getUrl());
    ASSERT_EQ(0, source.getRepeatCount());
    ASSERT_EQ(0, source.getOffset());
    source = sources.at(1).getMediaSource();
    ASSERT_EQ("", source.getDescription());
    ASSERT_EQ(0, source.getDuration());
    ASSERT_EQ("URL2", source.getUrl());
    ASSERT_EQ(0, source.getRepeatCount());
    ASSERT_TRUE(source.getEntities().empty());
    ASSERT_EQ(0, source.getOffset());
}

static const char *MEDIA_SOURCE_2 =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"parameters\": ["
    "      \"payload\""
    "    ],"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Video\","
    "          \"source\": \"${payload.movie.properties.single}\""
    "        },"
    "        {"
    "          \"type\": \"Video\","
    "          \"source\": ["
    "            \"${payload.movie.properties.single}\""
    "          ]"
    "        },"
    "        {"
    "          \"type\": \"Video\","
    "          \"source\": {"
    "            \"url\": \"${payload.movie.properties.single}\""
    "          }"
    "        },"
    "        {"
    "          \"type\": \"Video\","
    "          \"source\": ["
    "            {"
    "              \"url\": \"${payload.movie.properties.single}\""
    "            }"
    "          ]"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

static const char *MEDIA_SOURCE_2_DATA =
    "{"
    "  \"movie\": {"
    "    \"properties\": {"
    "      \"single\": \"URL1\""
    "    }"
    "  }"
    "}";

TEST_F(BuilderTest, MediaSource2)
{
    loadDocument(MEDIA_SOURCE_2, MEDIA_SOURCE_2_DATA);

    ASSERT_EQ(kComponentTypeContainer, component->getType());
    ASSERT_EQ(4, component->getChildCount());

    for (int i = 0 ; i < component->getChildCount() ; i++) {
        std::string msg = "Test case #" + std::to_string(i);
        auto video = component->getCoreChildAt(i);
        ASSERT_EQ(kComponentTypeVideo, video->getType()) << msg;
        auto sources = video->getCalculated(kPropertySource);
        ASSERT_TRUE(sources.isArray()) << msg;
        ASSERT_EQ(1, sources.size()) << msg;
        auto source = sources.at(0).getMediaSource();
        ASSERT_EQ("URL1", source.getUrl()) << msg;
    }
}

static const char * KARAOKE_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"styles\": {"
    "    \"basic\": {"
    "      \"values\": ["
    "        {"
    "          \"color\": \"green\""
    "        },"
    "        {"
    "          \"when\": \"${state.karaoke}\","
    "          \"color\": \"red\""
    "        },"
    "        {"
    "          \"when\": \"${state.karaokeTarget}\","
    "          \"color\": \"yellow\""
    "        },"
    "        {"
    "          \"when\": \"${state.disabled}\","
    "          \"color\": \"blue\""
    "        },"
    "        {"
    "          \"when\": \"${state.karaoke && state.disabled}\","
    "          \"color\": \"black\""
    "        }"
    "      ]"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"style\": \"basic\""
    "    }"
    "  }"
    "}";

TEST_F(BuilderTest, KaraokeStyle)
{
    loadDocument(KARAOKE_TEST);

    // Both colors should be green
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), component->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), component->getCalculated(kPropertyColorKaraokeTarget)));
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), component->getCalculated(kPropertyColorNonKaraoke)));

    // Karaoke State
    component->setState(kStateKaraoke, true);
    ASSERT_TRUE(IsEqual(Color(Color::RED), component->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual(Color(Color::YELLOW), component->getCalculated(kPropertyColorKaraokeTarget)));
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), component->getCalculated(kPropertyColorNonKaraoke)));
    ASSERT_TRUE(CheckDirty(component, kPropertyColor, kPropertyColorKaraokeTarget));
    ASSERT_TRUE(CheckDirty(root, component));

    // Karaoke + disabled
    component->setProperty(kPropertyDisabled, true);
    ASSERT_TRUE(IsEqual(Color(Color::BLACK), component->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual(Color(Color::BLACK), component->getCalculated(kPropertyColorKaraokeTarget)));
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), component->getCalculated(kPropertyColorNonKaraoke)));
    ASSERT_TRUE(CheckDirty(component, kPropertyColor, kPropertyColorKaraokeTarget,
                           kPropertyColorNonKaraoke, kPropertyDisabled));
    ASSERT_TRUE(CheckDirty(root, component));

    // Disabled
    component->setState(kStateKaraoke, false);
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), component->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), component->getCalculated(kPropertyColorKaraokeTarget)));
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), component->getCalculated(kPropertyColorNonKaraoke)));
    ASSERT_TRUE(CheckDirty(component, kPropertyColor, kPropertyColorKaraokeTarget));
    ASSERT_TRUE(CheckDirty(root, component));

    // Back to the start
    component->setProperty(kPropertyDisabled, false);
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), component->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), component->getCalculated(kPropertyColorKaraokeTarget)));
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), component->getCalculated(kPropertyColorNonKaraoke)));
    ASSERT_TRUE(CheckDirty(component, kPropertyColor, kPropertyColorKaraokeTarget,
                           kPropertyColorNonKaraoke, kPropertyDisabled));
    ASSERT_TRUE(CheckDirty(root, component));
}

static const char * BIND_SIMPLE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.2\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"bind\": {"
    "        \"name\": \"foo\","
    "        \"value\": 10"
    "      },"
    "      \"text\": \"${foo}\""
    "    }"
    "  }"
    "}";

TEST_F(BuilderTest, BindSimple)
{
    loadDocument(BIND_SIMPLE);

    ASSERT_EQ("10", component->getCalculated(kPropertyText).asString());
}

static const char *BIND_TWO =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.2\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"bind\": ["
    "        {"
    "          \"name\": \"foo\","
    "          \"value\": 10"
    "        },"
    "        {"
    "          \"name\": \"bar\","
    "          \"value\": 20"
    "        }"
    "      ],"
    "      \"text\": \"${foo + bar}\""
    "    }"
    "  }"
    "}";

TEST_F(BuilderTest, BindTwo)
{
    loadDocument(BIND_TWO);

    ASSERT_EQ("30", component->getCalculated(kPropertyText).asString());
}

static const char *BIND_UNUSED =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.2\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"bind\": ["
    "        {"
    "          \"name\": \"foo\","
    "          \"value\": 10"
    "        },"
    "        {"
    "          \"name\": \"bar\","
    "          \"value\": \"${foo + 20}\""
    "        }"
    "      ],"
    "      \"text\": \"Hello 10\""
    "    }"
    "  }"
    "}";

TEST_F(BuilderTest, BindUnused)
{
    loadDocument(BIND_UNUSED);

    ASSERT_EQ("Hello 10", component->getCalculated(kPropertyText).asString());
}

static const char * BIND_NUMBER =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"${foo + ':' + bar}\","
    "      \"bind\": ["
    "        {"
    "          \"name\": \"foo\","
    "          \"value\": 10,"
    "          \"type\": \"number\""
    "        },"
    "        {"
    "          \"name\": \"bar\","
    "          \"value\": \"${foo + 23}\","
    "          \"type\": \"number\""
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";


TEST_F(BuilderTest, BindNumber) {
    loadDocument(BIND_NUMBER);

    ASSERT_EQ("10:33", component->getCalculated(kPropertyText).asString());
}

static const char *BIND_VARIOUS =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"${mixedBag}\","
    "      \"color\": \"${myTextColorName}\","
    "      \"fontSize\": \"${myFontSize}\","
    "      \"opacity\": \"${isHidden ? 0 : 1}\","
    "      \"bind\": ["
    "        {"
    "          \"name\": \"myTextColor\","
    "          \"value\": \"green\","
    "          \"type\": \"color\""
    "        },"
    "        {"
    "          \"name\": \"myFontSize\","
    "          \"value\": \"20dp\","
    "          \"type\": \"dimension\""
    "        },"
    "        {"
    "          \"name\": \"isHidden\","
    "          \"value\": \"true\","
    "          \"type\": \"boolean\""
    "        },"
    "        {"
    "          \"name\": \"myTextColorName\","
    "          \"value\": \"green\","
    "          \"type\": \"string\""
    "        },"
    "        {"
    "          \"name\": \"mixedBag\","
    "          \"value\": \"${myTextColorName+isHidden}\""
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(BuilderTest, BindVarious) {
    loadDocument(BIND_VARIOUS);

    ASSERT_EQ("greentrue", component->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Object(Dimension(20)), component->getCalculated(kPropertyFontSize));
    ASSERT_EQ(Object(0), component->getCalculated(kPropertyOpacity));
    ASSERT_EQ(Object(Color(Color::GREEN)), component->getCalculated(kPropertyColor));
}

static const char *TRANSFORM_ON_PRESS =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"id\": \"myFrame\","
    "        \"width\": 20,"
    "        \"height\": 100"
    "      },"
    "      \"onPress\": {"
    "        \"type\": \"SetValue\","
    "        \"componentId\": \"myFrame\","
    "        \"property\": \"transform\","
    "        \"value\": ["
    "          {"
    "            \"scale\": 2"
    "          },"
    "          {"
    "            \"translateX\": 30"
    "          }"
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";


TEST_F(BuilderTest, TransformOnPress)
{
    loadDocument(TRANSFORM_ON_PRESS);

    auto frame = component->getChildAt(0);

    ASSERT_EQ(Object::IDENTITY_2D(), frame->getCalculated(kPropertyTransform));

    performClick(1, 1);
    root->clearPending();

    auto t = frame->getCalculated(kPropertyTransform).getTransform2D();
    // (0,0) -> (-10, -50) -> (20, -50) -> (40, -100) -> (50, -50)
    ASSERT_EQ(Point(50,-50), t * Point());
}

static const char *TRANSFORM_WITH_RESOURCES =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"resources\": ["
    "    {"
    "      \"numbers\": {"
    "        \"ROTATE\": -90,"
    "        \"SCALE\": 0.5"
    "      },"
    "      \"dimensions\": {"
    "        \"ONE\": \"50vh\""
    "      }"
    "    }"
    "  ],"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"id\": \"myFrame\","
    "        \"width\": 20,"
    "        \"height\": 100,"
    "        \"transform\": ["
    "          {"
    "            \"rotate\": \"@ROTATE\""
    "          },"
    "          {"
    "            \"translateY\": \"@ONE\""
    "          }"
    "        ]"
    "      },"
    "      \"onPress\": {"
    "        \"type\": \"SetValue\","
    "        \"componentId\": \"myFrame\","
    "        \"property\": \"transform\","
    "        \"value\": ["
    "          {"
    "            \"scale\": \"@SCALE\""
    "          },"
    "          {"
    "            \"translateX\": \"25%\""
    "          }"
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(BuilderTest, TransformWithResources)
{
    loadDocument(TRANSFORM_WITH_RESOURCES);

    auto frame = component->getChildAt(0);
    auto t = frame->getCalculated(kPropertyTransform).getTransform2D();

    //     Center      Ty=+400       Rot=-90       De-Center
    // (0,0) -> (-10,-50) -> (-10, 350) -> (350,10) -> (360, 60)
    ASSERT_EQ(Point(360,60), t * Point());

    // Now press and replace the existing things
    performClick(1, 1);
    root->clearPending();

    t = frame->getCalculated(kPropertyTransform).getTransform2D();
    //     Center        Tx=+5        Scale=0.5     De-center
    // (0,0) -> (-10, -50) -> (-5, -50) -> (-2.5, -25) -> (7.5, 25)
    ASSERT_EQ(Point(7.5,25), t * Point());
}

static const char *DISPLAY_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Frame\","
    "          \"id\": \"thing1\","
    "          \"height\": 100,"
    "          \"width\": 200"
    "        },"
    "        {"
    "          \"type\": \"Frame\","
    "          \"id\": \"thing2\","
    "          \"height\": 200,"
    "          \"width\": 100"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(BuilderTest, DisplayTest)
{
    loadDocument(DISPLAY_TEST);
    auto thing1 = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("thing1"));
    auto thing2 = std::dynamic_pointer_cast<CoreComponent>(root->context().findComponentById("thing2"));

    ASSERT_TRUE(thing1);
    ASSERT_TRUE(thing2);

    ASSERT_EQ(Object(Rect(0,0,200,100)), thing1->getCalculated(kPropertyBounds));
    ASSERT_EQ(Object(Rect(0,100,100,200)), thing2->getCalculated(kPropertyBounds));

    thing1->setProperty(kPropertyDisplay, "none");
    root->clearPending();

    ASSERT_EQ(Object(kDisplayNone), thing1->getCalculated(kPropertyDisplay));
    ASSERT_EQ(Object(Rect(0,0,0,0)), thing1->getCalculated(kPropertyBounds));
    ASSERT_EQ(Object(Rect(0,0,100,200)), thing2->getCalculated(kPropertyBounds));  // Shifts upwards

    ASSERT_TRUE(CheckDirty(thing1,
                           kPropertyDisplay, kPropertyBounds, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(thing2, kPropertyBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

    thing1->setProperty(kPropertyDisplay, "invisible");
    root->clearPending();

    ASSERT_EQ(Object(kDisplayInvisible), thing1->getCalculated(kPropertyDisplay));
    ASSERT_EQ(Object(Rect(0,0,200,100)), thing1->getCalculated(kPropertyBounds));
    ASSERT_EQ(Object(Rect(0,100,100,200)), thing2->getCalculated(kPropertyBounds));  // Shifts downwards

    ASSERT_TRUE(CheckDirty(thing1,
                           kPropertyDisplay, kPropertyBounds, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(thing2, kPropertyBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

    thing1->setProperty(kPropertyDisplay, "normal");
    root->clearPending();

    ASSERT_EQ(Object(kDisplayNormal), thing1->getCalculated(kPropertyDisplay));
    ASSERT_EQ(Object(Rect(0,0,200,100)), thing1->getCalculated(kPropertyBounds));
    ASSERT_EQ(Object(Rect(0,100,100,200)), thing2->getCalculated(kPropertyBounds));

    ASSERT_TRUE(CheckDirty(thing1, kPropertyDisplay));
    ASSERT_TRUE(CheckDirty(thing2));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));
}

static const char *USER_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"-user-tag\": 234,"
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"id\": \"text0\","
    "        \"-user-note\": \"This is a note\","
    "        \"-user-array\": ["
    "          1,"
    "          2,"
    "          3"
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(BuilderTest, UserTest)
{
    loadDocument(USER_TEST);
    auto text0 = context->findComponentById("text0");

    auto user1 = component->getCalculated(kPropertyUser);
    ASSERT_TRUE(user1.isMap());
    ASSERT_EQ(1, user1.size());
    ASSERT_EQ(Object(234), user1.get("tag"));

    auto user2 = text0->getCalculated(kPropertyUser);
    ASSERT_TRUE(user2.isMap());
    ASSERT_EQ(2, user2.size());
    ASSERT_EQ(Object("This is a note"), user2.get("note"));
    ASSERT_TRUE(user2.get("array").isArray());
    ASSERT_EQ(3, user2.get("array").size());
    ASSERT_EQ(Object(1), user2.get("array").at(0));
    ASSERT_EQ(Object(2), user2.get("array").at(1));
    ASSERT_EQ(Object(3), user2.get("array").at(2));
}

static const char *LABEL_TEST_BASE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"id\": \": 234_abZ\""
    "    }"
    "  }"
    "}";

TEST_F(BuilderTest, LabelTestBase)
{
    loadDocument(LABEL_TEST_BASE);
    ASSERT_EQ(Object("234_abZ"), component->getId());
}

static const char *LABEL_TEST_HYPHEN =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"item\": {"
        "      \"type\": \"Container\","
        "      \"id\": \": 234-abZ\""
        "    }"
        "  }"
        "}";

TEST_F(BuilderTest, LabelTestHyphen)
{
    loadDocument(LABEL_TEST_HYPHEN);
    // we secretly allow hyphens
    ASSERT_EQ(Object("234-abZ"), component->getId());
}

static const char *LABEL_TEST_INVALID =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"item\": {"
        "      \"type\": \"Container\","
        "      \"id\": \": 234-ab*&*Z@\""
        "    }"
        "  }"
        "}";

TEST_F(BuilderTest, LabelTestInvalid)
{
    loadDocument(LABEL_TEST_INVALID);
    // should strip out bad characters
    ASSERT_EQ(Object("234-abZ"), component->getId());
}

static const char *ENTITY_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"resources\": ["
    "    {"
    "      \"string\": {"
    "        \"myString\": \"23\""
    "      },"
    "      \"number\": {"
    "        \"myNumber\": \"${1+2+3}\""
    "      }"
    "    }"
    "  ],"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Frame\","
    "      \"entities\": {"
    "        \"a\": {"
    "          \"alpha\": \"@myString\","
    "          \"beta\": \"${2+3}\""
    "        },"
    "        \"b\": ["
    "          \"@myNumber\","
    "          92"
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(BuilderTest, EntityTest)
{
    loadDocument(ENTITY_TEST);
    auto entity = component->getCalculated(kPropertyEntities);

    ASSERT_TRUE(entity.isArray());
    ASSERT_EQ(1, entity.size());

    auto inner = entity.at(0);
    ASSERT_TRUE(inner.isMap());
    ASSERT_EQ(2, inner.size());
    ASSERT_TRUE(inner.has("a"));
    ASSERT_TRUE(inner.has("b"));

    auto a = inner.get("a");
    ASSERT_TRUE(a.isMap());
    ASSERT_EQ(2, a.size());
    ASSERT_TRUE(a.has("alpha"));
    ASSERT_TRUE(a.has("beta"));

    auto alpha = a.get("alpha");
    ASSERT_TRUE(alpha.isString());
    ASSERT_TRUE(IsEqual(Object("23"), alpha));

    auto beta = a.get("beta");
    ASSERT_TRUE(beta.isNumber());
    ASSERT_EQ(5, beta.asNumber());

    auto b = inner.get("b");
    ASSERT_TRUE(b.isArray());
    ASSERT_EQ(2, b.size());

    auto first = b.at(0);
    ASSERT_TRUE(first.isNumber());
    ASSERT_EQ(6, first.asNumber());

    auto second = b.at(1);
    ASSERT_TRUE(second.isNumber());
    ASSERT_EQ(92, second.asNumber());
}


static const char *CONFIG_TEXT_DEFAULT_THEME =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"hello\""
    "    }"
    "  }"
    "}";

// Verify that we can configure the default text color and font family
TEST_F(BuilderTest, ConfigTextDarkTheme)
{
    config->defaultFontFamily("Helvetica");

    // The default theme is "dark", which has a color of 0xFAFAFAFF
    loadDocument(CONFIG_TEXT_DEFAULT_THEME);
    ASSERT_TRUE(IsEqual(Color(0xFAFAFAFF), component->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual(Color(0xFAFAFAFF), component->getCalculated(kPropertyColorKaraokeTarget)));
    ASSERT_TRUE(IsEqual("Helvetica", component->getCalculated(kPropertyFontFamily)));

    // Override the generic theme color.  The document defaults to dark theme, so this is ignored
    config->defaultFontColor(0x11223344);
    loadDocument(CONFIG_TEXT_DEFAULT_THEME);
    ASSERT_TRUE(IsEqual(Color(0xFAFAFAFF), component->getCalculated(kPropertyColor)));

    // Explicitly override the 'dark' theme color
    config->defaultFontColor("dark", 0x44332211);
    loadDocument(CONFIG_TEXT_DEFAULT_THEME);
    ASSERT_TRUE(IsEqual(Color(0x44332211), component->getCalculated(kPropertyColor)));
}

static const char *CONFIG_TEXT_LIGHT_THEME =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"theme\": \"light\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"hello\""
    "    }"
    "  }"
    "}";

// Check the light theme
TEST_F(BuilderTest, ConfigTextLightTheme)
{
    // The default light theme color color is 0x1E2222FF
    loadDocument(CONFIG_TEXT_LIGHT_THEME);
    ASSERT_TRUE(IsEqual(Color(0x1E2222FF), component->getCalculated(kPropertyColor)));

    // Override the generic theme color.  The document has a theme, so this is ignored
    config->defaultFontColor(0x11223344);
    loadDocument(CONFIG_TEXT_LIGHT_THEME);
    ASSERT_TRUE(IsEqual(Color(0x1E2222FF), component->getCalculated(kPropertyColor)));

    // Explicitly override the 'light' theme color
    config->defaultFontColor("light", 0x44332211);
    loadDocument(CONFIG_TEXT_LIGHT_THEME);
    ASSERT_TRUE(IsEqual(Color(0x44332211), component->getCalculated(kPropertyColor)));
}

static const char *CONFIG_TEXT_FUZZY_THEME =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"theme\": \"fuzzy\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"hello\""
    "    }"
    "  }"
    "}";

// Check the use of a custom theme
TEST_F(BuilderTest, ConfigTextFuzzyTheme)
{
    // The default color is 0xFAFAFAFF
    loadDocument(CONFIG_TEXT_FUZZY_THEME);
    ASSERT_TRUE(IsEqual(Color(0xfafafaff), component->getCalculated(kPropertyColor)));

    // Override the generic theme color.  Because 'fuzzy' isn't light or dark, this should happen
    config->defaultFontColor(0x11223344);
    loadDocument(CONFIG_TEXT_FUZZY_THEME);
    ASSERT_TRUE(IsEqual(Color(0x11223344), component->getCalculated(kPropertyColor)));

    // Explicitly override the 'fuzzy' theme color
    config->defaultFontColor("fuzzy", 0x44332211);
    loadDocument(CONFIG_TEXT_FUZZY_THEME);
    ASSERT_TRUE(IsEqual(Color(0x44332211), component->getCalculated(kPropertyColor)));
}

static const char *TEST_NULL_PAYLOAD =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"parameters\": ["
    "      \"payload\""
    "    ],"
    "    \"items\": ["
    "      {"
    "        \"when\": \"${payload.data.quantity == null}\","
    "        \"type\": \"Text\","
    "        \"text\": \"Null worked\""
    "      },"
    "      {"
    "        \"type\": \"Text\","
    "        \"text\": \"Null did not work\""
    "      }"
    "    ]"
    "  }"
    "}";

static const char *NULL_PAYLOAD =
    "{"
    "  \"data\": {"
    "    \"quantity\": null,"
    "    \"price\": 14.99"
    "  }"
    "}";

TEST_F(BuilderTest, NullPayload)
{
    loadDocument(TEST_NULL_PAYLOAD, NULL_PAYLOAD);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("Null worked", component->getCalculated(kPropertyText).asString()));
}

static const char *RESOURCE_LOOKUP_AT_BINDING = R"apl(
{
    "type":"APL",
    "version":"1.5",
    "resources":{
        "numbers":{
            "NumberDown":2,
            "NumberUp":1
        }
    },
    "mainTemplate":{
        "items":[
            {
                "type":"TouchWrapper",
                "bind":[
                    {
                        "name": "Dummy",
                        "value": 10
                    },
                    {
                        "name": "Action",
                        "value": "Up"
                    },
                    {
                        "name":"NumberAction",
                        "value":"@Number${Action}"
                    },
                    {
                        "name":"NumberActionResult",
                        "value":"${NumberAction + Dummy}"
                    }
                ],
                "onDown":[
                    {
                        "type":"SetValue",
                        "property":"Action",
                        "value":"Down"
                    }
                ],
                "onUp":[
                    {
                        "type":"SetValue",
                        "property":"Action",
                        "value":"Up"
                    }
                ],
                "item":{
                    "type":"Text",
                    "id": "myText",
                    "text":"${NumberActionResult}",
                    "Action": "Down"
                }
            }
        ]
    }
}
)apl";

TEST_F(BuilderTest, ResourceLookupAtBinding)
{
    loadDocument(RESOURCE_LOOKUP_AT_BINDING);
    ASSERT_TRUE(component);
    auto text = component->findComponentById("myText");

    //Default value of text component
    ASSERT_TRUE(IsEqual("11", text->getCalculated(kPropertyText).asString()));

    //onDown text component will be updated to 12
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(1,1)));
    ASSERT_TRUE(IsEqual("12", text->getCalculated(kPropertyText).asString()));

    //onUp text component will be updated to 11
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(1,1)));
    ASSERT_TRUE(IsEqual("11", text->getCalculated(kPropertyText).asString()));
}

static const char *BASIC_START_END_PADDING = R"(
{
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "item": {
      "id": "cont",
      "type": "Container",
      "direction": "row",
      "height": 200,
      "width": 500,
      "items": [
        {
          "id": "paddedFrame",
          "type": "Frame",
          "backgroundColor": "blue",
          "paddingEnd": 20,
          "item": {
            "id": "paddedFrameChild",
            "type": "Frame",
            "backgroundColor": "pink",
            "item": {
              "type": "Text",
              "text": "Test text"
            }
          }
        },
        {
          "id": "paddedFrame2",
          "type": "Frame",
          "backgroundColor": "blue",
          "paddingStart": 10,
          "item": {
            "id": "paddedFrameChild2",
            "type": "Frame",
            "backgroundColor": "pink",
            "item": {
              "type": "Text",
              "text": "Test text"
            }
          }
        }
      ]
    }
  }
}
)";

/**
 * Make sure paddingStart overrides paddingLeft
 */
TEST_F(BuilderTest, BasicStartEndPaddingLTR)
{
    loadDocument(BASIC_START_END_PADDING);

    auto frame = component->findComponentById("paddedFrame");
    auto frame2 = component->findComponentById("paddedFrame2");
    auto child = component->findComponentById("paddedFrameChild");
    auto child2 = component->findComponentById("paddedFrameChild2");
    ASSERT_EQ(Object::NULL_OBJECT(), frame->getCalculated(kPropertyPaddingBottom));
    ASSERT_EQ(Object::NULL_OBJECT(), frame->getCalculated(kPropertyPaddingLeft));
    ASSERT_EQ(Object::NULL_OBJECT(), frame->getCalculated(kPropertyPaddingRight));
    ASSERT_EQ(Object::NULL_OBJECT(), frame->getCalculated(kPropertyPaddingTop));
    ASSERT_EQ(Object::NULL_OBJECT(), frame->getCalculated(kPropertyPaddingStart));
    ASSERT_EQ(20.0, frame->getCalculated(kPropertyPaddingEnd).asNumber());
    ASSERT_EQ(Object(ObjectArray{}), frame->getCalculated(kPropertyPadding));

    EXPECT_TRUE(expectBounds(frame, 0, 0, 200, 110));
    EXPECT_TRUE(expectInnerBounds(frame, 0, 0, 200, 90));
    EXPECT_TRUE(expectBounds(child, 0, 0, 10, 90));
    EXPECT_TRUE(expectInnerBounds(child, 0, 0, 10, 90));
    EXPECT_TRUE(expectBounds(frame2, 0, 110, 200, 210));
    EXPECT_TRUE(expectInnerBounds(frame2, 0, 10, 200, 100));
    EXPECT_TRUE(expectBounds(child2, 0, 10, 10, 100));
    EXPECT_TRUE(expectInnerBounds(child2, 0, 0, 10, 90));
}

static const char *START_END_PADDING_OVERRIDE = R"(
{
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "item": {
      "id": "cont",
      "type": "Container",
      "direction": "row",
      "height": 200,
      "width": 500,
      "items": [
        {
          "id": "paddedFrame",
          "type": "Frame",
          "backgroundColor": "blue",
          "paddingRight": 10,
          "paddingEnd": 20,
          "item": {
            "id": "paddedFrameChild",
            "type": "Frame",
            "backgroundColor": "pink",
            "item": {
              "type": "Text",
              "text": "Test text"
            }
          }
        },
        {
          "id": "paddedFrame2",
          "type": "Frame",
          "backgroundColor": "blue",
          "paddingStart": 10,
          "paddingLeft": 20,
          "item": {
            "id": "paddedFrameChild2",
            "type": "Frame",
            "backgroundColor": "pink",
            "item": {
              "type": "Text",
              "text": "Test text"
            }
          }
        }
      ]
    }
  }
}
)";

/**
 * Make sure paddingEnd overrides paddingRight and padding start overrides paddingLeft
 */
TEST_F(BuilderTest, ComplexStartEndPaddingLTR)
{
    loadDocument(START_END_PADDING_OVERRIDE);

    auto frame = component->findComponentById("paddedFrame");
    auto frame2 = component->findComponentById("paddedFrame2");
    auto child = component->findComponentById("paddedFrameChild");
    auto child2 = component->findComponentById("paddedFrameChild2");
    ASSERT_EQ(Object::NULL_OBJECT(), frame->getCalculated(kPropertyPaddingBottom));
    ASSERT_EQ(Object::NULL_OBJECT(), frame->getCalculated(kPropertyPaddingLeft));
    ASSERT_EQ(10.0, frame->getCalculated(kPropertyPaddingRight).asNumber());
    ASSERT_EQ(Object::NULL_OBJECT(), frame->getCalculated(kPropertyPaddingTop));
    ASSERT_EQ(Object::NULL_OBJECT(), frame->getCalculated(kPropertyPaddingStart));
    ASSERT_EQ(20.0, frame->getCalculated(kPropertyPaddingEnd).asNumber());
    ASSERT_EQ(Object(ObjectArray{}), frame->getCalculated(kPropertyPadding));

    EXPECT_TRUE(expectBounds(frame, 0, 0, 200, 110));
    EXPECT_TRUE(expectInnerBounds(frame, 0, 0, 200, 90));
    EXPECT_TRUE(expectBounds(child, 0, 0, 10, 90));
    EXPECT_TRUE(expectInnerBounds(child, 0, 0, 10, 90));
    EXPECT_TRUE(expectBounds(frame2, 0, 110, 200, 210));
    EXPECT_TRUE(expectInnerBounds(frame2, 0, 10, 200, 100));
    EXPECT_TRUE(expectBounds(child2, 0, 10, 10, 100));
    EXPECT_TRUE(expectInnerBounds(child2, 0, 0, 10, 90));
}

static const char *START_END_NO_PADDING_OVERRIDE = R"(
{
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "item": {
      "id": "cont",
      "type": "Container",
      "direction": "row",
      "height": 200,
      "width": 500,
      "items": [
        {
          "id": "paddedFrame",
          "type": "Frame",
          "backgroundColor": "blue",
          "item": {
            "id": "paddedFrameChild",
            "type": "Frame",
            "backgroundColor": "pink",
            "item": {
              "type": "Text",
              "text": "Test text"
            }
          }
        },
        {
          "id": "paddedFrame2",
          "type": "Frame",
          "backgroundColor": "blue",
          "item": {
            "id": "paddedFrameChild2",
            "type": "Frame",
            "backgroundColor": "pink",
            "item": {
              "type": "Text",
              "text": "Test text"
            }
          }
        }
      ]
    }
  }
}
)";

/**
 * Make sure paddingEnd overrides paddingRight and padding start overrides paddingLeft
 */
TEST_F(BuilderTest, DynamicStartEndPaddingLTR)
{
    loadDocument(START_END_NO_PADDING_OVERRIDE);

    auto frame = std::dynamic_pointer_cast<CoreComponent>(component->findComponentById("paddedFrame"));
    auto frame2 = std::dynamic_pointer_cast<CoreComponent>(component->findComponentById("paddedFrame2"));
    auto child = std::dynamic_pointer_cast<CoreComponent>(component->findComponentById("paddedFrameChild"));
    auto child2 = std::dynamic_pointer_cast<CoreComponent>(component->findComponentById("paddedFrameChild2"));

    //Check setting End and the right doesn't apply the right padding
    {
        frame->setProperty(kPropertyPaddingEnd, 20);
        root->clearPending(); // force layout changes

        EXPECT_TRUE(expectBounds(frame, 0, 0, 200, 110));
        EXPECT_TRUE(expectInnerBounds(frame, 0, 0, 200, 90));
        EXPECT_TRUE(expectBounds(child, 0, 0, 10, 90));
        EXPECT_TRUE(expectInnerBounds(child, 0, 0, 10, 90));
        EXPECT_TRUE(expectBounds(frame2, 0, 110, 200, 200));
        EXPECT_TRUE(expectInnerBounds(frame2, 0, 0, 200, 90));
        EXPECT_TRUE(expectBounds(child2, 0, 0, 10, 90));
        EXPECT_TRUE(expectInnerBounds(child2, 0, 0, 10, 90));

        // make sure adding right padding has no affect
        frame->setProperty(kPropertyPaddingRight, 20);
        root->clearPending(); // force layout changes

        EXPECT_TRUE(expectBounds(frame, 0, 0, 200, 110));
        EXPECT_TRUE(expectInnerBounds(frame, 0, 0, 200, 90));
        EXPECT_TRUE(expectBounds(child, 0, 0, 10, 90));
        EXPECT_TRUE(expectInnerBounds(child, 0, 0, 10, 90));
        EXPECT_TRUE(expectBounds(frame2, 0, 110, 200, 200));
        EXPECT_TRUE(expectInnerBounds(frame2, 0, 0, 200, 90));
        EXPECT_TRUE(expectBounds(child2, 0, 0, 10, 90));
        EXPECT_TRUE(expectInnerBounds(child2, 0, 0, 10, 90));

    }

    //Check the start overrides the left when setting dynamically
    {
        frame2->setProperty(kPropertyPaddingLeft, 20);
        root->clearPending(); // force layout changes

        EXPECT_TRUE(expectBounds(frame, 0, 0, 200, 110));
        EXPECT_TRUE(expectInnerBounds(frame, 0, 0, 200, 90));
        EXPECT_TRUE(expectBounds(child, 0, 0, 10, 90));
        EXPECT_TRUE(expectInnerBounds(child, 0, 0, 10, 90));
        EXPECT_TRUE(expectBounds(frame2, 0, 110, 200, 220));
        EXPECT_TRUE(expectInnerBounds(frame2, 0, 20, 200, 110));
        EXPECT_TRUE(expectBounds(child2, 0, 20, 10, 110));
        EXPECT_TRUE(expectInnerBounds(child2, 0, 0, 10, 90));

        frame2->setProperty(kPropertyPaddingStart, 10);
        root->clearPending(); // force layout changes

        EXPECT_TRUE(expectBounds(frame, 0, 0, 200, 110));
        EXPECT_TRUE(expectInnerBounds(frame, 0, 0, 200, 90));
        EXPECT_TRUE(expectBounds(child, 0, 0, 10, 90));
        EXPECT_TRUE(expectInnerBounds(child, 0, 0, 10, 90));
        EXPECT_TRUE(expectBounds(frame2, 0, 110, 200, 210));
        EXPECT_TRUE(expectInnerBounds(frame2, 0, 10, 200, 100));
        EXPECT_TRUE(expectBounds(child2, 0, 10, 10, 100));
        EXPECT_TRUE(expectInnerBounds(child2, 0, 0, 10, 90));
    }
}

static const char *START_END_PADDING_OVERRIDE_RTL = R"(
{
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "item": {
      "id": "cont",
      "layoutDirection": "RTL",
      "type": "Container",
      "direction": "row",
      "height": 200,
      "width": 500,
      "items": [
        {
          "id": "paddedFrame",
          "type": "Frame",
          "backgroundColor": "blue",
          "paddingRight": 10,
          "paddingEnd": 20,
          "item": {
            "id": "paddedFrameChild",
            "type": "Frame",
            "backgroundColor": "pink",
            "item": {
              "type": "Text",
              "text": "Test text"
            }
          }
        },
        {
          "id": "paddedFrame2",
          "type": "Frame",
          "backgroundColor": "blue",
          "paddingStart": 10,
          "paddingLeft": 20,
          "item": {
            "id": "paddedFrameChild2",
            "type": "Frame",
            "backgroundColor": "pink",
            "item": {
              "type": "Text",
              "text": "Test text"
            }
          }
        }
      ]
    }
  }
}
)";

/*
 * Test with RTL layout
 */
TEST_F(BuilderTest, BasicStartEndPaddingRTL)
{
    loadDocument(START_END_PADDING_OVERRIDE_RTL);

    auto frame = component->findComponentById("paddedFrame");
    auto frame2 = component->findComponentById("paddedFrame2");
    auto child = component->findComponentById("paddedFrameChild");
    auto child2 = component->findComponentById("paddedFrameChild2");

    EXPECT_TRUE(expectBounds(frame, 0, 380, 200, 500));
    EXPECT_TRUE(expectInnerBounds(frame, 0, 20, 200, 110));
    EXPECT_TRUE(expectBounds(child, 0, 20, 10, 110));
    EXPECT_TRUE(expectInnerBounds(child, 0, 0, 10, 90));
    EXPECT_TRUE(expectBounds(frame2, 0, 260, 200, 380));
    EXPECT_TRUE(expectInnerBounds(frame2, 0, 20, 200, 110));
    EXPECT_TRUE(expectBounds(child2, 0, 20, 10, 110));
    EXPECT_TRUE(expectInnerBounds(child2, 0, 0, 10, 90));
}

/*
 * Check padding works when layout direction is set dynamically
 */
TEST_F(BuilderTest, ComplexDynamicStartEndPaddingRTL)
{
    loadDocument(START_END_PADDING_OVERRIDE);

    auto cont = std::dynamic_pointer_cast<CoreComponent>(component->findComponentById("cont"));
    cont->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();

    auto frame = component->findComponentById("paddedFrame");
    auto frame2 = component->findComponentById("paddedFrame2");
    auto child = component->findComponentById("paddedFrameChild");
    auto child2 = component->findComponentById("paddedFrameChild2");

    EXPECT_TRUE(expectBounds(frame, 0, 380, 200, 500));
    EXPECT_TRUE(expectInnerBounds(frame, 0, 20, 200, 110));
    EXPECT_TRUE(expectBounds(child, 0, 20, 10, 110));
    EXPECT_TRUE(expectInnerBounds(child, 0, 0, 10, 90));
    EXPECT_TRUE(expectBounds(frame2, 0, 260, 200, 380));
    EXPECT_TRUE(expectInnerBounds(frame2, 0, 20, 200, 110));
    EXPECT_TRUE(expectBounds(child2, 0, 20, 10, 110));
    EXPECT_TRUE(expectInnerBounds(child2, 0, 0, 10, 90));
}

const char *POSITION_TYPE_TEST = R"apl(
{
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "items": {
      "type": "ScrollView",
      "width": "500",
      "height": "400",
      "item": {
        "id": "containerComp",
        "type": "Container",
        "width": "500",
        "height": "1000",
        "items": [
          {
            "id": "frameComp1",
            "type": "Frame",
            "width": "100",
            "height": "100",
            "backgroundColor": "blue"
          },
          {
            "id": "frameComp2",
            "type": "Frame",
            "width": "100",
            "height": "100",
            "backgroundColor": "red"
          }
        ]
      }
    }
  }
}
)apl";

/*
 * Check that changing the position type from relative, absolute and sticky works as expected and
 * verify that position insets can be unset using "auto"
 */
TEST_F(BuilderTest, PositionTypeRelativeToAbsolute)
{
    loadDocument(POSITION_TYPE_TEST);

    auto cont = std::dynamic_pointer_cast<CoreComponent>(component->findComponentById("frameComp1"));
    cont->setProperty(kPropertyRight, 0);

    EXPECT_TRUE(expectBounds(cont, 0, 0, 100, 100));

    cont->setProperty(kPropertyPosition, "absolute");
    root->clearPending(); // Force layout

    EXPECT_TRUE(expectBounds(cont, 0, 400, 100, 500));

    // verify setting a position inset to auto works.
    cont->setProperty(kPropertyRight, "auto");
    root->clearPending(); // Force layout

    EXPECT_TRUE(expectBounds(cont, 0, 0, 100, 100));

    // verify the NAN value passed to yoga hasn't broken anything
    cont->setProperty(kPropertyRight, 10);
    root->clearPending(); // Force layout

    EXPECT_TRUE(expectBounds(cont, 0, 390, 100, 490));

    cont->setProperty(kPropertyLeft, 10);
    root->clearPending(); // Force layout

    EXPECT_TRUE(expectBounds(cont, 0, 10, 100, 110));

    cont->setProperty(kPropertyPosition, "sticky");
    root->clearPending(); // Force layout

    EXPECT_TRUE(expectBounds(cont, 0, 0, 100, 100));

    cont->setProperty(kPropertyPosition, "absolute");
    root->clearPending(); // Force layout

    // unset the left inset so the element is inset from the right side
    cont->setProperty(kPropertyLeft, "auto");
    root->clearPending(); // Force layout

    EXPECT_TRUE(expectBounds(cont, 0, 390, 100, 490));
}

/*
 * Check that changing the position type from relative, absolute and sticky works as expected and
 * verify that start/end insets can be unset using "auto"
 */
TEST_F(BuilderTest, PositionTypeRelativeToAbsoluteStartEndInsets)
{
    loadDocument(POSITION_TYPE_TEST);

    auto cont = std::dynamic_pointer_cast<CoreComponent>(component->findComponentById("frameComp1"));
    auto containerComp = std::dynamic_pointer_cast<CoreComponent>(component->findComponentById("containerComp"));
    cont->setProperty(kPropertyStart, 10);
    cont->setProperty(kPropertyRight, 20);

    EXPECT_TRUE(expectBounds(cont, 0, 0, 100, 100));

    cont->setProperty(kPropertyPosition, "absolute");
    root->clearPending(); // Force layout

    EXPECT_TRUE(expectBounds(cont, 0, 10, 100, 110));

    // Switch the layout direction and verify start now overrides right and that the component
    // is offset from the right side now that left has been set back to "auto"
    containerComp->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending(); // Force layout

    EXPECT_TRUE(expectBounds(cont, 0, 390, 100, 490));

    // verify changing to/from position: sticky restores the same values
    cont->setProperty(kPropertyPosition, "sticky");
    root->clearPending(); // Force layout

    EXPECT_TRUE(expectBounds(cont, 0, 400, 100, 500));

    cont->setProperty(kPropertyPosition, "absolute");
    root->clearPending(); // Force layout

    EXPECT_TRUE(expectBounds(cont, 0, 390, 100, 490));
}

static const char *NULL_LAYOUT_NULL_POINTER = R"(
{
    "type": "APL",
    "version": "1.7",
    "mainTemplate":     4      }             { "items": {
            "type": "Container",
            "items": [
    {
                    "type": "Image",
                    "id": "myImage",
                    "source": [],
                    "onReady": {
                        "type": "Select",
  / ?                  {
commands": [
                            {
                                "when": "${success}",
                "type": "SetValue",
                "componentId": "textComp",
                                "prop
                                     rty": "text",
                                "value": "tango"
                            },
                {
                                "when": "${!success}",
                                "type": "SetValue",
                                "componentId": "textComp",
     	                          "property": "text",
                                "value": "bravo"
                            }         ]
                    }
                },
                {
                    "type": "Text",
                    "id": "textComi",
                    "text": "tiger"
                {
    "type": "APL",
    "vers
)";

/*
 * Test the template doesn't assert. Just check it finishes.
 */
TEST_F(BuilderTest, NullLayoutReturnsNullPointer)
{
    auto content = apl::Content::create(NULL_LAYOUT_NULL_POINTER);
    EXPECT_TRUE(content != nullptr);
    EXPECT_FALSE(apl::RootContext::create(apl::Metrics()
                                              .size(1280, 800)
                                              .dpi(160)
                                              .shape(apl::ScreenShape::ROUND),
                                          content));

}