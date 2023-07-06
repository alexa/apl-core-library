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

class DynamicContainerProperties : public DocumentWrapper {};

static const char *CONTAINER_ALIGN_ITEMS = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "styles": {
        "CStyle": {
          "values": [
            {
              "alignItems": "start",
              "direction": "column"
            },
            {
              "when": "${state.checked}",
              "alignItems": "end"
            }
          ]
        }
      },
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": 500,
          "height": 500,
          "style": "CStyle",
          "items": {
            "type": "Text",
            "text": "Item ${data}",
            "width": 50,
            "height": 50
          },
          "data": "${Array.range(4)}"
        }
      }
    }
)apl";

/*
 * Demonstrate that the "alignItems" property in a container can be styled and set dynamically
 */
TEST_F(DynamicContainerProperties, ContainerAlignItems)
{
    loadDocument(CONTAINER_ALIGN_ITEMS);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual(Rect(0, 50, 50, 50), component->getChildAt(1)->getCalculated(kPropertyBounds)));

    // Checking the container switches the "alignItems" property from start (left) to end (right)
    executeCommand("SetValue", {{"componentId", component->getUniqueId()},
                                {"property", "checked"},
                                {"value", true}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(450, 50, 50, 50), component->getChildAt(1)->getCalculated(kPropertyBounds)));

    // Assigning a value to "alignItems" overrides the style
    executeCommand("SetValue", {{"componentId", component->getUniqueId()},
                                {"property", "alignItems"},
                                {"value", "center"}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(225, 50, 50, 50), component->getChildAt(1)->getCalculated(kPropertyBounds)));

    // Changing the style now won't affect the layout
    executeCommand("SetValue", {{"componentId", component->getUniqueId()},
                                {"property", "checked"},
                                {"value", false}}, true);

    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(225, 50, 50, 50), component->getChildAt(1)->getCalculated(kPropertyBounds)));
}


static const char *CONTAINER_DIRECTION = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "styles": {
        "CStyle": {
          "values": [
            {
              "alignItems": "start",
              "direction": "column"
            },
            {
              "when": "${state.checked}",
              "direction": "row"
            }
          ]
        }
      },
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": 500,
          "height": 500,
          "style": "CStyle",
          "items": {
            "type": "Text",
            "text": "Item ${data}",
            "width": 50,
            "height": 50
          },
          "data": "${Array.range(4)}"
        }
      }
    }
)apl";

/*
 * Demonstrate that the "direction" property in a container can be styled and set dynamically
 */
TEST_F(DynamicContainerProperties, ContainerDirection)
{
    loadDocument(CONTAINER_DIRECTION);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual(Rect(0, 50, 50, 50), component->getChildAt(1)->getCalculated(kPropertyBounds)));

    // Checking the container switches the "direction" property from column to row
    executeCommand("SetValue", {{"componentId", component->getUniqueId()},
                                {"property", "checked"},
                                {"value", true}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(50, 0, 50, 50), component->getChildAt(1)->getCalculated(kPropertyBounds)));

    // Assigning a value to "direction" overrides the style
    executeCommand("SetValue", {{"componentId", component->getUniqueId()},
                                {"property", "direction"},
                                {"value", "column"}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(0, 50, 50, 50), component->getChildAt(1)->getCalculated(kPropertyBounds)));
}


static const char *CONTAINER_JUSTIFY_CONTENT = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "styles": {
        "CStyle": {
          "values": [
            {
              "alignItems": "start",
              "justifyContent": "start",
              "direction": "column"
            },
            {
              "when": "${state.checked}",
              "justifyContent": "end"
            }
          ]
        }
      },
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": 500,
          "height": 500,
          "style": "CStyle",
          "items": {
            "type": "Text",
            "text": "Item ${data}",
            "width": 50,
            "height": 50
          },
          "data": "${Array.range(4)}"
        }
      }
    }
)apl";

/*
 * Demonstrate that the "justifyContent" property in a container can be styled and set dynamically
 */
TEST_F(DynamicContainerProperties, ContainerJustifyContent)
{
    loadDocument(CONTAINER_JUSTIFY_CONTENT);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual(Rect(0, 50, 50, 50), component->getChildAt(1)->getCalculated(kPropertyBounds)));

    // Checking the container switches the "justifyContent" property from start to end, pushing the components down
    executeCommand("SetValue", {{"componentId", component->getUniqueId()},
                                {"property", "checked"},
                                {"value", true}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(0, 350, 50, 50), component->getChildAt(1)->getCalculated(kPropertyBounds)));

    // Assigning a value to "justifyContent" overrides the style.  100 units of space will be inserted between each pair
    executeCommand("SetValue", {{"componentId", component->getUniqueId()},
                                {"property", "justifyContent"},
                                {"value", "spaceBetween"}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(0, 150, 50, 50), component->getChildAt(1)->getCalculated(kPropertyBounds)));
}


static const char *CONTAINER_WRAP = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "styles": {
        "CStyle": {
          "values": [
            {
              "alignItems": "start",
              "wrap": "noWrap",
              "direction": "column"
            },
            {
              "when": "${state.checked}",
              "wrap": "wrap"
            }
          ]
        }
      },
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": 500,
          "height": 500,
          "style": "CStyle",
          "items": {
            "type": "Text",
            "text": "Item ${data}",
            "width": 100,
            "height": 200
          },
          "data": "${Array.range(4)}"
        }
      }
    }
)apl";

/*
 * Demonstrate that the "wrap" property in a container can be styled and set dynamically
 */
TEST_F(DynamicContainerProperties, ContainerWrap)
{
    loadDocument(CONTAINER_WRAP);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual(Rect(0, 200, 100, 200), component->getChildAt(1)->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0, 400, 100, 200), component->getChildAt(2)->getCalculated(kPropertyBounds)));

    // Checking the container switches the "wrap" property from noWrap to wrap
    executeCommand("SetValue", {{"componentId", component->getUniqueId()},
                                {"property", "checked"},
                                {"value", true}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(0, 200, 100, 200), component->getChildAt(1)->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(100, 0, 100, 200), component->getChildAt(2)->getCalculated(kPropertyBounds)));  // Wrapped to next column

    // Assigning a value to "wrap" overrides the style. Setting 'wrapReverse' shifts the first column all the way to the right
    executeCommand("SetValue", {{"componentId", component->getUniqueId()},
                                {"property", "wrap"},
                                {"value", "wrapReverse"}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(400, 200, 100, 200), component->getChildAt(1)->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(300, 0, 100, 200), component->getChildAt(2)->getCalculated(kPropertyBounds)));  // Wrapped to first column
}


static const char *CONTAINER_CHILD_ALIGN_SELF = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "styles": {
        "CStyle": {
          "values": [
            {
              "alignItems": "center",
              "direction": "column"
            }
          ]
        },
        "ChildStyle": {
          "values": [
            {
              "alignSelf": "start"
            },
            {
              "when": "${state.checked}",
              "alignSelf": "end"
            }
          ]
        }
      },
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": 500,
          "height": 500,
          "style": "CStyle",
          "items": {
            "type": "Text",
            "style": "ChildStyle",
            "text": "Item ${data}",
            "width": 100,
            "height": 200
          },
          "data": "${Array.range(4)}"
        }
      }
    }
)apl";

/*
 * Demonstrate that the "alignSelf" property in the child of a container can be styled and set dynamically
 */
TEST_F(DynamicContainerProperties, ContainerChildAlignSelf)
{
    loadDocument(CONTAINER_CHILD_ALIGN_SELF);
    ASSERT_TRUE(component);
    auto child = component->getChildAt(1);

    ASSERT_TRUE(IsEqual(Rect(0, 200, 100, 200), child->getCalculated(kPropertyBounds)));

    // Checking the child switches the "alignSelf" property from start to end
    executeCommand("SetValue", {{"componentId", child->getUniqueId()},
                                {"property", "checked"},
                                {"value", true}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(400, 200, 100, 200), child->getCalculated(kPropertyBounds)));

    // Assigning a value to "alignSelf" overrides the style.
    executeCommand("SetValue", {{"componentId", child->getUniqueId()},
                                {"property", "alignSelf"},
                                {"value", "center"}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(200, 200, 100, 200), child->getCalculated(kPropertyBounds)));
}


static const char *CONTAINER_CHILD_LEFT_TOP = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "styles": {
        "CStyle": {
          "values": [
            {
              "alignItems": "start",
              "direction": "column"
            }
          ]
        },
        "ChildStyle": {
          "values": [
            {
              "left": 10,
              "top": 20
            },
            {
              "when": "${state.checked}",
              "left": 30,
              "top": 40
            }
          ]
        }
      },
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": 500,
          "height": 500,
          "style": "CStyle",
          "items": {
            "type": "Text",
            "style": "ChildStyle",
            "text": "Item ${data}",
            "width": 100,
            "height": 200
          },
          "data": "${Array.range(4)}"
        }
      }
    }
)apl";

/*
 * Demonstrate that the "left" and "top" properties in the child of a container can be styled and set dynamically
 */
TEST_F(DynamicContainerProperties, ContainerChildLeftTop)
{
    loadDocument(CONTAINER_CHILD_LEFT_TOP);
    ASSERT_TRUE(component);
    auto child = component->getChildAt(1);

    ASSERT_TRUE(IsEqual(Rect(10, 220, 100, 200), child->getCalculated(kPropertyBounds)));

    // Checking the child switches the "left/top" properties
    executeCommand("SetValue", {{"componentId", child->getUniqueId()},
                                {"property", "checked"},
                                {"value", true}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(30, 240, 100, 200), child->getCalculated(kPropertyBounds)));

    // Assigning a value to "left" overrides the style.
    executeCommand("SetValue", {{"componentId", child->getUniqueId()},
                                {"property", "left"},
                                {"value", 75}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(75, 240, 100, 200), child->getCalculated(kPropertyBounds)));
}


static const char *CONTAINER_CHILD_RIGHT_BOTTOM = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "styles": {
        "CStyle": {
          "values": [
            {
              "alignItems": "start",
              "direction": "column"
            }
          ]
        },
        "ChildStyle": {
          "values": [
            {
              "right": 10,
              "bottom": 20
            },
            {
              "when": "${state.checked}",
              "right": 30,
              "bottom": 40
            }
          ]
        }
      },
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": 500,
          "height": 500,
          "style": "CStyle",
          "items": {
            "type": "Text",
            "style": "ChildStyle",
            "text": "Item ${data}",
            "width": 100,
            "height": 200
          },
          "data": "${Array.range(4)}"
        }
      }
    }
)apl";

/*
 * Demonstrate that the "right" and "bottom" properties in the child of a container can be styled and set dynamically
 */
TEST_F(DynamicContainerProperties, ContainerChildRightBottom)
{
    loadDocument(CONTAINER_CHILD_RIGHT_BOTTOM);
    ASSERT_TRUE(component);
    auto child = component->getChildAt(1);

    ASSERT_TRUE(IsEqual(Rect(-10, 180, 100, 200), child->getCalculated(kPropertyBounds)));

    // Checking the child switches the "right/bottom" properties
    executeCommand("SetValue", {{"componentId", child->getUniqueId()},
                                {"property", "checked"},
                                {"value", true}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(-30, 160, 100, 200), child->getCalculated(kPropertyBounds)));

    // Assigning a value to "bottom" overrides the style.
    executeCommand("SetValue", {{"componentId", child->getUniqueId()},
                                {"property", "bottom"},
                                {"value", 75}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(-30, 125, 100, 200), child->getCalculated(kPropertyBounds)));
}


static const char *CONTAINER_CHILD_POSITION = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "styles": {
        "CStyle": {
          "values": [
            {
              "alignItems": "end",
              "direction": "column"
            }
          ]
        },
        "ChildStyle": {
          "values": [
            {
              "top": 10,
              "left": 20,
              "position": "relative"
            },
            {
              "when": "${state.checked}",
              "position": "absolute"
            }
          ]
        }
      },
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": 500,
          "height": 500,
          "style": "CStyle",
          "items": {
            "type": "Text",
            "style": "ChildStyle",
            "text": "Item ${data}",
            "width": 100,
            "height": 200
          },
          "data": "${Array.range(4)}"
        }
      }
    }
)apl";

/*
 * Demonstrate that the "position" property can be switched between "relative" and "absolute"
 */
TEST_F(DynamicContainerProperties, ContainerChildPosition)
{
    loadDocument(CONTAINER_CHILD_POSITION);
    ASSERT_TRUE(component);
    auto child = component->getChildAt(1);

    ASSERT_TRUE(IsEqual(Rect(420, 210, 100, 200), child->getCalculated(kPropertyBounds)));

    // Checking the child switches to absolute position
    executeCommand("SetValue", {{"componentId", child->getUniqueId()},
                                {"property", "checked"},
                                {"value", true}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(20, 10, 100, 200), child->getCalculated(kPropertyBounds)));

    // Assigning a value to "position" overrides the style.
    executeCommand("SetValue", {{"componentId", child->getUniqueId()},
                                {"property", "position"},
                                {"value", "relative"}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(420, 210, 100, 200), child->getCalculated(kPropertyBounds)));
}


static const char *CONTAINER_CHILD_GROW = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "styles": {
        "CStyle": {
          "values": [
            {
              "alignItems": "start",
              "direction": "column"
            }
          ]
        },
        "ChildStyle": {
          "values": [
            {
              "grow": 1
            },
            {
              "when": "${state.checked}",
              "grow": 2
            }
          ]
        }
      },
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": 500,
          "height": 500,
          "style": "CStyle",
          "items": {
            "type": "Text",
            "style": "ChildStyle",
            "text": "Item ${data}",
            "width": 100,
            "height": 50
          },
          "data": "${Array.range(4)}"
        }
      }
    }
)apl";

/*
 * Demonstrate that the "grow" property of a child can be dynamically adjusted
 */
TEST_F(DynamicContainerProperties, ContainerChildGrow)
{
    loadDocument(CONTAINER_CHILD_GROW);
    ASSERT_TRUE(component);
    auto child = component->getChildAt(1);

    // 300 dp of extra space is divided equally between four children (+75 height)
    ASSERT_TRUE(IsEqual(Rect(0, 125, 100, 125), child->getCalculated(kPropertyBounds)));

    // Checking the child sets growth to "2", so regular children get +60 height and this child gets +120
    executeCommand("SetValue", {{"componentId", child->getUniqueId()},
                                {"property", "checked"},
                                {"value", true}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(0, 110, 100, 170), child->getCalculated(kPropertyBounds)));

    // Assigning a value to "growth" overrides the style.  Regular children now get +100; this child gets 0
    executeCommand("SetValue", {{"componentId", child->getUniqueId()},
                                {"property", "grow"},
                                {"value", 0}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(0, 150, 100, 50), child->getCalculated(kPropertyBounds)));
}



static const char *CONTAINER_CHILD_SHRINK = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "styles": {
        "CStyle": {
          "values": [
            {
              "alignItems": "start",
              "direction": "column"
            }
          ]
        },
        "ChildStyle": {
          "values": [
            {
              "shrink": 1
            },
            {
              "when": "${state.checked}",
              "shrink": 2
            }
          ]
        }
      },
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": 500,
          "height": 500,
          "style": "CStyle",
          "items": {
            "type": "Text",
            "style": "ChildStyle",
            "text": "Item ${data}",
            "width": 100,
            "height": 200
          },
          "data": "${Array.range(4)}"
        }
      }
    }
)apl";

/*
 * Demonstrate that the "shrink" property of a child can be dynamically adjusted
 */
TEST_F(DynamicContainerProperties, ContainerChildShrink)
{
    loadDocument(CONTAINER_CHILD_SHRINK);
    ASSERT_TRUE(component);
    auto child = component->getChildAt(1);

    // 300 dp of too much space is divided equally between four children (-75 height)
    ASSERT_TRUE(IsEqual(Rect(0, 125, 100, 125), child->getCalculated(kPropertyBounds)));

    // Checking the child sets shrink to "2", so regular children get -60 height and this child gets -120
    executeCommand("SetValue", {{"componentId", child->getUniqueId()},
                                {"property", "checked"},
                                {"value", true}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(0, 140, 100, 80), child->getCalculated(kPropertyBounds)));

    // Assigning a value to "shrink" overrides the style.  Regular children now get -100; this child doesn't change
    executeCommand("SetValue", {{"componentId", child->getUniqueId()},
                                {"property", "shrink"},
                                {"value", 0}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(0, 100, 100, 200), child->getCalculated(kPropertyBounds)));
}


static const char *CONTAINER_CHILD_SPACING = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "styles": {
        "CStyle": {
          "values": [
            {
              "alignItems": "start",
              "direction": "column"
            }
          ]
        },
        "ChildStyle": {
          "values": [
            {
              "spacing": 10
            },
            {
              "when": "${state.checked}",
              "spacing": 20
            }
          ]
        }
      },
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": 500,
          "height": 500,
          "style": "CStyle",
          "items": {
            "type": "Text",
            "style": "ChildStyle",
            "text": "Item ${data}",
            "width": 100,
            "height": 100
          },
          "data": "${Array.range(4)}"
        }
      }
    }
)apl";

/*
 * Demonstrate that the "spacing" property of a child can be dynamically adjusted
 */
TEST_F(DynamicContainerProperties, ContainerChildSpacing)
{
    loadDocument(CONTAINER_CHILD_SPACING);
    ASSERT_TRUE(component);
    auto child = component->getChildAt(1);
    auto lastChild = component->getChildAt(3);

    // All children have 10 units of spacing
    ASSERT_TRUE(IsEqual(Rect(0, 110, 100, 100), child->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0, 330, 100, 100), lastChild->getCalculated(kPropertyBounds)));

    // Checking the child sets the spacing to 20 units
    executeCommand("SetValue", {{"componentId", child->getUniqueId()},
                                {"property", "checked"},
                                {"value", true}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(0, 120, 100, 100), child->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0, 340, 100, 100), lastChild->getCalculated(kPropertyBounds)));

    // Assigning a value to "spacing" overrides the style.
    executeCommand("SetValue", {{"componentId", child->getUniqueId()},
                                {"property", "spacing"},
                                {"value", 50}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect(0, 150, 100, 100), child->getCalculated(kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(0, 370, 100, 100), lastChild->getCalculated(kPropertyBounds)));

}
