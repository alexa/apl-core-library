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
#include "apl/time/sequencer.h"

using namespace apl;

/**
 * The AnimateItem command can animate bound values as well as properties.  These unit
 * tests focus on bound values
 */

class AnimateItemValueTest : public CommandTest {};

static const char *VALUE_ANIMATION = R"apl(
    {
      "type": "APL",
      "version": "1.8",
      "commands": {
        "ChangeValue": {
          "parameters": "TO",
          "command": {
            "type": "AnimateItem",
            "componentId": "ValueTest",
            "easing": "linear",
            "duration": 1000,
            "values": [
              {
                "property": "VALUE",
                "to": "${TO}"
              }
            ]
          }
        }
      },
      "mainTemplate": {
        "item": {
          "type": "Text",
          "id": "ValueTest",
          "bind": {
            "name": "VALUE",
            "value": 0.0
          },
          "text": "${VALUE}"
        }
      }
    }
)apl";

TEST_F(AnimateItemValueTest, ValueAnimation)
{
    loadDocument(VALUE_ANIMATION);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual("0", component->getCalculated(kPropertyText).asString()));

    // Animate forwards to 1000
    executeCommand("ChangeValue", {{"TO", 1000}}, false);
    auto startTime = loop->currentTime();
    for (int i = 0; i <= 1000; i += 100) {
        loop->advanceToTime(startTime + i);

        // Convert the text back into a numeric value to simplify the comparison
        auto value = std::stod(component->getCalculated(kPropertyText).asString());
        ASSERT_NEAR(i, value, 0.001);
    }

    // Animate backwards to -1000
    executeCommand("ChangeValue", {{"TO", -1000}}, false);
    startTime = loop->currentTime();
    for (int i = 0; i <= 1000; i += 100) {
        loop->advanceToTime(startTime + i);

        // Convert the text back into a numeric value to simplify the comparison
        auto value = std::stod(component->getCalculated(kPropertyText).asString());
        ASSERT_NEAR(1000 - i*2, value, 0.001);   // We step by 200 each time
    }
}

static const char *VALUE_FROM_ANIMATION = R"apl(
    {
      "type": "APL",
      "version": "1.8",
      "commands": {
        "ChangeValue": {
          "parameters": [
            "TO",
            "FROM"
          ],
          "command": {
            "type": "AnimateItem",
            "componentId": "ValueTest",
            "easing": "linear",
            "duration": 1000,
            "values": [
              {
                "property": "VALUE",
                "to": "${TO}",
                "from": "${FROM}"
              }
            ]
          }
        }
      },
      "mainTemplate": {
        "item": {
          "type": "Text",
          "id": "ValueTest",
          "bind": {
            "name": "VALUE",
            "value": 0.0
          },
          "text": "${VALUE}"
        }
      }
    }
)apl";

TEST_F(AnimateItemValueTest, ValueFromAnimation)
{
    loadDocument(VALUE_FROM_ANIMATION);
    ASSERT_TRUE(component);

    // Animate between -1000 to 1000
    executeCommand("ChangeValue", {{"FROM", -1000}, {"TO", 1000}}, false);
    auto startTime = loop->currentTime();
    for (int i = 0; i <= 1000; i += 100) {
        loop->advanceToTime(startTime + i);

        // Convert the text back into a numeric value to simplify the comparison
        auto value = std::stod(component->getCalculated(kPropertyText).asString());
        ASSERT_NEAR(-1000 + 2 * i, value, 0.001);
    }
}

TEST_F(AnimateItemValueTest, FastMode)
{
    loadDocument(VALUE_FROM_ANIMATION);
    ASSERT_TRUE(component);

    // Animate between -1000 to 1000, but run in fast mode (should jump to the end)
    executeCommand("ChangeValue", {{"FROM", -1000}, {"TO", 1000}}, true);
    auto startTime = loop->currentTime();
    for (int i = 0; i <= 1000; i += 100) {
        loop->advanceToTime(startTime + i);

        // Convert the text back into a numeric value to simplify the comparison
        auto value = std::stod(component->getCalculated(kPropertyText).asString());
        ASSERT_NEAR(1000, value, 0.001);
    }
}

TEST_F(AnimateItemValueTest, InteruptWithSetValue)
{
    loadDocument(VALUE_FROM_ANIMATION);
    ASSERT_TRUE(component);

    // Animate between -1000 to 1000
    executeCommand("ChangeValue", {{"FROM", -1000}, {"TO", 1000}}, false);
    auto startTime = loop->currentTime();
    for (int i = 0; i <= 500; i += 100) {
        loop->advanceToTime(startTime + i);

        // Convert the text back into a numeric value to simplify the comparison
        auto value = std::stod(component->getCalculated(kPropertyText).asString());
        ASSERT_NEAR(-1000 + 2 * i, value, 0.001);
    }

    // Now interrupt everything by setting the value explicitly.  This should kill the animation
    executeCommand("SetValue", {{"componentId", "ValueTest"}, {"property", "VALUE"}, {"value", 2000}}, false);

    // Run time forwards to verify that nothing changes
    for (int i = 500; i <= 1000; i += 100) {
        loop->advanceToTime(startTime + i);

        // Convert the text back into a numeric value to simplify the comparison
        auto value = std::stod(component->getCalculated(kPropertyText).asString());
        ASSERT_NEAR(2000, value, 0.001);
    }
}

static const char * INTERRUPT_WITH_ANIMATION = R"apl(
    {
      "type": "APL",
      "version": "1.8",
      "commands": {
        "ChangeValue": {
          "parameters": [
            "TO",
            "DURATION"
          ],
          "command": {
            "type": "AnimateItem",
            "componentId": "ValueTest",
            "easing": "linear",
            "duration": "${DURATION}",
            "values": {
              "property": "VALUE",
              "to": "${TO}"
            }
          }
        }
      },
      "mainTemplate": {
        "item": {
          "type": "Text",
          "id": "ValueTest",
          "bind": {
            "name": "VALUE",
            "value": 0.0
          },
          "text": "${VALUE}"
        }
      }
    }
)apl";

TEST_F(AnimateItemValueTest, InterruptWithAnimation)
{
    loadDocument(INTERRUPT_WITH_ANIMATION);
    ASSERT_TRUE(component);

    // Animate to 1000 over 5000 milliseconds
    executeCommand("ChangeValue", {{"TO", 1000}, {"DURATION", 5000}}, false);
    auto startTime = loop->currentTime();
    for (int i = 0; i <= 2500; i += 100) {
        loop->advanceToTime(startTime + i);
        auto value = std::stod(component->getCalculated(kPropertyText).asString());
        ASSERT_NEAR(i / 5, value, 0.001);
    }

    // We've gone 2500 milliseconds and should be on the value 500
    // Start a second animation that only lasts 1000 milliseconds.
    // This should cancel the first animation - which causes it to jump to the end state (of 1000)
    executeCommand("ChangeValue", {{"TO", 2000}, {"DURATION", 1000}}, false);
    startTime = loop->currentTime();
    for (int i = 0; i <= 1000; i += 100) {
        loop->advanceToTime(startTime + i);
        auto value = std::stod(component->getCalculated(kPropertyText).asString());
        ASSERT_NEAR(1000 + i, value, 0.001);
    }

    // Both animations are done, so advancing time shouldn't matter
    startTime = loop->currentTime();
    for (int i = 0; i <= 1500; i += 100) {
        loop->advanceToTime(startTime + i);
        auto value = std::stod(component->getCalculated(kPropertyText).asString());
        ASSERT_NEAR(2000, value, 0.001);
    }
}


static const char *BAD_VALUES_TO_ANIMATE = R"apl(
    {
      "type": "APL",
      "version": "1.8",
      "commands": {
        "ChangeValue": {
          "parameters": "ITEM",
          "command": {
            "type": "AnimateItem",
            "componentId": "ValueTest",
            "easing": "linear",
            "duration": 1000,
            "values": {
              "property": "${ITEM}",
              "from": 0,
              "to": 1000
            }
          }
        }
      },
      "mainTemplate": {
        "item": {
          "type": "Text",
          "id": "ValueTest",
          "bind": {
            "name": "VALUE",
            "value": "I am a string"
          },
          "text": "${VALUE}"
        }
      }
    }
)apl";

TEST_F(AnimateItemValueTest, BadValuesToAnimate)
{
    loadDocument(BAD_VALUES_TO_ANIMATE);
    ASSERT_TRUE(component);

    // Try to animate a variable that doesn't exist
    executeCommand("ChangeValue", {{"ITEM", "VALUE2"}}, false);
    ASSERT_TRUE(ConsoleMessage());

    // Try to animate a string
    executeCommand("ChangeValue", {{"ITEM", "VALUE"}}, false);
    ASSERT_TRUE(ConsoleMessage());

    // Try to animate a system-only property (non-writeable)
    executeCommand("ChangeValue", {{"ITEM", "elapsedTime"}}, false);
    ASSERT_TRUE(ConsoleMessage());

    // Unrecognized animation command
    executeCommand("AnimateItem", {{"componentId", "ValueTest"}, {"duration", 1000}, {"values", "X"}}, false);
    ASSERT_TRUE(ConsoleMessage());

    // Pick a valid component property that cannot be animated
    executeCommand("ChangeValue", {{"ITEM", "bounds"}}, false);  // "bounds" is a valid output property
    ASSERT_TRUE(ConsoleMessage());

    // Pick a valid component property that is dynamic, but _not_ opacity
    executeCommand("ChangeValue", {{"ITEM", "minHeight"}}, false);
    ASSERT_TRUE(ConsoleMessage());
}


static const char *ANIMATE_VG = R"apl(
    {
      "type": "APL",
      "version": "1.7",
      "graphics": {
        "Box": {
          "type": "AVG",
          "version": "1.2",
          "height": 100,
          "width": 100,
          "parameters": [
            {
              "name": "W",
              "default": 1
            }
          ],
          "items": {
            "type": "path",
            "pathData": "M25,25 h50 v50 h-50 z",
            "stroke": "blue",
            "strokeWidth": "${W * 50}"
          }
        }
      },
      "commands": {
        "ChangeValue": {
          "parameters": "TO",
          "command": {
            "type": "AnimateItem",
            "componentId": "MYBOX",
            "easing": "linear",
            "duration": 1000,
            "values": {
              "property": "W",
              "to": "${TO}"
            }
          }
        }
      },
      "mainTemplate": {
        "items": [
          {
            "type": "VectorGraphic",
            "id": "MYBOX",
            "source": "Box"
          }
        ]
      }
    }
)apl";

TEST_F(AnimateItemValueTest, AnimateVG)
{
    loadDocument(ANIMATE_VG);
    ASSERT_TRUE(component);

    auto graphic = component->getCalculated(apl::kPropertyGraphic).getGraphic();
    ASSERT_TRUE(graphic);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    ASSERT_EQ(1, container->getChildCount());
    auto path = container->getChildAt(0);
    ASSERT_TRUE(path);

    ASSERT_EQ(50.0, path->getValue(kGraphicPropertyStrokeWidth).getDouble());

    // Animate the "W" parameter in the vector graphic
    executeCommand("ChangeValue", {{"TO", 0.0}}, false);
    loop->advanceToEnd();
    ASSERT_EQ(0.0, path->getValue(kGraphicPropertyStrokeWidth).getDouble());

    // Bring it back up
    executeCommand("ChangeValue", {{"TO", 1.0}}, false);
    ASSERT_EQ(0.0, path->getValue(kGraphicPropertyStrokeWidth).getDouble());
    loop->advanceBy(500);
    ASSERT_EQ(25.0, path->getValue(kGraphicPropertyStrokeWidth).getDouble());
    loop->advanceToEnd();
    ASSERT_EQ(50.0, path->getValue(kGraphicPropertyStrokeWidth).getDouble());
}


static const char *BAD_VG_PARAMETERS = R"apl(
    {
      "type": "APL",
      "version": "1.7",
      "graphics": {
        "Box": {
          "type": "AVG",
          "version": "1.2",
          "height": 100,
          "width": 100,
          "parameters": [
            {
              "name": "COLOR",
              "default": "blue"
            },
            {
              "name": "WIDTH",
              "default": 10
            }
          ],
          "items": {
            "type": "path",
            "pathData": "M25,25 h50 v50 h-50 z",
            "stroke": "${COLOR}",
            "strokeWidth": "${WIDTH}"
          }
        }
      },
      "commands": {
        "ChangeValue": {
          "parameters": [ "PARAM", "TO" ],
          "command": {
            "type": "AnimateItem",
            "componentId": "MYBOX",
            "easing": "linear",
            "duration": 1000,
            "values": {
              "property": "${PARAM}",
              "to": "${TO}"
            }
          }
        }
      },
      "mainTemplate": {
        "items": [
          {
            "type": "VectorGraphic",
            "id": "MYBOX",
            "source": "Box"
          }
        ]
      }
    }
)apl";

TEST_F(AnimateItemValueTest, BadVGParameters)
{
    loadDocument(BAD_VG_PARAMETERS);
    ASSERT_TRUE(component);

    auto graphic = component->getCalculated(apl::kPropertyGraphic).getGraphic();
    ASSERT_TRUE(graphic);

    auto container = graphic->getRoot();
    ASSERT_TRUE(container);

    ASSERT_EQ(1, container->getChildCount());
    auto path = container->getChildAt(0);
    ASSERT_TRUE(path);

    ASSERT_EQ(10.0, path->getValue(kGraphicPropertyStrokeWidth).getDouble());
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), path->getValue(kGraphicPropertyStroke)));

    // Animate the "WIDTH" parameter in the vector graphic to make sure this worked
    executeCommand("ChangeValue", {{"TO", 0.0}, {"PARAM", "WIDTH"}}, false);
    loop->advanceToEnd();
    ASSERT_EQ(0.0, path->getValue(kGraphicPropertyStrokeWidth).getDouble());

    // Try to change the color.  This fails; only numbers and dimensions can be animated
    executeCommand("ChangeValue", {{"TO", "red"}, {"PARAM", "COLOR"}}, false);
    ASSERT_TRUE(ConsoleMessage());
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), path->getValue(kGraphicPropertyStroke)));

    // Animate a property that doesn't exist; this should trigger the default from getPropertyInternal
    executeCommand("ChangeValue", {{"TO", "red"}, {"PARAM", "FOO"}}, false);
    ASSERT_TRUE(ConsoleMessage());
}

static const char *NO_VG = R"apl(
    {
      "type": "APL",
      "version": "1.7",
      "commands": {
        "ChangeValue": {
          "parameters": [ "PARAM", "TO" ],
          "command": {
            "type": "AnimateItem",
            "componentId": "MYBOX",
            "easing": "linear",
            "duration": 1000,
            "values": {
              "property": "${PARAM}",
              "to": "${TO}"
            }
          }
        }
      },
      "mainTemplate": {
        "items": [
          {
            "type": "VectorGraphic",
            "id": "MYBOX"
          }
        ]
      }
    }
)apl";

TEST_F(AnimateItemValueTest, NoVG)
{
    loadDocument(NO_VG);
    ASSERT_TRUE(component);

    auto graphic = component->getCalculated(apl::kPropertyGraphic);
    ASSERT_FALSE(graphic.isGraphic());

    // Animate a property that doesn't exist
    executeCommand("ChangeValue", {{"TO", "red"}, {"PARAM", "COLOR"}}, false);
    ASSERT_TRUE(ConsoleMessage());
}