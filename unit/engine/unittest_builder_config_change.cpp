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
      }
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

    reinflate(ConfigurationChange(500, 1000));
    component = std::static_pointer_cast<CoreComponent>(root->topComponent());
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Color(Color::RED), component->getCalculated(kPropertyBackgroundColor)));
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
      }
    }
)apl";

/**
 * This test verifies that all ConfigurationChange properties work
 */
TEST_F(BuilderConfigChange, AllSettings) {
    metrics.size(400, 600).theme("light").mode(kViewportModeAuto);
    config.fontScale(2.0).screenMode(RootConfig::kScreenModeNormal).screenReader(true);

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
    reinflate(ConfigurationChange().fontScale(1.5));

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
    reinflate(ConfigurationChange()
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
      }
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

    reinflate(ConfigurationChange().size(500,500));
    ASSERT_FALSE(component);

    reinflate(ConfigurationChange().size(2000,2000));
    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeFrame, component->getType());
}


static const char *PAGER = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "item": {
          "type": "Pager",
          "id": "PAGER",
          "items": {
            "type": "Frame"
          },
          "data": "${Array.range(viewport.width < viewport.height ? 3 : 10)}"
        }
      }
    }
)apl";

/**
 * Rebuild the DOM and verify that all outstanding actions are terminated.
 * The events that generate actions are:  OpenURL, PlayMedia(foreground), FirstLineBounds, ScrollTo,
 * SetPage, Speak, Extension
 *
 * Note that any events setting on the outgoing queue must also be terminated.
 */
TEST_F(BuilderConfigChange, StopEvents)
{
    metrics.size(1500, 1000);
    loadDocument(PAGER);
    ASSERT_TRUE(component);
    ASSERT_EQ(10, component->getChildCount());

    // Set the page, but leave the event on the root stack
    executeCommand("SetPage", {{"componentId", "PAGER"},
                               {"position",    "absolute"},
                               {"value",       2}}, false);
    ASSERT_TRUE(root->hasEvent());

    reinflate(ConfigurationChange(1000, 1500));
    ASSERT_TRUE(component);

    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(3, component->getChildCount());
    ASSERT_EQ(0, component->getCalculated(kPropertyCurrentPage).getInteger());  // Still on page zero
}

TEST_F(BuilderConfigChange, StopEventsPopped)
{
    metrics.size(1500, 1000);
    loadDocument(PAGER);
    ASSERT_TRUE(component);
    ASSERT_EQ(10, component->getChildCount());

    // Set the page, and pull the event off of the root stack
    executeCommand("SetPage", {{"componentId", "PAGER"},
                               {"position",    "absolute"},
                               {"value",       2}}, false);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSetPage, event.getType());

    reinflate(ConfigurationChange(1000, 1500));
    ASSERT_TRUE(component);

    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(3, component->getChildCount());
    ASSERT_EQ(0, component->getCalculated(kPropertyCurrentPage).getInteger());  // Still on page zero

    // The event should have been terminated
    ASSERT_TRUE(event.getActionRef().isTerminated());
}



static const char *SINGLE_COMPONENT = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "item": {
          "type": "Frame"
        }
      }
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

    reinflate(ConfigurationChange(1000,1500));
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
      }
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

    reinflate(ConfigurationChange(1000,1500));
    ASSERT_TRUE(component);

    // Check that our weak pointers have expired
    ASSERT_TRUE(ptr2.expired());
    ASSERT_TRUE(ptr.expired());
}