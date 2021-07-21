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

class BuilderPreserveScrollTest : public DocumentWrapper {
public:
    // Return a calculated value from a named component
    Object getCalc(const std::string& name, PropertyKey key) {
        auto component = root->findComponentById(name);
        return component->getCalculated(key);
    }

    // Return a named PROPERTY from a component - this access the internal system
    Object getProp(const std::string& name, PropertyKey key) {
        auto component = root->findComponentById(name);
        auto keyName = sComponentPropertyBimap.at(key);
        return std::dynamic_pointer_cast<CoreComponent>(component)->getProperty(keyName);
    }

    // Set the scroll position of a named component
    void setScroll(const std::string& name, float value) {
        auto component = root->findComponentById(name);
        component->update(kUpdateScrollPosition, value);
    }
};

static const char *SCROLL_VIEW_OFFSET = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "onConfigChange": {
        "type": "Reinflate"
      },
      "mainTemplate": {
        "items": {
          "type": "ScrollView",
          "id": "MyScrollView",
          "preserve": [
            "scrollOffset"
          ],
          "width": 100,
          "height": 100,
          "items": {
            "type": "Frame",
            "width": 100,
            "height": 500
          }
        }
      }
    }
)apl";

TEST_F(BuilderPreserveScrollTest, ScrollViewOffset)
{
    metrics.size(200,200);
    loadDocument(SCROLL_VIEW_OFFSET);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual(Dimension(0), component->getCalculated(kPropertyScrollPosition)));

    // Scroll down
    component->update(kUpdateScrollPosition, 321);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Dimension(321), component->getCalculated(kPropertyScrollPosition)));

    // Trigger reinflate
    auto old = component;
    configChangeReinflate(ConfigurationChange(100,100));
    ASSERT_TRUE(component);
    ASSERT_EQ(component->getId(), old->getId());
    ASSERT_TRUE(IsEqual(Dimension(321), component->getCalculated(kPropertyScrollPosition)));
}


static const char *SCROLL_VIEW_PERCENT = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "onConfigChange": {
        "type": "Reinflate"
      },
      "mainTemplate": {
        "items": {
          "type": "ScrollView",
          "id": "MyScrollView",
          "preserve": [
            "scrollPercent"
          ],
          "width": "100%",
          "height": "100%",
          "items": {
            "type": "Frame",
            "width": 100,
            "height": "500vh"
          }
        }
      }
    }
)apl";

TEST_F(BuilderPreserveScrollTest, ScrollViewPercent)
{
    metrics.size(200,200);
    loadDocument(SCROLL_VIEW_PERCENT);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual(Dimension(0), component->getCalculated(kPropertyScrollPosition)));

    // Scroll down
    component->update(kUpdateScrollPosition, 100);  // This is 50% of the height of the scroll view (200 dp)
    root->clearPending();
    ASSERT_TRUE(IsEqual(Dimension(100), component->getCalculated(kPropertyScrollPosition)));

    // Trigger reinflate
    auto old = component;
    configChangeReinflate(ConfigurationChange(100,100));
    ASSERT_TRUE(component);
    ASSERT_EQ(component->getId(), old->getId());
    ASSERT_TRUE(IsEqual(Dimension(50), component->getCalculated(kPropertyScrollPosition)));  // This is 50% of the NEW height
}


static const char *SCROLL_VIEW_CANCEL_SCROLL_COMMAND = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "onConfigChange": {
        "type": "Reinflate"
      },
      "mainTemplate": {
        "items": {
          "type": "ScrollView",
          "id": "MyScrollView",
          "width": "100%",
          "height": "100%",
          "items": {
            "type": "Frame",
            "width": 100,
            "height": "500vh"
          },
          "onScroll": {
            "type": "SetValue",
            "property": "scrollOffset",
            "value": 10,
            "when": "${event.source.position > 1}"
          }
        }
      }
    }
)apl";

/**
 * Explicitly using SetValue with "scrollOffset" or "scrollPercent" should cancel any
 * long-running command that is updating the scroll position.
 */
TEST_F(BuilderPreserveScrollTest, ScrollViewCancelScrollCommand)
{
    metrics.size(200,200);
    loadDocument(SCROLL_VIEW_CANCEL_SCROLL_COMMAND);

    ASSERT_TRUE(IsEqual(Dimension(0), component->getCalculated(kPropertyScrollPosition)));

    // Scroll down
    component->update(kUpdateScrollPosition, 100);  // This is 50% of the height of the scroll view (200 dp)
    root->clearPending();
    ASSERT_TRUE(IsEqual(Dimension(100), component->getCalculated(kPropertyScrollPosition)));

    // Start a "Scroll" command running
    executeCommand("Scroll", {{"componentId", "MyScrollView"}, {"distance", 1}}, false);

    // As we scroll down eventually the "onScroll" handler will trigger (at 200dp).  When that happens,
    //  (1) we'll jump back to a scroll offset of 10 and
    //  (2) the scroll event will be terminated
    advanceTime(500);
    ASSERT_TRUE(component->getCalculated(kPropertyScrollPosition).asNumber() > 0);

    // Now we cross the threshold
    advanceTime(500);
    ASSERT_TRUE(IsEqual(Dimension(10), component->getCalculated(kPropertyScrollPosition)));
}


static const char *SCROLL_VIEW_PRESERVE_WITH_EVENT_HANDLER = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "onConfigChange": {
        "type": "Reinflate"
      },
      "mainTemplate": {
        "items": {
          "type": "ScrollView",
          "id": "ID",
          "width": "100%",
          "height": "100%",
          "preserve": [
            "scrollOffset"
          ],
          "items": {
            "type": "Text",
            "id": "MyText",
            "width": 100,
            "height": 500
          },
          "onScroll": {
            "type": "SetValue",
            "componentId": "MyText",
            "property": "text",
            "value": "Position: ${event.source.position}"
          }
        }
      }
    }
)apl";

TEST_F(BuilderPreserveScrollTest, ScrollViewEventHandler)
{
    metrics.size(200,200);
    loadDocument(SCROLL_VIEW_PRESERVE_WITH_EVENT_HANDLER);
    ASSERT_TRUE(component);
    auto text = component->getChildAt(0);

    // Scroll down, triggering the onScroll handler
    component->update(kUpdateScrollPosition, 100);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Dimension(100), component->getCalculated(kPropertyScrollPosition)));
    ASSERT_TRUE(IsEqual("Position: 0.5", text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(text, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, component, text));

    // Trigger reinflate
    auto oldComponent = component;
    auto oldText = text;
    configChangeReinflate(ConfigurationChange(100,100));
    ASSERT_TRUE(component);
    text = component->getChildAt(0);
    ASSERT_EQ(component->getId(), oldComponent->getId());
    ASSERT_TRUE(IsEqual(Dimension(100), component->getCalculated(kPropertyScrollPosition)));  // This is 50% of the NEW height
    ASSERT_TRUE(IsEqual("", text->getCalculated(kPropertyText).asString()));   // The onScroll was not triggered in re-inflate

    // Scroll down to verify that onScroll works
    component->update(kUpdateScrollPosition, 200);
    root->clearPending();
    ASSERT_TRUE(IsEqual(Dimension(200), component->getCalculated(kPropertyScrollPosition)));
    ASSERT_TRUE(IsEqual("Position: 2", text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(text, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, component, text));
}


static const char *SCROLL_VIEW_CANCEL_NATIVE_SCROLLING = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "items": {
          "type": "ScrollView",
          "id": "ID",
          "width": "100%",
          "height": "100%",
          "items": {
            "type": "Frame",
            "width": 100,
            "height": 500
          },
          "onScroll": {
            "when": "${event.source.position > 0.5}",
            "type": "SetValue",
            "componentId": "ID",
            "property": "scrollOffset",
            "value": 20
          }
        }
      }
    }
)apl";

// When native scrolling, if we set the scroll position we need to cancel any existing scrolling action or fling.
TEST_F(BuilderPreserveScrollTest, ScrollViewCancelNativeScrolling)
{
    metrics.size(200,200);
    loadDocument(SCROLL_VIEW_CANCEL_NATIVE_SCROLLING);

    ASSERT_FALSE(root->handlePointerEvent({PointerEventType::kPointerDown, Point(10,190)}));

    // Scroll up 90 units
    advanceTime(100);
    ASSERT_TRUE(root->handlePointerEvent({PointerEventType::kPointerMove, Point(10,100)}));  // The scroll gesture should take control
    ASSERT_TRUE(IsEqual(Dimension(90), component->getCalculated(kPropertyScrollPosition)));

    // Scroll up another 50 units.  The SetValue method should execute and cancel the scrolling
    advanceTime(50);
    ASSERT_TRUE(root->handlePointerEvent({PointerEventType::kPointerMove, Point(10,50)}));
    ASSERT_TRUE(IsEqual(Dimension(20), component->getCalculated(kPropertyScrollPosition)));

    // Keep scrolling - but the gesture should be cancelled now, so nothing happens
    advanceTime(50);
    ASSERT_TRUE(root->handlePointerEvent({PointerEventType::kPointerMove, Point(10,10)}));
    ASSERT_TRUE(IsEqual(Dimension(20), component->getCalculated(kPropertyScrollPosition)));
}


const static char *SEQUENCE_PRESERVE_FIRST_INDEX = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "onConfigChange": {
        "type": "Reinflate"
      },
      "mainTemplate": {
        "items": {
          "type": "Sequence",
          "id": "SEQUENCE",
          "preserve": [
            "firstIndex"
          ],
          "width": "100%",
          "height": "100%",
          "items": {
            "type": "Text",
            "id": "TEXT_${index}",
            "text": "text-${index}",
            "width": "100%",
            "height": "50vh"
          },
          "data": "${Array.range(10)}"
        }
      }
    }
)apl";

TEST_F(BuilderPreserveScrollTest, SequencePreserveFirstIndex)
{
    // Note that the child height is always 50% of the screen height
    metrics.size(200,200);
    loadDocument(SEQUENCE_PRESERVE_FIRST_INDEX);
    ASSERT_TRUE(component);
    ASSERT_EQ(0, component->getCalculated(kPropertyScrollPosition).asNumber());
    ASSERT_TRUE(IsEqual(Rect(0,0,200,100), component->getChildAt(0)->getCalculated(kPropertyBounds)));

    // Reinflate
    configChangeReinflate(ConfigurationChange(300,100));   // Child height will be 50
    ASSERT_TRUE(component);
    ASSERT_EQ(0, component->getCalculated(kPropertyScrollPosition).asNumber());

    // Scroll forwards so that the first text box is half exposed
    component->update(kUpdateScrollPosition, 25);
    root->clearPending();
    ASSERT_EQ(25, component->getCalculated(kPropertyScrollPosition).asNumber());

    // Reinflate
    configChangeReinflate(ConfigurationChange(200,100));    // Child height didn't change; scroll position is fixed
    ASSERT_EQ(25, component->getCalculated(kPropertyScrollPosition).asNumber());

    // Scroll forwards so that the third text box is half exposed
    component->update(kUpdateScrollPosition, 125);
    root->clearPending();
    configChangeReinflate(ConfigurationChange(200,200));  // Increase the child size by x2
    ASSERT_EQ(250, component->getCalculated(kPropertyScrollPosition).asNumber());

    // Go all the way to the end of the list.  This forces layout of all the child components
    component->update(kUpdateScrollPosition, 100000);
    root->clearPending();
    ASSERT_EQ(800, component->getCalculated(kPropertyScrollPosition).asNumber());
    configChangeReinflate(ConfigurationChange(200,500));
    ASSERT_EQ(2000, component->getCalculated(kPropertyScrollPosition).asNumber());
}


const static char *SEQUENCES_PRESERVE_FIRST = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "onConfigChange": {
        "type": "Reinflate"
      },
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": "100%",
          "height": "100%",
          "direction": "row",
          "items": {
            "type": "Sequence",
            "bind": {
              "name": "NAME",
              "value": "${data.name}"
            },
            "id": "SEQUENCE-${NAME}",
            "preserve": [
              "${data.value}"
            ],
            "width": "50%",
            "height": "100%",
            "items": {
              "type": "Text",
              "when": "${viewport.theme == 'light' || data % 2 == 0}",
              "id": "TEXT-${NAME}-${data}",
              "text": "text-${NAME}-${data}",
              "width": "100%",
              "height": "50vh"
            },
            "data": "${Array.range(12)}"
          },
          "data": [
            {
              "name": "INDEX",
              "value": "firstIndex"
            },
            {
              "name": "ID",
              "value": "firstId"
            }
          ]
        }
      }
    }
)apl";

/**
 * This test places two sequences side-by-side.  The first preserves "firstIndex"; the second preserves "firstId".
 * The children of the sequence are conditionally inflated by the theme; "light" -> 12 children,
 * "dark" -> every other child (6 children total).
 *
 * We scroll up and down and verify that resizing, relaying out, and adding/removing children preserves the scroll
 * position for both sequences.
 */
TEST_F(BuilderPreserveScrollTest, SequencesPreserveFirst)
{
    // The child height is always 50% of the screen height
    // If the theme is not "light", then odd child components are dropped
    metrics.size(200,200).theme("light");
    loadDocument(SEQUENCES_PRESERVE_FIRST);
    ASSERT_TRUE(component);

    const std::string INDEX = "SEQUENCE-INDEX";
    const std::string ID = "SEQUENCE-ID";

    ASSERT_TRUE(IsEqual(Rect(0,0,100,200), getCalc(INDEX, kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(100,0,100,200), getCalc(ID, kPropertyBounds)));

    // Scroll both of the sequences down and reinflate without a size change
    // This will place the third (index 2) component with 25% of it hiding off the top of the Sequence
    setScroll(INDEX, 225);
    setScroll(ID, 225);
    configChangeReinflate(ConfigurationChange(300,200));  // Child size remains the same; scroll position is the same
    ASSERT_EQ(225, getCalc(INDEX, kPropertyScrollPosition).asNumber());
    ASSERT_EQ(225, getCalc(ID, kPropertyScrollPosition).asNumber());

    // Double the height of the text boxes by doubling the screen height
    // The child height will double, which makes the scroll position double
    configChangeReinflate(ConfigurationChange(300,400));
    ASSERT_EQ(450, getCalc(INDEX, kPropertyScrollPosition).asNumber());
    ASSERT_EQ(450, getCalc(ID, kPropertyScrollPosition).asNumber());

    // Change the theme to dark.  This will cause the odd-numbered components to disappear
    // The INDEX-saving sequence stays at the same scroll position (which shows index=2, 25% off the top)
    // The ID-saving sequence switches to a new scroll position (which shows index=1, 25% off the top)
    configChangeReinflate(ConfigurationChange().theme("dark"));
    ASSERT_EQ(450, getCalc(INDEX, kPropertyScrollPosition).asNumber());
    ASSERT_EQ(250, getCalc(ID, kPropertyScrollPosition).asNumber());

    // Change the theme back to light and set the text height to 100.  All the components re-appear.  The scroll positions
    // go back to what they were before we threw away half of the components
    configChangeReinflate(ConfigurationChange(200,200).theme("light"));
    ASSERT_EQ(225, getCalc(INDEX, kPropertyScrollPosition).asNumber());   // The scroll position is tracking the INDEX
    ASSERT_EQ(225, getCalc(ID, kPropertyScrollPosition).asNumber());  // The scroll position is tracking the ID - which goes back

    // Scroll down so that the fourth child (index=3) is just at the top of the screen
    // The ID-saving sequence doesn't work because the component no longer exists. It goes back to 0.
    // The INDEX-saving sequence works and stays in the same place
    setScroll(INDEX, 300);
    setScroll(ID, 300);
    configChangeReinflate(ConfigurationChange().theme("dark"));
    ASSERT_EQ(300, getCalc(INDEX, kPropertyScrollPosition).asNumber());
    ASSERT_EQ(0, getCalc(ID, kPropertyScrollPosition).asNumber());
    ASSERT_TRUE(ConsoleMessage());   // There should be an exception warning that we can't find a component

    // Reshow ALL of the components, scroll down to the very bottom of the list, and HIDE all of the components.
    // This will put the 11th component (index=10) at the top of the screen.
    // The ID-saving sequence will work because the component still exists
    // The INDEX-saving sequence will fail because the component no longer exists.
    configChangeReinflate(ConfigurationChange().theme("light"));
    setScroll(INDEX, 100000);
    setScroll(ID, 100000);
    ASSERT_EQ(1000, getCalc(INDEX, kPropertyScrollPosition).asNumber());  // Sanity check our scroll position
    ASSERT_EQ(1000, getCalc(ID, kPropertyScrollPosition).asNumber());
    configChangeReinflate(ConfigurationChange().theme("dark"));   // Throw away half of the components
    ASSERT_EQ(0, getCalc(INDEX, kPropertyScrollPosition).asNumber());
    ASSERT_EQ(400, getCalc(ID, kPropertyScrollPosition).asNumber());  // This is the max scroll position
    ASSERT_TRUE(ConsoleMessage());   // There should be an exception warning that we can't find a component
}



const static char *SEQUENCES_PRESERVE_CENTER = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "onConfigChange": {
        "type": "Reinflate"
      },
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": "100%",
          "height": "100%",
          "direction": "row",
          "items": {
            "type": "Sequence",
            "bind": {
              "name": "NAME",
              "value": "${data.name}"
            },
            "id": "SEQUENCE-${NAME}",
            "preserve": [
              "${data.value}"
            ],
            "width": "50%",
            "height": "100%",
            "items": {
              "type": "Text",
              "when": "${viewport.theme == 'light' || data % 2 == 0}",
              "id": "TEXT-${NAME}-${data}",
              "text": "text-${NAME}-${data}",
              "width": "100%",
              "height": "50vh"
            },
            "data": "${Array.range(16)}"
          },
          "data": [
            {
              "name": "INDEX",
              "value": "centerIndex"
            },
            {
              "name": "ID",
              "value": "centerId"
            }
          ]
        }
      }
    }
)apl";

/**
 * This test places two sequences side-by-side.  The first preserves "centerIndex"; the second preserves "centerId".
 * The children of the sequence are conditionally inflated by the theme; "light" -> 16 children,
 * "dark" -> every other child (8 children total).
 *
 * Exactly two children fit on the screen at one time.
 *
 * We scroll up and down and verify that resizing, relaying out, and adding/removing children preserves the scroll
 * position for both sequences.
 */
TEST_F(BuilderPreserveScrollTest, SequencesPreserveCenter)
{
    // The child height is always 50% of the screen height
    // If the theme is not "light", then odd child components are dropped
    metrics.size(200,200).theme("light");
    loadDocument(SEQUENCES_PRESERVE_CENTER);
    ASSERT_TRUE(component);

    const std::string INDEX = "SEQUENCE-INDEX";
    const std::string ID = "SEQUENCE-ID";

    ASSERT_TRUE(IsEqual(Rect(0,0,100,200), getCalc(INDEX, kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(100,0,100,200), getCalc(ID, kPropertyBounds)));

    // Scroll both of the sequences down and reinflate without a size change
    // This will place the fifth (index 4) component hanging down from the center by 25% (child height=100)
    setScroll(INDEX, 325);
    setScroll(ID, 325);
    LOG(LogLevel::kDebug) << getProp(INDEX, kPropertyCenterIndex);
    configChangeReinflate(ConfigurationChange(300,200));  // Child size remains the same; scroll position is the same
    ASSERT_EQ(325, getCalc(INDEX, kPropertyScrollPosition).asNumber());
    ASSERT_EQ(325, getCalc(ID, kPropertyScrollPosition).asNumber());

    // Double the height of the text boxes by doubling the screen height
    // The child height will double, which makes the scroll position double (child height=200)
    configChangeReinflate(ConfigurationChange(300,400));
    ASSERT_EQ(650, getCalc(INDEX, kPropertyScrollPosition).asNumber());
    ASSERT_EQ(650, getCalc(ID, kPropertyScrollPosition).asNumber());

    // Change the theme to dark.  This will cause the odd-numbered components to disappear
    // The INDEX-saving sequence stays at the same scroll position (which shows index=4, 25% hanging down)
    // The ID-saving sequence switches to a new scroll position (which shows index=2, 25% hanging down)
    configChangeReinflate(ConfigurationChange().theme("dark"));
    ASSERT_EQ(650, getCalc(INDEX, kPropertyScrollPosition).asNumber());
    ASSERT_EQ(250, getCalc(ID, kPropertyScrollPosition).asNumber());

    // Change the theme back to light and set the text height to 100.  All the components re-appear.  The scroll positions
    // go back to what they were before we threw away half of the components
    configChangeReinflate(ConfigurationChange(200,200).theme("light"));
    ASSERT_EQ(325, getCalc(INDEX, kPropertyScrollPosition).asNumber());   // The scroll position is tracking the INDEX
    ASSERT_EQ(325, getCalc(ID, kPropertyScrollPosition).asNumber());  // The scroll position is tracking the ID - which goes back

    // Scroll down so that the fourth child (index=3) is just at the top of the screen
    // The ID-saving sequence doesn't work because the component no longer exists. It goes back to 0.
    // The INDEX-saving sequence works and stays in the same place
    setScroll(INDEX, 300);
    setScroll(ID, 300);
    configChangeReinflate(ConfigurationChange().theme("dark"));
    ASSERT_EQ(300, getCalc(INDEX, kPropertyScrollPosition).asNumber());
    ASSERT_EQ(0, getCalc(ID, kPropertyScrollPosition).asNumber());
    ASSERT_TRUE(ConsoleMessage());   // There should be an exception warning that we can't find a component

    // Reshow ALL of the components, scroll down to the very bottom of the list, and HIDE all of the components.
    // This will put the 15th component (index=14) at the top of the screen.
    // The ID-saving sequence will work because the component still exists
    // The INDEX-saving sequence will fail because the component no longer exists.
    configChangeReinflate(ConfigurationChange().theme("light"));
    setScroll(INDEX, 100000);
    setScroll(ID, 100000);
    ASSERT_EQ(1400, getCalc(INDEX, kPropertyScrollPosition).asNumber());  // Sanity check our scroll position
    ASSERT_EQ(1400, getCalc(ID, kPropertyScrollPosition).asNumber());
    configChangeReinflate(ConfigurationChange().theme("dark"));   // Throw away half of the components
    ASSERT_EQ(0, getCalc(INDEX, kPropertyScrollPosition).asNumber());
    ASSERT_EQ(600, getCalc(ID, kPropertyScrollPosition).asNumber());  // This is the max scroll position (8 components, 2 per screen)
    ASSERT_TRUE(ConsoleMessage());   // There should be an exception warning that we can't find a component
}


static const char *HORIZONTAL_SEQUENCE_PRESERVE_PERCENT = R"apl(
    {
      "type": "APL",
      "version": "1.7",
      "onConfigChange": {
        "type": "Reinflate"
      },
      "mainTemplate": {
        "items": {
          "type": "Sequence",
          "scrollDirection": "horizontal",
          "layoutDirection": "RTL",
          "width": "100%",
          "height": "100%",
          "id": "SEQUENCE",
          "preserve": [
            "scrollPercent"
          ],
          "items": {
            "type": "Text",
            "id": "TEXT-${data}",
            "width": 100,
            "height": 100
          },
          "data": "${Array.range(10)}"
        }
      }
    }
)apl";

TEST_F(BuilderPreserveScrollTest, HorezontalSequencePercentRTL)
{
    metrics.size(200,200);
    loadDocument(HORIZONTAL_SEQUENCE_PRESERVE_PERCENT);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual(Dimension(0), component->getCalculated(kPropertyScrollPosition)));

    // Scroll down
    component->update(kUpdateScrollPosition, -100);  // This is 50% of the height of the scroll view (200 dp)
    root->clearPending();
    ASSERT_TRUE(IsEqual(Dimension(-100), component->getCalculated(kPropertyScrollPosition)));

    // Trigger reinflate
    auto old = component;
    configChangeReinflate(ConfigurationChange(100,100));
    ASSERT_TRUE(component);
    ASSERT_EQ(component->getId(), old->getId());
    ASSERT_TRUE(IsEqual(Dimension(-50), component->getCalculated(kPropertyScrollPosition)));  // This is 50% of the NEW height
}

const static char *SEQUENCES_PRESERVE_FIRST_LIVE = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "onConfigChange": {
        "type": "Reinflate"
      },
      "mainTemplate": {
        "items": {
          "type": "Container",
          "width": "100%",
          "height": "100%",
          "direction": "row",
          "items": {
            "type": "Sequence",
            "bind": {
              "name": "NAME",
              "value": "${data.name}"
            },
            "id": "SEQUENCE-${NAME}",
            "preserve": [
              "${data.value}"
            ],
            "width": "50%",
            "height": "100%",
            "items": {
              "type": "Text",
              "when": "${viewport.theme == 'light' || data % 2 == 0}",
              "id": "TEXT-${NAME}-${data}",
              "text": "text-${NAME}-${data}",
              "width": "100%",
              "height": "50vh"
            },
            "data": "${Array.range(12)}"
          },
          "data": "${TestArray}"
        }
      }
    }
)apl";

/**
 * Same as @see SequencesPreserveFirstLive but with LiveArray as a base for top container.
 */
TEST_F(BuilderPreserveScrollTest, SequencesPreserveFirstLive)
{
    // Define container through LiveArray
    auto firstElement = ObjectMap{{"name", "INDEX"}, {"value", "firstIndex"}};
    auto secondElement = ObjectMap{{"name", "ID"}, {"value", "firstId"}};
    auto myArray = LiveArray::create(ObjectArray{
        Object(std::make_shared<ObjectMap>(firstElement)),
        Object(std::make_shared<ObjectMap>(secondElement))});
    config->liveData("TestArray", myArray);

    // The child height is always 50% of the screen height
    // If the theme is not "light", then odd child components are dropped
    metrics.size(200,200).theme("light");
    loadDocument(SEQUENCES_PRESERVE_FIRST_LIVE);
    ASSERT_TRUE(component);

    const std::string INDEX = "SEQUENCE-INDEX";
    const std::string ID = "SEQUENCE-ID";

    ASSERT_TRUE(IsEqual(Rect(0,0,100,200), getCalc(INDEX, kPropertyBounds)));
    ASSERT_TRUE(IsEqual(Rect(100,0,100,200), getCalc(ID, kPropertyBounds)));

    // Scroll both of the sequences down and reinflate without a size change
    // This will place the third (index 2) component with 25% of it hiding off the top of the Sequence
    setScroll(INDEX, 225);
    setScroll(ID, 225);
    configChangeReinflate(ConfigurationChange(300,200));  // Child size remains the same; scroll position is the same
    ASSERT_EQ(225, getCalc(INDEX, kPropertyScrollPosition).asNumber());
    ASSERT_EQ(225, getCalc(ID, kPropertyScrollPosition).asNumber());

    // Double the height of the text boxes by doubling the screen height
    // The child height will double, which makes the scroll position double
    configChangeReinflate(ConfigurationChange(300,400));
    ASSERT_EQ(450, getCalc(INDEX, kPropertyScrollPosition).asNumber());
    ASSERT_EQ(450, getCalc(ID, kPropertyScrollPosition).asNumber());

    // Change the theme to dark.  This will cause the odd-numbered components to disappear
    // The INDEX-saving sequence stays at the same scroll position (which shows index=2, 25% off the top)
    // The ID-saving sequence switches to a new scroll position (which shows index=1, 25% off the top)
    configChangeReinflate(ConfigurationChange().theme("dark"));
    ASSERT_EQ(450, getCalc(INDEX, kPropertyScrollPosition).asNumber());
    ASSERT_EQ(250, getCalc(ID, kPropertyScrollPosition).asNumber());

    // Change the theme back to light and set the text height to 100.  All the components re-appear.  The scroll positions
    // go back to what they were before we threw away half of the components
    configChangeReinflate(ConfigurationChange(200,200).theme("light"));
    ASSERT_EQ(225, getCalc(INDEX, kPropertyScrollPosition).asNumber());   // The scroll position is tracking the INDEX
    ASSERT_EQ(225, getCalc(ID, kPropertyScrollPosition).asNumber());  // The scroll position is tracking the ID - which goes back

    // Scroll down so that the fourth child (index=3) is just at the top of the screen
    // The ID-saving sequence doesn't work because the component no longer exists. It goes back to 0.
    // The INDEX-saving sequence works and stays in the same place
    setScroll(INDEX, 300);
    setScroll(ID, 300);
    configChangeReinflate(ConfigurationChange().theme("dark"));
    ASSERT_EQ(300, getCalc(INDEX, kPropertyScrollPosition).asNumber());
    ASSERT_EQ(0, getCalc(ID, kPropertyScrollPosition).asNumber());
    ASSERT_TRUE(ConsoleMessage());   // There should be an exception warning that we can't find a component

    // Reshow ALL of the components, scroll down to the very bottom of the list, and HIDE all of the components.
    // This will put the 11th component (index=10) at the top of the screen.
    // The ID-saving sequence will work because the component still exists
    // The INDEX-saving sequence will fail because the component no longer exists.
    configChangeReinflate(ConfigurationChange().theme("light"));
    setScroll(INDEX, 100000);
    setScroll(ID, 100000);
    ASSERT_EQ(1000, getCalc(INDEX, kPropertyScrollPosition).asNumber());  // Sanity check our scroll position
    ASSERT_EQ(1000, getCalc(ID, kPropertyScrollPosition).asNumber());
    configChangeReinflate(ConfigurationChange().theme("dark"));   // Throw away half of the components
    ASSERT_EQ(0, getCalc(INDEX, kPropertyScrollPosition).asNumber());
    ASSERT_EQ(400, getCalc(ID, kPropertyScrollPosition).asNumber());  // This is the max scroll position
    ASSERT_TRUE(ConsoleMessage());   // There should be an exception warning that we can't find a component
}

static const char *HORIZONTAL_WITH_PADDING = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "onConfigChange": {
        "type": "Reinflate"
      },
      "mainTemplate": {
        "items": {
          "type": "Sequence",
          "scrollDirection": "horizontal",
          "paddingLeft": 100,
          "paddingRight": 50,
          "width": 300,
          "height": 100,
          "id": "SEQUENCE",
          "preserve": [
            "firstIndex"
          ],
          "items": {
            "type": "Text",
            "id": "TEXT-${data}",
            "width": 20,
            "height": 200
          },
          "data": "${Array.range(100)}"
        }
      }
    }
)apl";

/**
 * Horizontally scrolling sequence with asymetric padding.
 *
 *      Left Padding      Right Padding
 *            |              |
 * +------------------------------+
 * |          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |          |0|1|2|3|4|5|6|7|8|9|A|B|C|D|E|F|G|H|I|J|K|L|M|....
 * |          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * +------------------------------+
 *            |       |
 *          First   Center (takes into account padding)
 */
TEST_F(BuilderPreserveScrollTest, HorizontalWithPadding)
{
    metrics.size(300,300);
    loadDocument(HORIZONTAL_WITH_PADDING);
    ASSERT_TRUE(component);
    auto c = std::dynamic_pointer_cast<CoreComponent>(component);

    // Access the "getProperty" method to check the positions we are reading
    // The width of the sequence inner bounds is 150, so 7.5 Text blocks should fit, putting
    // the center in block index #3 shifted by 25%
    ASSERT_TRUE(IsEqual(ObjectArray{0, 0}, c->getProperty("firstIndex")));
    ASSERT_TRUE(IsEqual(ObjectArray{"TEXT-0", 0}, c->getProperty("firstId")));
    ASSERT_TRUE(IsEqual(ObjectArray{3, -0.25}, c->getProperty("centerIndex")));  // 100% shifted
    ASSERT_TRUE(IsEqual(ObjectArray{"TEXT-3", -0.25}, c->getProperty("centerId")));

    // Scroll over 10 units.  Now index=0 should be 50% off the screen
    component->update(kUpdateScrollPosition, 10);   // Move 10 units over
    ASSERT_EQ(10, component->getCalculated(kPropertyScrollPosition).asNumber());
    ASSERT_TRUE(IsEqual(ObjectArray{0, -0.5}, c->getProperty("firstIndex")));
    ASSERT_TRUE(IsEqual(ObjectArray{"TEXT-0", -0.5}, c->getProperty("firstId")));
    ASSERT_TRUE(IsEqual(ObjectArray{4, 0.25}, c->getProperty("centerIndex")));  // 100% shifted
    ASSERT_TRUE(IsEqual(ObjectArray{"TEXT-4", 0.25}, c->getProperty("centerId")));

    // Reinflate
    configChangeReinflate(ConfigurationChange(400,400));
    ASSERT_EQ(10, component->getCalculated(kPropertyScrollPosition).asNumber());
}


static const char *VERTICAL_WITH_PADDING = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "onConfigChange": {
        "type": "Reinflate"
      },
      "mainTemplate": {
        "items": {
          "type": "Sequence",
          "scrollDirection": "vertical",
          "paddingTop": 100,
          "paddingBottom": 50,
          "width": 100,
          "height": 300,
          "id": "SEQUENCE",
          "preserve": [
            "firstIndex"
          ],
          "items": {
            "type": "Text",
            "id": "TEXT-${data}",
            "width": 200,
            "height": 20
          },
          "data": "${Array.range(100)}"
        }
      }
    }
)apl";

/*
 * Same as the horizontal scroll test with padding, only vertical.
 */
TEST_F(BuilderPreserveScrollTest, VerticalWithPadding)
{
    metrics.size(300,300);
    loadDocument(VERTICAL_WITH_PADDING);
    ASSERT_TRUE(component);
    auto c = std::dynamic_pointer_cast<CoreComponent>(component);

    // Access the "getProperty" method to check the positions we are reading
    // The width of the sequence inner bounds is 150, so 7.5 Text blocks should fit, putting
    // the center in block index #3 shifted by 25%
    ASSERT_TRUE(IsEqual(ObjectArray{0, 0}, c->getProperty("firstIndex")));
    ASSERT_TRUE(IsEqual(ObjectArray{"TEXT-0", 0}, c->getProperty("firstId")));
    ASSERT_TRUE(IsEqual(ObjectArray{3, -0.25}, c->getProperty("centerIndex")));  // 100% shifted
    ASSERT_TRUE(IsEqual(ObjectArray{"TEXT-3", -0.25}, c->getProperty("centerId")));

    // Scroll over 10 units.  Now index=0 should be 50% off the screen
    component->update(kUpdateScrollPosition, 10);   // Move 10 units over
    ASSERT_EQ(10, component->getCalculated(kPropertyScrollPosition).asNumber());
    ASSERT_TRUE(IsEqual(ObjectArray{0, -0.5}, c->getProperty("firstIndex")));
    ASSERT_TRUE(IsEqual(ObjectArray{"TEXT-0", -0.5}, c->getProperty("firstId")));
    ASSERT_TRUE(IsEqual(ObjectArray{4, 0.25}, c->getProperty("centerIndex")));  // 100% shifted
    ASSERT_TRUE(IsEqual(ObjectArray{"TEXT-4", 0.25}, c->getProperty("centerId")));

    // Reinflate
    configChangeReinflate(ConfigurationChange(400,400));
    ASSERT_EQ(10, component->getCalculated(kPropertyScrollPosition).asNumber());
}


static const char *VERTICAL_WITH_PADDING_AND_SPACING = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "onConfigChange": {
        "type": "Reinflate"
      },
      "mainTemplate": {
        "items": {
          "type": "Sequence",
          "scrollDirection": "vertical",
          "paddingTop": 100,
          "paddingBottom": 50,
          "width": 100,
          "height": 300,
          "id": "SEQUENCE",
          "preserve": [
            "firstIndex"
          ],
          "items": {
            "type": "Text",
            "id": "TEXT-${data}",
            "width": 200,
            "height": 20,
            "spacing": "${10*(1+data)}"
          },
          "data": "${Array.range(10)}"
        }
      }
    }
)apl";

/*
 * Sequence children with spacing still need to be positioned correctly
 *
 * Index = 0: 100-120   (top padding)
 *         1: 140-160   (20 dp of spacing)
 *         2: 190-210   (30 dp of spacing)
 *         3: 250-270   (40 dp of spacing)
 *
 * Center of sequence innerBounds is at 175 dp, so centerIndex=1, offset=(150-175)/20 or -1.25
 */
TEST_F(BuilderPreserveScrollTest, VerticalWithPaddingAndSpacing)
{
    metrics.size(300,300);
    loadDocument(VERTICAL_WITH_PADDING_AND_SPACING);
    ASSERT_TRUE(component);
    auto c = std::dynamic_pointer_cast<CoreComponent>(component);

    // Access the "getProperty" method to check the positions we are reading
    ASSERT_TRUE(IsEqual(ObjectArray{0, 0}, c->getProperty("firstIndex")));
    ASSERT_TRUE(IsEqual(ObjectArray{"TEXT-0", 0}, c->getProperty("firstId")));
    ASSERT_TRUE(IsEqual(ObjectArray{1, -1.25}, c->getProperty("centerIndex")));  // 125% shifted
    ASSERT_TRUE(IsEqual(ObjectArray{"TEXT-1", -1.25}, c->getProperty("centerId")));

    // Scroll 10 units.
    // First will be index=0 should be 50% off the screen (0, -0.5)
    // Center will be index=2, offset=(200 - (175 + 10))/20 = 0.75
    component->update(kUpdateScrollPosition, 10);   // Move 10 units over
    ASSERT_EQ(10, component->getCalculated(kPropertyScrollPosition).asNumber());
    ASSERT_TRUE(IsEqual(ObjectArray{0, -0.5}, c->getProperty("firstIndex")));
    ASSERT_TRUE(IsEqual(ObjectArray{"TEXT-0", -0.5}, c->getProperty("firstId")));
    ASSERT_TRUE(IsEqual(ObjectArray{2, 0.75}, c->getProperty("centerIndex")));  //
    ASSERT_TRUE(IsEqual(ObjectArray{"TEXT-2", 0.75}, c->getProperty("centerId")));

    // Reinflate
    configChangeReinflate(ConfigurationChange(400,400));
    ASSERT_EQ(10, component->getCalculated(kPropertyScrollPosition).asNumber());
}



static const char * SWITCH_SEQUENCE_TYPE = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "onConfigChange": {
        "type": "Reinflate"
      },
      "layouts": {
        "MySequence": {
          "parameters": [
            "DATA"
          ],
          "items": [
            {
              "documentation": "Show a horizontal sequence on landscape device",
              "when": "${viewport.width > viewport.height}",
              "type": "Sequence",
              "scrollDirection": "horizontal",
              "preserve": "centerId",
              "items": {
                "type": "Text",
                "id": "TEXT-${data.id}",
                "text": "Text ${data.text}",
                "width": 100,
                "height": "100%"
              },
              "data": "${DATA}"
            },
            {
              "documentation": "Show a vertical two-column grid sequence on a portrait device",
              "type": "GridSequence",
              "scrollDirection": "vertical",
              "childWidth": "50%",
              "childHeight": 100,
              "preserve": "centerId",
              "items": {
                "type": "Text",
                "id": "TEXT-${data.id}",
                "text": "Text ${data.text}",
                "width": "100%",
                "height": "100%"
              },
              "data": "${DATA}"
            }
          ]
        }
      },
      "mainTemplate": {
        "items": {
          "type": "MySequence",
          "id": "SEQUENCE",
          "DATA": [
            { "id": "A", "text": "Apple" },
            { "id": "B", "text": "Banana" },
            { "id": "C", "text": "Cat" },
            { "id": "D", "text": "Dog" },
            { "id": "E", "text": "Elephant" },
            { "id": "F", "text": "Fox" },
            { "id": "G", "text": "Giraffe" },
            { "id": "H", "text": "House" },
            { "id": "I", "text": "Idea" },
            { "id": "J", "text": "Jack-o-Lantern" },
            { "id": "K", "text": "Kilo" }
          ],
          "width": "100%",
          "height": "100%"
        }
      }
    }
)apl";

/**
 * This test toggles between two-column vertical grid sequence and a horizontal sequence based on screen aspect ratio
 */
TEST_F(BuilderPreserveScrollTest, SwitchSequenceType)
{
    metrics.size(400, 300);  // Start in landscape mode
    loadDocument(SWITCH_SEQUENCE_TYPE);
    ASSERT_TRUE(component);

    component->update(kUpdateScrollPosition, 25);   // Move 25dp over
    ASSERT_EQ(25, component->getCalculated(kPropertyScrollPosition).asNumber());
    auto c = std::dynamic_pointer_cast<CoreComponent>(component);
    ASSERT_TRUE(IsEqual(ObjectArray{"TEXT-C", +0.25}, c->getProperty("centerId")));

    // When we switch to vertical two-column format, will try to put TEXT-B in the center, which is not possible
    // since it is pinned to the top of the sequence.  Hence scroll position returns to zero
    configChangeReinflate(ConfigurationChange(300,400));
    ASSERT_EQ(0, component->getCalculated(kPropertyScrollPosition).asNumber());

    // Move 25 dp.  TEXT-E should now be the "centered" component
    component->update(kUpdateScrollPosition, 25);
    c = std::dynamic_pointer_cast<CoreComponent>(component);
    ASSERT_TRUE(IsEqual(ObjectArray{"TEXT-E", +0.25}, c->getProperty("centerId")));

    // Switch back to horizontal
    // TEXT-E stays in the center with a 25% offset
    configChangeReinflate(ConfigurationChange(400,300));
    ASSERT_EQ(225, component->getCalculated(kPropertyScrollPosition).asNumber());

    // Changing back to vertical should go back to the 25dp scroll we started with
    configChangeReinflate(ConfigurationChange(300,400));
    ASSERT_EQ(25, component->getCalculated(kPropertyScrollPosition).asNumber());
}




