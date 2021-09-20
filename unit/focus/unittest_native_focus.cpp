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

#include "apl/focus/focusmanager.h"

using namespace apl;

class NativeFocusTest : public DocumentWrapper {
public:
    void
    prepareMainFocus() {
        auto& fm = root->context().focusManager();
        ASSERT_TRUE(component);

        executeCommand("SetFocus", {{"componentId", "initial"}}, false);
        ASSERT_EQ(component, fm.getFocus());

        auto event = root->popEvent();
        ASSERT_EQ(kEventTypeFocus, event.getType());
        ASSERT_EQ(component->getCalculated(kPropertyBounds), event.getValue(kEventPropertyValue));
    }

    ::testing::AssertionResult
    CheckFocusMapChildren(const std::map<std::string, Rect>& focusMap, std::vector<std::string> children) {
        if (focusMap.size() != children.size())
            return ::testing::AssertionFailure() << "Size mismatch";

        for (const auto& id : children) {
            auto child = root->findComponentById(id);
            if (!child)
                return ::testing::AssertionFailure() << "Non-existent child ID " << id;
            if (focusMap.count(child->getUniqueId()) == 0)
                return ::testing::AssertionFailure() << "Child: " << id << " is not in the focus map";
        }
        return ::testing::AssertionSuccess();
    }

    ::testing::AssertionResult
    eventGlobalBoundsEqual(const ComponentPtr& ptr, const Event& event) {
        Rect expectedBounds;
        ptr->getBoundsInParent(component, expectedBounds);
        auto eventBounds = event.getValue(kEventPropertyValue).getRect();
        if (expectedBounds != eventBounds) {
            return ::testing::AssertionFailure()
                << "Reported bounds. "
                << "Expected: " << expectedBounds.toDebugString()
                << ", actual: " << eventBounds.toDebugString();
        }
        return ::testing::AssertionSuccess();
    }

    ::testing::AssertionResult
    verifyFocusSwitchEvent(const ComponentPtr& ptr, const Event& event) {
        if (kEventTypeFocus != event.getType()) {
            return ::testing::AssertionFailure()
                << "Event type wrong. "
                << "Expected: " << kEventTypeFocus
                << ", actual: " << event.getType();
        }
        if (ptr != event.getComponent()) {
            return ::testing::AssertionFailure()
                << "Reported component wrong. "
                << "Expected: " << ptr->getUniqueId()
                << ", actual: " << event.getComponent()->getUniqueId();
        }
        auto boundsResult = eventGlobalBoundsEqual(ptr, event);
        if (boundsResult != ::testing::AssertionSuccess()) {
            return boundsResult;
        }

        return ::testing::AssertionSuccess();
    }

    ::testing::AssertionResult
    verifyFocusReleaseEvent(const ComponentPtr& ptr, const Event& event, FocusDirection direction) {
        if (kEventTypeFocus != event.getType()) {
            return ::testing::AssertionFailure()
                << "Event type wrong. "
                << "Expected: " << kEventTypeFocus
                << ", actual: " << event.getType();
        }
        if (event.getComponent()) {
            return ::testing::AssertionFailure()
                << "Not expecting component to be reported.";
        }
        if (direction != event.getValue(kEventPropertyDirection).getInteger()) {
            return ::testing::AssertionFailure()
                << "Focus move direction wrong. "
                << "Expected: " << direction
                << ", actual: " << event.getValue(kEventPropertyDirection).getInteger();
        }
        auto boundsResult = eventGlobalBoundsEqual(ptr, event);
        if (boundsResult != ::testing::AssertionSuccess()) {
            return boundsResult;
        }
        if (!event.getActionRef().isEmpty() && event.getActionRef().isPending()) {
            event.getActionRef().resolve(true);
        }
        root->clearPending();

        return ::testing::AssertionSuccess();
    }
};

static const char *SIMPLE_GRID = R"({
  "type": "APL",
  "version": "1.5",
  "theme": "dark",
  "layouts": {
    "Textbox": {
      "parameters": [
        "definedText"
      ],
      "item": {
        "type": "Frame",
        "inheritParentState": true,
        "style": "focusablePressableButton",
        "width": "100%",
        "height": "100%",
        "item": {
          "type": "Text",
          "inheritParentState": true,
          "style": "textStyleBody",
          "width": "100%",
          "height": "100%",
          "text": "${definedText}",
          "color": "black"
        }
      }
    },
    "Box": {
      "parameters": [
        "label"
      ],
      "item": {
        "type": "Container",
        "width": "100dp",
        "height": "100dp",
        "item": {
          "type": "Textbox",
          "definedText": "T ${label}"
        }
      }
    },
    "Button": {
      "parameters": [
        "label"
      ],
      "item": {
        "type": "TouchWrapper",
        "id": "${label}",
        "width": "100dp",
        "height": "100dp",
        "item": {
          "type": "Textbox",
          "definedText": "B ${label}"
        }
      }
    }
  },
  "resources": [
    {
      "colors": {
        "colorItemBase": "#D6DBDF",
        "colorItemPressed": "#808B96",
        "colorItemBorderNormal": "#566573",
        "colorItemBorderFocused": "#C0392B"
      }
    }
  ],
  "styles": {
    "textStyleBody": {
      "textAlign": "center",
      "textAlignVertical": "center",
      "color": "black"
    },
    "focusablePressableButton": {
      "extend": "textStyleBody",
      "values": [
        {
          "backgroundColor": "@colorItemBase",
          "borderColor": "@colorItemBorderNormal",
          "borderWidth": "2dp"
        },
        {
          "when": "${state.focused}",
          "borderColor": "@colorItemBorderFocused"
        },
        {
          "when": "${state.pressed}",
          "backgroundColor": "@colorItemPressed"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "height": "100%",
        "width": "100%",
        "direction": "column",
        "items": [
          {
            "type": "Container",
            "height": "auto",
            "width": "auto",
            "direction": "row",
            "data": [ "1.1", "1.2", "1.3" ],
            "items": [ { "type": "Button", "label": "${data}" } ]
          },
          {
            "type": "Container",
            "height": "auto",
            "width": "auto",
            "direction": "row",
            "data": [ "2.1", "2.2", "2.3" ],
            "items": [ { "type": "Button", "label": "${data}" } ]
          },
          {
            "type": "Container",
            "height": "auto",
            "width": "auto",
            "direction": "row",
            "data": [ "3.1", "3.2", "3.3" ],
            "items": [ { "type": "Button", "label": "${data}" } ]
          }
        ]
      }
    ]
  }
})";

TEST_F(NativeFocusTest, SimpleGridSet)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("22");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "22"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    executeCommand("SetFocus", {{"componentId", "11"}}, false);
    ASSERT_NE(child, fm.getFocus());

    child = root->findComponentById("11");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SimpleGridClear)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("22");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "22"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    executeCommand("ClearFocus", {}, false);
    ASSERT_TRUE(root->hasEvent());

    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(nullptr, event.getComponent().get());
    ASSERT_TRUE(event.getActionRef().isEmpty());
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, SimpleGridDown)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("22");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "22"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());

    child = root->findComponentById("32");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SimpleGridUp)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("22");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "22"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());

    child = root->findComponentById("12");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SimpleGridLeft)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("22");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "22"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());

    child = root->findComponentById("21");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SimpleGridRight)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("22");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "22"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());

    child = root->findComponentById("23");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SimpleGridRightFromEdgeExit)
{
    loadDocument(SIMPLE_GRID);

    auto& fm = root->context().focusManager();
    executeCommand("SetFocus", {{"componentId", "13"}}, false);

    auto child = root->findComponentById("13");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());

    ASSERT_TRUE(verifyFocusReleaseEvent(child, root->popEvent(), kFocusDirectionRight));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, SimpleGridLeftFromEdge)
{
    loadDocument(SIMPLE_GRID);

    auto& fm = root->context().focusManager();
    executeCommand("SetFocus", {{"componentId", "13"}}, false);

    auto child = root->findComponentById("13");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());

    child = root->findComponentById("12");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SimpleGridNext)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("11");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "11"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Tab order just goes through hierarchy in kind of DFS way.
    root->handleKeyboard(kKeyDown, Keyboard::TAB_KEY());
    child = root->findComponentById("12");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::TAB_KEY());
    child = root->findComponentById("13");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::TAB_KEY());
    child = root->findComponentById("21");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SimpleGridNextExit)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("33");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "33"}}, false);
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::TAB_KEY());

    ASSERT_TRUE(verifyFocusReleaseEvent(child, root->popEvent(), kFocusDirectionForward));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, SimpleGridPrevious)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("21");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "21"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Tab order just goes through hierarchy in kind of DFS way.
    root->handleKeyboard(kKeyDown, Keyboard::SHIFT_TAB_KEY());
    child = root->findComponentById("13");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::SHIFT_TAB_KEY());
    child = root->findComponentById("12");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::SHIFT_TAB_KEY());
    child = root->findComponentById("11");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SimpleGridPreviousExit)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("11");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "11"}}, false);
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::SHIFT_TAB_KEY());

    ASSERT_TRUE(verifyFocusReleaseEvent(child, root->popEvent(), kFocusDirectionBackwards));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, SimpleGridAPIDown)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();
    auto child = root->findComponentById("11");
    ASSERT_TRUE(child);
    ASSERT_TRUE(root->nextFocus(kFocusDirectionDown));
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SimpleGridAPIRight)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();
    auto child = root->findComponentById("11");
    ASSERT_TRUE(child);
    ASSERT_TRUE(root->nextFocus(kFocusDirectionRight));
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SimpleGridAPILeft)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();
    auto child = root->findComponentById("13");
    ASSERT_TRUE(child);
    ASSERT_TRUE(root->nextFocus(kFocusDirectionLeft));
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SimpleGridAPIUp)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();
    auto child = root->findComponentById("31");
    ASSERT_TRUE(child);
    ASSERT_TRUE(root->nextFocus(kFocusDirectionUp));
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SimpleGridAPIForward)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();
    auto child = root->findComponentById("11");
    ASSERT_TRUE(child);
    ASSERT_TRUE(root->nextFocus(kFocusDirectionForward));
    ASSERT_EQ(child->getId(), fm.getFocus()->getId());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SimpleGridAPIBackwards)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();
    auto child = root->findComponentById("33");
    ASSERT_TRUE(child);
    ASSERT_TRUE(root->nextFocus(kFocusDirectionBackwards));
    ASSERT_EQ(child->getId(), fm.getFocus()->getId());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char *SIMPLE_GRID_INVISIBLE = R"({
  "type": "APL",
  "version": "1.5",
  "theme": "dark",
  "layouts": {
    "Textbox": {
      "parameters": [
        "definedText"
      ],
      "item": {
        "type": "Frame",
        "inheritParentState": true,
        "style": "focusablePressableButton",
        "width": "100%",
        "height": "100%",
        "item": {
          "type": "Text",
          "inheritParentState": true,
          "style": "textStyleBody",
          "width": "100%",
          "height": "100%",
          "text": "${definedText}",
          "color": "black"
        }
      }
    },
    "Box": {
      "parameters": [
        "label"
      ],
      "item": {
        "type": "Container",
        "width": "100dp",
        "height": "100dp",
        "item": {
          "type": "Textbox",
          "definedText": "T ${label}"
        }
      }
    },
    "Button": {
      "parameters": [
        "label"
      ],
      "item": {
        "type": "TouchWrapper",
        "id": "${label}",
        "width": "100dp",
        "height": "100dp",
        "item": {
          "type": "Textbox",
          "definedText": "B ${label}"
        }
      }
    }
  },
  "resources": [
    {
      "colors": {
        "colorItemBase": "#D6DBDF",
        "colorItemPressed": "#808B96",
        "colorItemBorderNormal": "#566573",
        "colorItemBorderFocused": "#C0392B"
      }
    }
  ],
  "styles": {
    "textStyleBody": {
      "textAlign": "center",
      "textAlignVertical": "center",
      "color": "black"
    },
    "focusablePressableButton": {
      "extend": "textStyleBody",
      "values": [
        {
          "backgroundColor": "@colorItemBase",
          "borderColor": "@colorItemBorderNormal",
          "borderWidth": "2dp"
        },
        {
          "when": "${state.focused}",
          "borderColor": "@colorItemBorderFocused"
        },
        {
          "when": "${state.pressed}",
          "backgroundColor": "@colorItemPressed"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "height": "100%",
        "width": "100%",
        "direction": "column",
        "items": [
          {
            "type": "Container",
            "height": "auto",
            "width": "auto",
            "direction": "row",
            "data": [ "1.1", "1.2", "1.3" ],
            "items": [ { "type": "Button", "label": "${data}" } ]
          },
          {
            "type": "Container",
            "opacity": 0,
            "height": "auto",
            "width": "auto",
            "direction": "row",
            "data": [ "2.1", "2.2", "2.3" ],
            "items": [ { "type": "Button", "label": "${data}" } ]
          },
          {
            "type": "Container",
            "height": "auto",
            "width": "auto",
            "direction": "row",
            "data": [ "3.1", "3.2", "3.3" ],
            "items": [ { "type": "Button", "label": "${data}" } ]
          }
        ]
      }
    ]
  }
})";

TEST_F(NativeFocusTest, SimpleGridInvisible)
{
    loadDocument(SIMPLE_GRID_INVISIBLE);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("11");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "11"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());

    child = root->findComponentById("31");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SimpleGridInvisibleNext)
{
    loadDocument(SIMPLE_GRID_INVISIBLE);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("13");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "13"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::TAB_KEY());

    child = root->findComponentById("31");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SimpleGridInvisiblePrevious)
{
    loadDocument(SIMPLE_GRID_INVISIBLE);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("31");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "31"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::SHIFT_TAB_KEY());

    child = root->findComponentById("13");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char *SIMPLE_GRID_DISABLED = R"({
  "type": "APL",
  "version": "1.5",
  "theme": "dark",
  "layouts": {
    "Textbox": {
      "parameters": [
        "definedText"
      ],
      "item": {
        "type": "Frame",
        "inheritParentState": true,
        "style": "focusablePressableButton",
        "width": "100%",
        "height": "100%",
        "item": {
          "type": "Text",
          "inheritParentState": true,
          "style": "textStyleBody",
          "width": "100%",
          "height": "100%",
          "text": "${definedText}",
          "color": "black"
        }
      }
    },
    "Box": {
      "parameters": [
        "label"
      ],
      "item": {
        "type": "Container",
        "width": "100dp",
        "height": "100dp",
        "item": {
          "type": "Textbox",
          "definedText": "T ${label}"
        }
      }
    },
    "Button": {
      "parameters": [
        "label"
      ],
      "item": {
        "type": "TouchWrapper",
        "id": "${label}",
        "width": "100dp",
        "height": "100dp",
        "item": {
          "type": "Textbox",
          "definedText": "B ${label}"
        }
      }
    }
  },
  "resources": [
    {
      "colors": {
        "colorItemBase": "#D6DBDF",
        "colorItemPressed": "#808B96",
        "colorItemBorderNormal": "#566573",
        "colorItemBorderFocused": "#C0392B"
      }
    }
  ],
  "styles": {
    "textStyleBody": {
      "textAlign": "center",
      "textAlignVertical": "center",
      "color": "black"
    },
    "focusablePressableButton": {
      "extend": "textStyleBody",
      "values": [
        {
          "backgroundColor": "@colorItemBase",
          "borderColor": "@colorItemBorderNormal",
          "borderWidth": "2dp"
        },
        {
          "when": "${state.focused}",
          "borderColor": "@colorItemBorderFocused"
        },
        {
          "when": "${state.pressed}",
          "backgroundColor": "@colorItemPressed"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "height": "100%",
        "width": "100%",
        "direction": "column",
        "items": [
          { "type": "Button", "label": "1" },
          { "type": "Button", "label": "2", "disabled": true },
          { "type": "Button", "label": "3" }
        ]
      }
    ]
  }
})";

TEST_F(NativeFocusTest, SimpleGridDisabled)
{
    loadDocument(SIMPLE_GRID_DISABLED);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("1");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "1"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());

    child = root->findComponentById("3");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SimpleGridDisabledNext)
{
    loadDocument(SIMPLE_GRID_DISABLED);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("1");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "1"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::TAB_KEY());

    child = root->findComponentById("3");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SimpleGridDisabledPrevious)
{
    loadDocument(SIMPLE_GRID_DISABLED);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("3");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "3"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::SHIFT_TAB_KEY());

    child = root->findComponentById("1");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char *SIMPLE_GRID_OUT_OF_VIEWPORT = R"({
  "type": "APL",
  "version": "1.5",
  "theme": "dark",
  "layouts": {
    "Textbox": {
      "parameters": [
        "definedText"
      ],
      "item": {
        "type": "Frame",
        "inheritParentState": true,
        "style": "focusablePressableButton",
        "width": "100%",
        "height": "100%",
        "item": {
          "type": "Text",
          "inheritParentState": true,
          "style": "textStyleBody",
          "width": "100%",
          "height": "100%",
          "text": "${definedText}",
          "color": "black"
        }
      }
    },
    "Box": {
      "parameters": [
        "label"
      ],
      "item": {
        "type": "Container",
        "width": "100dp",
        "height": "100dp",
        "item": {
          "type": "Textbox",
          "definedText": "T ${label}"
        }
      }
    },
    "Button": {
      "parameters": [
        "label"
      ],
      "item": {
        "type": "TouchWrapper",
        "id": "${label}",
        "width": "100dp",
        "height": "100dp",
        "item": {
          "type": "Textbox",
          "definedText": "B ${label}"
        }
      }
    }
  },
  "resources": [
    {
      "colors": {
        "colorItemBase": "#D6DBDF",
        "colorItemPressed": "#808B96",
        "colorItemBorderNormal": "#566573",
        "colorItemBorderFocused": "#C0392B"
      }
    }
  ],
  "styles": {
    "textStyleBody": {
      "textAlign": "center",
      "textAlignVertical": "center",
      "color": "black"
    },
    "focusablePressableButton": {
      "extend": "textStyleBody",
      "values": [
        {
          "backgroundColor": "@colorItemBase",
          "borderColor": "@colorItemBorderNormal",
          "borderWidth": "2dp"
        },
        {
          "when": "${state.focused}",
          "borderColor": "@colorItemBorderFocused"
        },
        {
          "when": "${state.pressed}",
          "backgroundColor": "@colorItemPressed"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "height": 200,
        "width": 200,
        "direction": "column",
        "items": [
          { "type": "Button", "label": "1", "position": "absolute" },
          { "type": "Button", "label": "2", "position": "absolute", "top": "200" }
        ]
      }
    ]
  }
})";

TEST_F(NativeFocusTest, SimpleGridOutOfViewport)
{
    loadDocument(SIMPLE_GRID_OUT_OF_VIEWPORT);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("1");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "1"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    ASSERT_TRUE(root->hasEvent());

    ASSERT_TRUE(verifyFocusReleaseEvent(child, root->popEvent(), kFocusDirectionDown));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, SimpleGridOutOfViewportNext)
{
    loadDocument(SIMPLE_GRID_OUT_OF_VIEWPORT);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("1");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "1"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::TAB_KEY());
    ASSERT_TRUE(root->hasEvent());

    ASSERT_TRUE(verifyFocusReleaseEvent(child, root->popEvent(), kFocusDirectionForward));
    ASSERT_EQ(nullptr, fm.getFocus());
}

static const char* FUNKY_GRID = R"({
  "type": "APL",
  "version": "1.5",
  "layouts": {
      "Textbox": {
        "parameters": [
          "definedText"
        ],
        "item": {
          "type": "Frame",
          "inheritParentState": true,
          "style": "focusablePressableButton",
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Text",
            "inheritParentState": true,
            "style": "textStyleBody",
            "width": "100%",
            "height": "100%",
            "text": "${definedText}",
            "color": "black"
          }
        }
      },
      "Button": {
        "parameters": [
          "label"
        ],
        "item": {
          "type": "TouchWrapper",
          "id": "${label}",
          "width": 100,
          "height": 100,
          "item": {
            "type": "Textbox",
            "definedText": "B ${label}"
          }
        }
      }
    },
    "resources": [
      {
        "colors": {
          "colorItemBase": "#D6DBDF",
          "colorItemPressed": "#808B96",
          "colorItemBorderNormal": "#566573",
          "colorItemBorderFocused": "#C0392B"
        }
      }
    ],
    "styles": {
      "textStyleBody": {
        "textAlign": "center",
        "textAlignVertical": "center",
        "color": "black"
      },
      "focusablePressableButton": {
        "extend": "textStyleBody",
        "values": [
          {
            "backgroundColor": "@colorItemBase",
            "borderColor": "@colorItemBorderNormal",
            "borderWidth": "2dp"
          },
          {
            "when": "${state.focused}",
            "borderColor": "@colorItemBorderFocused"
          },
          {
            "when": "${state.pressed}",
            "backgroundColor": "@colorItemPressed"
          }
        ]
      }
    },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "height": 500,
      "width": 500,
      "items": [
        { "type": "Button", "label": "0", "position": "absolute", "left": 0, "top": 0 },
        { "type": "Button", "label": "1", "position": "absolute", "left": 100, "top": 200 },
        { "type": "Button", "label": "2", "position": "absolute", "left": 0, "top": 300 }
      ]
    }
  }
})";

TEST_F(NativeFocusTest, FunkyGridNarrowLeft)
{
    loadDocument(FUNKY_GRID);
    auto& fm = root->context().focusManager();
    auto child = root->findComponentById("1");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "1"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());

    child = root->findComponentById("2");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, FunkyGridNarrowDown)
{
    loadDocument(FUNKY_GRID);
    auto& fm = root->context().focusManager();
    auto child = root->findComponentById("0");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "0"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());

    child = root->findComponentById("2");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, FunkyGridNarrowUp)
{
    loadDocument(FUNKY_GRID);
    auto& fm = root->context().focusManager();
    auto child = root->findComponentById("2");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "2"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());

    child = root->findComponentById("0");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, FunkyGridNarrowRight)
{
    loadDocument(FUNKY_GRID);
    auto& fm = root->context().focusManager();
    auto child = root->findComponentById("0");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "0"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());

    child = root->findComponentById("1");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char* INTERSECT_GRID = R"({
  "type": "APL",
  "version": "1.5",
  "layouts": {
      "Textbox": {
        "parameters": [
          "definedText"
        ],
        "item": {
          "type": "Frame",
          "inheritParentState": true,
          "style": "focusablePressableButton",
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Text",
            "inheritParentState": true,
            "style": "textStyleBody",
            "width": "100%",
            "height": "100%",
            "text": "${definedText}",
            "color": "black"
          }
        }
      },
      "Button": {
        "parameters": [
          "label"
        ],
        "item": {
          "type": "TouchWrapper",
          "id": "${label}",
          "width": 100,
          "height": 100,
          "item": {
            "type": "Textbox",
            "definedText": "B ${label}"
          }
        }
      }
    },
    "resources": [
      {
        "colors": {
          "colorItemBase": "#D6DBDF",
          "colorItemPressed": "#808B96",
          "colorItemBorderNormal": "#566573",
          "colorItemBorderFocused": "#C0392B"
        }
      }
    ],
    "styles": {
      "textStyleBody": {
        "textAlign": "center",
        "textAlignVertical": "center",
        "color": "black"
      },
      "focusablePressableButton": {
        "extend": "textStyleBody",
        "values": [
          {
            "backgroundColor": "@colorItemBase",
            "borderColor": "@colorItemBorderNormal",
            "borderWidth": "2dp"
          },
          {
            "when": "${state.focused}",
            "borderColor": "@colorItemBorderFocused"
          },
          {
            "when": "${state.pressed}",
            "backgroundColor": "@colorItemPressed"
          }
        ]
      }
    },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "height": 500,
      "width": 500,
      "items": [
        { "type": "Button", "label": "0", "position": "absolute", "left": 0, "top": 0 },
        { "type": "Button", "label": "1", "position": "absolute", "left": 50, "top": 200 },
        { "type": "Button", "label": "2", "position": "absolute", "left": 0, "top": 300 }
      ]
    }
  }
})";

TEST_F(NativeFocusTest, IntersectGridNarrowLeft)
{
    loadDocument(INTERSECT_GRID);
    auto& fm = root->context().focusManager();
    auto child = root->findComponentById("1");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "1"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());

    child = root->findComponentById("2");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, IntersectGridNarrowDown)
{
    loadDocument(INTERSECT_GRID);
    auto& fm = root->context().focusManager();
    auto child = root->findComponentById("0");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "0"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());

    child = root->findComponentById("1");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, IntersectGridNarrowUp)
{
    loadDocument(INTERSECT_GRID);
    auto& fm = root->context().focusManager();
    auto child = root->findComponentById("2");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "2"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());

    child = root->findComponentById("1");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, IntersectGridNarrowRight)
{
    loadDocument(INTERSECT_GRID);
    auto& fm = root->context().focusManager();
    auto child = root->findComponentById("0");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "0"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());

    child = root->findComponentById("1");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char *PAGER = R"({
    "type": "APL",
    "version": "1.4",
    "theme": "dark",
    "layouts": {
        "Textbox": {
            "parameters": [
                "definedText"
            ],
            "item": {
                "type": "Frame",
                "inheritParentState": true,
                "style": "focusablePressableButton",
                "width": "100%",
                "height": "100%",
                "item": {
                    "type": "Text",
                    "inheritParentState": true,
                    "style": "textStyleBody",
                    "width": "100%",
                    "height": "100%",
                    "text": "${definedText}",
                    "color": "black"
                }
            }
        },
        "Box": {
            "parameters": [
                "label"
            ],
            "item": {
                "type": "Container",
                "width": "10vw",
                "height": "10vw",
                "item": {
                    "type": "Textbox",
                    "definedText": "T ${label}"
                }
            }
        },
        "Button": {
            "parameters": [
                "label"
            ],
            "item": {
                "type": "TouchWrapper",
                "id": "${label}",
                "width": "10vw",
                "height": "10vw",
                "item": {
                    "type": "Textbox",
                    "definedText": "B ${label}"
                }
            }
        }
    },
    "resources": [
        {
            "colors": {
                "colorItemBase": "#D6DBDF",
                "colorItemPressed": "#808B96",
                "colorItemBorderNormal": "#566573",
                "colorItemBorderFocused": "#C0392B"
            }
        }
    ],
    "styles": {
        "textStyleBody": {
            "textAlign": "center",
            "textAlignVertical": "center",
            "color": "black"
        },
        "focusablePressableButton": {
            "extend": "textStyleBody",
            "values": [
                {
                    "backgroundColor": "@colorItemBase",
                    "borderColor": "@colorItemBorderNormal",
                    "borderWidth": "2dp"
                },
                {
                    "when": "${state.focused}",
                    "borderColor": "@colorItemBorderFocused",
                    "backgroundColor": "yellow"
                },
                {
                    "when": "${state.pressed}",
                    "backgroundColor": "@colorItemPressed"
                }
            ]
        }
    },
    "mainTemplate": {
        "items": [
            {
                "type": "Container",
                "height": "600",
                "width": "1024",
                "id": "root",
                "direction": "row",
                "justifyContent": "spaceBetween",
                "alignItems": "center",
                "items": [
                    {
                        "type": "Button",
                        "label": "LF"
                    },
                    {
                        "type": "Container",
                        "height": "100%",
                        "width": "30%",
                        "direction": "column",
                        "alignItems": "center",
                        "justifyContent": "spaceBetween",
                        "items": [
                            {
                                "type": "Button",
                                "label": "TOP"
                            },
                            {
                                "type": "Pager",
                                "id": "pager",
                                "height": "55%",
                                "width": "100%",
                                "navigation": "wrap",
                                "items": [
                                    {
                                    "type": "Container",
                                    "height": "100%",
                                    "width": "100%",
                                    "direction": "column",
                                    "items": [
                                        {
                                        "type": "Container",
                                        "height": "auto",
                                        "width": "auto",
                                        "direction": "row",
                                        "data": [ "1.1", "1.2", "1.3" ],
                                        "items": [ { "type": "Button", "label": "${data}" } ]
                                        },
                                        {
                                        "type": "Container",
                                        "height": "auto",
                                        "width": "auto",
                                        "direction": "row",
                                        "data": [ "2.1", "2.2", "2.3" ],
                                        "items": [ { "type": "Button", "label": "${data}" } ]
                                        },
                                        {
                                        "type": "Container",
                                        "height": "auto",
                                        "width": "auto",
                                        "direction": "row",
                                        "data": [ "3.1", "3.2", "3.3" ],
                                        "items": [ { "type": "Button", "label": "${data}" } ]
                                        }
                                    ]
                                    },
                                    {
                                        "type": "Container",
                                        "height": "100%",
                                        "width": "100%",
                                        "item": [ { "type": "Box", "label": "2" } ]
                                    },
                                    {
                                        "type": "Container",
                                        "height": "100%",
                                        "width": "100%",
                                        "item": [ { "type": "Box", "label": "3" } ]
                                    },
                                    {
                                        "type": "Container",
                                        "height": "100%",
                                        "width": "100%",
                                        "alignItems": "center",
                                        "justifyContent": "center",
                                        "item": [ { "type": "Button", "label": "4" } ]
                                    },
                                    {
                                        "type": "Container",
                                        "height": "100%",
                                        "width": "100%",
                                        "item": [ { "type": "Box", "label": "5" } ]
                                    }
                                ]
                            },
                            {
                                "type": "Button",
                                "label": "BOT"
                            }
                        ]
                    },
                    {
                        "type": "Button",
                        "label": "RT"
                    }
                ]
            }
        ]
    }
}
)";

TEST_F(NativeFocusTest, PagerCombinationRight)
{
    loadDocument(PAGER);
    auto& fm = root->context().focusManager();
    executeCommand("SetFocus", {{"componentId", "LF"}}, false);

    auto child = root->findComponentById("LF");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Go into pager
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());

    child = root->findComponentById("21");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Inside of a pager
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());

    child = root->findComponentById("22");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, PagerCombinationRightExit)
{
    loadDocument(PAGER);
    auto& fm = root->context().focusManager();
    executeCommand("SetFocus", {{"componentId", "LF"}}, false);

    auto pager = root->findComponentById("pager");
    ASSERT_TRUE(pager);

    auto child = root->findComponentById("LF");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    executeCommand("SetFocus", {{"componentId", "23"}}, false);
    ASSERT_NE(child, fm.getFocus());

    child = root->findComponentById("23");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Exit
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());

    ASSERT_EQ(pager, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(pager, root->popEvent()));
}

TEST_F(NativeFocusTest, PagerCombinationUpFromBot)
{
    loadDocument(PAGER);
    auto& fm = root->context().focusManager();
    executeCommand("SetFocus", {{"componentId", "LF"}}, false);

    auto pager = root->findComponentById("pager");
    ASSERT_TRUE(pager);

    auto child = root->findComponentById("LF");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    executeCommand("SetFocus", {{"componentId", "23"}}, false);
    ASSERT_NE(child, fm.getFocus());

    child = root->findComponentById("23");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Exit to root
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());

    ASSERT_EQ(pager, fm.getFocus());
    advanceTime(1000);
    ASSERT_EQ(1, pager->pagePosition());

    ASSERT_EQ(pager, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(pager, root->popEvent()));

    executeCommand("SetFocus", {{"componentId", "BOT"}}, false);
    ASSERT_NE(child, fm.getFocus());

    child = root->findComponentById("BOT");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Enter
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());

    ASSERT_EQ(pager, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(pager, root->popEvent()));
}

TEST_F(NativeFocusTest, PagerCombinationNext)
{
    loadDocument(PAGER);
    auto& fm = root->context().focusManager();
    executeCommand("SetFocus", {{"componentId", "LF"}}, false);

    auto child = root->findComponentById("LF");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Go into pager
    root->handleKeyboard(kKeyDown, Keyboard::TAB_KEY());

    child = root->findComponentById("TOP");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Inside of a pager
    root->handleKeyboard(kKeyDown, Keyboard::TAB_KEY());

    child = root->findComponentById("pager");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
    ASSERT_EQ(0, child->pagePosition());
}

TEST_F(NativeFocusTest, PagerCombinationPageNext)
{
    loadDocument(PAGER);
    auto& fm = root->context().focusManager();
    executeCommand("SetFocus", {{"componentId", "33"}}, false);

    auto child = root->findComponentById("33");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Go into pager
    root->handleKeyboard(kKeyDown, Keyboard::TAB_KEY());

    child = root->findComponentById("pager");
    ASSERT_TRUE(child);

    ASSERT_EQ(child->getId(), fm.getFocus()->getId());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
    ASSERT_EQ(1, child->pagePosition());
}

TEST_F(NativeFocusTest, PagerCombinationPrevious)
{
    loadDocument(PAGER);
    auto& fm = root->context().focusManager();
    executeCommand("SetFocus", {{"componentId", "RT"}}, false);

    auto child = root->findComponentById("RT");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Go into pager
    root->handleKeyboard(kKeyDown, Keyboard::SHIFT_TAB_KEY());

    child = root->findComponentById("BOT");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Inside of a pager
    root->handleKeyboard(kKeyDown, Keyboard::SHIFT_TAB_KEY());

    child = root->findComponentById("33");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
    ASSERT_EQ(0, child->pagePosition());
}

TEST_F(NativeFocusTest, PagerCombinationPagePrevious)
{
    loadDocument(PAGER);
    auto& fm = root->context().focusManager();
    executeCommand("SetFocus", {{"componentId", "11"}}, false);

    auto child = root->findComponentById("11");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Go into pager
    root->handleKeyboard(kKeyDown, Keyboard::SHIFT_TAB_KEY());

    child = root->findComponentById("pager");
    ASSERT_TRUE(child);

    ASSERT_EQ(child->getId(), fm.getFocus()->getId());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
    ASSERT_EQ(4, child->pagePosition());
}

TEST_F(NativeFocusTest, PagerFirstLine)
{
    loadDocument(PAGER);
    auto& fm = root->context().focusManager();
    executeCommand("SetFocus", {{"componentId", "11"}}, false);

    auto child = root->findComponentById("11");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Go into pager
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());

    child = root->findComponentById("12");
    ASSERT_TRUE(child);

    ASSERT_EQ(child->getId(), fm.getFocus()->getId());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, PagerTrappedInB4Up)
{
    loadDocument(PAGER);
    auto& fm = root->context().focusManager();

    auto pager = root->findComponentById("pager");
    executeCommand("SetPage", {{"componentId", "pager"}, {"value", "3"}}, false);
    advanceTime(600);
    ASSERT_EQ(3, pager->pagePosition());

    executeCommand("SetFocus", {{"componentId", "4"}}, false);
    auto child = root->findComponentById("4");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Go into pager
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());

    child = root->findComponentById("TOP");
    ASSERT_TRUE(child);

    ASSERT_EQ(child->getId(), fm.getFocus()->getId());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, PagerTrappedInB4Down)
{
    loadDocument(PAGER);
    auto& fm = root->context().focusManager();

    auto pager = root->findComponentById("pager");
    executeCommand("SetPage", {{"componentId", "pager"}, {"value", "3"}}, false);
    advanceTime(600);
    ASSERT_EQ(3, pager->pagePosition());

    executeCommand("SetFocus", {{"componentId", "4"}}, false);
    auto child = root->findComponentById("4");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Go into pager
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());

    child = root->findComponentById("BOT");
    ASSERT_TRUE(child);

    ASSERT_EQ(child->getId(), fm.getFocus()->getId());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, PagerFocusInternalRight)
{
    loadDocument(PAGER);
    auto& fm = root->context().focusManager();

    auto pager = root->findComponentById("pager");
    executeCommand("SetPage", {{"componentId", "pager"}, {"value", "2"}}, false);
    advanceTime(600);
    ASSERT_EQ(2, pager->pagePosition());

    executeCommand("SetFocus", {{"componentId", "pager"}}, false);
    auto child = root->findComponentById("pager");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Go into pager
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());

    child = root->findComponentById("4");
    ASSERT_TRUE(child);

    ASSERT_EQ(child->getId(), fm.getFocus()->getId());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, PagerFocusInternalLeft)
{
    loadDocument(PAGER);
    auto& fm = root->context().focusManager();

    auto pager = root->findComponentById("pager");
    executeCommand("SetPage", {{"componentId", "pager"}, {"value", "4"}}, false);
    advanceTime(600);
    ASSERT_EQ(4, pager->pagePosition());

    executeCommand("SetFocus", {{"componentId", "pager"}}, false);
    auto child = root->findComponentById("pager");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Go into pager
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());

    child = root->findComponentById("4");
    ASSERT_TRUE(child);

    ASSERT_EQ(child->getId(), fm.getFocus()->getId());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char* CONFIGURABLE_PAGER = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "parameters": ["nav", "direction"],
    "item": {
      "type": "Pager",
      "id": "initial",
      "navigation": "${nav}",
      "pageDirection": "${direction}",
      "initialPage": 1,
      "height": 500,
      "width": 500,
      "data": ["red", "green", "yellow"],
      "items": [
        {
          "type": "Frame",
          "width": "100%",
          "height": "100%",
          "id": "${data}${index}",
          "backgroundColor":"${data}"
        }
      ]
    }
  }
})";

static const char *PAGER_HORIZONTAL_NORMAL = R"apl({
    "nav": "normal",
    "direction": "horizontal"
})apl";

TEST_F(NativeFocusTest, PagerNormalHorizontalForward) {
    loadDocument(CONFIGURABLE_PAGER, PAGER_HORIZONTAL_NORMAL);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    advanceTime(1000);
    ASSERT_EQ(2, component->pagePosition());

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    advanceTime(1000);
    ASSERT_EQ(2, component->pagePosition());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionRight));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, PagerNormalHorizontalBackwards) {
    loadDocument(CONFIGURABLE_PAGER, PAGER_HORIZONTAL_NORMAL);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());
    advanceTime(1000);
    ASSERT_EQ(0, component->pagePosition());

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());
    advanceTime(1000);
    ASSERT_EQ(0, component->pagePosition());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionLeft));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, PagerNormalHorizontalExitUp) {
    loadDocument(CONFIGURABLE_PAGER, PAGER_HORIZONTAL_NORMAL);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());
    advanceTime(1000);
    ASSERT_EQ(1, component->pagePosition());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionUp));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, PagerNormalHorizontalExitDown) {
    loadDocument(CONFIGURABLE_PAGER, PAGER_HORIZONTAL_NORMAL);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(1, component->pagePosition());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionDown));
    ASSERT_EQ(nullptr, fm.getFocus());
}

static const char *PAGER_VERTICAL_NORMAL = R"apl({
    "nav": "normal",
    "direction": "vertical"
})apl";

TEST_F(NativeFocusTest, PagerNormalVerticalForward) {
    loadDocument(CONFIGURABLE_PAGER, PAGER_VERTICAL_NORMAL);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(2, component->pagePosition());

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(2, component->pagePosition());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionDown));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, PagerNormalVerticalBackwards) {
    loadDocument(CONFIGURABLE_PAGER, PAGER_VERTICAL_NORMAL);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());
    advanceTime(1000);
    ASSERT_EQ(0, component->pagePosition());

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());
    advanceTime(1000);
    ASSERT_EQ(0, component->pagePosition());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionUp));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, PagerNormalVerticalExitLeft) {
    loadDocument(CONFIGURABLE_PAGER, PAGER_VERTICAL_NORMAL);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());
    advanceTime(1000);
    ASSERT_EQ(1, component->pagePosition());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionLeft));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, PagerNormalVerticalExitRight) {
    loadDocument(CONFIGURABLE_PAGER, PAGER_VERTICAL_NORMAL);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    advanceTime(1000);
    ASSERT_EQ(1, component->pagePosition());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionRight));
    ASSERT_EQ(nullptr, fm.getFocus());
}

static const char *PAGER_HORIZONTAL_WRAP = R"apl({
    "nav": "wrap",
    "direction": "horizontal"
})apl";

TEST_F(NativeFocusTest, PagerNormalHorizontalWrapForward) {
    loadDocument(CONFIGURABLE_PAGER, PAGER_HORIZONTAL_WRAP);
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    advanceTime(1000);
    ASSERT_EQ(2, component->pagePosition());

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    advanceTime(1000);
    ASSERT_EQ(0, component->pagePosition());
}

TEST_F(NativeFocusTest, PagerNormalHorizontalWrapBackwards) {
    loadDocument(CONFIGURABLE_PAGER, PAGER_HORIZONTAL_WRAP);
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());
    advanceTime(1000);
    ASSERT_EQ(0, component->pagePosition());

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());
    advanceTime(1000);
    ASSERT_EQ(2, component->pagePosition());
}

static const char *PAGER_HORIZONTAL_NONE = R"apl({
    "nav": "none",
    "direction": "horizontal"
})apl";

TEST_F(NativeFocusTest, PagerNormalHorizontalNoneForward) {
    loadDocument(CONFIGURABLE_PAGER, PAGER_HORIZONTAL_NONE);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    advanceTime(1000);
    ASSERT_EQ(1, component->pagePosition());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionRight));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, PagerNormalHorizontalNoneBackwards) {
    loadDocument(CONFIGURABLE_PAGER, PAGER_HORIZONTAL_NONE);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());
    advanceTime(1000);
    ASSERT_EQ(1, component->pagePosition());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionLeft));
    ASSERT_EQ(nullptr, fm.getFocus());
}

static const char *PAGER_HORIZONTAL_FO = R"apl({
    "nav": "forward-only",
    "direction": "horizontal"
})apl";

TEST_F(NativeFocusTest, PagerNormalHorizontalFOForward) {
    loadDocument(CONFIGURABLE_PAGER, PAGER_HORIZONTAL_FO);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    advanceTime(1000);
    ASSERT_EQ(2, component->pagePosition());

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    advanceTime(1000);
    ASSERT_EQ(2, component->pagePosition());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionRight));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, PagerNormalHorizontalFOBackwards) {
    loadDocument(CONFIGURABLE_PAGER, PAGER_HORIZONTAL_FO);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());
    advanceTime(1000);
    ASSERT_EQ(1, component->pagePosition());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionLeft));
    ASSERT_EQ(nullptr, fm.getFocus());
}

static const char* TOUCHABLE_PAGER = R"({
    "type": "APL",
    "version": "1.1",
    "layouts": {
        "Textbox": {
            "parameters": [
                "definedText"
            ],
            "item": {
                "type": "Frame",
                "inheritParentState": true,
                "style": "focusablePressableButton",
                "width": "100%",
                "height": "100%",
                "item": {
                    "type": "Text",
                    "inheritParentState": true,
                    "style": "textStyleBody",
                    "width": "100%",
                    "height": "100%",
                    "text": "${definedText}",
                    "color": "black"
                }
            }
        },
        "Box": {
            "parameters": [
                "label"
            ],
            "item": {
                "type": "Container",
                "width": "10vw",
                "height": "10vw",
                "item": {
                    "type": "Textbox",
                    "definedText": "T ${label}"
                }
            }
        },
        "Button": {
            "parameters": [
                "label"
            ],
            "item": {
                "type": "TouchWrapper",
                "id": "${label}",
                "width": "10vw",
                "height": "10vw",
                "item": {
                    "type": "Textbox",
                    "definedText": "B ${label}"
                }
            }
        }
    },
    "resources": [
        {
            "colors": {
                "colorItemBase": "#D6DBDF",
                "colorItemPressed": "#808B96",
                "colorItemBorderNormal": "#566573",
                "colorItemBorderFocused": "#C0392B"
            }
        }
    ],
    "styles": {
        "textStyleBody": {
            "textAlign": "center",
            "textAlignVertical": "center",
            "color": "black"
        },
        "focusablePressableButton": {
            "extend": "textStyleBody",
            "values": [
                {
                    "backgroundColor": "@colorItemBase",
                    "borderColor": "@colorItemBorderNormal",
                    "borderWidth": "2dp"
                },
                {
                    "when": "${state.focused}",
                    "borderColor": "@colorItemBorderFocused",
                    "backgroundColor": "yellow"
                },
                {
                    "when": "${state.pressed}",
                    "backgroundColor": "@colorItemPressed"
                }
            ]
        }
    },
    "mainTemplate": {
        "items": [
            {
                "type": "Container",
                "height": "100%",
                "width": "100%",
                "direction": "row",
                "justifyContent": "spaceBetween",
                "alignItems": "center",
                "items": [
                    {
                        "type": "Button",
                        "label": "LF"
                    },
                    {
                        "type": "Container",
                        "height": "100%",
                        "width": "30%",
                        "direction": "column",
                        "alignItems": "center",
                        "justifyContent": "spaceBetween",
                        "items": [
                            {
                                "type": "Button",
                                "label": "TOP"
                            },
                            {
                                "type": "Pager",
                                "id": "pager",
                                "height": "55%",
                                "width": "100%",
                                "navigation": "normal",
                                "items": [
                                    {
                                        "type": "Container",
                                        "height": "100%",
                                        "width": "100%",
                                        "item": [
                                            {
                                                "type": "Box",
                                                "label": "0"
                                            }
                                        ]
                                    },
                                    {
                                        "type": "Container",
                                        "height": "100%",
                                        "width": "100%",
                                        "alignItems": "center",
                                        "justifyContent": "center",
                                        "item": [
                                            {
                                                "type": "Button",
                                                "label": "1"
                                            }
                                        ]
                                    }
                                ]
                            },
                            {
                                "type": "Button",
                                "label": "BOT"
                            }
                        ]
                    },
                    {
                        "type": "Button",
                        "label": "RT"
                    }
                ]
            }
        ]
    }
})";

TEST_F(NativeFocusTest, PagerTouchablePassThrough) {
    loadDocument(TOUCHABLE_PAGER);
    ASSERT_TRUE(component);
    ASSERT_EQ(0, component->pagePosition());
    advanceTime(10);
    root->clearDirty();

    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "pager"}}, false);
    auto pager = root->findComponentById("pager");
    ASSERT_EQ(pager, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(pager, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    advanceTime(1000);
    ASSERT_EQ(1, pager->pagePosition());

    auto child = root->findComponentById("1");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char * SIMPLER_PAGER = R"({
    "type": "APL",
    "version": "1.5",
    "theme": "dark",
    "layouts": {
        "Button": {
            "parameters": [
                "label"
            ],
            "item": {
                "type": "TouchWrapper",
                "id": "${label}",
                "width": "10vw",
                "height": "10vw",
                "item": {
                    "type": "Frame",
                    "inheritParentState": true,
                    "style": "focusablePressableButton",
                    "width": "100%",
                    "height": "100%",
                    "item": {
                        "type": "Text",
                        "inheritParentState": true,
                        "textAlign": "center",
                        "textAlignVertical": "center",
                        "color": "black",
                        "width": "100%",
                        "height": "100%",
                        "text": "${label}"
                    }
                }
            }
        }
    },
    "resources": [
        {
            "colors": {
                "colorItemBase": "#D6DBDF",
                "colorItemPressed": "#808B96",
                "colorItemBorderNormal": "#566573",
                "colorItemBorderFocused": "#C0392B"
            }
        }
    ],
    "styles": {
        "focusablePressableButton": {
            "extend": "textStyleBody",
            "values": [
                {
                    "backgroundColor": "@colorItemBase",
                    "borderColor": "@colorItemBorderNormal",
                    "borderWidth": "2dp"
                },
                {
                    "when": "${state.focused}",
                    "borderColor": "@colorItemBorderFocused",
                    "backgroundColor": "yellow"
                },
                {
                    "when": "${state.pressed}",
                    "backgroundColor": "@colorItemPressed"
                }
            ]
        }
    },
    "mainTemplate": {
        "parameters": ["direction"],
        "items": [
            {
                "type": "Pager",
                "id": "pager",
                "height": "55%",
                "width": "100%",
                "navigation": "wrap",
                "pageDirection": "${direction}",
                "items": [
                    {
                        "type": "Container",
                        "height": "100%",
                        "width": "100%",
                        "direction": "column",
                        "data": [ "1.1", "1.2", "1.3" ],
                        "items": [ { "type": "Button", "label": "${data}" } ]
                    },
                    {
                        "type": "Container",
                        "height": "100%",
                        "width": "100%",
                        "direction": "column",
                        "data": [ "2.1", "2.2", "2.3" ],
                        "items": [ { "type": "Button", "label": "${data}" } ]
                    },
                    {
                        "type": "Container",
                        "height": "100%",
                        "width": "100%",
                        "direction": "column",
                        "data": [ "3.1", "3.2", "3.3" ],
                        "items": [ { "type": "Button", "label": "${data}" } ]
                    }
                ]
            }
        ]
    }
})";

TEST_F(NativeFocusTest, PagerSwitchBoundsRight) {
    loadDocument(SIMPLER_PAGER, R"({"direction": "horizontal"})");
    ASSERT_TRUE(component);
    ASSERT_EQ(0, component->pagePosition());
    advanceTime(10);
    root->clearDirty();

    auto& fm = root->context().focusManager();
    auto pager = root->findComponentById("pager");

    executeCommand("SetFocus", {{"componentId", "12"}}, false);
    auto child = root->findComponentById("12");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    advanceTime(1000);
    ASSERT_EQ(1, pager->pagePosition());

    child = root->findComponentById("22");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);

    child = root->findComponentById("23");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    advanceTime(1000);
    ASSERT_EQ(2, pager->pagePosition());

    child = root->findComponentById("33");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, PagerSwitchBoundsLeft) {
    loadDocument(SIMPLER_PAGER, R"({"direction": "horizontal"})");
    ASSERT_TRUE(component);
    ASSERT_EQ(0, component->pagePosition());
    advanceTime(10);
    root->clearDirty();

    auto& fm = root->context().focusManager();
    auto pager = root->findComponentById("pager");

    executeCommand("SetFocus", {{"componentId", "12"}}, false);
    auto child = root->findComponentById("12");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());
    advanceTime(1000);
    ASSERT_EQ(2, pager->pagePosition());

    child = root->findComponentById("32");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);

    child = root->findComponentById("33");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());
    advanceTime(1000);
    ASSERT_EQ(1, pager->pagePosition());

    child = root->findComponentById("23");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, PagerSwitchBoundsDown) {
    loadDocument(SIMPLER_PAGER, R"({"direction": "vertical"})");
    ASSERT_TRUE(component);
    ASSERT_EQ(0, component->pagePosition());

    auto& fm = root->context().focusManager();
    auto pager = root->findComponentById("pager");

    executeCommand("SetFocus", {{"componentId", "12"}}, false);
    auto child = root->findComponentById("12");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);

    child = root->findComponentById("13");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(1, pager->pagePosition());

    child = root->findComponentById("21");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, PagerSwitchBoundsUp) {
    loadDocument(SIMPLER_PAGER, R"({"direction": "vertical"})");
    ASSERT_TRUE(component);
    ASSERT_EQ(0, component->pagePosition());

    auto& fm = root->context().focusManager();
    auto pager = root->findComponentById("pager");

    executeCommand("SetFocus", {{"componentId", "12"}}, false);
    auto child = root->findComponentById("12");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());
    advanceTime(1000);

    child = root->findComponentById("11");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());
    advanceTime(1000);
    ASSERT_EQ(2, pager->pagePosition());

    child = root->findComponentById("33");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char *NESTED_EDITTEXT = R"({
  "type": "APL",
  "version": "1.6",
  "theme": "dark",
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "height": "100%",
        "width": "100%",
        "direction": "column",
        "data": [1, 2, 3],
        "items": [
          {
            "type": "TouchWrapper",
            "id": "tw${data}",
            "width": 200,
            "height": 100,
            "item": {
              "type": "EditText",
              "id": "e${data}",
              "text": "${data}"
            }
          }
        ]
      }
    ]
  }
})";

TEST_F(NativeFocusTest, NestedEditText)
{
    loadDocument(NESTED_EDITTEXT);
    auto& fm = root->context().focusManager();

    root->nextFocus(kFocusDirectionDown);

    auto child = root->findComponentById("e1");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->nextFocus(kFocusDirectionDown);
    child = root->findComponentById("e2");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    child = root->findComponentById("e3");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char* SCROLLVIEW = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "ScrollView",
      "id": "initial",
      "height": 500,
      "width": 400,
      "item": {
        "type": "Container",
        "height": 1000,
        "width": "100%",
        "data": ["red", "blue", "green", "yellow"],
        "items": [
          {
            "type": "Frame",
            "width": "100%",
            "height": 250,
            "id": "${data}${index}",
            "backgroundColor": "${data}"
          }
        ]
      }
    }
  }
})";

TEST_F(NativeFocusTest, ScrollViewDownUp) {
    loadDocument(SCROLLVIEW);
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(500, component->scrollPosition().getY());

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());
    advanceTime(1000);
    ASSERT_EQ(0, component->scrollPosition().getY());
}

TEST_F(NativeFocusTest, ScrollViewExitUp) {
    loadDocument(SCROLLVIEW);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionUp));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, ScrollViewExitLeft) {
    loadDocument(SCROLLVIEW);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionLeft));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, ScrollViewExitRight) {
    loadDocument(SCROLLVIEW);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionRight));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, ScrollViewExitDown) {
    loadDocument(SCROLLVIEW);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(500, component->scrollPosition().getY());

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(500, component->scrollPosition().getY());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionDown));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, ScrollViewExitNext) {
    loadDocument(SCROLLVIEW);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::TAB_KEY());
    advanceTime(1000);
    ASSERT_EQ(500, component->scrollPosition().getY());

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::TAB_KEY());
    advanceTime(1000);
    ASSERT_EQ(500, component->scrollPosition().getY());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionForward));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, ScrollViewExitPrevious) {
    loadDocument(SCROLLVIEW);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::SHIFT_TAB_KEY());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionBackwards));
    ASSERT_EQ(nullptr, fm.getFocus());
}

static const char* COMPLEX_SCROLL_VIEW = R"({
  "type": "APL",
  "version": "1.4",
  "layouts": {
      "Textbox": {
        "parameters": [
          "definedText"
        ],
        "item": {
          "type": "Frame",
          "inheritParentState": true,
          "style": "focusablePressableButton",
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Text",
            "inheritParentState": true,
            "style": "textStyleBody",
            "width": "100%",
            "height": "100%",
            "text": "${definedText}",
            "color": "black"
          }
        }
      },
      "Box": {
        "parameters": [
          "label"
        ],
        "item": {
          "type": "Container",
          "width": 400,
          "height": 250,
          "item": {
            "type": "Textbox",
            "definedText": "T ${label}"
          }
        }
      },
      "Button": {
        "parameters": [
          "label"
        ],
        "item": {
          "type": "TouchWrapper",
          "id": "${label}",
          "width": 400,
          "height": 250,
          "item": {
            "type": "Textbox",
            "definedText": "B ${label}"
          }
        }
      }
    },
    "resources": [
      {
        "colors": {
          "colorItemBase": "#D6DBDF",
          "colorItemPressed": "#808B96",
          "colorItemBorderNormal": "#566573",
          "colorItemBorderFocused": "#C0392B"
        }
      }
    ],
    "styles": {
      "textStyleBody": {
        "textAlign": "center",
        "textAlignVertical": "center",
        "color": "black"
      },
      "focusablePressableButton": {
        "extend": "textStyleBody",
        "values": [
          {
            "backgroundColor": "@colorItemBase",
            "borderColor": "@colorItemBorderNormal",
            "borderWidth": "2dp"
          },
          {
            "when": "${state.focused}",
            "borderColor": "@colorItemBorderFocused"
          },
          {
            "when": "${state.pressed}",
            "backgroundColor": "@colorItemPressed"
          }
        ]
      }
    },
  "mainTemplate": {
    "item": {
      "type": "ScrollView",
      "height": 500,
      "width": 400,
      "item": {
        "type": "Container",
        "height": "auto",
        "width": "100%",
        "items": [
          { "type": "Button", "label": "initial" },
          {
            "type": "Container",
            "height": "auto",
            "width": "100%",
            "data": [1,2,3,4],
            "items": [{ "type": "Box", "label": "${data}" }]
          }
        ]
      }
    }
  }
})";

TEST_F(NativeFocusTest, ComplexScrollViewExitDown) {
    loadDocument(COMPLEX_SCROLL_VIEW);
    ASSERT_TRUE(component);

    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "initial"}}, false);
    auto child = root->findComponentById("initial");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(500, component->scrollPosition().getY());

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(750, component->scrollPosition().getY());

    ASSERT_EQ(component, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(component, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionDown));
    ASSERT_EQ(nullptr, fm.getFocus());
}

static const char* SEQUENCE = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "id": "initial",
      "height": 500,
      "width": 400,
      "data": ["red", "blue", "green", "yellow"],
      "items": [
        {
          "type": "Frame",
          "width": "100%",
          "height": 250,
          "id": "${data}${index}",
          "backgroundColor": "${data}"
        }
      ]
    }
  }
})";

TEST_F(NativeFocusTest, SequenceDownUp) {
    loadDocument(SEQUENCE);
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(500, component->scrollPosition().getY());

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());
    advanceTime(1000);
    ASSERT_EQ(0, component->scrollPosition().getY());
}

TEST_F(NativeFocusTest, SequenceExitUp) {
    loadDocument(SEQUENCE);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionUp));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, SequenceExitLeft) {
    loadDocument(SEQUENCE);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionLeft));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, SequenceExitRight) {
    loadDocument(SEQUENCE);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionRight));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, SequenceExitDown) {
    loadDocument(SEQUENCE);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(500, component->scrollPosition().getY());

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(500, component->scrollPosition().getY());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionDown));
    ASSERT_EQ(nullptr, fm.getFocus());
}

static const char* HORIZONTAL_SEQUENCE_RTL = R"({
  "type": "APL",
  "version": "1.7",
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "layoutDirection": "RTL",
      "id": "initial",
      "scrollDirection": "horizontal",
      "height": 400,
      "width": 500,
      "data": ["red", "blue", "green", "yellow"],
      "items": [
        {
          "type": "Frame",
          "height": "100%",
          "width": 250,
          "id": "${data}${index}",
          "backgroundColor": "${data}"
        }
      ]
    }
  }
})";

TEST_F(NativeFocusTest, RTLHorizontalSequenceLeftRight) {
    loadDocument(HORIZONTAL_SEQUENCE_RTL);
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());
    advanceTime(1000);
    ASSERT_EQ(-500, component->scrollPosition().getX());

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    advanceTime(1000);
    ASSERT_EQ(0, component->scrollPosition().getX());

}

TEST_F(NativeFocusTest, RTLHorizontalSequenceExitRight) {
    loadDocument(HORIZONTAL_SEQUENCE_RTL);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionRight));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, RTLHorizontalSequenceExitUp) {
    loadDocument(HORIZONTAL_SEQUENCE_RTL);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionUp));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, RTLHorizontalSequenceExitDown) {
    loadDocument(HORIZONTAL_SEQUENCE_RTL);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionDown));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, RTLHorizontalSequenceExitLeft) {
    loadDocument(HORIZONTAL_SEQUENCE_RTL);
    auto& fm = root->context().focusManager();
    prepareMainFocus();

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());
    advanceTime(1000);
    ASSERT_EQ(-500, component->scrollPosition().getX());

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());
    advanceTime(1000);
    ASSERT_EQ(-500, component->scrollPosition().getX());

    ASSERT_TRUE(verifyFocusReleaseEvent(component, root->popEvent(), kFocusDirectionLeft));
    ASSERT_EQ(nullptr, fm.getFocus());
}

static const char* COMPLEX_SEQUENCE = R"({
  "type": "APL",
  "version": "1.4",
  "layouts": {
      "Textbox": {
        "parameters": [
          "definedText"
        ],
        "item": {
          "type": "Frame",
          "inheritParentState": true,
          "style": "focusablePressableButton",
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Text",
            "inheritParentState": true,
            "style": "textStyleBody",
            "width": "100%",
            "height": "100%",
            "text": "${definedText}",
            "color": "black"
          }
        }
      },
      "Box": {
        "parameters": [
          "label"
        ],
        "item": {
          "type": "Container",
          "width": 400,
          "height": 250,
          "item": {
            "type": "Textbox",
            "definedText": "T ${label}"
          }
        }
      },
      "Button": {
        "parameters": [
          "label"
        ],
        "item": {
          "type": "TouchWrapper",
          "id": "${label}",
          "width": 400,
          "height": 250,
          "item": {
            "type": "Textbox",
            "definedText": "B ${label}"
          }
        }
      }
    },
    "resources": [
      {
        "colors": {
          "colorItemBase": "#D6DBDF",
          "colorItemPressed": "#808B96",
          "colorItemBorderNormal": "#566573",
          "colorItemBorderFocused": "#C0392B"
        }
      }
    ],
    "styles": {
      "textStyleBody": {
        "textAlign": "center",
        "textAlignVertical": "center",
        "color": "black"
      },
      "focusablePressableButton": {
        "extend": "textStyleBody",
        "values": [
          {
            "backgroundColor": "@colorItemBase",
            "borderColor": "@colorItemBorderNormal",
            "borderWidth": "2dp"
          },
          {
            "when": "${state.focused}",
            "borderColor": "@colorItemBorderFocused"
          },
          {
            "when": "${state.pressed}",
            "backgroundColor": "@colorItemPressed"
          }
        ]
      }
    },
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "id": "scrollable",
      "height": 500,
      "width": 400,
      "data": [0,1,2,3,4],
      "items": [
        { "when": "${index == 0}", "type": "Button", "label": "${data}" },
        { "when": "${index == 4}", "type": "Button", "label": "${data}" },
        { "type": "Box", "label": "${data}" }
      ]
    }
  }
})";

TEST_F(NativeFocusTest, ComplexSequenceExitDown) {
    loadDocument(COMPLEX_SEQUENCE);
    advanceTime(10);
    ASSERT_TRUE(component);

    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "0"}}, false);
    auto child = root->findComponentById("0");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(750, component->scrollPosition().getY());
    child = root->findComponentById("4");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);

    ASSERT_TRUE(verifyFocusReleaseEvent(child, root->popEvent(), kFocusDirectionDown));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, ComplexSequenceEntryDown) {
    loadDocument(COMPLEX_SEQUENCE);
    ASSERT_TRUE(component);

    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "scrollable"}}, false);
    auto child = root->findComponentById("scrollable");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(0, component->scrollPosition().getY());
    child = root->findComponentById("0");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char* SNAP_SEQUENCE = R"({
  "type": "APL",
  "version": "1.5",
  "layouts": {
      "Textbox": {
        "parameters": [
          "definedText"
        ],
        "item": {
          "type": "Frame",
          "inheritParentState": true,
          "style": "focusablePressableButton",
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Text",
            "inheritParentState": true,
            "style": "textStyleBody",
            "width": "100%",
            "height": "100%",
            "text": "${definedText}",
            "color": "black"
          }
        }
      },
      "Button": {
        "parameters": [
          "label"
        ],
        "item": {
          "type": "TouchWrapper",
          "id": "${label}",
          "width": 400,
          "height": 250,
          "item": {
            "type": "Textbox",
            "definedText": "B ${label}"
          }
        }
      }
    },
    "resources": [
      {
        "colors": {
          "colorItemBase": "#D6DBDF",
          "colorItemPressed": "#808B96",
          "colorItemBorderNormal": "#566573",
          "colorItemBorderFocused": "#C0392B"
        }
      }
    ],
    "styles": {
      "textStyleBody": {
        "textAlign": "center",
        "textAlignVertical": "center",
        "color": "black"
      },
      "focusablePressableButton": {
        "extend": "textStyleBody",
        "values": [
          {
            "backgroundColor": "@colorItemBase",
            "borderColor": "@colorItemBorderNormal",
            "borderWidth": "2dp"
          },
          {
            "when": "${state.focused}",
            "borderColor": "@colorItemBorderFocused"
          },
          {
            "when": "${state.pressed}",
            "backgroundColor": "@colorItemPressed"
          }
        ]
      }
    },
  "mainTemplate": {
    "parameters": ["s"],
    "item": {
      "type": "Sequence",
      "snap": "${s}",
      "height": 500,
      "width": 400,
      "data": ["initial",1,2,3,4],
      "items": [
        { "type": "Button", "label": "${data}" }
      ]
    }
  }
})";

static const char* SNAP_CONFIG_START = R"({
    "s": "start"
})";

TEST_F(NativeFocusTest, SnapSequenceStart) {
    loadDocument(SNAP_SEQUENCE, SNAP_CONFIG_START);
    ASSERT_TRUE(component);

    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "initial"}}, false);
    auto child = root->findComponentById("initial");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(250, component->scrollPosition().getY());

    child = root->findComponentById("1");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(500, component->scrollPosition().getY());

    child = root->findComponentById("2");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());
    advanceTime(1000);
    ASSERT_EQ(250, component->scrollPosition().getY());

    child = root->findComponentById("1");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SnapSequenceStartNexpPrevious) {
    loadDocument(SNAP_SEQUENCE, SNAP_CONFIG_START);
    ASSERT_TRUE(component);

    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "initial"}}, false);
    auto child = root->findComponentById("initial");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::TAB_KEY());
    advanceTime(1000);
    ASSERT_EQ(250, component->scrollPosition().getY());

    child = root->findComponentById("1");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(500, component->scrollPosition().getY());

    child = root->findComponentById("2");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());
    advanceTime(1000);
    ASSERT_EQ(250, component->scrollPosition().getY());

    child = root->findComponentById("1");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char* SNAP_CONFIG_END = R"({
    "s": "end"
})";

TEST_F(NativeFocusTest, SnapSequenceEnd) {
    loadDocument(SNAP_SEQUENCE, SNAP_CONFIG_END);
    ASSERT_TRUE(component);

    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "initial"}}, false);
    auto child = root->findComponentById("initial");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(0, component->scrollPosition().getY());

    child = root->findComponentById("1");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(250, component->scrollPosition().getY());

    child = root->findComponentById("2");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());
    advanceTime(1000);
    ASSERT_EQ(0, component->scrollPosition().getY());

    child = root->findComponentById("1");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char* SNAP_CONFIG_CENTER = R"({
    "s": "center"
})";

TEST_F(NativeFocusTest, SnapSequenceCenter) {
    loadDocument(SNAP_SEQUENCE, SNAP_CONFIG_CENTER);
    ASSERT_TRUE(component);

    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "initial"}}, false);
    auto child = root->findComponentById("initial");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(125, component->scrollPosition().getY());

    child = root->findComponentById("1");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(375, component->scrollPosition().getY());

    child = root->findComponentById("2");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());
    advanceTime(1000);
    ASSERT_EQ(125, component->scrollPosition().getY());

    child = root->findComponentById("1");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char* SNAP_CONFIG_NONE = R"({
    "s": "none"
})";

TEST_F(NativeFocusTest, SnapSequenceNone) {
    loadDocument(SNAP_SEQUENCE, SNAP_CONFIG_NONE);
    ASSERT_TRUE(component);

    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "initial"}}, false);
    auto child = root->findComponentById("initial");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(0, component->scrollPosition().getY());

    child = root->findComponentById("1");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(250, component->scrollPosition().getY());

    child = root->findComponentById("2");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());
    advanceTime(1000);
    ASSERT_EQ(250, component->scrollPosition().getY());

    child = root->findComponentById("1");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char* SEQUENCE_WITH_TOUCHABLES = R"({
  "type": "APL",
  "version": "1.5",
  "layouts": {
    "Textbox": {
      "parameters": [
        "definedText"
      ],
      "item": {
        "type": "Frame",
        "inheritParentState": true,
        "style": "focusablePressableButton",
        "width": "100%",
        "height": "100%",
        "item": {
          "type": "Text",
          "inheritParentState": true,
          "style": "textStyleBody",
          "width": "100%",
          "height": "100%",
          "text": "${definedText}",
          "color": "black"
        }
      }
    },
    "Button": {
      "parameters": [
        "label"
      ],
      "item": {
        "type": "TouchWrapper",
        "id": "${label}",
        "width": "10vw",
        "height": "10vw",
        "item": {
          "type": "Textbox",
          "definedText": "B ${label}"
        }
      }
    }
  },
  "resources": [
    {
      "colors": {
        "colorItemBase": "#D6DBDF",
        "colorItemPressed": "#808B96",
        "colorItemBorderNormal": "#566573",
        "colorItemBorderFocused": "#C0392B"
      }
    }
  ],
  "styles": {
    "textStyleBody": {
      "textAlign": "center",
      "textAlignVertical": "center",
      "color": "black"
    },
    "focusablePressableButton": {
      "extend": "textStyleBody",
      "values": [
        {
          "backgroundColor": "@colorItemBase",
          "borderColor": "@colorItemBorderNormal",
          "borderWidth": "2dp"
        },
        {
          "when": "${state.focused}",
          "borderColor": "@colorItemBorderFocused",
          "backgroundColor": "yellow"
        },
        {
          "when": "${state.pressed}",
          "backgroundColor": "@colorItemPressed"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "height": "100%",
        "width": "100%",
        "direction": "row",
        "justifyContent": "spaceBetween",
        "alignItems": "center",
        "items": [
          { "type": "Button", "label": "LF" },
          {
            "type": "Container",
            "height": "100%",
            "width": "30%",
            "direction": "column",
            "alignItems": "center",
            "justifyContent": "spaceBetween",
            "items": [
              { "type": "Button", "label": "TOP" },
              {
                "type": "Sequence",
                "id": "scrollable",
                "height": "55%",
                "width": "100%",
                "data": [0,1,2,3,4],
                "items": [ { "type": "Button", "label": "${data}" } ]
              },
              { "type": "Button", "label": "BOT" }
            ]
          },
          { "type": "Button", "label": "RT" }
        ]
      }
    ]
  }
})";

TEST_F(NativeFocusTest, TouchableSequenceExitUp) {
    metrics.size(1024,600);
    loadDocument(SEQUENCE_WITH_TOUCHABLES);
    ASSERT_TRUE(component);

    auto scrollable = root->findComponentById("scrollable");
    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "1"}}, false);
    auto child = root->findComponentById("1");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());
    advanceTime(1000);
    ASSERT_EQ(0, scrollable->scrollPosition().getY());

    child = root->findComponentById("0");
    ASSERT_EQ(child->getId(), fm.getFocus()->getId());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());
    advanceTime(1000);
    ASSERT_EQ(0, scrollable->scrollPosition().getY());

    child = root->findComponentById("TOP");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, TouchableSequenceExitDown) {
    metrics.size(1024,600);
    loadDocument(SEQUENCE_WITH_TOUCHABLES);
    ASSERT_TRUE(component);

    auto scrollable = root->findComponentById("scrollable");
    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "2"}}, false);
    auto child = root->findComponentById("2");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(80, scrollable->scrollPosition().getY());

    child = root->findComponentById("3");
    ASSERT_EQ(child->getId(), fm.getFocus()->getId());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(182, scrollable->scrollPosition().getY());

    child = root->findComponentById("4");
    ASSERT_EQ(child->getId(), fm.getFocus()->getId());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);

    child = root->findComponentById("BOT");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, TouchableSequenceExitBack) {
    loadDocument(SEQUENCE_WITH_TOUCHABLES);
    ASSERT_TRUE(component);

    auto scrollable = root->findComponentById("scrollable");
    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "1"}}, false);
    auto child = root->findComponentById("1");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::SHIFT_TAB_KEY());
    advanceTime(1000);
    ASSERT_EQ(0, scrollable->scrollPosition().getY());

    child = root->findComponentById("0");
    ASSERT_EQ(child->getId(), fm.getFocus()->getId());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::SHIFT_TAB_KEY());
    advanceTime(1000);
    ASSERT_EQ(0, scrollable->scrollPosition().getY());

    child = root->findComponentById("scrollable");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, TouchableSequenceEnterFromRight) {
    metrics.size(1024,600);
    loadDocument(SEQUENCE_WITH_TOUCHABLES);
    ASSERT_TRUE(component);

    auto scrollable = root->findComponentById("scrollable");
    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "2"}}, false);
    auto child = root->findComponentById("2");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(80, scrollable->scrollPosition().getY());

    child = root->findComponentById("3");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    executeCommand("SetFocus", {{"componentId", "RT"}}, false);
    child = root->findComponentById("RT");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());
    advanceTime(1000);

    child = root->findComponentById("2");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, TouchableSequenceEnterFromBottom) {
    metrics.size(1024,600);
    loadDocument(SEQUENCE_WITH_TOUCHABLES);
    ASSERT_TRUE(component);

    auto scrollable = root->findComponentById("scrollable");
    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "BOT"}}, false);
    auto child = root->findComponentById("BOT");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());
    advanceTime(1000);
    ASSERT_EQ(80, scrollable->scrollPosition().getY());

    child = root->findComponentById("3");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char *NESTED_CAROUSEL_SEQUENCE = R"({
  "type": "APL",
  "version": "1.5",
  "layouts": {
    "Focusable": {
      "parameters": [
        "label"
      ],
      "item": {
        "type": "TouchWrapper",
        "id": "${label}",
        "width": 100,
        "height": 100,
        "item": {
          "type": "Frame",
          "inheritParentState": true,
          "style": "focusablePressableButton",
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Text",
            "inheritParentState": true,
            "textAlign": "center",
            "textAlignVertical": "center",
            "width": "100%",
            "height": "100%",
            "text": "B${label}",
            "color": "black"
          }
        }
      }
    }
  },
  "styles": {
    "focusablePressableButton": {
      "values": [
        {
          "backgroundColor": "#D6DBDF",
          "borderColor": "#566573",
          "borderWidth": "2dp"
        },
        {
          "when": "${state.focused}",
          "borderColor": "#C0392B",
          "backgroundColor": "yellow"
        },
        {
          "when": "${state.pressed}",
          "backgroundColor": "#808B96"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "id": "scrollable",
      "height": 300,
      "width": 500,
      "data": [0,1,2,3,4],
      "items": [
        {
          "type": "Sequence",
          "id": "scrollable",
          "scrollDirection": "horizontal",
          "height": 100,
          "width": 500,
          "data": ["${data}0","${data}1","${data}2","${data}3","${data}4","${data}5","${data}6"],
          "items": [
            {
              "type": "Focusable",
              "label": "${data}"
            }
          ]
        }
      ]
    }
  }
})";

TEST_F(NativeFocusTest, NestedSequenceParentTraversal) {
    loadDocument(NESTED_CAROUSEL_SEQUENCE);
    ASSERT_TRUE(component);

    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "20"}}, false);
    auto child = root->findComponentById("20");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(100, component->scrollPosition().getY());

    child = root->findComponentById("30");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);

    child = root->findComponentById("40");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    ASSERT_EQ(200, component->scrollPosition().getY());

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());
    advanceTime(1000);

    child = root->findComponentById("30");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    ASSERT_EQ(200, component->scrollPosition().getY());
}

TEST_F(NativeFocusTest, NestedSequenceCrossChildLeft) {
    loadDocument(NESTED_CAROUSEL_SEQUENCE);
    ASSERT_TRUE(component);

    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "06"}}, false);
    auto child = root->findComponentById("06");
    ASSERT_EQ(child, fm.getFocus());

    advanceTime(1000);
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
    ASSERT_EQ(200, component->getChildAt(0)->scrollPosition().getX());

    //////////////////////////////////////////////////////////////

    executeCommand("SetFocus", {{"componentId", "10"}}, false);
    child = root->findComponentById("10");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());
    ASSERT_TRUE(verifyFocusReleaseEvent(child, root->popEvent(), kFocusDirectionLeft));
}

TEST_F(NativeFocusTest, NestedSequenceCrossChildRight) {
    loadDocument(NESTED_CAROUSEL_SEQUENCE);
    ASSERT_TRUE(component);

    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "16"}}, false);
    auto child = root->findComponentById("16");
    ASSERT_EQ(child, fm.getFocus());

    advanceTime(1000);
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
    ASSERT_EQ(200, component->getChildAt(1)->scrollPosition().getX());

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    ASSERT_TRUE(verifyFocusReleaseEvent(child, root->popEvent(), kFocusDirectionRight));
}

TEST_F(NativeFocusTest, NestedSequenceRepeatKey) {
    loadDocument(NESTED_CAROUSEL_SEQUENCE);
    ASSERT_TRUE(component);

    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "03"}}, false);
    auto child = root->findComponentById("03");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    advanceTime(100);
    child = root->findComponentById("04");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    advanceTime(100);
    child = root->findComponentById("05");
    ASSERT_EQ(child, fm.getFocus());

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    advanceTime(100);
    ASSERT_EQ(child, fm.getFocus());

    advanceTime(100);
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char *EXITABLE_SEQUENCE = R"({
  "type": "APL",
  "version": "1.6",
  "layouts": {
    "Focusable": {
      "parameters": [
        "label"
      ],
      "item": {
        "type": "TouchWrapper",
        "id": "${label}",
        "width": 100,
        "height": 100,
        "item": {
          "type": "Frame",
          "inheritParentState": true,
          "style": "focusablePressableButton",
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Text",
            "inheritParentState": true,
            "textAlign": "center",
            "textAlignVertical": "center",
            "width": "100%",
            "height": "100%",
            "text": "B${label}",
            "color": "black"
          }
        }
      }
    },
    "Visible": {
      "parameters": [
        "label"
      ],
      "item": {
        "type": "Container",
        "id": "${label}",
        "width": 100,
        "height": 100,
        "item": {
          "type": "Frame",
          "inheritParentState": true,
          "style": "focusablePressableButton",
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Text",
            "inheritParentState": true,
            "textAlign": "center",
            "textAlignVertical": "center",
            "width": "100%",
            "height": "100%",
            "text": "T${label}",
            "color": "black"
          }
        }
      }
    }
  },
  "styles": {
    "focusablePressableButton": {
      "values": [
        {
          "backgroundColor": "#D6DBDF",
          "borderColor": "#566573",
          "borderWidth": "2dp"
        },
        {
          "when": "${state.focused}",
          "borderColor": "#C0392B",
          "backgroundColor": "yellow"
        },
        {
          "when": "${state.pressed}",
          "backgroundColor": "#808B96"
        }
      ]
    }
  },
  "onMount": {
    "type": "SetFocus",
    "componentId": "nfs"
  },
  "mainTemplate": {
    "items": {
      "type": "Container",
      "id": "scrollable",
      "height": 600,
      "width": 600,
      "items": [
        {
          "type": "Focusable",
          "label": "00",
          "left": 0
        },
        {
          "type": "Sequence",
          "id": "nfs",
          "width": 400,
          "scrollDirection": "horizontal",
          "height": 100,
          "data": [10,11,12,13,14],
          "items": [{"type": "Visible", "label": "${data}"}],
          "left": 100
        },
        {
          "type": "Sequence",
          "id": "fs",
          "scrollDirection": "horizontal",
          "width": 350,
          "height": 100,
          "data": [20,21,22,23],
          "items": [{"type": "Focusable", "label": "${data}"}],
          "left": 150
        },
        {
          "type": "Sequence",
          "id": "nfs",
          "width": 350,
          "scrollDirection": "horizontal",
          "height": 100,
          "data": [30,31,32,33],
          "items": [{"type": "Focusable", "label": "${data}"}],
          "left": 100
        },
        {
          "type": "Sequence",
          "id": "nfs",
          "width": 400,
          "scrollDirection": "horizontal",
          "height": 100,
          "data": [40,41,42,43,44],
          "items": [{"type": "Focusable", "label": "${data}"}],
          "left": 100
        },
        {
          "type": "Focusable",
          "label": 50,
          "left": 500
        }
      ]
    }
  }
})";

TEST_F(NativeFocusTest, ExitableSequenceFromVisible) {
    loadDocument(EXITABLE_SEQUENCE);
    ASSERT_TRUE(component);

    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("nfs");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    advanceTime(1000);
    ASSERT_EQ(child, fm.getFocus());

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    advanceTime(1000);
    child = root->findComponentById("50");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, ExitableSequenceProperPositionDownUp) {
    loadDocument(EXITABLE_SEQUENCE);
    ASSERT_TRUE(component);

    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("nfs");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    executeCommand("SetFocus", {{"componentId", "44"}}, false);
    advanceTime(1000);
    child = root->findComponentById("44");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    child = root->findComponentById("50");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());
    advanceTime(1000);
    child = root->findComponentById("44");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, ExitableSequenceProperPositionRight) {
    loadDocument(EXITABLE_SEQUENCE);
    ASSERT_TRUE(component);

    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("nfs");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    executeCommand("SetFocus", {{"componentId", "44"}}, false);
    advanceTime(1000);
    child = root->findComponentById("44");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    advanceTime(1000);
    child = root->findComponentById("50");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());
    advanceTime(1000);
    child = root->findComponentById("44");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char *JUMPING_SEQUENCE = R"({
    "type": "APL",
    "version": "1.6",
    "theme": "dark",
    "layouts": {
        "Textbox": {
            "parameters": ["definedText"],
            "item": {
                "type": "Frame",
                "inheritParentState": true,
                "style": "focusablePressableButton",
                "width": "100%",
                "height": "100%",
                "item": {
                    "type": "Text",
                    "inheritParentState": true,
                    "style": "textStyleBody",
                    "width": "100%",
                    "height": "100%",
                    "text": "${definedText}",
                    "color": "black"
                }
            }
        },
        "Box": {
            "parameters": ["label"],
            "item": {
                "type": "Container",
                "width": 100,
                "height": 100,
                "item": { "type": "Textbox", "definedText": "T${label}" }
            }
        },
        "Button": {
            "parameters": ["label"],
            "item": {
                "type": "TouchWrapper",
                "id": "${label}",
                "width": 100,
                "height": 100,
                "item": { "type": "Textbox", "definedText": "B${label}" }
            }
        }
    },
    "styles": {
        "textStyleBody": {
            "textAlign": "center",
            "textAlignVertical": "center",
            "color": "black"
        },
        "focusablePressableButton": {
            "extend": "textStyleBody",
            "values": [
                {
                    "backgroundColor": "#D6DBDF",
                    "borderColor": "#566573",
                    "borderWidth": "2dp"
                },
                {
                    "when": "${state.focused}",
                    "borderColor": "#C0392B"
                },
                {
                    "when": "${state.pressed}",
                    "backgroundColor": "#808B96"
                }
            ]
        }
    },
    "mainTemplate": {
        "items": [
            {
                "type": "Container",
                "height": "100%",
                "width": "100%",
                "direction": "column",
                "items": [
                    {
                        "type": "Sequence",
                        "height": 100,
                        "width": 250,
                        "scrollDirection": "horizontal",
                        "data": [1,2,3,4,5,6],
                        "items": {
                            "type": "Box",
                            "label": "${data}"
                        }
                    },
                    {
                        "type": "Sequence",
                        "height": 100,
                        "width": 250,
                        "scrollDirection": "horizontal",
                        "data": [10,11,12],
                        "items": { "type": "Button", "label": "${data}" }
                    },
                    {
                        "type": "Sequence",
                        "id": "lowerSequence",
                        "height": 100,
                        "width": 250,
                        "scrollDirection": "horizontal",
                        "items": [
                            { "type": "Box", "label": 20 },
                            { "type": "Button", "label": 21 },
                            { "type": "Box", "label": 22 },
                            { "type": "Button", "label": 23 }
                        ]
                    }
                ]
            }
        ]
    }
})";

TEST_F(NativeFocusTest, JumpingSequence) {
    loadDocument(JUMPING_SEQUENCE);
    ASSERT_TRUE(component);

    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "11"}}, false);
    auto child = root->findComponentById("11");
    ASSERT_EQ(child, fm.getFocus());
    root->clearPending();
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    advanceTime(1000);
    child = root->findComponentById("12");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    advanceTime(1000);
    ASSERT_TRUE(verifyFocusReleaseEvent(child, root->popEvent(), kFocusDirectionRight));
}

TEST_F(NativeFocusTest, WrapSequence) {
    loadDocument(JUMPING_SEQUENCE);
    ASSERT_TRUE(component);

    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "23"}}, false);
    advanceTime(1000);
    auto child = root->findComponentById("23");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());
    advanceTime(1000);
    child = root->findComponentById("21");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());
    advanceTime(1000);
    child = root->findComponentById("lowerSequence");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());
    advanceTime(1000);
    ASSERT_TRUE(verifyFocusReleaseEvent(child, root->popEvent(), kFocusDirectionLeft));
}

static const char *PAGER_TO_SEQUENCE = R"({
  "type": "APL",
  "version": "1.5",
  "layouts": {
    "Focusable": {
      "parameters": [
        "label"
      ],
      "item": {
        "type": "TouchWrapper",
        "id": "${label}",
        "width": 100,
        "height": 100,
        "item": {
          "type": "Frame",
          "inheritParentState": true,
          "style": "focusablePressableButton",
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Text",
            "inheritParentState": true,
            "textAlign": "center",
            "textAlignVertical": "center",
            "width": "100%",
            "height": "100%",
            "text": "B${label}",
            "color": "black"
          }
        }
      }
    }
  },
  "styles": {
    "focusablePressableButton": {
      "values": [
        {
          "backgroundColor": "#D6DBDF",
          "borderColor": "#566573",
          "borderWidth": "2dp"
        },
        {
          "when": "${state.focused}",
          "borderColor": "#C0392B",
          "backgroundColor": "yellow"
        },
        {
          "when": "${state.pressed}",
          "backgroundColor": "#808B96"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Container",
      "height": 300,
      "width": 500,
      "items": [
        {
          "type": "Sequence",
          "scrollDirection": "horizontal",
          "height": 100,
          "width": 500,
          "data": ["00","01","02","03","04","05","06"],
          "items": [
            {
              "type": "Focusable",
              "label": "${data}"
            }
          ]
        },
        {
          "type": "Pager",
          "id": "pager",
          "navigation": "normal",
          "height": 100,
          "width": 100,
          "data": ["10","11","12","13","14","15","16"],
          "items": [
            {
              "type": "Focusable",
              "label": "${data}"
            }
          ]
        }
      ]
    }
  }
})";

TEST_F(NativeFocusTest, PagerToSequenceCrossChild) {
    loadDocument(PAGER_TO_SEQUENCE);
    ASSERT_TRUE(component);

    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "06"}}, false);
    auto child = root->findComponentById("06");
    ASSERT_EQ(child, fm.getFocus());

    advanceTime(1000);
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
    ASSERT_EQ(200, component->getChildAt(0)->scrollPosition().getX());

    //////////////////////////////////////////////////////////////

    executeCommand("SetFocus", {{"componentId", "pager"}}, false);
    child = root->findComponentById("pager");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());
    ASSERT_TRUE(verifyFocusReleaseEvent(child, root->popEvent(), kFocusDirectionLeft));
}

static const char *SEQUENCE_PARALLEL_TO_CONTAINER = R"({
  "type": "APL",
  "version": "1.5",
  "layouts": {
    "Button": {
      "parameters": [
        "label"
      ],
      "item": {
        "type": "TouchWrapper",
        "id": "${label}",
        "width": 100,
        "height": 100,
        "item": {
          "type": "Frame",
          "inheritParentState": true,
          "style": "focusablePressableButton",
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Text",
            "inheritParentState": true,
            "width": "100%",
            "height": "100%",
            "text": "${label}",
            "textAlign": "center",
            "textAlignVertical": "center",
            "color": "black"
          }
        }
      }
    }
  },
  "styles": {
    "focusablePressableButton": {
      "extend": "textStyleBody",
      "values": [
        {
          "backgroundColor": "#D6DBDF",
          "borderColor": "#566573",
          "borderWidth": "2dp"
        },
        {
          "when": "${state.focused}",
          "borderColor": "#C0392B",
          "backgroundColor": "yellow"
        },
        {
          "when": "${state.pressed}",
          "backgroundColor": "#808B96"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "height": "100%",
        "width": "100%",
        "direction": "row",
        "items": [
          {
            "type": "Sequence",
            "id": "scrollable",
            "height": 250,
            "width": 200,
            "data": [0, 1, 2, 3, 4],
            "items": [{ "type": "Button", "label": "${data}" }]
          },
          {
            "type": "Container",
            "id": "scrollable",
            "height": 500,
            "width": 200,
            "data": [10, 11, 12, 13, 14],
            "items": [{ "type": "Button", "label": "${data}" }]
          }
        ]
      }
    ]
  }
})";

TEST_F(NativeFocusTest, SequenceExitRightToContainer) {
    loadDocument(SEQUENCE_PARALLEL_TO_CONTAINER);
    ASSERT_TRUE(component);

    auto& fm = root->context().focusManager();
    auto scrollable = root->findComponentById("scrollable");

    executeCommand("SetFocus", {{"componentId", "0"}}, false);
    auto child = root->findComponentById("0");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    //////////////////////////////////////////////////////////////

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    advanceTime(1000);

    child = root->findComponentById("10");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    // Should exit here as normal
    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);

    child = root->findComponentById("11");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);

    child = root->findComponentById("12");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());
    advanceTime(1000);
    ASSERT_EQ(50, scrollable->scrollPosition().getY());

    child = root->findComponentById("2");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}
TEST_F(NativeFocusTest, RuntimeAPIFocusablesSimple)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();
    ASSERT_TRUE(CheckFocusMapChildren(fm.getFocusableAreas(), {"11", "12", "13", "21", "22", "23", "31", "32", "33"}));
}

TEST_F(NativeFocusTest, RuntimeAPIFocusSimple)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    // Let's say we want to focus 21
    auto child = std::dynamic_pointer_cast<CoreComponent>(root->findComponentById("21"));
    auto result = root->setFocus(
        FocusDirection::kFocusDirectionRight,
        Rect(-100, 100, 100, 100),
        child->getUniqueId());

    ASSERT_TRUE(result);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, RuntimeAPIFocusablesParentPager)
{
    loadDocument(TOUCHABLE_PAGER);
    auto& fm = root->context().focusManager();
    ASSERT_TRUE(CheckFocusMapChildren(fm.getFocusableAreas(), {"TOP", "LF", "BOT", "RT", "pager"}));
}

TEST_F(NativeFocusTest, RuntimeAPIFocusParentPager)
{
    loadDocument(TOUCHABLE_PAGER);
    auto child = std::dynamic_pointer_cast<CoreComponent>(root->findComponentById("pager"));
    executeCommand("SetPage", {{"componentId", "pager"}, {"position", "relative"}, {"value", 1}}, false);
    advanceTime(1000);

    ASSERT_EQ(1, child->pagePosition());

    auto& fm = root->context().focusManager();
    auto result = root->setFocus(
        FocusDirection::kFocusDirectionRight,
        Rect(-100, 100, 100, 100),
        child->getUniqueId());

    ASSERT_TRUE(result);
    child = std::dynamic_pointer_cast<CoreComponent>(root->findComponentById("1"));
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, RuntimeAPIFocusablesParentSequence)
{
    loadDocument(SEQUENCE_WITH_TOUCHABLES);
    auto& fm = root->context().focusManager();
    ASSERT_TRUE(CheckFocusMapChildren(fm.getFocusableAreas(), {"TOP", "LF", "BOT", "RT", "scrollable"}));
}

TEST_F(NativeFocusTest, RuntimeAPIFocusParentSequence)
{
    loadDocument(SEQUENCE_WITH_TOUCHABLES);
    auto& fm = root->context().focusManager();

    auto child = std::dynamic_pointer_cast<CoreComponent>(root->findComponentById("scrollable"));
    auto result = root->setFocus(
        FocusDirection::kFocusDirectionRight,
        Rect(-100, 100, 100, 100),
        child->getUniqueId());

    ASSERT_TRUE(result);
    child = std::dynamic_pointer_cast<CoreComponent>(root->findComponentById("0"));
    LOG(LogLevel::kWarn) << fm.getFocus()->getId();
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, RuntimeAPIRelease)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "13"}}, false);
    auto child = root->findComponentById("13");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    ASSERT_TRUE(root->hasEvent());

    ASSERT_TRUE(verifyFocusReleaseEvent(child, root->popEvent(), kFocusDirectionRight));
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, RuntimeAPINoRelease)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "13"}}, false);
    auto child = root->findComponentById("13");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_FALSE(event.getComponent());
    ASSERT_EQ(kFocusDirectionRight, event.getValue(kEventPropertyDirection).getInteger());
    ASSERT_TRUE(eventGlobalBoundsEqual(child, event));

    event.getActionRef().resolve(false);
    root->clearPending();
    ASSERT_EQ(child, fm.getFocus());
}

TEST_F(NativeFocusTest, RuntimeAPIInterruptedRelease)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "13"}}, false);
    auto child = root->findComponentById("13");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    ASSERT_TRUE(root->hasEvent());
    auto resolveEvent = root->popEvent();

    ASSERT_EQ(kEventTypeFocus, resolveEvent.getType());
    ASSERT_FALSE(resolveEvent.getComponent());
    ASSERT_EQ(kFocusDirectionRight, resolveEvent.getValue(kEventPropertyDirection).getInteger());
    ASSERT_TRUE(eventGlobalBoundsEqual(child, resolveEvent));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());
    ASSERT_TRUE(root->hasEvent());
    child = root->findComponentById("12");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    ASSERT_TRUE(resolveEvent.getActionRef().isTerminated());
    root->clearPending();
    ASSERT_EQ(child, fm.getFocus());
}

TEST_F(NativeFocusTest, RuntimeAPIForceRelease)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    executeCommand("SetFocus", {{"componentId", "13"}}, false);
    auto child = root->findComponentById("13");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->clearFocus();
    ASSERT_EQ(nullptr, fm.getFocus());
}

TEST_F(NativeFocusTest, RuntimeAPINextUp)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("22");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "22"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    ASSERT_TRUE(root->nextFocus(kFocusDirectionUp));

    child = root->findComponentById("12");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, RuntimeAPINextDown)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("22");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "22"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    ASSERT_TRUE(root->nextFocus(kFocusDirectionDown));

    child = root->findComponentById("32");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, RuntimeAPINextLeft)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("22");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "22"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    ASSERT_TRUE(root->nextFocus(kFocusDirectionLeft));

    child = root->findComponentById("21");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, RuntimeAPINextRight)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("22");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "22"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    ASSERT_TRUE(root->nextFocus(kFocusDirectionRight));

    child = root->findComponentById("23");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, RuntimeAPINextForward)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("22");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "22"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    ASSERT_TRUE(root->nextFocus(kFocusDirectionForward));

    child = root->findComponentById("23");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, RuntimeAPINextBackwards)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("22");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "22"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    ASSERT_TRUE(root->nextFocus(kFocusDirectionBackwards));

    child = root->findComponentById("21");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, RuntimeAPIFocused)
{
    loadDocument(SIMPLE_GRID);
    auto& fm = root->context().focusManager();

    ASSERT_EQ(nullptr, fm.getFocus());
    ASSERT_EQ("", root->getFocused());

    auto child = root->findComponentById("22");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "22"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_EQ(child->getUniqueId(), root->getFocused());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char *COMPLEX_PAGER = R"({
    "type": "APL",
    "version": "1.5",
    "theme": "dark",
    "layouts": {
        "Textbox": {
            "parameters": ["definedText"],
            "item": {
                "type": "Frame",
                "inheritParentState": true,
                "style": "focusablePressableButton",
                "width": "100%",
                "height": "100%",
                "item": {
                    "type": "Text",
                    "inheritParentState": true,
                    "style": "textStyleBody",
                    "width": "100%",
                    "height": "100%",
                    "text": "${definedText}",
                    "color": "black"
                }
            }
        },
        "Button": {
            "parameters": ["label"],
            "item": {
                "type": "TouchWrapper",
                "id": "${label}",
                "width": "10vw",
                "height": "10vw",
                "item": {
                    "type": "Textbox",
                    "definedText": "B ${label}"
                }
            }
        }
    },
    "styles": {
        "textStyleBody": {
            "textAlign": "center",
            "textAlignVertical": "center",
            "color": "black"
        },
        "focusablePressableButton": {
            "extend": "textStyleBody",
            "values": [
                {
                    "backgroundColor": "#D6DBDF",
                    "borderColor": "#566573",
                    "borderWidth": "2dp"
                },
                {
                    "when": "${state.focused}",
                    "borderColor": "#C0392B",
                    "backgroundColor": "yellow"
                },
                {
                    "when": "${state.pressed}",
                    "backgroundColor": "#808B96"
                }
            ]
        }
    },
    "mainTemplate": {
        "items": [
            {
                "type": "Container",
                "height": "100%",
                "width": "100%",
                "direction": "row",
                "justifyContent": "spaceBetween",
                "alignItems": "center",
                "items": [
                    { "type": "Button", "label": "LF" },
                    {
                        "type": "Container",
                        "height": "100%",
                        "width": "30%",
                        "direction": "column",
                        "alignItems": "center",
                        "justifyContent": "spaceBetween",
                        "items": [
                            { "type": "Button", "label": "TOP" },
                            {
                                "type": "Pager",
                                "id": "pager",
                                "height": "55%",
                                "width": "100%",
                                "navigation": "normal",
                                "items": [
                                    {
                                        "type": "Container",
                                        "height": "100%",
                                        "width": "100%",
                                        "direction": "column",
                                        "items": [
                                            {
                                                "type": "Container",
                                                "height": "auto",
                                                "width": "auto",
                                                "direction": "row",
                                                "data": [11,12,13],
                                                "items": [{ "type": "Button", "label": "${data}" }]
                                            },
                                            {
                                                "type": "Container",
                                                "height": "auto",
                                                "width": "auto",
                                                "direction": "row",
                                                "data": [21,22,23],
                                                "items": [{ "type": "Button", "label": "${data}" }]
                                            },
                                            {
                                                "type": "Container",
                                                "height": "auto",
                                                "width": "auto",
                                                "direction": "row",
                                                "data": [31,32,33],
                                                "items": [{ "type": "Button", "label": "${data}" }]
                                            }
                                        ]
                                    },
                                    {
                                        "type": "Container",
                                        "height": "100%",
                                        "width": "auto",
                                        "direction": "row",
                                        "alignItems": "center",
                                        "data": [41,42,43],
                                        "items": [{ "type": "Button", "label": "${data}" }]
                                    }
                                ]
                            },
                            { "type": "Button", "label": "BOT" }
                        ]
                    },
                    { "type": "Button", "label": "RT" }
                ]
            }
        ]
    }
})";

TEST_F(NativeFocusTest, ComplexPagerLeft)
{
    loadDocument(COMPLEX_PAGER);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("23");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "23"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());

    child = root->findComponentById("22");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, ComplexPagerRight)
{
    loadDocument(COMPLEX_PAGER);
    auto& fm = root->context().focusManager();

    // Verify that if we have focus on a component outside the pager the focus doesn't change when
    // we change page.
    executeCommand("SetFocus", {{"componentId", "LF"}}, false);
    ASSERT_TRUE(verifyFocusSwitchEvent(root->findComponentById("LF"), root->popEvent()));
    ASSERT_EQ(root->findComponentById("LF"), fm.getFocus());

    auto pager = root->findComponentById("pager");
    executeCommand("SetPage", {{"componentId", "pager"}, {"value", "1"}}, false);
    advanceTime(1000);
    ASSERT_EQ(1, pager->pagePosition());

    // Make sure the focus hasn't changed
    ASSERT_EQ(root->findComponentById("LF"), fm.getFocus());

    auto child = root->findComponentById("41");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "41"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());

    child = root->findComponentById("42");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, ComplexPagerUp)
{
    loadDocument(COMPLEX_PAGER);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("32");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "32"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());

    child = root->findComponentById("22");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, ComplexPagerDown)
{
    loadDocument(COMPLEX_PAGER);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("12");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "12"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());

    child = root->findComponentById("22");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char *SIMPLE_GRID_WITH_NEXT = R"({
  "type": "APL",
  "version": "1.6",
  "theme": "dark",
  "layouts": {
    "Button": {
      "parameters": [
        "label"
      ],
      "item": {
        "type": "TouchWrapper",
        "id": "${label}",
        "width": 200,
        "height": 200,
        "item": {
          "type": "Frame",
          "inheritParentState": true,
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Text",
            "inheritParentState": true,
            "width": "100%",
            "height": "100%",
            "text": "${label}",
            "color": "black"
          }
        }
      }
    }
  },
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "height": "100%",
        "width": "100%",
        "direction": "column",
        "items": [
          {
            "type": "Container",
            "height": "auto",
            "width": "auto",
            "direction": "row",
            "data": [ "1.1", "1.2", "1.3" ],
            "items": [ { "type": "Button", "label": "${data}" } ]
          },
          {
            "type": "Container",
            "height": "auto",
            "width": "auto",
            "direction": "row",
            "data": [ "2.1", "2.2", "2.3" ],
            "items": [
              {
                "type": "Button",
                "label": "${data}",
                "nextFocusDown": "33",
                "nextFocusUp": "33",
                "nextFocusLeft": "33",
                "nextFocusRight": "33",
                "nextFocusForward": "33"
              }
            ]
          },
          {
            "type": "Container",
            "height": "auto",
            "width": "auto",
            "direction": "row",
            "data": [ "3.1", "3.2", "3.3" ],
            "items": [ { "type": "Button", "label": "${data}" } ]
          }
        ]
      }
    ]
  }
})";

TEST_F(NativeFocusTest, SimpleGridWithNextDown)
{
    loadDocument(SIMPLE_GRID_WITH_NEXT);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("22");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "22"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());

    child = root->findComponentById("33");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SimpleGridWithNextUp)
{
    loadDocument(SIMPLE_GRID_WITH_NEXT);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("22");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "22"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());

    child = root->findComponentById("33");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SimpleGridWithNextLeft)
{
    loadDocument(SIMPLE_GRID_WITH_NEXT);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("22");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "22"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_LEFT_KEY());

    child = root->findComponentById("33");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SimpleGridWithNextRight)
{
    loadDocument(SIMPLE_GRID_WITH_NEXT);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("22");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "22"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());

    child = root->findComponentById("33");
    ASSERT_TRUE(child);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, SimpleGridWithNextForward)
{
    loadDocument(SIMPLE_GRID_WITH_NEXT);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("22");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "22"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::TAB_KEY());

    child = root->findComponentById("33");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char* CONTAINERS_WITH_NEXT = R"({
  "type": "APL",
  "version": "1.6",
  "layouts": {
    "Button": {
      "parameters": [
        "label"
      ],
      "item": {
        "type": "TouchWrapper",
        "id": "${label}",
        "width": 200,
        "height": 200,
        "item": {
          "type": "Frame",
          "inheritParentState": true,
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Text",
            "inheritParentState": true,
            "width": "100%",
            "height": "100%",
            "text": "${label}",
            "color": "black"
          }
        }
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Container",
      "height": "100%",
      "width": "30%",
      "direction": "column",
      "alignItems": "center",
      "justifyContent": "spaceBetween",
      "items": [
        {
          "type": "Pager",
          "id": "pager",
          "height": 200,
          "width": "100%",
          "navigation": "wrap",
          "data": [ 0, 1, 2, 3, 4 ],
          "nextFocusForward": "BOT",
          "items": {
            "type": "Button",
            "label": "P${data}"
          }
        },
        {
          "type": "Sequence",
          "id": "scrollable",
          "height": 200,
          "width": "100%",
          "data": [ 0, 1, 2, 3, 4 ],
          "nextFocusForward": "BOT",
          "items": [
            {
              "type": "Button",
              "label": "S${data}"
            }
          ]
        },
        {
          "type": "Button",
          "label": "BOT"
        }
      ]
    }
  }
})";

TEST_F(NativeFocusTest, SequenceNextForward)
{
    loadDocument(CONTAINERS_WITH_NEXT);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("scrollable");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "scrollable"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::TAB_KEY());

    child = root->findComponentById("BOT");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, PagerNextForward)
{
    loadDocument(CONTAINERS_WITH_NEXT);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("pager");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "pager"}}, false);

    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::TAB_KEY());

    child = root->findComponentById("BOT");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char* GRID_SEQUENCE_NESTED = R"({
  "type": "APL",
  "version": "1.6",
  "styles": {
    "textStylePressable": {
      "values": [
        {
          "color": "white"
        },
        {
          "when": "${state.pressed}",
          "color": "orange"
        },
        {
          "when": "${state.focused}",
          "color": "green"
        },
        {
          "when": "${state.disabled}",
          "opacity": 0.5
        },
        {
          "when": "${!state.disabled}",
          "opacity": 1
        }
      ]
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": "100vw",
      "height": "100vh",
      "items": [
        {
          "type": "GridSequence",
          "width": "100vw",
          "height": "100vh",
          "id": "grid",
          "scrollDirection": "vertical",
          "childWidths": [
            "auto",
            "auto",
            "auto"
          ],
          "childHeight": "5vh",
          "snap": "start",
          "data": [
            { "text":  "First" },
            { "text":  "Second" },
            { "text":  "Third" },
            { "text":  "Fourth" },
            { "text":  "Fifth" },
            { "text":  "Sixth" }
          ],
          "items": [
            {
              "type": "TouchWrapper",
              "id": "grid${index}",
              "onPress": {
                "type": "SendEvent",
                "arguments": [
                  "${data.args}"
                ]
              },
              "item": {
                "type": "Text",
                "style": "textStylePressable",
                "inheritParentState": true,
                "fontSize": 24,
                "text": "${index + 1}. ${data.text}"
              }
            }
          ]
        }
      ]
    }
  }
})";

TEST_F(NativeFocusTest, NestedGridDown)
{
    loadDocument(GRID_SEQUENCE_NESTED);
    auto& fm = root->context().focusManager();

    ASSERT_TRUE(root->nextFocus(kFocusDirectionDown));

    auto child = root->findComponentById("grid0");
    ASSERT_TRUE(child);
    ASSERT_EQ(child->getId(), fm.getFocus()->getId());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, NestedGridNext)
{
    loadDocument(GRID_SEQUENCE_NESTED);
    auto& fm = root->context().focusManager();

    ASSERT_TRUE(root->nextFocus(kFocusDirectionForward));

    auto child = root->findComponentById("grid");
    ASSERT_TRUE(child);
    ASSERT_EQ(child->getId(), fm.getFocus()->getId());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::TAB_KEY());

    child = root->findComponentById("grid0");
    ASSERT_EQ(child->getId(), fm.getFocus()->getId());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char* PAGER_NESTED = R"({
  "type": "APL",
  "version": "1.6",
  "styles": {
    "textStylePressable": {
      "values": [
        {
          "color": "white"
        },
        {
          "when": "${state.pressed}",
          "color": "orange"
        },
        {
          "when": "${state.focused}",
          "color": "green"
        },
        {
          "when": "${state.disabled}",
          "opacity": 0.5
        },
        {
          "when": "${!state.disabled}",
          "opacity": 1
        }
      ]
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": "100vw",
      "height": "100vh",
      "items": [
        {
          "type": "Pager",
          "width": "100vw",
          "height": "100vh",
          "navigation": "none",
          "id": "pager",
          "data": [
            { "text":  "First" }
          ],
          "items": [
            {
              "type": "TouchWrapper",
              "id": "pager${index}",
              "onPress": {
                "type": "SendEvent",
                "arguments": [
                  "${data.args}"
                ]
              },
              "item": {
                "type": "Text",
                "style": "textStylePressable",
                "inheritParentState": true,
                "fontSize": 24,
                "text": "${index + 1}. ${data.text}"
              }
            }
          ]
        }
      ]
    }
  }
})";

TEST_F(NativeFocusTest, NestedPagerDown)
{
    loadDocument(PAGER_NESTED);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("pager");
    ASSERT_TRUE(root->nextFocus(kFocusDirectionDown));
    child = root->findComponentById("pager0");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

TEST_F(NativeFocusTest, NestedPagerNext)
{
    loadDocument(PAGER_NESTED);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("pager");
    ASSERT_TRUE(child);
    executeCommand("SetFocus", {{"componentId", "pager"}}, false);

    ASSERT_EQ(child->getId(), fm.getFocus()->getId());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::TAB_KEY());

    child = root->findComponentById("pager0");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char *LIVE_SEQUENCE = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "onMount": {
        "type": "SetFocus",
        "componentId": "0"
      },
      "scrollDirection": "vertical",
      "height": 200,
      "width": 100,
      "data": "${TestArray}",
      "items": [
        {
          "type": "TouchWrapper",
          "id": "${data}",
          "width": 100,
          "height": 100
        }
      ]
    }
  }
})";

TEST_F(NativeFocusTest, RemoveWhileFocused)
{
    auto myArray = LiveArray::create(ObjectArray{0,1});
    config->liveData("TestArray", myArray);

    loadDocument(LIVE_SEQUENCE);

    auto& fm = root->context().focusManager();
    auto child = root->findComponentById("0");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    root->popEvent();

    myArray->remove(0);
    root->clearPending();

    child = root->findComponentById("1");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    root->popEvent();

    myArray->clear();
    root->clearPending();

    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(event.getComponent(), nullptr);
    ASSERT_TRUE(event.getActionRef().isEmpty());

    // Releases as component dissapeared. It's up to a viewhost to figure what to do in that case
    ASSERT_EQ(nullptr, fm.getFocus());
}

static const char *EDITTEXT = R"({
  "type": "APL",
  "version": "1.6",
  "theme": "dark",
  "mainTemplate": {
    "item": {
      "type": "EditText",
      "height": 100,
      "hint": "Example EditText",
      "hintWeight": "100",
      "hintColor": "grey"
    }
  }
})";

TEST_F(NativeFocusTest, EditTextFocusedOnTap)
{
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureFocusEditTextOnTap);
    loadDocument(EDITTEXT);

    auto& fm = root->context().focusManager();
    ASSERT_EQ(nullptr, fm.getFocus());

    ASSERT_FALSE(root->handlePointerEvent(PointerEvent(apl::kPointerDown, Point(10, 10))));
    ASSERT_TRUE(root->handlePointerEvent(PointerEvent(apl::kPointerUp, Point(10, 10))));

    ASSERT_TRUE(verifyFocusSwitchEvent(component, root->popEvent()));
    ASSERT_EQ(component, fm.getFocus());
}

static const char *EDIT_TEXT_IN_TAP_TOUCHABLE = R"apl({
  "type": "APL",
  "version": "1.6",
  "theme": "dark",
  "mainTemplate": {
    "items": [
      {
        "type": "Sequence",
        "width": "100%",
        "height": "100%",
        "alignItems": "center",
        "justifyContent": "spaceAround",
        "data": [{"color": "blue", "text": "Magic"}],
        "items": [
          {
            "type": "Frame",
            "backgroundColor": "white",
            "items": [
              {
                "type": "TouchWrapper",
                "width": 500,
                "item": {
                  "type": "Frame",
                  "backgroundColor": "${data.color}",
                  "height": 200,
                  "items": {
                    "type": "EditText",
                    "id": "targetEdit",
                    "text": "${data.text}",
                    "width": 500,
                    "height": 100,
                    "fontSize": 60
                  }
                },
                "onDown": {
                  "type": "SendEvent",
                  "arguments": "onDown",
                  "sequencer": "MAIN"
                },
                "onUp": {
                  "type": "SendEvent",
                  "arguments": "onUp",
                  "sequencer": "MAIN"
                }
              }
            ]
          }
        ]
      }
    ]
  }
})apl";

TEST_F(NativeFocusTest, WrappedEditTextTap)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureFocusEditTextOnTap);
    loadDocument(EDIT_TEXT_IN_TAP_TOUCHABLE);

    auto& fm = root->context().focusManager();
    ASSERT_EQ(nullptr, fm.getFocus());

    ASSERT_TRUE(HandlePointerEvent(root, PointerEventType::kPointerDown, Point(400,50), false, "onDown"));
    advanceTime(20);

    ASSERT_TRUE(root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,50))));

    auto editText = root->findComponentById("targetEdit");
    ASSERT_TRUE(editText);
    ASSERT_EQ(editText, fm.getFocus());
    verifyFocusSwitchEvent(editText, root->popEvent());

    ASSERT_TRUE(CheckSendEvent(root, "onUp"));
}

static const char *EDIT_TEXT_IN_UP_TOUCHABLE = R"apl({
  "type": "APL",
  "version": "1.6",
  "theme": "dark",
  "mainTemplate": {
    "items": [
      {
        "type": "Sequence",
        "width": "100%",
        "height": "100%",
        "alignItems": "center",
        "justifyContent": "spaceAround",
        "data": [{"color": "blue", "text": "Magic"}],
        "items": [
          {
            "type": "Frame",
            "backgroundColor": "white",
            "items": [
              {
                "type": "TouchWrapper",
                "width": 500,
                "item": {
                  "type": "Frame",
                  "backgroundColor": "${data.color}",
                  "height": 200,
                  "items": {
                    "type": "EditText",
                    "id": "targetEdit",
                    "text": "${data.text}",
                    "width": 500,
                    "height": 100,
                    "fontSize": 60
                  }
                },
                "onUp": {
                  "type": "SendEvent",
                  "arguments": "onUp",
                  "sequencer": "MAIN"
                }
              }
            ]
          }
        ]
      }
    ]
  }
})apl";

TEST_F(NativeFocusTest, WrappedEditTextUp)
{
    config->enableExperimentalFeature(apl::RootConfig::kExperimentalFeatureFocusEditTextOnTap);
    loadDocument(EDIT_TEXT_IN_UP_TOUCHABLE);

    auto& fm = root->context().focusManager();
    ASSERT_EQ(nullptr, fm.getFocus());

    ASSERT_FALSE(root->handlePointerEvent(PointerEvent(PointerEventType::kPointerDown, Point(400,50))));
    advanceTime(20);

    ASSERT_TRUE(root->handlePointerEvent(PointerEvent(PointerEventType::kPointerUp, Point(400,50))));

    auto editText = root->findComponentById("targetEdit");
    ASSERT_TRUE(editText);
    ASSERT_EQ(editText, fm.getFocus());
    verifyFocusSwitchEvent(editText, root->popEvent());

    ASSERT_TRUE(CheckSendEvent(root, "onUp"));
}

static const char *GRID_SEQUENCE = R"({
  "type": "APL",
  "version": "1.5",
  "styles": {
    "textStylePressable": {
      "values": [
        {"color": "white"},
        {"when": "${state.focused}", "color": "red"}
      ]
    }
  },
  "onMount": {
    "type": "SetFocus",
    "componentId": 0
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": 300,
      "height": 300,
      "items": [
        {
          "type": "GridSequence",
          "width": "100%",
          "height": "100%",
          "id": "sequence",
          "childWidths": [ "auto", "auto", "auto"],
          "childHeight": 50,
          "data": [0,1,2,3,4,5],
          "items": [
            {
              "type": "TouchWrapper",
              "id": "${data}",
              "item": {
                "type": "Text",
                "style": "textStylePressable",
                "inheritParentState": true,
                "fontSize": 24,
                "text": "${data}"
              }
            }
          ]
        }
      ]
    }
  }
})";

TEST_F(NativeFocusTest, GridMoves)
{
    loadDocument(GRID_SEQUENCE);
    auto& fm = root->context().focusManager();
    auto child = root->findComponentById("0");

    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    child = root->findComponentById("3");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    auto event = root->popEvent();
    event.getActionRef().resolve(false);
    root->clearPending();
    ASSERT_EQ(child, fm.getFocus());

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    child = root->findComponentById("4");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_UP_KEY());
    child = root->findComponentById("1");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_RIGHT_KEY());
    child = root->findComponentById("2");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    child = root->findComponentById("5");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char *SCROLLABLE_IN_PAGER = R"({
  "type": "APL",
  "version": "1.6",
  "theme": "dark",
  "layouts": {
    "Textbox": {
      "parameters": [ "definedText" ],
      "item": {
        "type": "Frame",
        "inheritParentState": true,
        "style": "focusablePressableButton",
        "width": "100%",
        "height": "100%",
        "item": {
          "type": "Text",
          "width": "100%",
          "height": "100%",
          "text": "${definedText}",
          "color": "black"
        }
      }
    },
    "Box": {
      "parameters": [ "label" ],
      "item": {
        "type": "Container",
        "width": "10vw",
        "height": "10vw",
        "item": {
          "type": "Textbox",
          "definedText": "T ${label}"
        }
      }
    },
    "Button": {
      "parameters": [ "label" ],
      "item": {
        "type": "TouchWrapper",
        "id": "${label}",
        "width": "10vw",
        "height": "10vw",
        "item": {
          "type": "Textbox",
          "definedText": "B ${label}"
        }
      }
    }
  },
  "styles": {
    "focusablePressableButton": {
      "extend": "textStyleBody",
      "values": [
        {
          "backgroundColor": "#D6DBDF",
          "borderColor": "#566573",
          "borderWidth": "2dp"
        },
        {
          "when": "${state.focused}",
          "borderColor": "#C0392B",
          "backgroundColor": "yellow"
        },
        {
          "when": "${state.pressed}",
          "backgroundColor": "#808B96"
        }
      ]
    }
  },
  "onMount": {
    "type": "SetFocus",
    "componentId": "13"
  },
  "mainTemplate": {
    "items": {
      "type": "Pager",
      "id": "pager",
      "height": "100%",
      "width": "100%",
      "navigation": "wrap",
      "items": [
        {
          "type": "ScrollView",
          "height": "100%",
          "width": "100%",
          "items": [
            {
              "type": "Container",
              "height": "auto",
              "width": "auto",
              "direction": "row",
              "data": [ "1.1", "1.2", "1.3" ],
              "items": [ { "type": "Button", "label": "${data}" } ]
            }
          ]
        },
        {
          "type": "Container",
          "height": "100%",
          "width": "100%",
          "item": [ { "type": "Box", "label": "2" } ]
        },
        {
          "type": "Container",
          "height": "100%",
          "width": "100%",
          "item": [ { "type": "Box", "label": "3" } ]
        }
      ]
    }
  }
})";

TEST_F(NativeFocusTest, CapturingScrollable)
{
    loadDocument(SCROLLABLE_IN_PAGER);
    auto& fm = root->context().focusManager();

    auto child = root->findComponentById("13");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    ASSERT_TRUE(root->nextFocus(kFocusDirectionRight));
    advanceTime(1000);
    child = root->findComponentById("pager");
    ASSERT_TRUE(child);
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
    ASSERT_EQ(1, child->pagePosition());
}

static const char *MEDIA_KEYS_TAKE_IN = R"({
   "type":"APL",
   "version":"1.2",
   "mainTemplate":{
      "item":{
         "type":"TouchWrapper",
         "item":{
            "type":"Frame",
            "id":"testFrame",
            "backgroundColor":"red",
            "width":"300dp",
            "height":"300dp"
         },
         "handleKeyDown":[
            {
               "when":"${event.keyboard.code == 'ArrowUp' || event.keyboard.code == 'KeyW' || event.keyboard.code == 'MediaRewind' }",
               "commands":[
                  {
                     "type":"SetValue",
                     "property":"backgroundColor",
                     "value":"green",
                     "componentId":"testFrame"
                  }
               ]
            },
            {
               "when":"${event.keyboard.code == 'ArrowDown' || event.keyboard.code == 'KeyS' || event.keyboard.code == 'MediaFastForward' }",
               "commands":[
                  {
                     "type":"SetValue",
                     "property":"backgroundColor",
                     "value":"blue",
                     "componentId":"testFrame"
                  }
               ]
            },
            {
               "when":"${event.keyboard.code == 'Enter' || event.keyboard.code == 'MediaPlayPause' || event.keyboard.code == 'KeyG' }",
               "commands":[
                  {
                     "type":"SetValue",
                     "property":"backgroundColor",
                     "value":"yellow",
                     "componentId":"testFrame"
                  }
               ]
            }
         ],
         "handleKeyUp":[
            {
               "when":"${event.keyboard.code == 'Home' || event.keyboard.code == 'KeyK' || event.keyboard.code == 'VolumeUp' }",
               "commands":[
                  {
                     "type":"SetValue",
                     "property":"backgroundColor",
                     "value":"pink",
                     "componentId":"testFrame"
                  }
               ]
            },
            {
               "when":"${event.keyboard.code == 'Back' || event.keyboard.code == 'KeyL' || event.keyboard.code == 'VolumeDown' }",
               "commands":[
                  {
                     "type":"SetValue",
                     "property":"backgroundColor",
                     "value":"white",
                     "componentId":"testFrame"
                  }
               ]
            }
         ]
      }
   }
})";

TEST_F(NativeFocusTest, MediaKeysTakeInNext)
{
    loadDocument(MEDIA_KEYS_TAKE_IN);
    auto& fm = root->context().focusManager();

    root->nextFocus(kFocusDirectionForward);
    root->clearPending();

    ASSERT_EQ(component, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(component, root->popEvent()));
}

TEST_F(NativeFocusTest, MediaKeysTakeInRight)
{
    loadDocument(MEDIA_KEYS_TAKE_IN);
    auto& fm = root->context().focusManager();

    root->nextFocus(kFocusDirectionRight);
    root->clearPending();

    ASSERT_EQ(component, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(component, root->popEvent()));
}

static const char *TW_IN_TW = R"({
  "type": "APL",
  "version": "1.6",
  "theme": "dark",
  "resources": [
    {
      "colors": {
        "colorItemBase": "#D6DBDF",
        "colorItemPressed": "#808B96",
        "colorItemBorderNormal": "#566573",
        "colorItemBorderFocused": "#C0392B"
      }
    }
  ],
  "styles": {
    "textStyleBody": {
      "textAlign": "center",
      "textAlignVertical": "center",
      "color": "black"
    },
    "focusablePressableButton": {
      "extend": "textStyleBody",
      "values": [
        {
          "backgroundColor": "@colorItemBase",
          "borderColor": "@colorItemBorderNormal",
          "borderWidth": "2dp"
        },
        {
          "when": "${state.focused}",
          "borderColor": "@colorItemBorderFocused"
        },
        {
          "when": "${state.pressed}",
          "backgroundColor": "@colorItemPressed"
        }
      ]
    },
    "focusablePressableRow": {
      "extend": "textStyleBody",
      "values": [
        {
          "borderColor": "@colorItemBorderNormal",
          "borderWidth": "2dp"
        },
        {
          "when": "${state.focused}",
          "borderColor": "@colorItemBorderFocused"
        },
        {
          "when": "${state.pressed}",
          "backgroundColor": "@colorItemPressed"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Sequence",
      "height": "100%",
      "width": "100%",
      "data": [1,2,3,4,5],
      "items": {
        "type": "TouchWrapper",
        "id": "row${data}",
        "width": "100%",
        "height": 50,
        "nextFocusRight": "button${data}",
        "item": {
          "type": "Frame",
          "inheritParentState": true,
          "style": "focusablePressableRow",
          "width": "100%",
          "height": "100%",
          "item": {
            "type": "Container",
            "width": "100%",
            "height": "100%",
            "items": [
              {
                "type": "Text",
                "text": "Text${data}",
                "width": "100%",
                "height": "100%",
                "position": "absolute"
              },
              {
                "type": "TouchWrapper",
                "id": "button${data}",
                "width": 150,
                "height": "100%",
                "right": 5,
                "position": "absolute",
                "onPress": {
                  "type": "SetValue",
                  "property": "disabled",
                  "value": "true"
                },
                "item": {
                  "type": "Frame",
                  "style": "focusablePressableButton",
                  "inheritParentState": true,
                  "height": "100%",
                  "width": "100%",
                  "item": {
                    "type": "Text",
                    "text": "Text${data}",
                    "height": "100%",
                    "width": "100%"
                  }
                }
              }
            ]
          }
        }
      }
    }
  }
})";

TEST_F(NativeFocusTest, JumpBetweenTheRows)
{
    loadDocument(TW_IN_TW);
    advanceTime(10);
    auto& fm = root->context().focusManager();

    auto focusableAreas = root->getFocusableAreas();
    auto toFocus = focusableAreas.begin();
    ASSERT_TRUE(root->setFocus(kFocusDirectionForward, toFocus->second, toFocus->first));

    auto child = component->findComponentById("row1");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->nextFocus(kFocusDirectionDown);
    root->clearPending();

    child = component->findComponentById("row2");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->nextFocus(kFocusDirectionRight);
    root->clearPending();

    child = component->findComponentById("button2");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->nextFocus(kFocusDirectionDown);
    root->clearPending();

    child = component->findComponentById("row3");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->nextFocus(kFocusDirectionRight);
    root->clearPending();

    child = component->findComponentById("button3");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ENTER_KEY());
    root->handleKeyboard(kKeyUp, Keyboard::ENTER_KEY());
    root->clearPending();

    child = component->findComponentById("row4");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}


static const char *PAGE_WEB_VH = R"({
  "mainTemplate": {
    "items": [
      {
        "type": "Pager",
        "id": "1000",
        "items": [
          {
            "type": "TouchWrapper",
            "id": "1001",
            "inheritParentState": true,
            "items": [
              {
                "type": "Text",
                "text":"page1"
              }
            ]
          },
          {
            "type": "TouchWrapper",
            "id": "1002",
            "inheritParentState": true,
            "items": [
              {
                "type": "Text",
                "text":"page2"
              }
            ]
          }
        ]
      }
    ]
  },
  "theme": "dark",
  "type": "APL",
  "version": "1.6"
})";

TEST_F(NativeFocusTest, JustATest) {
    loadDocument(PAGE_WEB_VH);
    advanceTime(10);

    auto& fm = root->context().focusManager();
    // Simulate the following message
    // "type":"setFocus","payload":{"direction":1,"origin":{"top":0,"left":0,"width":100,"height":100},"targetId":"1000"}
    auto direction = 1;
    auto top = 0;
    auto left = 0;
    auto width = 100;
    auto height = 100;
    auto targetId = "1000";
    auto origin = apl::Rect(top, left, width, height);
    ASSERT_TRUE(root->setFocus(static_cast<apl::FocusDirection>(direction), origin, targetId));
    auto child = component->findComponentById("1000");
    ASSERT_EQ(child, fm.getFocus());
    ASSERT_TRUE(verifyFocusSwitchEvent(child, root->popEvent()));
}

static const char* PAGERED_SCROLL_VIEW = R"({
  "type": "APL",
  "version": "1.8",
  "theme": "dark",
  "mainTemplate": {
    "items": [
      {
        "type": "Pager",
        "id": "root",
        "height": 200,
        "width": 200,
        "navigation": "none",
        "items": [
          {
            "type": "ScrollView",
            "id": "scroller",
            "width": "100%",
            "height": "100%",
            "onMount": {
              "type": "SetFocus"
            },
            "item": {
              "type": "Container",
              "height": 600,
              "width": "100%",
              "items": [
                {
                  "type": "TouchWrapper",
                  "id": "redTW",
                  "width": "100%",
                  "height": 100,
                  "item": {
                    "type": "Frame",
                    "height": "100%",
                    "width": "100%",
                    "borderColor": "red",
                    "borderWidth": 2
                  }
                },
                {
                  "type": "Text",
                  "width": "100%",
                  "height": 100,
                  "text": "Am I text?"
                },
                {
                  "type": "TouchWrapper",
                  "id": "greenTW",
                  "width": "100%",
                  "height": 100,
                  "item": {
                    "type": "Frame",
                    "height": "100%",
                    "width": "100%",
                    "borderColor": "green",
                    "borderWidth": 2
                  }
                },
                {
                  "type": "Text",
                  "position": "absolute",
                  "bottom": 0,
                  "height": 100,
                  "width": "100%",
                  "text": "I am text No, really."
                }
              ]
            }
          }
        ]
      }
    ]
  }
})";

TEST_F(NativeFocusTest, PageredScrollView)
{
    loadDocument(PAGERED_SCROLL_VIEW);
    auto& fm = root->context().focusManager();

    ASSERT_TRUE(component);
    auto scroller = root->findComponentById("scroller");
    ASSERT_TRUE(scroller);
    auto redTW = root->findComponentById("redTW");
    ASSERT_TRUE(redTW);
    auto greenTW = root->findComponentById("greenTW");
    ASSERT_TRUE(greenTW);

    root->clearPending();

    ASSERT_EQ(scroller->getId(), fm.getFocus()->getId());
    ASSERT_TRUE(verifyFocusSwitchEvent(scroller, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    root->clearPending();
    ASSERT_EQ(0, scroller->scrollPosition().getY());

    ASSERT_EQ(redTW->getId(), fm.getFocus()->getId());
    ASSERT_TRUE(verifyFocusSwitchEvent(redTW, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    ASSERT_EQ(100, scroller->scrollPosition().getY());

    ASSERT_EQ(greenTW->getId(), fm.getFocus()->getId());
    ASSERT_TRUE(verifyFocusSwitchEvent(greenTW, root->popEvent()));

    root->handleKeyboard(kKeyDown, Keyboard::ARROW_DOWN_KEY());
    advanceTime(1000);
    //ASSERT_EQ(300, scroller->scrollPosition().getY());

    ASSERT_EQ(scroller->getId(), fm.getFocus()->getId());
    ASSERT_TRUE(verifyFocusSwitchEvent(scroller, root->popEvent()));
}