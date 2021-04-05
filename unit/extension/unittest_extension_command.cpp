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

class ExtensionCommandTest : public DocumentWrapper {};

TEST_F(ExtensionCommandTest, CommandDefEmpty)
{
    ExtensionCommandDefinition def("aplext:TEST", "MyFooCommand");

    ASSERT_STREQ("MyFooCommand", def.getName().c_str());
    ASSERT_STREQ("aplext:TEST", def.getURI().c_str());
    ASSERT_FALSE(def.getAllowFastMode());
    ASSERT_FALSE(def.getRequireResolution());
    ASSERT_TRUE(def.getPropertyMap().empty());
}

TEST_F(ExtensionCommandTest, CommandDefSimple)
{
    auto def = ExtensionCommandDefinition("aplext:Test", "MyFooCommand")
        .allowFastMode(true)
        .requireResolution(true)
        .property("width", 100, false)
        .property("height", 120, true);

    ASSERT_STREQ("MyFooCommand", def.getName().c_str());
    ASSERT_STREQ("aplext:Test", def.getURI().c_str());
    ASSERT_TRUE(def.getAllowFastMode());
    ASSERT_TRUE(def.getRequireResolution());
    ASSERT_EQ(2, def.getPropertyMap().size());
    ASSERT_TRUE(IsEqual(100, def.getPropertyMap().at("width").defvalue));
    ASSERT_FALSE(def.getPropertyMap().at("width").required);
    ASSERT_TRUE(IsEqual(120, def.getPropertyMap().at("height").defvalue));
    ASSERT_TRUE(def.getPropertyMap().at("height").required);
}

TEST_F(ExtensionCommandTest, CommandDefIllegal)
{
    auto def = ExtensionCommandDefinition("aplext:Test", "MyFooCommand")
        .property("type", 100, false)
        .property("when", false, false);

    ASSERT_TRUE(def.getPropertyMap().empty());
    ASSERT_TRUE(LogMessage());
}

static const char * BASIC =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.2\","
    "  \"extensions\": {"
    "    \"name\": \"T\","
    "    \"uri\": \"aplext:Test\""
    "  },"
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"onPress\": ["
    "        {"
    "          \"type\": \"T:MyCommand\","
    "          \"value\": 7"
    "        },"
    "        {"
    "          \"type\": \"SetValue\","
    "          \"componentId\": \"MyFrame\","
    "          \"property\": \"backgroundColor\","
    "          \"value\": \"black\""
    "        }"
    "      ],"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"id\": \"MyFrame\","
    "        \"backgroundColor\": \"white\""
    "      }"
    "    }"
    "  }"
    "}";

/**
 * Invoking a extension command when it has not been set up in the Rootconfig->
 * The extension command should be ignored and the following command should run normally.
 */
TEST_F(ExtensionCommandTest, BasicMissingCommand)
{
    loadDocument(BASIC);

    ASSERT_TRUE(component);
    auto frame = component->findComponentById("MyFrame");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(IsEqual(Color(Color::WHITE), frame->getCalculated(kPropertyBackgroundColor)));

    performTap(0, 0);
    loop->runPending();

    // Since the command wasn't registered, we expect to have no event and a console message
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());
    ASSERT_TRUE(IsEqual(Color(Color::BLACK), frame->getCalculated(kPropertyBackgroundColor)));
}

/**
 * Invoke a extension command when it HAS been set up correctly in the Rootconfig->
 * We expect to get an event with the command and correctly set property values.
 */
TEST_F(ExtensionCommandTest, BasicCommand)
{
    config->registerExtensionCommand(ExtensionCommandDefinition("aplext:Test", "MyCommand")
                                        .property("value", -1, false));
    loadDocument(BASIC);

    ASSERT_TRUE(component);
    auto frame = component->findComponentById("MyFrame");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(IsEqual(Color(Color::WHITE), frame->getCalculated(kPropertyBackgroundColor)));

    performTap(0, 0);
    loop->runPending();

    ASSERT_TRUE(root->hasEvent());
    ASSERT_FALSE(ConsoleMessage());

    auto event = root->popEvent();
    ASSERT_TRUE(IsEqual("MyCommand", event.getValue(kEventPropertyName)));
    ASSERT_TRUE(IsEqual("aplext:Test", event.getValue(kEventPropertyExtensionURI)));

    auto source = event.getValue(kEventPropertySource);
    ASSERT_TRUE(source.isMap());
    ASSERT_TRUE(IsEqual("TouchWrapper", source.get("type")));

    auto ext = event.getValue(kEventPropertyExtension);
    ASSERT_TRUE(ext.isMap());
    ASSERT_TRUE(IsEqual(7, ext.get("value")));

    ASSERT_TRUE(event.getActionRef().isEmpty());

    // The SetValue command should also have run by now
    ASSERT_TRUE(IsEqual(Color(Color::BLACK), frame->getCalculated(kPropertyBackgroundColor)));
}

/**
 * Invoke a extension command that requires resolution.  The next command in the sequence will
 * be pended until the first command is resolved.
 */
TEST_F(ExtensionCommandTest, BasicCommandWithActionRef)
{
    config->registerExtensionCommand(ExtensionCommandDefinition("aplext:Test", "MyCommand")
                                        .property("value", -1, false)
                                        .requireResolution(true)
    );
    loadDocument(BASIC);

    ASSERT_TRUE(component);
    auto frame = component->findComponentById("MyFrame");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(IsEqual(Color(Color::WHITE), frame->getCalculated(kPropertyBackgroundColor)));

    performTap(0, 0);

    ASSERT_TRUE(root->hasEvent());
    ASSERT_FALSE(ConsoleMessage());

    auto event = root->popEvent();
    ASSERT_TRUE(IsEqual("MyCommand", event.getValue(kEventPropertyName)));
    ASSERT_TRUE(IsEqual("aplext:Test", event.getValue(kEventPropertyExtensionURI)));

    auto source = event.getValue(kEventPropertySource);
    ASSERT_TRUE(source.isMap());
    ASSERT_TRUE(IsEqual("TouchWrapper", source.get("type")));

    auto ext = event.getValue(kEventPropertyExtension);
    ASSERT_TRUE(ext.isMap());
    ASSERT_TRUE(IsEqual(7, ext.get("value")));

    ASSERT_FALSE(event.getActionRef().isEmpty());

    // The SetValue command should NOT have run
    ASSERT_TRUE(IsEqual(Color(Color::WHITE), frame->getCalculated(kPropertyBackgroundColor)));

    // Now we resolve the action reference and release the color change
    event.getActionRef().resolve();
    loop->runPending();

    ASSERT_TRUE(IsEqual(Color(Color::BLACK), frame->getCalculated(kPropertyBackgroundColor)));
}

static const char * RICH_ARGUMENTS =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.3\","
    "  \"extensions\": ["
    "    {"
    "      \"name\": \"A\","
    "      \"uri\": \"URI_A\""
    "    }"
    "  ],"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"id\": \"MyTouchWrapper\","
    "      \"onPress\": {"
    "        \"type\": \"A:doIt\","
    "        \"value\": ["
    "          \"${event.source.id}\","
    "          \"${event.source.value}\""
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

/**
 * Verify that data-binding evaluation is occuring inside of an array
 */
TEST_F(ExtensionCommandTest, RichArguments)
{
    config->registerExtensionCommand(
        ExtensionCommandDefinition("URI_A", "doIt")
            .property("value", Object::EMPTY_ARRAY(), false)
    );

    loadDocument(RICH_ARGUMENTS);

    ASSERT_TRUE(component);
    performTap(0, 0);

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    ASSERT_TRUE(IsEqual("doIt", event.getValue(kEventPropertyName)));
    ASSERT_TRUE(IsEqual("URI_A", event.getValue(kEventPropertyExtensionURI)));

    auto source = event.getValue(kEventPropertySource);
    ASSERT_TRUE(source.isMap());
    ASSERT_TRUE(IsEqual("MyTouchWrapper", source.get("id")));
    ASSERT_TRUE(IsEqual("TouchWrapper", source.get("type")));

    auto ext = event.getValue(kEventPropertyExtension);
    ASSERT_TRUE(ext.isMap());

    auto value = ext.get("value");
    ASSERT_TRUE(value.isArray());
    ASSERT_EQ(2, value.size());
    ASSERT_TRUE(IsEqual("MyTouchWrapper", value.at(0)));
    ASSERT_TRUE(IsEqual(false, value.at(1)));
}

static const char *RICH_ARGUMENTS_WITH_PAYLOAD =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.3\","
    "  \"extensions\": ["
    "    {"
    "      \"name\": \"A\","
    "      \"uri\": \"URI_A\""
    "    }"
    "  ],"
    "  \"mainTemplate\": {"
    "    \"parameters\": ["
    "      \"payload\""
    "    ],"
    "    \"items\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"id\": \"MyTouchWrapper\","
    "      \"onPress\": {"
    "        \"type\": \"A:doIt\","
    "        \"positions\": ["
    "          \"${payload.subarray}\""
    "        ],"
    "        \"map\": {"
    "          \"key\": \"${payload.key}\","
    "          \"value\": ["
    "            \"${payload.basePosition}\","
    "            \"${payload.basePosition + 10}\""
    "          ]"
    "        }"
    "      }"
    "    }"
    "  }"
    "}";

/**
 * Verify that data-binding evaluation is occurring inside of a map and an array-ified array
 */
TEST_F(ExtensionCommandTest, RichArgumentsArrayify)
{
    config->registerExtensionCommand(
        ExtensionCommandDefinition("URI_A", "doIt")
            .arrayProperty("positions", false)
            .arrayProperty("missing", false)
            .property("map", Object::NULL_OBJECT(), false)
    );

    loadDocument(RICH_ARGUMENTS_WITH_PAYLOAD,
        R"({"subarray": [1,2,"foo"], "key": "TheKey", "basePosition": 20})");

    ASSERT_TRUE(component);
    performTap(0, 0);

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    ASSERT_TRUE(IsEqual("doIt", event.getValue(kEventPropertyName)));
    ASSERT_TRUE(IsEqual("URI_A", event.getValue(kEventPropertyExtensionURI)));

    auto source = event.getValue(kEventPropertySource);
    ASSERT_TRUE(source.isMap());
    ASSERT_TRUE(IsEqual("MyTouchWrapper", source.get("id")));
    ASSERT_TRUE(IsEqual("TouchWrapper", source.get("type")));

    auto ext = event.getValue(kEventPropertyExtension);
    ASSERT_TRUE(ext.isMap());

    auto positions = ext.get("positions");
    ASSERT_TRUE(positions.isArray());
    ASSERT_EQ(3, positions.size());
    ASSERT_TRUE(IsEqual(1, positions.at(0)));
    ASSERT_TRUE(IsEqual(2, positions.at(1)));
    ASSERT_TRUE(IsEqual("foo", positions.at(2)));

    auto missing = ext.get("missing");
    ASSERT_TRUE(missing.isArray());
    ASSERT_EQ(0, missing.size());

    auto map = ext.get("map");
    ASSERT_TRUE(map.isMap());
    ASSERT_TRUE(IsEqual("TheKey", map.get("key")));

    auto subarray = map.get("value");
    ASSERT_TRUE(subarray.isArray());
    ASSERT_TRUE(IsEqual(20, subarray.at(0)));
    ASSERT_TRUE(IsEqual(30, subarray.at(1)));
}

static const char * SCROLL_VIEW =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.2\","
    "  \"extensions\": {"
    "    \"name\": \"T\","
    "    \"uri\": \"aplext:Test\""
    "  },"
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"ScrollView\","
    "      \"id\": \"MyScrollView\","
    "      \"height\": 100,"
    "      \"onScroll\": ["
    "        {"
    "          \"type\": \"T:MyCommand\","
    "          \"id\": \"${event.source.id}\","
    "          \"value\": \"${event.source.value}\""
    "        },"
    "        {"
    "          \"type\": \"SetValue\","
    "          \"componentId\": \"MyFrame\","
    "          \"property\": \"backgroundColor\","
    "          \"value\": \"red\""
    "        }"
    "      ],"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"id\": \"MyFrame\","
    "        \"height\": \"200\","
    "        \"backgroundColor\": \"green\""
    "      }"
    "    }"
    "  }"
    "}";

/**
 * Run a extension command in fast mode.  Mark the extension command as NOT runnable in fast mode.
 * The command should be skipped and the following command should be executed.
 */
TEST_F(ExtensionCommandTest, FastModeNotAllowed)
{
    config->registerExtensionCommand(ExtensionCommandDefinition("aplext:Test", "MyCommand")
                                        .property("id", "NO_ID", true)   // Required property
                                        .property("value", 0, false)
                                        .allowFastMode(false)     // Do not run in fast mode
                                        .requireResolution(true)  // Resolution isn't required in fast mode
    );
    loadDocument(SCROLL_VIEW);

    ASSERT_TRUE(component);
    auto frame = component->findComponentById("MyFrame");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), frame->getCalculated(kPropertyBackgroundColor)));

    component->update(kUpdateScrollPosition, 50);  // Halfway
    loop->runPending();

    ASSERT_FALSE(root->hasEvent());  // No event generated because fast mode is not supported
    ASSERT_TRUE(ConsoleMessage());   // A console message is logged about skipping the command

    // The SetValue command will have run
    ASSERT_TRUE(IsEqual(Color(Color::RED), frame->getCalculated(kPropertyBackgroundColor)));
}

/**
 * Run a extension command in fast mode.  Mark the extension command as runnable in fast mode,
 * but also mark it as requiring resolution.  Because it is fast mode, the command should
 * run and NOT require resolution.
 */
TEST_F(ExtensionCommandTest, FastModeAllowed)
{
    config->registerExtensionCommand(ExtensionCommandDefinition("aplext:Test", "MyCommand")
                                     .property("id", "NO_ID", true)   // Required property
                                     .property("value", 0, false)
                                     .allowFastMode(true)     // Allow running in fast mode
                                     .requireResolution(true) // Resolution isn't required in fast mode even if set
    );
    loadDocument(SCROLL_VIEW);

    ASSERT_TRUE(component);
    auto frame = component->findComponentById("MyFrame");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), frame->getCalculated(kPropertyBackgroundColor)));

    component->update(kUpdateScrollPosition, 50);  // Halfway
    loop->runPending();

    ASSERT_TRUE(root->hasEvent());  // An event is generated in fast mode
    ASSERT_FALSE(ConsoleMessage());

    auto event = root->popEvent();
    ASSERT_TRUE(IsEqual("MyCommand", event.getValue(kEventPropertyName)));
    ASSERT_TRUE(IsEqual("aplext:Test", event.getValue(kEventPropertyExtensionURI)));

    auto source = event.getValue(kEventPropertySource);
    ASSERT_TRUE(source.isMap());
    ASSERT_TRUE(IsEqual("ScrollView", source.get("type")));

    auto ext = event.getValue(kEventPropertyExtension);
    ASSERT_TRUE(ext.isMap());
    ASSERT_TRUE(IsEqual(0.5, ext.get("value")));  // Scroll position of 50%
    ASSERT_TRUE(IsEqual("MyScrollView", ext.get("id")));

    ASSERT_TRUE(event.getActionRef().isEmpty());  // No action ref is generated in fast mode

    // The SetValue command should have run
    ASSERT_TRUE(IsEqual(Color(Color::RED), frame->getCalculated(kPropertyBackgroundColor)));
}

static const char * SCROLL_VIEW_BAD_COMMAND =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.2\","
    "  \"extensions\": {"
    "    \"name\": \"T\","
    "    \"uri\": \"aplext:Test\""
    "  },"
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"ScrollView\","
    "      \"id\": \"MyScrollView\","
    "      \"height\": 100,"
    "      \"onScroll\": ["
    "        {"
    "          \"type\": \"T:MyCommand\""
    "        },"
    "        {"
    "          \"type\": \"SetValue\","
    "          \"componentId\": \"MyFrame\","
    "          \"property\": \"backgroundColor\","
    "          \"value\": \"red\""
    "        }"
    "      ],"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"id\": \"MyFrame\","
    "        \"height\": \"200\","
    "        \"backgroundColor\": \"green\""
    "      }"
    "    }"
    "  }"
    "}";

/**
 * Try to run a command that is missing a required property
 */
TEST_F(ExtensionCommandTest, MissingRequiredProperty)
{
    config->registerExtensionCommand(ExtensionCommandDefinition("aplext:Test", "MyCommand")
                                        .property("id", "NO_ID", true)   // Required property
                                        .property("value", 0, false)
                                        .allowFastMode(true)     // Allow running in fast mode
                                        .requireResolution(true) // Resolution isn't required in fast mode even if set
    );
    loadDocument(SCROLL_VIEW_BAD_COMMAND);

    ASSERT_TRUE(component);
    auto frame = component->findComponentById("MyFrame");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), frame->getCalculated(kPropertyBackgroundColor)));

    component->update(kUpdateScrollPosition, 50);  // Halfway
    loop->runPending();

    ASSERT_FALSE(root->hasEvent());  // No event is generated
    ASSERT_TRUE(ConsoleMessage());   // There should be a console message saying which property is missing

    // The SetValue command should have run
    ASSERT_TRUE(IsEqual(Color(Color::RED), frame->getCalculated(kPropertyBackgroundColor)));
}

/**
 * Run a command with missing properties, where those properties are not required.
 * Verify that the properties get assigned default values.
 */
TEST_F(ExtensionCommandTest, OptionalProperties)
{
    config->registerExtensionCommand(ExtensionCommandDefinition("aplext:Test", "MyCommand")
                                        .property("id", "NO_ID", false)
                                        .property("value", -1001, false)
                                        .allowFastMode(true)     // Allow running in fast mode
                                        .requireResolution(true) // Resolution isn't required in fast mode even if set
    );
    loadDocument(SCROLL_VIEW_BAD_COMMAND);

    ASSERT_TRUE(component);
    auto frame = component->findComponentById("MyFrame");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), frame->getCalculated(kPropertyBackgroundColor)));

    component->update(kUpdateScrollPosition, 50);  // Halfway
    loop->runPending();

    ASSERT_TRUE(root->hasEvent());  // An event is generated in fast mode
    ASSERT_FALSE(ConsoleMessage());   // No warning messages

    auto event = root->popEvent();
    ASSERT_TRUE(IsEqual("MyCommand", event.getValue(kEventPropertyName)));
    ASSERT_TRUE(IsEqual("aplext:Test", event.getValue(kEventPropertyExtensionURI)));

    auto source = event.getValue(kEventPropertySource);
    ASSERT_TRUE(source.isMap());
    ASSERT_TRUE(IsEqual("ScrollView", source.get("type")));

    auto ext = event.getValue(kEventPropertyExtension);
    ASSERT_TRUE(ext.isMap());
    ASSERT_TRUE(IsEqual(-1001, ext.get("value")));  // Expect the default value
    ASSERT_TRUE(IsEqual("NO_ID", ext.get("id")));   // Expect the default value

    ASSERT_TRUE(event.getActionRef().isEmpty());  // No action ref is generated in fast mode

    // The SetValue command should have run
    ASSERT_TRUE(IsEqual(Color(Color::RED), frame->getCalculated(kPropertyBackgroundColor)));
}

static const char *MULTIPLE_NAMES_FOR_SAME_COMMAND =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.3\","
    "  \"extensions\": ["
    "    {"
    "      \"name\": \"A\","
    "      \"uri\": \"URI1\""
    "    },"
    "    {"
    "      \"name\": \"B\","
    "      \"uri\": \"URI1\""
    "    }"
    "  ],"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"onPress\": ["
    "        {"
    "          \"type\": \"A:doIt\","
    "          \"value\": \"A\""
    "        },"
    "        {"
    "          \"type\": \"B:doIt\","
    "          \"value\": \"B\""
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(ExtensionCommandTest, MultipleNames)
{
    config->registerExtensionCommand(ExtensionCommandDefinition("URI1", "doIt")
                                        .property("value", "none", true));
    loadDocument(MULTIPLE_NAMES_FOR_SAME_COMMAND);

    ASSERT_TRUE(component);
    performTap(0, 0);
    loop->runPending();

    // The first event used namespace "A"
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    ASSERT_TRUE(IsEqual("doIt", event.getValue(kEventPropertyName)));
    ASSERT_TRUE(IsEqual("URI1", event.getValue(kEventPropertyExtensionURI)));
    auto ext = event.getValue(kEventPropertyExtension);
    ASSERT_TRUE(IsEqual("A", ext.get("value")));

    // The second event used namespace "B"
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();

    ASSERT_TRUE(IsEqual("doIt", event.getValue(kEventPropertyName)));
    ASSERT_TRUE(IsEqual("URI1", event.getValue(kEventPropertyExtensionURI)));
    ext = event.getValue(kEventPropertyExtension);
    ASSERT_TRUE(IsEqual("B", ext.get("value")));

    ASSERT_FALSE(root->hasEvent());
}
