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
#include "apl/engine/event.h"
#include "apl/extension/extensioncomponent.h"
#include "apl/primitives/object.h"

using namespace apl;

class ExtensionComponentTest : public DocumentWrapper {
};

// use "ExtensionComponent" to test the component independent of extension definition
static const char* DEFAULT_DOC = R"({
  "type": "APL",
  "version": "1.7",
  "extensions": [
    {
      "uri": "ext:cmp:1",
      "name": "Ext"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Ext:ExtensionComponent"
    }
  }
})";

/**
 * Test that the defaults are as expected when no values are set.
 */
TEST_F(ExtensionComponentTest, ComponentDefaults) {
    ExtensionComponentDefinition componentDef = ExtensionComponentDefinition("ext:cmp:1", "ExtensionComponent");
    config->registerExtensionComponent(componentDef);

    loadDocument(DEFAULT_DOC);
    ASSERT_EQ(kComponentTypeExtension, component->getType());
}

// Use extension component definition with context
static const char* DEFAULT_CONTEXT_DOC = R"({
  "type": "APL",
  "version": "1.7",
  "extensions": [
    {
      "uri": "ext:cmp:1",
      "name": "Draw"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Draw:DrawCanvas"
    }
  }
})";

/**
 * Tests that extension component returns "VISUAL_CONTEXT_TYPE_EMPTY" as default
 */
TEST_F(ExtensionComponentTest, ComponentVisualContextDefault) {
    ExtensionComponentDefinition componentDef = ExtensionComponentDefinition("ext:cmp:1", "DrawCanvas");
    config->registerExtensionComponent(componentDef);

    loadDocument(DEFAULT_CONTEXT_DOC);
    ASSERT_EQ(kComponentTypeExtension, component->getType());
    ASSERT_EQ(VISUAL_CONTEXT_TYPE_EMPTY, component->getVisualContextType());
}

/**
 * Tests that extension component returns context specified in component definition.
 */
TEST_F(ExtensionComponentTest, ComponentVisualContextGraphic) {
    ExtensionComponentDefinition componentDef = ExtensionComponentDefinition("ext:cmp:1", "DrawCanvas");
    std::string visualContext(VISUAL_CONTEXT_TYPE_GRAPHIC);
    componentDef.visualContextType(visualContext);
    config->registerExtensionComponent(componentDef);

    loadDocument(DEFAULT_CONTEXT_DOC);
    ASSERT_EQ(kComponentTypeExtension, component->getType());
    ASSERT_EQ(VISUAL_CONTEXT_TYPE_GRAPHIC, component->getVisualContextType());
}

// use "ExtensionComponent" to test the component independent of extension definition
// TODO add all values that are not defaults
static const char* NON_DEFAULT_DOC = R"({
  "type": "APL",
  "version": "1.7",
  "extensions": [
    {
      "uri": "ext:cmp:1",
      "name": "Ext"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Ext:ExtensionComponent"
    }
  }
})";

/**
 * Test the setting of all properties to non default values.
 */
TEST_F(ExtensionComponentTest, NonDefaults) {
    ExtensionComponentDefinition componentDef = ExtensionComponentDefinition("ext:cmp:1", "ExtensionComponent");
    config->registerExtensionComponent(componentDef);

    loadDocument(NON_DEFAULT_DOC);

    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeExtension, component->getType());
}

// use "ExtensionComponent" to test the component independent of extension definition
static const char* STYLED_DOC = R"({
  "type": "APL",
  "version": "1.7",
  "extensions": [
    {
      "uri": "ext:cmp:1",
      "name": "Ext"
    }
  ],
  "styles": {
    "myStyle": {
      "values": [
        {
      "backgroundColor": "yellow"
        }
      ]
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Ext:ExtensionComponent",
      "style": "myStyle"
    }
  }
})";

/**
 * Verify styled properties can be set via style, and non-styled properties cannot be set via style
 */
TEST_F(ExtensionComponentTest, Styled) {
    ExtensionComponentDefinition componentDef = ExtensionComponentDefinition("ext:cmp:1", "ExtensionComponent");
    config->registerExtensionComponent(componentDef);

    loadDocument(STYLED_DOC);

    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeExtension, component->getType());

    // all values are styled
    // TODO
}

// use "ExtensionComponent" to test the component independent of extension definition
static const char* SIMPLE = R"apl(
    {
      "type": "APL",
      "version": "1.7",
      "extensions": [
        {
          "uri": "ext:cmp:1",
          "name": "Ext"
        }
      ],
      "mainTemplate": {
        "items": {
          "type": "Ext:ExtensionComponent",
          "width": 400,
          "height": 400
        }
      }
    }
)apl";

/*
 * No pointer event without interaction mode.
 */
TEST_F(ExtensionComponentTest, NoPointerHandler) {
    ExtensionComponentDefinition componentDef = ExtensionComponentDefinition("ext:cmp:1", "ExtensionComponent");
    config->registerExtensionComponent(componentDef);

    loadDocument(SIMPLE);

    ASSERT_FALSE(component->isFocusable());
    ASSERT_FALSE(component->isTouchable());
    ASSERT_FALSE(MouseClick(root, 200, 200));
}

// Expects an extension component definition
static const char* EXTENSION_DOC = R"({
  "type": "APL",
  "version": "1.7",
  "extension": {
    "uri": "aplext:hello:10",
    "name": "Hello"
  },
  "mainTemplate": {
    "item": {
      "type": "Hello:Display"
    }
  }
})";

/**
 * Test the setting of all properties to non default values.
 */
TEST_F(ExtensionComponentTest, NamedExtensionComponent) {
    config->registerExtensionComponent(ExtensionComponentDefinition("aplext:hello:10", "Display"));

    loadDocument(EXTENSION_DOC);

    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeExtension, component->getType());
}

// Use extension component definition with command definition
static const char* COMPONENT_COMMAND_DOC = R"({
  "type": "APL",
  "version": "1.7",
  "extensions": [
    {
      "uri": "ext:cmp:1",
      "name": "Draw"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Draw:DrawCanvas",
      "onMount" : [
        {
          "type": "Draw:StartPaint"
        }
      ]
    }
  }
})";


/**
 * Tests that extension component commands is invoked with extension component as source and
 * default parameter value.
 */
TEST_F(ExtensionComponentTest, ComponentCommand) {
    ExtensionComponentDefinition componentDef =
        ExtensionComponentDefinition("ext:cmp:1", "DrawCanvas");
    std::map<std::string, ExtensionCommandDefinitionPtr> componentCommands;

    ExtensionCommandDefinition componentCommand("ext:cmp:1", "StartPaint");
    componentCommand.property("value", -1, false);
    componentDef.commands({componentCommand});

    config->registerExtensionCommand(componentCommand);
    config->registerExtensionComponent(componentDef);

    loadDocument(COMPONENT_COMMAND_DOC);
    ASSERT_EQ(kComponentTypeExtension, component->getType());

    ASSERT_TRUE(root->hasEvent());
    ASSERT_FALSE(ConsoleMessage());

    auto event = root->popEvent();
    ASSERT_TRUE(IsEqual("StartPaint", event.getValue(kEventPropertyName)));
    ASSERT_TRUE(IsEqual("ext:cmp:1", event.getValue(kEventPropertyExtensionURI)));

    auto ext = event.getValue(kEventPropertyExtension);
    ASSERT_TRUE(ext.isMap());
    ASSERT_TRUE(IsEqual(-1, ext.get("value")));

    auto source = event.getValue(kEventPropertySource);
    ASSERT_TRUE(source.isMap());
    ASSERT_TRUE(IsEqual("DrawCanvas", source.get("type")));
}

static const char* COMPONENT_COMMAND_WITH_VALUE_DOC = R"({
  "type": "APL",
  "version": "1.7",
  "extensions": [
    {
      "uri": "ext:cmp:1",
      "name": "Draw"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Draw:DrawCanvas",
      "onMount" : [
        {
          "type": "Draw:StartPaint",
          "value": 99
        }
      ]
    }
  }
})";

/**
 * Tests that extension component commands is invoked with extension component as source and
 * parameter value defined in doc.
 */
TEST_F(ExtensionComponentTest, ComponentCommandParameter) {
    ExtensionComponentDefinition componentDef =
        ExtensionComponentDefinition("ext:cmp:1", "DrawCanvas");

    ExtensionCommandDefinition componentCommand("ext:cmp:1", "StartPaint");
    componentCommand.property("value", -1, false);
    componentDef.commands({componentCommand});

    config->registerExtensionCommand(componentCommand);
    config->registerExtensionComponent(componentDef);

    loadDocument(COMPONENT_COMMAND_WITH_VALUE_DOC);
    ASSERT_EQ(kComponentTypeExtension, component->getType());
    ASSERT_TRUE(root->hasEvent());
    ASSERT_FALSE(ConsoleMessage());

    auto event = root->popEvent();
    auto ext = event.getValue(kEventPropertyExtension);
    ASSERT_TRUE(ext.isMap());
    ASSERT_TRUE(IsEqual(99, ext.get("value")));
}

TEST_F(ExtensionComponentTest, GetURIAndName) {
    config->registerExtensionComponent(ExtensionComponentDefinition("aplext:hello:10", "Display"));

    loadDocument(EXTENSION_DOC);

    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeExtension, component->getType());
    auto extensionComponent = dynamic_cast<ExtensionComponent*>(component.get());
    ASSERT_STREQ(extensionComponent->getUri().c_str(), "aplext:hello:10");
    ASSERT_STREQ(extensionComponent->name().c_str(), "Display");
}

TEST_F(ExtensionComponentTest, AddEventHandler) {
    ExtensionComponentDefinition componentDefinition = ExtensionComponentDefinition("ext:cmp:1", "DrawCanvas");
    ExtensionEventHandler eventHandler("ext:cmp:1", "extensionEvent");

    componentDefinition.addEventHandler(1, eventHandler);
    auto eventHandlers = componentDefinition.getEventHandlers();
    ASSERT_EQ(eventHandlers.size(), 1);
    ASSERT_STREQ(eventHandlers.at(1).getName().c_str(), "extensionEvent");
}

/**
 * Viewhost initiated resource state changes for the extension component are reflected in properties.
 */
TEST_F(ExtensionComponentTest, ResourceState) {
    config->registerExtensionComponent(ExtensionComponentDefinition("aplext:hello:10", "Display"));

    loadDocument(EXTENSION_DOC);

    // initial state of component is pending
    ASSERT_FALSE(ConsoleMessage());
    ASSERT_TRUE(IsEqual(kResourcePending, component->getCalculated(kPropertyResourceState)));

    // the viewhost is aware of the component, and is allocating the system resource
    component->updateResourceState(kResourceReady);
    ASSERT_FALSE(ConsoleMessage());
    ASSERT_FALSE(CheckDirty(component, kPropertyResourceState));
    ASSERT_TRUE(IsEqual(kResourceReady, component->getCalculated(kPropertyResourceState)));

    // the viewhost has destroyed the resource normally
    component->updateResourceState(kResourceReleased);
    ASSERT_FALSE(ConsoleMessage());
    ASSERT_FALSE(CheckDirty(component, kPropertyResourceState));
    ASSERT_TRUE(IsEqual(kResourceReleased, component->getCalculated(kPropertyResourceState)));

    // the viewhost has destroyed the resource abnormally
    component->updateResourceState(kResourceError);
    ASSERT_FALSE(ConsoleMessage());
    ASSERT_FALSE(CheckDirty(component, kPropertyResourceState));
    ASSERT_TRUE(IsEqual(kResourceError, component->getCalculated(kPropertyResourceState)));
}

TEST_F(ExtensionComponentTest, ResourceStateNotSupported) {

    const char* DOC = R"({
      "type": "APL",
      "version": "1.7",
      "mainTemplate": {
        "item": {
          "type": "Frame"
        }
      }
    })";
    loadDocument(DOC);

    // component does not support resource state updates
    component->updateResourceState(kResourceReleased);
    ASSERT_TRUE(LogMessage());
}

/**
* The component state is changed to error when the extension fails.
*/
TEST_F(ExtensionComponentTest, ExtensionError) {
    config->registerExtensionComponent(ExtensionComponentDefinition("aplext:hello:10", "Display"));
    loadDocument(EXTENSION_DOC);

    // initial state of component is pending
    ASSERT_FALSE(ConsoleMessage());
    ASSERT_TRUE(IsEqual(kResourcePending, component->getCalculated(kPropertyResourceState)));

    auto extensionComponent = dynamic_cast<ExtensionComponent*>(component.get());
    extensionComponent->extensionComponentFail(42, "extension failure");
    ASSERT_FALSE(ConsoleMessage());
    ASSERT_TRUE(CheckDirty(component, kPropertyResourceState));
    ASSERT_TRUE(IsEqual(kResourceError, component->getCalculated(kPropertyResourceState)));
}