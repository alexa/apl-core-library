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
#include "apl/component/selector.h"

using namespace apl;

class SelectorTest : public DocumentWrapper {};

static const char *BASIC = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "TOP",
      "items": {
        "type": "Text",
        "id": "TEXT_${index}",
        "text": "Item ${index}"
      },
      "data": "${Array.range(10)}"
    }
  }
}
)apl";

TEST_F(SelectorTest, Basic)
{
    loadDocument(BASIC);
    ASSERT_TRUE(component);
    auto child3 = component->getCoreChildAt(3);
    auto child6 = component->getCoreChildAt(6);

    // ":root" -> always returns root
    ASSERT_EQ(component, Selector::resolve(":root", context, component)); // Start from the root
    ASSERT_EQ(component, Selector::resolve(":root", context, child3));    // Start from a child

    // ":source" -> always return the element you start with
    ASSERT_EQ(component, Selector::resolve(":source", context, component));
    ASSERT_EQ(child3, Selector::resolve(":source", context, child3));

    // "TEXT_3" -> findComponentById starting at the current point
    ASSERT_EQ(child3, Selector::resolve("TEXT_3",context,component));
    ASSERT_EQ(child3, Selector::resolve("TEXT_3",context,child3));
    ASSERT_EQ(child3, Selector::resolve("TEXT_3",context,child6));

    // Use the unique ID of one of the components
    ASSERT_EQ(child6, Selector::resolve(child6->getUniqueId(), context,component));
    ASSERT_EQ(child6, Selector::resolve(child6->getUniqueId(), context,child3));
    ASSERT_EQ(child6, Selector::resolve(child6->getUniqueId(), context,child6));

    // Empty selector should not return anything
    ASSERT_FALSE(Selector::resolve("", context, component));
    ASSERT_FALSE(Selector::resolve("     ", context, component));

    // White-space only parse fail.
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(SelectorTest, BasicWithWhitespace)
{
    loadDocument(BASIC);
    ASSERT_TRUE(component);
    auto child3 = component->getCoreChildAt(3);
    auto child6 = component->getCoreChildAt(6);

    // ":root" -> always returns root
    std::string s = "  :root  ";
    ASSERT_EQ(component, Selector::resolve(s, context, component)); // Start from the root
    ASSERT_EQ(component, Selector::resolve(s, context, child3));    // Start from a child

    // ":source" -> always return the element you start with
    s = "   :source    ";
    ASSERT_EQ(component, Selector::resolve(s, context, component));
    ASSERT_EQ(child3, Selector::resolve(s, context, child3));

    // "TEXT_3" -> findComponentById starting at the current point
    s = "  TEXT_3  ";
    ASSERT_EQ(child3, Selector::resolve(s, context, component));
    ASSERT_EQ(child3, Selector::resolve(s, context, child3));
    ASSERT_EQ(child3, Selector::resolve(s, context, child6));

    // Use the unique ID of one of the components
    s = "   " + child6->getUniqueId() + "  ";
    ASSERT_EQ(child6, Selector::resolve(s, context, component));
    ASSERT_EQ(child6, Selector::resolve(s, context, child3));
    ASSERT_EQ(child6, Selector::resolve(s, context, child6));
}

TEST_F(SelectorTest, BasicChildByIndex)
{
    loadDocument(BASIC);
    ASSERT_TRUE(component);
    auto child3 = component->getCoreChildAt(3);
    auto child6 = component->getCoreChildAt(6);

    ASSERT_EQ(child3, Selector::resolve(":root:child(3)", context, component));
    ASSERT_EQ(child3, Selector::resolve(":root:child(3)", context, child3));
    ASSERT_EQ(child3, Selector::resolve(":root:child(3)", context, child6));

    ASSERT_EQ(child6, Selector::resolve(":root:child(-4)", context, component));
    ASSERT_EQ(child6, Selector::resolve(":root:child(-4)", context, child3));
    ASSERT_EQ(child6, Selector::resolve(":root:child(-4)", context, child6));

    ASSERT_EQ(child3, Selector::resolve(":child(3)", context, component));
    ASSERT_EQ(child3, Selector::resolve(":child(-7)", context, component));
    ASSERT_EQ(child6, Selector::resolve(":child(6)", context, component));
    ASSERT_EQ(child6, Selector::resolve(":child(-4)", context, component));

    ASSERT_EQ(nullptr, Selector::resolve(":child(20)", context, component));
    ASSERT_EQ(nullptr, Selector::resolve(":child(-20)", context, component));
}

TEST_F(SelectorTest, BasicChildById)
{
    loadDocument(BASIC);
    ASSERT_TRUE(component);
    auto child3 = component->getCoreChildAt(3);
    auto child6 = component->getCoreChildAt(6);

    ASSERT_EQ(child3, Selector::resolve(":root:child(id=TEXT_3)", context, component));
}

TEST_F(SelectorTest, BasicChildByRelative)
{
    loadDocument(BASIC);
    ASSERT_TRUE(component);
    auto child3 = component->getCoreChildAt(3);
    auto child4 = component->getCoreChildAt(4);
    auto child6 = component->getCoreChildAt(6);

    ASSERT_EQ(child4, Selector::resolve(":next()", context, child3));
    ASSERT_EQ(child3, Selector::resolve(":previous()", context, child4));
    ASSERT_EQ(child6, Selector::resolve(":next(2)", context, child4));
    ASSERT_EQ(child3, Selector::resolve(":previous(3)", context, child6));
}

TEST_F(SelectorTest, Missing)
{
    loadDocument(BASIC);
    ASSERT_TRUE(component);

    ASSERT_FALSE(Selector::resolve(":child(id=TEXT_99)", context, component));
    ASSERT_FALSE(Selector::resolve(":find(id=TEXT_99)", context, component));
    ASSERT_FALSE(Selector::resolve(":next()", context, component));
    ASSERT_FALSE(Selector::resolve(":previous()", context, component));
    ASSERT_FALSE(Selector::resolve(":parent()", context, component));
}

static std::vector<std::string> BAD_CASES = {
    ":",
    ":roo",
    "fo:oo",
    ":previous(color=blue)",
    ":parent(typ=Container",
    ":parent(type=Container",
};

TEST_F(SelectorTest, BadParser)
{
    loadDocument(BASIC);
    ASSERT_TRUE(component);

    for (const auto& m : {":", ":roo", "fo:oo", ":previous(color=blue)", ":parent(typ=Container"}) {
        auto s = Selector::resolve(m, context);
        ASSERT_FALSE(s);
        ASSERT_TRUE(ConsoleMessage());
    }
}


static const char *ALTERNATE_TEXT_IMAGE = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "TOP",
      "items": [
        {
          "when": "${index%2}",
          "type": "Text",
          "id": "TEXT_${index}",
          "text": "Item ${index}"
        },
        {
          "type": "Image",
          "id": "IMAGE_${index}",
          "source": "foo"
        }
      ],
      "data": "${Array.range(10)}"
    }
  }
}
)apl";

TEST_F(SelectorTest, ChildByType)
{
    loadDocument(ALTERNATE_TEXT_IMAGE);
    ASSERT_TRUE(component);

    ASSERT_EQ(component->getCoreChildAt(0),
              Selector::resolve(":root:child(type=Image)", context, component));
    ASSERT_EQ(component->getCoreChildAt(1),
              Selector::resolve(":root:child(type=Text)", context, component));
}

static const char *DEEP = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "TOP",
      "items": {
        "type": "Container",
        "id": "BOX_${index}",
        "bind": { "name": "X", "value": "${index}" },
        "item": [
          {
            "when": "${index%3 == 0}",
            "type": "Text",
            "id": "TEXT_${index}",
            "text": "Item ${X},${index}"
          },
          {
            "when": "${index%3 == 1}",
            "type": "Image",
            "id": "IMAGE_${index}",
            "source": "${X}/${index}"
          },
          {
            "type": "Frame",
            "id": "FRAME_${index}",
            "bind": { "name": "Y", "value": "${index}" },
            "item": {
              "type": "Video",
              "id": "VIDEO",
              "source": "${X}/${Y}"
            }
          }
        ],
        "data": "${Array.range(6)}"
      },
      "data": "${Array.range(10)}"
    }
  }
}
)apl";

TEST_F(SelectorTest, Deep)
{
    loadDocument(DEEP);
    ASSERT_TRUE(component);

    auto t2_3 = component->getCoreChildAt(2)->getCoreChildAt(3);
    ASSERT_EQ(t2_3, Selector::resolve(":root:child(2):child(3)", context, component));

    // Use the parent relative reference and search for the first video
    auto t2_2_1 = component->getCoreChildAt(2)->getCoreChildAt(2)->getCoreChildAt(0);
    ASSERT_EQ(t2_2_1, Selector::resolve(":source:parent():find(type=Video)", context, t2_3));

    // The ":source" is optional
    ASSERT_EQ(t2_2_1, Selector::resolve(":parent():find(type=Video)", context, t2_3));

    // If you use the child method, you will fail because it is deeply buried
    ASSERT_FALSE(Selector::resolve(":source:parent():child(type=Video)", context, t2_3));

    // Move around based on type
    ASSERT_EQ("IMAGE_4", Selector::resolve(":next(type=Image)", context, t2_3)->getId());

    // Grandparent
    ASSERT_EQ(component, Selector::resolve(":parent(2)", context, t2_3));
}

TEST_F(SelectorTest, Parent)
{
    loadDocument(DEEP);
    ASSERT_TRUE(component);

    auto container = component->getCoreChildAt(2);
    auto frame = container->getCoreChildAt(2);
    auto video = frame->getCoreChildAt(0);

    ASSERT_EQ(container, Selector::resolve(":parent()", context, frame));
    ASSERT_EQ(container, Selector::resolve(":parent(1)", context, frame));
    ASSERT_EQ(component, Selector::resolve(":parent(2)", context, frame));
    ASSERT_EQ(nullptr, Selector::resolve(":parent(3)", context, frame));
    ASSERT_EQ(nullptr, Selector::resolve(":parent(212)", context, frame));

    ASSERT_EQ(frame, Selector::resolve(":parent(1)", context, video));
    ASSERT_EQ(container, Selector::resolve(":parent(2)", context, video));
    ASSERT_EQ(component, Selector::resolve(":parent(3)", context, video));
    ASSERT_EQ(nullptr, Selector::resolve(":parent(4)", context, video));
}


static const char * LAYOUTS = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "layouts": {
    "Label": {
      "parameters": [
        "LABEL",
        "COLOR"
      ],
      "item": {
        "type": "Text",
        "text": "${LABEL}",
        "color": "${COLOR}"
      }
    },
    "BlueLabel": {
      "item": {
        "type": "Label",
        "COLOR": "blue"
      }
    },
    "RedLabel": {
      "item": {
        "type": "Label",
        "COLOR": "red"
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Container",
      "items": [
        {
          "type": "BlueLabel",
          "LABEL": "This is blue"
        },
        {
          "type": "RedLabel",
          "LABEL": "This is red"
        }
      ]
    }
  }
}
)apl";

TEST_F(SelectorTest, Layouts)
{
    loadDocument(LAYOUTS);
    ASSERT_TRUE(component);

    auto blueText = component->getCoreChildAt(0);
    auto redText = component->getCoreChildAt(1);

    ASSERT_EQ(blueText, Selector::resolve(":root:find(type=Text)", context));
    ASSERT_EQ(blueText, Selector::resolve(":root:find(type=Label)", context));
    ASSERT_EQ(blueText, Selector::resolve(":root:find(type=BlueLabel)", context));
    ASSERT_EQ(redText, Selector::resolve(":root:find(type=RedLabel)", context));

    ASSERT_FALSE(Selector::resolve(":root:find(type=Video)", context));
    ASSERT_FALSE(Selector::resolve(":find(type=Label)", context, blueText));
}

TEST_F(SelectorTest, Spacing)
{
    loadDocument(LAYOUTS);
    ASSERT_TRUE(component);

    auto blueText = component->getCoreChildAt(0);
    auto redText = component->getCoreChildAt(1);

    ASSERT_EQ(blueText, Selector::resolve(" :root :find(type=Text) ", context));
    ASSERT_EQ(blueText, Selector::resolve(" :root :find(type=Label) ", context));
    ASSERT_EQ(blueText, Selector::resolve(" :root :find(type=BlueLabel) ", context));
}


::testing::AssertionResult
CheckSendEventComponents(const RootContextPtr& root,
                         const std::string& label,
                         const std::map<std::string, Object>& map)
{
    if (!root->hasEvent())
        return ::testing::AssertionFailure() << "Has no events.";

    auto event = root->popEvent();
    if (event.getType() != kEventTypeSendEvent)
        return ::testing::AssertionFailure() << "Event has wrong type:"
                                             << " Expected: SendEvent"
                                             << ", actual: " << sEventTypeBimap.at(event.getType());

    auto arguments = event.getValue(kEventPropertyArguments);
    if (arguments.size() != 1)
        return ::testing::AssertionFailure() << "Expected one argument";

    auto actual_label =  event.getValue(apl::kEventPropertyArguments).at(0);
    if (!IsEqual(label, actual_label))
        return ::testing::AssertionFailure() << "Mismatched label. Expected=" << label
                                             << " actual=" << actual_label;

    auto actual_components = event.getValue(apl::kEventPropertyComponents);
    if (actual_components.size() != map.size())
        return ::testing::AssertionFailure() << "Component map size mismatch.  Expected size=" <<
               map.size() << " actual size=" << actual_components.size();

    for (const auto& iter : map) {
        if (!actual_components.has(iter.first))
            return ::testing::AssertionFailure()
                   << "Did not find key " << iter.first << " in components map";

        auto value = actual_components.get(iter.first);
        if (!IsEqual(value, iter.second))
            return ::testing::AssertionFailure() << "Component mismatch for key=" << iter.first
                                                 << " expected=" << iter.second
                                                 << " actual=" << value;
    }

    return ::testing::AssertionSuccess();
}

static const char * SEND_EVENT = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "item": {
        "type": "TouchWrapper",
        "id": "TOUCH_${index}",
        "onPress": {
          "type": "SendEvent",
          "arguments": [
            "INDEX ${index}"
          ],
          "components": "${data}"
        }
      },
      "data": [
        ":source",
        ":root",
        "TOUCH_0",
        ":previous(1)",
        ":next(1)",
        [
          ":source",
          ":previous(1)",
          "TOUCH_2:previous(2)"
        ]
      ]
    }
  }
}
)apl";

TEST_F(SelectorTest, SendEvent) {
    loadDocument(SEND_EVENT);
    ASSERT_TRUE(component);

    std::vector<CoreComponentPtr> touch;
    for (auto i = 0; i < component->getChildCount(); i++)
        touch.emplace_back(component->getCoreChildAt(i));

    // Press the first button
    touch[0]->update(apl::kUpdatePressed, 0);
    ASSERT_TRUE(CheckSendEventComponents(root, "INDEX 0", {{":source", false}}));

    touch[0]->setState(apl::kStateChecked, true);
    touch[0]->update(kUpdatePressed, 0);
    ASSERT_TRUE(CheckSendEventComponents(root, "INDEX 0", {{":source", true}}));

    // The second button refers to null
    touch[1]->update(apl::kUpdatePressed, 0);
    ASSERT_TRUE(CheckSendEventComponents(root, "INDEX 1", {{":root", Object::NULL_OBJECT()}}));

    // The third button refers to the first button
    touch[2]->update(apl::kUpdatePressed, 0);
    ASSERT_TRUE(CheckSendEventComponents(root, "INDEX 2", {{"TOUCH_0", true}}));

    // The fourth button refers to the previous button
    touch[3]->update(apl::kUpdatePressed, 0);
    ASSERT_TRUE(CheckSendEventComponents(root, "INDEX 3", {{":previous(1)", false}}));
    touch[2]->setState(kStateChecked, true);
    touch[3]->update(apl::kUpdatePressed, 0);
    ASSERT_TRUE(CheckSendEventComponents(root, "INDEX 3", {{":previous(1)", true}}));

    // The fifth button refers to the next button
    touch[4]->update(apl::kUpdatePressed, 0);
    ASSERT_TRUE(CheckSendEventComponents(root, "INDEX 4", {{":next(1)", false}}));
    touch[5]->setState(kStateChecked, true);
    touch[4]->update(apl::kUpdatePressed, 0);
    ASSERT_TRUE(CheckSendEventComponents(root, "INDEX 4", {{":next(1)", true}}));

    // The sixth button lists out three buttons (including itself)
    touch[5]->update(apl::kUpdatePressed, 0);
    ASSERT_TRUE(CheckSendEventComponents(
        root, "INDEX 5",
        {{":source", true}, {":previous(1)", false}, {"TOUCH_2:previous(2)", true}}));
    touch[5]->setState(kStateChecked, false);
    touch[4]->setState(kStateChecked, true);
    touch[0]->setState(kStateChecked, false);
    touch[5]->update(apl::kUpdatePressed, 0);
    ASSERT_TRUE(CheckSendEventComponents(
        root, "INDEX 5",
        {{":source", false}, {":previous(1)", true}, {"TOUCH_2:previous(2)", false}}));
}