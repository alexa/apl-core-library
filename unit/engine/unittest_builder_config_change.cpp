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

class BuilderConfigChange : public DocumentWrapper {
public:
    ::testing::AssertionResult
    CheckChildStrings(const std::vector<std::string>& values) {
        if (values.size() != component->getChildCount())
            return ::testing::AssertionFailure() << "Wrong number of children, expected=" << values.size()
                << " actual=" << component->getChildCount();

        for (size_t i = 0; i < component->getChildCount(); i++) {
            auto expected = values.at(i);
            auto actual = component->getChildAt(i)->getCalculated(kPropertyText).asString();

            if (expected != actual)
                return ::testing::AssertionFailure() << "Mismatched text string, expected='" << expected
                    << "' actual='" << actual << "'";

        }

        return ::testing::AssertionSuccess();
    }
};

static const char *CHECK_ENVIRONMENT = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "item": {
          "type": "Text",
          "text": ""
        }
      },
      "onConfigChange": [
        {
          "type": "SendEvent",
          "sequencer": "DUMMY",
          "arguments": [
            "${event.source.type}",
            "${event.source.handler}",
            "${event.width}",
            "${event.height}",
            "${event.theme}",
            "${event.viewportMode}",
            "${event.fontScale}",
            "${event.screenMode}",
            "${event.screenReader}",
            "${event.sizeChanged}",
            "${event.rotated}"
          ]
        }
      ]
    }
)apl";

/**
 * This test verifies the onConfigChange data-binding context
 */
TEST_F(BuilderConfigChange, CheckEnvironment)
{
    // Note: explicitly set these properties although most of them are the default values
    metrics.size(100,200).theme("dark").mode(kViewportModeHub);
    config->fontScale(1.0).screenMode(RootConfig::kScreenModeNormal).screenReader(false);

    loadDocument(CHECK_ENVIRONMENT);
    ASSERT_TRUE(component);

    // Rotate the screen
    configChange(ConfigurationChange(200,100));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", 200, 100, "dark", "hub", 1.0, "normal", false, true, true));

    // Resize the screen
    configChange(ConfigurationChange(400,400));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", 400, 400, "dark", "hub", 1.0, "normal", false, true, false));

    // Rotate back. Since we never re-inflated, the sizeChanged and rotated flags should be false now
    configChange(ConfigurationChange(100,200));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", 100, 200, "dark", "hub", 1.0, "normal", false, false, false));

    // Modify other properties
    configChange(ConfigurationChange().theme("purple").screenReader(true));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", 100, 200, "purple", "hub", 1.0, "normal", true, false, false));

    configChange(ConfigurationChange().mode(kViewportModeAuto).fontScale(3.0));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", 100, 200, "purple", "auto", 3.0, "normal", true, false, false));

    configChange(ConfigurationChange().screenMode(RootConfig::kScreenModeHighContrast));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", 100, 200, "purple", "auto", 3.0, "high-contrast", true, false, false));
}


static const char *BASIC_REINFLATE = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "resources": [
        {
          "colors": {
            "BKGND": "blue"
          }
        },
        {
          "when": "${viewport.width < viewport.height}",
          "colors": {
            "BKGND": "red"
          }
        }
      ],
      "mainTemplate": {
        "item": {
          "type": "Frame",
          "backgroundColor": "@BKGND"
        }
      },
      "onConfigChange": { "type": "Reinflate" }
    }
)apl";

/**
 * Rebuild the DOM and verify that resources change appropriately with viewport size
 */
TEST_F(BuilderConfigChange, Basic)
{
    metrics.size(1000,500);
    loadDocument(BASIC_REINFLATE);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), component->getCalculated(kPropertyBackgroundColor)));
    ASSERT_TRUE(IsEqual("initial", evaluate(*context, "${environment.reason}")));

    configChangeReinflate(ConfigurationChange(500, 1000));

    component = std::static_pointer_cast<CoreComponent>(root->topComponent());
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Color(Color::RED), component->getCalculated(kPropertyBackgroundColor)));
    ASSERT_TRUE(IsEqual("reinflation", evaluate(*context, "${environment.reason}")));
}


static const char *ALL_SETTINGS = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "item": {
          "type": "Container",
          "items": {
            "type": "Text",
            "text": "${data}"
          },
          "data": [
            "Width: ${viewport.width}",
            "Height: ${viewport.height}",
            "ViewportMode: ${viewport.mode}",
            "Theme: ${viewport.theme}",
            "FontScale: ${environment.fontScale}",
            "ScreenMode: ${environment.screenMode}",
            "ScreenReader: ${environment.screenReader}"
          ]
        }
      },
      "onConfigChange": { "type": "Reinflate" }
    }
)apl";

/**
 * This test verifies that all ConfigurationChange properties work
 */
TEST_F(BuilderConfigChange, AllSettings) {
    metrics.size(400, 600).theme("light").mode(kViewportModeAuto);
    config->fontScale(2.0).screenMode(RootConfig::kScreenModeNormal).screenReader(true);

    loadDocument(ALL_SETTINGS);
    ASSERT_TRUE(component);

    ASSERT_TRUE(CheckChildStrings({
        "Width: 400",
        "Height: 600",
        "ViewportMode: auto",
        "Theme: light",
        "FontScale: 2",
        "ScreenMode: normal",
        "ScreenReader: true"
    })) << "Starting condition";

    // Verify that changing a single element doesn't reset the others to the defaults
    configChangeReinflate(ConfigurationChange().fontScale(1.5));

    ASSERT_TRUE(CheckChildStrings({
        "Width: 400",
        "Height: 600",
        "ViewportMode: auto",
        "Theme: light",
        "FontScale: 1.5",
        "ScreenMode: normal",
        "ScreenReader: true"
    })) << "One element changed";

    // Change the remaining items and verify that they work correctly
    configChangeReinflate(ConfigurationChange()
                  .size(1000, 1200)
                  .mode(kViewportModeTV)
                  .theme("blue")
                  .screenMode(RootConfig::kScreenModeHighContrast)
                  .screenReader(false));

    ASSERT_TRUE(CheckChildStrings({
        "Width: 1000",
        "Height: 1200",
        "ViewportMode: tv",
        "Theme: blue",
        "FontScale: 1.5",
        "ScreenMode: high-contrast",
        "ScreenReader: false"
    })) << "All elements changed";
}


static const char *REINFLATE_FAIL = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "item": [
          {
            "when": "${viewport.width == 1000}",
            "type": "Text",
            "text": "Club 1000"
          },
          {
            "when": "${viewport.width == 2000}",
            "type": "Frame"
          }
        ]
      },
      "onConfigChange": { "type": "Reinflate" }
    }
)apl";

/**
 * Test reinflation where sometimes we don't get a component
 */
TEST_F(BuilderConfigChange, ReinflateFail)
{
    metrics.size(1000, 500);
    loadDocument(REINFLATE_FAIL);
    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeText, component->getType());

    configChangeReinflate(ConfigurationChange().size(500,500));
    ASSERT_FALSE(component);

    configChangeReinflate(ConfigurationChange().size(2000,2000));
    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeFrame, component->getType());
}


static const char *PAGER = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "item": {
          "type": "Pager",
          "id": "PAGER",
          "items": {
            "type": "Frame"
          },
          "data": "${Array.range(viewport.width < viewport.height ? 3 : 10)}"
        }
      },
      "onConfigChange": { "type": "Reinflate" }
    }
)apl";

/**
 * Rebuild the DOM and verify that all outstanding actions are terminated.
 * The events that generate actions are:  OpenURL, PlayMedia(foreground), FirstLineBounds, ScrollTo,
 * SetPage, Speak, Extension
 */

TEST_F(BuilderConfigChange, StopEventsPopped)
{
    metrics.size(1500, 1000);
    loadDocument(PAGER);
    ASSERT_TRUE(component);
    ASSERT_EQ(10, component->getChildCount());

    // Set the page, and pull the event off of the root stack
    auto actionRef = executeCommand("SetPage", {{"componentId", "PAGER"},
                                                            {"position",    "relative"},
                                                            {"value",       2}}, false);
    ASSERT_EQ(0, component->pagePosition());

    configChangeReinflate(ConfigurationChange(1000, 1500));
    ASSERT_TRUE(component);
    ASSERT_TRUE(actionRef->isTerminated());

    advanceTime(1000);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(3, component->getChildCount());
    ASSERT_EQ(0, component->getCalculated(kPropertyCurrentPage).getInteger());  // Still on page zero
}



static const char *SINGLE_COMPONENT = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "item": {
          "type": "Frame"
        }
      },
      "onConfigChange": { "type": "Reinflate" }
    }
)apl";


/**
 * After a configuration change the old components should go away UNLESS someone is holding
 * onto a reference to them.
 */
TEST_F(BuilderConfigChange, ReleaseOldComponent)
{
    metrics.size(1500,1000);
    loadDocument(SINGLE_COMPONENT);
    ASSERT_TRUE(component);

    auto ptr = std::weak_ptr<CoreComponent>(component);

    configChangeReinflate(ConfigurationChange(1000,1500));
    ASSERT_TRUE(component);

    // Check that our weak pointers have expired
    ASSERT_TRUE(ptr.expired());
}


static const char *COMPONENT_TREE = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "item": {
          "type": "Pager",
          "item": {
            "type": "Text",
            "text": "Item ${index+1}"
          },
          "data": "${Array.range(10)}"
        }
      },
      "onConfigChange": { "type": "Reinflate" }
    }
)apl";


/**
 * Verify that the entire component tree is released after reinflation.
 */
TEST_F(BuilderConfigChange, ReleaseOldComponentTree)
{
    metrics.size(1500,1000);
    loadDocument(COMPONENT_TREE);
    ASSERT_TRUE(component);
    ASSERT_EQ(10, component->getChildCount());

    auto ptr = std::weak_ptr<CoreComponent>(component);
    auto ptr2 = std::weak_ptr<Component>(component->getChildAt(4));

    configChangeReinflate(ConfigurationChange(1000,1500));
    ASSERT_TRUE(component);

    // Check that our weak pointers have expired
    ASSERT_TRUE(ptr2.expired());
    ASSERT_TRUE(ptr.expired());
}


static const char *NO_EVENTS_AFTER_REINFLATE = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "item": {
          "type": "Frame"
        }
      },
      "onConfigChange": [
        {
          "type": "SendEvent",
          "sequencer": "DUMMY",
          "arguments": [
            "prereinflate"
          ]
        },
        {
          "type": "Reinflate"
        },
        {
          "type": "SendEvent",
          "sequencer": "DUMMY",
          "arguments": [
            "postreinflate"
          ]
        }
      ]
    }
)apl";

TEST_F(BuilderConfigChange, NoEventsAfterReinflate)
{
    metrics.size(200,200);
    loadDocument(NO_EVENTS_AFTER_REINFLATE);
    ASSERT_TRUE(component);

    configChange(ConfigurationChange(400,400));
    root->clearPending();

    ASSERT_TRUE(CheckSendEvent(root, "prereinflate"));
    processReinflate();
    ASSERT_FALSE(root->hasEvent());
}


static const char *CONFIG_CHANGE_RUNS_IN_FAST_MODE = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "item": {
          "type": "Frame"
        }
      },
      "onConfigChange": [
        {
          "type": "SendEvent",
          "arguments": [
            "blocked by fast mode"
          ]
        },
        {
          "type": "SendEvent",
          "sequencer": "DUMMY",
          "arguments": [
            "prereinflate"
          ]
        }
      ]
    }
)apl";

TEST_F(BuilderConfigChange, ConfigChangeRunsInFastMode)
{
    metrics.size(200,200);
    loadDocument(CONFIG_CHANGE_RUNS_IN_FAST_MODE);
    ASSERT_TRUE(component);

    configChange(ConfigurationChange(400,400));
    root->clearPending();

    ASSERT_TRUE(CheckSendEvent(root, "prereinflate"));
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());  // There should be a console message warning about SendEvent in fast mode
}


static const char * DEFAULT_RESIZE_BEHAVIOR = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "item": {
          "type": "Frame"
        }
      }
    }
)apl";

TEST_F(BuilderConfigChange, DefaultResizeBehavior)
{
    metrics.size(400,400).dpi(320);
    loadDocument(DEFAULT_RESIZE_BEHAVIOR);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Rect({0,0,200,200}), component->getCalculated(kPropertyBounds)));

    // Change the size.  There is no onConfigChange handler, so the document should resize automatically
    configChange(ConfigurationChange(600,200));
    root->clearPending();
    ASSERT_TRUE(IsEqual(Rect({0,0,300,100}), component->getCalculated(kPropertyBounds).getRect()));
    ASSERT_TRUE(CheckDirty(component, kPropertyBounds, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, component));
}


static const char *ON_CONFIG_CHANGE_NO_RELAYOUT = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "item": {
          "type": "Frame"
        }
      },
      "onConfigChange": {
        "type": "SendEvent",
        "sequencer": "FOO",
        "arguments": [
          "normal"
        ]
      }
    }
)apl";

// This test case includes an onConfigChange command which does not call Relayout
TEST_F(BuilderConfigChange, OnConfigChangeNoRelayout)
{
    metrics.size(200,200);
    loadDocument(ON_CONFIG_CHANGE_NO_RELAYOUT);
    ASSERT_TRUE(component);
    ASSERT_EQ(Rect({0,0,200,200}), component->getCalculated(kPropertyBounds).getRect());

    // Change the size.
    configChange(ConfigurationChange(300,100));
    root->clearPending();
    ASSERT_EQ(Rect({0,0,300,100}), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_TRUE(CheckDirty(component, kPropertyBounds, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, component));
    ASSERT_TRUE(CheckSendEvent(root, "normal"));  // The normal event has fired
}


static const char *ON_CONFIG_CHANGE_BASIC_RELAYOUT = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "item": {
          "type": "Frame"
        }
      },
      "onConfigChange": {
        "type": "Reinflate"
      }
    }
)apl";

/**
 * Verify that the Reinflate action reference is terminated after RootContext::reinflate()
 */
TEST_F(BuilderConfigChange, ReinflateActionRefIsTerminated)
{
    metrics.size(200,200);
    loadDocument(ON_CONFIG_CHANGE_BASIC_RELAYOUT);
    ASSERT_TRUE(component);
    ASSERT_EQ(Rect({0,0,200,200}), component->getCalculated(kPropertyBounds).getRect());

    // Change the size.
    configChange(ConfigurationChange(300,100));
    root->clearPending();
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeReinflate, event.getType());
    ASSERT_TRUE(event.getActionRef().isPending());  // There is a pending action reference

    // No reinflation has occurred yet - we haven't resolved the action reference
    ASSERT_EQ(Rect({0,0,200,200}), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_TRUE(CheckDirty(component));
    ASSERT_TRUE(CheckDirty(root));

    // Call Reinflate - this should kill the action ref
    root->reinflate();
    context = root->contextPtr();
    ASSERT_TRUE(context);
    component = std::dynamic_pointer_cast<CoreComponent>(root->topComponent());
    ASSERT_EQ(Rect({0,0,300,100}), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_TRUE(CheckDirty(component));
    ASSERT_TRUE(CheckDirty(root));
    ASSERT_TRUE(event.getActionRef().isTerminated());
}

static const char *RESIZE_QUEUE = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "item": {
          "type": "Frame"
        }
      },
      "onConfigChange": {
        "type": "Reinflate"
      }
    }
)apl";

/**
 * Queue up a bunch of resize events behind a reinflate; then resolve the reinflate to
 * let the resizes take place just once.
 */
TEST_F(BuilderConfigChange, ResizeQueue)
{
    metrics.size(200,200);
    loadDocument(RESIZE_QUEUE);
    ASSERT_TRUE(component);
    ASSERT_EQ(Rect({0,0,200,200}), component->getCalculated(kPropertyBounds).getRect());

    // Change the size.
    configChange(ConfigurationChange(300,100));
    root->clearPending();
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeReinflate, event.getType());
    ASSERT_TRUE(event.getActionRef().isPending());  // There is a pending action reference

    // No reinflation has occurred yet - we haven't resolved the first action reference
    ASSERT_EQ(Rect({0,0,200,200}), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_TRUE(CheckDirty(component));
    ASSERT_TRUE(CheckDirty(root));

    // Change the size again.  The first Reinflate action ref should be terminated; the size stays the same
    configChange(ConfigurationChange(400,500));
    root->clearPending();
    ASSERT_TRUE(root->hasEvent());
    auto event2 = root->popEvent();
    ASSERT_EQ(kEventTypeReinflate, event2.getType());
    ASSERT_TRUE(event.getActionRef().isTerminated());  // There is a pending action reference
    ASSERT_TRUE(event2.getActionRef().isPending());  // There is a pending action reference

    event.getActionRef().resolve();  // Try to resolve the terminated action ref
    root->clearPending();

    // No reinflation has occurred yet - we haven't resolved the new "live" action reference
    ASSERT_EQ(Rect({0,0,200,200}), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_TRUE(CheckDirty(component));
    ASSERT_TRUE(CheckDirty(root));

    // Resolve the second action ref - this will unblock the resize.
    event2.getActionRef().resolve();
    root->clearPending();
    ASSERT_EQ(Rect({0,0,400,500}), component->getCalculated(kPropertyBounds).getRect());
    ASSERT_TRUE(CheckDirty(component, kPropertyBounds, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, component));
    ASSERT_TRUE(event.getActionRef().isTerminated());
    ASSERT_TRUE(event2.getActionRef().isResolved());
}

static const char *HANDLE_TICK_REINFLATE = R"({
  "type": "APL",
  "version": "1.6",
  "theme": "dark",
  "settings": {
    "supportsResizing": true
  },
  "onConfigChange": [
    {
      "type": "Reinflate"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Container",
      "direction": "row",
      "bind": [
        {
          "name": "Transparency",
          "value": 0.5,
          "type": "number"
        }
      ],
      "handleTick": {
        "minimumDelay": 1000,
        "commands": {
            "type": "SetValue",
            "property": "Transparency",
            "value": "${Transparency < 1 ? 1 : 0.5}"
        }
      },
      "items": [
        {
          "type": "Text",
          "id": "textField",
          "opacity": "${Transparency}",
          "height": 50,
          "width": 200,
          "text": "Party time!"
        }
      ]
    }
  }
})";

/**
 * Verify that the Reinflate clears out any scheduled ticks handlers.
 */
TEST_F(BuilderConfigChange, ReinflateWithHandleTick)
{
    metrics.size(200,200);
    loadDocument(HANDLE_TICK_REINFLATE);
    ASSERT_TRUE(component);
    ASSERT_EQ(Rect({0,0,200,200}), component->getCalculated(kPropertyBounds).getRect());

    auto text = component->findComponentById("textField");
    ASSERT_TRUE(text);
    ASSERT_EQ(kComponentTypeText, text->getType());
    ASSERT_EQ(0.5, text->getCalculated(kPropertyOpacity).getDouble());
    advanceTime(1100);
    ASSERT_EQ(1.0, text->getCalculated(kPropertyOpacity).getDouble());

    // Change the size.
    configChangeReinflate(ConfigurationChange(300,300));
    ASSERT_TRUE(component);
    ASSERT_EQ(Rect({0,0,300,300}), component->getCalculated(kPropertyBounds).getRect());

    text = component->findComponentById("textField");
    ASSERT_TRUE(text);
    ASSERT_EQ(kComponentTypeText, text->getType());
    ASSERT_EQ(0.5, text->getCalculated(kPropertyOpacity).getDouble());
    advanceTime(1100);
    ASSERT_EQ(1.0, text->getCalculated(kPropertyOpacity).getDouble());
}