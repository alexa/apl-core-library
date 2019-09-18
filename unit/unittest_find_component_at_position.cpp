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

#include <apl/component/imagecomponent.h>
#include "testeventloop.h"

using namespace apl;

class FindComponentAtPosition : public DocumentWrapper {};

static const char *BASIC =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Image\","
    "      \"width\": 100,"
    "      \"height\": 100"
    "    }"
    "  }"
    "}";

TEST_F(FindComponentAtPosition, Basic)
{
    loadDocument(BASIC);
    ASSERT_TRUE(component);

    ASSERT_EQ(component, component->findComponentAtPosition(Point(10, 10)));
    ASSERT_EQ(nullptr, component->findComponentAtPosition(Point(200, 200)));

    component->setProperty(kPropertyOpacity, 0.0);
    ASSERT_EQ(nullptr, component->findComponentAtPosition(Point(10, 10)));

    component->setProperty(kPropertyOpacity, 0.001);
    ASSERT_EQ(component, component->findComponentAtPosition(Point(10, 10)));
}

static const char *INVISIBLE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Image\","
    "      \"width\": 100,"
    "      \"height\": 100,"
    "      \"display\": \"invisible\""
    "    }"
    "  }"
    "}";

TEST_F(FindComponentAtPosition, Invisible)
{
    loadDocument(INVISIBLE);
    ASSERT_TRUE(component);

    ASSERT_EQ(nullptr, component->findComponentAtPosition(Point(10, 10)));
    ASSERT_EQ(nullptr, component->findComponentAtPosition(Point(200, 200)));
}

static const char *CONTAINER_OVERLAP =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"width\": 50,"
    "      \"height\": 50,"
    "      \"paddingTop\": 10,"
    "      \"paddingBottom\": 10,"
    "      \"paddingLeft\": 10,"
    "      \"paddingRight\": 10,"
    "      \"items\": ["
    "        {"
    "          \"type\": \"Image\","
    "          \"width\": 20,"
    "          \"height\": 20"
    "        },"
    "        {"
    "          \"type\": \"Text\","
    "          \"width\": 20,"
    "          \"height\": 20,"
    "          \"left\": 20,"
    "          \"top\": 20,"
    "          \"position\": \"absolute\""
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FindComponentAtPosition, ContainerOverlap)
{
    loadDocument(CONTAINER_OVERLAP);
    ASSERT_TRUE(component);

    ASSERT_EQ(2, component->getChildCount());
    auto image = component->getCoreChildAt(0);
    auto text = component->getCoreChildAt(1);
    ASSERT_TRUE(image);
    ASSERT_TRUE(text);

    ASSERT_EQ(nullptr, component->findComponentAtPosition(Point(-1, -1)));
    ASSERT_EQ(nullptr, component->findComponentAtPosition(Point(51, 51)));

    ASSERT_EQ(component, component->findComponentAtPosition(Point(0, 0)));
    ASSERT_EQ(image, component->findComponentAtPosition(Point(10, 10)));
    ASSERT_EQ(text, component->findComponentAtPosition(Point(20, 20)));
    ASSERT_EQ(text, component->findComponentAtPosition(Point(29, 29)));
    ASSERT_EQ(text, component->findComponentAtPosition(Point(30, 30)));
    ASSERT_EQ(text, component->findComponentAtPosition(Point(40, 40)));
    ASSERT_EQ(component, component->findComponentAtPosition(Point(50, 50)));

    text->setProperty(kPropertyOpacity, 0.0);
    ASSERT_EQ(component, component->findComponentAtPosition(Point(0, 0)));
    ASSERT_EQ(image, component->findComponentAtPosition(Point(10, 10)));
    ASSERT_EQ(image, component->findComponentAtPosition(Point(20, 20)));
    ASSERT_EQ(image, component->findComponentAtPosition(Point(29, 29)));
    ASSERT_EQ(image, component->findComponentAtPosition(Point(30, 30)));
    ASSERT_EQ(component, component->findComponentAtPosition(Point(40, 40)));
    ASSERT_EQ(component, component->findComponentAtPosition(Point(50, 50)));
}

static const char *SEQUENCE_WITH_PADDING =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Sequence\","
    "      \"width\": 100,"
    "      \"height\": 40,"
    "      \"paddingTop\": 10,"
    "      \"paddingBottom\": 10,"
    "      \"paddingLeft\": 10,"
    "      \"paddingRight\": 10,"
    "      \"items\": {"
    "        \"type\": \"Image\","
    "        \"width\": 50,"
    "        \"height\": 10"
    "      },"
    "      \"data\": ["
    "        0,"
    "        1,"
    "        2,"
    "        3,"
    "        4,"
    "        5"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FindComponentAtPosition, SequenceWithPadding)
{
    loadDocument(SEQUENCE_WITH_PADDING);
    ASSERT_TRUE(component);

    ASSERT_EQ(6, component->getChildCount());
    component->getChildAt(5)->ensureLayout(false);

    ASSERT_EQ(nullptr, component->findComponentAtPosition(Point(-1, -1)));
    ASSERT_EQ(nullptr, component->findComponentAtPosition(Point(101, 41)));

    // Left/right sides
    ASSERT_EQ(component, component->findComponentAtPosition(Point(5, 20)));
    ASSERT_EQ(component, component->findComponentAtPosition(Point(95, 20)));

    // Note that the bottom child is sticking out just barely into the visible region
    ASSERT_EQ(component, component->findComponentAtPosition(Point(50, 0)));
    ASSERT_EQ(component->getChildAt(0), component->findComponentAtPosition(Point(50, 10)));
    ASSERT_EQ(component->getChildAt(1), component->findComponentAtPosition(Point(50, 20)));
    ASSERT_EQ(component->getChildAt(2), component->findComponentAtPosition(Point(50, 30)));
    ASSERT_EQ(component->getChildAt(3), component->findComponentAtPosition(Point(50, 40)));

    // Scroll up
    component->update(kUpdateScrollPosition, 20);
    ASSERT_EQ(component->getChildAt(1), component->findComponentAtPosition(Point(50, 0)));
    ASSERT_EQ(component->getChildAt(2), component->findComponentAtPosition(Point(50, 10)));
    ASSERT_EQ(component->getChildAt(3), component->findComponentAtPosition(Point(50, 20)));
    ASSERT_EQ(component->getChildAt(4), component->findComponentAtPosition(Point(50, 30)));
    ASSERT_EQ(component->getChildAt(5), component->findComponentAtPosition(Point(50, 40)));

    // Maximum scroll (there are 6 children for a total child height of 60, plus 20 units
    // of padding in a container of height 40).
    component->update(kUpdateScrollPosition, 40);
    ASSERT_EQ(component->getChildAt(3), component->findComponentAtPosition(Point(50, 0)));
    ASSERT_EQ(component->getChildAt(4), component->findComponentAtPosition(Point(50, 10)));
    ASSERT_EQ(component->getChildAt(5), component->findComponentAtPosition(Point(50, 20)));
    ASSERT_EQ(component->getChildAt(5), component->findComponentAtPosition(Point(50, 30)));
    ASSERT_EQ(component, component->findComponentAtPosition(Point(50, 40)));
}

/**
 * TODO: The Pager component doesn't work correctly with padding values (there's a bug
 * open on that).  For now we will test the pager without padding.
 */
static const char * PAGER_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Pager\","
    "      \"width\": 100,"
    "      \"height\": 100,"
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"width\": \"100%\","
    "        \"height\": \"100%\""
    "      },"
    "      \"data\": ["
    "        0,"
    "        1,"
    "        2"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FindComponentAtPosition, Pager)
{
    loadDocument(PAGER_TEST);
    ASSERT_TRUE(component);

    ASSERT_EQ(3, component->getChildCount());

    ASSERT_EQ(nullptr, component->findComponentAtPosition(Point(-1, -1)));
    ASSERT_EQ(nullptr, component->findComponentAtPosition(Point(101, 101)));

    ASSERT_EQ(component->getChildAt(0), component->findComponentAtPosition(Point(50, 50)));

    component->update(kUpdatePagerPosition, 1);
    ASSERT_EQ(component->getChildAt(1), component->findComponentAtPosition(Point(50, 50)));
}

static const char * NESTED_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Frame\","
    "      \"paddingLeft\": 10,"
    "      \"paddingTop\": 10,"
    "      \"paddingRight\": 10,"
    "      \"paddingBottom\": 10,"
    "      \"width\": 100,"
    "      \"height\": 100,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"paddingLeft\": 10,"
    "        \"paddingTop\": 10,"
    "        \"paddingRight\": 10,"
    "        \"paddingBottom\": 10,"
    "        \"items\": {"
    "          \"type\": \"Image\","
    "          \"width\": 50,"
    "          \"height\": 50"
    "        }"
    "      }"
    "    }"
    "  }"
    "}";


TEST_F(FindComponentAtPosition, Nested)
{
    loadDocument(NESTED_TEST);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Rect(0, 0, 100, 100), component->getGlobalBounds()));

    ASSERT_EQ(1, component->getChildCount());
    auto innerFrame = component->getCoreChildAt(0);
    ASSERT_TRUE(IsEqual(Rect(10, 10, 70, 70), innerFrame->getGlobalBounds()));

    ASSERT_EQ(1, innerFrame->getChildCount());
    auto innerImage = innerFrame->getChildAt(0);
    ASSERT_TRUE(IsEqual(Rect(20, 20, 50, 50), innerImage->getGlobalBounds()));

    ASSERT_EQ(nullptr, component->findComponentAtPosition(Point(-1, -1)));
    ASSERT_EQ(nullptr, component->findComponentAtPosition(Point(101, 101)));

    ASSERT_EQ(component, component->findComponentAtPosition(Point(5, 5)));
    ASSERT_EQ(component, component->findComponentAtPosition(Point(20, 90)));
    ASSERT_EQ(innerFrame, component->findComponentAtPosition(Point(15, 15)));
    ASSERT_EQ(innerImage, component->findComponentAtPosition(Point(30, 30)));

    // Hide the innerFrame.  This should block access to the innerImage
    innerFrame->setProperty(kPropertyOpacity, 0);

    ASSERT_EQ(component, component->findComponentAtPosition(Point(5, 5)));
    ASSERT_EQ(component, component->findComponentAtPosition(Point(20, 90)));
    ASSERT_EQ(component, component->findComponentAtPosition(Point(15, 15)));
    ASSERT_EQ(component, component->findComponentAtPosition(Point(30, 30)));
}

static const char *NON_LAID_OUT_SEQUENCE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Sequence\","
    "      \"width\": 100,"
    "      \"height\": 100,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"width\": 100,"
    "        \"height\": 40"
    "      },"
    "      \"data\": ["
    "        \"a\","
    "        \"b\","
    "        \"c\","
    "        \"d\""
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FindComponentAtPosition, NonLaidOutSequence)
{
    loadDocument(NON_LAID_OUT_SEQUENCE);
    ASSERT_TRUE(component);
    ASSERT_EQ(4, component->getChildCount());

    // Deliberately don't lay out the children - the top-level sequence is the only visible object
    ASSERT_EQ(nullptr, component->findComponentAtPosition(Point(-1, -1)));
    ASSERT_EQ(nullptr, component->findComponentAtPosition(Point(101, 101)));
    ASSERT_EQ(component, component->findComponentAtPosition(Point(5, 5)));
    ASSERT_EQ(component, component->findComponentAtPosition(Point(5, 45)));
    ASSERT_EQ(component, component->findComponentAtPosition(Point(5, 85)));

    // Now force a few child layouts
    component->getChildAt(1)->ensureLayout(false);
    ASSERT_EQ(component->getChildAt(0), component->findComponentAtPosition(Point(5, 5)));
    ASSERT_EQ(component->getChildAt(1), component->findComponentAtPosition(Point(5, 45)));
    ASSERT_EQ(component, component->findComponentAtPosition(Point(5, 85)));

    // Finish laying out all children
    component->getChildAt(3)->ensureLayout(false);
    ASSERT_EQ(component->getChildAt(0), component->findComponentAtPosition(Point(5, 5)));
    ASSERT_EQ(component->getChildAt(1), component->findComponentAtPosition(Point(5, 45)));
    ASSERT_EQ(component->getChildAt(2), component->findComponentAtPosition(Point(5, 85)));
}
