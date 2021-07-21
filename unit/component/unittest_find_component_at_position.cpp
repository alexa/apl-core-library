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
#include "apl/utils/searchvisitor.h"

using namespace apl;

class FindComponentAtPosition : public DocumentWrapper {};

static const char *BASIC = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Image",
      "width": 100,
      "height": 100
    }
  }
})";

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

static const char *INVISIBLE = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Image",
      "width": 100,
      "height": 100,
      "display": "invisible"
    }
  }
})";

TEST_F(FindComponentAtPosition, Invisible)
{
    loadDocument(INVISIBLE);
    ASSERT_TRUE(component);

    ASSERT_EQ(nullptr, component->findComponentAtPosition(Point(10, 10)));
    ASSERT_EQ(nullptr, component->findComponentAtPosition(Point(200, 200)));
}

static const char *CONTAINER_OVERLAP = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "width": 50,
      "height": 50,
      "paddingTop": 10,
      "paddingBottom": 10,
      "paddingLeft": 10,
      "paddingRight": 10,
      "items": [
        {
          "type": "Image",
          "width": 20,
          "height": 20
        },
        {
          "type": "Text",
          "width": 20,
          "height": 20,
          "left": 20,
          "top": 20,
          "position": "absolute"
        }
      ]
    }
  }
})";

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

static const char *SEQUENCE_WITH_PADDING = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "width": 100,
      "height": 40,
      "paddingTop": 10,
      "paddingBottom": 10,
      "paddingLeft": 10,
      "paddingRight": 10,
      "items": {
        "type": "Image",
        "width": 50,
        "height": 10
      },
      "data": [
        0,
        1,
        2,
        3,
        4,
        5
      ]
    }
  }
})";

TEST_F(FindComponentAtPosition, SequenceWithPadding)
{
    // Force loading of all items we are looking at to simplify testing.
    config->sequenceChildCache(5);
    loadDocument(SEQUENCE_WITH_PADDING);
    ASSERT_TRUE(component);

    ASSERT_EQ(6, component->getChildCount());

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

static const char *GRID_SEQUENCE_WITH_PADDING = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "GridSequence",
      "scrollDirection": "vertical",
      "width": 80,
      "height": 50,
      "paddingTop": 5,
      "paddingBottom": 5,
      "paddingLeft": 5,
      "paddingRight": 5,
      "childWidth": [30, 30],
      "childHeight": 20,
      "items": {
        "type": "Image"
      },
      "data": [
        0,
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9
      ]
    }
  }
})";

TEST_F(FindComponentAtPosition, GridSequenceWithPadding)
{
    // Force loading of all items we are looking at to simplify testing.
    config->sequenceChildCache(10);
    loadDocument(GRID_SEQUENCE_WITH_PADDING);
    ASSERT_TRUE(component);

    ASSERT_EQ(10, component->getChildCount());

    ASSERT_EQ(nullptr, component->findComponentAtPosition(Point(-1, -1)));
    ASSERT_EQ(nullptr, component->findComponentAtPosition(Point(101, 41)));

    ASSERT_EQ(component, component->findComponentAtPosition(Point(1, 1)));
    ASSERT_EQ(component, component->findComponentAtPosition(Point(75, 45)));

    ASSERT_EQ(component, component->findComponentAtPosition(Point(50, 0)));

    ASSERT_EQ(component->getChildAt(0), component->findComponentAtPosition(Point(15, 15)));
    ASSERT_EQ(component->getChildAt(1), component->findComponentAtPosition(Point(40, 15)));
    ASSERT_EQ(component->getChildAt(2), component->findComponentAtPosition(Point(15, 40)));
    ASSERT_EQ(component->getChildAt(3), component->findComponentAtPosition(Point(40, 40)));

    // Scroll down
    component->update(kUpdateScrollPosition, 20);
    ASSERT_EQ(component->getChildAt(2), component->findComponentAtPosition(Point(15, 15)));
    ASSERT_EQ(component->getChildAt(3), component->findComponentAtPosition(Point(40, 15)));
    ASSERT_EQ(component->getChildAt(4), component->findComponentAtPosition(Point(15, 40)));
    ASSERT_EQ(component->getChildAt(5), component->findComponentAtPosition(Point(40, 40)));

    // Scroll down
    component->update(kUpdateScrollPosition, 40);

    ASSERT_EQ(component->getChildAt(4), component->findComponentAtPosition(Point(15, 15)));
    ASSERT_EQ(component->getChildAt(5), component->findComponentAtPosition(Point(40, 15)));
    ASSERT_EQ(component->getChildAt(6), component->findComponentAtPosition(Point(15, 40)));
    ASSERT_EQ(component->getChildAt(7), component->findComponentAtPosition(Point(40, 40)));
}

/**
 * TODO: The Pager component doesn't work correctly with padding values (there's a bug
 * open on that).  For now we will test the pager without padding.
 */
static const char * PAGER_TEST = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Pager",
      "width": 100,
      "height": 100,
      "items": {
        "type": "Text",
        "width": "100%",
        "height": "100%"
      },
      "data": [
        0,
        1,
        2
      ]
    }
  }
})";

TEST_F(FindComponentAtPosition, Pager)
{
    loadDocument(PAGER_TEST);
    ASSERT_TRUE(component);
    advanceTime(10);

    ASSERT_EQ(3, component->getChildCount());

    ASSERT_EQ(nullptr, component->findComponentAtPosition(Point(-1, -1)));
    ASSERT_EQ(nullptr, component->findComponentAtPosition(Point(101, 101)));

    ASSERT_EQ(component->getChildAt(0), component->findComponentAtPosition(Point(50, 50)));

    component->update(kUpdatePagerPosition, 1);
    ASSERT_EQ(component->getChildAt(1), component->findComponentAtPosition(Point(50, 50)));
}

static const char * NESTED_TEST = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "paddingLeft": 10,
      "paddingTop": 10,
      "paddingRight": 10,
      "paddingBottom": 10,
      "width": 100,
      "height": 100,
      "items": {
        "type": "Frame",
        "paddingLeft": 10,
        "paddingTop": 10,
        "paddingRight": 10,
        "paddingBottom": 10,
        "items": {
          "type": "Image",
          "width": 50,
          "height": 50
        }
      }
    }
  }
})";


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

static const char *ABSOLUTE_POSITIONED_PAGER = R"({
  "type": "APL",
  "version": "1.4",
  "theme": "dark",
  "styles": {
    "primaryButtonStyle": {
      "values": [
        {
          "backgroundColor": "#FCAE2DFF"
        },
        {
          "when": "${state.pressed}",
          "backgroundColor": "#E8A029FF"
        },
        {
          "when": "${state.focused}",
          "backgroundColor": "#FCAE2DFF"
        },
        {
          "when": "${state.disabled}",
          "backgroundColor": "#FECF81FF"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "width": "100vw",
        "height": "100vh",
        "items": [
          {
            "type": "Pager",
            "width": "100vw",
            "height": "100vh",
            "position": "absolute",
            "left": "200dp",
            "navigation": "normal",
            "items": [
              {
                "type": "Container",
                "items": [
                  {
                    "type": "TouchWrapper",
                    "width": "200dp",
                    "height": "100dp",
                    "item": {
                      "type": "Frame",
                      "style": "primaryButtonStyle",
                      "inheritParentState": true,
                      "item": {
                        "type": "Text",
                        "id": "button",
                        "text": "Not Pressed"
                      }
                    },
                    "onPress": [
                      {
                        "type": "SetValue",
                        "componentId": "button",
                        "property": "text",
                        "value": "Pressed"
                      }
                    ]
                  }
                ]
              }
            ]
          }
        ]
      }
    ]
  }
})";

TEST_F(FindComponentAtPosition, AbsolutePositionedPager)
{
    loadDocument(ABSOLUTE_POSITIONED_PAGER);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Rect(0, 0, 1024, 800), component->getGlobalBounds()));

    ASSERT_EQ(1, component->getChildCount());
    auto pager = component->getCoreChildAt(0);
    ASSERT_TRUE(IsEqual(Rect(200, 0, 1024, 800), pager->getGlobalBounds()));

    ASSERT_EQ(1, pager->getChildCount());
    ASSERT_EQ(1, pager->getCoreChildAt(0)->getChildCount());
    auto tw = pager->getCoreChildAt(0)->getCoreChildAt(0);
    ASSERT_EQ(kComponentTypeTouchWrapper, tw->getType());
    ASSERT_TRUE(IsEqual(Rect(200, 0, 200, 100), tw->getGlobalBounds()));

    auto foundComponent = component->findComponentAtPosition(Point(201, 1));
    ASSERT_EQ("button", foundComponent->getId());

    auto visitor = TouchableAtPosition(Point(201, 1));
    component->raccept(visitor);
    foundComponent = visitor.getResult();
    ASSERT_EQ(tw->getUniqueId(), foundComponent->getUniqueId());
}
