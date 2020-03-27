/**
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "testeventloop.h"

using namespace apl;

class BuilderTestSequence : public DocumentWrapper {};

const char *SIMPLE_SEQUENCE = "{"
                              "  \"type\": \"APL\","
                              "  \"version\": \"1.0\","
                              "  \"mainTemplate\": {"
                              "    \"item\": {"
                              "      \"type\": \"Sequence\","
                              "      \"height\": 100,"
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

TEST_F(BuilderTestSequence, Simple)
{
    loadDocument(SIMPLE_SEQUENCE);

    auto map = component->getCalculated();
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    // Standard properties
    ASSERT_EQ("", component->getCalculated(kPropertyAccessibilityLabel).getString());
    ASSERT_EQ(Object::FALSE_OBJECT(), component->getCalculated(kPropertyDisabled));
    ASSERT_EQ(Object(Dimension(100)), component->getCalculated(kPropertyHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxWidth));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyMinHeight));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyMinWidth));
    ASSERT_EQ(1.0, component->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyPaddingBottom));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyPaddingLeft));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyPaddingRight));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyPaddingTop));
    ASSERT_EQ(Object(kSnapNone), component->getCalculated(kPropertySnap));
    ASSERT_EQ(Object(1.0), component->getCalculated(kPropertyFastScrollScale));
    ASSERT_EQ(Object(Dimension()), component->getCalculated(kPropertyWidth));
    ASSERT_EQ(Object::TRUE_OBJECT(), component->getCalculated(kPropertyLaidOut));

    // Sequence properties
    ASSERT_EQ(kScrollDirectionVertical, component->getCalculated(kPropertyScrollDirection).getInteger());
    ASSERT_EQ(false, component->getCalculated(kPropertyNumbered).getBoolean());

    // Children
    ASSERT_EQ(2, component->getChildCount());
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 1), true));

    auto scrollPosition = component->getCalculated(kPropertyScrollPosition);
    ASSERT_TRUE(scrollPosition.isDimension());
    ASSERT_EQ(0, scrollPosition.asNumber());

    component->release();
}

const char *EMPTY_SEQUENCE = "{"
                             "  \"type\": \"APL\","
                             "  \"version\": \"1.0\","
                             "  \"mainTemplate\": {"
                             "    \"item\": {"
                             "      \"type\": \"Sequence\","
                             "      \"height\": 100"
                             "    }"
                             "  }"
                             "}";

TEST_F(BuilderTestSequence, Empty)
{
    loadDocument(EMPTY_SEQUENCE);

    auto map = component->getCalculated();
    ASSERT_EQ(kComponentTypeSequence, component->getType());

    // Standard properties
    ASSERT_EQ("", component->getCalculated(kPropertyAccessibilityLabel).getString());
    ASSERT_EQ(Object::FALSE_OBJECT(), component->getCalculated(kPropertyDisabled));
    ASSERT_EQ(Object(Dimension(100)), component->getCalculated(kPropertyHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxHeight));
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyMaxWidth));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyMinHeight));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyMinWidth));
    ASSERT_EQ(1.0, component->getCalculated(kPropertyOpacity).getDouble());
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyPaddingBottom));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyPaddingLeft));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyPaddingRight));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyPaddingTop));
    ASSERT_EQ(Object(Dimension()), component->getCalculated(kPropertyWidth));
    ASSERT_EQ(Object::TRUE_OBJECT(), component->getCalculated(kPropertyLaidOut));

    // Sequence properties
    ASSERT_EQ(kScrollDirectionVertical, component->getCalculated(kPropertyScrollDirection).getInteger());
    ASSERT_EQ(false, component->getCalculated(kPropertyNumbered).getBoolean());

    // Children
    ASSERT_EQ(0, component->getChildCount());

    component->release();
}

const char *CHILDREN_TEST = "{"
                            "  \"type\": \"APL\","
                            "  \"version\": \"1.0\","
                            "  \"mainTemplate\": {"
                            "    \"item\": {"
                            "      \"type\": \"Sequence\","
                            "      \"scrollDirection\": \"horizontal\","
                            "      \"snap\": \"center\","
                            "      \"-fastScrollScale\": 0.5,"
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
                            "          \"numbering\": \"skip\","
                            "          \"spacing\": 20"
                            "        },"
                            "        {"
                            "          \"type\": \"Text\","
                            "          \"text\": \"C ${index}-${ordinal}-${length}\""
                            "        }"
                            "      ]"
                            "    }"
                            "  }"
                            "}";

TEST_F(BuilderTestSequence, Children)
{
    loadDocument(CHILDREN_TEST);
    auto map = component->getCalculated();

    ASSERT_EQ(kComponentTypeSequence, component->getType());
    ASSERT_EQ(kScrollDirectionHorizontal, component->getCalculated(kPropertyScrollDirection).getInteger());
    ASSERT_EQ(kSnapCenter, component->getCalculated(kPropertySnap).getInteger());
    ASSERT_EQ(0.5, component->getCalculated(kPropertyFastScrollScale).getDouble());
    ASSERT_TRUE(IsEqual(Dimension(100), component->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Dimension(), component->getCalculated(kPropertyHeight)));

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 3), true));

    auto child = component->getChildAt(0)->getCalculated();
    ASSERT_EQ(Object("C 0-1-5"), child.get(kPropertyText).asString());
    ASSERT_EQ(Object(Dimension(0)), child.get(kPropertySpacing));

    child = component->getChildAt(1)->getCalculated();
    ASSERT_EQ(Object("A 1-2-5"), child.get(kPropertyText).asString());
    ASSERT_EQ(Object(Dimension(0)), child.get(kPropertySpacing));

    child = component->getChildAt(2)->getCalculated();
    ASSERT_EQ(Object("C 2-1-5"), child.get(kPropertyText).asString());
    ASSERT_EQ(Object(Dimension(0)), child.get(kPropertySpacing));

    child = component->getChildAt(3)->getCalculated();
    ASSERT_EQ(Object("B 3-2-5"), child.get(kPropertyText).asString());
    ASSERT_EQ(Object(Dimension(20)), child.get(kPropertySpacing));

    child = component->getChildAt(4)->getCalculated();
    ASSERT_EQ(Object("C 4-2-5"), child.get(kPropertyText).asString());
    ASSERT_EQ(Object(Dimension(0)), child.get(kPropertySpacing));

    component->release();
}

const char *LAYOUT_CACHE_TEST = "{"
                                "  \"type\": \"APL\","
                                "  \"version\": \"1.0\","
                                "  \"mainTemplate\": {"
                                "    \"item\": {"
                                "      \"type\": \"Sequence\","
                                "      \"height\": 100,"
                                "      \"width\": \"auto\","
                                "      \"data\": ["
                                "        \"One\","
                                "        \"Two\","
                                "        \"Three\","
                                "        \"Four\","
                                "        \"Five\""
                                "      ],"
                                "      \"items\": ["
                                "        {"
                                "          \"type\": \"Text\","
                                "          \"height\": 50,"
                                "          \"text\": \"${data}\""
                                "        }"
                                "      ]"
                                "    }"
                                "  }"
                                "}";

TEST_F(BuilderTestSequence, LayoutCache)
{
    loadDocument(LAYOUT_CACHE_TEST);
    auto map = component->getCalculated();

    ASSERT_EQ(kComponentTypeSequence, component->getType());

    ASSERT_EQ(5, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 3), true));
    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(4, 4), false));

    component->release();
}
