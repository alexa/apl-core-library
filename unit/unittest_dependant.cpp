/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <apl/component/touchwrappercomponent.h>
#include "testeventloop.h"
#include "apl/engine/contextdependant.h"

using namespace apl;

class DependantTest : public DocumentWrapper {};

static const char *CONTEXT_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"bind\": ["
    "        {"
    "          \"name\": \"a\","
    "          \"value\": 22"
    "        }"
    "      ],"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"bind\": ["
    "          {"
    "            \"name\": \"b\","
    "            \"value\": \"${a}\""
    "          }"
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

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

static const char *CONTEXT_TEST_2 =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"bind\": ["
    "        {"
    "          \"name\": \"a\","
    "          \"value\": 22"
    "        },"
    "        {"
    "          \"name\": \"b\","
    "          \"value\": \"red\","
    "          \"type\": \"color\""
    "        },"
    "        {"
    "          \"name\": \"c\","
    "          \"value\": \"${a+10}\""
    "        }"
    "      ],"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"bind\": ["
    "          {"
    "            \"name\": \"x\","
    "            \"value\": \"${Math.min(a, 100)}\""
    "          },"
    "          {"
    "            \"name\": \"y\","
    "            \"value\": \"${b}\""
    "          }"
    "        ],"
    "        \"items\": {"
    "          \"type\": \"Text\","
    "          \"bind\": ["
    "            {"
    "              \"name\": \"z\","
    "              \"value\": \"${a*c}\""
    "            }"
    "          ]"
    "        }"
    "      }"
    "    }"
    "  }"
    "}";

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

const static char * COMPONENT_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"bind\": ["
    "        {"
    "          \"name\": \"a\","
    "          \"value\": 22"
    "        }"
    "      ],"
    "      \"text\": \"Is ${a}\""
    "    }"
    "  }"
    "}";

TEST_F(DependantTest, Component)
{
    loadDocument(COMPONENT_TEST);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("Is 22", component->getCalculated(kPropertyText).asString()));

    // Update the context and verify that things change
    ASSERT_TRUE(component->getContext()->userUpdateAndRecalculate("a", "fuzzy", true));
    ASSERT_TRUE(IsEqual("Is fuzzy", component->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(component, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, component));

    // Updating the context with the same value should not set dirty flags
    ASSERT_TRUE(component->getContext()->userUpdateAndRecalculate("a", "fuzzy", true));
    ASSERT_TRUE(IsEqual("Is fuzzy", component->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(component));
    ASSERT_TRUE(CheckDirty(root));

    // Now assign a value - this should cancel the assignment.
    component->setProperty(kPropertyText, "hello");
    ASSERT_TRUE(IsEqual("hello", component->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(component, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, component));

    // Verify that the assignment is cancelled.
    ASSERT_TRUE(component->getContext()->userUpdateAndRecalculate("a", 10, true));
    ASSERT_TRUE(IsEqual("hello", component->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(component));
    ASSERT_TRUE(CheckDirty(root));
}

static const char *COUNTER_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"bind\": ["
    "        {"
    "          \"name\": \"myCount\","
    "          \"value\": 0,"
    "          \"type\": \"number\""
    "        }"
    "      ],"
    "      \"onPress\": {"
    "        \"type\": \"SetValue\","
    "        \"property\": \"myCount\","
    "        \"value\": \"${myCount + 1}\""
    "      },"
    "      \"item\": {"
    "        \"type\": \"Text\","
    "        \"text\": \"Count: ${myCount}\""
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(DependantTest, Counter)
{
    loadDocument(COUNTER_TEST);
    ASSERT_TRUE(component);
    auto text = component->getChildAt(0);

    ASSERT_TRUE(IsEqual("Count: 0", text->getCalculated(kPropertyText).asString()));

    // Fire the press event
    component->update(kUpdatePressed, 0);
    ASSERT_TRUE(IsEqual("Count: 1", text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(text, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, text));

    // Repeat
    component->update(kUpdatePressed, 0);
    ASSERT_TRUE(IsEqual("Count: 2", text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(text, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, text));
}

TEST_F(DependantTest, FreeContext)
{
    context = Context::create(metrics, makeDefaultSession());

    // Parent context
    auto first = Context::create(context);
    first->putUserWriteable("source", 23);

    // Child context
    auto second = Context::create(first);
    second->putUserWriteable("target", 10);
    ASSERT_EQ(10, second->opt("target").asNumber());

    // Manually construct a dependency between source and target
    auto node = parseDataBinding(context, "${source * 2}");
    ASSERT_TRUE(node.isNode());
    ContextDependant::create(first, "source",
                             second, "target", second,
                             node, sBindingFunctions.at(BindingType::kBindingTypeNumber));

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

static const char *FREE_COMPONENT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"bind\": ["
    "        {"
    "          \"name\": \"a\","
    "          \"value\": 22"
    "        }"
    "      ],"
    "      \"text\": \"Is ${a}\""
    "    }"
    "  }"
    "}";

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

static const char *BREAK_CHAIN =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"bind\": ["
    "        {"
    "          \"name\": \"a\","
    "          \"value\": 22"
    "        },"
    "        {"
    "          \"name\": \"b\","
    "          \"value\": \"${a*a}\""
    "        }"
    "      ],"
    "      \"text\": \"Is ${b}\""
    "    }"
    "  }"
    "}";

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

static const char *STATIC_PROPERTY =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"bind\": ["
    "        {"
    "          \"name\": \"a\","
    "          \"value\": 22"
    "        }"
    "      ],"
    "      \"fontSize\": \"${a}\""
    "    }"
    "  }"
    "}";

TEST_F(DependantTest, StaticProperty)
{
    loadDocument(STATIC_PROPERTY);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(Dimension(22), component->getCalculated(kPropertyFontSize)));

    // FontSize is not dynamic.  It can't be changed through propagation
    ASSERT_TRUE(component->getContext()->userUpdateAndRecalculate("a", 10, false));
    ASSERT_TRUE(IsEqual(Dimension(22), component->getCalculated(kPropertyFontSize)));
}

static const char *MUTABLE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"id\": \"textId\","
    "      \"bind\": ["
    "        {"
    "          \"name\": \"a\","
    "          \"value\": \"${TestMutable}\""
    "        },"
    "        {"
    "          \"name\": \"b\","
    "          \"value\": \"${TestImmutable}\""
    "        }"
    "      ],"
    "      \"text\": \"${a} ${b} ${viewport.width}\""
    "    }"
    "  }"
    "}";

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
    ASSERT_EQ(2, component->countUpstream());
    ASSERT_EQ(2, component->countUpstream(kPropertyText));

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
    ASSERT_EQ(2, component->countUpstream());
    ASSERT_EQ(2, component->countUpstream(kPropertyText));

    // Downstream from component context:   a->Text, b->Text
    ASSERT_EQ(2, component->getContext()->countDownstream());
    ASSERT_EQ(1, component->getContext()->countDownstream("a"));
    ASSERT_EQ(1, component->getContext()->countDownstream("b"));

    // Upstream from component context: None (it was killed)
    ASSERT_EQ(0, component->getContext()->countUpstream());

    // Downstream from root context: TestMutable->a
    ASSERT_EQ(0, context->countDownstream());
}

static const char *NESTED =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"layouts\": {"
    "    \"TestLayout\": {"
    "      \"parameters\": ["
    "        \"Name\""
    "      ],"
    "      \"items\": {"
    "        \"type\": \"Container\","
    "        \"bind\": ["
    "          {"
    "            \"name\": \"InnerName\","
    "            \"value\": \"${Name} the great\""
    "          }"
    "        ],"
    "        \"items\": {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"TouchId\","
    "          \"onPress\": {"
    "            \"type\": \"SetValue\","
    "            \"property\": \"InnerName\","
    "            \"value\": \"${Name} the not so great\""
    "          },"
    "          \"items\": {"
    "            \"type\": \"Text\","
    "            \"id\": \"TextId\","
    "            \"text\": \"${InnerName} of Mesopotamia\""
    "          }"
    "        }"
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"TestLayout\","
    "      \"Name\": \"Pat\""
    "    }"
    "  }"
    "}";

/**
 * Test changing properties from internal press events and reaching upwards.  In this case the TouchWrapper
 * changes a bound property that was defined in the outer container.
 */
TEST_F(DependantTest, Nested)
{
    loadDocument(NESTED);
    ASSERT_TRUE(component);

    auto wrapper = std::dynamic_pointer_cast<TouchWrapperComponent>(component->findComponentById("TouchId"));
    ASSERT_TRUE(wrapper);

    auto text = component->findComponentById("TextId");
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
    wrapper->update(kUpdatePressed, 1);
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