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

#include "apl/component/touchwrappercomponent.h"
#include "apl/engine/typeddependant.h"
#include "apl/datagrammar/bytecode.h"

using namespace apl;

class DependantTest : public DocumentWrapper {};

static const char *CONTEXT_TEST = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "bind": [
        {
          "name": "a",
          "value": 22
        }
      ],
      "items": {
        "type": "Frame",
        "bind": [
          {
            "name": "b",
            "value": "${a}"
          }
        ]
      }
    }
  }
})";

TEST_F(DependantTest, Context)
{
    loadDocument(CONTEXT_TEST);

    ASSERT_TRUE(component);
    auto frame = component->getChildAt(0);

    ASSERT_TRUE(IsEqual(22, frame->getContext()->opt("b")));

    // Change the parent value
    ASSERT_TRUE(component->getContext()->userUpdateAndRecalculate("a", 23, false));
    ASSERT_TRUE(IsEqual(23, frame->getContext()->opt("b")));

    // Try a different type
    ASSERT_TRUE(component->getContext()->userUpdateAndRecalculate("a", "fuzzy", false));
    ASSERT_TRUE(IsEqual("fuzzy", frame->getContext()->opt("b")));
}

static const char *CONTEXT_TEST_2 = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "bind": [
        {
          "name": "a",
          "value": 22
        },
        {
          "name": "b",
          "value": "red",
          "type": "color"
        },
        {
          "name": "c",
          "value": "${a+10}"
        }
      ],
      "items": {
        "type": "Frame",
        "bind": [
          {
            "name": "x",
            "value": "${Math.min(a, 100)}"
          },
          {
            "name": "y",
            "value": "${b}"
          }
        ],
        "items": {
          "type": "Text",
          "bind": [
            {
              "name": "z",
              "value": "${a*c}"
            }
          ]
        }
      }
    }
  }
})";

TEST_F(DependantTest, Context2)
{
    loadDocument(CONTEXT_TEST_2);

    ASSERT_TRUE(component);
    auto frame = component->getChildAt(0);
    auto text = frame->getChildAt(0);

    ASSERT_TRUE(IsEqual(22, component->getContext()->opt("a")));
    ASSERT_TRUE(IsEqual(Color(Color::RED), component->getContext()->opt("b")));
    ASSERT_TRUE(IsEqual(32, component->getContext()->opt("c")));
    ASSERT_TRUE(IsEqual(22, frame->getContext()->opt("x")));
    ASSERT_TRUE(IsEqual(Color(Color::RED), frame->getContext()->opt("y")));
    ASSERT_TRUE(IsEqual(22 * 32, text->getContext()->opt("z")));

    // Update a few values
    ASSERT_TRUE(component->getContext()->userUpdateAndRecalculate("a", 102, false));
    ASSERT_TRUE(component->getContext()->userUpdateAndRecalculate("b", Color(0xfefefeff), false));

    ASSERT_TRUE(IsEqual(102, component->getContext()->opt("a")));
    ASSERT_TRUE(IsEqual(Color(0xfefefeff), component->getContext()->opt("b")));
    ASSERT_TRUE(IsEqual(112, component->getContext()->opt("c")));
    ASSERT_TRUE(IsEqual(100, frame->getContext()->opt("x")));
    ASSERT_TRUE(IsEqual(Color(0xfefefeff), frame->getContext()->opt("y")));
    ASSERT_TRUE(IsEqual(102 * 112, text->getContext()->opt("z")));

    // Put in something creative
    ASSERT_TRUE(component->getContext()->userUpdateAndRecalculate("a", "fuzzy", false));
    ASSERT_TRUE(IsEqual("fuzzy", component->getContext()->opt("a")));
    ASSERT_TRUE(IsEqual("fuzzy10", component->getContext()->opt("c")));
    ASSERT_TRUE(frame->getContext()->opt("x").isNaN());  // Non-numbers become 0
    ASSERT_TRUE(text->getContext()->opt("z").isNaN());  // Neither does multiplication
}

const static char * COMPONENT_TEST = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Text",
      "bind": [
        {
          "name": "a",
          "value": 22
        }
      ],
      "text": "Is ${a}"
    }
  }
})";

TEST_F(DependantTest, Component)
{
    loadDocument(COMPONENT_TEST);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("Is 22", component->getCalculated(kPropertyText).asString()));

    // Update the context and verify that things change
    ASSERT_TRUE(component->getContext()->userUpdateAndRecalculate("a", "fuzzy", true));
    ASSERT_TRUE(IsEqual("Is fuzzy", component->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(component, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component));

    // Updating the context with the same value should not set dirty flags
    ASSERT_TRUE(component->getContext()->userUpdateAndRecalculate("a", "fuzzy", true));
    ASSERT_TRUE(IsEqual("Is fuzzy", component->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(component));
    ASSERT_TRUE(CheckDirty(root));

    // Now assign a value - this should cancel the assignment.
    component->setProperty(kPropertyText, "hello");
    ASSERT_TRUE(IsEqual("hello", component->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(component, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component));

    // Verify that the assignment is cancelled.
    ASSERT_TRUE(component->getContext()->userUpdateAndRecalculate("a", 10, true));
    ASSERT_TRUE(IsEqual("hello", component->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(component));
    ASSERT_TRUE(CheckDirty(root));
}

static const char *COUNTER_TEST = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "TouchWrapper",
      "bind": [
        {
          "name": "myCount",
          "value": 0,
          "type": "number"
        }
      ],
      "onPress": {
        "type": "SetValue",
        "property": "myCount",
        "value": "${myCount + 1}"
      },
      "item": {
        "type": "Text",
        "text": "Count: ${myCount}"
      }
    }
  }
})";

TEST_F(DependantTest, Counter)
{
    loadDocument(COUNTER_TEST);
    ASSERT_TRUE(component);
    auto text = component->getChildAt(0);

    ASSERT_TRUE(IsEqual("Count: 0", text->getCalculated(kPropertyText).asString()));

    // Fire the press event
    performTap(0, 0);
    ASSERT_TRUE(IsEqual("Count: 1", text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, text));

    // Repeat
    performTap(0, 0);
    ASSERT_TRUE(IsEqual("Count: 2", text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, text));
}

TEST_F(DependantTest, FreeContext)
{
    context = Context::createTestContext(metrics, makeDefaultSession());

    // Parent context
    auto first = Context::createFromParent(context);
    first->putUserWriteable("source", 23);

    // Child context
    auto second = Context::createFromParent(first);
    second->putUserWriteable("target", 10);
    ASSERT_EQ(10, second->opt("target").asNumber());

    // Manually construct a dependency between source and target
    auto result = parseAndEvaluate(*first, "${source * 2}");
    ASSERT_TRUE(IsEqual(result.value, 46));
    ASSERT_TRUE(result.expression.isEvaluable());
    ASSERT_EQ(1, result.symbols.size());
    auto bf = sBindingFunctions.at(BindingType::kBindingTypeNumber);
    ContextDependant::create(second, "target", std::move(result.expression), first, bf,
                             std::move(result.symbols));

    // Test that changing the source now changes the target
    ASSERT_TRUE(first->userUpdateAndRecalculate("source", 10, false));
    ASSERT_EQ(10, first->opt("source").asNumber());
    ASSERT_EQ(20, second->opt("target").asNumber());

    // Verify that there is a single dependant hanging off of the "first" context
    ASSERT_EQ(1, first->countDownstream("source"));
    ASSERT_EQ(1, second->countUpstream("target"));

    // Remove the second context.
    second = nullptr;

    ASSERT_EQ(0, first->countDownstream("source"));
}

static const char *FREE_COMPONENT = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Text",
      "bind": [
        {
          "name": "a",
          "value": 22
        }
      ],
      "text": "Is ${a}"
    }
  }
})";

TEST_F(DependantTest, FreeComponent)
{
    loadDocument(FREE_COMPONENT);
    ASSERT_TRUE(component);
    ASSERT_STREQ("Is 22", component->getCalculated(kPropertyText).asString().c_str());

    // Make sure the binding is active
    ASSERT_TRUE(component->getContext()->userUpdateAndRecalculate("a", 44, false));
    ASSERT_STREQ("Is 44", component->getCalculated(kPropertyText).asString().c_str());

    // Verify that the correct number of bindings are present
    ASSERT_EQ(1, component->getContext()->countDownstream("a"));
    ASSERT_EQ(1, component->countUpstream(kPropertyText));

    // Remove the component binding
    component->setProperty(kPropertyText, "Hello");

    // Verify that the bindings are removed
    ASSERT_EQ(0, component->getContext()->countDownstream("a"));
    ASSERT_EQ(0, component->countUpstream(kPropertyText));

    // Verify that changing "a" no longer changes the text.
    ASSERT_TRUE(component->getContext()->userUpdateAndRecalculate("a", 100, false));
    ASSERT_STREQ("Hello", component->getCalculated(kPropertyText).asString().c_str());
}

static const char *BREAK_CHAIN = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Text",
      "bind": [
        {
          "name": "a",
          "value": 22
        },
        {
          "name": "b",
          "value": "${a*a}"
        }
      ],
      "text": "Is ${b}"
    }
  }
})";

TEST_F(DependantTest, BreakChain)
{
    loadDocument(BREAK_CHAIN);
    ASSERT_TRUE(component);
    ASSERT_STREQ("Is 484", component->getCalculated(kPropertyText).asString().c_str());

    // Make sure the binding is active
    ASSERT_TRUE(component->getContext()->userUpdateAndRecalculate("a", 10, false));
    ASSERT_STREQ("Is 100", component->getCalculated(kPropertyText).asString().c_str());

    // Verify that the correct number of bindings are present
    ASSERT_EQ(1, component->getContext()->countDownstream("a"));
    ASSERT_EQ(1, component->getContext()->countUpstream("b"));

    ASSERT_EQ(1, component->getContext()->countDownstream("b"));
    ASSERT_EQ(1, component->countUpstream(kPropertyText));

    // Break the chain by assigning to 'b' directly
    ASSERT_TRUE(component->getContext()->userUpdateAndRecalculate("b", 12, false));

    // Check that the text was updated
    ASSERT_STREQ("Is 12", component->getCalculated(kPropertyText).asString().c_str());

    // Verify that the bindings have been reset
    ASSERT_EQ(0, component->getContext()->countDownstream("a"));
    ASSERT_EQ(0, component->getContext()->countUpstream("b"));

    ASSERT_EQ(1, component->getContext()->countDownstream("b"));
    ASSERT_EQ(1, component->countUpstream(kPropertyText));
}

static const char *REATTACH = R"(
{
  "type": "APL",
  "version": "2023.1",
  "mainTemplate": {
    "items": {
      "type": "Text",
      "bind": [
        {
          "name": "Rodent",
          "value": true
        },
        {
          "name": "HasTail",
          "value": false
        },
        {
          "name": "Tailful",
          "value": "Rat"
        },
        {
          "name": "Tailless",
          "value": "Hamster"
        },
        {
          "name": "WagsTail",
          "value": false
        },
        {
          "name": "Waggly",
          "value": "Dog"
        },
        {
          "name": "NotWaggly",
          "value": "Cat"
        }
      ],
      "text": "${Rodent ? (HasTail ? Tailful : Tailless) : (WagsTail ? Waggly : NotWaggly)}"
    }
  }
})";

TEST_F(DependantTest, Reattach)
{
    loadDocument(REATTACH);
    ASSERT_TRUE(component);
    auto c = component->getContext();

    ASSERT_TRUE(IsEqual("Hamster", component->getCalculated(apl::kPropertyText).asString()));
    ASSERT_EQ(1, c->countDownstream("Rodent"));
    ASSERT_EQ(1, c->countDownstream("HasTail"));
    ASSERT_EQ(0, c->countDownstream("Tailful"));
    ASSERT_EQ(1, c->countDownstream("Tailless"));
    ASSERT_EQ(0, c->countDownstream("WagsTail"));
    ASSERT_EQ(0, c->countDownstream("Waggly"));
    ASSERT_EQ(0, c->countDownstream("NotWaggly"));

    ASSERT_TRUE(c->userUpdateAndRecalculate("Rodent", false, false));
    ASSERT_EQ(1, c->countDownstream("Rodent"));
    ASSERT_EQ(0, c->countDownstream("HasTail"));
    ASSERT_EQ(0, c->countDownstream("Tailful"));
    ASSERT_EQ(0, c->countDownstream("Tailless"));
    ASSERT_EQ(1, c->countDownstream("WagsTail"));
    ASSERT_EQ(0, c->countDownstream("Waggly"));
    ASSERT_EQ(1, c->countDownstream("NotWaggly"));
}


static const char *STATIC_PROPERTY = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Text",
      "bind": [
        {
          "name": "a",
          "value": 5
        }
      ],
      "letterSpacing": "${a}"
    }
  }
})";

TEST_F(DependantTest, StaticProperty)
{
    loadDocument(STATIC_PROPERTY);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Dimension(5), component->getCalculated(kPropertyLetterSpacing)));

    // letterSpacing is not dynamic.  It can't be changed through propagation
    ASSERT_TRUE(component->getContext()->userUpdateAndRecalculate("a", 10, false));
    ASSERT_FALSE(IsEqual(Dimension(10), component->getCalculated(kPropertyLetterSpacing)));
}

static const char *MUTABLE = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Text",
      "id": "textId",
      "bind": [
        {
          "name": "a",
          "value": "${TestMutable}"
        },
        {
          "name": "b",
          "value": "${TestImmutable}"
        }
      ],
      "text": "${a} ${b} ${viewport.width}"
    }
  }
})";

static const std::string KEY_MUTABLE = "TestMutable";
static const std::string KEY_IMMUTABLE = "TestImmutable";

/**
 * Test adding top-level mutable and immutable values in the context.  We add bindings
 * between the context values and the text in a text box.  The mutable upstream value
 * creates a Node dependency; the immutable upstream value does not.
 */
TEST_F(DependantTest, Mutable)
{
    createCallback = [](const RootContextPtr& root) {
        auto& context = root->context();
        context.putUserWriteable(KEY_MUTABLE, "Hello");
        context.putConstant(KEY_IMMUTABLE, "Goodbye");
    };

    metrics.size(200,200).dpi(160);

    loadDocument(MUTABLE);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("Hello Goodbye 200", component->getCalculated(kPropertyText).asString()));

    // Both "a" and "b" can be modified, because all bound properties can respond to SetValue
    // They generate a single upstream driver
    ASSERT_EQ(1, component->countUpstream());
    ASSERT_EQ(1, component->countUpstream(kPropertyText));

    // Downstream from component context:   a->Text, b->Text
    ASSERT_EQ(2, component->getContext()->countDownstream());
    ASSERT_EQ(1, component->getContext()->countDownstream("a"));
    ASSERT_EQ(1, component->getContext()->countDownstream("b"));

    // Upstream from component context: TestMutable->a
    ASSERT_EQ(1, component->getContext()->countUpstream());
    ASSERT_EQ(1, component->getContext()->countUpstream("a"));
    ASSERT_EQ(0, component->getContext()->countUpstream("b"));

    // Downstream from root context: TestMutable->a
    ASSERT_EQ(1, context->countDownstream());
    ASSERT_EQ(1, context->countDownstream(KEY_MUTABLE));

    // Now change the mutable element AND the immutable one - only the mutable will propagate.
    ASSERT_FALSE(ConsoleMessage());
    ASSERT_TRUE(context->userUpdateAndRecalculate(KEY_MUTABLE, "Changed", false));
    ASSERT_TRUE(context->userUpdateAndRecalculate(KEY_IMMUTABLE, "Changed", false));
    ASSERT_TRUE(ConsoleMessage());

    ASSERT_TRUE(IsEqual("Changed Goodbye 200", component->getCalculated(kPropertyText).asString()));

    // Call SetValue on "a".  That should kill one dependency
    executeCommand("SetValue", {{"property", "a"}, {"value", "Fixed"}, {"componentId", "textId"}}, true);
    ASSERT_TRUE(IsEqual("Fixed Goodbye 200", component->getCalculated(kPropertyText).asString()));

    // Check all of the upstream and downstream dependencies
    // Both "a" and "b" can be modified, because all bound properties can respond to SetValue
    // They generate a single upstream driver
    ASSERT_EQ(1, component->countUpstream());
    ASSERT_EQ(1, component->countUpstream(kPropertyText));

    // Downstream from component context:   a->Text, b->Text
    ASSERT_EQ(2, component->getContext()->countDownstream());
    ASSERT_EQ(1, component->getContext()->countDownstream("a"));
    ASSERT_EQ(1, component->getContext()->countDownstream("b"));

    // Upstream from component context: None (it was killed)
    ASSERT_EQ(0, component->getContext()->countUpstream());

    // Downstream from root context: TestMutable->a
    ASSERT_EQ(0, context->countDownstream());
}

static const char *NESTED = R"({
  "type": "APL",
  "version": "1.1",
  "layouts": {
    "TestLayout": {
      "parameters": [
        "Name"
      ],
      "items": {
        "type": "Container",
        "bind": [
          {
            "name": "InnerName",
            "value": "${Name} the great"
          }
        ],
        "items": {
          "type": "TouchWrapper",
          "id": "TouchId",
          "onPress": {
            "type": "SetValue",
            "property": "InnerName",
            "value": "${Name} the not so great"
          },
          "items": {
            "type": "Text",
            "id": "TextId",
            "text": "${InnerName} of Mesopotamia"
          }
        }
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "TestLayout",
      "Name": "Pat"
    }
  }
})";

/**
 * Test changing properties from internal press events and reaching upwards.  In this case the TouchWrapper
 * changes a bound property that was defined in the outer container.
 */
TEST_F(DependantTest, Nested)
{
    loadDocument(NESTED);
    ASSERT_TRUE(component);

    auto wrapper = TouchWrapperComponent::cast(root->findComponentById("TouchId"));
    ASSERT_TRUE(wrapper);

    auto text = root->findComponentById("TextId");
    ASSERT_TRUE(text);

    // First, we change the parameter passed to the TestLayout to verify that the name changes correctly
    // Notice that we start with the Text component and allow it to search outwards through the contexts
    // until it finds a value it can change
    executeCommand("SetValue", {{"componentId", "TextId"},
                                {"property",    "Name"},
                                {"value",       "Sam"}}, true);
    loop->advanceToEnd();
    ASSERT_TRUE(IsEqual("Sam the great of Mesopotamia", text->getCalculated(kPropertyText).asString()));

    // Next we fire the touch event.  This also searches outwards through the contexts to find a value it can change.
    performTap(0, 0);
    loop->advanceToEnd();
    ASSERT_TRUE(IsEqual("Sam the not so great of Mesopotamia", text->getCalculated(kPropertyText).asString()));

    // Finally we try running the command again.  The SetValue fired by the onPress command broke the dependency
    // from "Name" to "InnerName", so this command does nothing.
    executeCommand("SetValue", {{"componentId", "TextId"},
                                {"property",    "Name"},
                                {"value",       "Fred"}}, true);
    loop->advanceToEnd();
    ASSERT_TRUE(IsEqual("Sam the not so great of Mesopotamia", text->getCalculated(kPropertyText).asString()));
}

static const char *LAYOUT_TEST = R"({
  "type": "APL",
  "version": "1.1",
  "layouts": {
    "square": {
      "parameters": [
        "cnt"
      ],
      "item": {
        "type": "Text",
        "text": "Count: ${cnt}"
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "TouchWrapper",
      "bind": [
        {
          "name": "myCount",
          "value": 1,
          "type": "number"
        }
      ],
      "onPress": {
        "type": "SetValue",
        "property": "myCount",
        "value": "${myCount + 1}"
      },
      "item": {
        "type": "square",
        "cnt": "${myCount}"
      }
    }
  }
})";

TEST_F(DependantTest, Layout)
{
    loadDocument(LAYOUT_TEST);
    ASSERT_TRUE(component);
    auto text = component->getChildAt(0);

    ASSERT_TRUE(IsEqual("Count: 1", text->getCalculated(kPropertyText).asString()));

    // Fire the press event
    performTap(0, 0);;
    ASSERT_TRUE(IsEqual("Count: 2", text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, text));

    // Repeat
    performTap(0, 0);
    ASSERT_TRUE(IsEqual("Count: 3", text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, text));
}

static const char *LAYOUT_MISSING_PROPERTY_TEST = R"({
  "type": "APL",
  "version": "1.1",
  "layouts": {
    "square": {
      "parameters": [
        "cnt"
      ],
      "item": {
        "type": "Text",
        "text": "Count: ${cnt}"
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "TouchWrapper",
      "bind": [
        {
          "name": "myCount",
          "value": 1,
          "type": "number"
        }
      ],
      "item": {
        "type": "square"
      }
    }
  }
})";

TEST_F(DependantTest, LayoutMissingProperty)
{
    loadDocument(LAYOUT_MISSING_PROPERTY_TEST);
    ASSERT_TRUE(component);
    auto text = component->getCoreChildAt(0);

    ASSERT_TRUE(IsEqual("Count: ", text->getCalculated(kPropertyText).asString()));

    // Property should still be live and writable.
    text->setProperty("cnt", 1);
    ASSERT_TRUE(IsEqual("Count: 1", text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, text));

    // Repeat
    text->setProperty("cnt", 3);
    ASSERT_TRUE(IsEqual("Count: 3", text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, text));
}

static const char *LAYOUT_BAD_PROPERTY_TEST = R"({
  "type": "APL",
  "version": "1.1",
  "layouts": {
    "square": {
      "parameters": [
        "cnt"
      ],
      "item": {
        "type": "Text",
        "text": "Count: ${cnt}"
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "TouchWrapper",
      "bind": [
        {
          "name": "myCount",
          "value": 1,
          "type": "number"
        }
      ],
      "item": {
        "type": "square",
        "cnt": "${myCount7}"
      }
    }
  }
})";

TEST_F(DependantTest, LayoutBadProperty)
{
    loadDocument(LAYOUT_BAD_PROPERTY_TEST);
    ASSERT_TRUE(component);
    auto text = component->getCoreChildAt(0);

    ASSERT_TRUE(IsEqual("Count: ", text->getCalculated(kPropertyText).asString()));

    // Property should still be live and writtable.
    text->setProperty("cnt", 1);
    ASSERT_TRUE(IsEqual("Count: 1", text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, text));

    // Repeat
    text->setProperty("cnt", 3);
    ASSERT_TRUE(IsEqual("Count: 3", text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, text));
}


static const char *RESOURCE_LOOKUP = R"apl({
  "type": "APL",
  "version": "1.4",
  "resources": [
    {
      "string": {
        "SUN": "Sunday",
        "MON": "Monday",
        "TUE": "Tuesday",
        "WED": "Wednesday",
        "THU": "Thursday",
        "FRI": "Friday",
        "SAT": "Saturday"
      }
    }
  ],
  "mainTemplate": {
    "items": {
      "type": "TouchWrapper",
      "bind": [
        {
          "name": "DayOfWeek",
          "value": 0
        },
        {
          "name": "DayNames",
          "value": [
            "@SUN",
            "@MON",
            "@TUE",
            "@WED",
            "@THU",
            "@FRI",
            "@SAT"
          ]
        }
      ],
      "items": {
        "type": "Text",
        "text": "${DayNames[DayOfWeek]}"
      },
      "onPress": {
        "type": "SetValue",
        "property": "DayOfWeek",
        "value": "${(DayOfWeek + 1) % 7}"
      }
    }
  }
})apl";

TEST_F(DependantTest, ResourceLookup)
{
    loadDocument(RESOURCE_LOOKUP);
    ASSERT_TRUE(component);
    auto text = component->getChildAt(0);
    ASSERT_TRUE(IsEqual("Sunday", text->getCalculated(kPropertyText).asString()));

    component->update(kUpdatePressed, 1);
    ASSERT_TRUE(IsEqual("Monday", text->getCalculated(kPropertyText).asString()));

}

static const char *AVG_DEPENDENCY = R"apl({
  "type": "APL",
  "version": "1.4",
  "graphics": {
    "equalizer": {
      "type": "AVG",
      "version": "1.1",
      "parameters": [
        {
          "name": "Tick",
          "type": "number",
          "default": 0
        },
        {
          "name": "ColorOn",
          "default": "white",
          "type": "color"
        },
        {
          "name": "ColorOff",
          "default": "transparent",
          "type": "color"
        },
        {
          "name": "Values",
          "default": [
            [1,2,2],
            [2,2,2],
            [3,2,2],
            [4,2,2],
            [5,2,3],
            [6,2,3],
            [6,2,4],
            [5,3,4],
            [5,3,5],
            [4,3,5],
            [4,4,6],
            [3,4,6],
            [3,4,6],
            [2,4,5],
            [2,4,5],
            [1,5,4],
            [2,5,4],
            [3,5,3],
            [4,5,3],
            [5,4,3],
            [6,4,2],
            [6,4,2],
            [5,4,2],
            [5,4,3],
            [4,3,4],
            [4,3,5],
            [3,3,4],
            [3,3,4],
            [2,2,4],
            [2,2,3]
          ]
        }
      ],
      "width": 48,
      "height": 48,
      "items": [
        {
          "type": "path",
          "pathData": "M4,4 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][0] >= 7 ? ColorOn : ColorOff}"
        },
        {
          "type": "path",
          "pathData": "M4,10 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][0] >= 6 ? ColorOn : ColorOff}"
        },
        {
          "type": "path",
          "pathData": "M4,16 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][0] >= 5 ? ColorOn : ColorOff}"
        },
        {
          "type": "path",
          "pathData": "M4,22 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][0] >= 4 ? ColorOn : ColorOff}"
        },
        {
          "type": "path",
          "pathData": "M4,28 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][0] >= 3 ? ColorOn : ColorOff}"
        },
        {
          "type": "path",
          "pathData": "M4,34 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][0] >= 2 ? ColorOn : ColorOff}"
        },
        {
          "type": "path",
          "pathData": "M4,40 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][0] >= 1 ? ColorOn : ColorOff}"
        },
        {
          "type": "path",
          "pathData": "M18,4 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][1] >= 7 ? ColorOn : ColorOff}"
        },
        {
          "type": "path",
          "pathData": "M18,10 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][1] >= 6 ? ColorOn : ColorOff}"
        },
        {
          "type": "path",
          "pathData": "M18,16 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][1] >= 5 ? ColorOn : ColorOff}"
        },
        {
          "type": "path",
          "pathData": "M18,22 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][1] >= 4 ? ColorOn : ColorOff}"
        },
        {
          "type": "path",
          "pathData": "M18,28 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][1] >= 3 ? ColorOn : ColorOff}"
        },
        {
          "type": "path",
          "pathData": "M18,34 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][1] >= 2 ? ColorOn : ColorOff}"
        },
        {
          "type": "path",
          "pathData": "M18,40 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][1] >= 1 ? ColorOn : ColorOff}"
        },
        {
          "type": "path",
          "pathData": "M32,40 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][1] >= 1 ? ColorOn : ColorOff}"
        },
        {
          "type": "path",
          "pathData": "M32,4 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][2] >= 7 ? ColorOn : ColorOff}"
        },
        {
          "type": "path",
          "pathData": "M32,10 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][2] >= 6 ? ColorOn : ColorOff}"
        },
        {
          "type": "path",
          "pathData": "M32,16 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][2] >= 5 ? ColorOn : ColorOff}"
        },
        {
          "type": "path",
          "pathData": "M32,22 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][2] >= 4 ? ColorOn : ColorOff}"
        },
        {
          "type": "path",
          "pathData": "M32,28 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][2] >= 3 ? ColorOn : ColorOff}"
        },
        {
          "type": "path",
          "pathData": "M32,34 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][2] >= 2 ? ColorOn : ColorOff}"
        },
        {
          "type": "path",
          "pathData": "M32,40 l12,0 l0,4 l-12,0 Z",
          "fill": "${Values[Tick % Values.length][2] >= 1 ? ColorOn : ColorOff}"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Container",
      "width": "100%",
      "height": "100%",
      "items":{
        "type": "VectorGraphic",
        "source": "equalizer",
        "scale": "best-fit",
        "width": "100%",
        "align": "center",
        "ColorOn": "white",
        "Tick": "${Math.floor(utcTime / 34)}"
      }
    }
  }
})apl";

TEST_F(DependantTest, AVGDependency)
{
    auto document = std::string(AVG_DEPENDENCY);
    loadDocument(document.c_str());
    ASSERT_TRUE(component);

    auto graphic = component->getCoreChildAt(0)->getCalculated(kPropertyGraphic).get<Graphic>();
    ASSERT_TRUE(graphic);

    ASSERT_FALSE(root->hasEvent());
    root->clearDirty();
    ASSERT_FALSE(root->isDirty());

    advanceTime(34);
    advanceTime(66);

    ASSERT_FALSE(root->hasEvent());
    root->clearDirty();
    ASSERT_FALSE(root->isDirty());

    component = nullptr;
    context = nullptr;
    root = nullptr;
    content = nullptr;
    document = "";

    // Release graphic element last.
    graphic = nullptr;
}

static const char *LAYOUT_LIVE_ARRAY = R"({
  "type": "APL",
  "version": "1.10",
  "theme": "dark",
  "layouts": {
    "MyLayout": {
      "parameters": [
        "things",
        "stuff"
      ],
      "item": {
        "type": "Container",
        "height": "100%",
        "width": "100%",
        "direction": "column",
        "items": [
          {
            "type": "Text",
            "id": "calculatedThings",
            "text": "${things.length}"
          },
          {
            "type": "Text",
            "id": "calculatedStuff",
            "text": "${stuff.potato}"
          }
        ]
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "MyLayout",
      "things": "${ExampleArray}",
      "stuff": "${ExampleMap}"
    }
  }
})";

TEST_F(DependantTest, LayoutLiveArray)
{
    auto la = LiveArray::create();
    config->liveData("ExampleArray", la);
    auto lm = LiveMap::create();
    lm->set("potato", 0);
    config->liveData("ExampleMap", lm);
    loadDocument(LAYOUT_LIVE_ARRAY);
    ASSERT_TRUE(component);

    auto calculatedThings = root->findComponentById("calculatedThings");
    auto calculatedStuff = root->findComponentById("calculatedStuff");

    ASSERT_EQ("0", calculatedThings->getCalculated(kPropertyText).asString());

    la->push_back(0);
    la->push_back(1);
    la->push_back(2);
    la->push_back(3);
    la->push_back(4);
    advanceTime(10);

    ASSERT_EQ("5", calculatedThings->getCalculated(kPropertyText).asString());

    ASSERT_EQ("0", calculatedStuff->getCalculated(kPropertyText).asString());

    lm->set("potato", 5);
    advanceTime(10);

    ASSERT_EQ("5", calculatedStuff->getCalculated(kPropertyText).asString());
}

static const char *LAYOUT_LIVE_ARRAY_SWAP = R"({
  "type": "APL",
  "version": "1.10",
  "theme": "dark",
  "layouts": {
    "MyLayout": {
      "parameters": [
        "things"
      ],
      "item": {
        "type": "Sequence",
        "id": "cont",
        "height": "100%",
        "width": "100%",
        "direction": "column",
        "data": "${things}",
        "items": [
          {
            "type": "Container",
            "bind": [
              {
                "name": "Item",
                "type": "number",
                "value": "${data}"
              }
            ],
            "items": [
              {
                "type": "Text",
                "display": "${Item > 0 ? 'normal' : 'none'}",
                "text": "${Item}"
              },
              {
                "type": "Text",
                "display": "${Item <= 0 ? 'normal' : 'none'}",
                "text": "NAN"
              }
            ]
          }
        ]
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "MyLayout",
      "things": "${ExampleArray}"
    }
  }
})";

TEST_F(DependantTest, LayoutLiveArrayFromEmpty)
{
    auto la = LiveArray::create();
    config->liveData("ExampleArray", la);
    loadDocument(LAYOUT_LIVE_ARRAY_SWAP);
    ASSERT_TRUE(component);

    advanceTime(10);
    ASSERT_TRUE(CheckDirty(root));
    ASSERT_EQ(0, component->getChildCount());

    la->push_back(0);
    la->push_back(1);
    advanceTime(10);

    ASSERT_TRUE(CheckDirty(component->getChildAt(0)->getChildAt(0)));
    ASSERT_TRUE(CheckDirty(component->getChildAt(0)->getChildAt(1),
                           kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(component->getChildAt(1)->getChildAt(0),
                           kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(component->getChildAt(1)->getChildAt(1)));

    auto notifyInternal0 = component->getChildAt(0)->getCalculated(kPropertyNotifyChildrenChanged).getArray();
    ASSERT_EQ(2, notifyInternal0.size());
    ASSERT_EQ(Object("insert"), notifyInternal0.at(0).getMap().at("action"));
    ASSERT_EQ(Object(0), notifyInternal0.at(0).getMap().at("index"));
    ASSERT_EQ(Object(component->getChildAt(0)->getChildAt(0)->getUniqueId()), notifyInternal0.at(0).getMap().at("uid"));
    ASSERT_EQ(Object("insert"), notifyInternal0.at(1).getMap().at("action"));
    ASSERT_EQ(Object(1), notifyInternal0.at(1).getMap().at("index"));
    ASSERT_EQ(Object(component->getChildAt(0)->getChildAt(1)->getUniqueId()), notifyInternal0.at(1).getMap().at("uid"));
    ASSERT_TRUE(CheckChildLaidOutDirtyFlagsWithNotify(component, 0));

    auto notifyInternal1 = component->getChildAt(1)->getCalculated(kPropertyNotifyChildrenChanged).getArray();
    ASSERT_EQ(2, notifyInternal1.size());
    ASSERT_EQ(Object("insert"), notifyInternal1.at(0).getMap().at("action"));
    ASSERT_EQ(Object(0), notifyInternal1.at(0).getMap().at("index"));
    ASSERT_EQ(Object(component->getChildAt(1)->getChildAt(0)->getUniqueId()), notifyInternal1.at(0).getMap().at("uid"));
    ASSERT_EQ(Object("insert"), notifyInternal1.at(1).getMap().at("action"));
    ASSERT_EQ(Object(1), notifyInternal1.at(1).getMap().at("index"));
    ASSERT_EQ(Object(component->getChildAt(1)->getChildAt(1)->getUniqueId()), notifyInternal1.at(1).getMap().at("uid"));
    ASSERT_TRUE(CheckChildLaidOutDirtyFlagsWithNotify(component, 1));

    auto notify = component->getCalculated(kPropertyNotifyChildrenChanged).getArray();
    ASSERT_EQ(2, notify.size());
    ASSERT_EQ(Object("insert"), notify.at(0).getMap().at("action"));
    ASSERT_EQ(Object(0), notify.at(0).getMap().at("index"));
    ASSERT_EQ(Object(component->getChildAt(0)->getUniqueId()), notify.at(0).getMap().at("uid"));
    ASSERT_EQ(Object("insert"), notify.at(1).getMap().at("action"));
    ASSERT_EQ(Object(1), notify.at(1).getMap().at("index"));
    ASSERT_EQ(Object(component->getChildAt(1)->getUniqueId()), notify.at(1).getMap().at("uid"));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

    root->clearDirty();

    ASSERT_EQ(2, component->getChildCount());

    ASSERT_EQ(2, component->getChildAt(0)->getChildAt(0)->getCalculated(kPropertyDisplay).asNumber());
    ASSERT_EQ(0, component->getChildAt(0)->getChildAt(1)->getCalculated(kPropertyDisplay).asNumber());
    ASSERT_EQ("NAN", component->getChildAt(0)->getChildAt(1)->getCalculated(kPropertyText).asString());

    ASSERT_EQ(0, component->getChildAt(1)->getChildAt(0)->getCalculated(kPropertyDisplay).asNumber());
    ASSERT_EQ(2, component->getChildAt(1)->getChildAt(1)->getCalculated(kPropertyDisplay).asNumber());
    ASSERT_EQ("1", component->getChildAt(1)->getChildAt(0)->getCalculated(kPropertyText).asString());

    la->update(0, 2);
    la->update(1, 0);
    advanceTime(10);

    ASSERT_TRUE(CheckDirty(component->getChildAt(0)->getChildAt(0),
                           kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut, kPropertyVisualHash,
                           kPropertyDisplay, kPropertyText));
    ASSERT_TRUE(CheckDirty(component->getChildAt(0)->getChildAt(1),
                           kPropertyBounds, kPropertyInnerBounds, kPropertyVisualHash, kPropertyDisplay));
    ASSERT_TRUE(CheckDirty(component->getChildAt(1)->getChildAt(0),
                           kPropertyBounds, kPropertyInnerBounds, kPropertyVisualHash, kPropertyDisplay,
                           kPropertyText));
    ASSERT_TRUE(CheckDirty(component->getChildAt(1)->getChildAt(1),
                           kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut, kPropertyVisualHash,
                           kPropertyDisplay));

    notifyInternal0 = component->getChildAt(0)->getCalculated(kPropertyNotifyChildrenChanged).getArray();
    ASSERT_EQ(0, notifyInternal0.size());
    ASSERT_TRUE(CheckDirty(component->getChildAt(0), kPropertyNotifyChildrenChanged));

    notifyInternal1 = component->getChildAt(1)->getCalculated(kPropertyNotifyChildrenChanged).getArray();
    ASSERT_EQ(0, notifyInternal1.size());
    ASSERT_TRUE(CheckDirty(component->getChildAt(1), kPropertyNotifyChildrenChanged));

    notify = component->getCalculated(kPropertyNotifyChildrenChanged).getArray();
    ASSERT_EQ(0, notify.size());
    ASSERT_TRUE(CheckDirty(component));

    root->clearDirty();

    ASSERT_EQ(2, component->getChildCount());

    ASSERT_EQ(0, component->getChildAt(0)->getChildAt(0)->getCalculated(kPropertyDisplay).asNumber());
    ASSERT_EQ(2, component->getChildAt(0)->getChildAt(1)->getCalculated(kPropertyDisplay).asNumber());
    ASSERT_EQ("2", component->getChildAt(0)->getChildAt(0)->getCalculated(kPropertyText).asString());

    ASSERT_EQ(2, component->getChildAt(1)->getChildAt(0)->getCalculated(kPropertyDisplay).asNumber());
    ASSERT_EQ(0, component->getChildAt(1)->getChildAt(1)->getCalculated(kPropertyDisplay).asNumber());
    ASSERT_EQ("NAN", component->getChildAt(1)->getChildAt(1)->getCalculated(kPropertyText).asString());
}

TEST_F(DependantTest, LayoutLiveArrayFromEmptyReplace)
{
    auto la = LiveArray::create();
    config->liveData("ExampleArray", la);
    loadDocument(LAYOUT_LIVE_ARRAY_SWAP);
    ASSERT_TRUE(component);

    advanceTime(10);
    ASSERT_TRUE(CheckDirty(root));

    ASSERT_EQ(0, component->getChildCount());

    la->push_back(0);
    la->push_back(1);
    advanceTime(10);

    ASSERT_TRUE(CheckDirty(component->getChildAt(0)->getChildAt(0)));
    ASSERT_TRUE(CheckDirty(component->getChildAt(0)->getChildAt(1),
                           kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(component->getChildAt(1)->getChildAt(0),
                           kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(component->getChildAt(1)->getChildAt(1)));

    auto notifyInternal0 = component->getChildAt(0)->getCalculated(kPropertyNotifyChildrenChanged).getArray();
    ASSERT_EQ(2, notifyInternal0.size());
    ASSERT_EQ(Object("insert"), notifyInternal0.at(0).getMap().at("action"));
    ASSERT_EQ(Object(0), notifyInternal0.at(0).getMap().at("index"));
    ASSERT_EQ(Object(component->getChildAt(0)->getChildAt(0)->getUniqueId()), notifyInternal0.at(0).getMap().at("uid"));
    ASSERT_EQ(Object("insert"), notifyInternal0.at(1).getMap().at("action"));
    ASSERT_EQ(Object(1), notifyInternal0.at(1).getMap().at("index"));
    ASSERT_EQ(Object(component->getChildAt(0)->getChildAt(1)->getUniqueId()), notifyInternal0.at(1).getMap().at("uid"));
    ASSERT_TRUE(CheckChildLaidOutDirtyFlagsWithNotify(component, 0));

    auto notifyInternal1 = component->getChildAt(1)->getCalculated(kPropertyNotifyChildrenChanged).getArray();
    ASSERT_EQ(2, notifyInternal1.size());
    ASSERT_EQ(Object("insert"), notifyInternal1.at(0).getMap().at("action"));
    ASSERT_EQ(Object(0), notifyInternal1.at(0).getMap().at("index"));
    ASSERT_EQ(Object(component->getChildAt(1)->getChildAt(0)->getUniqueId()), notifyInternal1.at(0).getMap().at("uid"));
    ASSERT_EQ(Object("insert"), notifyInternal1.at(1).getMap().at("action"));
    ASSERT_EQ(Object(1), notifyInternal1.at(1).getMap().at("index"));
    ASSERT_EQ(Object(component->getChildAt(1)->getChildAt(1)->getUniqueId()), notifyInternal1.at(1).getMap().at("uid"));
    ASSERT_TRUE(CheckChildLaidOutDirtyFlagsWithNotify(component, 1));

    auto cachedUid0 = component->getChildAt(0)->getUniqueId();
    auto cachedUid1 = component->getChildAt(1)->getUniqueId();
    auto notify = component->getCalculated(kPropertyNotifyChildrenChanged).getArray();
    ASSERT_EQ(2, notify.size());
    ASSERT_EQ(Object("insert"), notify.at(0).getMap().at("action"));
    ASSERT_EQ(Object(0), notify.at(0).getMap().at("index"));
    ASSERT_EQ(Object(cachedUid0), notify.at(0).getMap().at("uid"));
    ASSERT_EQ(Object("insert"), notify.at(1).getMap().at("action"));
    ASSERT_EQ(Object(1), notify.at(1).getMap().at("index"));
    ASSERT_EQ(Object(cachedUid1), notify.at(1).getMap().at("uid"));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged));

    root->clearDirty();

    ASSERT_EQ(2, component->getChildCount());

    ASSERT_EQ(2, component->getChildAt(0)->getChildAt(0)->getCalculated(kPropertyDisplay).asNumber());
    ASSERT_EQ(0, component->getChildAt(0)->getChildAt(1)->getCalculated(kPropertyDisplay).asNumber());
    ASSERT_EQ("NAN", component->getChildAt(0)->getChildAt(1)->getCalculated(kPropertyText).asString());

    ASSERT_EQ(0, component->getChildAt(1)->getChildAt(0)->getCalculated(kPropertyDisplay).asNumber());
    ASSERT_EQ(2, component->getChildAt(1)->getChildAt(1)->getCalculated(kPropertyDisplay).asNumber());
    ASSERT_EQ("1", component->getChildAt(1)->getChildAt(0)->getCalculated(kPropertyText).asString());

    la->clear();
    la->push_back(2);
    la->push_back(0);
    advanceTime(10);

    ASSERT_TRUE(CheckDirty(component->getChildAt(0)->getChildAt(0),
                           kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(component->getChildAt(0)->getChildAt(1)));
    ASSERT_TRUE(CheckDirty(component->getChildAt(1)->getChildAt(0)));
    ASSERT_TRUE(CheckDirty(component->getChildAt(1)->getChildAt(1),
                           kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut, kPropertyVisualHash));

    notifyInternal0 = component->getChildAt(0)->getCalculated(kPropertyNotifyChildrenChanged).getArray();
    ASSERT_EQ(2, notifyInternal0.size());
    ASSERT_EQ(Object("insert"), notifyInternal0.at(0).getMap().at("action"));
    ASSERT_EQ(Object(0), notifyInternal0.at(0).getMap().at("index"));
    ASSERT_EQ(Object(component->getChildAt(0)->getChildAt(0)->getUniqueId()), notifyInternal0.at(0).getMap().at("uid"));
    ASSERT_EQ(Object("insert"), notifyInternal0.at(1).getMap().at("action"));
    ASSERT_EQ(Object(1), notifyInternal0.at(1).getMap().at("index"));
    ASSERT_EQ(Object(component->getChildAt(0)->getChildAt(1)->getUniqueId()), notifyInternal0.at(1).getMap().at("uid"));
    ASSERT_TRUE(CheckChildLaidOutDirtyFlagsWithNotify(component, 0));

    notifyInternal1 = component->getChildAt(1)->getCalculated(kPropertyNotifyChildrenChanged).getArray();
    ASSERT_EQ(2, notifyInternal1.size());
    ASSERT_EQ(Object("insert"), notifyInternal1.at(0).getMap().at("action"));
    ASSERT_EQ(Object(0), notifyInternal1.at(0).getMap().at("index"));
    ASSERT_EQ(Object(component->getChildAt(1)->getChildAt(0)->getUniqueId()), notifyInternal1.at(0).getMap().at("uid"));
    ASSERT_EQ(Object("insert"), notifyInternal1.at(1).getMap().at("action"));
    ASSERT_EQ(Object(1), notifyInternal1.at(1).getMap().at("index"));
    ASSERT_EQ(Object(component->getChildAt(1)->getChildAt(1)->getUniqueId()), notifyInternal1.at(1).getMap().at("uid"));
    ASSERT_TRUE(CheckChildLaidOutDirtyFlagsWithNotify(component, 1));

    auto sp = component->scrollPosition();

    notify = component->getCalculated(kPropertyNotifyChildrenChanged).getArray();
    ASSERT_EQ(4, notify.size());
    ASSERT_EQ(Object("insert"), notify.at(0).getMap().at("action"));
    ASSERT_EQ(Object(0), notify.at(0).getMap().at("index"));
    ASSERT_EQ(Object(component->getChildAt(0)->getUniqueId()), notify.at(0).getMap().at("uid"));
    ASSERT_EQ(Object("insert"), notify.at(1).getMap().at("action"));
    ASSERT_EQ(Object(1), notify.at(1).getMap().at("index"));
    ASSERT_EQ(Object(component->getChildAt(1)->getUniqueId()), notify.at(1).getMap().at("uid"));
    ASSERT_EQ(Object("remove"), notify.at(2).getMap().at("action"));
    ASSERT_EQ(Object(2), notify.at(2).getMap().at("index"));
    ASSERT_EQ(Object(cachedUid0), notify.at(2).getMap().at("uid"));
    ASSERT_EQ(Object("remove"), notify.at(3).getMap().at("action"));
    ASSERT_EQ(Object(2), notify.at(3).getMap().at("index"));
    ASSERT_EQ(Object(cachedUid1), notify.at(3).getMap().at("uid"));
    ASSERT_TRUE(CheckDirty(component, kPropertyNotifyChildrenChanged, kPropertyScrollPosition));

    ASSERT_EQ(sp, component->scrollPosition());

    ASSERT_EQ(2, component->getChildCount());

    ASSERT_EQ(0, component->getChildAt(0)->getChildAt(0)->getCalculated(kPropertyDisplay).asNumber());
    ASSERT_EQ(2, component->getChildAt(0)->getChildAt(1)->getCalculated(kPropertyDisplay).asNumber());
    ASSERT_EQ("2", component->getChildAt(0)->getChildAt(0)->getCalculated(kPropertyText).asString());

    ASSERT_EQ(2, component->getChildAt(1)->getChildAt(0)->getCalculated(kPropertyDisplay).asNumber());
    ASSERT_EQ(0, component->getChildAt(1)->getChildAt(1)->getCalculated(kPropertyDisplay).asNumber());
    ASSERT_EQ("NAN", component->getChildAt(1)->getChildAt(1)->getCalculated(kPropertyText).asString());
}