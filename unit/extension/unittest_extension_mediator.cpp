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

#ifdef ALEXAEXTENSIONS

#include "../testeventloop.h"

using namespace apl;
using namespace alexaext;


static const char* EXTENSION_DEFINITION = R"(
    "type":"Schema",
    "version":"1.0"
)";

static const char* EXTENSION_TYPES = R"(
    ,"types": [
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
        "name": "Entity",
        "properties": {
          "alive": "boolean",
          "position": "string"
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
          "rotation": {
            "type": "float",
            "required": false,
            "default": 0.0
          },
          "position": {
            "type": "string",
            "required": false,
            "default": "none"
          }
        }
      }
    ]
)";

static const char* EXTENSION_COMMANDS = R"(
  ,"commands": [
    {
      "name": "follow"
    },
    {
      "name": "lead",
      "requireResponse": "true"
    },
    {
      "name": "freeze",
      "requireResponse": false,
      "payload": "FreezePayload"
    },
    {
      "name": "clipEntity",
      "requireResponse": false,
      "payload": {
        "type": "FreezePayload",
        "description": "Don't really care about this property."
      }
    }
  ]
)";


static const char* EXTENSION_EVENTS = R"(
    ,"events": [
      { "name": "onEntityAdded" },
      { "name": "onEntityChanged" },
      { "name": "onEntityLost" },
      { "name": "onDeviceUpdate" },
      { "name": "onDeviceRemove" },
      { "name": "onGenericExternallyComingEvent", "mode": "NORMAL" }
    ]
)";

static const char* EXTENSION_DATA_BINDINGS = R"(
    ,"liveData": [
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
                "name": "position",
                "update": true,
                "collapse": true
              },
              {
                "name": "rotation",
                "update": true
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
                "name": "notCollapsed",
                "update": true,
                "collapse": false
              }
            ]
          }
        }
      }
    ]
)";

static bool sForceFail = false;

/**
 * Sample Extension for testing.
 */
class TestExtension final : public alexaext::ExtensionBase {

public:
    explicit TestExtension(const std::set<std::string>& uris) : ExtensionBase(uris) {};

    bool invokeCommand(const std::string& uri, const rapidjson::Value& command) override {
        auto id = alexaext::Command::ID().Get(command);
        auto name = alexaext::Command::NAME().Get(command);
        if (id && name) {
            lastCommandId = (int)id->GetDouble();
            lastCommandName = name->GetString();
            return true;
        }
        return false;
    }

    rapidjson::Document createRegistration(const std::string& uri, const rapidjson::Value& registerRequest) override {

        if (sForceFail) {
            return rapidjson::Document();
        }

        std::string schema = "{";
        schema += EXTENSION_DEFINITION;
        if (uri == "aplext:hello:10") { // hello extension has data binding
            schema += EXTENSION_TYPES;
            schema += EXTENSION_COMMANDS;
            schema += EXTENSION_EVENTS;
            schema += EXTENSION_DATA_BINDINGS;
        }
        schema += "}";
        rapidjson::Document doc;
        doc.Parse(schema.c_str());
        doc.AddMember("uri", rapidjson::Value(uri.c_str(), doc.GetAllocator()), doc.GetAllocator());
        return RegistrationSuccess("1.0").uri(uri).token("SessionToken12").schema(doc);
    }

    // test method to simulate internally generated
    bool generateTestEvent(const std::string& uri, const std::string& event) {
        rapidjson::Document doc;
        doc.Parse(event.c_str());
        return invokeExtensionEventHandler(uri, doc);
    }

    // test method to simulate internally generated
    bool generateLiveDataUpdate(const std::string& uri, const std::string& update) {
        rapidjson::Document doc;
        doc.Parse(update.c_str());
        return invokeLiveDataUpdate(uri, doc);
    }

    void onRegistered(const std::string &uri, const std::string &token) override {
        registered = true;
    }

    int lastCommandId;
    std::string lastCommandName;
    bool registered = false;
};


class ExtensionMediatorTest : public DocumentWrapper {
public:

    ExtensionMediatorTest() : DocumentWrapper() {
    }

    void createProvider() {
        extensionProvider = std::make_shared<alexaext::ExtensionRegistrar>();
        mediator = ExtensionMediator::create(extensionProvider);
    }

    void loadExtensions(const char* document) {
        createContent(document, nullptr);

        if (!extensionProvider) {
            createProvider();
        }

        // Experimental feature required
        config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
                .extensionProvider(extensionProvider)
                .extensionMediator(mediator);

        auto requests = content->getExtensionRequests();
        //  create a test extension for every request
        for (auto& req: requests) {
            auto ext = std::make_shared<TestExtension>(
                    std::set<std::string>({req}));
            auto proxy = std::make_shared<alexaext::LocalExtensionProxy>(ext);
            extensionProvider->registerExtension(proxy);
            // save direct access to extension for test use
            testExtensions.emplace(req, ext);
        }
        // load them into config via the mediator
        mediator->loadExtensions(config, content);
    }

    void TearDown() override {
        extensionProvider = nullptr;
        mediator = nullptr;
        testExtensions.clear();
    }

    void testLifecycle();

    ExtensionRegistrarPtr extensionProvider;                 // provider instance for tests
    ExtensionMediatorPtr mediator;
    std::map<std::string, std::shared_ptr<TestExtension>> testExtensions; // direct access to extensions for test
};

static const char* EXT_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "extension": [
      {
        "uri": "aplext:hello:10",
        "name": "Hello"
      },
      {
        "uri": "aplext:goodbye:10",
        "name": "Bye"
      }
  ],
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
          "height": 100,
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
          "height": 100,
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
          "height": 100,
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
          "height": 100,
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
      "value": "onGenericExternallyComingEvent:${event.potatoes}"
    }
  ]
})";

/**
 * Experimental feature flag.
 */
TEST_F(ExtensionMediatorTest, ExperimentalFeature) {
    createProvider();

    // provider and mediator are ignored without experimental feature
    config->extensionProvider(extensionProvider).extensionMediator(mediator);
    ASSERT_EQ(nullptr, config->getExtensionProvider());
    ASSERT_EQ(nullptr, config->getExtensionMediator());

    // provider and mediator are available when experimental flag set
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
            .extensionProvider(extensionProvider).extensionMediator(mediator);
    ASSERT_NE(nullptr, config->getExtensionProvider());
    ASSERT_NE(nullptr, config->getExtensionMediator());
}

/**
 * Test that the mediator loads available extensions into the RootConfig.
 */
TEST_F(ExtensionMediatorTest, RegistrationConfig) {

    loadExtensions(EXT_DOC);

    // 2 extensions with the same schema are registered
    auto uris = config->getSupportedExtensions();
    ASSERT_EQ(2, uris.size());
    ASSERT_EQ(1, uris.count("aplext:hello:10"));
    ASSERT_EQ(1, uris.count("aplext:goodbye:10"));

    auto commands = config->getExtensionCommands();
    ASSERT_EQ(4, commands.size());

    auto events = config->getExtensionEventHandlers();
    ASSERT_EQ(6, events.size());

    auto liveDataMap = config->getLiveObjectMap();
    ASSERT_EQ(2, liveDataMap.size());
}


TEST_F(ExtensionMediatorTest, ExtensionParseCommands) {

    loadExtensions(EXT_DOC);

    auto commands = config->getExtensionCommands();
    ASSERT_EQ(4, commands.size());

    ASSERT_EQ("aplext:hello:10", commands[0].getURI());
    ASSERT_EQ("follow", commands[0].getName());
    ASSERT_FALSE(commands[0].getRequireResolution());
    ASSERT_TRUE(commands[0].getPropertyMap().empty());

    ASSERT_EQ("aplext:hello:10", commands[1].getURI());
    ASSERT_EQ("lead", commands[1].getName());
    ASSERT_TRUE(commands[1].getRequireResolution());
    ASSERT_TRUE(commands[1].getPropertyMap().empty());

    ASSERT_EQ("aplext:hello:10", commands[2].getURI());
    ASSERT_EQ("freeze", commands[2].getName());
    ASSERT_FALSE(commands[3].getRequireResolution());

    auto props = commands[2].getPropertyMap();
    ASSERT_EQ(4, props.size());
    ASSERT_TRUE(IsEqual(true, props.at("foo").required));
    ASSERT_TRUE(IsEqual(64, props.at("foo").defvalue));
    ASSERT_TRUE(IsEqual(false, props.at("bar").required));
    ASSERT_TRUE(IsEqual("boom", props.at("bar").defvalue));
    ASSERT_TRUE(IsEqual(true, props.at("baz").required));
    ASSERT_TRUE(IsEqual(true, props.at("baz").defvalue));

    ASSERT_EQ("aplext:hello:10", commands[3].getURI());
    ASSERT_EQ("clipEntity", commands[3].getName());
    ASSERT_FALSE(commands[3].getRequireResolution());

    props = commands[3].getPropertyMap();
    ASSERT_EQ(4, props.size());
    ASSERT_TRUE(IsEqual(true, props.at("foo").required));
    ASSERT_TRUE(IsEqual(64, props.at("foo").defvalue));
    ASSERT_TRUE(IsEqual(false, props.at("bar").required));
    ASSERT_TRUE(IsEqual("boom", props.at("bar").defvalue));
    ASSERT_TRUE(IsEqual(true, props.at("baz").required));
    ASSERT_TRUE(IsEqual(true, props.at("baz").defvalue));
}


TEST_F(ExtensionMediatorTest, ExtensionParseEventHandlers) {
    loadExtensions(EXT_DOC);

    auto handlers = config->getExtensionEventHandlers();
    ASSERT_EQ(6, handlers.size());
    ASSERT_EQ("aplext:hello:10", handlers[0].getURI());
    ASSERT_EQ("onEntityAdded", handlers[0].getName());
    ASSERT_EQ("aplext:hello:10", handlers[1].getURI());
    ASSERT_EQ("onEntityChanged", handlers[1].getName());
    ASSERT_EQ("aplext:hello:10", handlers[2].getURI());
    ASSERT_EQ("onEntityLost", handlers[2].getName());
    ASSERT_EQ("aplext:hello:10", handlers[3].getURI());
    ASSERT_EQ("onDeviceUpdate", handlers[3].getName());
    ASSERT_EQ("aplext:hello:10", handlers[4].getURI());
    ASSERT_EQ("onDeviceRemove", handlers[4].getName());
    ASSERT_EQ("aplext:hello:10", handlers[5].getURI());
    ASSERT_EQ("onGenericExternallyComingEvent", handlers[5].getName());
}


TEST_F(ExtensionMediatorTest, ExtensionParseEventDataBindings) {
    loadExtensions(EXT_DOC);

    auto ext = config->getSupportedExtensions();
    ASSERT_EQ(2, ext.size());
    auto ex = ext.find("aplext:hello:10");
    ASSERT_NE(ext.end(), ex);

    auto liveDataMap = config->getLiveObjectMap();
    ASSERT_EQ(2, liveDataMap.size());
    auto arr = liveDataMap.at("entityList");
    auto map = liveDataMap.at("deviceState");
    ASSERT_EQ(Object::ObjectType::kArrayType, arr->getType());
    ASSERT_EQ(Object::ObjectType::kMapType, map->getType());
}


static const char* EXT_EVENT = R"({
    "version": "1.0",
    "method": "Event",
    "target": "aplext:hello:10",
    "name": "onGenericExternallyComingEvent",
    "payload": { "potatoes": "exactly" }
})";


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

static const char* MAP_SET = R"({
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


static const char* MAP_MULTI_OP = R"({
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


TEST_F(ExtensionMediatorTest, CommandResolve) {
    loadExtensions(EXT_DOC);

    // We have all we need. Inflate.
    inflate();

    auto text = component->findComponentById("label");
    ASSERT_EQ(kComponentTypeText, text->getType());

    // Tap happened!
    performTap(1, 1);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    // Runtime needs to redirect this events to the server.
    ASSERT_TRUE(mediator->invokeCommand(event));

    // verify resolve by testing next event in sequence is live
    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
}

void ExtensionMediatorTest::testLifecycle() {
    loadExtensions(EXT_DOC);

    // verify the extension was registered
    ASSERT_TRUE(extensionProvider->hasExtension("aplext:hello:10"));
    auto ext = extensionProvider->getExtension("aplext:hello:10");
    ASSERT_TRUE(ext);
    // direct access to extension for test inspection
    auto hello = testExtensions["aplext:hello:10"];

    // We have all we need. Inflate.
    inflate();

    ASSERT_TRUE(hello->registered);

    auto text = component->findComponentById("label");
    ASSERT_EQ(kComponentTypeText, text->getType());

    // Tap happened! Initiate command sequence : kEventTypeExtension, kEventTypeSendEvent
    performTap(1, 1);
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    // Redirect this events to the extension.
    hello->lastCommandId = 0;
    hello->lastCommandName = "";
    ASSERT_EQ(kEventTypeExtension, event.getType());
    mediator->invokeCommand(event);
    ASSERT_FALSE(ConsoleMessage());
    ASSERT_NE(0, hello->lastCommandId);
    ASSERT_EQ("freeze", hello->lastCommandName);

    // verify resolve by testing the next command in the sequence fired
    event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());

    // simulate an event from the extension
    ASSERT_TRUE(hello->generateTestEvent("aplext:hello:10", EXT_EVENT));
    ASSERT_EQ("onGenericExternallyComingEvent:exactly", text->getCalculated(kPropertyText).asString());

    // simulate a live data update from the extension
    ASSERT_TRUE(hello->generateLiveDataUpdate("aplext:hello:10", ENTITY_LIST_INSERT));
    ASSERT_FALSE(ConsoleMessage());
    root->clearPending();
    ASSERT_TRUE(root->hasEvent());
    root->popEvent();
    ASSERT_EQ("onEntityAdded:3", text->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(hello->generateLiveDataUpdate("aplext:hello:10", ENTITY_LIST_UPDATE));
    ASSERT_FALSE(ConsoleMessage());
    root->clearPending();
    ASSERT_EQ("onEntityChanged:3", text->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(hello->generateLiveDataUpdate("aplext:hello:10", ENTITY_LIST_REMOVE));
    ASSERT_FALSE(ConsoleMessage());
    root->clearPending();
    ASSERT_EQ("onEntityChanged:3", text->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(hello->generateLiveDataUpdate("aplext:hello:10", ENTITY_LIST_CLEAR));
    ASSERT_FALSE(ConsoleMessage());
    root->clearPending();

    ASSERT_TRUE(hello->generateLiveDataUpdate("aplext:hello:10", ENTITY_LIST_INSERT_RANGE));
    ASSERT_FALSE(ConsoleMessage());
    root->clearPending();
    root->popEvent();
    ASSERT_EQ("onEntityAdded:3", text->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(hello->generateLiveDataUpdate("aplext:hello:10", MAP_MULTI_OP));
    ASSERT_FALSE(ConsoleMessage());
    root->clearPending();
    root->popEvent();
    ASSERT_EQ("onDeviceUpdate::pos:7.9", text->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(hello->generateLiveDataUpdate("aplext:hello:10", MAP_SET));
    ASSERT_FALSE(ConsoleMessage());
    root->clearPending();
    root->popEvent();
    ASSERT_EQ("onDeviceUpdate:false:pos:7.9", text->getCalculated(kPropertyText).asString());

}

TEST_F(ExtensionMediatorTest, ExtensionLifecycleNoExecutor) {
    // Test the lifecycle using the mediator as the executor
    testLifecycle();
}

/**
 * Executor class used by viewhost to sequence message processing.
 */
class TestExecutor : public alexaext::Executor {
public:

    bool enqueueTask(Task task) override {
        task();
        return true;
    }
};


TEST_F(ExtensionMediatorTest, ExtensionLifecycleWithExecutor) {
    // Test the lifecycle using an assigned executor
    extensionProvider = std::make_shared<alexaext::ExtensionRegistrar>();
    auto executor = std::make_shared<TestExecutor>();
    mediator = ExtensionMediator::create(extensionProvider, executor);
    testLifecycle();
}


static const char* BAD_EVENT = R"({
    "version": "1.0",
    "method": "Event",
    "target": "aplext:hello:10",
    "name": "bad"
})";

TEST_F(ExtensionMediatorTest, EventBad) {
    loadExtensions(EXT_DOC);

    // verify the extension was registered
    ASSERT_TRUE(extensionProvider->hasExtension("aplext:hello:10"));
    auto ext = extensionProvider->getExtension("aplext:hello:10");
    ASSERT_TRUE(ext);
    // direct access to extension for test inspection
    auto hello = testExtensions["aplext:hello:10"];

    inflate();

    // send bad event
    hello->generateTestEvent("aplext:hello:10", BAD_EVENT);
    ASSERT_TRUE(ConsoleMessage());

    // send good event
    hello->generateTestEvent("aplext:hello:10", EXT_EVENT);
    ASSERT_FALSE(ConsoleMessage());
}

static const char* BAD_DATA_UPDATE = R"({
  "version": "1.0",
  "method": "LiveDataUpdate",
  "name": "bad",
  "target": "aplext:hello:10",
  "operations": [
    {
      "type": "Bad"
    }
  ]
})";

TEST_F(ExtensionMediatorTest, DataUpdateBad) {
    loadExtensions(EXT_DOC);

    // verify the extension was registered
    ASSERT_TRUE(extensionProvider->hasExtension("aplext:hello:10"));
    auto ext = extensionProvider->getExtension("aplext:hello:10");
    ASSERT_TRUE(ext);
    // direct access to extension for test inspection
    auto hello = testExtensions["aplext:hello:10"];

    inflate();

    // send bad update
    hello->generateLiveDataUpdate("aplext:hello:10", BAD_DATA_UPDATE);
    ASSERT_TRUE(ConsoleMessage());

    // send a good update
    hello->generateLiveDataUpdate("aplext:hello:10", ENTITY_LIST_INSERT);
    ASSERT_FALSE(ConsoleMessage());
}

TEST_F(ExtensionMediatorTest, RegisterBad) {
    sForceFail = true;
    loadExtensions(EXT_DOC);
    ASSERT_TRUE(ConsoleMessage());
    ASSERT_EQ(0, config->getSupportedExtensions().size());
}

#endif