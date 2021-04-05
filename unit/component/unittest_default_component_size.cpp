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

#include "../testeventloop.h"

using namespace apl;

class DefaultComponentTest : public DocumentWrapper {};

/*************** Container ***************/

static const char *DEFAULT_CONTAINER_SIZE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Frame\","
    "      \"width\": 1000,"
    "      \"height\": 1000,"
    "      \"item\": {"
    "        \"type\": \"Container\""
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(DefaultComponentTest, Container)
{
    loadDocument(DEFAULT_CONTAINER_SIZE);
    ASSERT_TRUE(component);

    auto container = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(), container->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(), container->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,0,0), container->getCalculated(kPropertyBounds)));
}

TEST_F(DefaultComponentTest, ContainerOverride)
{
    config->defaultComponentSize(kComponentTypeContainer, Dimension(30), Dimension(40));
    loadDocument(DEFAULT_CONTAINER_SIZE);
    ASSERT_TRUE(component);

    auto container = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(40), container->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(30), container->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,30,40), container->getCalculated(kPropertyBounds)));
}

static const char *DEFAULT_CONTAINER_WITH_CHILD_SIZE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Frame\","
    "      \"width\": 1000,"
    "      \"height\": 1000,"
    "      \"item\": {"
    "        \"type\": \"Container\","
    "        \"item\": {"
    "          \"type\": \"Text\","
    "          \"text\": \"Hello\""
    "        }"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(DefaultComponentTest, ContainerWithChild)
{
    loadDocument(DEFAULT_CONTAINER_WITH_CHILD_SIZE);
    ASSERT_TRUE(component);

    auto container = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(), container->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(), container->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,50,10), container->getCalculated(kPropertyBounds)));
}

/*************** Frame ***************/

static const char *DEFAULT_FRAME_SIZE =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"Frame\","
        "      \"width\": 1000,"
        "      \"height\": 1000,"
        "      \"item\": {"
        "        \"type\": \"Frame\""
        "      }"
        "    }"
        "  }"
        "}";

TEST_F(DefaultComponentTest, Frame)
{
    loadDocument(DEFAULT_FRAME_SIZE);
    ASSERT_TRUE(component);

    auto frame = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(), frame->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(), frame->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,0,0), frame->getCalculated(kPropertyBounds)));
}

TEST_F(DefaultComponentTest, FrameOverride)
{
    config->defaultComponentSize(kComponentTypeFrame, 55, 66);
    loadDocument(DEFAULT_FRAME_SIZE);
    ASSERT_TRUE(component);

    auto frame = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(66), frame->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(55), frame->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,55,66), frame->getCalculated(kPropertyBounds)));
}

static const char *DEFAULT_FRAME_WITH_CHILD_SIZE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Frame\","
    "      \"width\": 1000,"
    "      \"height\": 1000,"
    "      \"item\": {"
    "        \"type\": \"Frame\","
    "        \"item\": {"
    "          \"type\": \"Text\","
    "          \"text\": \"Puppy!\""
    "        }"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(DefaultComponentTest, FrameWithChild)
{
    loadDocument(DEFAULT_FRAME_WITH_CHILD_SIZE);
    ASSERT_TRUE(component);

    auto frame = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(), frame->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(), frame->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,60,10), frame->getCalculated(kPropertyBounds)));
}

/*************** Image ***************/

static const char *DEFAULT_IMAGE_SIZE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Frame\","
    "      \"width\": 1000,"
    "      \"height\": 1000,"
    "      \"item\": {"
    "        \"type\": \"Image\""
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(DefaultComponentTest, Image)
{
    loadDocument(DEFAULT_IMAGE_SIZE);
    ASSERT_TRUE(component);

    auto image = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(100), image->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(100), image->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), image->getCalculated(kPropertyBounds)));
}

TEST_F(DefaultComponentTest, ImageOverride)
{
    config->defaultComponentSize(kComponentTypeImage, 22, 33);

    loadDocument(DEFAULT_IMAGE_SIZE);
    ASSERT_TRUE(component);

    auto image = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(33), image->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(22), image->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,22,33), image->getCalculated(kPropertyBounds)));
}

/*************** Pager ***************/

static const char *DEFAULT_PAGER_SIZE =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"Frame\","
        "      \"width\": 1000,"
        "      \"height\": 1000,"
        "      \"item\": {"
        "        \"type\": \"Pager\""
        "      }"
        "    }"
        "  }"
        "}";

TEST_F(DefaultComponentTest, Pager)
{
    loadDocument(DEFAULT_PAGER_SIZE);
    ASSERT_TRUE(component);

    auto pager = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(100), pager->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(100), pager->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), pager->getCalculated(kPropertyBounds)));
}

TEST_F(DefaultComponentTest, PagerOverride)
{
    config->defaultComponentSize(kComponentTypePager, 111, 222);

    loadDocument(DEFAULT_PAGER_SIZE);
    ASSERT_TRUE(component);

    auto pager = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(222), pager->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(111), pager->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,111,222), pager->getCalculated(kPropertyBounds)));
}

static const char *DEFAULT_PAGER_WITH_CHILD_SIZE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Frame\","
    "      \"width\": 1000,"
    "      \"height\": 1000,"
    "      \"item\": {"
    "        \"type\": \"Pager\","
    "        \"item\": {"
    "          \"type\": \"Text\""
    "        }"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(DefaultComponentTest, PagerWithChild)
{
    loadDocument(DEFAULT_PAGER_WITH_CHILD_SIZE);
    ASSERT_TRUE(component);

    auto pager = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(100), pager->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(100), pager->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), pager->getCalculated(kPropertyBounds)));
}

/*************** ScrollView ***************/

static const char *DEFAULT_SCROLL_VIEW_SIZE =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"Frame\","
        "      \"width\": 1000,"
        "      \"height\": 1000,"
        "      \"item\": {"
        "        \"type\": \"ScrollView\""
        "      }"
        "    }"
        "  }"
        "}";

TEST_F(DefaultComponentTest, ScrollView)
{
    loadDocument(DEFAULT_SCROLL_VIEW_SIZE);
    ASSERT_TRUE(component);

    auto scrollView = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(100), scrollView->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(), scrollView->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,0,100), scrollView->getCalculated(kPropertyBounds)));
}

TEST_F(DefaultComponentTest, ScrollViewOverride)
{
    config->defaultComponentSize(kComponentTypeScrollView, 99, 400);
    loadDocument(DEFAULT_SCROLL_VIEW_SIZE);
    ASSERT_TRUE(component);

    auto scrollView = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(400), scrollView->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(99), scrollView->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,99,400), scrollView->getCalculated(kPropertyBounds)));
}

static const char *DEFAULT_SCROLL_VIEW_WITH_CHILD_SIZE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Frame\","
    "      \"width\": 1000,"
    "      \"height\": 1000,"
    "      \"item\": {"
    "        \"type\": \"ScrollView\","
    "        \"item\": {"
    "          \"type\": \"Text\","
    "          \"text\": \"test\""
    "        }"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(DefaultComponentTest, ScrollViewWithChild)
{
    loadDocument(DEFAULT_SCROLL_VIEW_WITH_CHILD_SIZE);
    ASSERT_TRUE(component);

    auto scrollView = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(100), scrollView->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(), scrollView->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,40,100), scrollView->getCalculated(kPropertyBounds)));
}

/*************** Sequence: Vertical ***************/

static const char *DEFAULT_SEQUENCE_VERTICAL_SIZE =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"Frame\","
        "      \"width\": 1000,"
        "      \"height\": 1000,"
        "      \"item\": {"
        "        \"type\": \"Sequence\","
        "        \"scrollDirection\": \"vertical\""
        "      }"
        "    }"
        "  }"
        "}";

TEST_F(DefaultComponentTest, SequenceVertical)
{
    loadDocument(DEFAULT_SEQUENCE_VERTICAL_SIZE);
    ASSERT_TRUE(component);

    auto sequence = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(kScrollDirectionVertical, sequence->getCalculated(kPropertyScrollDirection)));
    ASSERT_TRUE(IsEqual(Dimension(100), sequence->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(), sequence->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,0,100), sequence->getCalculated(kPropertyBounds)));
}

TEST_F(DefaultComponentTest, SequenceVerticalOverride)
{
    config->defaultComponentSize(kComponentTypeSequence, true, 200, 300);

    loadDocument(DEFAULT_SEQUENCE_VERTICAL_SIZE);
    ASSERT_TRUE(component);

    auto sequence = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(kScrollDirectionVertical, sequence->getCalculated(kPropertyScrollDirection)));
    ASSERT_TRUE(IsEqual(Dimension(300), sequence->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(200), sequence->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,200,300), sequence->getCalculated(kPropertyBounds)));
}

static const char *DEFAULT_SEQUENCE_VERTICAL_WITH_CHILD_SIZE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Frame\","
    "      \"width\": 1000,"
    "      \"height\": 1000,"
    "      \"item\": {"
    "        \"type\": \"Sequence\","
    "        \"scrollDirection\": \"vertical\","
    "        \"item\": {"
    "          \"type\": \"Text\","
    "          \"text\": \"Text\""
    "        },"
    "        \"data\": ["
    "          1"
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(DefaultComponentTest, SequenceVerticalWithChild)
{
    loadDocument(DEFAULT_SEQUENCE_VERTICAL_WITH_CHILD_SIZE);
    ASSERT_TRUE(component);

    auto sequence = component->getChildAt(0);
    ASSERT_EQ(1, sequence->getChildCount());

    ASSERT_TRUE(IsEqual(Dimension(100), sequence->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(), sequence->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,40,100), sequence->getCalculated(kPropertyBounds)));
}

/*************** Sequence: Horizontal ***************/

static const char *DEFAULT_SEQUENCE_HORIZONTAL_SIZE =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"Frame\","
        "      \"width\": 1000,"
        "      \"height\": 1000,"
        "      \"item\": {"
        "        \"type\": \"Sequence\","
        "        \"scrollDirection\": \"horizontal\""
        "      }"
        "    }"
        "  }"
        "}";

TEST_F(DefaultComponentTest, SequenceHorizontal)
{
    loadDocument(DEFAULT_SEQUENCE_HORIZONTAL_SIZE);
    ASSERT_TRUE(component);

    auto sequence = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(kScrollDirectionHorizontal, sequence->getCalculated(kPropertyScrollDirection)));
    ASSERT_TRUE(IsEqual(Dimension(), sequence->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(100), sequence->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,0), sequence->getCalculated(kPropertyBounds)));
}

TEST_F(DefaultComponentTest, SequenceHorizontalOverride)
{
    config->defaultComponentSize(kComponentTypeSequence, false, 300, 400);
    config->defaultComponentSize(kComponentTypeSequence, true, 500, 600);  // Vertical scrolling
    loadDocument(DEFAULT_SEQUENCE_HORIZONTAL_SIZE);
    ASSERT_TRUE(component);

    auto sequence = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(kScrollDirectionHorizontal, sequence->getCalculated(kPropertyScrollDirection)));
    ASSERT_TRUE(IsEqual(Dimension(400), sequence->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(300), sequence->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,300,400), sequence->getCalculated(kPropertyBounds)));
}

static const char *DEFAULT_SEQUENCE_HORIZONTAL_WITH_CHILD_SIZE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Frame\","
    "      \"width\": 1000,"
    "      \"height\": 1000,"
    "      \"item\": {"
    "        \"type\": \"Sequence\","
    "        \"scrollDirection\": \"horizontal\","
    "        \"item\": {"
    "          \"type\": \"Text\","
    "          \"text\": \"T\""
    "        },"
    "        \"data\": ["
    "          1"
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(DefaultComponentTest, SequenceHorizontalWithChild)
{
    loadDocument(DEFAULT_SEQUENCE_HORIZONTAL_WITH_CHILD_SIZE);
    ASSERT_TRUE(component);

    auto sequence = component->getChildAt(0);
    ASSERT_EQ(1, sequence->getChildCount());

    ASSERT_TRUE(IsEqual(Dimension(), sequence->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(100), sequence->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,10), sequence->getCalculated(kPropertyBounds)));
}

/*************** Text ***************/

static const char *DEFAULT_TEXT_SIZE =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"Frame\","
        "      \"width\": 1000,"
        "      \"height\": 1000,"
        "      \"item\": {"
        "        \"type\": \"Text\","
        "        \"text\": \"Test\""
        "      }"
        "    }"
        "  }"
        "}";

TEST_F(DefaultComponentTest, Text)
{
    loadDocument(DEFAULT_TEXT_SIZE);
    ASSERT_TRUE(component);

    auto text = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(), text->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(), text->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,40,10), text->getCalculated(kPropertyBounds)));
}

TEST_F(DefaultComponentTest, TextOverride)
{
    config->defaultComponentSize(kComponentTypeText, 33, 44);

    loadDocument(DEFAULT_TEXT_SIZE);
    ASSERT_TRUE(component);

    auto text = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(44), text->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(33), text->getCalculated(kPropertyWidth)));
    // NOTE: The default text measurement size is 10x10
    ASSERT_TRUE(IsEqual(Rect(0,0,33,44), text->getCalculated(kPropertyBounds)));
}

/*************** TouchWrapper ***************/

static const char *DEFAULT_TOUCH_WRAPPER_SIZE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Frame\","
    "      \"width\": 1000,"
    "      \"height\": 1000,"
    "      \"item\": {"
    "        \"type\": \"TouchWrapper\""
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(DefaultComponentTest, TouchWrapper)
{
    loadDocument(DEFAULT_TOUCH_WRAPPER_SIZE);
    ASSERT_TRUE(component);

    auto touchWrapper = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(), touchWrapper->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(), touchWrapper->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,0,0), touchWrapper->getCalculated(kPropertyBounds)));
}

TEST_F(DefaultComponentTest, TouchWrapperOverride)
{
    config->defaultComponentSize(kComponentTypeTouchWrapper, 33, 44);

    loadDocument(DEFAULT_TOUCH_WRAPPER_SIZE);
    ASSERT_TRUE(component);

    auto touchWrapper = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(44), touchWrapper->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(33), touchWrapper->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,33,44), touchWrapper->getCalculated(kPropertyBounds)));
}

static const char *DEFAULT_TOUCH_WRAPPER_VIEW_WITH_CHILD_SIZE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Frame\","
    "      \"width\": 1000,"
    "      \"height\": 1000,"
    "      \"item\": {"
    "        \"type\": \"TouchWrapper\","
    "        \"item\": {"
    "          \"type\": \"Text\","
    "          \"text\": \"Text\""
    "        }"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(DefaultComponentTest, TouchWrapperWithChild)
{
    loadDocument(DEFAULT_TOUCH_WRAPPER_VIEW_WITH_CHILD_SIZE);
    ASSERT_TRUE(component);

    auto touchWrapper = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(), touchWrapper->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(), touchWrapper->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,40,10), touchWrapper->getCalculated(kPropertyBounds)));
}
/*************** VectorGraphic ***************/

static const char *DEFAULT_VECTOR_GRAPHIC_SIZE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Frame\","
    "      \"width\": 1000,"
    "      \"height\": 1000,"
    "      \"item\": {"
    "        \"type\": \"VectorGraphic\""
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(DefaultComponentTest, VectorGraphic)
{
    loadDocument(DEFAULT_VECTOR_GRAPHIC_SIZE);
    ASSERT_TRUE(component);

    auto vectorGraphic = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(100), vectorGraphic->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(100), vectorGraphic->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), vectorGraphic->getCalculated(kPropertyBounds)));
}

TEST_F(DefaultComponentTest, VectorGraphicOverride)
{
    config->defaultComponentSize(kComponentTypeVectorGraphic, 123, 345);

    loadDocument(DEFAULT_VECTOR_GRAPHIC_SIZE);
    ASSERT_TRUE(component);

    auto vectorGraphic = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(345), vectorGraphic->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(123), vectorGraphic->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,123,345), vectorGraphic->getCalculated(kPropertyBounds)));
}

/*************** Video ***************/

static const char *DEFAULT_VIDEO_SIZE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Frame\","
    "      \"width\": 1000,"
    "      \"height\": 1000,"
    "      \"item\": {"
    "        \"type\": \"Video\""
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(DefaultComponentTest, Video)
{
    loadDocument(DEFAULT_VIDEO_SIZE);
    ASSERT_TRUE(component);

    auto video = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(100), video->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(100), video->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), video->getCalculated(kPropertyBounds)));
}

TEST_F(DefaultComponentTest, VideoOverride)
{
    config->defaultComponentSize(kComponentTypeVideo, 22, 33);

    loadDocument(DEFAULT_VIDEO_SIZE);
    ASSERT_TRUE(component);

    auto video = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Dimension(33), video->getCalculated(kPropertyHeight)));
    ASSERT_TRUE(IsEqual(Dimension(22), video->getCalculated(kPropertyWidth)));
    ASSERT_TRUE(IsEqual(Rect(0,0,22,33), video->getCalculated(kPropertyBounds)));
}

static const char *DEFAULT_IS_VALID_CHAR_TEST = R"(
{
    "type": "APL",
    "version": "1.4",
    "mainTemplate": {
        "item": {
            "type": "Text",
            "text": "Hello"
        }
    }
} )";

TEST_F(DefaultComponentTest, DefaultIsChar)
{
    loadDocument(DEFAULT_IS_VALID_CHAR_TEST);
    Component* textComponent = dynamic_cast<Component*>(root->topComponent().get());
    ASSERT_EQ(kComponentTypeText, textComponent->getType());
    //if isCharacterValid is unsupported for the component, it should return false.
    ASSERT_FALSE(textComponent->isCharacterValid(L'0'));
}
