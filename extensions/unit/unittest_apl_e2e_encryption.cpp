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

#include "gtest/gtest.h"

#include "alexaext/APLE2EEncryptionExtension/AplE2eEncryptionExtension.h"
#include "alexaext/extensionmessage.h"

using namespace alexaext;
using namespace E2EEncryption;
using namespace rapidjson;

class TestE2EEncryptionObserver : public AplE2eEncryptionExtensionObserverInterface {
public:
    void onBase64EncryptValue(const std::string& token,
                              const std::string& key,
                              const std::string& algorithm,
                              const std::string& aad,
                              const std::string& value,
                              bool base64Encoded,
                              EncryptionCallbackSuccess successCallback,
                              EncryptionCallbackError errorCallback) override {
        mCommand = "Base64EncryptValue";
        if (value == "forcesuccess") {
            mEvent = "OnEncryptSuccess";
            successCallback(token, "onEncryptSuccessData", "onEncryptSuccessIVData", "onEncryptSuccessKey");
        } else {
            mEvent = "OnEncryptFailure";
            errorCallback(token, "error");
        }
    }

    void onBase64EncodeValue(const std::string& token, const std::string& value,
                             EncodeCallbackSuccess successCallback) override {
        mCommand = "Base64EncodeValue";
        if (value == "forcesuccess") {
            mEvent = "OnBase64EncodeSuccess";
            successCallback(token, "XXXYY");
        }
    }

    std::string mCommand;
    std::string mEvent;
};

// Inject the UUID generator so we can reproduce tests
static int uuidValue = 0;
std::string testUuid() {
    return "AplE2EEncryptionUuid-" + std::to_string(uuidValue);
}

class AplE2EEncryptionExtensionTest : public ::testing::Test {
public:
    void SetUp() override {
        mObserver = std::make_shared<TestE2EEncryptionObserver>();
        mExtension = std::make_shared<AplE2eEncryptionExtension>(mObserver, Executor::getSynchronousExecutor(), testUuid);
        // Register the event handler
        mExtension->registerEventCallback([this](const std::string& uri, const rapidjson::Value& event){
            if (uri != "aplext:e2eencryption:10")
                return;
            std::string eventName = GetWithDefault("name", event, "");
            if (eventName == "OnEncryptSuccess") {
                GetWithDefault("payload/base64EncryptedData", event, "");
                mEncryptedData = GetWithDefault("payload/base64EncryptedData", event, "");
                mEncodedIVData = GetWithDefault("payload/base64EncodedIV", event, "");
                mEncodedKey = GetWithDefault("payload/base64EncodedKey", event, "");
            }
            if (eventName == "OnEncryptFailure") {
                mErrorReason = GetWithDefault("payload/errorReason", event, "");
            }
            if (eventName == "OnBase64EncodeSuccess") {
                mEncodedData = GetWithDefault("payload/base64EncodedData", event, "");
            }
        });
    }

    /**
     * Simple registration for testing event/command/data.
     */
    ::testing::AssertionResult registerExtension()
    {
        Document settings(kObjectType);
        Document regReq = RegistrationRequest("1.0").uri("aplext:e2eencryption:10")
            .settings(settings);
        auto registration = mExtension->createRegistration("aplext:e2eencryption:10", regReq);
        auto method = GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, "Fail");
        if (std::strcmp("RegisterSuccess", method) != 0)
            return testing::AssertionFailure() << "Failed Registration:" << method;
        mClientToken = GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, "");
        if (mClientToken.length() == 0)
            return testing::AssertionFailure() << "Failed Token:" << mClientToken;

        return ::testing::AssertionSuccess();
    }

    std::shared_ptr<TestE2EEncryptionObserver> mObserver;
    std::shared_ptr<AplE2eEncryptionExtension> mExtension;
    std::string mClientToken;
    std::string mEncodedData;
    std::string mEncryptedData;
    std::string mEncodedIVData;
    std::string mEncodedKey;
    std::string mErrorReason;
};

/**
 * Simple create test for sanity.
 */
TEST_F(AplE2EEncryptionExtensionTest, CreateExtension)
{
    ASSERT_TRUE(mObserver);
    ASSERT_TRUE(mExtension);
    auto supported = mExtension->getURIs();
    ASSERT_EQ(1, supported.size());
    ASSERT_NE(supported.end(), supported.find("aplext:e2eencryption:10"));
}

/**
 * Registration request with bad URI.
 */
TEST_F(AplE2EEncryptionExtensionTest, RegistrationURIBad)
{
    Document regReq = RegistrationRequest("aplext:e2eencryption:BAD");
    auto registration = mExtension->createRegistration("aplext:e2eencryption:BAD", regReq);
    ASSERT_FALSE(registration.HasParseError());
    ASSERT_FALSE(registration.IsNull());
    ASSERT_STREQ("RegisterFailure",
                 GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, ""));
}

/**
 * Registration Success has required fields
 */
TEST_F(AplE2EEncryptionExtensionTest, RegistrationSuccess)
{
    uuidValue = 1;
    Document regReq = RegistrationRequest("1.0").uri("aplext:e2eencryption:10");
    auto registration = mExtension->createRegistration("aplext:e2eencryption:10", regReq);
    ASSERT_STREQ("RegisterSuccess",
                 GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, ""));
    ASSERT_STREQ("aplext:e2eencryption:10",
                 GetWithDefault<const char *>(RegistrationSuccess::URI(), registration, ""));
    auto schema = RegistrationSuccess::SCHEMA().Get(registration);
    ASSERT_TRUE(schema);
    ASSERT_STREQ("aplext:e2eencryption:10", GetWithDefault<const char *>("uri", *schema, ""));
    std::string token = GetWithDefault<const char *>(RegistrationSuccess::TOKEN(), registration, "");
    ASSERT_EQ(token, "AplE2EEncryptionUuid-1");
}

/**
 * Commands are defined at registration.
 */
TEST_F(AplE2EEncryptionExtensionTest, RegistrationCommands)
{
    Document regReq = RegistrationRequest("1.0").uri("aplext:e2eencryption:10");

    auto registration = mExtension->createRegistration("aplext:e2eencryption:10", regReq);
    ASSERT_STREQ("RegisterSuccess",
                 GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, ""));
    Value *schema = RegistrationSuccess::SCHEMA().Get(registration);
    ASSERT_TRUE(schema);

    Value *commands = ExtensionSchema::COMMANDS().Get(*schema);
    ASSERT_TRUE(commands);

    auto expectedCommandSet = std::set<std::string>();
    expectedCommandSet.insert("Base64EncryptValue");
    expectedCommandSet.insert("Base64EncodeValue");
    ASSERT_TRUE(commands->IsArray() && commands->Size() == expectedCommandSet.size());

    for (const Value &com : commands->GetArray()) {
        ASSERT_TRUE(com.IsObject());
        auto name = GetWithDefault<const char *>(Command::NAME(), com, "MissingName");
        ASSERT_TRUE(expectedCommandSet.count(name) == 1) << "Unknown Command:" << name;
        expectedCommandSet.erase(name);
    }
    ASSERT_TRUE(expectedCommandSet.empty());
}

/**
 * Events are defined
 */
TEST_F(AplE2EEncryptionExtensionTest, RegistrationEvents)
{
    Document regReq = RegistrationRequest("1.0").uri("aplext:e2eencryption:10");
    auto registration = mExtension->createRegistration("aplext:e2eencryption:10", regReq);
    ASSERT_STREQ("RegisterSuccess",
                 GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, ""));
    Value *schema = RegistrationSuccess::SCHEMA().Get(registration);
    ASSERT_TRUE(schema);

    Value *events = ExtensionSchema::EVENTS().Get(*schema);
    ASSERT_TRUE(events);

    // FullSet event handler for audio player
    auto expectedHandlerSet = std::set<std::string>();
    expectedHandlerSet.insert("OnEncryptSuccess");
    expectedHandlerSet.insert("OnEncryptFailure");
    expectedHandlerSet.insert("OnBase64EncodeSuccess");
    ASSERT_TRUE(events->IsArray() && events->Size() == expectedHandlerSet.size());

    // should have all event handlers defined
    for (const Value &evt : events->GetArray()) {
        ASSERT_TRUE(evt.IsObject());
        auto name = GetWithDefault<const char *>(Event::NAME(), evt, "MissingName");
        ASSERT_TRUE(expectedHandlerSet.count(name) == 1);
        expectedHandlerSet.erase(name);
    }
    ASSERT_TRUE(expectedHandlerSet.empty());
}


/**
 * Command Base64EncodeValue calls observer.
 */
TEST_F(AplE2EEncryptionExtensionTest, InvokeBase64EncodeValue)
{
    auto testText = "forcesuccess";
    ASSERT_TRUE(registerExtension());

    auto command = Command("1.0").target(mClientToken)
        .uri("aplext:e2eencryption:10")
        .name("Base64EncodeValue")
        .property("token", mClientToken)
        .property("value", testText);
    auto invoke = mExtension->invokeCommand("aplext:e2eencryption:10", command);
    ASSERT_TRUE(invoke);

    ASSERT_EQ("Base64EncodeValue", mObserver->mCommand);
    ASSERT_EQ("OnBase64EncodeSuccess", mObserver->mEvent);
    ASSERT_EQ("XXXYY", mEncodedData);
}

/**
 * Command Base64EncodeValue calls observer.
 */
TEST_F(AplE2EEncryptionExtensionTest, InvokeEncrypSuccess)
{
    auto testText = "forcesuccess";
    auto testKey = "key";
    auto testAlgorithm = "";
    auto testAad = "testAad";
    auto testBase64Encoded = true;
    ASSERT_TRUE(registerExtension());

    auto command = Command("1.0").target(mClientToken)
        .uri("aplext:e2eencryption:10")
        .name("Base64EncryptValue")
        .property("token", mClientToken)
        .property("value", testText)
        .property("key", testKey)
        .property("algorithm", testAlgorithm)
        .property("aad", testAad)
        .property("base64Encoded", testBase64Encoded);
    auto invoke = mExtension->invokeCommand("aplext:e2eencryption:10", command);
    ASSERT_TRUE(invoke);

    ASSERT_EQ("Base64EncryptValue", mObserver->mCommand);
    ASSERT_EQ("OnEncryptSuccess", mObserver->mEvent);
    ASSERT_EQ("onEncryptSuccessData", mEncryptedData);
    ASSERT_EQ("onEncryptSuccessIVData", mEncodedIVData);
    ASSERT_EQ("onEncryptSuccessKey", mEncodedKey);
}

/**
 * Command Base64EncodeValue calls observer.
 */
TEST_F(AplE2EEncryptionExtensionTest, InvokeEncrypError)
{
    auto testText = "forceerror";
    auto testKey = "key";
    auto testAlgorithm = "";
    auto testAad = "testAad";
    auto testBase64Encoded = true;
    ASSERT_TRUE(registerExtension());

    auto command = Command("1.0").target(mClientToken)
        .uri("aplext:e2eencryption:10")
        .name("Base64EncryptValue")
        .property("token", mClientToken)
        .property("value", testText)
        .property("key", testKey)
        .property("algorithm", testAlgorithm)
        .property("aad", testAad)
        .property("base64Encoded", testBase64Encoded);
    auto invoke = mExtension->invokeCommand("aplext:e2eencryption:10", command);
    ASSERT_TRUE(invoke);

    ASSERT_EQ("Base64EncryptValue", mObserver->mCommand);
    ASSERT_EQ("OnEncryptFailure", mObserver->mEvent);
    ASSERT_EQ("error", mErrorReason);
}