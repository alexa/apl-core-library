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

#include "gtest/gtest.h"

#include "apl/engine/evaluate.h"
#include "apl/engine/builder.h"
#include "apl/component/component.h"

#include "../testeventloop.h"

using namespace apl;

class BoundsTest : public DocumentWrapper {};

static const char *SCROLL_VIEW =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"parameters\": [],"
    "    \"item\": {"
    "      \"type\": \"ScrollView\","
    "      \"width\": \"100vw\","
    "      \"height\": \"100vh\","
    "      \"items\": {"
    "        \"type\": \"Container\","
    "        \"items\": {"
    "          \"type\": \"Frame\","
    "          \"width\": 200,"
    "          \"height\": 200"
    "        },"
    "        \"data\": ["
    "          1,"
    "          2,"
    "          3,"
    "          4,"
    "          5,"
    "          6,"
    "          7,"
    "          8,"
    "          9,"
    "          10"
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(BoundsTest, ScrollView)
{
    loadDocument(SCROLL_VIEW);

    ASSERT_EQ(Rect(0,0,1024,800), component->getCalculated(kPropertyBounds).getRect());
    auto frame = component->getChildAt(0);
    ASSERT_EQ(10, frame->getChildCount());

    for (auto i = 0 ; i < frame->getChildCount() ; i++) {
        auto child = frame->getChildAt(i);
        ASSERT_EQ(Rect(0, 200*i, 200, 200), child->getCalculated(kPropertyBounds).getRect());
        Rect rect;
        ASSERT_TRUE(child->getBoundsInParent(nullptr, rect));
        ASSERT_EQ(Rect(0, 200*i, 200, 200), rect);
    }

    component->update(kUpdateScrollPosition, 100);
    for (auto i = 0 ; i < frame->getChildCount() ; i++) {
        auto child = frame->getChildAt(i);
        ASSERT_EQ(Rect(0, 200*i, 200, 200), child->getCalculated(kPropertyBounds).getRect());
        Rect rect;
        ASSERT_TRUE(child->getBoundsInParent(nullptr, rect));
        ASSERT_EQ(Rect(0, 200*i - 100, 200, 200), rect);
    }

    component->update(kUpdateScrollPosition, 500);
    for (auto i = 0 ; i < frame->getChildCount() ; i++) {
        auto child = frame->getChildAt(i);
        ASSERT_EQ(Rect(0, 200*i, 200, 200), child->getCalculated(kPropertyBounds).getRect());
        Rect rect;
        ASSERT_TRUE(child->getBoundsInParent(nullptr, rect));
        ASSERT_EQ(Rect(0, 200*i - 500, 200, 200), rect);
    }
}

static const char *VERTICAL_SEQUENCE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"parameters\": [],"
    "    \"item\": {"
    "      \"type\": \"Sequence\","
    "      \"scrollDirection\": \"vertical\","
    "      \"width\": 200,"
    "      \"height\": 500,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": 200,"
    "        \"height\": 200"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4,"
    "        5"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(BoundsTest, VerticalSequence)
{
    loadDocument(VERTICAL_SEQUENCE);
    advanceTime(10);

    ASSERT_EQ(Rect(0,0,200,500), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(5, component->getChildCount());

    for (auto i = 0 ; i < component->getChildCount() ; i++) {
        auto child = component->getChildAt(i);
        ASSERT_EQ(Rect(0, 200*i, 200, 200), child->getCalculated(kPropertyBounds).getRect());
        Rect rect;
        ASSERT_TRUE(child->getBoundsInParent(nullptr, rect));
        ASSERT_EQ(Rect(0, 200*i, 200, 200), rect);
    }

    component->update(kUpdateScrollPosition, 100);
    for (auto i = 0 ; i < component->getChildCount() ; i++) {
        auto child = component->getChildAt(i);
        ASSERT_EQ(Rect(0, 200*i, 200, 200), child->getCalculated(kPropertyBounds).getRect());
        Rect rect;
        ASSERT_TRUE(child->getBoundsInParent(nullptr, rect));
        ASSERT_EQ(Rect(0, 200*i - 100, 200, 200), rect);
    }

    component->update(kUpdateScrollPosition, 500);
    for (auto i = 0 ; i < component->getChildCount() ; i++) {
        auto child = component->getChildAt(i);
        ASSERT_EQ(Rect(0, 200*i, 200, 200), child->getCalculated(kPropertyBounds).getRect());
        Rect rect;
        ASSERT_TRUE(child->getBoundsInParent(nullptr, rect));
        ASSERT_EQ(Rect(0, 200*i - 500, 200, 200), rect);
    }

    // Verify that we can't set out of bounds position
    component->update(kUpdateScrollPosition, 1000);
    ASSERT_EQ(500, component->getCalculated(kPropertyScrollPosition).asNumber());
}

static const char *HORIZONTAL_SEQUENCE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"parameters\": [],"
    "    \"item\": {"
    "      \"type\": \"Sequence\","
    "      \"scrollDirection\": \"horizontal\","
    "      \"width\": 500,"
    "      \"height\": 200,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": 200,"
    "        \"height\": 200"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4,"
    "        5"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(BoundsTest, HorizontalSequence)
{
    loadDocument(HORIZONTAL_SEQUENCE);
    advanceTime(10);

    ASSERT_EQ(Rect(0,0,500,200), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(5, component->getChildCount());

    for (auto i = 0 ; i < component->getChildCount() ; i++) {
        auto child = component->getChildAt(i);
        ASSERT_EQ(Rect(200*i, 0, 200, 200), child->getCalculated(kPropertyBounds).getRect());
        Rect rect;
        ASSERT_TRUE(child->getBoundsInParent(nullptr, rect));
        ASSERT_EQ(Rect(200*i, 0, 200, 200), rect);
    }

    component->update(kUpdateScrollPosition, 100);
    for (auto i = 0 ; i < component->getChildCount() ; i++) {
        auto child = component->getChildAt(i);
        ASSERT_EQ(Rect(200*i, 0, 200, 200), child->getCalculated(kPropertyBounds).getRect());
        Rect rect;
        ASSERT_TRUE(child->getBoundsInParent(nullptr, rect));
        ASSERT_EQ(Rect(200*i - 100, 0, 200, 200), rect);
    }

    component->update(kUpdateScrollPosition, 500);
    for (auto i = 0 ; i < component->getChildCount() ; i++) {
        auto child = component->getChildAt(i);
        ASSERT_EQ(Rect(200*i, 0, 200, 200), child->getCalculated(kPropertyBounds).getRect());
        Rect rect;
        ASSERT_TRUE(child->getBoundsInParent(nullptr, rect));
        ASSERT_EQ(Rect(200*i - 500, 0, 200, 200), rect);
    }

    // Verify that we can't set out of bounds position
    component->update(kUpdateScrollPosition, 1000);
    ASSERT_EQ(500, component->getCalculated(kPropertyScrollPosition).asNumber());
}

TEST_F(BoundsTest, HorizontalSequenceRTL)
{
    loadDocument(HORIZONTAL_SEQUENCE);
    component->setProperty(kPropertyLayoutDirectionAssigned, "RTL");
    root->clearPending();

    ASSERT_EQ(Rect(0,0,500,200), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(5, component->getChildCount());

    for (auto i = 0 ; i < component->getChildCount() ; i++) {
        auto child = component->getChildAt(i);
        ASSERT_EQ(Rect(300 - 200*i, 0, 200, 200), child->getCalculated(kPropertyBounds).getRect());
        Rect rect;
        ASSERT_TRUE(child->getBoundsInParent(nullptr, rect));
        ASSERT_EQ(Rect(300 - 200*i, 0, 200, 200), rect);
    }

    component->update(kUpdateScrollPosition, -100);
    for (auto i = 0 ; i < component->getChildCount() ; i++) {
        auto child = component->getChildAt(i);
        ASSERT_EQ(Rect(300 - 200*i, 0, 200, 200), child->getCalculated(kPropertyBounds).getRect());
        Rect rect;
        ASSERT_TRUE(child->getBoundsInParent(nullptr, rect));
        ASSERT_EQ(Rect(300 - 200*i + 100, 0, 200, 200), rect);
    }

    component->update(kUpdateScrollPosition, -500);
    for (auto i = 0 ; i < component->getChildCount() ; i++) {
        auto child = component->getChildAt(i);
        ASSERT_EQ(Rect(300 - 200*i, 0, 200, 200), child->getCalculated(kPropertyBounds).getRect());
        Rect rect;
        ASSERT_TRUE(child->getBoundsInParent(nullptr, rect));
        ASSERT_EQ(Rect(300 - 200*i + 500, 0, 200, 200), rect);
    }

    // Verify that we can't set out of bounds position
    component->update(kUpdateScrollPosition, -1000);
    ASSERT_EQ(-500, component->getCalculated(kPropertyScrollPosition).asNumber());
}

static const char *CHILD_IN_PARENT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"parameters\": [],"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Text\","
    "          \"width\": \"100%\","
    "          \"height\": \"150dp\","
    "          \"text\": \"Title goes here\""
    "        },"
    "        {"
    "          \"type\": \"Sequence\","
    "          \"scrollDirection\": \"vertical\","
    "          \"width\": \"100%\","
    "          \"grow\": 1,"
    "          \"items\": {"
    "            \"type\": \"Container\","
    "            \"width\": \"100%\","
    "            \"direction\": \"row\","
    "            \"bind\": ["
    "              {"
    "                \"name\": \"childIndex\","
    "                \"value\": \"${index}\""
    "              }"
    "            ],"
    "            \"items\": ["
    "              {"
    "                \"type\": \"Text\","
    "                \"text\": \"${childIndex + 1}\","
    "                \"width\": \"100dp\","
    "                \"height\": \"100dp\""
    "              },"
    "              {"
    "                \"type\": \"Text\","
    "                \"text\": \"${data}\","
    "                \"grow\": 1,"
    "                \"width\": \"100dp\","
    "                \"height\": \"100dp\""
    "              }"
    "            ]"
    "          },"
    "          \"data\": ["
    "            \"Frog\","
    "            \"Puppy\","
    "            \"Turtle\","
    "            \"Chili\","
    "            \"Dandelion\","
    "            \"Couch\","
    "            \"Duck\","
    "            \"Snitch\","
    "            \"Tweedledum\""
    "          ]"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";


TEST_F(BoundsTest, ChildInParent)
{
    loadDocument(CHILD_IN_PARENT);
    advanceTime(10);

    auto sequence = component->getChildAt(1);
    ASSERT_EQ(9, sequence->getChildCount());

    ASSERT_TRUE(IsEqual(Rect(0, 150, 1024, 650), sequence->getCalculated(kPropertyBounds)));

    for (auto i = 0 ; i < sequence->getChildCount() ; i++) {
        Rect rect;
        auto child = sequence->getChildAt(i);
        auto number = child->getChildAt(0);
        auto label = child->getChildAt(1);

        // Position w.r.t. the holding container
        ASSERT_EQ(Rect(0, 0, 100, 100), number->getCalculated(kPropertyBounds).getRect());
        ASSERT_EQ(Rect(100, 0, 924, 100), label->getCalculated(kPropertyBounds).getRect());

        // Global position
        ASSERT_EQ(Rect(0, 150 + i * 100, 100, 100), number->getGlobalBounds());
        ASSERT_EQ(Rect(100, 150 + i * 100, 924, 100), label->getGlobalBounds());

        // Position w.r.t. the sequence
        ASSERT_TRUE(number->getBoundsInParent(sequence, rect));
        ASSERT_EQ(Rect(0, i * 100, 100, 100), rect);
        ASSERT_TRUE(label->getBoundsInParent(sequence, rect));
        ASSERT_EQ(Rect(100, i * 100, 924, 100), rect);
    }

    // Now scroll
    sequence->update(kUpdateScrollPosition, 25);

    // Check new positions
    for (auto i = 0 ; i < sequence->getChildCount() ; i++) {
        Rect rect;
        auto child = sequence->getChildAt(i);
        auto number = child->getChildAt(0);
        auto label = child->getChildAt(1);

        // Position w.r.t. the holding container doesn't change
        ASSERT_EQ(Rect(0, 0, 100, 100), number->getCalculated(kPropertyBounds).getRect());
        ASSERT_EQ(Rect(100, 0, 924, 100), label->getCalculated(kPropertyBounds).getRect());

        // Global position moves upwards by 25
        ASSERT_EQ(Rect(0, 125 + i * 100, 100, 100), number->getGlobalBounds());
        ASSERT_EQ(Rect(100, 125 + i * 100, 924, 100), label->getGlobalBounds());

        // Position w.r.t. the sequence doesn't change
        ASSERT_TRUE(number->getBoundsInParent(sequence, rect));
        ASSERT_EQ(Rect(0, i * 100, 100, 100), rect);
        ASSERT_TRUE(label->getBoundsInParent(sequence, rect));
        ASSERT_EQ(Rect(100, i * 100, 924, 100), rect);
    }

    // Sanity test some binding logic
    auto context = Context::createTestContext(Metrics(), makeDefaultSession());
    ASSERT_EQ(StyledText::create(*context, "3"), sequence->getChildAt(2)->getChildAt(0)->getCalculated(kPropertyText));
    ASSERT_EQ(StyledText::create(*context, "Turtle"), sequence->getChildAt(2)->getChildAt(1)->getCalculated(kPropertyText));
}

static const char *NESTED_CHILD = "{"
                                  "  \"type\": \"APL\","
                                  "  \"version\": \"1.1\","
                                  "  \"mainTemplate\": {"
                                  "    \"item\": {"
                                  "      \"type\": \"Container\","
                                  "      \"id\": \"ctr\","
                                  "      \"width\": \"1000dp\","
                                  "      \"height\": \"500dp\","
                                  "      \"items\": ["
                                  "        {"
                                  "          \"type\": \"Text\","
                                  "          \"id\": \"text1\","
                                  "          \"height\": \"100dp\","
                                  "          \"width\": \"100dp\","
                                  "          \"text\": \"Background.\""
                                  "        },"
                                  "        {"
                                  "          \"type\": \"Container\","
                                  "          \"id\": \"ctr2\","
                                  "          \"height\": \"100dp\","
                                  "          \"width\": \"100dp\","
                                  "          \"items\":"
                                  "          ["
                                  "            {"
                                  "              \"type\": \"Text\","
                                  "              \"id\": \"text2\","
                                  "              \"height\": \"50dp\","
                                  "              \"width\": \"50dp\","
                                  "              \"text\": \"Foreground.\""
                                  "            }"
                                  "          ]"
                                  "        }"
                                  "      ]"
                                  "    }"
                                  "  }"
                                  "}";

TEST_F(BoundsTest, NestedChild)
{
    loadDocument(NESTED_CHILD);

    ASSERT_EQ(kComponentTypeContainer, component->getType());
    ASSERT_EQ(Rect(0, 0, 1000, 500), component->getGlobalBounds());

    auto text1 = component->getCoreChildAt(0);
    ASSERT_EQ(kComponentTypeText, text1->getType());
    ASSERT_EQ(Rect(0, 0, 100, 100), text1->getGlobalBounds());

    auto ctr2 = component->getCoreChildAt(1);
    ASSERT_EQ(kComponentTypeContainer, ctr2->getType());
    ASSERT_EQ(Rect(0, 100, 100, 100), ctr2->getGlobalBounds());

    auto text2 = ctr2->getCoreChildAt(0);
    ASSERT_EQ(kComponentTypeText, text2->getType());
    ASSERT_EQ(Rect(0, 0, 50, 50), text2->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 100, 50, 50), text2->getGlobalBounds());
}



static const char *ABSOLUTE_POSITIONING = "{"
                                          "  \"type\": \"APL\","
                                          "  \"version\": \"1.1\","
                                          "  \"mainTemplate\": {"
                                          "    \"item\": {"
                                          "      \"type\": \"Container\","
                                          "      \"id\": \"ctr\","
                                          "      \"width\": \"1000dp\","
                                          "      \"height\": \"500dp\","
                                          "      \"items\": ["
                                          "        {"
                                          "          \"type\": \"Text\","
                                          "          \"id\": \"text1\","
                                          "          \"height\": \"100dp\","
                                          "          \"width\": \"100dp\","
                                          "          \"position\": \"absolute\","
                                          "          \"left\": \"100dp\","
                                          "          \"top\": \"100dp\","
                                          "          \"text\": \"Background.\""
                                          "        },"
                                          "        {"
                                          "          \"type\": \"Container\","
                                          "          \"id\": \"ctr2\","
                                          "          \"height\": \"100dp\","
                                          "          \"width\": \"100dp\","
                                          "          \"position\": \"absolute\","
                                          "          \"right\": \"100dp\","
                                          "          \"bottom\": \"100dp\","
                                          "          \"items\":"
                                          "          ["
                                          "            {"
                                          "              \"type\": \"Text\","
                                          "              \"id\": \"text2\","
                                          "              \"height\": \"50dp\","
                                          "              \"width\": \"50dp\","
                                          "              \"position\": \"absolute\","
                                          "              \"left\": \"10dp\","
                                          "              \"bottom\": \"10dp\","
                                          "              \"text\": \"Foreground.\""
                                          "            }"
                                          "          ]"
                                          "        }"
                                          "      ]"
                                          "    }"
                                          "  }"
                                          "}";

TEST_F(BoundsTest, AbsolutePositioning)
{
    loadDocument(ABSOLUTE_POSITIONING);

    ASSERT_EQ(kComponentTypeContainer, component->getType());
    ASSERT_EQ(Rect(0, 0, 1000, 500), component->getGlobalBounds());

    auto text1 = component->getCoreChildAt(0);
    ASSERT_EQ(kComponentTypeText, text1->getType());
    ASSERT_EQ(Rect(100, 100, 100, 100), text1->getGlobalBounds());

    auto ctr2 = component->getCoreChildAt(1);
    ASSERT_EQ(kComponentTypeContainer, ctr2->getType());
    ASSERT_EQ(Rect(800, 300, 100, 100), ctr2->getGlobalBounds());

    auto text2 = ctr2->getCoreChildAt(0);
    ASSERT_EQ(kComponentTypeText, text2->getType());
    ASSERT_EQ(Rect(10, 40, 50, 50), text2->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(810, 340, 50, 50), text2->getGlobalBounds());
}
