/*
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
 *
 */

#include "../testeventloop.h"

using namespace apl;

class AutoSizeTest : public apl::DocumentWrapper {
public:
    ::testing::AssertionResult
    doInitialize(const char *document, float width, float height) {
        loadDocument(document);
        if (!component)
            return ::testing::AssertionFailure() << "Failed to load document";
        return IsEqual(Rect(0, 0, width, height), component->getCalculated(kPropertyBounds));
    }

    ::testing::AssertionResult
    doTest(const char *property, int value, float width, float height) {
        executeCommand("SetValue", {{"componentId", "FOO"}, {"property", property}, {"value", value}}, true);
        root->clearPending();
        return checkComponent(width, height);
    }
  
    ::testing::AssertionResult
    doTest(const char *property, const char *value, float width, float height) {
        executeCommand("SetValue", {{"componentId", "FOO"}, {"property", property}, {"value", value}}, true);
        root->clearPending();
        return checkComponent(width, height);
    }

    ::testing::AssertionResult
    checkComponent(float width, float height) {
        return CheckComponent(component, width, height);
    }

    ::testing::AssertionResult
    checkViewport(float width, float height) {
        return CheckViewport(root, width, height);
    }
};

/**
 * In this test the frame is small but set to auto-size.
 */
static const char *BASIC_TEST = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "borderWidth": 100
    }
  }
}
)apl";

TEST_F(AutoSizeTest, Basic)
{
    metrics = Metrics().size(100, 100).minAndMaxHeight(50, 150).minAndMaxWidth(50, 150);
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(checkComponent(200, 200));
    ASSERT_TRUE(checkViewport(150, 150));

    // Fixed everything
    metrics = Metrics().size(300, 300);
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(checkComponent(300, 300));
    ASSERT_TRUE(checkViewport(300, 300));

    // Fixed height, variable width
    metrics = Metrics().size(300, 300).minAndMaxWidth(100, 500);
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(checkComponent(200, 300));
    ASSERT_TRUE(checkViewport(200, 300));

    // Variable height, fixed width
    metrics = Metrics().size(300, 300).minAndMaxHeight(100, 500);
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(checkComponent(300, 200));
    ASSERT_TRUE(checkViewport(300, 200));

    // Variable height and width
    metrics = Metrics().size(300, 300).minAndMaxHeight(100, 500).minAndMaxWidth(50, 350);
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(checkComponent(200, 200));
    ASSERT_TRUE(checkViewport(200, 200));

    // These test cases use a viewport that starts at 150x150, which is smaller than the document
    // Small: Fixed everything
    metrics = Metrics().size(150, 150);
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(checkComponent(200, 200));
    ASSERT_TRUE(checkViewport(150, 150));

    // Small: Fixed height, variable width
    metrics = Metrics().size(150, 150).minAndMaxWidth(100, 500);
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(checkComponent(200, 200));
    ASSERT_TRUE(checkViewport(200, 150));

    // Small: Variable height, fixed width
    metrics = Metrics().size(150, 150).minAndMaxHeight(100, 500);
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(checkComponent(200, 200));
    ASSERT_TRUE(checkViewport(150, 200));

    // Small: Variable height and width
    metrics = Metrics().size(150, 150).minAndMaxHeight(100, 500).minAndMaxWidth(50, 350);
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(checkComponent(200, 200));
    ASSERT_TRUE(checkViewport(200, 200));

    // Even smaller test cases where the variable size can't accommodate the entire Frame
    // Tiny: Fixed everything
    metrics = Metrics().size(100, 100);
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(checkComponent(200, 200));
    ASSERT_TRUE(checkViewport(100, 100));

    // Tiny: Fixed height, variable width
    metrics = Metrics().size(100, 100).minAndMaxWidth(50, 150);
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(checkComponent(200, 200));
    ASSERT_TRUE(checkViewport(150, 100));

    // Tiny: Variable height, fixed width
    metrics = Metrics().size(100, 100).minAndMaxHeight(50, 150);
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(checkComponent(200, 200));
    ASSERT_TRUE(checkViewport(100, 150));

    // Tiny: Variable height and width
    metrics = Metrics().size(100, 100).minAndMaxHeight(50, 150).minAndMaxWidth(50, 150);
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(checkComponent(200, 200));
    ASSERT_TRUE(checkViewport(150, 150));
}

/**
 * Here the frame has a fixed size - it's not auto-sizing, so the viewport doesn't matter
 */
static const char *BASIC_BOUNDED_TEST = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": 123,
      "height": 345
    }
  }
}
)apl";

TEST_F(AutoSizeTest, BasicBounded)
{
    // These test cases use a viewport that starts at 400x400 which is bigger than the document
    // Fixed everything
    metrics = Metrics().size(400, 400);
    loadDocument(BASIC_BOUNDED_TEST);
    ASSERT_TRUE(checkComponent(123, 345));
    ASSERT_TRUE(checkViewport(400, 400));

    // Fixed height, variable width
    metrics = Metrics().size(400, 400).minAndMaxWidth(100, 500);
    loadDocument(BASIC_BOUNDED_TEST);
    ASSERT_TRUE(checkComponent(123, 345));
    ASSERT_TRUE(checkViewport(123, 400));

    // Variable height, fixed width
    metrics = Metrics().size(400, 400).minAndMaxHeight(100, 500);
    loadDocument(BASIC_BOUNDED_TEST);
    ASSERT_TRUE(checkComponent(123, 345));
    ASSERT_TRUE(checkViewport(400, 345));

    // Variable height and width
    metrics = Metrics().size(400, 400).minAndMaxHeight(100, 500).minAndMaxWidth(50, 350);
    loadDocument(BASIC_BOUNDED_TEST);
    ASSERT_TRUE(checkComponent(123, 345));
    ASSERT_TRUE(checkViewport(123, 345));

    // These test cases use a viewport that starts at 200x200, which sort-of in the document
    // Small: Fixed everything
    metrics = Metrics().size(200, 200);
    loadDocument(BASIC_BOUNDED_TEST);
    ASSERT_TRUE(checkComponent(123, 345));
    ASSERT_TRUE(checkViewport(200, 200));

    // Small: Fixed height, variable width
    metrics = Metrics().size(200, 200).minAndMaxWidth(100, 500);
    loadDocument(BASIC_BOUNDED_TEST);
    ASSERT_TRUE(checkComponent(123, 345));
    ASSERT_TRUE(checkViewport(123, 200));

    // Small: Variable height, fixed width
    metrics = Metrics().size(200, 200).minAndMaxHeight(100, 300);
    loadDocument(BASIC_BOUNDED_TEST);
    ASSERT_TRUE(checkComponent(123, 345));
    ASSERT_TRUE(checkViewport(200, 300));

    // Small: Variable height and width
    metrics = Metrics().size(200, 200).minAndMaxHeight(100, 500).minAndMaxWidth(50, 350);
    loadDocument(BASIC_BOUNDED_TEST);
    ASSERT_TRUE(checkComponent(123, 345));
    ASSERT_TRUE(checkViewport(123, 345));

    // Even smaller test cases where the variable size can't accommodate the entire Frame
    // Tiny: Fixed everything
    metrics = Metrics().size(100, 100);
    loadDocument(BASIC_BOUNDED_TEST);
    ASSERT_TRUE(checkComponent(123, 345));
    ASSERT_TRUE(checkViewport(100, 100));

    // Tiny: Fixed height, variable width
    metrics = Metrics().size(100, 100).minAndMaxWidth(50, 150);
    loadDocument(BASIC_BOUNDED_TEST);
    ASSERT_TRUE(checkComponent(123, 345));
    ASSERT_TRUE(checkViewport(123, 100));

    // Tiny: Variable height, fixed width
    metrics = Metrics().size(100, 100).minAndMaxHeight(50, 150);
    loadDocument(BASIC_BOUNDED_TEST);
    ASSERT_TRUE(checkComponent(123, 345));
    ASSERT_TRUE(checkViewport(100, 150));

    // Tiny: Variable height and width
    metrics = Metrics().size(100, 100).minAndMaxHeight(50, 150).minAndMaxWidth(50, 150);
    loadDocument(BASIC_BOUNDED_TEST);
    ASSERT_TRUE(checkComponent(123, 345));
    ASSERT_TRUE(checkViewport(123, 150));
}

/**
 * Here the frame has a fixed size in percentage
 */
static const char *PERCENTAGE_BOUNDED_TEST = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "50%",
      "height": "30%"
    }
  }
}
)apl";

TEST_F(AutoSizeTest, PercentageBounded)
{
    // The frame is 50% the width of the viewport and 30% the height
    // Fixed everything
    metrics = Metrics().size(1000, 1000);
    loadDocument(PERCENTAGE_BOUNDED_TEST);
    ASSERT_TRUE(checkComponent(500, 300));
    ASSERT_TRUE(checkViewport(1000, 1000));

    // Fixed height, variable width
    metrics = Metrics().size(1000, 1000).minAndMaxWidth(500, 1500);
    loadDocument(PERCENTAGE_BOUNDED_TEST);
    ASSERT_TRUE(checkComponent(500, 300));
    ASSERT_TRUE(checkViewport(1000, 1000));

    // Variable height, fixed width
    metrics = Metrics().size(1000, 1000).minAndMaxHeight(500, 1500);
    loadDocument(PERCENTAGE_BOUNDED_TEST);
    ASSERT_TRUE(checkComponent(500, 300));
    ASSERT_TRUE(checkViewport(1000, 1000));

    // Variable height and width
    metrics = Metrics().size(1000, 1000).minAndMaxHeight(500, 1500).minAndMaxWidth(500, 1500);
    loadDocument(PERCENTAGE_BOUNDED_TEST);
    ASSERT_TRUE(checkComponent(500, 300));
    ASSERT_TRUE(checkViewport(1000, 1000));
}

/**
 * The wrapping test puts a bunch of 100x100 dp boxes in a container
 * with wrapping set to true.  The container auto-sizes in width and height
 */
static const char *WRAP_TEST = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "direction": "row",
      "wrap": "wrap",
      "items": {
        "type": "Frame",
        "width": 100,
        "height": 100
      },
      "data": "${Array.range(10)}"
    }
  }
}
)apl";

TEST_F(AutoSizeTest, WrapTest) {
    // Fixed viewport, single line
    metrics = Metrics().size(1000, 1000);
    loadDocument(WRAP_TEST);
    ASSERT_TRUE(checkComponent(1000, 1000)); // Auto-scale both directions, fixed viewport
    ASSERT_TRUE(checkViewport(1000, 1000));

    // Fixed viewport, two lines
    metrics = Metrics().size(800, 1000);
    loadDocument(WRAP_TEST);
    ASSERT_TRUE(checkComponent(800, 1000));
    ASSERT_TRUE(checkViewport(800, 1000));

    // Fixed viewport, single line, allow wrap horizontal
    metrics = Metrics().size(1000, 1000).minAndMaxWidth(500, 1000);
    loadDocument(WRAP_TEST);
    ASSERT_TRUE(checkComponent(1000, 1000)); // Auto-scale both directions, fixed viewport
    ASSERT_TRUE(checkViewport(1000, 1000));

    // Fixed viewport, single line, allow wrap horizontal and vertical
    metrics = Metrics().size(1000, 1000).minAndMaxHeight(100, 1000).minAndMaxWidth(500, 1000);
    loadDocument(WRAP_TEST);
    ASSERT_TRUE(checkComponent(1000, 100)); // Auto-scale both directions, fixed viewport
    ASSERT_TRUE(checkViewport(1000, 100));

    // Fixed viewport, two lines, allow wrap horizontal and vertical
    metrics = Metrics().size(600, 1000).minAndMaxHeight(100, 1000).minAndMaxWidth(500, 750);
    loadDocument(WRAP_TEST);
    ASSERT_TRUE(checkComponent(750, 200)); // Fixes width to max width first, then height to calculated
    ASSERT_TRUE(checkViewport(750, 200));

    // Fixed viewport, multiple lines, allow wrap horizontal and vertical
    metrics = Metrics().size(200, 200).minAndMaxHeight(100, 400).minAndMaxWidth(100, 400);
    loadDocument(WRAP_TEST);
    ASSERT_TRUE(checkComponent(400, 300)); // Fixes width to max width first, then height to calculated
    ASSERT_TRUE(checkViewport(400, 300));
}


/**
 * This test has an auto-sizing frame wrapped around something of a known size.
 */
static const char *EMBEDDED_TEST = R"apl(
{
    "type": "APL",
    "version": "2022.2",
    "mainTemplate": {
        "item": {
            "type": "Frame",
            "id": "OUTER",
            "item": {
                "type": "Frame",
                "id": "INNER",
                "width": 100,
                "height": 200
            }
        }
    }
}
)apl";

TEST_F(AutoSizeTest, Embedded)
{
    metrics = Metrics().size(300, 300);
    loadDocument(EMBEDDED_TEST);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Rect(0,0,300,300), component->getCalculated(apl::kPropertyBounds)));

    metrics = Metrics().size(300, 300).minAndMaxWidth(200, 400);
    loadDocument(EMBEDDED_TEST);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Rect(0,0,200,300), component->getCalculated(apl::kPropertyBounds)));

    metrics = Metrics().size(500,500).minAndMaxHeight(100, 600);
    loadDocument(EMBEDDED_TEST);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Rect(0,0,500,200), component->getCalculated(apl::kPropertyBounds)));

    metrics = Metrics().size(400,400).minAndMaxWidth(300, 500).minAndMaxHeight(350, 450);
    loadDocument(EMBEDDED_TEST);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Rect(0,0,300,350), component->getCalculated(apl::kPropertyBounds)));
}

static const char *SCROLL_VIEW = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "item": {
      "type": "ScrollView",
      "item": {
        "type": "Frame",
        "width": 300,
        "height": 1000
      }
    }
  }
}
)apl";

TEST_F(AutoSizeTest, ScrollView)
{
    // The ScrollView defaults to an auto-sized width and a height of 100.
    metrics.minAndMaxWidth(200, 400).minAndMaxHeight(50, 2000);
    loadDocument(SCROLL_VIEW);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Rect(0,0,300,100), component->getCalculated(apl::kPropertyBounds)));
}

static const char *RESIZING = R"apl(
{
    "type": "APL",
    "version": "2022.2",
    "mainTemplate": {
        "item": {
            "type": "Frame",
            "borderWidth": 1,
            "item": {
                "type": "Frame",
                "id": "FOO",
                "width": 10,
                "height": 20
            }
        }
    }
}
)apl";

/**
 * The Frame doesn't have any min/max, so it will take on the dimensions of the viewport
 */
TEST_F(AutoSizeTest, Resizing)
{
    // Allow resizing in both direction
    metrics = Metrics().size(100,200).minAndMaxWidth(50, 1000).minAndMaxHeight(100, 900);
    ASSERT_TRUE(doInitialize(RESIZING, 50, 100)); // Starts at 50,100
    ASSERT_TRUE(checkViewport(50, 100));
    ASSERT_TRUE(doTest("width", 70, 72, 100));
    ASSERT_TRUE(checkViewport(72, 100));
    ASSERT_TRUE(doTest("width", 700, 702, 100));
    ASSERT_TRUE(checkViewport(702, 100));
    ASSERT_TRUE(doTest("width", 2000, 1000, 100));
    ASSERT_TRUE(checkViewport(1000, 100));
    ASSERT_TRUE(doTest("height", 700, 1000, 702));
    ASSERT_TRUE(checkViewport(1000, 702));
    ASSERT_TRUE(doTest("height", 1000, 1000, 900));
    ASSERT_TRUE(checkViewport(1000, 900));
    ASSERT_TRUE(doTest("height", 10, 1000, 100));
    ASSERT_TRUE(checkViewport(1000, 100));

    // Auto-size width
    metrics = Metrics().size(100,200).minAndMaxWidth(50, 1000);
    ASSERT_TRUE(doInitialize(RESIZING, 50, 200));
    ASSERT_TRUE(checkViewport(50, 200));
    ASSERT_TRUE(doTest("width", 40, 50, 200));
    ASSERT_TRUE(checkViewport(50, 200));
    ASSERT_TRUE(doTest("width", 100, 102, 200));
    ASSERT_TRUE(checkViewport(102, 200));
    ASSERT_TRUE(doTest("height", 70, 102, 200));
    ASSERT_TRUE(checkViewport(102, 200));

    // Auto-size height
    metrics = Metrics().size(100,200).minAndMaxHeight(100, 900);
    ASSERT_TRUE(doInitialize(RESIZING, 100, 100));
    ASSERT_TRUE(checkViewport(100, 100));
    ASSERT_TRUE(doTest("width", 200, 100, 100));
    ASSERT_TRUE(checkViewport(100, 100));
    ASSERT_TRUE(doTest("height", 170, 100, 172));
    ASSERT_TRUE(checkViewport(100, 172));

    // No auto-sizing
    metrics = Metrics().size(100,200);
    ASSERT_TRUE(doInitialize(RESIZING, 100, 200));
    ASSERT_TRUE(checkViewport(100, 200));
    ASSERT_TRUE(doTest("width", 40, 100, 200));
    ASSERT_TRUE(checkViewport(100, 200));
    ASSERT_TRUE(doTest("height", 70, 100, 200));
    ASSERT_TRUE(checkViewport(100, 200));
}


/**
 * Fixed size viewport.
 * Auto-sizing frame with min/max values
 */
static const char *MIN_MAX_BOUNDED_TEST = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "auto",
      "minWidth": 100,
      "maxWidth": 200,
      "height": "auto",
      "minHeight": 100,
      "maxHeight": 200,
      "item": {
        "type": "Frame",
        "id": "FOO",
        "width": 125,
        "height": 200
      }
    }
  }
}
)apl";

/*
 * The Frame has a min/max, so it will not take on the viewport dimensions.
 * The viewport has a fixed size
 */
TEST_F(AutoSizeTest, MinMaxBounded)
{
    // Allow resizing in both direction; larger viewport
    metrics = Metrics().size(400,400);
    ASSERT_TRUE(doInitialize(MIN_MAX_BOUNDED_TEST, 125, 200));
    ASSERT_TRUE(checkViewport(400,400));

    // Wider than the maxWidth
    ASSERT_TRUE(doTest("width", 300, 200, 200));
    ASSERT_TRUE(checkViewport(400,400));

    // Narrower than the minWidth
    ASSERT_TRUE(doTest("width", 20, 100, 200));
    ASSERT_TRUE(checkViewport(400,400));

    // Shorter
    ASSERT_TRUE(doTest("height", 20, 100, 100));
    ASSERT_TRUE(checkViewport(400,400));

    // Taller
    ASSERT_TRUE(doTest("height", 250, 100, 200));
    ASSERT_TRUE(checkViewport(400,400));

    // Shrink width
    ASSERT_TRUE(doTest("width", 150, 150, 200));
    ASSERT_TRUE(checkViewport(400,400));

    // Switch to a viewport that's a little smaller than the max size of the frame
    // The same tests result in the same basic size because the component has a maxWidth/Height,
    // and hence the component width/height is calculated and clamped to the min/max Width/Height.
    metrics = Metrics().size(150,150);
    ASSERT_TRUE(doInitialize(MIN_MAX_BOUNDED_TEST, 125, 200));
    ASSERT_TRUE(checkViewport(150,150));

    // Wider than the maxWidth
    ASSERT_TRUE(doTest("width", 300, 200, 200));
    ASSERT_TRUE(checkViewport(150,150));

    // Narrower than the minWidth
    ASSERT_TRUE(doTest("width", 20, 100, 200));
    ASSERT_TRUE(checkViewport(150,150));

    // Shorter
    ASSERT_TRUE(doTest("height", 20, 100, 100));
    ASSERT_TRUE(checkViewport(150,150));

    // Taller
    ASSERT_TRUE(doTest("height", 250, 100, 200));
    ASSERT_TRUE(checkViewport(150,150));

    // Shrink width
    ASSERT_TRUE(doTest("width", 150, 150, 200));
    ASSERT_TRUE(checkViewport(150,150));
}

/**
 * Bounded with max/min width.
 */
static const char *MIN_MAX_VARIABLE_TEST = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "auto",
      "minWidth": 100,
      "maxWidth": 200,
      "height": "auto",
      "minHeight": 100,
      "maxHeight": 200,
      "item": {
        "type": "Frame",
        "id": "FOO",
        "width": 125,
        "height": 200
      }
    }
  }
}
)apl";

/*
 * The Frame has a min/max, so it will not take on the viewport dimensions.
 * The viewport also has a min/max, so it will stretch/shrink to match the frame (to a point)
 */
TEST_F(AutoSizeTest, MinMaxVariable)
{
    // Allow resizing in both direction; larger viewport
    metrics = Metrics().size(400,400).minAndMaxWidth(150, 500).minAndMaxHeight(150,500);
    ASSERT_TRUE(doInitialize(MIN_MAX_VARIABLE_TEST, 125, 200));  // Clamps to viewport.minWidth
    ASSERT_TRUE(checkViewport(150,200));

    // Wider than the maxWidth
    ASSERT_TRUE(doTest("width", 300, 200, 200));  // Component clamps to 200
    ASSERT_TRUE(checkViewport(200,200));

    // Narrower than the minWidth
    ASSERT_TRUE(doTest("width", 20, 100, 200));
    ASSERT_TRUE(checkViewport(150,200));

    // Shorter
    ASSERT_TRUE(doTest("height", 20, 100, 100));
    ASSERT_TRUE(checkViewport(150,150));

    // Taller
    ASSERT_TRUE(doTest("height", 250, 100, 200));
    ASSERT_TRUE(checkViewport(150,200));

    // Widen width
    ASSERT_TRUE(doTest("width", 150, 150, 200));
    ASSERT_TRUE(checkViewport(150,200));

    // Smaller viewport that will clamp _before_ the component min/max
    metrics = Metrics().size(400,400).minAndMaxWidth(125, 175).minAndMaxHeight(125,175);
    ASSERT_TRUE(doInitialize(MIN_MAX_VARIABLE_TEST, 125, 200));  // Clamps to viewport.minWidth
    ASSERT_TRUE(checkViewport(125,175));  // The viewport has been clamped (the component leaks out a bit)

    // Wider than the maxWidth
    ASSERT_TRUE(doTest("width", 300, 200, 200));  // Component clamps to 200
    ASSERT_TRUE(checkViewport(175,175));  // Viewport clamps smaller

    // Narrower than the minWidth
    ASSERT_TRUE(doTest("width", 20, 100, 200));
    ASSERT_TRUE(checkViewport(125,175));

    // Shorter
    ASSERT_TRUE(doTest("height", 20, 100, 100));
    ASSERT_TRUE(checkViewport(125,125));

    // Taller
    ASSERT_TRUE(doTest("height", 250, 100, 200));
    ASSERT_TRUE(checkViewport(125,175));

    // Widen width
    ASSERT_TRUE(doTest("width", 150, 150, 200));
    ASSERT_TRUE(checkViewport(150,175));
}

/**
 * Configuration change.
 */
static const char *CONFIGURATION_CHANGE_TEST = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "auto",
      "minWidth": 100,
      "maxWidth": 200,
      "height": "auto",
      "minHeight": 100,
      "maxHeight": 200,
      "item": {
        "type": "Frame",
        "id": "FOO",
        "width": 125,
        "height": 200
      }
    }
  }
}
)apl";

TEST_F(AutoSizeTest, ConfigurationChange)
{
    // Allow resizing in both direction; larger viewport
    // DPI=320 -> width 400dp, height 400dp, minWidth 150dp, maxWidth 500dp, minHeight 150dp, maxHeight 500dp
    metrics = Metrics().dpi(320).size(800, 800).minAndMaxWidth(300, 1000).minAndMaxHeight(300, 1000);
    ASSERT_TRUE(doInitialize(CONFIGURATION_CHANGE_TEST, 125, 200)); // Inner 125x200, outer matches
    ASSERT_TRUE(checkViewport(150, 200));

    // Wider than the maxWidth
    ASSERT_TRUE(doTest("width", 300, 200, 200));  // Component clamps to 200
    ASSERT_TRUE(checkViewport(200, 200));

    // Viewport width 175, minWidth 100, maxWidth 175, height 300, minHeight 250, maxHeight 375
    auto configChange = ConfigurationChange().sizeRange(350, 200, 350, 600, 500, 750);
    root->configurationChange(configChange);
    root->clearPending();
    ASSERT_TRUE(checkComponent(200, 200));  // Inner frame 300x200, Outer frame 200,200
    ASSERT_TRUE(checkViewport(175, 250));  // Viewport minHeight=250, maxWidth=175

    // Viewport width 175, minWidth 150, maxWidth 250, height 150, minHeight 150, maxHeight 175
    configChange = ConfigurationChange().sizeRange(350, 300, 500, 300, 300, 350);
    root->configurationChange(configChange);
    root->clearPending();
    ASSERT_TRUE(checkComponent(200, 200));  // Inner frame 300x200, Outer frame 200,200
    ASSERT_TRUE(checkViewport(200, 175));  // Viewport, maxHeight 175
}

static const char *TEXT_RESIZING = R"apl(
{
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "items": [
        {
          "type": "Text",
          "id": "FOO",
          "text": "Lorem"
        }
      ]
    }
  }
}
)apl";

TEST_F(AutoSizeTest, ResizingWithText)
{
    // Allow resizing in both direction
    metrics = Metrics().size(100,200).minAndMaxWidth(100, 200).minAndMaxHeight(20, 200);
    ASSERT_TRUE(doInitialize(TEXT_RESIZING, 100, 20)); // Starts at 100, 20
    ASSERT_TRUE(checkViewport(100, 20));

    // Auto-size to extend the width first
    ASSERT_TRUE(doTest("text", "Lorem ipsum dolor", 170, 20));
    ASSERT_TRUE(checkViewport(170, 20));

    // Now auto-size to extend the height
    ASSERT_TRUE(doTest("text", "Lorem ipsum dolor sit amet, consectetur adipiscing elit", 200, 30));
    ASSERT_TRUE(checkViewport(200, 30));
}
