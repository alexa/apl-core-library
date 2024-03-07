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
            "${event.minWidth}",
            "${event.maxWidth}",
            "${event.minHeight}",
            "${event.maxHeight}",
            "${event.theme}",
            "${event.viewportMode}",
            "${event.disallowVideo}",
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
    config->set(RootProperty::kDisallowVideo, false)
        .set(RootProperty::kFontScale, 1.0)
        .set(RootProperty::kScreenMode, RootConfig::kScreenModeNormal)
        .set(RootProperty::kScreenReader, false);

    loadDocument(CHECK_ENVIRONMENT);
    ASSERT_TRUE(component);

    // Empty change
    configChange(ConfigurationChange());
    ASSERT_FALSE(root->hasEvent());

    // Just theme, to existing one
    configChange(ConfigurationChange().theme("dark"));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", 100, 100, 100, 100, 100, 100, "dark", "hub", false, 1.0, "normal", false, false, false));

    // Rotate the screen
    configChange(ConfigurationChange(200,100));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", 200, 100, 200, 200, 100, 100, "dark", "hub", false, 1.0, "normal", false, true, true));

    // Resize the screen
    configChange(ConfigurationChange(400,400));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", 400, 400, 400, 400, 400, 400, "dark", "hub", false, 1.0, "normal", false, true, false));

    // Rotate back. Since we never re-inflated, the sizeChanged and rotated flags should be false now
    configChange(ConfigurationChange(100,200));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", 100, 200, 100, 100, 200, 200, "dark", "hub", false, 1.0, "normal", false, false, false));

    // Modify other properties
    configChange(ConfigurationChange().theme("purple").screenReader(true));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", 100, 200, 100, 100, 200, 200, "purple", "hub", false, 1.0, "normal", true, false, false));

    configChange(ConfigurationChange().mode(kViewportModeAuto).fontScale(3.0));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", 100, 200, 100, 100, 200, 200, "purple", "auto", false, 3.0, "normal", true, false, false));

    configChange(ConfigurationChange().screenMode(RootConfig::kScreenModeHighContrast));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", 100, 200, 100, 100, 200, 200, "purple", "auto", false, 3.0, "high-contrast", true, false, false));

    configChange(ConfigurationChange().disallowVideo(true));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", 100, 200, 100, 100, 200, 200, "purple", "auto", true, 3.0, "high-contrast", true, false, false));

    configChange(ConfigurationChange().mode("tv"));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", 100, 200, 100, 100, 200, 200, "purple", "tv", true, 3.0, "high-contrast", true, false, false));

    configChange(ConfigurationChange().screenMode("normal"));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", 100, 200, 100, 100, 200, 200, "purple", "tv", true, 3.0, "normal", true, false, false));

    // Resize to a variable size
    configChange(ConfigurationChange().sizeRange(100, 50, 150, 300, 250, 350));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", 100, 300, 50, 150, 250, 350, "purple", "tv", true, 3.0, "normal", true, true, false));
}

TEST_F(BuilderConfigChange, NoopConfigurationChangeDoesNotCreateEvent)
{
    loadDocument(CHECK_ENVIRONMENT);
    ASSERT_TRUE(component);

    configChange(ConfigurationChange());
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(BuilderConfigChange, InvalidConfigurationChangeEmitsLog)
{
    loadDocument(CHECK_ENVIRONMENT);
    ASSERT_TRUE(component);

    // Clear logs
    LogMessage();
    configChange(ConfigurationChange().mode("foo"));
    // Assert log is emitted
    ASSERT_TRUE(LogMessage());
    ASSERT_FALSE(root->hasEvent());

    // Clear logs
    LogMessage();
    configChange(ConfigurationChange().screenMode(""));
    // Assert log is emitted
    ASSERT_TRUE(LogMessage());
    ASSERT_FALSE(root->hasEvent());
}

static const char *CHECK_CUSTOM_ENVIRONMENT = R"apl(
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
            "${event.reason ?? environment.reason}",
            "${event.sizeChanged}",
            "${event.theme}",
            "${event.environment.vehicleState}",
            "${event.environment.navigationSupported}",
            "${event.environment.notDeclared ?? environment.notDeclared}"
          ]
        }
      ]
    }
)apl";

TEST_F(BuilderConfigChange, CheckCustomEnvironmentProperties) {
    metrics.size(100,200).theme("dark").mode(kViewportModeHub);
    config->setEnvironmentValue("vehicleState", "parked")
        .setEnvironmentValue("navigationSupported", false);

    loadDocument(CHECK_CUSTOM_ENVIRONMENT);
    ASSERT_TRUE(component);

    // Rotate the screen
    configChange(ConfigurationChange(200, 100));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", "initial", true, "dark", "parked", false, Object::NULL_OBJECT()));

    configChange(ConfigurationChange().environmentValue("vehicleState", "driving"));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", "initial", true, "dark", "driving", false, Object::NULL_OBJECT()));

    configChange(ConfigurationChange().environmentValue("navigationSupported", true));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", "initial", true, "dark", "driving", true, Object::NULL_OBJECT()));

    configChange(ConfigurationChange().environmentValue("vehicleState", "reversing").environmentValue("navigationSupported", false));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", "initial", true, "dark", "reversing", false, Object::NULL_OBJECT()));

    configChange(ConfigurationChange().environmentValue("reason", "should_be_ignored"));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", "initial", true, "dark", "reversing", false, Object::NULL_OBJECT()));

    configChange(ConfigurationChange().environmentValue("theme", "should_be_ignored"));
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", "initial", true, "dark", "reversing", false, Object::NULL_OBJECT()));

    configChange(ConfigurationChange().environmentValue("sizeChanged", false)); // should be ignored
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", "initial", true, "dark", "reversing", false, Object::NULL_OBJECT()));

    // Check that an attempt to define a new property via ConfigurationChange is not allowed
    configChange(ConfigurationChange().environmentValue("notDeclared", 42)); // should be ignored
    ASSERT_TRUE(CheckSendEvent(root, "Document", "ConfigChange", "initial", true, "dark", "reversing", false, Object::NULL_OBJECT()));
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

    component = CoreComponent::cast(root->topComponent());
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Color(Color::RED), component->getCalculated(kPropertyBackgroundColor)));
    ASSERT_TRUE(IsEqual("reinflation", evaluate(*context, "${environment.reason}")));
}

static const char *BASIC_REINFLATE_WITH_DROP = R"apl(
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
      "onConfigChange": { "type": "Reinflate" },
      "onMount": {
        "type": "SendEvent",
        "delay": 200,
        "sequencer": "MOUNT_SEQUENCER"
      }
    }
)apl";

/**
 * Rebuild the DOM and verify that resources change appropriately with viewport size + pending
 * command drops.
 */
TEST_F(BuilderConfigChange, BasicWithDrop)
{
    metrics.size(1000,500);
    loadDocument(BASIC_REINFLATE_WITH_DROP);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), component->getCalculated(kPropertyBackgroundColor)));
    ASSERT_TRUE(IsEqual("initial", evaluate(*context, "${environment.reason}")));

    advanceTime(100);

    configChangeReinflate(ConfigurationChange(500, 1000));

    advanceTime(100);

    component = CoreComponent::cast(root->topComponent());
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
            "DisallowVideo: ${environment.disallowVideo}",
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
    config->set(RootProperty::kDisallowVideo, false)
        .set(RootProperty::kFontScale, 2.0)
        .set(RootProperty::kScreenMode, RootConfig::kScreenModeNormal)
        .set(RootProperty::kScreenReader, true);

    loadDocument(ALL_SETTINGS);
    ASSERT_TRUE(component);

    ASSERT_TRUE(CheckChildStrings({
        "Width: 400",
        "Height: 600",
        "ViewportMode: auto",
        "Theme: light",
        "DisallowVideo: false",
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
        "DisallowVideo: false",
        "FontScale: 1.5",
        "ScreenMode: normal",
        "ScreenReader: true"
    })) << "One element changed";

    // Change the remaining items and verify that they work correctly
    configChangeReinflate(ConfigurationChange()
                  .size(1000, 1200)
                  .mode(kViewportModeTV)
                  .theme("blue")
                  .disallowVideo(true)
                  .screenMode(RootConfig::kScreenModeHighContrast)
                  .screenReader(false));

    ASSERT_TRUE(CheckChildStrings({
        "Width: 1000",
        "Height: 1200",
        "ViewportMode: tv",
        "Theme: blue",
        "DisallowVideo: true",
        "FontScale: 1.5",
        "ScreenMode: high-contrast",
        "ScreenReader: false"
    })) << "All elements changed";
}

static const char *REINFLATE_CUSTOM_ENV_PROPERTIES = R"apl(
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
            "VehicleState: ${environment.vehicleState}",
            "NavSupported: ${environment.navigationSupported}"
          ]
        }
      },
      "onConfigChange": { "type": "Reinflate" }
    }
)apl";


TEST_F(BuilderConfigChange, ReinflateCustomEnvProperties) {
    config->setEnvironmentValue("vehicleState", "parked")
        .setEnvironmentValue("navigationSupported", false);

    loadDocument(REINFLATE_CUSTOM_ENV_PROPERTIES);
    ASSERT_TRUE(component);

    ASSERT_TRUE(CheckChildStrings({
                                      "VehicleState: parked",
                                      "NavSupported: false"
                                  })) << "Starting condition";

    // Verify that changing a single property doesn't reset the other
    configChangeReinflate(ConfigurationChange().environmentValue("vehicleState", "driving"));

    ASSERT_TRUE(CheckChildStrings({
                                      "VehicleState: driving",
                                      "NavSupported: false"
                                  })) << "One element changed";

    // Change another property and verify that the first one is unaffected
    configChangeReinflate(ConfigurationChange().environmentValue("navigationSupported", true));

    ASSERT_TRUE(CheckChildStrings({
                                      "VehicleState: driving",
                                      "NavSupported: true"
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
    ASSERT_TRUE(IsEqual(Rect({0,0,300,100}), component->getCalculated(kPropertyBounds).get<Rect>()));
    ASSERT_TRUE(CheckDirty(component, kPropertyBounds, kPropertyInnerBounds, kPropertyNotifyChildrenChanged,
                           kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component));
}

static const char *SINGLE_RELATIVE_COMPONENT = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "item": {
          "type": "Frame",
          "height": "100%",
          "width": "100%"
        }
      }
    }
)apl";

TEST_F(BuilderConfigChange, OtherDpi) {
    metrics.size(400,400).theme("light").dpi(320);;

    loadDocument(SINGLE_RELATIVE_COMPONENT);
    ASSERT_TRUE(component);

    ASSERT_EQ(Rect(0,0,200,200), component->getCalculated(apl::kPropertyBounds).get<Rect>());

    // Verify that changing theme changes only theme
    configChange(ConfigurationChange().theme("dark"));
    root->clearPending();

    ASSERT_EQ(Rect(0,0,200,200), component->getCalculated(apl::kPropertyBounds).get<Rect>());
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
    ASSERT_EQ(Rect({0,0,200,200}), component->getCalculated(kPropertyBounds).get<Rect>());

    // Change the size.
    configChange(ConfigurationChange(300,100));
    root->clearPending();
    ASSERT_EQ(Rect({0,0,300,100}), component->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_TRUE(CheckDirty(component, kPropertyBounds, kPropertyInnerBounds, kPropertyNotifyChildrenChanged,
                           kPropertyVisualHash));
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
    ASSERT_EQ(Rect({0,0,200,200}), component->getCalculated(kPropertyBounds).get<Rect>());

    // Change the size.
    configChange(ConfigurationChange(300,100));
    root->clearPending();
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeReinflate, event.getType());
    ASSERT_TRUE(event.getActionRef().isPending());  // There is a pending action reference

    // No reinflation has occurred yet - we haven't resolved the action reference
    ASSERT_EQ(Rect({0,0,200,200}), component->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_TRUE(CheckDirty(component));
    ASSERT_TRUE(CheckDirty(root));

    // Call Reinflate - this should kill the action ref
    root->reinflate();
    context = root->contextPtr();
    ASSERT_TRUE(context);
    component = CoreComponent::cast(root->topComponent());
    ASSERT_EQ(Rect({0,0,300,100}), component->getCalculated(kPropertyBounds).get<Rect>());
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
    ASSERT_EQ(Rect({0,0,200,200}), component->getCalculated(kPropertyBounds).get<Rect>());

    // Change the size.
    configChange(ConfigurationChange(300,100));
    root->clearPending();
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeReinflate, event.getType());
    ASSERT_TRUE(event.getActionRef().isPending());  // There is a pending action reference

    // No reinflation has occurred yet - we haven't resolved the first action reference
    ASSERT_EQ(Rect({0,0,200,200}), component->getCalculated(kPropertyBounds).get<Rect>());
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
    ASSERT_EQ(Rect({0,0,200,200}), component->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_TRUE(CheckDirty(component));
    ASSERT_TRUE(CheckDirty(root));

    // Resolve the second action ref - this will unblock the resize.
    event2.getActionRef().resolve();
    root->clearPending();
    ASSERT_EQ(Rect({0,0,400,500}), component->getCalculated(kPropertyBounds).get<Rect>());
    ASSERT_TRUE(CheckDirty(component, kPropertyBounds, kPropertyInnerBounds, kPropertyNotifyChildrenChanged,
                           kPropertyVisualHash));
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
    ASSERT_EQ(Rect({0,0,200,200}), component->getCalculated(kPropertyBounds).get<Rect>());

    auto text = root->findComponentById("textField");
    ASSERT_TRUE(text);
    ASSERT_EQ(kComponentTypeText, text->getType());
    ASSERT_EQ(0.5, text->getCalculated(kPropertyOpacity).getDouble());
    advanceTime(1100);
    ASSERT_EQ(1.0, text->getCalculated(kPropertyOpacity).getDouble());

    // Change the size.
    configChangeReinflate(ConfigurationChange(300,300));
    ASSERT_TRUE(component);
    ASSERT_EQ(Rect({0,0,300,300}), component->getCalculated(kPropertyBounds).get<Rect>());

    text = root->findComponentById("textField");
    ASSERT_TRUE(text);
    ASSERT_EQ(kComponentTypeText, text->getType());
    ASSERT_EQ(0.5, text->getCalculated(kPropertyOpacity).getDouble());
    advanceTime(1100);
    ASSERT_EQ(1.0, text->getCalculated(kPropertyOpacity).getDouble());
}


static const char *CHECK_SCALED_WIDTH_HEIGHT = R"apl(
    {
      "type": "APL",
      "version": "1.9",
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
            "${event.width}",
            "${event.height}"
          ]
        }
      ]
    }
)apl";

/**
 * Verify that the "event.width" and "event.height" values reported are in DP, not Pixels.
 * This configuration change only causes a layout pass which resizes the component to exactly
 * fill the view host.
 */
TEST_F(BuilderConfigChange, ScaledWidthHeight)
{
    metrics.size(1000,600).dpi(320);
    loadDocument(CHECK_SCALED_WIDTH_HEIGHT);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Rect({0,0,500,300}), component->getCalculated(kPropertyBounds).get<Rect>()));

    // Change the size of the view host
    configChange(ConfigurationChange(600,1000));
    ASSERT_TRUE(CheckSendEvent(root, 300, 500));
    ASSERT_TRUE(IsEqual(Rect({0,0,300,500}), component->getCalculated(kPropertyBounds).get<Rect>()));
}