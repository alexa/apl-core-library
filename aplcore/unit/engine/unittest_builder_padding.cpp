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

class BuilderTestPadding : public DocumentWrapper {};

static const char *BASIC_PADDING = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Frame",
            "width": 100,
            "height": 100,
            "padding": 10
          }
        }
      }
    }
)apl";

TEST_F(BuilderTestPadding, Basic)
{
    loadDocument(BASIC_PADDING);
    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getChildCount());
    auto frame = component->getChildAt(0);

    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), frame->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(10,10,80,80), frame->getCalculated(kPropertyInnerBounds)));
}


static const char *OVERRIDE_PADDING = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Frame",
            "width": 100,
            "height": 100,
            "padding": 10,
            "paddingLeft": 20,
            "paddingBottom": 15
          }
        }
      }
    }
)apl";

TEST_F(BuilderTestPadding, OverridePadding)
{
    loadDocument(OVERRIDE_PADDING);
    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getChildCount());
    auto frame = component->getChildAt(0);

    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), frame->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(20,10,70,75), frame->getCalculated(kPropertyInnerBounds)));
}


static const char *OVERRIDE_PADDING_2 = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Frame",
            "width": 100,
            "height": 100,
            "padding": 10,
            "paddingTop": 2,
            "paddingRight": 3
          }
        }
      }
    }
)apl";

TEST_F(BuilderTestPadding, OverridePadding2)
{
    loadDocument(OVERRIDE_PADDING_2);
    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getChildCount());
    auto frame = component->getChildAt(0);

    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), frame->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(10,2,87,88), frame->getCalculated(kPropertyInnerBounds)));
}


static const char *PADDING_ARRAY = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Frame",
            "width": 100,
            "height": 100,
            "padding": [10, "2vh", 20, 5]
          }
        }
      }
    }
)apl";

TEST_F(BuilderTestPadding, PaddingArray)
{
    metrics.size(200,200);
    loadDocument(PADDING_ARRAY);
    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getChildCount());
    auto frame = component->getChildAt(0);

    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), frame->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(10,4,70,91), frame->getCalculated(kPropertyInnerBounds)));
}


static const char *PADDING_ARRAY_ZERO_ELEMENT = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Frame",
            "width": 100,
            "height": 100,
            "padding": []
          }
        }
      }
    }
)apl";

TEST_F(BuilderTestPadding, PaddingArrayZeroElement)
{
    metrics.size(200,200);
    loadDocument(PADDING_ARRAY_ZERO_ELEMENT);
    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getChildCount());
    auto frame = component->getChildAt(0);

    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), frame->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), frame->getCalculated(kPropertyInnerBounds)));
}


static const char *PADDING_ARRAY_ONE_ELEMENT = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Frame",
            "width": 100,
            "height": 100,
            "padding": [10]
          }
        }
      }
    }
)apl";

TEST_F(BuilderTestPadding, PaddingArrayOneElement)
{
    metrics.size(200,200);
    loadDocument(PADDING_ARRAY_ONE_ELEMENT);
    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getChildCount());
    auto frame = component->getChildAt(0);

    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), frame->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(10,10,80,80), frame->getCalculated(kPropertyInnerBounds)));
}


static const char *PADDING_ARRAY_TWO_ELEMENT = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Frame",
            "width": 100,
            "height": 100,
            "padding": [10, 5]
          }
        }
      }
    }
)apl";

TEST_F(BuilderTestPadding, PaddingArrayTwoElement)
{
    metrics.size(200,200);
    loadDocument(PADDING_ARRAY_TWO_ELEMENT);
    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getChildCount());
    auto frame = component->getChildAt(0);

    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), frame->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(10,5,80,90), frame->getCalculated(kPropertyInnerBounds)));
}


static const char *PADDING_ARRAY_THREE_ELEMENT = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Frame",
            "width": 100,
            "height": 100,
            "padding": [10, 5, 20]
          }
        }
      }
    }
)apl";

TEST_F(BuilderTestPadding, PaddingArrayThreeElement)
{
    metrics.size(200,200);
    loadDocument(PADDING_ARRAY_THREE_ELEMENT);
    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getChildCount());
    auto frame = component->getChildAt(0);

    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), frame->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(10,5,70,90), frame->getCalculated(kPropertyInnerBounds)));
}


static const char *PADDING_ARRAY_FIVE_ELEMENT = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Frame",
            "width": 100,
            "height": 100,
            "padding": [10, 5, 20, 15, 25]
          }
        }
      }
    }
)apl";

TEST_F(BuilderTestPadding, PaddingArrayFiveElement)
{
    metrics.size(200,200);
    loadDocument(PADDING_ARRAY_FIVE_ELEMENT);
    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getChildCount());
    auto frame = component->getChildAt(0);

    ASSERT_TRUE(IsEqual(Rect(0,0,100,100), frame->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(10,5,70,80), frame->getCalculated(kPropertyInnerBounds)));
}