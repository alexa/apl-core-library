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

class ExtensionHandlerTest : public DocumentWrapper {};

static const char *BASIC = R"({
  "type": "APL",
  "version": "1.2",
  "extensions": [
    {
      "name": "T",
      "uri": "aplext:Test"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "MyText"
    }
  },
  "T:onSetArguments": {
    "type": "SetValue",
    "componentId": "MyText",
    "property": "text",
    "value": "Hello"
  }
})";

/**
 * Don't register for the extension handler.  When the system tries to invoke it,
 * that should generate a error message on the log (not the console)
 */
TEST_F(ExtensionHandlerTest, BasicMissingHandler)
{
    loadDocument(BASIC);

    ASSERT_TRUE(component);

    root->invokeExtensionEventHandler("aplext:Test", "onSetArguments", {}, false);
    loop->runPending();
    ASSERT_TRUE(IsEqual("", component->getCalculated(kPropertyText).asString()));
}

/**
 * Register for the custom handler and invoke it.
 */
TEST_F(ExtensionHandlerTest, BasicWithHandler)
{
    config->registerExtensionEventHandler(ExtensionEventHandler{"aplext:Test", "onSetArguments"});
    loadDocument(BASIC);

    ASSERT_TRUE(component);

    root->invokeExtensionEventHandler("aplext:Test", "onSetArguments", {}, false);
    loop->runPending();

    ASSERT_TRUE(IsEqual("Hello", component->getCalculated(kPropertyText).asString()));
}

static const char *WITH_ARGUMENTS = R"({
  "type": "APL",
  "version": "1.2",
  "extensions": [
    {
      "name": "T",
      "uri": "aplext:Test"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "MyText"
    }
  },
  "T:onSetArguments": {
    "type": "SetValue",
    "componentId": "MyText",
    "property": "text",
    "value": "Hello ${a} ${b} ${event.a} ${event.b}"
  }
})";

/**
 * Provide arguments when invoking the custom handler and verify that those arguments
 * are passed through.
 */
TEST_F(ExtensionHandlerTest, WithArguments)
{
    config->registerExtensionEventHandler(ExtensionEventHandler{"aplext:Test", "onSetArguments"});
    loadDocument(WITH_ARGUMENTS);

    ASSERT_TRUE(component);

    root->invokeExtensionEventHandler("aplext:Test", "onSetArguments", {{"a", 2},
                                                                        {"b", "Hello"}}, false);
    loop->runPending();

    ASSERT_TRUE(IsEqual("Hello 2 Hello 2 Hello", component->getCalculated(kPropertyText).asString()));
}

static const char *IMPORT_TEST = R"({
  "type": "APL",
  "version": "1.2",
  "import": [
    {
      "name": "simple",
      "version": "1.0"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "MyText"
    }
  }
})";

static const char *IMPORT_TEST_PACKAGE = R"({
  "type": "APL",
  "version": "1.2",
  "extensions": [
    {
      "name": "T",
      "uri": "aplext:Test"
    }
  ],
  "T:onSetArguments": {
    "type": "SetValue",
    "componentId": "MyText",
    "property": "text",
    "value": "FromImport"
  }
})";

/**
 * Define a custom handler in an imported package
 */
TEST_F(ExtensionHandlerTest, ImportTest)
{
    config->registerExtensionEventHandler(ExtensionEventHandler{"aplext:Test", "onSetArguments"});

    loadDocumentWithPackage(IMPORT_TEST, IMPORT_TEST_PACKAGE);
    ASSERT_TRUE(component);

    root->invokeExtensionEventHandler("aplext:Test", "onSetArguments", {}, false);
    loop->runPending();

    ASSERT_TRUE(IsEqual("FromImport", component->getCalculated(kPropertyText).asString()));
}

static const char *IMPORT_TEST_OVERRIDE = R"({
  "type": "APL",
  "version": "1.2",
  "import": [
    {
      "name": "simple",
      "version": "1.0"
    }
  ],
  "extensions": [
    {
      "name": "T",
      "uri": "aplext:Test"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "MyText"
    }
  },
  "T:onSetArguments": {
    "type": "SetValue",
    "componentId": "MyText",
    "property": "text",
    "value": "FromMain"
  }
})";

/**
 * Override the imported package handler with a document handler
 */
TEST_F(ExtensionHandlerTest, ImportTestOverride)
{
    config->registerExtensionEventHandler(ExtensionEventHandler{"aplext:Test", "onSetArguments"});

    loadDocumentWithPackage(IMPORT_TEST_OVERRIDE, IMPORT_TEST_PACKAGE);
    ASSERT_TRUE(component);

    root->invokeExtensionEventHandler("aplext:Test", "onSetArguments", {}, false);
    loop->runPending();

    ASSERT_TRUE(IsEqual("FromMain", component->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(ConsoleMessage());
}

static const char *FAST_MODE = R"({
  "type": "APL",
  "version": "1.2",
  "extensions": [
    {
      "name": "T",
      "uri": "aplext:Test"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "MyText"
    }
  },
  "T:onSetArguments": [
    {
      "type": "SendEvent"
    },
    {
      "type": "SetValue",
      "componentId": "MyText",
      "property": "text",
      "value": "FromMain"
    }
  ]
})";

/**
 * Run the custom handler in fast mode
 */
TEST_F(ExtensionHandlerTest, FastMode)
{
    config->registerExtensionEventHandler(ExtensionEventHandler{"aplext:Test", "onSetArguments"});

    loadDocument(FAST_MODE);
    ASSERT_TRUE(component);

    root->invokeExtensionEventHandler("aplext:Test", "onSetArguments", {}, true);
    loop->runPending();

    ASSERT_TRUE(IsEqual("FromMain", component->getCalculated(kPropertyText).asString()));
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());
}

static const char *DUPLICATE_EXTENSION_NAME = R"({
  "type": "APL",
  "version": "1.2",
  "extensions": [
    {
      "name": "A",
      "uri": "test"
    },
    {
      "name": "B",
      "uri": "test"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "MyText"
    }
  },
  "A:onExecute": {
    "type": "SetValue",
    "componentId": "MyText",
    "property": "text",
    "value": "FromA"
  },
  "B:onExecute": {
    "type": "SetValue",
    "componentId": "MyText",
    "property": "text",
    "value": "FromB"
  }
})";

/**
 * Register the same extension twice under different names.  Only the last handler should execute.
 */
TEST_F(ExtensionHandlerTest, DuplicateExtensionName)
{
    config->registerExtensionEventHandler(ExtensionEventHandler{"test", "onExecute"});

    loadDocument(DUPLICATE_EXTENSION_NAME);
    ASSERT_TRUE(component);

    root->invokeExtensionEventHandler("test", "onExecute", {}, true);
    loop->runPending();

    ASSERT_TRUE(IsEqual("FromB", component->getCalculated(kPropertyText).asString()));
    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(ConsoleMessage());  // Should get warning about overwriting message
}

static const char *EXTENSION_ACCESSING_PAYLOAD = R"({
  "type": "APL",
  "version": "1.3",
  "extensions": [
    {
      "name": "A",
      "uri": "URI_A"
    }
  ],
  "mainTemplate": {
    "parameters": [
      "payload"
    ],
    "item": {
      "type": "Text",
      "id": "MyText",
      "text": "${payload.start}"
    }
  },
  "A:onExecute": {
    "type": "SetValue",
    "componentId": "MyText",
    "property": "text",
    "value": "${payload.end}"
  }
})";

/**
 * Verify that the extension handler can access the payload.
 */
TEST_F(ExtensionHandlerTest, ExtensionAccessingPayload)
{
    config->registerExtensionEventHandler(ExtensionEventHandler{"URI_A", "onExecute"});

    loadDocument(EXTENSION_ACCESSING_PAYLOAD, R"({"start": "START", "end": "END"})");
   // ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual("START", component->getCalculated(kPropertyText).asString()));

    root->invokeExtensionEventHandler("URI_A", "onExecute", {}, true);
    loop->runPending();

    ASSERT_TRUE(IsEqual("END", component->getCalculated(kPropertyText).asString()));
}

static const char *DOCUMENT_DEFINED_COMMAND = R"({
  "type": "APL",
  "version": "1.4",
  "extensions": [
    {
      "name": "MyMagicExtension",
      "uri": "aplext:magic:0"
    }
  ],
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "id": "root",
        "height": "100%",
        "width": "100%",
        "bind": [
          {
            "name": "NumScreenTouches",
            "value": 0,
            "type": "number"
          }
        ],
        "items": [
          {
            "type": "TouchWrapper",
            "height": "100%",
            "width": "100%",
            "onPress": [
              {
                "type": "IncrementTouches"
              }
            ],
            "item": {
              "type": "Text",
              "text": "${NumScreenTouches}"
            }
          }
        ]
      }
    ]
  },
  "commands": {
    "IncrementTouches": {
      "command": {
        "type": "SetValue",
        "componentId": "root",
        "property": "NumScreenTouches",
        "value": "${event.target.bind.NumScreenTouches + 1}"
      }
    }
  },
  "MyMagicExtension:OnPress": [
    {
      "type": "IncrementTouches"
    }
  ]
})";

/**
 * Verify that the extension handler can access document defined commands.
 */
TEST_F(ExtensionHandlerTest, ExtensionAccessingCommands)
{
    config->registerExtensionEventHandler(ExtensionEventHandler{"aplext:magic:0", "OnPress"});

    loadDocument(DOCUMENT_DEFINED_COMMAND);

    auto text = component->getChildAt(0)->getChildAt(0);

    ASSERT_EQ("0", text->getCalculated(kPropertyText).asString());

    root->invokeExtensionEventHandler("aplext:magic:0", "OnPress", {}, true);
    root->clearPending();

    ASSERT_EQ("1", text->getCalculated(kPropertyText).asString());

    performClick(0, 0);
    root->clearPending();

    ASSERT_EQ("2", text->getCalculated(kPropertyText).asString());
}
