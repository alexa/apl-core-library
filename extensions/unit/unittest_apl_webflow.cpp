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
 */

#include <chrono>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <utility>

#include "gtest/gtest.h"

#include "alexaext/APLWebflowExtension/AplWebflowExtension.h"
#include "alexaext/extensionmessage.h"

using namespace alexaext;
using namespace alexaext::webflow;
using namespace rapidjson;

static int uuidValue = 1;

std::string
testGenUuid() {
    return std::string("TestWebflowUUID-") + std::to_string(uuidValue);
}

class SimpleTestWebflowObserver : public AplWebflowExtensionObserverInterface {
public:
    SimpleTestWebflowObserver() : AplWebflowExtensionObserverInterface() {}

    ~SimpleTestWebflowObserver() override = default;

    void onStartFlow(const ActivityDescriptor &activity, const std::string& token, const std::string& url, const std::string& flowId,
                     std::function<void(const std::string&, const std::string&)> onFlowEndEvent) override {
        mCommand = "START_FLOW";
        mUrl = url;
        mFlowId = flowId;
        mToken = testGenUuid();
        onFlowEndEvent(mToken, flowId);
    }

    void resetTestData() {
        mCommand.clear();
        mUrl.clear();
        mFlowId.clear();
        mToken.clear();
    }

    std::string mCommand;
    std::string mUrl;
    std::string mFlowId;
    std::string mToken;
};

class SimpleLifecycleTestWebflowObserver : public AplWebflowExtensionObserverInterface {
public:
    SimpleLifecycleTestWebflowObserver() : AplWebflowExtensionObserverInterface() {}

    ~SimpleLifecycleTestWebflowObserver() override = default;

    // no-op
    void onStartFlow(const ActivityDescriptor &activity, const std::string& token, const std::string& url, const std::string& flowId,
                     std::function<void(const std::string&, const std::string&)> onFlowEndEvent) override {}

    void onForeground(const ActivityDescriptor &activity) override {
        mLifecycleState = "FOREGROUND";
    }

    void onBackground(const ActivityDescriptor &activity) override {
        mLifecycleState = "BACKGROUND";
    }

    void onHidden(const ActivityDescriptor &activity) override {
        mLifecycleState = "HIDDEN";
    }

    std::string mLifecycleState = "CREATED";
};

class SimpleTestWebflowExtension : public AplWebflowExtension {
public:
    explicit SimpleTestWebflowExtension(
        std::function<std::string(void)> uuidGenerator,
        std::shared_ptr<AplWebflowExtensionObserverInterface> observer)
        : AplWebflowExtension(
              std::move(uuidGenerator),
              std::move(observer),
              alexaext::Executor::getSynchronousExecutor()) {}
};

class SimpleAplWebflowExtensionTest : public ::testing::Test {
public:
    void SetUp() override {
        mObserver = std::make_shared<SimpleTestWebflowObserver>();
        mExtension = std::make_shared<SimpleTestWebflowExtension>(testGenUuid, mObserver);
        mExtension->registerEventCallback(
            [&](const std::string& uri, const rapidjson::Value& event) {
                if (uri == "aplext:webflow:10") {
                    auto eventFlow = GetWithDefault<std::string>("payload/flowId", event, "");
                    if (!eventFlow.empty()) {
                        mEventFlow = eventFlow;
                    }
                    auto token = GetWithDefault<std::string>("payload/token", event, "");
                    if (!token.empty()) {
                        mToken = token;
                    }
                }
            });
    }

    /**
     * Simple registration for testing event/command/data.
     */
    ::testing::AssertionResult registerExtension() {
        Document regReq = RegistrationRequest("1.0").uri("aplext:webflow:10");
        auto registration = mExtension->createRegistration(createActivityDescriptor(), regReq);
        auto method =
            GetWithDefault<const char*>(RegistrationSuccess::METHOD(), registration, "Fail");
        if (std::strcmp("RegisterSuccess", method) != 0)
            return testing::AssertionFailure() << "Failed Registration:" << method;
        mClientToken = GetWithDefault<const char*>(RegistrationSuccess::METHOD(), registration, "");
        if (mClientToken.length() == 0)
            return testing::AssertionFailure() << "Failed Token:" << mClientToken;

        return ::testing::AssertionSuccess();
    }

    /**
     * Simple utility to create activity descriptors accross the tests
     */
    ActivityDescriptor createActivityDescriptor(std::string uri = URI) {
        // Create Activity
        SessionDescriptorPtr sessionPtr = SessionDescriptor::create("TestSessionId");
        ActivityDescriptor activityDescriptor(
            uri,
            sessionPtr);
        return activityDescriptor;
    }

    void resetTestData() {
        mClientToken.clear();
        mEventFlow.clear();
        mToken.clear();
    }

    std::shared_ptr<SimpleTestWebflowObserver> mObserver;
    AplWebflowExtensionPtr mExtension;
    std::string mClientToken;
    std::string mEventFlow;
    std::string mToken;
};

/**
 * Simple create test for sanity.
 */
TEST_F(SimpleAplWebflowExtensionTest, CreateExtension) {
    ASSERT_TRUE(mObserver);
    ASSERT_TRUE(mExtension);
    auto supported = mExtension->getURIs();
    ASSERT_EQ(1, supported.size());
    ASSERT_NE(supported.end(), supported.find("aplext:webflow:10"));
}

/**
 * Registration request with bad URI.
 */
TEST_F(SimpleAplWebflowExtensionTest, RegistrationURIBad) {
    Document regReq = RegistrationRequest("aplext:webflow:BAD");
    auto registration = mExtension->createRegistration(createActivityDescriptor("aplext:webflow:BAD"), regReq);
    ASSERT_FALSE(registration.HasParseError());
    ASSERT_FALSE(registration.IsNull());
    ASSERT_STREQ("RegisterFailure",
                 GetWithDefault<const char*>(RegistrationSuccess::METHOD(), registration, ""));
}

/**
 * Registration Success has required fields
 */
TEST_F(SimpleAplWebflowExtensionTest, RegistrationSuccess) {
    Document regReq = RegistrationRequest("1.0").uri("aplext:webflow:10");
    auto registration = mExtension->createRegistration(createActivityDescriptor(), regReq);
    ASSERT_STREQ("RegisterSuccess",
                 GetWithDefault<const char*>(RegistrationSuccess::METHOD(), registration, ""));
    ASSERT_STREQ("aplext:webflow:10",
                 GetWithDefault<const char*>(RegistrationSuccess::URI(), registration, ""));
    auto schema = RegistrationSuccess::SCHEMA().Get(registration);
    ASSERT_TRUE(schema);
    ASSERT_STREQ("aplext:webflow:10", GetWithDefault<const char*>("uri", *schema, ""));
    std::string token = GetWithDefault<const char*>(RegistrationSuccess::TOKEN(), registration, "");
    ASSERT_TRUE(token.rfind("TestWebflowUUID") == 0);
}

/**
 * Commands are defined at registration.
 */
TEST_F(SimpleAplWebflowExtensionTest, RegistrationCommands) {
    Document regReq = RegistrationRequest("1.0").uri("aplext:webflow:10");

    auto registration = mExtension->createRegistration(createActivityDescriptor(), regReq);
    ASSERT_STREQ("RegisterSuccess",
                 GetWithDefault<const char*>(RegistrationSuccess::METHOD(), registration, ""));
    Value* schema = RegistrationSuccess::SCHEMA().Get(registration);
    ASSERT_TRUE(schema);

    Value* commands = ExtensionSchema::COMMANDS().Get(*schema);
    ASSERT_TRUE(commands);

    auto expectedCommandSet = std::set<std::string>();
    expectedCommandSet.insert("StartFlow");
    ASSERT_TRUE(commands->IsArray() && commands->Size() == expectedCommandSet.size());

    for (const Value& com : commands->GetArray()) {
        ASSERT_TRUE(com.IsObject());
        auto name = GetWithDefault<const char*>(Command::NAME(), com, "MissingName");
        ASSERT_TRUE(expectedCommandSet.count(name) == 1) << "Unknown Command:" << name;
        expectedCommandSet.erase(name);
    }
    ASSERT_TRUE(expectedCommandSet.empty());
}

/**
 * Events are defined
 */
TEST_F(SimpleAplWebflowExtensionTest, RegistrationEvents) {
    Document regReq = RegistrationRequest("1.0").uri("aplext:webflow:10");
    auto registration = mExtension->createRegistration(createActivityDescriptor(), regReq);
    ASSERT_STREQ("RegisterSuccess",
                 GetWithDefault<const char*>(RegistrationSuccess::METHOD(), registration, ""));
    Value* schema = RegistrationSuccess::SCHEMA().Get(registration);
    ASSERT_TRUE(schema);

    Value* events = ExtensionSchema::EVENTS().Get(*schema);
    ASSERT_TRUE(events);

    // FullSet event handler for audio player
    auto expectedHandlerSet = std::set<std::string>();
    expectedHandlerSet.insert("OnFlowEnd");
    ASSERT_TRUE(events->IsArray() && events->Size() == expectedHandlerSet.size());

    // should have all event handlers defined
    for (const Value& evt : events->GetArray()) {
        ASSERT_TRUE(evt.IsObject());
        auto name = GetWithDefault<const char*>(Event::NAME(), evt, "MissingName");
        ASSERT_TRUE(expectedHandlerSet.count(name) == 1);
        expectedHandlerSet.erase(name);
    }
    ASSERT_TRUE(expectedHandlerSet.empty());
}

/**
 * Command StartFlow calls observer.
 */
TEST_F(SimpleAplWebflowExtensionTest, InvokeCommandStartFlowSuccess) {
    ASSERT_TRUE(registerExtension());

    auto command = Command("1.0")
                       .target(mClientToken)
                       .uri("aplext:webflow:10")
                       .name("StartFlow")
                       .property("url", "test_url");
    auto invoke = mExtension->invokeCommand(createActivityDescriptor(), command);
    ASSERT_TRUE(invoke);
    ASSERT_EQ("START_FLOW", mObserver->mCommand);
    ASSERT_EQ("test_url", mObserver->mUrl);
    ASSERT_TRUE(mObserver->mFlowId.empty());
}

/**
 * Command StartFlow forward ClientId observer.
 */
TEST_F(SimpleAplWebflowExtensionTest, InvokeCommandStartFlowWithFlowIdSuccess) {
    ASSERT_TRUE(registerExtension());

    auto command = Command("1.0")
                       .target(mClientToken)
                       .uri("aplext:webflow:10")
                       .name("StartFlow")
                       .property("url", "test_url")
                       .property("flowId", "test_flow");
    auto invoke = mExtension->invokeCommand(createActivityDescriptor(), command);
    ASSERT_TRUE(invoke);
    ASSERT_EQ("START_FLOW", mObserver->mCommand);
    ASSERT_EQ("test_url", mObserver->mUrl);
    ASSERT_EQ("test_flow", mObserver->mFlowId);
    ASSERT_EQ("test_flow", mEventFlow);
}

/**
 * Ensure base implementation of lifecycle callbacks in the observer run with no effect
 */
 TEST_F(SimpleAplWebflowExtensionTest, VerifyLifecycleCallbacksRun) {
     auto activity = createActivityDescriptor();

     ASSERT_TRUE(registerExtension());

     mExtension->onForeground(activity);
     mExtension->onBackground(activity);
     mExtension->onHidden(activity);

     ASSERT_TRUE(mObserver->mCommand.empty());
     ASSERT_TRUE(mObserver->mUrl.empty());
     ASSERT_TRUE(mObserver->mFlowId.empty());
 }

/**
 * Lifecycle callbacks are forwarded to observer.
 */
TEST_F(SimpleAplWebflowExtensionTest, LifecycleCallbacksForwardToObserver) {
    auto activity = createActivityDescriptor();

    auto lifecycleObserver = std::make_shared<SimpleLifecycleTestWebflowObserver>();
    auto extension = std::make_shared<SimpleTestWebflowExtension>(testGenUuid, lifecycleObserver);

    ASSERT_EQ("CREATED", lifecycleObserver->mLifecycleState);

    extension->onForeground(activity);
    ASSERT_EQ("FOREGROUND", lifecycleObserver->mLifecycleState);

    extension->onBackground(activity);
    ASSERT_EQ("BACKGROUND", lifecycleObserver->mLifecycleState);

    extension->onHidden(activity);
    ASSERT_EQ("HIDDEN", lifecycleObserver->mLifecycleState);
}
