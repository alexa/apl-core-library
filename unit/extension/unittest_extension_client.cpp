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

class ExtensionClientTest : public DocumentWrapper {
public:
    void createConfigAndClient(JsonData&& document) {
        createConfig(std::move(document));

        client = createClient("aplext:hello:10");
        ASSERT_FALSE(ConsoleMessage());
    }

    void createConfig(JsonData&& document) {
        configPtr = std::make_shared<RootConfig>();
        configPtr->agent("Unit tests", "1.0").timeManager(loop).session(session);
        content = Content::create(std::move(document), session);
        ASSERT_TRUE(content->isReady());
    }

    std::shared_ptr<ExtensionClient> createClient(const std::string& extension) {
        return ExtensionClient::create(configPtr, extension);
    }


    void initializeContext() {
        root = RootContext::create(metrics, content, *configPtr, createCallback);
        if (root) {
            context = root->contextPtr();
            ASSERT_TRUE(context);
            component = std::dynamic_pointer_cast<CoreComponent>(root->topComponent());
        }
    }

    std::shared_ptr<RootConfig> configPtr;
    std::shared_ptr<ExtensionClient> client;
    rapidjson::Document doc;

    void TearDown() override
    {
        client = nullptr;
        configPtr = nullptr;
        DocumentWrapper::TearDown();
    }
};

static const char* EXT_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "extension": {
    "uri": "aplext:hello:10",
    "name": "Hello"
  },
  "settings": {
    "Hello": {
      "authorizationCode": "MAGIC"
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": 500,
      "height": 500,
      "items": [
        {
          "type": "TouchWrapper",
          "id": "tw1",
          "width": 100,
          "heigth": 100,
          "onPress": [
            {
              "type": "Sequential",
              "commands" : [
                {
                  "type": "Hello:freeze",
                  "description": "Full parameters",
                  "foo": 128,
                  "bar": "push",
                  "baz": false
                },
                {
                  "type": "SendEvent",
                  "description": "Resolve checker."
                }
              ]
            }
          ],
          "item": {
              "type": "Frame",
              "backgroundColor": "red",
              "height": 100,
              "width": 100
          }
        },
        {
          "type": "TouchWrapper",
          "id": "tw2",
          "width": 100,
          "heigth": 100,
          "onPress": [
            {
              "type": "Hello:freeze",
              "description": "Missing required"
            }
          ],
          "item": {
              "type": "Frame",
              "backgroundColor": "blue",
              "height": 100,
              "width": 100
          }
        },
        {
          "type": "TouchWrapper",
          "id": "tw3",
          "width": 100,
          "heigth": 100,
          "onPress": [
            {
              "type": "Hello:freeze",
              "description": "Missing non-required",
              "foo": 128,
              "baz": false
            }
          ],
          "item": {
              "type": "Frame",
              "backgroundColor": "green",
              "height": 100,
              "width": 100
          }
        },
        {
          "type": "Text",
          "id": "label",
          "width": 100,
          "heigth": 100,
          "text": "Empty"
        }
      ]
    }
  },
  "Hello:onEntityChanged": [
    {
      "type": "SetValue",
      "componentId": "label",
      "property": "text",
      "value": "onEntityChanged:${entityList.length}"
    }
  ],
  "Hello:onEntityAdded": [
    {
      "type": "SetValue",
      "componentId": "label",
      "property": "text",
      "value": "onEntityAdded:${entityList.length}"
    },
    {
      "type": "SendEvent",
      "sequencer": "SEQ_ARR",
      "arguments": ["${event.current}"]
    }
  ],
  "Hello:onEntityRemoved": [
    {
      "type": "SetValue",
      "componentId": "label",
      "property": "text",
      "value": "onEntityRemoved:${entityList.length}"
    }
  ],
  "Hello:onDeviceUpdate": [
    {
      "type": "SetValue",
      "componentId": "label",
      "property": "text",
      "value": "onDeviceUpdate:${deviceState.alive}:${deviceState.position}:${deviceState.rotation}"
    },
    {
      "type": "SendEvent",
      "sequencer": "SEQ${changed.length}",
      "arguments": ["${event.current}", "${event.changed.length}"]
    }
  ],
  "Hello:onGenericExternallyComingEvent": [
    {
      "type": "SetValue",
      "componentId": "label",
      "property": "text",
      "value": "onGenericExternallyComingEvent:${event.potatos}"
    }
  ]
})";

static const char* REGISTER_HEADER = R"({
  "method": "RegisterSuccess",
  "version": "1.0",
  "token": "TOKEN",
  "extension": "aplext:hello:10",
  "schema":
)";

static const char* REGISTER_FAILURE = R"({
  "method": "RegisterFailure",
  "version": "1.0"
})";

static const char* WRONG_MESSAGE = R"({
  "method": "Potato"
})";

TEST_F(ExtensionClientTest, ExtensionParseRequiredMalFormed) {
    const std::string HEADER = REGISTER_HEADER;
    auto configPtr = std::make_shared<RootConfig>();
    configPtr->session(session);
    auto client = ExtensionClient::create(configPtr, "aplext:hello:10");
    ASSERT_FALSE(client->processMessage(nullptr, R"()"));
    ASSERT_TRUE(ConsoleMessage());

    ASSERT_FALSE(client->processMessage(nullptr, R"({})"));
    ASSERT_TRUE(ConsoleMessage());

    ASSERT_FALSE(client->processMessage(nullptr, HEADER + R"({})" + "}"));
    ASSERT_TRUE(ConsoleMessage());

    ASSERT_FALSE(client->processMessage(nullptr, HEADER + R"({"type":"foo", "version":3})" + "}"));
    ASSERT_TRUE(ConsoleMessage());

    ASSERT_FALSE(client->processMessage(nullptr, HEADER + R"({"type":"Schema", "version":"bar"})" + "}"));
    ASSERT_TRUE(ConsoleMessage());

    ASSERT_FALSE(client->processMessage(nullptr, HEADER + R"({"type":"Schema", "version":"1.4"})" + "}"));
    ASSERT_TRUE(ConsoleMessage());

    ASSERT_FALSE(client->processMessage(nullptr, HEADER + R"({"type":"Schema", "version":"1.0"})" + "}"));
    ASSERT_TRUE(ConsoleMessage());

    ASSERT_FALSE(client->processMessage(nullptr, HEADER + R"({"type":"Schema", "version":"1.0", "uri":2})" + "}"));
    ASSERT_TRUE(ConsoleMessage());
}

static const char* EXTENSION_SIMPLE = R"({
  "method": "RegisterSuccess",
  "version": "1.0",
  "token": "TOKEN-12",
  "extension": "aplext:hello:10",
  "schema": {"type":"Schema", "version":"1.0", "uri":"aplext:hello:10"}
})";

TEST_F(ExtensionClientTest, ExtensionParseRequired) {
    createConfigAndClient(EXT_DOC);
    ASSERT_TRUE(client->processMessage(nullptr, EXTENSION_SIMPLE));
    ASSERT_FALSE(ConsoleMessage());
    ASSERT_TRUE(client->registrationMessageProcessed());
    ASSERT_EQ("TOKEN-12", client->getConnectionToken());
    auto ext = configPtr->getSupportedExtensions();
    ASSERT_EQ(1, ext.size());
    ASSERT_NE(ext.end(), ext.find("aplext:hello:10"));
}

static const char* EXTENSION_DEFERRED = R"({
  "method": "RegisterSuccess",
  "version": "1.0",
  "token": "<AUTO_TOKEN>",
  "extension": "aplext:hello:10",
  "schema": {"type":"Schema", "version":"1.0", "uri":"aplext:hello:10"}
})";

TEST_F(ExtensionClientTest, ExtensionParseAutoToken){
    createConfigAndClient(EXT_DOC);
    ASSERT_TRUE(client->processMessage(nullptr, EXTENSION_DEFERRED));
    ASSERT_FALSE(ConsoleMessage());
    ASSERT_TRUE(client->registrationMessageProcessed());
    const auto &token = client->getConnectionToken();
    ASSERT_EQ(0, token.rfind("aplext:hello:10", 0));
    auto ext = configPtr->getSupportedExtensions();
    ASSERT_EQ(1, ext.size());
    ASSERT_NE(ext.end(), ext.find("aplext:hello:10"));
}


static const char* EXTENSION_DEFINITION = R"(
  "method": "RegisterSuccess",
  "version": "1.0",
  "token": "TOKEN",
  "extension": "aplext:hello:10",
  "schema": {
    "type":"Schema",
    "version":"1.0",
    "uri":"aplext:hello:10",
)";

static const char* EXTENSION_TYPES = R"(
"types": [
  {
    "name": "FreezePayload",
    "properties": {
      "foo": {
        "type": "number",
        "required": true,
        "default": 64
      },
      "bar": {
        "type": "string",
        "required": false,
        "default": "boom"
      },
      "baz": {
        "type": "boolean",
        "required": true,
        "default": true
      }
    }
  },
  {
    "name": "Entity",
    "properties": {
      "alive": "boolean",
      "position": "string"
    }
  },
  {
    "name": "DeviceState",
    "properties": {
      "alive": "boolean",
      "rotation": "float"
    }
  }
],)";

static const char* EXTENSION_COMMANDS = R"(
  "commands": [
    {
      "name": "follow"
    },
    {
      "name": "lead",
      "requireResponse": true,
      "allowFastMode":  false
    },
    {
      "name": "freeze",
      "requireResponse": false,
      "allowFastMode": true,
      "payload": "FreezePayload"
    },
    {
      "name": "clipEntity",
      "requireResponse": false,
      "allowFastMode": true,
      "payload": {
        "type": "FreezePayload",
        "description": "Don't really care about this property."
      }
    }
  ]
)";

static const char* EXTENSION_COMMANDS_BLANK_PROPS = R"(
  "commands": [
    {
      "name": "follow"
    },
    {
      "name": "lead",
      "requireResponse": "true"
    },
    {
      "name": "freeze",
      "requireResponse": false
)";

static const char* EXTENSION_COMMANDS_BLANK_PROPS_END = R"(
    }
  ]
)";

static const char* EXTENSION_EVENTS = R"(
"events": [
    {"name": "onEntityAdded"},
    {"name": "onEntityChanged"},
    {"name": "onEntityLost"}
  ]
)";


TEST_F(ExtensionClientTest, ExtensionParseCommandsMalformed) {
    createConfigAndClient(EXT_DOC);
    std::string doc = "{";
    doc += EXTENSION_DEFINITION;
    doc += R"("commands":"nogood")";
    doc += "}}";


    ASSERT_TRUE(client->processMessage(nullptr, doc));
    ASSERT_TRUE(ConsoleMessage());

    createConfigAndClient(EXT_DOC);
    std::string doc2 = "{";
    doc2 += EXTENSION_DEFINITION;
    doc2 += R"("commands":[])";
    doc2 += "}}";

    ASSERT_TRUE(client->processMessage(nullptr, doc2));
    ASSERT_FALSE(ConsoleMessage());

    createConfigAndClient(EXT_DOC);
    std::string doc3 = "{";
    doc3 += EXTENSION_DEFINITION;
    doc3 += R"("commands":[{"nope":"nope"}])";
    doc3 += "}}";

    ASSERT_TRUE(client->processMessage(nullptr, doc3));
    ASSERT_TRUE(ConsoleMessage());

    createConfigAndClient(EXT_DOC);
    std::string doc4 = "{";
    doc3 += EXTENSION_DEFINITION;
    doc3 += R"("commands":[{"name":4}])";
    doc3 += "}}";

    ASSERT_FALSE(client->processMessage(nullptr, doc4));
    ASSERT_TRUE(ConsoleMessage());
}


TEST_F(ExtensionClientTest, ExtensionParseCommandsMalformedPropertis) {
    createConfigAndClient(EXT_DOC);

    // empty props
    std::string doc = "{";
    doc += EXTENSION_DEFINITION;
    doc += EXTENSION_TYPES;
    doc += EXTENSION_COMMANDS_BLANK_PROPS;
    doc += EXTENSION_COMMANDS_BLANK_PROPS_END;
    doc += "}}";

    ASSERT_TRUE(client->processMessage(nullptr, doc));
    ASSERT_FALSE(ConsoleMessage());

    createConfigAndClient(EXT_DOC);
    // invalid name
    std::string doc0 = "{";
    doc0 += EXTENSION_DEFINITION;
    doc0 += EXTENSION_TYPES;
    doc0 += EXTENSION_COMMANDS_BLANK_PROPS;
    doc0 += R"({"payload": 2})";
    doc0 += EXTENSION_COMMANDS_BLANK_PROPS_END;
    doc0 += "}}";

    ASSERT_FALSE(client->processMessage(nullptr, doc0));
    ASSERT_TRUE(ConsoleMessage());

    createConfigAndClient(EXT_DOC);
    // missing type
    std::string doc1 = "{";
    doc1 += EXTENSION_DEFINITION;
    doc1 += EXTENSION_TYPES;
    doc1 += EXTENSION_COMMANDS_BLANK_PROPS;
    doc1 += R"({"payload": "foo"})";
    doc1 += EXTENSION_COMMANDS_BLANK_PROPS_END;
    doc1 += "}}";

    ASSERT_FALSE(client->processMessage(nullptr, doc1));
    ASSERT_TRUE(ConsoleMessage());
}


TEST_F(ExtensionClientTest, ExtensionParseCommands) {
    createConfigAndClient(EXT_DOC);

    std::string doc = "{";
    doc += EXTENSION_DEFINITION;
    doc += EXTENSION_TYPES;
    doc += EXTENSION_COMMANDS;
    doc += "}}";

    ASSERT_TRUE(client->processMessage(nullptr, doc));
    ASSERT_FALSE(ConsoleMessage());
    auto commands = configPtr->getExtensionCommands();
    ASSERT_EQ(4, commands.size());

    ASSERT_EQ("aplext:hello:10", commands[0].getURI());
    ASSERT_EQ("follow", commands[0].getName());
    ASSERT_FALSE(commands[0].getRequireResolution());
    ASSERT_FALSE(commands[0].getAllowFastMode());
    ASSERT_TRUE(commands[0].getPropertyMap().empty());

    ASSERT_EQ("aplext:hello:10", commands[1].getURI());
    ASSERT_EQ("lead", commands[1].getName());
    ASSERT_TRUE(commands[1].getRequireResolution());
    ASSERT_FALSE(commands[1].getAllowFastMode());
    ASSERT_TRUE(commands[1].getPropertyMap().empty());

    ASSERT_EQ("aplext:hello:10", commands[2].getURI());
    ASSERT_EQ("freeze", commands[2].getName());
    ASSERT_TRUE(commands[2].getAllowFastMode());
    ASSERT_FALSE(commands[2].getRequireResolution());

    auto props = commands[2].getPropertyMap();
    ASSERT_EQ(3, props.size());
    ASSERT_TRUE(IsEqual(true,props.at("foo").required));
    ASSERT_TRUE(IsEqual(64, props.at("foo").defvalue));
    ASSERT_TRUE(IsEqual(false, props.at("bar").required));
    ASSERT_TRUE(IsEqual("boom", props.at("bar").defvalue));
    ASSERT_TRUE(IsEqual(true, props.at("baz").required));
    ASSERT_TRUE(IsEqual(true, props.at("baz").defvalue));

    ASSERT_EQ("aplext:hello:10", commands[3].getURI());
    ASSERT_EQ("clipEntity", commands[3].getName());
    ASSERT_FALSE(commands[3].getRequireResolution());
    ASSERT_TRUE(commands[3].getAllowFastMode());

    props = commands[3].getPropertyMap();
    ASSERT_EQ(3, props.size());
    ASSERT_TRUE(IsEqual(true,props.at("foo").required));
    ASSERT_TRUE(IsEqual(64, props.at("foo").defvalue));
    ASSERT_TRUE(IsEqual(false, props.at("bar").required));
    ASSERT_TRUE(IsEqual("boom", props.at("bar").defvalue));
    ASSERT_TRUE(IsEqual(true, props.at("baz").required));
    ASSERT_TRUE(IsEqual(true, props.at("baz").defvalue));
}


TEST_F(ExtensionClientTest, ExtensionParseEventHandlersMalformed) {
    createConfigAndClient(EXT_DOC);
    std::string doc = "{";
    doc += EXTENSION_DEFINITION;
    doc += EXTENSION_TYPES;
    doc += R"("events":"nogood")";
    doc += "}}";

    ASSERT_FALSE(client->processMessage(nullptr, doc));
    ASSERT_TRUE(ConsoleMessage());

    createConfigAndClient(EXT_DOC);
    std::string doc2 = "{";
    doc2 += EXTENSION_DEFINITION;
    doc2 += EXTENSION_TYPES;
    doc2 += R"("events":[])";
    doc2 += "}}";

    ASSERT_TRUE(client->processMessage(nullptr, doc2));
    ASSERT_FALSE(ConsoleMessage());

    createConfigAndClient(EXT_DOC);
    std::string doc3 = "{";
    doc3 += EXTENSION_DEFINITION;
    doc3 += EXTENSION_TYPES;
    doc3 += R"("events":[{"nope":"nope"}])";
    doc3 += "}}";

    ASSERT_FALSE(client->processMessage(nullptr, doc3));
    ASSERT_TRUE(ConsoleMessage());

    createConfigAndClient(EXT_DOC);
    std::string doc4 = "{";
    doc3 += EXTENSION_DEFINITION;
    doc3 += EXTENSION_TYPES;
    doc3 += R"("events":[{"name":4}])";
    doc3 += "}}";

    ASSERT_FALSE(client->processMessage(nullptr, doc4));
    ASSERT_TRUE(ConsoleMessage());
}


TEST_F(ExtensionClientTest, ExtensionParseEventHandlers) {
    createConfigAndClient(EXT_DOC);

    std::string doc = "{";
    doc += EXTENSION_DEFINITION;
    doc += EXTENSION_TYPES;
    doc += EXTENSION_EVENTS;
    doc += "}}";

    ASSERT_TRUE(client->processMessage(nullptr, doc));
    ASSERT_FALSE(ConsoleMessage());

    auto ext = configPtr->getSupportedExtensions();
    ASSERT_EQ(1, ext.size());
    auto ex = ext.find("aplext:hello:10");
    ASSERT_NE(ext.end(), ex);

    auto handlers = configPtr->getExtensionEventHandlers();
    ASSERT_EQ(3, handlers.size());
    ASSERT_EQ("aplext:hello:10", handlers[0].getURI());
    ASSERT_EQ("onEntityAdded", handlers[0].getName());
    ASSERT_EQ("aplext:hello:10", handlers[1].getURI());
    ASSERT_EQ("onEntityChanged", handlers[1].getName());
    ASSERT_EQ("aplext:hello:10", handlers[2].getURI());
    ASSERT_EQ("onEntityLost", handlers[2].getName());
}

static const char* EXTENSION_DATA_BINDINGS = R"(
"liveData": [
    {"name": "entityList", "type": "Entity[]"},
    {"name": "deviceState", "type": "DeviceState"}
  ]
)";

TEST_F(ExtensionClientTest, ExtensionParseEventDataBindings) {
    createConfigAndClient(EXT_DOC);

    std::string doc = "{";
    doc += EXTENSION_DEFINITION;
    doc += EXTENSION_TYPES;
    doc += EXTENSION_DATA_BINDINGS;
    doc += "}}";

    ASSERT_TRUE(client->processMessage(nullptr, doc));
    ASSERT_FALSE(ConsoleMessage());

    auto ext = configPtr->getSupportedExtensions();
    ASSERT_EQ(1, ext.size());
    auto ex = ext.find("aplext:hello:10");
    ASSERT_NE(ext.end(), ex);

    auto liveDataMap = configPtr->getLiveObjectMap();
    ASSERT_EQ(2, liveDataMap.size());
    auto arr = liveDataMap.at("entityList");
    auto map = liveDataMap.at("deviceState");
    ASSERT_EQ(Object::ObjectType::kArrayType, arr->getType());
    ASSERT_EQ(Object::ObjectType::kMapType, map->getType());
}

static const char* EXT_REGISTER_SUCCESS = R"({
  "method": "RegisterSuccess",
  "version": "1.0",
  "token": "TOKEN",
  "environment": {
    "something": "additional"
  },
  "schema": {
    "type": "Schema",
    "version": "1.0",
    "uri": "aplext:hello:10",
    "types": [
      {
        "name": "Entity",
        "properties": {
          "alive": "boolean",
          "position": "string"
        }
      },
      {
        "name": "FreezePayload",
        "properties": {
          "foo": {
            "type": "number",
            "required": true,
            "default": 64
          },
          "bar": {
            "type": "string",
            "required": false,
            "default": "boom"
          },
          "baz": {
            "type": "boolean",
            "required": true,
            "default": true
          },
          "entity": {
            "type": "Entity",
            "description": "Some non-required object reference"
          }
        }
      },
      {
        "name": "DeviceState",
        "properties": {
          "alive": {
            "type": "boolean",
            "required": true,
            "default": true
          },
          "rotation": "float",
          "collapsed1": "boolean",
          "collapsed2": "boolean",
          "uncollapsed": "boolean"
        }
      }
    ],
    "commands": [
      {
        "name": "freeze",
        "requireResponse": true,
        "payload": "FreezePayload"
      }
    ],
    "events": [
      { "name": "onEntityAdded" },
      { "name": "onEntityChanged" },
      { "name": "onEntityLost" },
      { "name": "onDeviceUpdate" },
      { "name": "onDeviceRemove" },
      { "name": "onGenericExternallyComingEvent", "mode": "NORMAL" }
    ],
    "liveData": [
      {
        "name": "entityList",
        "type": "Entity[]",
        "events": {
          "add": {
            "eventHandler": "onEntityAdded"
          },
          "update": {
            "eventHandler": "onEntityChanged"
          }
        }
      },
      {
        "name": "deviceState",
        "type": "DeviceState",
        "events": {
          "set": {
            "eventHandler": "onDeviceUpdate",
            "properties": [
              {
                "name": "*",
                "update": false
              },
              {
                "name": "alive",
                "update": true
              },
              {
                "name": "collapsed1",
                "update": true,
                "collapse": true
              },
              {
                "name": "collapsed2",
                "update": true
              },
              {
                "name": "uncollapsed",
                "update": true,
                "collapse": false
              }
            ]
          },
          "remove": {
            "eventHandler": "onDeviceRemove",
            "properties": [
              {
                "name": "*",
                "update": false
              },
              {
                "name": "alive",
                "update": true
              },
              {
                "name": "collapsed1",
                "update": true,
                "collapse": true
              },
              {
                "name": "collapsed2",
                "update": true
              },
              {
                "name": "uncollapsed",
                "update": true,
                "collapse": false
              }
            ]
          }
        }
      }
    ]
  }
})";

static const char* EXT_EVENT = R"({
    "version": "1.0",
    "method": "Event",
    "target": "aplext:hello:10",
    "name": "onGenericExternallyComingEvent",
    "payload": { "potatos": "exactly" }
})";

static const char* EXT_COMMAND_SUCCESS_HEADER = R"({
    "version": "1.0",
    "method": "CommandSuccess",
)";

static const char* EXT_COMMAND_FAILURE_HEADER = R"({
    "version": "1.0",
    "method": "CommandFailure",
)";

static const char* ENTITY_LIST_INSERT = R"({
  "version": "1.0",
  "method": "LiveDataUpdate",
  "name": "entityList",
  "target": "aplext:hello:10",
  "operations": [
    {
      "type": "Insert",
      "index": 0,
      "item": 2
    },
    {
      "type": "Insert",
      "index": 0,
      "item": 1
    },
    {
      "type": "Insert",
      "index": 0,
      "item": 0
    }
  ]
})";

static const char* ENTITY_LIST_INSERT_RANGE = R"({
  "version": "1.0",
  "method": "LiveDataUpdate",
  "name": "entityList",
  "target": "aplext:hello:10",
  "operations": [
    {
      "type": "Insert",
      "index": 0,
      "item": [101, 102, 103]
    }
  ]
})";

static const char* ENTITY_LIST_UPDATE = R"({
  "version": "1.0",
  "method": "LiveDataUpdate",
  "name": "entityList",
  "target": "aplext:hello:10",
  "operations": [
    {
      "type": "Update",
      "index": 0,
      "item": 10
    }
  ]
})";

static const char* ENTITY_LIST_REMOVE = R"({
  "version": "1.0",
  "method": "LiveDataUpdate",
  "name": "entityList",
  "target": "aplext:hello:10",
  "operations": [
    {
      "type": "Remove",
      "index": 0
    }
  ]
})";

static const char* ENTITY_LIST_CLEAR = R"({
  "version": "1.0",
  "method": "LiveDataUpdate",
  "name": "entityList",
  "target": "aplext:hello:10",
  "operations": [
    {
      "type": "Clear"
    }
  ]
})";

static const char* ENTITY_MAP_SET_DEAD = R"({
  "version": "1.0",
  "method": "LiveDataUpdate",
  "name": "deviceState",
  "target": "aplext:hello:10",
  "operations": [
    {
      "type": "Set",
      "key": "alive",
      "item": false
    }
  ]
})";

static const char* ENTITY_MAP_SET_ALIVE = R"({
  "version": "1.0",
  "method": "LiveDataUpdate",
  "name": "deviceState",
  "target": "aplext:hello:10",
  "operations": [
    {
      "type": "Set",
      "key": "alive",
      "item": true
    }
  ]
})";

static const char* ENTITY_MAP_SET_POSITION_AND_ROTATION = R"({
  "version": "1.0",
  "method": "LiveDataUpdate",
  "name": "deviceState",
  "target": "aplext:hello:10",
  "operations": [
    {
      "type": "Set",
      "key": "position",
      "item": "pos"
    },
    {
      "type": "Set",
      "key": "rotation",
      "item": 7.9
    }
  ]
})";

static const char* ENTITY_MAP_REMOVE_FAIL = R"({
  "version": "1.0",
  "method": "LiveDataUpdate",
  "name": "deviceState",
  "target": "aplext:hello:10",
  "operations": [
    {
      "type": "Remove",
      "key": "position"
    },
    {
      "type": "Remove",
      "key": "position"
    }
  ]
})";

static const char* ENTITY_LIST_UPDATE_FAIL = R"({
  "version": "1.0",
  "method": "LiveDataUpdate",
  "name": "entityList",
  "target": "aplext:hello:10",
  "operations": [
    {
      "type": "Update",
      "index": 10,
      "item": 110
    }
  ]
})";

TEST_F(ExtensionClientTest, ExtensionLifecycle) {
    createConfigAndClient(EXT_DOC);

    // Check what document wants.
    auto extRequests = content->getExtensionRequests();
    ASSERT_EQ(1, extRequests.size());
    auto extRequest = *extRequests.begin();
    ASSERT_EQ("aplext:hello:10", extRequest);
    auto extSettings = content->getExtensionSettings(extRequest);
    ASSERT_TRUE(extSettings.has("authorizationCode"));

    // Pass request and settings to connection request creation.
    auto connectionRequest = client->createRegistrationRequest(doc.GetAllocator(), *content);
    ASSERT_STREQ("aplext:hello:10", connectionRequest["uri"].GetString());

    // We assume that connection request will return Schema affected with passed settings and will contain all rules
    // required including liveData updates. We don't really need to verify this settings per se.

    // Runtime asked for connection. Process Schema message
    ASSERT_TRUE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS));
    ASSERT_FALSE(ConsoleMessage());

    // We have all we need. Inflate.
    initializeContext();

    auto text = component->findComponentById("label");
    ASSERT_EQ(kComponentTypeText, text->getType());

    // Tap happened!
    performTap(1,1);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    // Runtime needs to redirect this events to the server.
    auto processedCommand = client->processCommand(doc.GetAllocator(), event);
    ASSERT_STREQ("Command", processedCommand["method"].GetString());

    // Resolve a response
    std::string commandResponse = EXT_COMMAND_SUCCESS_HEADER;
    commandResponse += R"("id": )" + std::to_string(processedCommand["id"].GetDouble()) + R"(,)";
    commandResponse += R"("result": true })";
    ASSERT_TRUE(client->processMessage(root, commandResponse));
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());

    // Event comes up from service to be intercepted and directed to client by runtime
    ASSERT_TRUE(client->processMessage(root, EXT_EVENT));
    ASSERT_EQ("onGenericExternallyComingEvent:exactly", text->getCalculated(kPropertyText).asString());

    // Live data updates
    ASSERT_TRUE(client->processMessage(root, ENTITY_LIST_INSERT));
    root->clearPending();
    root->popEvent();
    ASSERT_EQ("onEntityAdded:3", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(client->processMessage(root, ENTITY_LIST_UPDATE));
    root->clearPending();
    ASSERT_EQ("onEntityChanged:3", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(client->processMessage(root, ENTITY_MAP_SET_DEAD));
    root->clearPending();
    root->popEvent();
    ASSERT_EQ("onDeviceUpdate:false::", text->getCalculated(kPropertyText).asString());
}

TEST_F(ExtensionClientTest, CommandResolve) {
    createConfigAndClient(EXT_DOC);

    ASSERT_TRUE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS));
    ASSERT_TRUE(client->registrationMessageProcessed());
    ASSERT_TRUE(client->registered());
    ASSERT_FALSE(ConsoleMessage());

    // We have all we need. Inflate.
    initializeContext();

    auto text = component->findComponentById("label");
    ASSERT_EQ(kComponentTypeText, text->getType());

    // Tap happened!
    performTap(1,1);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    // Runtime needs to redirect this events to the server.
    auto processedCommand = client->processCommand(doc.GetAllocator(), event);
    ASSERT_STREQ("Command", processedCommand["method"].GetString());

    // Resolve a response
    std::string commandResponse = EXT_COMMAND_SUCCESS_HEADER;
    commandResponse += R"("id": )" + std::to_string(processedCommand["id"].GetDouble()) + R"(,)";
    commandResponse += R"("result": true })";
    ASSERT_TRUE(client->processMessage(root, commandResponse));
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
}

TEST_F(ExtensionClientTest, CommandResolveWrong) {
    createConfigAndClient(EXT_DOC);

    ASSERT_TRUE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS));
    ASSERT_TRUE(client->registrationMessageProcessed());
    ASSERT_TRUE(client->registered());
    ASSERT_FALSE(ConsoleMessage());

    // We have all we need. Inflate.
    initializeContext();

    auto text = component->findComponentById("label");
    ASSERT_EQ(kComponentTypeText, text->getType());

    // Tap happened!
    performTap(1,1);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    // Runtime needs to redirect this events to the server.
    auto processedCommand = client->processCommand(doc.GetAllocator(), event);
    ASSERT_STREQ("Command", processedCommand["method"].GetString());

    // Resolve a response
    std::string commandResponse = EXT_COMMAND_SUCCESS_HEADER;
    commandResponse += R"("id": 11111,)";
    commandResponse += R"("result": true })";
    ASSERT_FALSE(client->processMessage(root, commandResponse));
    ASSERT_TRUE(ConsoleMessage());

    root->cancelExecution();
}

TEST_F(ExtensionClientTest, CommandInterruptedResolve) {
    createConfigAndClient(EXT_DOC);

    ASSERT_TRUE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS));
    ASSERT_TRUE(client->registrationMessageProcessed());
    ASSERT_TRUE(client->registered());
    ASSERT_FALSE(ConsoleMessage());

    // We have all we need. Inflate.
    initializeContext();

    auto text = component->findComponentById("label");
    ASSERT_EQ(kComponentTypeText, text->getType());

    // Tap happened!
    performTap(1,1);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    // Runtime needs to redirect this events to the server.
    auto processedCommand = client->processCommand(doc.GetAllocator(), event);
    ASSERT_STREQ("Command", processedCommand["method"].GetString());

    // Event comes up from service to be intercepted and directed to client by runtime
    ASSERT_TRUE(client->processMessage(root, EXT_EVENT));
    ASSERT_EQ("onGenericExternallyComingEvent:exactly", text->getCalculated(kPropertyText).asString());

    // Resolve a response
    std::string commandResponse = EXT_COMMAND_SUCCESS_HEADER;
    commandResponse += R"("id": )" + std::to_string(processedCommand["id"].GetDouble()) + R"(,)";
    commandResponse += R"("result": true })";
    ASSERT_TRUE(client->processMessage(root, commandResponse));
    ASSERT_FALSE(ConsoleMessage());
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(ExtensionClientTest, RegistrationRequest) {
    createConfigAndClient(EXT_DOC);

    // Pass request and settings to connection request creation.
    auto connectionRequest = client->createRegistrationRequest(doc.GetAllocator(), *content);
    ASSERT_STREQ("1.0", connectionRequest["version"].GetString());
    ASSERT_STREQ("Register", connectionRequest["method"].GetString());
    ASSERT_STREQ("aplext:hello:10", connectionRequest["uri"].GetString());
    auto& connRequestSettings = connectionRequest["settings"];
    ASSERT_TRUE(connRequestSettings.HasMember("authorizationCode"));
}

TEST_F(ExtensionClientTest, Registered) {
    createConfigAndClient(EXT_DOC);

    ASSERT_TRUE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS));
    ASSERT_FALSE(ConsoleMessage());
    ASSERT_TRUE(client->registrationMessageProcessed());
    ASSERT_TRUE(client->registered());

    auto ext = configPtr->getSupportedExtensions();
    ASSERT_EQ(1, ext.size());
    auto ex = ext.find("aplext:hello:10");
    ASSERT_NE(ext.end(), ex);

    auto env = configPtr->getExtensionEnvironment("aplext:hello:10");
    ASSERT_TRUE(env.has("something"));
    ASSERT_EQ("additional", env.get("something").asString());

    auto commands = configPtr->getExtensionCommands();
    ASSERT_EQ(1, commands.size());
    auto freeze = commands.at(0);
    ASSERT_EQ("freeze", freeze.getName());
    ASSERT_TRUE(freeze.getRequireResolution());
    const auto& freezeParams = freeze.getPropertyMap();
    ASSERT_EQ(4, freezeParams.size());
    ASSERT_TRUE(freezeParams.count("foo"));
    ASSERT_TRUE(freezeParams.count("bar"));
    ASSERT_TRUE(freezeParams.count("baz"));
    ASSERT_TRUE(freezeParams.count("entity"));

    auto events = configPtr->getExtensionEventHandlers();
    ASSERT_EQ(6, events.size());
    auto event = events.at(0);
    ASSERT_EQ("onEntityAdded", event.getName());

    auto liveData = configPtr->getLiveObjectMap();
    ASSERT_EQ(2, liveData.size());
    ASSERT_TRUE(liveData.count("entityList"));
    ASSERT_TRUE(liveData.count("deviceState"));
}

TEST_F(ExtensionClientTest, NotRegistered) {
    createConfigAndClient(EXT_DOC);
    initializeContext();

    // Never registered. Should fail.
    ASSERT_FALSE(client->processMessage(root, EXT_EVENT));
    ASSERT_FALSE(client->registrationMessageProcessed());
    ASSERT_FALSE(client->registered());
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(ExtensionClientTest, BadMessage) {
    createConfigAndClient(EXT_DOC);

    ASSERT_TRUE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS));
    ASSERT_TRUE(client->registrationMessageProcessed());
    ASSERT_TRUE(client->registered());
    ASSERT_FALSE(ConsoleMessage());

    initializeContext();

    // Bad message
    ASSERT_FALSE(client->processMessage(root, WRONG_MESSAGE));
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(ExtensionClientTest, RegisterFailure) {
    createConfigAndClient(EXT_DOC);
    initializeContext();

    ASSERT_TRUE(client->processMessage(root, REGISTER_FAILURE));
    ASSERT_TRUE(client->registrationMessageProcessed());
    ASSERT_FALSE(client->registered());
    ASSERT_FALSE(ConsoleMessage());
}

TEST_F(ExtensionClientTest, OrderOfOperation) {
    createConfigAndClient(EXT_DOC);

    // We can't really do any messages before registration. So try some funky type. Should fail.
    ASSERT_FALSE(client->processMessage(nullptr, EXT_EVENT));
    ASSERT_TRUE(ConsoleMessage());

    ASSERT_TRUE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS));
    ASSERT_FALSE(ConsoleMessage());

    // Can't register twice
    ASSERT_FALSE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS));
    ASSERT_TRUE(ConsoleMessage());

    initializeContext();

    // Requires root config
    ASSERT_FALSE(client->processMessage(nullptr, EXT_EVENT));
    ASSERT_TRUE(ConsoleMessage());
}

static const char* LIVE_DATA_INIT = R"({
  "type": "APL",
  "version": "1.4",
  "extension": {
    "uri": "aplext:hello:10",
    "name": "Hello"
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": 500,
      "height": 500,
      "items": [
        {
          "type": "Text",
          "id": "label",
          "width": 100,
          "heigth": 100,
          "text": "${deviceState.alive}:${deviceState.rotation}"
        }
      ]
    }
  }
})";

static const char* DEVICE_STATE_INITIALIZE = R"({
  "version": "1.0",
  "method": "LiveDataUpdate",
  "name": "deviceState",
  "target": "aplext:hello:10",
  "operations": [
    {
      "type": "Set",
      "key": "alive",
      "item": true
    },
    {
      "type": "Set",
      "key": "rotation",
      "item": 7.9
    }
  ]
})";

TEST_F(ExtensionClientTest, LiveDataInitialize) {
    createConfigAndClient(LIVE_DATA_INIT);

    ASSERT_TRUE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS));
    ASSERT_FALSE(ConsoleMessage());

    // Arrives before root context is there as was send just after registration
    client->processMessage(nullptr, DEVICE_STATE_INITIALIZE);

    initializeContext();

    auto text = component->findComponentById("label");
    ASSERT_EQ(kComponentTypeText, text->getType());

    ASSERT_EQ("true:7.9", text->getCalculated(kPropertyText).asString());
}

TEST_F(ExtensionClientTest, LiveDataUpdates) {
    createConfigAndClient(EXT_DOC);

    ASSERT_TRUE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS));
    ASSERT_FALSE(ConsoleMessage());

    initializeContext();

    auto text = component->findComponentById("label");
    ASSERT_EQ(kComponentTypeText, text->getType());

    ASSERT_TRUE(client->processMessage(root, ENTITY_LIST_INSERT));
    root->clearPending();
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    auto arguments = event.getValue(kEventPropertyArguments);
    ASSERT_EQ(3, arguments.size());
    auto current = arguments.at(0);
    ASSERT_EQ(0, current.getDouble());

    ASSERT_EQ("onEntityAdded:3", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(client->processMessage(root, ENTITY_LIST_UPDATE));
    root->clearPending();
    ASSERT_EQ("onEntityChanged:3", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(client->processMessage(root, ENTITY_LIST_REMOVE));
    root->clearPending();
    ASSERT_EQ("onEntityChanged:3", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(client->processMessage(root, ENTITY_LIST_CLEAR));
    root->clearPending();
    ASSERT_TRUE(client->processMessage(root, ENTITY_LIST_INSERT_RANGE));
    root->clearPending();
    root->popEvent();
    ASSERT_EQ("onEntityAdded:3", text->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(client->processMessage(root, ENTITY_MAP_SET_DEAD));
    root->clearPending();
    root->popEvent();
    ASSERT_EQ("onDeviceUpdate:false::", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(client->processMessage(root, ENTITY_MAP_SET_ALIVE));
    root->clearPending();
    root->popEvent();
    ASSERT_EQ("onDeviceUpdate:true::", text->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(client->processMessage(root, ENTITY_MAP_SET_POSITION_AND_ROTATION));
    ASSERT_FALSE(ConsoleMessage());
    ASSERT_EQ("onDeviceUpdate:true::", text->getCalculated(kPropertyText).asString());
    ASSERT_TRUE(client->processMessage(root, ENTITY_MAP_REMOVE_FAIL));
    ASSERT_TRUE(ConsoleMessage());
    ASSERT_TRUE(client->processMessage(root, ENTITY_LIST_UPDATE_FAIL));
    ASSERT_TRUE(ConsoleMessage());
}

static const char* EXT_DOC_MULTI = R"({
  "type": "APL",
  "version": "1.4",
  "extensions": [
    {
      "uri": "aplext:hello:10",
      "name": "Hello"
    },
    {
      "uri": "aplext:greetings:10",
      "name": "Greetings"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": 500,
      "height": 500
    }
  }
})";

static const char* EXT_REGISTER_GREETINGS = R"({
  "method": "RegisterSuccess",
  "version": "1.0",
  "token": "TOKEN",
  "environment": {
    "something": "additional"
  },
  "schema": {
    "type": "Schema",
    "version": "1.0",
    "uri": "aplext:greetings:10"
  }
})";

TEST_F(ExtensionClientTest, ManyClients) {
    createConfig(EXT_DOC_MULTI);
    auto client1 = createClient("aplext:hello:10");
    auto client2 = createClient("aplext:greetings:10");

    client1->processMessage(nullptr, EXT_REGISTER_SUCCESS);
    ASSERT_FALSE(ConsoleMessage());

    client2->processMessage(nullptr, EXT_REGISTER_GREETINGS);
    ASSERT_FALSE(ConsoleMessage());

    auto ext = configPtr->getSupportedExtensions();
    ASSERT_EQ(2, ext.size());
    auto ex = ext.find("aplext:hello:10");
    ASSERT_NE(ext.end(), ex);
    ex = ext.find("aplext:greetings:10");
    ASSERT_NE(ext.end(), ex);
}

TEST_F(ExtensionClientTest, Command) {
    createConfigAndClient(EXT_DOC);

    ASSERT_TRUE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS));
    ASSERT_FALSE(ConsoleMessage());

    initializeContext();

    // Check interactions
    performTap(1,1);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    // Runtime needs to redirect this events to the server.
    auto processedCommand = client->processCommand(doc.GetAllocator(), event);
    ASSERT_STREQ("1.0", processedCommand["version"].GetString());
    ASSERT_STREQ("Command", processedCommand["method"].GetString());
    ASSERT_STREQ("TOKEN", processedCommand["token"].GetString());
    ASSERT_TRUE(processedCommand.HasMember("id"));
    ASSERT_STREQ("freeze", processedCommand["name"].GetString());
    ASSERT_STREQ("aplext:hello:10", processedCommand["target"].GetString());
    auto& payload = processedCommand["payload"];
    ASSERT_STREQ("push", payload["bar"].GetString());
    ASSERT_EQ(false, payload["baz"].GetBool());
    ASSERT_EQ(128, payload["foo"].GetDouble());

    // Resolve a response
    std::string commandResponse = EXT_COMMAND_SUCCESS_HEADER;
    commandResponse += R"("id": )" + std::to_string(processedCommand["id"].GetDouble()) + R"(,)";
    commandResponse += R"("result": true })";

    // Command result here.
    ASSERT_TRUE(client->processMessage(root, commandResponse));
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
}

TEST_F(ExtensionClientTest, CommandMissingRequired) {
    createConfigAndClient(EXT_DOC);

    ASSERT_TRUE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS));
    ASSERT_FALSE(ConsoleMessage());

    initializeContext();

    // Check interactions
    performTap(1,101);
    ASSERT_TRUE(ConsoleMessage());
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(ExtensionClientTest, CommandMissingNonRequired) {
    createConfigAndClient(EXT_DOC);

    ASSERT_TRUE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS));
    ASSERT_FALSE(ConsoleMessage());

    initializeContext();

    // Check interactions
    performTap(1,201);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    // Runtime needs to redirect this events to the server.
    auto processedCommand = client->processCommand(doc.GetAllocator(), event);
    ASSERT_STREQ("1.0", processedCommand["version"].GetString());
    ASSERT_STREQ("Command", processedCommand["method"].GetString());
    ASSERT_STREQ("TOKEN", processedCommand["token"].GetString());
    ASSERT_TRUE(processedCommand.HasMember("id"));
    ASSERT_STREQ("freeze", processedCommand["name"].GetString());
    ASSERT_STREQ("aplext:hello:10", processedCommand["target"].GetString());
    auto& payload = processedCommand["payload"];
    ASSERT_STREQ("boom", payload["bar"].GetString());
    ASSERT_EQ(false, payload["baz"].GetBool());
    ASSERT_EQ(128, payload["foo"].GetDouble());

    // Resolve a response
    std::string commandResponse = EXT_COMMAND_SUCCESS_HEADER;
    commandResponse += R"("id": )" + std::to_string(processedCommand["id"].GetDouble()) + R"(,)";
    commandResponse += R"("result": true })";

    // Command result here.
    ASSERT_TRUE(client->processMessage(root, commandResponse));
    ASSERT_FALSE(root->hasEvent());
}

TEST_F(ExtensionClientTest, CommandFail) {
    createConfigAndClient(EXT_DOC);
    rapidjson::Document document;

    ASSERT_TRUE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS));
    ASSERT_FALSE(ConsoleMessage());

    initializeContext();

    // Check interactions
    performTap(1,1);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    // Runtime needs to redirect this events to the server.
    auto processedCommand = client->processCommand(doc.GetAllocator(), event);

    // Resolve a response
    std::string commandResponse = EXT_COMMAND_FAILURE_HEADER;
    commandResponse += R"("id": )" + std::to_string(processedCommand["id"].GetDouble()) + R"(,)";
    commandResponse += R"("code": 7, "message": "Failed by some reason." })";

    // Command result here. No difference for fail ATM.
    ASSERT_TRUE(client->processMessage(root, commandResponse));
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
}

TEST_F(ExtensionClientTest, Event) {
    createConfigAndClient(EXT_DOC);

    ASSERT_TRUE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS));
    ASSERT_FALSE(ConsoleMessage());

    initializeContext();

    auto text = component->findComponentById("label");
    ASSERT_EQ(kComponentTypeText, text->getType());

    ASSERT_TRUE(client->processMessage(root, EXT_EVENT));
    ASSERT_EQ("onGenericExternallyComingEvent:exactly", text->getCalculated(kPropertyText).asString());
}

static const char* EXT_EVENT_NO_PAYLOAD = R"({
    "version": "1.0",
    "method": "Event",
    "target": "aplext:hello:10",
    "name": "onGenericExternallyComingEvent"
})";

TEST_F(ExtensionClientTest, EventEmpty) {
    createConfigAndClient(EXT_DOC);

    ASSERT_TRUE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS));
    ASSERT_FALSE(ConsoleMessage());

    initializeContext();

    auto text = component->findComponentById("label");
    ASSERT_EQ(kComponentTypeText, text->getType());

    ASSERT_TRUE(client->processMessage(root, EXT_EVENT_NO_PAYLOAD));
    ASSERT_EQ("onGenericExternallyComingEvent:", text->getCalculated(kPropertyText).asString());
}

static const char* EXT_EVENT_WRONG_TARGET = R"({
    "version": "1.0",
    "method": "Event",
    "target": "aplext:bye:10",
    "name": "onGenericExternallyComingEvent",
    "data": { "potatos": "exactly" }
})";

static const char* ENTITY_MAP_SET_WRONG_TARGET = R"({
  "version": "1.0",
  "method": "LiveDataUpdate",
  "name": "deviceState",
  "target": "aplext:bye:10",
  "operations": [
    {
      "type": "Set",
      "key": "alive",
      "item": false
    }
  ]
})";

TEST_F(ExtensionClientTest, TargetMistmatch) {
    createConfigAndClient(EXT_DOC);

    ASSERT_TRUE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS));
    ASSERT_FALSE(ConsoleMessage());

    initializeContext();

    ASSERT_FALSE(client->processMessage(root, EXT_EVENT_WRONG_TARGET));
    ASSERT_TRUE(ConsoleMessage());

    ASSERT_FALSE(client->processMessage(root, ENTITY_MAP_SET_WRONG_TARGET));
    ASSERT_TRUE(ConsoleMessage());
}

static const char* EXT_EVENT_WRONG_NAME = R"({
    "version": "1.0",
    "method": "Event",
    "target": "aplext:hello:10",
    "name": "badName",
    "payload": { "potatos": "exactly" }
})";

static const char* ENTITY_MAP_SET_WRONG_NAME = R"({
    "version": "1.0",
    "method": "LiveDataUpdate",
    "name": "badName",
    "target": "aplext:hello:10",
    "operations": [
        {
        "type": "Set",
        "key": "alive",
        "item": false
        }
    ]
})";

TEST_F(ExtensionClientTest, NameMistmatch) {
    createConfigAndClient(EXT_DOC);

    ASSERT_TRUE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS));
    ASSERT_FALSE(ConsoleMessage());

    initializeContext();

    ASSERT_FALSE(client->processMessage(root, EXT_EVENT_WRONG_NAME));
    ASSERT_TRUE(ConsoleMessage());

    ASSERT_FALSE(client->processMessage(root, ENTITY_MAP_SET_WRONG_NAME));
    ASSERT_TRUE(ConsoleMessage());
}

static const char* BAD_LIVE_DATA_UPDATE = R"({
  "version": "1.0",
  "method": "LiveDataUpdate",
  "name": "deviceState",
  "target": "aplext:hello:10",
  "operations": null
})";

TEST_F(ExtensionClientTest, BadLiveDataUpdate) {
    createConfigAndClient(EXT_DOC);

    ASSERT_TRUE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS));
    ASSERT_FALSE(ConsoleMessage());

    initializeContext();

    ASSERT_FALSE(client->processMessage(root, BAD_LIVE_DATA_UPDATE));
    ASSERT_TRUE(ConsoleMessage());
}

static const char* COLLAPSED_EXT_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "extension": {
    "uri": "aplext:hello:10",
    "name": "Hello"
  },
  "settings": {
    "Hello": {
      "authorizationCode": "MAGIC"
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": 500,
      "height": 500
    }
  },
  "Hello:onDeviceUpdate": [
    {
      "type": "SendEvent",
      "sequencer": "SEQ${changed.length}",
      "arguments": ["${event.current.uncollapsed}", "${event.current.collapsed1}", "${event.current.collapsed2}", "${event.changed}"]
    }
  ],
  "Hello:onDeviceRemove": [
    {
      "type": "SendEvent",
      "sequencer": "SEQ${Math.random}",
      "arguments": ["${event.current}", "${event.changed}"]
    }
  ]
})";

static const char* ENTITY_MAP_SET_COLLAPSED1 = R"({
  "version": "1.0",
  "method": "LiveDataUpdate",
  "name": "deviceState",
  "target": "aplext:hello:10",
  "operations": [
    {
      "type": "Set",
      "key": "collapsed1",
      "item": true
    }
  ]
})";

static const char* ENTITY_MAP_SET_COLLAPSED2 = R"({
  "version": "1.0",
  "method": "LiveDataUpdate",
  "name": "deviceState",
  "target": "aplext:hello:10",
  "operations": [
    {
      "type": "Set",
      "key": "collapsed2",
      "item": true
    }
  ]
})";

static const char* ENTITY_MAP_SET_UNCOLLAPSED = R"({
  "version": "1.0",
  "method": "LiveDataUpdate",
  "name": "deviceState",
  "target": "aplext:hello:10",
  "operations": [
    {
      "type": "Set",
      "key": "uncollapsed",
      "item": true
    }
  ]
})";

static const char* ENTITY_MAP_REMOVE_ALL = R"({
  "version": "1.0",
  "method": "LiveDataUpdate",
  "name": "deviceState",
  "target": "aplext:hello:10",
  "operations": [
    {
      "type": "Remove",
      "key": "uncollapsed"
    },
    {
      "type": "Remove",
      "key": "collapsed1"
    },
    {
      "type": "Remove",
      "key": "collapsed2"
    }
  ]
})";

TEST_F(ExtensionClientTest, LiveDataCollapse) {
    createConfigAndClient(COLLAPSED_EXT_DOC);

    ASSERT_TRUE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS));
    ASSERT_FALSE(ConsoleMessage());

    initializeContext();

    ASSERT_TRUE(client->processMessage(root, ENTITY_MAP_SET_COLLAPSED1));
    ASSERT_TRUE(client->processMessage(root, ENTITY_MAP_SET_COLLAPSED2));
    ASSERT_TRUE(client->processMessage(root, ENTITY_MAP_SET_UNCOLLAPSED));
    root->clearPending();

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    auto arguments = event.getValue(kEventPropertyArguments);
    ASSERT_EQ(4, arguments.size());
    auto uncollapsed = arguments.at(0);
    ASSERT_EQ(true, uncollapsed.getBoolean());
    auto collapsed1 = arguments.at(1);
    ASSERT_EQ(true, collapsed1.getBoolean());
    auto collapsed2 = arguments.at(2);
    ASSERT_EQ(true, collapsed2.getBoolean());
    auto changed = arguments.at(3);
    ASSERT_EQ(true, changed.get("uncollapsed").getBoolean());

    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    arguments = event.getValue(kEventPropertyArguments);
    ASSERT_EQ(4, arguments.size());
    uncollapsed = arguments.at(0);
    ASSERT_EQ(true, uncollapsed.getBoolean());
    collapsed1 = arguments.at(1);
    ASSERT_EQ(true, collapsed1.getBoolean());
    collapsed2 = arguments.at(2);
    ASSERT_EQ(true, collapsed2.getBoolean());
    changed = arguments.at(3);
    ASSERT_EQ(true, changed.get("collapsed1").getBoolean());
    ASSERT_EQ(true, changed.get("collapsed2").getBoolean());


    ASSERT_TRUE(client->processMessage(root, ENTITY_MAP_REMOVE_ALL));
    root->clearPending();

    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    arguments = event.getValue(kEventPropertyArguments);
    ASSERT_EQ(2, arguments.size());
    ASSERT_EQ(0, arguments.at(0).size());
    changed = arguments.at(1);
    ASSERT_EQ(true, changed.get("uncollapsed").isNull());

    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    arguments = event.getValue(kEventPropertyArguments);
    ASSERT_EQ(2, arguments.size());
    ASSERT_EQ(0, arguments.at(0).size());
    changed = arguments.at(1);
    ASSERT_EQ(true, changed.get("collapsed1").isNull());
    ASSERT_EQ(true, changed.get("collapsed2").isNull());
}

static const char* EXT_REGISTER_SUCCESS_EXTENDED_TYPE = R"({
  "method": "RegisterSuccess",
  "version": "1.0",
  "token": "TOKEN",
  "schema": {
    "type": "Schema",
    "version": "1.0",
    "uri": "aplext:hello:10",
    "types": [
      {
        "name": "User",
        "properties": {
          "id": "string",
          "position": "object"
        }
      },
      {
        "name": "DecoratedUser",
        "extends": "User",
        "properties": {
          "department": {
            "type": "string",
            "required": false,
            "default": "Sales"
          }
        }
      }
    ],
    "commands": [
      {
        "name": "Ping",
        "payload": "User"
      },
      {
        "name": "Ask",
        "payload": {
          "type": "DecoratedUser"
        }
      }
    ]
  }
})";

TEST_F(ExtensionClientTest, ExtendedTypes) {
    createConfigAndClient(EXT_DOC);

    ASSERT_TRUE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS_EXTENDED_TYPE));
    ASSERT_FALSE(ConsoleMessage());
    ASSERT_TRUE(client->registrationMessageProcessed());
    ASSERT_TRUE(client->registered());

    auto commands = configPtr->getExtensionCommands();
    ASSERT_EQ(2, commands.size());
    auto ping = commands.at(0);
    ASSERT_EQ("Ping", ping.getName());
    const auto& pingParams = ping.getPropertyMap();
    ASSERT_EQ(2, pingParams.size());
    ASSERT_TRUE(pingParams.count("id"));
    ASSERT_TRUE(pingParams.count("position"));

    auto ask = commands.at(1);
    ASSERT_EQ("Ask", ask.getName());
    const auto& askParams = ask.getPropertyMap();
    ASSERT_EQ(3, askParams.size());
    ASSERT_TRUE(askParams.count("id"));
    ASSERT_TRUE(askParams.count("position"));
    ASSERT_TRUE(askParams.count("department"));
}

static const char* EXT_REGISTER_SUCCESS_BADLY_EXTENDED_TYPE = R"({
  "method": "RegisterSuccess",
  "version": "1.0",
  "token": "TOKEN",
  "schema": {
    "type": "Schema",
    "version": "1.0",
    "uri": "aplext:hello:10",
    "types": [
      {
        "name": "DecoratedUser",
        "extends": "User",
        "properties": {
          "department": {
            "type": "string",
            "required": false,
            "default": "Sales"
          }
        }
      },
      {
        "name": "User",
        "properties": {
          "id": "string",
          "position": "object"
        }
      }
    ],
    "commands": [
      {
        "name": "Ping",
        "payload": "User"
      },
      {
        "name": "Ask",
        "payload": {
          "type": "DecoratedUser"
        }
      }
    ]
  }
})";

TEST_F(ExtensionClientTest, BadlyExtendedTypes) {
    createConfigAndClient(EXT_DOC);

    ASSERT_TRUE(client->processMessage(nullptr, EXT_REGISTER_SUCCESS_BADLY_EXTENDED_TYPE));
    ASSERT_TRUE(ConsoleMessage());
    ASSERT_TRUE(client->registrationMessageProcessed());
    ASSERT_TRUE(client->registered());

    auto commands = configPtr->getExtensionCommands();
    ASSERT_EQ(2, commands.size());
    auto ping = commands.at(0);
    ASSERT_EQ("Ping", ping.getName());
    const auto& pingParams = ping.getPropertyMap();
    ASSERT_EQ(2, pingParams.size());
    ASSERT_TRUE(pingParams.count("id"));
    ASSERT_TRUE(pingParams.count("position"));

    auto ask = commands.at(1);
    ASSERT_EQ("Ask", ask.getName());
    const auto& askParams = ask.getPropertyMap();
    ASSERT_EQ(1, askParams.size());
    ASSERT_TRUE(askParams.count("department"));
}


/**
 * A Weather Live Data map example.
 * The map does not have the properties defined at time of registration,
 * but provides LiveDataUpdates to property values post registration.
 */
static const char* WEATHER = R"(
"types": [
  {
    "name": "Weather",
    "properties": {
    }
  }
],
"liveData": [
    {"name": "MyWeather", "type": "Weather"}
]
)";

static const char* WEATHER_MAP_SET_PROP = R"(
{
  "version": "1.0",
  "method": "LiveDataUpdate",
  "name": "MyWeather",
  "target": "aplext:hello:10",
  "operations": [
    {
      "type": "Set",
      "key": "location",
      "item": "Boston"
    },
    {
      "type": "Set",
      "key": "temperature",
      "item": "64"
    },
    {
      "type": "Set",
      "key": "propNull"
    }
  ]
})";
TEST_F(ExtensionClientTest, TypeWithoutPropertis) {
    createConfigAndClient(EXT_DOC);

    std::string doc = "{";
    doc += EXTENSION_DEFINITION;
    doc += WEATHER;
    doc += "}}";

    ASSERT_TRUE(client->processMessage(nullptr, doc));
    ASSERT_FALSE(ConsoleMessage());

    // Verify the extension is registered
    auto ext = configPtr->getSupportedExtensions();
    ASSERT_EQ(1, ext.size());
    auto ex = ext.find("aplext:hello:10");
    ASSERT_NE(ext.end(), ex);

    // Verify the live map is configured, without properties
    auto liveDataMap = configPtr->getLiveObjectMap();
    ASSERT_EQ(1, liveDataMap.size());
    auto& map = liveDataMap.at("MyWeather");
    ASSERT_EQ(Object::ObjectType::kMapType, map->getType());
    auto liveMap = std::dynamic_pointer_cast<LiveMap>(map);
    ASSERT_EQ(0, liveMap->getMap().size());

    // Inflate the doc
    initializeContext();

    // Verify the defined LiveData object exist in the document context as an empty map
    ASSERT_TRUE(IsEqual(Object::EMPTY_MAP(), evaluate(*context, "${MyWeather}")));

    // Process an update message
    client->processMessage(root, WEATHER_MAP_SET_PROP);
    ASSERT_FALSE(ConsoleMessage());

    // Verify the LiveData object exists in the document context with expected properties
    ASSERT_TRUE(IsEqual(Object::EMPTY_MAP(), evaluate(*context, "${MyWeather}")));
    ASSERT_TRUE(IsEqual("Boston", evaluate(*context, "${MyWeather.location}")));
    ASSERT_TRUE(IsEqual("64", evaluate(*context, "${MyWeather.temperature}")));
    ASSERT_TRUE(IsEqual(Object::NULL_OBJECT(), evaluate(*context, "${MyWeather.propNull}")));

}