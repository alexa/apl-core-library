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
#include "apl/engine/event.h"
#include "apl/primitives/object.h"

using namespace apl;

class FrameComponentTest : public DocumentWrapper {
};


static const char* DEFAULT_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "Frame"
    }
  }
})";

/**
 * Test that the defaults are as expected when no values are set.
 */
TEST_F(FrameComponentTest, ComponentDefaults) {
    loadDocument(DEFAULT_DOC);

    auto frame = root->topComponent();
    ASSERT_TRUE(frame);
    ASSERT_EQ(kComponentTypeFrame, frame->getType());

    ASSERT_TRUE(IsEqual(Color(Color::TRANSPARENT), frame->getCalculated(kPropertyBackgroundColor)));
    ASSERT_TRUE(IsEqual(Color(Color::TRANSPARENT), frame->getCalculated(kPropertyBorderColor)));

    ASSERT_TRUE(IsEqual(Dimension(0), frame->getCalculated(kPropertyBorderRadius)));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), frame->getCalculated(kPropertyBorderBottomLeftRadius)));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), frame->getCalculated(kPropertyBorderBottomRightRadius)));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), frame->getCalculated(kPropertyBorderTopLeftRadius)));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), frame->getCalculated(kPropertyBorderTopRightRadius)));
    // kpropertyBorderRadii is calculated from all kpropertyBorderXXXRadius values
    ASSERT_TRUE(IsEqual(Radii(), frame->getCalculated(kPropertyBorderRadii)));

    ASSERT_TRUE(IsEqual(Dimension(0), frame->getCalculated(kPropertyBorderWidth)));
    ASSERT_TRUE(frame->getCalculated(kPropertyBorderStrokeWidth).isNull());
    ASSERT_TRUE(IsEqual(Dimension(0), frame->getCalculated(kPropertyDrawnBorderWidth)));

    // Should not have scrollable moves
    ASSERT_FALSE(component->allowForward());
    ASSERT_FALSE(component->allowBackwards());
}


static const char* NON_DEFAULT_DOC = R"({
  "type": "APL",
  "version": "1.r",
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "backgroundColor": "yellow",
      "borderColor": "blue",
      "borderWidth": 30,
      "borderStrokeWidth": 20,
      "borderRadius": 10,
      "borderBottomLeftRadius": 11,
      "borderBottomRightRadius": 12,
      "borderTopLeftRadius": 13,
      "borderTopRightRadius": 14
    }
  }
})";

/**
 * Test the setting of all properties to non default values.
 */
TEST_F(FrameComponentTest, NonDefaults) {

    loadDocument(NON_DEFAULT_DOC);

    auto frame = root->topComponent();
    ASSERT_TRUE(frame);
    ASSERT_EQ(kComponentTypeFrame, frame->getType());

    ASSERT_TRUE(IsEqual(Color(Color::YELLOW), frame->getCalculated(kPropertyBackgroundColor)));
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), frame->getCalculated(kPropertyBorderColor)));

    ASSERT_TRUE(IsEqual(Dimension(10), frame->getCalculated(kPropertyBorderRadius)));
    ASSERT_TRUE(IsEqual(Dimension(11), frame->getCalculated(kPropertyBorderBottomLeftRadius)));
    ASSERT_TRUE(IsEqual(Dimension(12), frame->getCalculated(kPropertyBorderBottomRightRadius)));
    ASSERT_TRUE(IsEqual(Dimension(13), frame->getCalculated(kPropertyBorderTopLeftRadius)));
    ASSERT_TRUE(IsEqual(Dimension(14), frame->getCalculated(kPropertyBorderTopRightRadius)));
    // kpropertyBorderRadii is calculated from all kpropertyBorderXXXRadius values
    ASSERT_TRUE(IsEqual(Radii(13, 14, 11, 12), frame->getCalculated(kPropertyBorderRadii)));

    ASSERT_TRUE(IsEqual(Dimension(20), frame->getCalculated(kPropertyBorderStrokeWidth)));
    ASSERT_TRUE(IsEqual(Dimension(30), frame->getCalculated(kPropertyBorderWidth)));
    // kPropertyDrawnBorderWidth is calculated from kPropertyBorderStrokeWidth (inputOnly) and (kPropertyBorderWidth)
    ASSERT_TRUE(IsEqual(Dimension(20), frame->getCalculated(kPropertyDrawnBorderWidth)));
}


static const char* INVALID_DIMENSIONS_DOC = R"({
  "type": "APL",
  "version": "1.r",
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "borderStrokeWidth": -20,
      "borderWidth": -30,
      "size": -44
    }
  }
})";

/**
 * Test the setting of all properties to non default values.
 */
TEST_F(FrameComponentTest, InvalidDimensions) {

    loadDocument(INVALID_DIMENSIONS_DOC);

    auto et = root->topComponent();
    ASSERT_TRUE(et);
    ASSERT_EQ(kComponentTypeFrame, et->getType());

    ASSERT_TRUE(IsEqual(Dimension(0), et->getCalculated(kPropertyBorderStrokeWidth)));
    ASSERT_TRUE(IsEqual(Dimension(0), et->getCalculated(kPropertyBorderWidth)));
    // kPropertyDrawnBorderWidth is calculated from kPropertyBorderStrokeWidth (inputOnly) and (kPropertyBorderWidth)
    // it is the minimum of the two
    ASSERT_TRUE(IsEqual(Dimension(0), et->getCalculated(kPropertyDrawnBorderWidth)));
}


static const char* BORDER_STROKE_CLAMP_DOC = R"({
  "type": "APL",
  "version": "1.r",
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "id": "myFrame",
      "borderStrokeWidth": 64,
      "borderWidth": 30
    }
  }
})";

/**
 * Test the drawn border is clamped to the min of borderWidth and borderStrokeWidth.
 */
TEST_F(FrameComponentTest, ClampDrawnBorder) {

    loadDocument(BORDER_STROKE_CLAMP_DOC);

    auto frame = root->topComponent();
    ASSERT_TRUE(frame);
    ASSERT_EQ(kComponentTypeFrame, frame->getType());

    ASSERT_TRUE(IsEqual(Dimension(30), frame->getCalculated(kPropertyBorderWidth)));
    ASSERT_TRUE(IsEqual(Dimension(64), frame->getCalculated(kPropertyBorderStrokeWidth)));
    // kPropertyDrawnBorderWidth is calculated from kPropertyBorderStrokeWidth (inputOnly) and (kPropertyBorderWidth)
    // and is clamped to kPropertyBorderWidth
    ASSERT_TRUE(IsEqual(Dimension(30), frame->getCalculated(kPropertyDrawnBorderWidth)));

    // execute command to set kPropertyBorderStrokeWidth within border bounds,
    // the drawn border should update
    executeCommand("SetValue", {{"componentId", "myFrame"}, {"property", "borderStrokeWidth"}, {"value", 17}}, false);
    ASSERT_TRUE(IsEqual(Dimension(17), frame->getCalculated(kPropertyBorderStrokeWidth)));
    ASSERT_TRUE(IsEqual(Dimension(17), frame->getCalculated(kPropertyDrawnBorderWidth)));

    // execute command to set kPropertyBorderWidth to something smaller.  Drawn border width should update
    executeCommand("SetValue", {{"componentId", "myFrame"}, {"property", "borderWidth"}, {"value", 5}}, false);
    ASSERT_TRUE(IsEqual(Dimension(5), frame->getCalculated(kPropertyBorderWidth)));
    ASSERT_TRUE(IsEqual(Dimension(17), frame->getCalculated(kPropertyBorderStrokeWidth)));
    ASSERT_TRUE(IsEqual(Dimension(5), frame->getCalculated(kPropertyDrawnBorderWidth)));
}


static const char* STYLED_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "styles": {
    "myStyle": {
      "values": [
        {
      "backgroundColor": "yellow",
      "borderColor": "blue",
      "borderWidth": 30,
      "borderStrokeWidth": 20,
      "borderRadius": 10,
      "borderBottomLeftRadius": 11,
      "borderBottomRightRadius": 12,
      "borderTopLeftRadius": 13,
      "borderTopRightRadius": 14
        }
      ]
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "style": "myStyle"
    }
  }
})";

/**
 * Verify styled properties can be set via style, and non-styled properties cannot be set via style
 */
TEST_F(FrameComponentTest, Styled) {
    loadDocument(STYLED_DOC);

    auto frame = root->topComponent();
    ASSERT_TRUE(frame);
    ASSERT_EQ(kComponentTypeFrame, frame->getType());


    // all values are styled

    ASSERT_TRUE(IsEqual(Color(Color::YELLOW), frame->getCalculated(kPropertyBackgroundColor)));
    ASSERT_TRUE(IsEqual(Color(Color::YELLOW), frame->getCalculated(kPropertyBackground)));
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), frame->getCalculated(kPropertyBorderColor)));

    ASSERT_TRUE(IsEqual(Dimension(10), frame->getCalculated(kPropertyBorderRadius)));
    ASSERT_TRUE(IsEqual(Dimension(11), frame->getCalculated(kPropertyBorderBottomLeftRadius)));
    ASSERT_TRUE(IsEqual(Dimension(12), frame->getCalculated(kPropertyBorderBottomRightRadius)));
    ASSERT_TRUE(IsEqual(Dimension(13), frame->getCalculated(kPropertyBorderTopLeftRadius)));
    ASSERT_TRUE(IsEqual(Dimension(14), frame->getCalculated(kPropertyBorderTopRightRadius)));
    // kpropertyBorderRadii is calculated from all kpropertyBorderXXXRadius values
    ASSERT_TRUE(IsEqual(Radii(13, 14, 11, 12), frame->getCalculated(kPropertyBorderRadii)));

    ASSERT_TRUE(IsEqual(Dimension(30), frame->getCalculated(kPropertyBorderWidth)));
    ASSERT_TRUE(IsEqual(Dimension(20), frame->getCalculated(kPropertyBorderStrokeWidth)));
    // kPropertyDrawnBorderWidth is calculated from kPropertyBorderStrokeWidth (inputOnly) and (kPropertyBorderWidth)
    ASSERT_TRUE(IsEqual(Dimension(20), frame->getCalculated(kPropertyDrawnBorderWidth)));
}

const char *SIMPLE_FRAME = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "items": [
        {
          "type": "Text"
        },
        {
          "type": "Text"
        }
      ]
    }
  }
})";

TEST_F(FrameComponentTest, SimpleFrame)
{
    loadDocument(SIMPLE_FRAME);

    auto map = component->getCalculated();
    ASSERT_EQ(kComponentTypeFrame, component->getType());

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
    ASSERT_EQ(Object(Object::NULL_OBJECT()), component->getCalculated(kPropertyPaddingBottom));
    ASSERT_EQ(Object(Object::NULL_OBJECT()), component->getCalculated(kPropertyPaddingLeft));
    ASSERT_EQ(Object(Object::NULL_OBJECT()), component->getCalculated(kPropertyPaddingRight));
    ASSERT_EQ(Object(Object::NULL_OBJECT()), component->getCalculated(kPropertyPaddingTop));
    ASSERT_EQ(Object(ObjectArray{}), component->getCalculated(kPropertyPadding));
    ASSERT_EQ(Object(Dimension()), component->getCalculated(kPropertyWidth));
    ASSERT_EQ(Object::TRUE_OBJECT(), component->getCalculated(kPropertyLaidOut));

    // Frame properties
    ASSERT_EQ(0x00000000, component->getCalculated(kPropertyBackgroundColor).getColor());
    ASSERT_EQ(Object(Radii()), component->getCalculated(kPropertyBorderRadii));
    ASSERT_EQ(0x00000000, component->getCalculated(kPropertyBorderColor).getColor());
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyBorderRadius));
    ASSERT_EQ(Object(Dimension(0)), component->getCalculated(kPropertyBorderWidth));

    // Children
    ASSERT_EQ(1, component->getChildCount());
}

static const char *BORDER_TEST = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "borderRadius": 10
    }
  }
})";

TEST_F(FrameComponentTest, Borders)
{
    loadDocument(BORDER_TEST);

    // The border radius should be set to 10
    auto map = component->getCalculated();
    ASSERT_EQ(Object(Dimension(10)), map.get(kPropertyBorderRadius));

    // The assigned values are still null
    ASSERT_EQ(Object::NULL_OBJECT(), map.get(kPropertyBorderTopLeftRadius));
    ASSERT_EQ(Object::NULL_OBJECT(), map.get(kPropertyBorderTopRightRadius));
    ASSERT_EQ(Object::NULL_OBJECT(), map.get(kPropertyBorderBottomLeftRadius));
    ASSERT_EQ(Object::NULL_OBJECT(), map.get(kPropertyBorderBottomRightRadius));

    // The output values match the border radius
    ASSERT_EQ(Radii(10), map.get(kPropertyBorderRadii).get<Radii>());
}

static const char *BORDER_TEST_2 = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "borderBottomLeftRadius": 1,
      "borderBottomRightRadius": 2,
      "borderTopLeftRadius": 3,
      "borderTopRightRadius": 4,
      "borderRadius": 5
    }
  }
})";

TEST_F(FrameComponentTest, Borders2)
{
    loadDocument(BORDER_TEST_2);

    // The border radius should be set to 5
    auto map = component->getCalculated();
    ASSERT_EQ(Object(Dimension(5)), map.get(kPropertyBorderRadius));

    // The assigned values all exist
    ASSERT_EQ(Object(Dimension(1)), map.get(kPropertyBorderBottomLeftRadius));
    ASSERT_EQ(Object(Dimension(2)), map.get(kPropertyBorderBottomRightRadius));
    ASSERT_EQ(Object(Dimension(3)), map.get(kPropertyBorderTopLeftRadius));
    ASSERT_EQ(Object(Dimension(4)), map.get(kPropertyBorderTopRightRadius));

    // The output values match the assigned values
    ASSERT_EQ(Object(Radii(3,4,1,2)), map.get(kPropertyBorderRadii));
}

static const char *FRAME_BORDER_SHRINK = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "items": {
        "type": "Frame",
        "width": 100,
        "height": 100,
        "borderWidth": 5
      }
    }
  }
})";

TEST_F(FrameComponentTest, BorderShrink)
{
    loadDocument(FRAME_BORDER_SHRINK);

    auto frame = component->getChildAt(0);
    ASSERT_EQ(Object(Dimension(5)), frame->getCalculated(kPropertyDrawnBorderWidth));
    ASSERT_EQ(Object(Rect(0, 0, 100, 100)), frame->getCalculated(kPropertyBounds));
    ASSERT_EQ(Object(Rect(5, 5, 90, 90)), frame->getCalculated(kPropertyInnerBounds));
}

static const char *FRAME_BORDER_EMPTY = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "items": {
        "type": "Frame",
        "width": 0,
        "height": 0,
        "borderWidth": 5
      }
    }
  }
})";

TEST_F(FrameComponentTest, BorderEmpty)
{
    loadDocument(FRAME_BORDER_EMPTY);

    auto frame = component->getChildAt(0);
    ASSERT_EQ(Object(Dimension(5)), frame->getCalculated(kPropertyDrawnBorderWidth));
    ASSERT_EQ(Object(Rect(0, 0, 0, 0)), frame->getCalculated(kPropertyBounds));
    ASSERT_EQ(Object(Rect(0, 0, 0, 0)), frame->getCalculated(kPropertyInnerBounds));
}

static const char *BORDER_TEST_STYLE = R"({
  "type": "APL",
  "version": "1.0",
  "styles": {
    "BorderToggle": {
      "values": [
        {
          "when": "${state.pressed}",
          "borderRadius": 100
        },
        {
          "when": "${state.karaoke}",
          "borderBottomLeftRadius": 1,
          "borderBottomRightRadius": 2,
          "borderTopLeftRadius": 3,
          "borderTopRightRadius": 4
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "style": "BorderToggle"
    }
  }
})";

TEST_F(FrameComponentTest, BordersStyle)
{
    loadDocument(BORDER_TEST_STYLE);

    // The border radius should be set to 0
    auto map = component->getCalculated();
    ASSERT_EQ(Object(Dimension(0)), map.get(kPropertyBorderRadius));

    // The assigned values are null
    ASSERT_EQ(Object::NULL_OBJECT(), map.get(kPropertyBorderBottomLeftRadius));
    ASSERT_EQ(Object::NULL_OBJECT(), map.get(kPropertyBorderBottomRightRadius));
    ASSERT_EQ(Object::NULL_OBJECT(), map.get(kPropertyBorderTopLeftRadius));
    ASSERT_EQ(Object::NULL_OBJECT(), map.get(kPropertyBorderTopRightRadius));

    // The output values match the master border radius
    ASSERT_EQ(Object(Radii()), map.get(kPropertyBorderRadii));

    // ********* Set the State to PRESSED **********

    component->setState(kStatePressed, true);

    // We should get four dirty properties: the four corners
    ASSERT_TRUE(CheckDirty(component, kPropertyBorderRadii, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component));

    // Check the assignments.  The main border radius should go to 100.
    map = component->getCalculated();
    ASSERT_EQ(Object(Dimension(100)), map.get(kPropertyBorderRadius));

    // The assigned values are null
    ASSERT_EQ(Object::NULL_OBJECT(), map.get(kPropertyBorderBottomLeftRadius));
    ASSERT_EQ(Object::NULL_OBJECT(), map.get(kPropertyBorderBottomRightRadius));
    ASSERT_EQ(Object::NULL_OBJECT(), map.get(kPropertyBorderTopLeftRadius));
    ASSERT_EQ(Object::NULL_OBJECT(), map.get(kPropertyBorderTopRightRadius));

    // The output values match the main border radius
    ASSERT_EQ(Object(Radii(100)), map.get(kPropertyBorderRadii));

    // ********* Add the KARAOKE state which overrides the borderRadius *******

    component->setState(kStateKaraoke, true);

    // We should get four dirty properties: the four corners
    ASSERT_TRUE(CheckDirty(component, kPropertyBorderRadii, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component));

    // Check the assignments.  The main border radius should still be 100.
    map = component->getCalculated();
    ASSERT_EQ(Object(Dimension(100)), map.get(kPropertyBorderRadius));

    // The assigned values are null
    ASSERT_EQ(Object(Dimension(1)), map.get(kPropertyBorderBottomLeftRadius));
    ASSERT_EQ(Object(Dimension(2)), map.get(kPropertyBorderBottomRightRadius));
    ASSERT_EQ(Object(Dimension(3)), map.get(kPropertyBorderTopLeftRadius));
    ASSERT_EQ(Object(Dimension(4)), map.get(kPropertyBorderTopRightRadius));

    // The output values match the main border radius
    ASSERT_EQ(Object(Radii(3,4,1,2)), map.get(kPropertyBorderRadii));

    // ********* Remove the PRESSED state *************************

    component->setState(kStatePressed, false);

    // We should get no dirty properties, because the individual corners haven't changed
    ASSERT_TRUE(CheckDirty(root));

    // Check the assignments.  The main border radius should still be 100.
    map = component->getCalculated();
    ASSERT_EQ(Object(Dimension(0)), map.get(kPropertyBorderRadius));

    // The assigned values are null
    ASSERT_EQ(Object(Dimension(1)), map.get(kPropertyBorderBottomLeftRadius));
    ASSERT_EQ(Object(Dimension(2)), map.get(kPropertyBorderBottomRightRadius));
    ASSERT_EQ(Object(Dimension(3)), map.get(kPropertyBorderTopLeftRadius));
    ASSERT_EQ(Object(Dimension(4)), map.get(kPropertyBorderTopRightRadius));

    // The output values match the main border radius
    ASSERT_EQ(Object(Radii(3,4,1,2)), map.get(kPropertyBorderRadii));
}

static const char *STYLE_FRAME_INNER_BOUNDS = R"({
  "type": "APL",
  "version": "1.0",
  "styles": {
    "frameStyle": {
      "values": [
        {
          "borderWidth": 0
        },
        {
          "when": "${state.pressed}",
          "borderWidth": 100
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "style": "frameStyle",
      "width": "100%",
      "height": "100%",
      "item": {
        "type": "Image",
        "width": "100%",
        "height": "100%",
        "paddingLeft": 100,
        "paddingRight": 100,
        "paddingTop": 100,
        "paddingBottom": 100
      }
    }
  }
})";

TEST_F(FrameComponentTest, StyleFrameInnerBounds)
{
    loadDocument(STYLE_FRAME_INNER_BOUNDS);

    auto image = component->getChildAt(0);
    auto width = metrics.getWidth();
    auto height = metrics.getHeight();

    ASSERT_EQ(Rect(0, 0, width, height), component->getCalculated(kPropertyInnerBounds).get<Rect>());
    ASSERT_EQ(Rect(100, 100, width - 200, height - 200), image->getCalculated(kPropertyInnerBounds).get<Rect>());

    component->setState(kStatePressed, true);
    root->clearPending();

    ASSERT_EQ(Rect(100, 100, width - 200, height - 200), component->getCalculated(kPropertyInnerBounds).get<Rect>());
    ASSERT_EQ(Rect(100, 100, width - 400, height - 400), image->getCalculated(kPropertyInnerBounds).get<Rect>());
}

static const char *FRAME_BACKGROUND_OPTIONS = R"({
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "width": "100%",
      "height": "100%",
      "items": [
        {
          "type": "Frame",
          "width": "33%",
          "height": "33%",
          "backgroundColor": "red"
        },
        {
          "type": "Frame",
          "width": "33%",
          "height": "33%",
          "background": "red"
        },
        {
          "type": "Frame",
          "width": "33%",
          "height": "33%",
          "background": {
            "type": "linear",
            "colorRange": [ "#FF000066", "#F7C10066", "#6DD40066", "#0091FF66", "#6236FF66"],
            "inputRange": [ 0, 0.25, 0.55, 0.8, 1.0 ],
            "angle": 270
          }
        }
      ]
    }
  }
})";

TEST_F(FrameComponentTest, FrameBackgroundOptions)
{
    loadDocument(FRAME_BACKGROUND_OPTIONS);

    ASSERT_EQ(Color(0xff0000ff), component->getChildAt(0)->getCalculated(apl::kPropertyBackground).asColor(session));
    ASSERT_EQ(Color(0xff0000ff), component->getChildAt(1)->getCalculated(apl::kPropertyBackground).asColor(session));
    auto gradient = component->getChildAt(2)->getCalculated(apl::kPropertyBackground).get<Gradient>();
    ASSERT_EQ(270, gradient.getProperty(apl::kGradientPropertyAngle).getInteger());
}

static const char *FRAME_BACKGROUND_OVERRIDE = R"({
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "items": {
        "type": "Frame",
        "width": "100%",
        "height": "100%",
        "backgroundColor": "red",
        "background": "blue"
      }
    }
  }
})";

TEST_F(FrameComponentTest, FrameBackgroundOverride)
{
    loadDocument(FRAME_BACKGROUND_OVERRIDE);

    ASSERT_EQ(Color(0x0000ffff), component->getCalculated(apl::kPropertyBackground).asColor(session));
}

static const char *STYLE_FRAME_BACKGROUND_FROM_COLOR = R"({
  "type": "APL",
  "version": "2023.3",
  "styles": {
    "FrameStyle": {
      "values": [
        {
          "backgroundColor": "red"
        },
        {
          "when": "${state.pressed}",
          "background": "blue"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "style": "FrameStyle",
      "width": "100%",
      "height": "100%"
    }
  }
})";

TEST_F(FrameComponentTest, StyleFrameBackgroundFromColor)
{
    loadDocument(STYLE_FRAME_BACKGROUND_FROM_COLOR);

    ASSERT_TRUE(component->getCalculated(apl::kPropertyBackground).is<Color>());
    ASSERT_EQ(0xff0000ff, component->getCalculated(apl::kPropertyBackground).getColor());

    component->setState(kStatePressed, true);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyBackground, kPropertyVisualHash));

    ASSERT_TRUE(component->getCalculated(apl::kPropertyBackground).is<Color>());
    ASSERT_EQ(0x0000ffff, component->getCalculated(apl::kPropertyBackground).getColor());
}

static const char *STYLE_FRAME_BACKGROUND_TO_GRADIENT = R"({
  "type": "APL",
  "version": "2023.3",
  "styles": {
    "FrameStyle": {
      "values": [
        {
          "background": "red"
        },
        {
          "when": "${state.pressed}",
          "background": {
            "type": "linear",
            "colorRange": [ "#FF000066", "#F7C10066", "#6DD40066", "#0091FF66", "#6236FF66"],
            "inputRange": [ 0, 0.25, 0.55, 0.8, 1.0 ],
            "angle": 270
          }
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "style": "FrameStyle",
      "width": "100%",
      "height": "100%"
    }
  }
})";

TEST_F(FrameComponentTest, StyleFrameBackgroundToGradient)
{
    loadDocument(STYLE_FRAME_BACKGROUND_TO_GRADIENT);

    ASSERT_TRUE(component->getCalculated(apl::kPropertyBackground).is<Color>());
    ASSERT_EQ(0xff0000ff, component->getCalculated(apl::kPropertyBackground).getColor());

    component->setState(kStatePressed, true);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyBackground, kPropertyVisualHash));

    ASSERT_TRUE(component->getCalculated(apl::kPropertyBackground).is<Gradient>());
}

static const char *STYLE_FRAME_COLOR_TO_BACKGROUND_OVERRIDE = R"({
  "type": "APL",
  "version": "2023.3",
  "styles": {
    "FrameStyle": {
      "values": [
        {
          "background": "red"
        },
        {
          "when": "${state.pressed}",
          "backgroundColor": "green",
          "background": "blue"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "style": "FrameStyle",
      "width": "100%",
      "height": "100%"
    }
  }
})";

TEST_F(FrameComponentTest, StyleFrameColorToBackgroundOverride)
{
    loadDocument(STYLE_FRAME_COLOR_TO_BACKGROUND_OVERRIDE);

    ASSERT_TRUE(component->getCalculated(apl::kPropertyBackground).is<Color>());
    ASSERT_EQ(0xff0000ff, component->getCalculated(apl::kPropertyBackground).getColor());

    component->setState(kStatePressed, true);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyBackgroundColor, kPropertyBackground, kPropertyVisualHash));

    ASSERT_EQ(0x0000ffff, component->getCalculated(apl::kPropertyBackground).getColor());
}

static const char *STYLE_FRAME_BACKGROUND_TO_COLOR = R"({
  "type": "APL",
  "version": "2023.3",
  "styles": {
    "FrameStyle": {
      "values": [
        {
          "background": "red"
        },
        {
          "when": "${state.pressed}",
          "backgroundColor": "green"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "style": "FrameStyle",
      "width": "100%",
      "height": "100%"
    }
  }
})";

TEST_F(FrameComponentTest, StyleFrameBackgroundToColor)
{
    loadDocument(STYLE_FRAME_BACKGROUND_TO_COLOR);

    ASSERT_TRUE(component->getCalculated(apl::kPropertyBackground).is<Color>());
    ASSERT_EQ(0xff0000ff, component->getCalculated(apl::kPropertyBackground).getColor());

    component->setState(kStatePressed, true);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyBackgroundColor, kPropertyVisualHash));

    // Can't override preferred "background", which is defined in the base style.
    ASSERT_EQ(0xff0000ff, component->getCalculated(apl::kPropertyBackground).getColor());
}

static const char *STYLE_FRAME_COLOR_TO_TRANSPARENT_BACKGROUND_OVERRIDE = R"({
  "type": "APL",
  "version": "2023.3",
  "styles": {
    "FrameStyle": {
      "values": [
        {
          "backgroundColor": "red"
        },
        {
          "when": "${state.pressed}",
          "backgroundColor": "red",
          "background": "transparent"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "style": "FrameStyle",
      "width": "100%",
      "height": "100%"
    }
  }
})";

TEST_F(FrameComponentTest, StyleFrameColorToTransparentBackgroundOverride)
{
    loadDocument(STYLE_FRAME_COLOR_TO_TRANSPARENT_BACKGROUND_OVERRIDE);

    ASSERT_TRUE(component->getCalculated(apl::kPropertyBackground).is<Color>());
    ASSERT_EQ(0xff0000ff, component->getCalculated(apl::kPropertyBackground).getColor());

    component->setState(kStatePressed, true);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyBackground, kPropertyVisualHash));

    ASSERT_EQ(0x00000000, component->getCalculated(apl::kPropertyBackground).getColor());
}

static const char *FRAME_COLOR_TO_TRANSPARENT_BACKGROUND_OVERRIDE = R"({
  "type": "APL",
  "version": "2023.3",
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "style": "FrameStyle",
      "width": "100%",
      "height": "100%",
      "backgroundColor": "red",
      "background": "transparent"
    }
  }
})";

TEST_F(FrameComponentTest, FrameColorToTransparentBackgroundOverride)
{
    loadDocument(FRAME_COLOR_TO_TRANSPARENT_BACKGROUND_OVERRIDE);

    ASSERT_TRUE(component->getCalculated(apl::kPropertyBackground).is<Color>());
    ASSERT_EQ(0x00000000, component->getCalculated(apl::kPropertyBackground).getColor());
}
