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
#include "apl/extension/extensioncomponent.h"

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

static const char* EXTENSION_COMPONENTS = R"(
    ,"components": [
    {
        "name": "Canvas"
    }
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

        auto flags = RegistrationRequest::FLAGS().Get(registerRequest);
        if (flags->IsString())
            mFlags = flags->GetString();
        auto settings = RegistrationRequest::SETTINGS().Get(registerRequest);
        if (settings->IsObject()) {
            auto find = settings->FindMember("authorizationCode");
            if (find != settings->MemberEnd())
                mAuthorizationCode = find->value.GetString();
        }

        std::string schema = "{";
        schema += EXTENSION_DEFINITION;
        if (uri == "aplext:hello:10") { // hello extension has data binding
            schema += EXTENSION_TYPES;
            schema += EXTENSION_COMMANDS;
            schema += EXTENSION_EVENTS;
            schema += EXTENSION_COMPONENTS;
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

    bool updateComponent(const std::string &uri, const rapidjson::Value &command) override {
        return true;
    };

    void onResourceReady(const std::string&uri, const ResourceHolderPtr& resource) override {
        mResource = resource;
    }

    int lastCommandId;
    std::string lastCommandName;
    bool registered = false;
    std::string mFlags;
    std::string mAuthorizationCode;
    ResourceHolderPtr mResource;
};


class TestResourceProvider final: public ExtensionResourceProvider {
public:
    bool requestResource(const std::string& uri, const std::string& resourceId,
                         ExtensionResourceSuccessCallback success,
                         ExtensionResourceFailureCallback error) override {

        // success callback if resource supported
        auto resource = std::make_shared<ResourceHolder>(resourceId);
        success(uri, resource);
        return true;
    };
};

class ExtensionMediatorTest : public DocumentWrapper {
public:

    ExtensionMediatorTest() : DocumentWrapper() {
    }

    void createProvider() {
        extensionProvider = std::make_shared<alexaext::ExtensionRegistrar>();
        resourceProvider = std::make_shared<TestResourceProvider>();
        mediator = ExtensionMediator::create(extensionProvider, resourceProvider,
                                             alexaext::Executor::getSynchronousExecutor());
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
        resourceProvider = nullptr;
        testExtensions.clear();

        DocumentWrapper::TearDown();
    }

    void testLifecycle();

    ExtensionRegistrarPtr extensionProvider;                 // provider instance for tests
    ExtensionResourceProviderPtr resourceProvider;    // provider for test resources
    ExtensionMediatorPtr mediator;
    std::map<std::string, std::weak_ptr<TestExtension>> testExtensions; // direct access to extensions for test
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
        },
        {
          "type": "Hello:Canvas",
          "id": "MyCanvas",
          "width": 100,
          "height": 100
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

/**
 * Test that runtime flags are passed to the extension.
 */
TEST_F(ExtensionMediatorTest, RegistrationFlags) {

    config->registerExtensionFlags("aplext:hello:10", "--hello");
    loadExtensions(EXT_DOC);

    // direct access to extension for test inspection
    auto hello = testExtensions["aplext:hello:10"].lock();
    ASSERT_TRUE(hello);

    ASSERT_EQ("--hello", hello->mFlags);
}

/**
 * Test that the document settings are passed to the extension.
 */
TEST_F(ExtensionMediatorTest, ParseSettings) {

    config->registerExtensionFlags("aplext:hello:10", "--hello");
    loadExtensions(EXT_DOC);

    // verify the extension was registered
    ASSERT_TRUE(extensionProvider->hasExtension("aplext:hello:10"));
    auto ext = extensionProvider->getExtension("aplext:hello:10");
    ASSERT_TRUE(ext);
    // direct access to extension for test inspection
    auto hello = testExtensions["aplext:hello:10"].lock();
    ASSERT_TRUE(hello);

    ASSERT_EQ("MAGIC", hello->mAuthorizationCode);
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
    // Extension event handled here, directly.
    root->clearPending();

    // verify resolve by testing next event in sequence is live
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
}

void ExtensionMediatorTest::testLifecycle() {
    loadExtensions(EXT_DOC);

    // verify the extension was registered
    ASSERT_TRUE(extensionProvider->hasExtension("aplext:hello:10"));
    auto ext = extensionProvider->getExtension("aplext:hello:10");
    ASSERT_TRUE(ext);
    // direct access to extension for test inspection
    auto hello = testExtensions["aplext:hello:10"].lock();

    // We have all we need. Inflate.
    inflate();

    ASSERT_TRUE(hello->registered);
    ASSERT_TRUE(IsEqual(Object::TRUE_OBJECT(), evaluate(*context, "${environment.extension.Hello}")));

    auto text = component->findComponentById("label");
    ASSERT_EQ(kComponentTypeText, text->getType());

    auto canvas = root->findComponentById("MyCanvas");
    ASSERT_TRUE(canvas);

    // Event should be redirected by them mediator.
    hello->lastCommandId = 0;
    hello->lastCommandName = "";
    // Tap happened! Initiate command sequence : kEventTypeExtension, kEventTypeSendEvent
    performTap(1, 1);
    root->clearPending();
    ASSERT_TRUE(root->hasEvent());

    ASSERT_NE(0, hello->lastCommandId);
    ASSERT_EQ("freeze", hello->lastCommandName);

    // verify resolve by testing the next command in the sequence fired
    auto event = root->popEvent();
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
    auto hello = testExtensions["aplext:hello:10"].lock();

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
    auto hello = testExtensions["aplext:hello:10"].lock();

    inflate();

    // send bad update
    hello->generateLiveDataUpdate("aplext:hello:10", BAD_DATA_UPDATE);
    ASSERT_TRUE(ConsoleMessage());

    // send a good update
    hello->generateLiveDataUpdate("aplext:hello:10", ENTITY_LIST_INSERT);
    ASSERT_FALSE(ConsoleMessage());

    auto event = root->popEvent();
    ASSERT_EQ(event.getType(), kEventTypeSendEvent);
}

TEST_F(ExtensionMediatorTest, RegisterBad) {
    sForceFail = true;
    loadExtensions(EXT_DOC);
    ASSERT_TRUE(ConsoleMessage());
    ASSERT_EQ(0, config->getSupportedExtensions().size());
    sForceFail = false;
}

TEST_F(ExtensionMediatorTest, ComponentReady) {
    loadExtensions(EXT_DOC);

    // verify the extension was registered
    ASSERT_TRUE(extensionProvider->hasExtension("aplext:hello:10"));
    auto ext = extensionProvider->getExtension("aplext:hello:10");
    ASSERT_TRUE(ext);
    // direct access to extension for test inspection
    auto hello = testExtensions["aplext:hello:10"].lock();

    inflate();

    ASSERT_FALSE(hello->mResource);

    auto canvas = root->findComponentById("MyCanvas");
    ASSERT_TRUE(canvas);
    ASSERT_TRUE(IsEqual(kResourcePending, canvas->getCalculated(kPropertyResourceState)));

    canvas->updateResourceState(kResourceReady);
    ASSERT_TRUE(IsEqual(kResourceReady, canvas->getCalculated(kPropertyResourceState)));

    ASSERT_TRUE(hello->mResource);
    ASSERT_TRUE(IsEqual(hello->mResource->resourceId(), canvas->getCalculated(kPropertyResourceId).asString()));
}


static const char* AUDIO_PLAYER = R"(
{
  "type": "APL",
  "version": "1.7",
  "extensions": [
    {
      "name": "AudioPlayer",
      "uri": "aplext:audioplayer:10"
    }
  ],
  "settings": {
    "AudioPlayer": {
      "playbackStateName": "playerStatus"
    }
  },
  "AudioPlayer:OnPlayerActivityUpdated": [
    {
      "type": "SetValue",
      "componentId": "ActivityTxt",
      "property": "text",
      "value": "${playerActivity}"
    },
    {
      "type": "SetValue",
      "componentId": "OffsetTxt",
      "property": "text",
      "value": "${offset}"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Container",
      "items": [
        {
          "type": "TouchWrapper",
          "id": "Touch",
          "width": "100%",
          "height": "100%",
          "onPress": [
            {
              "when": "${playerStatus.playerActivity == 'PLAYING'}",
              "type": "AudioPlayer:Pause"
            },
            {
              "when": "${playerStatus.playerActivity == 'PAUSED'}",
              "type": "AudioPlayer:Play"
            }
          ]
        },
        {
          "type": "Text",
          "id": "ActivityTxt"
        },
        {
          "type": "Text",
          "id": "OffsetTxt"
        }
      ]
    }
  }
}
)";

class AudioPlayerObserverStub : public AudioPlayer::AplAudioPlayerExtensionObserverInterface {
public:
    void onAudioPlayerPlay() override {}
    void onAudioPlayerPause() override {}
    void onAudioPlayerNext() override {}
    void onAudioPlayerPrevious() override {}
    void onAudioPlayerSeekToPosition(int offsetInMilliseconds) override {}
    void onAudioPlayerToggle(const std::string &name, bool checked) override {}
    void onAudioPlayerLyricDataFlushed(const std::string &token,
                                       long durationInMilliseconds,
                                       const std::string &lyricData) override {}
    void onAudioPlayerSkipForward() override {}
    void onAudioPlayerSkipBackward() override {}
};

TEST_F(ExtensionMediatorTest, AudioPlayerIntegration) {

    createProvider();
    auto stub = std::make_shared<AudioPlayerObserverStub>();
    auto extension = std::make_shared<AudioPlayer::AplAudioPlayerExtension>(stub);
    extensionProvider->registerExtension(std::make_shared<LocalExtensionProxy>(extension));
    loadExtensions(AUDIO_PLAYER);

    // The extension was registered
    auto uris = config->getSupportedExtensions();
    ASSERT_EQ(1, uris.size());
    ASSERT_EQ(1, uris.count("aplext:audioplayer:10"));

   auto commands = config->getExtensionCommands();
   ASSERT_EQ(11, commands.size());

    auto events = config->getExtensionEventHandlers();
    ASSERT_EQ(1, events.size());

    auto liveDataMap = config->getLiveObjectMap();
    ASSERT_EQ(1, liveDataMap.size());

    inflate();
    // Validate the Extension environment
    ASSERT_TRUE(evaluate(*context, "${environment.extension.AudioPlayer}").isMap());
    ASSERT_TRUE(IsEqual("APLAudioPlayerExtension-1.0", evaluate(*context, "${environment.extension.AudioPlayer.version}")));

    // Validate Live Data
    extension->updatePlayerActivity("PLAYING", 123);
    ASSERT_FALSE(ConsoleMessage());
    root->clearPending();

    ASSERT_TRUE(evaluate(*context, "${playerStatus}").isTrueMap());
    ASSERT_TRUE(IsEqual("PLAYING", evaluate(*context, "${playerStatus.playerActivity}")));
    ASSERT_TRUE(IsEqual(123, evaluate(*context, "${playerStatus.offset}")));

    auto activityText = root->findComponentById("ActivityTxt");
    ASSERT_TRUE(activityText);
    auto activityOffset = root->findComponentById("OffsetTxt");
    ASSERT_TRUE(activityOffset);
    auto touch = root->findComponentById("Touch");
    ASSERT_TRUE(touch);

    // Basic data is loaded
    ASSERT_TRUE(IsEqual("PLAYING", activityText->getCalculated(kPropertyText).getStyledText().getText()));
    ASSERT_TRUE(IsEqual("123", activityOffset->getCalculated(kPropertyText).getStyledText().getText()));
}

class SimpleExtensionTestAdapter : public ExtensionBase {
public:
    SimpleExtensionTestAdapter(const std::string& uri, const std::string& registrationMessage)
        : ExtensionBase(uri), registrationString(registrationMessage) {}

    std::string registrationString;

    std::map<std::string, std::string> commands;

    rapidjson::Document createRegistration(const std::string& uri, const rapidjson::Value& registrationRequest) override {
        rapidjson::Document doc;
        doc.Parse(registrationString.c_str());
        return doc;
    }

    bool invokeCommand(const std::string& uri, const rapidjson::Value& command) override {
        rapidjson::StringBuffer buffer;
        buffer.Clear();
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        command.Accept(writer);

        std::string commandString(buffer.GetString());
        LOG(LogLevel::kInfo) << "uri: " << uri << ", command: " << commandString;
        commands.emplace(command["name"].GetString(), commandString);
        return false;
    }

    void onRegistered(const std::string& uri, const std::string& token) override {
        LOG(LogLevel::kInfo) << "uri: " << uri << ", token: " << token;
    }

    void onUnregistered(const std::string& uri, const std::string& token) override {
        LOG(LogLevel::kInfo) << "uri: " << uri << ", token: " << token;
    }

    bool updateComponent(const std::string& uri, const rapidjson::Value& command) override {
        rapidjson::StringBuffer buffer;
        buffer.Clear();
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        command.Accept(writer);

        LOG(LogLevel::kInfo) << "uri: " << uri << ", command: " << buffer.GetString();
        return true;
    }

    void onResourceReady(const std::string& uri, const ResourceHolderPtr& resourceHolder) override {
        LOG(LogLevel::kInfo) << "uri: " << uri << ", resource: " << resourceHolder->resourceId();
    }

    void sendEvent(const std::string& uri, const rapidjson::Value& event) {
        invokeExtensionEventHandler(uri, event);
    }

};

class ExtensionCommunicationTestAdapter : public ExtensionProxy {
public:
    ExtensionCommunicationTestAdapter(const std::string& uri, bool shouldInitialize, bool shouldRegister) :
        mShouldInitialize(shouldInitialize), mShouldRegister(shouldRegister) {
        mURIs.emplace(uri);
    }

    std::set<std::string> getURIs() const override { return mURIs; }

    bool initializeExtension(const std::string &uri) override {
        if (mShouldInitialize) {
            mInitialized.emplace(uri);
        }
        return mShouldInitialize;
    }

    bool isInitialized(const std::string &uri) const override {
        return mInitialized.count(uri);
    }

    bool getRegistration(const std::string &uri, const rapidjson::Value &registrationRequest,
                         RegistrationSuccessCallback success, RegistrationFailureCallback error) override {
        mRegistrationSuccess = success;
        mRegistrationError = error;
        if (mShouldRegister) {
            auto request = AsPrettyString(registrationRequest);
            mPendingRegistrations.emplace(uri, request);
        }
        return mShouldRegister;
    }

    bool invokeCommand(const std::string &uri, const rapidjson::Value &command,
                               CommandSuccessCallback success, CommandFailureCallback error) override { return false; }

    bool sendMessage(const std::string &uri, const rapidjson::Value &message) override { return false; }

    void registerEventCallback(Extension::EventCallback callback) override {}

    void registerLiveDataUpdateCallback(Extension::LiveDataUpdateCallback callback) override {}

    void onRegistered(const std::string &uri, const std::string &token) override {
        mRegistered.emplace(uri, token);
    }

    void onUnregistered(const std::string &uri, const std::string &token) override {
        mRegistered.erase(uri);
    }

    void onResourceReady( const std::string& uri,
                                  const ResourceHolderPtr& resource) override {}

    bool isInitialized(const std::string& uri) { return mInitialized.count(uri); }

    bool isRegistered(const std::string& uri) { return mRegistered.count(uri); }

    void registrationSuccess(const std::string &uri, const rapidjson::Value &registrationSuccess) {
        mRegistrationSuccess(uri, registrationSuccess);
    }

    void registrationError(const std::string &uri, const rapidjson::Value &registrationError) {
        mRegistrationError(uri, registrationError);
    }

    bool hasPendingRequest(const std::string &uri) {
        return mPendingRegistrations.count(uri);
    }

    std::string getPendingRequest(const std::string &uri) {
        return mPendingRegistrations.find(uri)->second;
    }

private:
    std::set<std::string> mURIs;
    std::set<std::string> mInitialized;
    bool mShouldInitialize;
    bool mShouldRegister;
    RegistrationSuccessCallback mRegistrationSuccess;
    RegistrationFailureCallback mRegistrationError;
    std::map<std::string, std::string> mRegistered;
    std::map<std::string, std::string> mPendingRegistrations;
};

static const char* SIMPLE_EXT_DOC = R"({
  "type": "APL",
  "version": "1.8",
  "extension": [
      {
        "uri": "alexaext:test:10",
        "name": "Test"
      }
  ],
  "settings": {
    "Test": {
      "authorizationCode": "MAGIC"
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": 500,
      "height": 500,
      "items": []
    }
  }
})";

static const char* TEST_EXTENSION_URI = "alexaext:test:10";

TEST_F(ExtensionMediatorTest, TestRegistrationSchema) {
    createProvider();

    auto adapter = std::make_shared<ExtensionCommunicationTestAdapter>(TEST_EXTENSION_URI, true, true);
    extensionProvider->registerExtension(adapter);

    createContent(SIMPLE_EXT_DOC, nullptr);
    mediator->initializeExtensions(config, content);
    config->registerExtensionFlags(TEST_EXTENSION_URI, "--testflag");
    mediator->loadExtensions(config, content, [](){});

    ASSERT_TRUE(adapter->hasPendingRequest(TEST_EXTENSION_URI));
    auto registerRequest = adapter->getPendingRequest(TEST_EXTENSION_URI);

    rapidjson::Document requestJson;
    requestJson.Parse(registerRequest.c_str());

    //mandatory fields
    ASSERT_TRUE(requestJson.HasMember("uri"));
    ASSERT_STREQ(TEST_EXTENSION_URI, requestJson["uri"].GetString());
    ASSERT_TRUE(requestJson.HasMember("method"));
    ASSERT_STREQ("Register", requestJson["method"].GetString());
    ASSERT_TRUE(requestJson.HasMember("version"));
    ASSERT_STREQ("1.0", requestJson["version"].GetString());

    //optional fields
    ASSERT_TRUE(requestJson.HasMember("settings"));
    rapidjson::Value& settings = requestJson["settings"];
    ASSERT_TRUE(settings.HasMember("authorizationCode"));
    ASSERT_STREQ("MAGIC", settings["authorizationCode"].GetString());
    ASSERT_TRUE(requestJson.HasMember("flags"));
    ASSERT_STREQ("--testflag", requestJson["flags"].GetString());
}

TEST_F(ExtensionMediatorTest, FastInitialization) {
    createProvider();

    auto adapter = std::make_shared<ExtensionCommunicationTestAdapter>(TEST_EXTENSION_URI, true, true);
    extensionProvider->registerExtension(adapter);

    createContent(SIMPLE_EXT_DOC, nullptr);

    // Experimental feature required
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
            .extensionProvider(extensionProvider)
            .extensionMediator(mediator);

    ASSERT_TRUE(content->isReady());
    mediator->initializeExtensions(config, content);

    ASSERT_TRUE(adapter->isInitialized(TEST_EXTENSION_URI));

    auto loaded = std::make_shared<bool>(false);
    mediator->loadExtensions(config, content, [loaded](){
        *loaded = true;
    });

    ASSERT_FALSE(adapter->isRegistered(TEST_EXTENSION_URI));
    ASSERT_FALSE(*loaded);

    rapidjson::Document schemaDoc;
    auto schema = ExtensionSchema(&schemaDoc.GetAllocator(), "1.0").uri(TEST_EXTENSION_URI);
    auto success = RegistrationSuccess("1.0").token("MAGIC_TOKEN").schema(schema);
    adapter->registrationSuccess(TEST_EXTENSION_URI, success.getDocument());

    ASSERT_TRUE(adapter->isRegistered(TEST_EXTENSION_URI));
    ASSERT_TRUE(*loaded);

    // Finalize now
    mediator->finish();
    ASSERT_FALSE(adapter->isRegistered(TEST_EXTENSION_URI));
}

TEST_F(ExtensionMediatorTest, FastInitializationFailInitialize) {
    createProvider();

    auto adapter = std::make_shared<ExtensionCommunicationTestAdapter>(TEST_EXTENSION_URI, false, false);
    extensionProvider->registerExtension(adapter);

    createContent(SIMPLE_EXT_DOC, nullptr);

    // Experimental feature required
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
            .extensionProvider(extensionProvider)
            .extensionMediator(mediator);

    ASSERT_TRUE(content->isReady());
    mediator->initializeExtensions(config, content);

    ASSERT_FALSE(adapter->isInitialized(TEST_EXTENSION_URI));

    auto loaded = std::make_shared<bool>(false);
    mediator->loadExtensions(config, content, [loaded](){
        *loaded = true;
    });

    ASSERT_FALSE(adapter->isRegistered(TEST_EXTENSION_URI));
    // Still considered loaded. Extension just not available.
    ASSERT_TRUE(*loaded);
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(ExtensionMediatorTest, FastInitializationFailRegistrationRequest) {
    createProvider();

    auto adapter = std::make_shared<ExtensionCommunicationTestAdapter>(TEST_EXTENSION_URI, true, false);
    extensionProvider->registerExtension(adapter);

    createContent(SIMPLE_EXT_DOC, nullptr);

    // Experimental feature required
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
            .extensionProvider(extensionProvider)
            .extensionMediator(mediator);

    ASSERT_TRUE(content->isReady());
    mediator->initializeExtensions(config, content);

    ASSERT_TRUE(adapter->isInitialized(TEST_EXTENSION_URI));

    auto loaded = std::make_shared<bool>(false);
    mediator->loadExtensions(config, content, [loaded](){
        *loaded = true;
    });

    ASSERT_FALSE(adapter->isRegistered(TEST_EXTENSION_URI));
    ASSERT_TRUE(*loaded);
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(ExtensionMediatorTest, FastInitializationFailRegistration) {
    createProvider();

    auto adapter = std::make_shared<ExtensionCommunicationTestAdapter>(TEST_EXTENSION_URI, true, true);
    extensionProvider->registerExtension(adapter);

    createContent(SIMPLE_EXT_DOC, nullptr);

    // Experimental feature required
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
            .extensionProvider(extensionProvider)
            .extensionMediator(mediator);

    ASSERT_TRUE(content->isReady());
    mediator->initializeExtensions(config, content);

    ASSERT_TRUE(adapter->isInitialized(TEST_EXTENSION_URI));

    auto loaded = std::make_shared<bool>(false);
    mediator->loadExtensions(config, content, [loaded](){
        *loaded = true;
    });

    ASSERT_FALSE(adapter->isRegistered(TEST_EXTENSION_URI));
    ASSERT_FALSE(*loaded);

    auto fail = RegistrationFailure("1.0")
            .errorCode(alexaext::ExtensionError::kErrorException)
            .errorMessage(sErrorMessage.at(ExtensionError::kErrorException));

    adapter->registrationError(TEST_EXTENSION_URI, fail.getDocument());

    ASSERT_FALSE(adapter->isRegistered(TEST_EXTENSION_URI));
    ASSERT_TRUE(*loaded);
}


TEST_F(ExtensionMediatorTest, FastInitializationGranted) {
    createProvider();

    auto adapter = std::make_shared<ExtensionCommunicationTestAdapter>(TEST_EXTENSION_URI, true, true);
    extensionProvider->registerExtension(adapter);

    createContent(SIMPLE_EXT_DOC, nullptr);

    // Experimental feature required
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
        .extensionProvider(extensionProvider)
        .extensionMediator(mediator);

    ASSERT_TRUE(content->isReady());

    // grant extension access
    mediator->initializeExtensions(config, content,
        [](const std::string& uri, ExtensionMediator::ExtensionGrantResult grant,
           ExtensionMediator::ExtensionGrantResult deny) {
          grant(uri);
        });

    ASSERT_TRUE(adapter->isInitialized(TEST_EXTENSION_URI));

    auto loaded = std::make_shared<bool>(false);
    mediator->loadExtensions(config, content, [loaded](){
      *loaded = true;
    });

    ASSERT_FALSE(adapter->isRegistered(TEST_EXTENSION_URI));
    ASSERT_FALSE(*loaded);

    rapidjson::Document schemaDoc;
    auto schema = ExtensionSchema(&schemaDoc.GetAllocator(), "1.0").uri(TEST_EXTENSION_URI);
    auto success = RegistrationSuccess("1.0").token("MAGIC_TOKEN").schema(schema);
    adapter->registrationSuccess(TEST_EXTENSION_URI, success.getDocument());

    ASSERT_TRUE(adapter->isRegistered(TEST_EXTENSION_URI));
    ASSERT_TRUE(*loaded);

}


TEST_F(ExtensionMediatorTest, FastInitializationDenied) {
    createProvider();

    auto adapter = std::make_shared<ExtensionCommunicationTestAdapter>(TEST_EXTENSION_URI, true, true);
    extensionProvider->registerExtension(adapter);

    createContent(SIMPLE_EXT_DOC, nullptr);

    // Experimental feature required
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
        .extensionProvider(extensionProvider)
        .extensionMediator(mediator);

    ASSERT_TRUE(content->isReady());

    // deny extension access
    mediator->initializeExtensions(config, content,
        [](const std::string& uri, ExtensionMediator::ExtensionGrantResult grant,
           ExtensionMediator::ExtensionGrantResult deny) {
          deny(uri);
        });

    ASSERT_FALSE(adapter->isInitialized(TEST_EXTENSION_URI));
}


TEST_F(ExtensionMediatorTest, FastInitializationMissingGrant) {
    createProvider();

    auto adapter = std::make_shared<ExtensionCommunicationTestAdapter>(TEST_EXTENSION_URI, true, true);
    extensionProvider->registerExtension(adapter);

    createContent(SIMPLE_EXT_DOC, nullptr);

    // Experimental feature required
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
        .extensionProvider(extensionProvider)
        .extensionMediator(mediator);

    ASSERT_TRUE(content->isReady());

    // grant extension access
    auto grantRequest = std::make_shared<bool>(false);
    mediator->initializeExtensions(config, content,
                                   [grantRequest](const std::string& uri, ExtensionMediator::ExtensionGrantResult grant,
                                      ExtensionMediator::ExtensionGrantResult deny) {
                                      //neither grant nor deny
                                      *grantRequest = true;
                                   });
    ASSERT_TRUE(grantRequest);
    ASSERT_FALSE(adapter->isInitialized(TEST_EXTENSION_URI));

    auto loaded = std::make_shared<bool>(false);
    mediator->loadExtensions(config, content, [loaded](){
      *loaded = true;
    });
    ASSERT_TRUE(LogMessage());

    ASSERT_TRUE(*loaded);
    ASSERT_FALSE(adapter->isRegistered(TEST_EXTENSION_URI));
}

TEST_F(ExtensionMediatorTest, RootConfigNull) {
    createProvider();

    auto adapter = std::make_shared<ExtensionCommunicationTestAdapter>(TEST_EXTENSION_URI, true, true);
    extensionProvider->registerExtension(adapter);

    createContent(SIMPLE_EXT_DOC, nullptr);

    // Experimental feature required
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
        .extensionProvider(extensionProvider)
        .extensionMediator(mediator);

    ASSERT_TRUE(content->isReady());

    // grant extension access
    auto grantRequest = std::make_shared<bool>(false);
    mediator->initializeExtensions(config, content,
                                   [grantRequest](const std::string& uri, ExtensionMediator::ExtensionGrantResult grant,
                                      ExtensionMediator::ExtensionGrantResult deny) {
                                      //neither grant nor deny
                                      *grantRequest = true;
                                   });
    ASSERT_TRUE(grantRequest);
    ASSERT_FALSE(adapter->isInitialized(TEST_EXTENSION_URI));

    auto loaded = std::make_shared<bool>(false);
    mediator->loadExtensions(nullptr, content, [loaded](){
      *loaded = true;
    });
    ASSERT_TRUE(LogMessage());

    ASSERT_TRUE(*loaded);
    ASSERT_FALSE(adapter->isRegistered(TEST_EXTENSION_URI));
}

TEST_F(ExtensionMediatorTest, LoadGranted) {
    createProvider();

    auto adapter = std::make_shared<ExtensionCommunicationTestAdapter>(TEST_EXTENSION_URI, true, true);
    extensionProvider->registerExtension(adapter);

    createContent(SIMPLE_EXT_DOC, nullptr);

    // Experimental feature required
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
        .extensionProvider(extensionProvider)
        .extensionMediator(mediator);

    ASSERT_TRUE(content->isReady());

    // explicit grant of test extensions
    auto granted = adapter->getURIs();
    mediator->loadExtensions(config, content, &granted);

    ASSERT_TRUE(adapter->isInitialized(TEST_EXTENSION_URI));

    rapidjson::Document schemaDoc;
    auto schema = ExtensionSchema(&schemaDoc.GetAllocator(), "1.0").uri(TEST_EXTENSION_URI);
    auto success = RegistrationSuccess("1.0").token("MAGIC_TOKEN").schema(schema);
    adapter->registrationSuccess(TEST_EXTENSION_URI, success.getDocument());

    ASSERT_TRUE(adapter->isRegistered(TEST_EXTENSION_URI));

}

TEST_F(ExtensionMediatorTest, LoadDenied) {
    createProvider();

    auto adapter = std::make_shared<ExtensionCommunicationTestAdapter>(TEST_EXTENSION_URI, true, true);
    extensionProvider->registerExtension(adapter);

    createContent(SIMPLE_EXT_DOC, nullptr);

    // Experimental feature required
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
        .extensionProvider(extensionProvider)
        .extensionMediator(mediator);

    ASSERT_TRUE(content->isReady());

    // empty set results in all extension denied
    std::set<std::string> granted;
    mediator->loadExtensions(config, content, &granted);

    ASSERT_FALSE(adapter->isInitialized(TEST_EXTENSION_URI));
}

TEST_F(ExtensionMediatorTest, LoadAllGranted) {
    createProvider();

    auto adapter = std::make_shared<ExtensionCommunicationTestAdapter>(TEST_EXTENSION_URI, true, true);
    extensionProvider->registerExtension(adapter);

    createContent(SIMPLE_EXT_DOC, nullptr);

    // Experimental feature required
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
        .extensionProvider(extensionProvider)
        .extensionMediator(mediator);

    ASSERT_TRUE(content->isReady());

    // when content ready, unspecified grant list means all extensions granted
    mediator->loadExtensions(config, content);

    ASSERT_TRUE(adapter->isInitialized(TEST_EXTENSION_URI));

    rapidjson::Document schemaDoc;
    auto schema = ExtensionSchema(&schemaDoc.GetAllocator(), "1.0").uri(TEST_EXTENSION_URI);
    auto success = RegistrationSuccess("1.0").token("MAGIC_TOKEN").schema(schema);
    adapter->registrationSuccess(TEST_EXTENSION_URI, success.getDocument());

    ASSERT_TRUE(adapter->isRegistered(TEST_EXTENSION_URI));
}


TEST_F(ExtensionMediatorTest, LoadContentNotReady) {
    createProvider();

    auto adapter = std::make_shared<ExtensionCommunicationTestAdapter>(TEST_EXTENSION_URI, true, true);
    extensionProvider->registerExtension(adapter);

    // Experimental feature required
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
        .extensionProvider(extensionProvider)
        .extensionMediator(mediator);

    const char* DOC = R"(
        {
          "type": "APL",
          "version": "1.1",
          "mainTemplate": {
            "parameters": [
              "payload"
            ],
            "item": {
              "type": "Text"
            }
          }
        }
    )";

    createContent(DOC, nullptr);
    ASSERT_FALSE(content->isReady());

    // when content ready, unspecified grant list means all extensions granted
    // without ready content load not attempted
    mediator->loadExtensions(config, content);

    ASSERT_TRUE(ConsoleMessage());
    ASSERT_FALSE(adapter->isInitialized(TEST_EXTENSION_URI));
}



static const char* SIMPLE_COMPONENT_DOC = R"({
  "type": "APL",
  "version": "1.9",
  "theme": "dark",
  "extensions": [
    {
      "uri": "alexaext:example:10",
      "name": "Example"
    }
  ],
  "settings": {
    "Example": {
      "some": "setting"
    }
  },
  "mainTemplate": {
    "item": {
      "type": "TouchWrapper",
      "width": "100vw",
      "height": "100vh",
      "items": [
        {
          "when": "${environment.extension.Example}",
          "type": "Example:Example",
          "id": "ExampleComp",
          "width": "100%",
          "height": "100%",
          "onMount": [
            {
              "type": "Example:Hello"
            }
          ],
          "ComponentEvent": {
            "type": "SendEvent"
          }
        }
      ]
    }
  }
})";

static const char* SIMPLE_COMPONENT_SCHEMA = R"({
  "version": "1.0",
  "method": "RegisterSuccess",
  "token": "<AUTO_TOKEN>",
  "environment": {
    "version": "1.0"
  },
  "schema": {
    "type": "Schema",
    "version": "1.0",
    "uri": "alexaext:example:10",
    "components": [
      {
        "name": "Example",
        "resourceType": "Custom",
        "commands": [
          {
            "name": "Hello"
          }
        ],
        "events": [
          { "name": "ComponentEvent", "mode": "NORMAL" }
        ]
      },
      {
        "name": "AnotherExample",
        "resourceType": "Custom",
        "commands": [
          {
            "name": "Goodbye"
          }
        ]
      }
    ]
  }
})";

static const char* COMPONENT_TARGET_EVENT = R"({
  "version": "1.0",
  "method": "Event",
  "target": "alexaext:example:10",
  "name": "ComponentEvent",
  "resourceId": "[RESOURCE_ID]"
})";

TEST_F(ExtensionMediatorTest, ComponentInteractions) {
    extensionProvider = std::make_shared<alexaext::ExtensionRegistrar>();
    mediator = ExtensionMediator::create(extensionProvider, alexaext::Executor::getSynchronousExecutor());

    auto extension = std::make_shared<SimpleExtensionTestAdapter>("alexaext:example:10", SIMPLE_COMPONENT_SCHEMA);
    extensionProvider->registerExtension(std::make_shared<LocalExtensionProxy>(extension));

    createContent(SIMPLE_COMPONENT_DOC, nullptr);

    // Experimental feature required
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
            .extensionProvider(extensionProvider)
            .extensionMediator(mediator);

    ASSERT_TRUE(content->isReady());
    mediator->initializeExtensions(config, content);

    auto loaded = std::make_shared<bool>(false);
    mediator->loadExtensions(config, content, [loaded](){
        *loaded = true;
    });

    ASSERT_TRUE(*loaded);

    inflate();
    ASSERT_TRUE(root);
    advanceTime(10);

    // On mount
    ASSERT_EQ(1, extension->commands.size());
    ASSERT_EQ("Hello", extension->commands.begin()->first);

    // Invoke component event
    rapidjson::Document componentEvent;
    componentEvent.Parse(COMPONENT_TARGET_EVENT);
    componentEvent["resourceId"].SetString(component->getCoreChildAt(0)->getCalculated(kPropertyResourceId).asString().c_str(), componentEvent.GetAllocator());;
    extension->sendEvent("alexaext:example:10", componentEvent);

    advanceTime(10);
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    ASSERT_TRUE(ConsoleMessage());
}

static const char* SIMPLE_COMPONENT_COMMANDS = R"({
  "type": "APL",
  "version": "1.9",
  "theme": "dark",
  "extensions": [
    {
      "uri": "alexaext:example:10",
      "name": "Example"
    }
  ],
  "settings": {
    "Example": {
      "some": "setting"
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": "100vw",
      "height": "100vh",
      "items": [
        {
          "type": "Container",
          "width": "100vw",
          "height": "100vh",
          "items": [
            {
              "type": "TouchWrapper",
              "width": "100%",
              "height": 100,
              "onPress": {
                "type": "Example:Hello"
              }
            },
            {
              "type": "TouchWrapper",
              "width": "100%",
              "height": 100,
              "onPress": {
                "type": "Example:Hello",
                "componentId": "ExampleComp"
              }
            },
            {
              "type": "TouchWrapper",
              "width": "100%",
              "height": 100,
              "onPress": {
                "type": "Example:Hello",
                "componentId": "AnotherExampleComp"
              }
            }
          ]
        },
        {
          "when": "${environment.extension.Example}",
          "type": "Example:Example",
          "id": "ExampleComp",
          "width": "100%",
          "height": 100
        },
        {
          "when": "${environment.extension.Example}",
          "type": "Example:AnotherExample",
          "id": "AnotherExampleComp",
          "width": "100%",
          "height": 100
        }
      ]
    }
  }
})";

TEST_F(ExtensionMediatorTest, ComponentCommands) {
    extensionProvider = std::make_shared<alexaext::ExtensionRegistrar>();
    mediator = ExtensionMediator::create(extensionProvider, alexaext::Executor::getSynchronousExecutor());

    auto extension = std::make_shared<SimpleExtensionTestAdapter>("alexaext:example:10", SIMPLE_COMPONENT_SCHEMA);
    extensionProvider->registerExtension(std::make_shared<LocalExtensionProxy>(extension));

    createContent(SIMPLE_COMPONENT_COMMANDS, nullptr);

    // Experimental feature required
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
            .extensionProvider(extensionProvider)
            .extensionMediator(mediator);

    ASSERT_TRUE(content->isReady());
    mediator->initializeExtensions(config, content);

    auto loaded = std::make_shared<bool>(false);
    auto callCount = std::make_shared<int>(0);
    mediator->loadExtensions(config, content, [loaded, callCount](){
        *loaded = true;
        (*callCount)++;
    });

    ASSERT_TRUE(*loaded);
    // The ExtensionsLoadedCallback should be called only once for synchronous task executor.
    ASSERT_EQ(1, *callCount);

    inflate();
    ASSERT_TRUE(root);
    advanceTime(10);

    // Component command without component should work, but will not include anything component specific.
    performTap(10, 10);
    advanceTime(10);

    ASSERT_EQ(1, extension->commands.size());
    auto entry = extension->commands.begin();
    ASSERT_EQ("Hello", entry->first);
    ASSERT_TRUE(entry->second.find("resourceId") == std::string::npos);
    extension->commands.erase("Hello");

    // Component command targetting wrong component should still work.
    performTap(10, 210);
    advanceTime(10);

    ASSERT_EQ(1, extension->commands.size());
    entry = extension->commands.begin();
    ASSERT_EQ("Hello", entry->first);
    ASSERT_TRUE(entry->second.find("resourceId") != std::string::npos);
    extension->commands.erase("Hello");

    // Component command targetting it's own component should work.
    performTap(10, 110);
    advanceTime(10);

    ASSERT_EQ(1, extension->commands.size());
    entry = extension->commands.begin();
    ASSERT_EQ("Hello", entry->first);
    ASSERT_TRUE(entry->second.find("resourceId") != std::string::npos);
    extension->commands.erase("Hello");
    ASSERT_TRUE(ConsoleMessage());
}

static const char* COMPONENT_EVENT_DOC = R"({
  "type": "APL",
  "version": "1.9",
  "theme": "dark",
  "extensions": [
    {
      "uri": "alexaext:example:10",
      "name": "Example"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Example:Example",
      "id": "ExampleComp",
      "width": "100%",
      "height": "100%",
      "ComponentEvent": {
        "type": "SendEvent",
        "arguments": ["${event.potato}"]
      }
    }
  },
  "Example:DocumentEvent": {
    "type": "SendEvent",
    "arguments": ["${event.potato}"]
  }
})";

static const char* COMPONENT_EVENT_SCHEMA = R"({
  "version": "1.0",
  "method": "RegisterSuccess",
  "token": "<AUTO_TOKEN>",
  "environment": {
    "version": "1.0"
  },
  "schema": {
    "type": "Schema",
    "version": "1.0",
    "uri": "alexaext:example:10",
    "events": [
      { "name": "DocumentEvent", "mode": "NORMAL" }
    ],
    "components": [
      {
        "name": "Example",
        "resourceType": "Custom",
        "events": [
          { "name": "ComponentEvent", "mode": "NORMAL" }
        ]
      }
    ]
  }
})";

static const char* COMPONENT_TARGET_EVENT_WITH_ARGUMENTS = R"({
  "version": "1.0",
  "method": "Event",
  "target": "alexaext:example:10",
  "name": "ComponentEvent",
  "resourceId": "[RESOURCE_ID]",
  "payload": {
    "potato": "tasty"
  }
})";

TEST_F(ExtensionMediatorTest, ComponentEventCorrect) {
    extensionProvider = std::make_shared<alexaext::ExtensionRegistrar>();
    mediator = ExtensionMediator::create(extensionProvider, alexaext::Executor::getSynchronousExecutor());

    auto extension = std::make_shared<SimpleExtensionTestAdapter>("alexaext:example:10", COMPONENT_EVENT_SCHEMA);
    extensionProvider->registerExtension(std::make_shared<LocalExtensionProxy>(extension));

    createContent(COMPONENT_EVENT_DOC, nullptr);

    // Experimental feature required
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
            .extensionProvider(extensionProvider)
            .extensionMediator(mediator);

    ASSERT_TRUE(content->isReady());
    mediator->initializeExtensions(config, content);

    auto loaded = std::make_shared<bool>(false);
    mediator->loadExtensions(config, content, [loaded](){
        *loaded = true;
    });

    ASSERT_TRUE(*loaded);

    inflate();
    ASSERT_TRUE(root);
    advanceTime(10);

    // Invoke component event
    rapidjson::Document componentEvent;
    componentEvent.Parse(COMPONENT_TARGET_EVENT_WITH_ARGUMENTS);
    auto resourceId = component->getCalculated(kPropertyResourceId).asString();
    componentEvent["resourceId"].SetString(resourceId.c_str(), componentEvent.GetAllocator());
    extension->sendEvent("alexaext:example:10", componentEvent);

    advanceTime(10);
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    auto& map = event.getValue(kEventPropertySource).getMap();
    ASSERT_EQ("Example", map.at("type").getString());
    ASSERT_EQ("ComponentEvent", map.at("handler").getString());
    ASSERT_EQ(resourceId, map.at("resourceId").getString());

    auto& array = event.getValue(kEventPropertyArguments).getArray();
    ASSERT_EQ("tasty", array.at(0).getString());
}

static const char* COMPONENT_TARGET_EVENT_TARGETLESS = R"({
  "version": "1.0",
  "method": "Event",
  "target": "alexaext:example:10",
  "name": "ComponentEvent"
})";

TEST_F(ExtensionMediatorTest, ComponentEventWithoutResource) {
    extensionProvider = std::make_shared<alexaext::ExtensionRegistrar>();
    mediator = ExtensionMediator::create(extensionProvider, alexaext::Executor::getSynchronousExecutor());

    auto extension = std::make_shared<SimpleExtensionTestAdapter>("alexaext:example:10", COMPONENT_EVENT_SCHEMA);
    extensionProvider->registerExtension(std::make_shared<LocalExtensionProxy>(extension));

    createContent(COMPONENT_EVENT_DOC, nullptr);

    // Experimental feature required
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
            .extensionProvider(extensionProvider)
            .extensionMediator(mediator);

    ASSERT_TRUE(content->isReady());
    mediator->initializeExtensions(config, content);

    auto loaded = std::make_shared<bool>(false);
    mediator->loadExtensions(config, content, [loaded](){
        *loaded = true;
    });

    ASSERT_TRUE(*loaded);

    inflate();
    ASSERT_TRUE(root);
    advanceTime(10);

    // Invoke component event
    rapidjson::Document componentEvent;
    componentEvent.Parse(COMPONENT_TARGET_EVENT_TARGETLESS);
    extension->sendEvent("alexaext:example:10", componentEvent);

    advanceTime(10);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());
}

static const char* DOCUMENT_TARGET_EVENT_WITH_ARGUMENTS = R"({
  "version": "1.0",
  "method": "Event",
  "target": "alexaext:example:10",
  "name": "DocumentEvent",
  "payload": {
    "potato": "tasty"
  }
})";

TEST_F(ExtensionMediatorTest, DocumentEventCorrect) {
    extensionProvider = std::make_shared<alexaext::ExtensionRegistrar>();
    mediator = ExtensionMediator::create(extensionProvider, alexaext::Executor::getSynchronousExecutor());

    auto extension = std::make_shared<SimpleExtensionTestAdapter>("alexaext:example:10", COMPONENT_EVENT_SCHEMA);
    extensionProvider->registerExtension(std::make_shared<LocalExtensionProxy>(extension));

    createContent(COMPONENT_EVENT_DOC, nullptr);

    // Experimental feature required
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
            .extensionProvider(extensionProvider)
            .extensionMediator(mediator);

    ASSERT_TRUE(content->isReady());
    mediator->initializeExtensions(config, content);

    auto loaded = std::make_shared<bool>(false);
    mediator->loadExtensions(config, content, [loaded](){
        *loaded = true;
    });

    ASSERT_TRUE(*loaded);

    inflate();
    ASSERT_TRUE(root);
    advanceTime(10);

    // Invoke component event
    rapidjson::Document documentEvent;
    documentEvent.Parse(DOCUMENT_TARGET_EVENT_WITH_ARGUMENTS);
    extension->sendEvent("alexaext:example:10", documentEvent);

    advanceTime(10);
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    auto& map = event.getValue(kEventPropertySource).getMap();
    ASSERT_EQ("Document", map.at("type").getString());
    ASSERT_EQ("DocumentEvent", map.at("handler").getString());

    auto& array = event.getValue(kEventPropertyArguments).getArray();
    ASSERT_EQ("tasty", array.at(0).getString());
}

static const char* DOCUMENT_TARGET_EVENT_WITH_RESOURCE_ID = R"({
  "version": "1.0",
  "method": "Event",
  "target": "alexaext:example:10",
  "name": "DocumentEvent",
  "resourceId": "[RESOURCE_ID]"
})";

TEST_F(ExtensionMediatorTest, DocumentEventWithResourceId) {
    extensionProvider = std::make_shared<alexaext::ExtensionRegistrar>();
    mediator = ExtensionMediator::create(extensionProvider, alexaext::Executor::getSynchronousExecutor());

    auto extension = std::make_shared<SimpleExtensionTestAdapter>("alexaext:example:10", COMPONENT_EVENT_SCHEMA);
    extensionProvider->registerExtension(std::make_shared<LocalExtensionProxy>(extension));

    createContent(COMPONENT_EVENT_DOC, nullptr);

    // Experimental feature required
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
            .extensionProvider(extensionProvider)
            .extensionMediator(mediator);

    ASSERT_TRUE(content->isReady());
    mediator->initializeExtensions(config, content);

    auto loaded = std::make_shared<bool>(false);
    mediator->loadExtensions(config, content, [loaded](){
        *loaded = true;
    });

    ASSERT_TRUE(*loaded);

    inflate();
    ASSERT_TRUE(root);
    advanceTime(10);

    // Invoke document event
    rapidjson::Document documentEvent;
    documentEvent.Parse(DOCUMENT_TARGET_EVENT_WITH_RESOURCE_ID);
    auto resourceId = component->getCalculated(kPropertyResourceId).asString();
    documentEvent["resourceId"].SetString(resourceId.c_str(), documentEvent.GetAllocator());
    extension->sendEvent("alexaext:example:10", documentEvent);

    advanceTime(10);
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());
}

class FastEventExtensionTestAdapter : public SimpleExtensionTestAdapter {
public:
    FastEventExtensionTestAdapter(const std::string& uri,
                                  const std::string& registrationMessage,
                                  const std::string& eventMessage)
            : SimpleExtensionTestAdapter(uri, registrationMessage), eventString(eventMessage) {}

    std::string eventString;

    void onRegistered(const std::string& uri, const std::string& token) override {
        SimpleExtensionTestAdapter::onRegistered(uri, token);
        rapidjson::Document doc;
        doc.Parse(eventString.c_str());
        sendEvent(uri, doc);
    }
};

TEST_F(ExtensionMediatorTest, DocumentEventBeforeRegistrationFinished) {
    extensionProvider = std::make_shared<alexaext::ExtensionRegistrar>();
    mediator = ExtensionMediator::create(extensionProvider, alexaext::Executor::getSynchronousExecutor());

    auto extension = std::make_shared<FastEventExtensionTestAdapter>("alexaext:example:10", COMPONENT_EVENT_SCHEMA, DOCUMENT_TARGET_EVENT_WITH_ARGUMENTS);
    extensionProvider->registerExtension(std::make_shared<LocalExtensionProxy>(extension));

    createContent(COMPONENT_EVENT_DOC, nullptr);

    // Experimental feature required
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
            .extensionProvider(extensionProvider)
            .extensionMediator(mediator);

    ASSERT_TRUE(content->isReady());
    mediator->initializeExtensions(config, content);

    auto loaded = std::make_shared<bool>(false);
    mediator->loadExtensions(config, content, [loaded](){
        *loaded = true;
    });

    ASSERT_TRUE(*loaded);

    inflate();
    ASSERT_TRUE(root);

    // Event should have invoked about now.
    advanceTime(10);
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    auto& map = event.getValue(kEventPropertySource).getMap();
    ASSERT_EQ("Document", map.at("type").getString());
    ASSERT_EQ("DocumentEvent", map.at("handler").getString());

    auto& array = event.getValue(kEventPropertyArguments).getArray();
    ASSERT_EQ("tasty", array.at(0).getString());
}

#endif