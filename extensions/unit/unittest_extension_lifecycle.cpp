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

#include <alexaext/alexaext.h>
#include <rapidjson/document.h>

#include "gtest/gtest.h"

using namespace alexaext;

namespace {

static const char *URI = "test:lifecycle:1.0";

static const char *COMMAND_MESSAGE = R"(
        {
            "version": "1.0",
            "method": "Command",
            "payload": {},
            "uri": "test:lifecycle:1.0",
            "target": "test:lifecycle:1.0",
            "id": 42,
            "name": "TestCommand"
        }
    )";

static const char *EVENT_MESSAGE = R"(
        {
            "version": "1.0",
            "method": "Event",
            "payload": {},
            "uri": "test:lifecycle:1.0",
            "target": "test:lifecycle:1.0",
            "name": "TestEvent"
        }
    )";

static const char * LIVE_DATA_MESSAGE = R"(
    {
        "version": "1.0",
        "method": "LiveDataUpdate",
        "operations": [
            {
                "type": "Insert",
                "index": 1,
                "item": 1
            }
        ],
        "uri": "test:lifecycle:1.0",
        "target": "test:lifecycle:1.0",
        "name": "MyLiveArray"
    }
)";

static const char *UPDATE_COMPONENT_MESSAGE = R"(
    {
        "version": "1.0",
        "method": "Component",
        "uri": "test:lifecycle:1.0",
        "target": "test:lifecycle:1.0",
        "resourceId": "SURFACE42",
        "state": "Ready"
    }
)";

class LegacyExtension : public alexaext::ExtensionBase {
public:
    explicit LegacyExtension() : ExtensionBase(URI) {}

    rapidjson::Document createRegistration(const std::string& uri,
                                           const rapidjson::Value& registerRequest) override {
        return RegistrationSuccess("1.0")
            .uri(uri)
            .token("<AUTO_TOKEN>")
            .schema("1.0", [uri](ExtensionSchema schema) { schema.uri(uri); });
    }

    void publishLiveData() {
        rapidjson::Document update;
        update.Parse(LIVE_DATA_MESSAGE);
        invokeLiveDataUpdate(URI, update);
    }

    bool invokeCommand(const std::string& uri, const rapidjson::Value& command) override {
        processedCommand = true;
        rapidjson::Document event;
        event.Parse(EVENT_MESSAGE);
        invokeExtensionEventHandler(uri, event);
        return true;
    }

    void onResourceReady(const std::string& uri, const ResourceHolderPtr& resourceHolder) override {
        resourceReady = true;
    }

    bool updateComponent(const std::string& uri, const rapidjson::Value& command) override {
        processedComponentUpdate = true;
        return true;
    }

    void onRegistered(const std::string& uri, const std::string& token) override {
        registered = true;
    }

    void onUnregistered(const std::string& uri, const std::string& token) override {
        registered = false;
    }

    bool registered = false;
    bool resourceReady = false;
    bool processedCommand = false;
    bool processedComponentUpdate = false;
};

class LifecycleExtension : public alexaext::ExtensionBase {
public:
    explicit LifecycleExtension() : ExtensionBase(URI),
          lastActivity(URI, nullptr)
    {}

    void publishLiveData() {
        publishLiveData(lastActivity);
    }

    void publishLiveData(const ActivityDescriptor& activity) {
        rapidjson::Document update;
        update.Parse(LIVE_DATA_MESSAGE);
        invokeLiveDataUpdate(activity, update);
    }

    void publishEvent(const ActivityDescriptor& activity) {
        rapidjson::Document event;
        event.Parse(EVENT_MESSAGE);
        invokeExtensionEventHandler(activity, event);
    }

    rapidjson::Document createRegistration(const ActivityDescriptor& activity,
                                const rapidjson::Value& registrationRequest) override {
        lastActivity = activity;
        const auto& uri = activity.getURI();

        return RegistrationSuccess("1.0")
            .uri(uri)
            .token("<AUTO_TOKEN>")
            .schema("1.0", [uri](ExtensionSchema schema) { schema.uri(uri); });
    }

    void onSessionStarted(const SessionDescriptor& session) override {
        sessionActive = true;
    }

    void onSessionEnded(const SessionDescriptor& session) override {
        sessionActive = false;
    }

    void onActivityRegistered(const ActivityDescriptor& activity) override {
        registered = true;
    }

    void onActivityUnregistered(const ActivityDescriptor& activity) override {
        registered = false;
    }

    void onForeground(const ActivityDescriptor& activity) override {
        displayState = "foreground";
    }

    void onBackground(const ActivityDescriptor& activity) override {
        displayState = "background";
    }

    void onHidden(const ActivityDescriptor& activity) override {
        displayState = "hidden";
    }

    bool invokeCommand(const ActivityDescriptor& activity, const rapidjson::Value& command) override {
        processedCommand = true;
        rapidjson::Document event;
        event.Parse(EVENT_MESSAGE);
        invokeExtensionEventHandler(activity, event);
        return true;
    }

    bool updateComponent(const ActivityDescriptor& activity, const rapidjson::Value& command) override {
        processedComponentUpdate = true;
        return true;
    }

    void onResourceReady(const ActivityDescriptor& activity,
                         const ResourceHolderPtr& resourceHolder) override {
        resourceReady = true;
    }

    bool registered = false;
    bool resourceReady = false;
    bool sessionActive = false;
    bool processedCommand = false;
    bool processedComponentUpdate = false;
    std::string displayState = "none";
    alexaext::ActivityDescriptor lastActivity;
};

class LegacyProxy : public alexaext::ExtensionProxy {
public:
    LegacyProxy(const ExtensionPtr& extension) : mExtension(extension) {}

    std::set<std::string> getURIs() const override { return mExtension ? mExtension->getURIs() : std::set<std::string>(); }

    bool initializeExtension(const std::string& uri) override { return isInitialized(uri); }
    bool isInitialized(const std::string& uri) const override { return mExtension && getURIs().count(uri) > 0; }

    bool getRegistration(const std::string& uri, const rapidjson::Value& registrationRequest,
                         RegistrationSuccessCallback success,
                         RegistrationFailureCallback error) override {
        if (!isInitialized(uri)) return false;

        auto response = mExtension->createRegistration(uri, registrationRequest);
        auto method = RegistrationSuccess::METHOD().Get(response);
        if (method &&  *method != "RegisterSuccess") {
            error(uri, response);
        } else {
            success(uri, response);
        }
        return true;
    }

    bool invokeCommand(const std::string& uri, const rapidjson::Value& command,
                       CommandSuccessCallback success, CommandFailureCallback error) override {
        if (!isInitialized(uri)) return false;

        auto commandId = (int) Command::ID().Get(command)->GetDouble();
        if (mExtension->invokeCommand(uri, command)) {
            auto response = CommandSuccess("1.0")
                                          .uri(uri)
                                          .id(commandId);
            success(uri, response);
        } else {
            auto response = CommandFailure("1.0")
                .uri(uri)
                .id(commandId)
                .errorCode(kErrorFailedCommand)
                .errorMessage("Extension failed");
            error(uri, response);
        }
        return true;
    }

    bool sendMessage(const std::string& uri, const rapidjson::Value& message) override {
        if (!isInitialized(uri)) return false;

        return mExtension->updateComponent(uri, message);
    }

    void onResourceReady(const std::string& uri, const ResourceHolderPtr& resourceHolder) override {
        mExtension->onResourceReady(uri, resourceHolder);
    }

    void registerEventCallback(Extension::EventCallback callback) override {
        mExtension->registerEventCallback(std::move(callback));
    }
    void registerLiveDataUpdateCallback(Extension::LiveDataUpdateCallback callback) override {
        mExtension->registerLiveDataUpdateCallback(std::move(callback));
    }

private:
    ExtensionPtr mExtension;
};

class ExtensionLifecycleTest : public ::testing::Test {
public:
    void SetUp() override {
        legacyExtension = std::make_shared<LegacyExtension>();
        legacyProxy = std::make_shared<LocalExtensionProxy>(legacyExtension);

        extension = std::make_shared<LifecycleExtension>();
        proxy = ThreadSafeExtensionProxy::create(extension);
    }

    std::shared_ptr<LegacyExtension> legacyExtension;
    ExtensionProxyPtr legacyProxy;

    std::shared_ptr<LifecycleExtension> extension;
    ExtensionProxyPtr proxy;
};

}

TEST_F(ExtensionLifecycleTest, LegacyExtension) {
    auto session = SessionDescriptor::create();
    auto activity = ActivityDescriptor::create(URI, session);
    bool receivedEvent = false;

    ASSERT_TRUE(legacyProxy->initializeExtension(URI));

    legacyProxy->registerEventCallback([&](const std::string& uri, const rapidjson::Value& event) {
        receivedEvent = true;
    });

    // No side effect expected for the legacy case
    legacyProxy->onSessionStarted(*session);

    auto req = RegistrationRequest("1.0")
        .uri(URI);

    bool successCallbackWasCalled = false;
    bool registered = legacyProxy->getRegistration(*activity, req,
                                             [&](const ActivityDescriptor& activity, const rapidjson::Value &response) {
                                               successCallbackWasCalled = true;
                                             },
                                             [](const ActivityDescriptor& activity, const rapidjson::Value &error) {
                                               FAIL();
                                             });
    legacyProxy->onRegistered(*activity);
    ASSERT_TRUE(registered);
    ASSERT_TRUE(successCallbackWasCalled);
    ASSERT_TRUE(legacyExtension->registered);

    // No side effect expected for the legacy case
    legacyProxy->onForeground(*activity);

    rapidjson::Document command;
    command.Parse(COMMAND_MESSAGE);

    ASSERT_FALSE(receivedEvent); // The extension will publish an event in response to the command
    bool commandSuccessCallbackWasCalled;
    bool commandAccepted = legacyProxy->invokeCommand(*activity, command,
            [&](const ActivityDescriptor& activity, const rapidjson::Value &response) {
                commandSuccessCallbackWasCalled = true;
            },
            [](const ActivityDescriptor& activity, const rapidjson::Value &error) {
                FAIL();
            });
    ASSERT_TRUE(commandAccepted);
    ASSERT_TRUE(commandSuccessCallbackWasCalled);
    ASSERT_TRUE(legacyExtension->processedCommand);
    ASSERT_TRUE(receivedEvent);

    bool liveDataUpdateReceived = false;
    legacyProxy->registerLiveDataUpdateCallback([&](const std::string& uri, const rapidjson::Value& liveDataUpdate) {
            liveDataUpdateReceived = true;
    });
    legacyExtension->publishLiveData();
    ASSERT_TRUE(liveDataUpdateReceived);

    // No side effect expected for the legacy case
    legacyProxy->onBackground(*activity);
    legacyProxy->onHidden(*activity);

    legacyProxy->onUnregistered(*activity);
    ASSERT_FALSE(legacyExtension->registered);

    // No side effect expected for the legacy case
    legacyProxy->onSessionEnded(*session);
}

TEST_F(ExtensionLifecycleTest, Lifecycle) {
    auto session = SessionDescriptor::create();
    auto activity = ActivityDescriptor::create(URI, session);
    bool receivedEvent = false;

    ASSERT_TRUE(proxy->initializeExtension(URI));

    proxy->registerEventCallback(*activity, [&](const ActivityDescriptor& activity, const rapidjson::Value& event) {
        receivedEvent = true;
    });

    ASSERT_FALSE(extension->sessionActive);

    proxy->onSessionStarted(*session);

    ASSERT_TRUE(extension->sessionActive);

    auto req = RegistrationRequest("1.0")
        .uri(URI);

    bool successCallbackWasCalled = false;
    bool registered = proxy->getRegistration(*activity, req,
            [&](const ActivityDescriptor& activity, const rapidjson::Value &response) {
                successCallbackWasCalled = true;
            },
            [](const ActivityDescriptor& activity, const rapidjson::Value &error) {
                FAIL();
            });
    proxy->onRegistered(*activity);
    ASSERT_TRUE(registered);
    ASSERT_TRUE(successCallbackWasCalled);
    ASSERT_TRUE(extension->registered);

    proxy->onForeground(*activity);
    ASSERT_EQ("foreground", extension->displayState);

    rapidjson::Document command;
    command.Parse(COMMAND_MESSAGE);

    ASSERT_FALSE(receivedEvent); // The extension will publish an event in response to the command
    bool commandSuccessCallbackWasCalled;
    bool commandAccepted = proxy->invokeCommand(*activity, command,
            [&](const ActivityDescriptor& activity, const rapidjson::Value &response) {
                commandSuccessCallbackWasCalled = true;
            },
            [](const ActivityDescriptor& activity, const rapidjson::Value &error) {
                FAIL();
            });
    ASSERT_TRUE(commandAccepted);
    ASSERT_TRUE(commandSuccessCallbackWasCalled);
    ASSERT_TRUE(extension->processedCommand);
    ASSERT_TRUE(receivedEvent);

    bool liveDataUpdateReceived = false;
    proxy->registerLiveDataUpdateCallback(*activity, [&](const ActivityDescriptor& callbackActivity, const rapidjson::Value& liveDataUpdate) {
        ASSERT_EQ(*activity, callbackActivity);
        liveDataUpdateReceived = true;
    });
    extension->publishLiveData();
    ASSERT_TRUE(liveDataUpdateReceived);

    proxy->onBackground(*activity);
    ASSERT_EQ("background", extension->displayState);

    proxy->onHidden(*activity);
    ASSERT_EQ("hidden", extension->displayState);

    proxy->onUnregistered(*activity);
    ASSERT_FALSE(extension->registered);

    proxy->onSessionEnded(*session);
    ASSERT_FALSE(extension->sessionActive);
}

TEST_F(ExtensionLifecycleTest, MultipleCallbacksForSameActivity) {
    auto session = SessionDescriptor::create();
    auto activity = ActivityDescriptor::create(URI, session);
    auto otherActivity = ActivityDescriptor::create(URI, session);

    ASSERT_TRUE(proxy->initializeExtension(URI));

    bool receivedFirstEvent = false;
    bool receivedSecondEvent = false;
    proxy->registerEventCallback(*activity, [&](const ActivityDescriptor& callbackActivity, const rapidjson::Value& event) {
        ASSERT_EQ(*activity, callbackActivity);
        receivedFirstEvent = true;
    });
    proxy->registerEventCallback(*activity, [&](const ActivityDescriptor& callbackActivity, const rapidjson::Value& event) {
        ASSERT_EQ(*activity, callbackActivity);
        receivedSecondEvent = true;
    });

    bool receivedFirstLiveDataUpdate = false;
    bool receivedSecondLiveDataUpdate = false;
    proxy->registerLiveDataUpdateCallback(*activity, [&](const ActivityDescriptor& callbackActivity, const rapidjson::Value& liveDataUpdate) {
        ASSERT_EQ(*activity, callbackActivity);
        receivedFirstLiveDataUpdate = true;
    });
    proxy->registerLiveDataUpdateCallback(*activity, [&](const ActivityDescriptor& callbackActivity, const rapidjson::Value& liveDataUpdate) {
        ASSERT_EQ(*activity, callbackActivity);
        receivedSecondLiveDataUpdate = true;
    });

    ASSERT_FALSE(extension->sessionActive);

    proxy->onSessionStarted(*session);

    ASSERT_TRUE(extension->sessionActive);

    auto req = RegistrationRequest("1.0")
                   .uri(URI);

    bool successCallbackWasCalled = false;
    bool registered = proxy->getRegistration(*activity, req,
        [&](const ActivityDescriptor& activity, const rapidjson::Value &response) {
            successCallbackWasCalled = true;
        },
        [](const ActivityDescriptor& activity, const rapidjson::Value &error) {
            FAIL();
        });
    proxy->onRegistered(*activity);
    ASSERT_TRUE(registered);
    ASSERT_TRUE(successCallbackWasCalled);
    ASSERT_TRUE(extension->registered);

    extension->publishEvent(*activity);
    ASSERT_TRUE(receivedFirstEvent);
    ASSERT_TRUE(receivedSecondEvent);

    extension->publishLiveData(*activity);
    ASSERT_TRUE(receivedFirstLiveDataUpdate);
    ASSERT_TRUE(receivedSecondLiveDataUpdate);

    proxy->onSessionEnded(*session);
    ASSERT_FALSE(extension->sessionActive);
}

TEST_F(ExtensionLifecycleTest, UnregisterCleansUpCallbacks) {
    auto session = SessionDescriptor::create();
    auto activity = ActivityDescriptor::create(URI, session);
    auto otherActivity = ActivityDescriptor::create(URI, session);

    ASSERT_TRUE(proxy->initializeExtension(URI));

    bool receivedEvent = false;
    proxy->registerEventCallback(*activity, [&](const ActivityDescriptor& callbackActivity, const rapidjson::Value& event) {
        ASSERT_EQ(*activity, callbackActivity);
        receivedEvent = true;
    });

    bool liveDataUpdateReceived = false;
    proxy->registerLiveDataUpdateCallback(*activity, [&](const ActivityDescriptor& callbackActivity, const rapidjson::Value& liveDataUpdate) {
        ASSERT_EQ(*activity, callbackActivity);
        liveDataUpdateReceived = true;
    });

    ASSERT_FALSE(extension->sessionActive);

    proxy->onSessionStarted(*session);

    ASSERT_TRUE(extension->sessionActive);

    auto req = RegistrationRequest("1.0")
                   .uri(URI);

    bool successCallbackWasCalled = false;
    bool registered = proxy->getRegistration(*activity, req,
        [&](const ActivityDescriptor& activity, const rapidjson::Value &response) {
            successCallbackWasCalled = true;
        },
        [](const ActivityDescriptor& activity, const rapidjson::Value &error) {
            FAIL();
        });
    proxy->onRegistered(*activity);
    ASSERT_TRUE(registered);
    ASSERT_TRUE(successCallbackWasCalled);
    ASSERT_TRUE(extension->registered);


    extension->publishEvent(*activity);
    ASSERT_TRUE(receivedEvent);

    extension->publishLiveData(*activity);
    ASSERT_TRUE(liveDataUpdateReceived);

    // Reset the state
    receivedEvent = false;
    liveDataUpdateReceived = false;

    extension->publishEvent(*otherActivity);
    ASSERT_FALSE(receivedEvent);

    extension->publishLiveData(*otherActivity);
    ASSERT_FALSE(liveDataUpdateReceived);

    // Unregister activity, this will clear event and live data callbacks
    proxy->onUnregistered(*activity);

    extension->publishEvent(*activity);
    ASSERT_FALSE(receivedEvent);
    extension->publishLiveData(*activity);
    ASSERT_FALSE(liveDataUpdateReceived);

    proxy->onSessionEnded(*session);
    ASSERT_FALSE(extension->sessionActive);
}

TEST_F(ExtensionLifecycleTest, BaseProxyEnsuresBackwardsCompatibility) {
    auto session = SessionDescriptor::create();
    auto activity = ActivityDescriptor::create(URI, session);
    bool receivedEvent = false;

    legacyProxy = std::make_shared<LegacyProxy>(legacyExtension);

    ASSERT_TRUE(legacyProxy->initializeExtension(URI));

    legacyProxy->registerEventCallback([&](const std::string& uri, const rapidjson::Value& event) {
        receivedEvent = true;
    });

    // No side effect expected for the legacy case
    legacyProxy->onSessionStarted(*session);

    auto req = RegistrationRequest("1.0")
                   .uri(URI);

    bool successCallbackWasCalled = false;
    bool registered = legacyProxy->getRegistration(*activity, req,
        [&](const ActivityDescriptor& activity, const rapidjson::Value &response) {
            successCallbackWasCalled = true;
        },
        [](const ActivityDescriptor& activity, const rapidjson::Value &error) {
            FAIL();
        });
    ASSERT_TRUE(registered);
    ASSERT_TRUE(successCallbackWasCalled);

    // No side effect expected for the legacy case
    legacyProxy->onForeground(*activity);

    rapidjson::Document command;
    command.Parse(COMMAND_MESSAGE);

    ASSERT_FALSE(receivedEvent); // The extension will publish an event in response to the command
    bool commandSuccessCallbackWasCalled;
    bool commandAccepted = legacyProxy->invokeCommand(*activity, command,
        [&](const ActivityDescriptor& activity, const rapidjson::Value &response) {
            commandSuccessCallbackWasCalled = true;
        },
        [](const ActivityDescriptor& activity, const rapidjson::Value &error) {
            FAIL();
        });
    ASSERT_TRUE(commandAccepted);
    ASSERT_TRUE(commandSuccessCallbackWasCalled);
    ASSERT_TRUE(legacyExtension->processedCommand);
    ASSERT_TRUE(receivedEvent);

    bool liveDataUpdateReceived = false;
    legacyProxy->registerLiveDataUpdateCallback([&](const std::string& uri, const rapidjson::Value& liveDataUpdate) {
        liveDataUpdateReceived = true;
    });
    legacyExtension->publishLiveData();
    ASSERT_TRUE(liveDataUpdateReceived);

    // No side effect expected for the legacy case
    legacyProxy->onBackground(*activity);
    legacyProxy->onHidden(*activity);

    legacyProxy->onUnregistered(*activity);
    ASSERT_FALSE(legacyExtension->registered);

    // No side effect expected for the legacy case
    legacyProxy->onSessionEnded(*session);
}

TEST_F(ExtensionLifecycleTest, ResourceReady) {
    auto session = SessionDescriptor::create();
    auto activity = ActivityDescriptor::create(URI, session);

    ASSERT_TRUE(proxy->initializeExtension(URI));

    auto resource = std::make_shared<ResourceHolder>("SURFACE42");
    proxy->onResourceReady(*activity, resource);

    ASSERT_TRUE(extension->resourceReady);
}

TEST_F(ExtensionLifecycleTest, ResourceReadyLegacy) {
    auto session = SessionDescriptor::create();
    auto activity = ActivityDescriptor::create(URI, session);

    ASSERT_TRUE(legacyProxy->initializeExtension(URI));

    auto resource = std::make_shared<ResourceHolder>("SURFACE42");
    legacyProxy->onResourceReady(*activity, resource);

    ASSERT_TRUE(legacyExtension->resourceReady);
}

TEST_F(ExtensionLifecycleTest, UpdateComponent) {
    auto session = SessionDescriptor::create();
    auto activity = ActivityDescriptor::create(URI, session);

    ASSERT_TRUE(proxy->initializeExtension(URI));

    rapidjson::Document message;
    message.Parse(UPDATE_COMPONENT_MESSAGE);
    proxy->sendComponentMessage(*activity, message);

    ASSERT_TRUE(extension->processedComponentUpdate);
}

TEST_F(ExtensionLifecycleTest, UpdateComponentLegacy) {
    auto session = SessionDescriptor::create();
    auto activity = ActivityDescriptor::create(URI, session);

    ASSERT_TRUE(legacyProxy->initializeExtension(URI));

    rapidjson::Document message;
    message.Parse(UPDATE_COMPONENT_MESSAGE);
    legacyProxy->sendComponentMessage(*activity, message);

    ASSERT_TRUE(legacyExtension->processedComponentUpdate);
}