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

#include "alexaext/APLMusicAlarmExtension/AplMusicAlarmExtension.h"
#include "alexaext/extensionmessage.h"

using namespace alexaext;
using namespace MusicAlarm;
using namespace rapidjson;


static const std::string DISMISSCOMMAND = "DISMISS";
static const std::string SNOOZECOMMAND = "SNOOZE";


class TestMusicAlarmObserver : public AplMusicAlarmExtensionObserverInterface {
public:

    TestMusicAlarmObserver() = default;

    /**
     * The DismissAlarm command is used to dismiss the current ringing alarm.
     */
    void dismissAlarm() {
        mCommand = DISMISSCOMMAND;
    }

    /**
     * The SnoozeAlarm command is used to snooze the current ringing alarm.
     */
    void snoozeAlarm() {
        mCommand = SNOOZECOMMAND;
    }
    std::string mCommand;
};

// Inject the UUID generator so we can reproduce tests
static int uuidValue = 0;
std::string testMusicUuid() {
    return "AplMusicAlarmUuid-" + std::to_string(uuidValue);
}


class AplMusicAlarmExtensionTest : public ::testing::Test {
public:
    void SetUp() override {
        mObserver = std::make_shared<TestMusicAlarmObserver>();
        mExtension = std::make_shared<AplMusicAlarmExtension>(
            mObserver, Executor::getSynchronousExecutor(), testMusicUuid);
    }

    /**
     * Simple registration for testing event/command/data.
     */
    ::testing::AssertionResult registerExtension()
    {
        Document settings(kObjectType);
        Document regReq = RegistrationRequest("1.0").uri(MusicAlarm::URI)
            .settings(settings);
        auto registration = mExtension->createRegistration(MusicAlarm::URI, regReq);
        auto method = GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, "Fail");
        if (std::strcmp("RegisterSuccess", method) != 0)
            return testing::AssertionFailure() << "Failed Registration:" << method;
        mClientToken = GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, "");
        if (mClientToken.length() == 0)
            return testing::AssertionFailure() << "Failed Token:" << mClientToken;

        return ::testing::AssertionSuccess();
    }

    std::shared_ptr<TestMusicAlarmObserver> mObserver;
    std::shared_ptr<AplMusicAlarmExtension> mExtension;
    std::string mClientToken;
};

/**
 * Simple create test for sanity.
 */
TEST_F(AplMusicAlarmExtensionTest, CreateExtension)
{
    ASSERT_TRUE(mObserver);
    ASSERT_TRUE(mExtension);
    auto supported = mExtension->getURIs();
    ASSERT_EQ(1, supported.size());
    ASSERT_NE(supported.end(), supported.find(MusicAlarm::URI));
}

/**
 * Registration request with bad URI.
 */
TEST_F(AplMusicAlarmExtensionTest, RegistrationURIBad)
{
    Document regReq = RegistrationRequest("aplext:music:BAD");
    auto registration = mExtension->createRegistration("aplext:music:BAD", regReq);
    ASSERT_FALSE(registration.HasParseError());
    ASSERT_FALSE(registration.IsNull());
    ASSERT_STREQ("RegisterFailure",
                 GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, ""));
}

/**
 * Registration Success has required fields
 */
TEST_F(AplMusicAlarmExtensionTest, RegistrationSuccess)
{
    uuidValue = 1;
    Document regReq = RegistrationRequest("1.0").uri(MusicAlarm::URI);
    auto registration = mExtension->createRegistration(MusicAlarm::URI, regReq);
    ASSERT_STREQ("RegisterSuccess",
                 GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, ""));
    ASSERT_STREQ(MusicAlarm::URI.c_str(),
                 GetWithDefault<const char *>(RegistrationSuccess::URI(), registration, ""));
    auto schema = RegistrationSuccess::SCHEMA().Get(registration);
    ASSERT_TRUE(schema);
    ASSERT_STREQ(MusicAlarm::URI.c_str(), GetWithDefault<const char *>("uri", *schema, ""));
    std::string token = GetWithDefault<const char *>(RegistrationSuccess::TOKEN(), registration, "");
    ASSERT_EQ(token, "AplMusicAlarmUuid-1");
}

/**
 * Commands are defined at registration.
 */
TEST_F(AplMusicAlarmExtensionTest, RegistrationCommands)
{
    Document regReq = RegistrationRequest("1.0").uri(MusicAlarm::URI);

    auto registration = mExtension->createRegistration(MusicAlarm::URI, regReq);
    ASSERT_STREQ("RegisterSuccess",
                 GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, ""));
    Value *schema = RegistrationSuccess::SCHEMA().Get(registration);
    ASSERT_TRUE(schema);

    Value *commands = ExtensionSchema::COMMANDS().Get(*schema);
    ASSERT_TRUE(commands);

    auto expectedCommandSet = std::set<std::string>();
    expectedCommandSet.insert("DismissAlarm");
    expectedCommandSet.insert("SnoozeAlarm");
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
 * Command Base64EncodeValue calls observer.
 */
TEST_F(AplMusicAlarmExtensionTest, InvokeDismiss)
{
    ASSERT_TRUE(registerExtension());

    auto command = Command("1.0").target(mClientToken)
        .uri(MusicAlarm::URI)
        .name("DismissAlarm");
    auto invoke = mExtension->invokeCommand(MusicAlarm::URI, command);
    ASSERT_TRUE(invoke);

    ASSERT_EQ(DISMISSCOMMAND, mObserver->mCommand);
}

/**
 * Command Base64EncodeValue calls observer.
 */
TEST_F(AplMusicAlarmExtensionTest, InvokeSnooze)
{
    ASSERT_TRUE(registerExtension());

    auto command = Command("1.0").target(mClientToken)
        .uri(MusicAlarm::URI)
        .name("SnoozeAlarm");
    auto invoke = mExtension->invokeCommand(MusicAlarm::URI, command);
    ASSERT_TRUE(invoke);

    ASSERT_EQ(SNOOZECOMMAND, mObserver->mCommand);
}