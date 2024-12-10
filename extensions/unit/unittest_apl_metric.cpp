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
#include <rapidjson/document.h>

#include "alexaext/APLMetricsExtension/AplMetricsExtension.h"
#include "alexaext/extensionmessage.h"

using namespace alexaext;
using namespace alexaext::metrics;
using namespace rapidjson;

// Metric Commands
enum MetricCommand {
    NONE,
    RECORD_COUNTER,
    RECORD_TIMER
};

static const char* METRIC_ID = "metricId";
static const char* AMOUNT = "amount";

static const int MAX_METRIC_ID_ALLOWED = 5;

class TestMetricObserver : public AplMetricsExtensionObserverInterface {
public:
    TestMetricObserver() : AplMetricsExtensionObserverInterface(10) {}

    bool recordCounter(const std::string& applicationId, const std::string& experienceId,
                       const std::string& metricId, const int amount) override {
        mCommand = RECORD_COUNTER;
        mRecordedCounter = amount;
        return true;
    }

    bool recordTimer(const std::string& applicationId,
                     const std::string& experienceId,
                     const std::string& metricId,
                     const std::chrono::milliseconds& duration) override {
        mCommand = RECORD_TIMER;
        return true;
    }

    MetricCommand mCommand = NONE;
    int mRecordedCounter = 0;
};

class AplMetricsExtensionTest : public ::testing::Test {
public:
    void SetUp() override {
        mObserver = std::make_shared<TestMetricObserver>();
        mExtension = std::make_shared<AplMetricsExtension>(mObserver, Executor::getSynchronousExecutor(), MAX_METRIC_ID_ALLOWED);
    }

    /**
     * Simple registration for testing event/command/data.
     */
    ::testing::AssertionResult registerExtension(const alexaext::ActivityDescriptor& activity)
    {
        Document metricsSettings(kObjectType);
        metricsSettings.AddMember("applicationId", "TestApplication", metricsSettings.GetAllocator());
        metricsSettings.AddMember("experienceId", "TestExperience", metricsSettings.GetAllocator());
        Document regReq = RegistrationRequest("1.0")
            .uri(URI)
            .settings(metricsSettings);
        auto registration = mExtension->createRegistration(activity, regReq);
        auto method = GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, "Fail");
        if (std::strcmp("RegisterSuccess", method) != 0)
            return testing::AssertionFailure() << "Registration failed:" << method;

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

    /**
     * Create activity descriptors with a specific session
     */
    ActivityDescriptor createActivityDescriptor(SessionDescriptorPtr session,
                                                std::string uri = URI) {
        ActivityDescriptor activityDescriptor(
            uri,
            session);
        return activityDescriptor;
    }

    std::shared_ptr<TestMetricObserver> mObserver;
    std::shared_ptr<AplMetricsExtension> mExtension;
};


TEST_F(AplMetricsExtensionTest, RegistrationTest)
{
    Document settings(kObjectType);
    settings.AddMember("applicationId", "TestApplication", settings.GetAllocator());
    settings.AddMember("experienceId", "TestExperience", settings.GetAllocator());
    Document regReq = RegistrationRequest("1.0")
        .uri(URI)
        .settings(settings);

    auto activity = createActivityDescriptor();
    auto registration = mExtension->createRegistration(activity, regReq);
    ASSERT_STREQ("RegisterSuccess",
                 GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, ""));
    ASSERT_STREQ(URI.c_str(),
                 GetWithDefault<const char *>(RegistrationSuccess::URI(), registration, ""));
    std::string token = GetWithDefault<const char *>(RegistrationSuccess::TOKEN(), registration, "");
    ASSERT_EQ("<AUTO_TOKEN>",token);

    // Validate that fails to register again with same
    registration = mExtension->createRegistration(activity, regReq);
    ASSERT_STREQ("RegisterFailure",
                 GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, ""));
}

TEST_F(AplMetricsExtensionTest, InvalidURI)
{
    Document regReq = RegistrationRequest("aplext:metrics:INVALID");
    auto registration = mExtension->createRegistration(createActivityDescriptor("aplext:metrics:INVALID"), regReq);
    ASSERT_FALSE(registration.HasParseError());
    ASSERT_FALSE(registration.IsNull());
    ASSERT_STREQ("RegisterFailure",
                 GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, ""));
}

TEST_F(AplMetricsExtensionTest, RegistrationWithoutSettings) {
    Document regReq = RegistrationRequest(URI);
    auto registration = mExtension->createRegistration(createActivityDescriptor(), regReq);
    ASSERT_FALSE(registration.HasParseError());
    ASSERT_FALSE(registration.IsNull());
    ASSERT_STREQ("RegisterFailure",
                 GetWithDefault<const char*>(RegistrationSuccess::METHOD(), registration, ""));
}

TEST_F(AplMetricsExtensionTest, RegistrationWithoutApplicationId) {
    Document settings(kObjectType);
    settings.AddMember("experienceId", "TestExperience", settings.GetAllocator());
    Document regReq = RegistrationRequest("1.0")
        .uri(URI)
        .settings(settings);
    auto registration = mExtension->createRegistration(createActivityDescriptor(), regReq);
    ASSERT_FALSE(registration.HasParseError());
    ASSERT_FALSE(registration.IsNull());
    ASSERT_STREQ("RegisterFailure",
                 GetWithDefault<const char*>(RegistrationSuccess::METHOD(), registration, ""));
}

TEST_F(AplMetricsExtensionTest, RegistrationWithEmptyApplicationId) {
    Document settings(kObjectType);
    settings.AddMember("applicationId", "", settings.GetAllocator());
    settings.AddMember("experienceId", "TestExperience", settings.GetAllocator());
    Document regReq = RegistrationRequest("1.0")
        .uri(URI)
        .settings(settings);
    auto registration = mExtension->createRegistration(createActivityDescriptor(), regReq);
    ASSERT_FALSE(registration.HasParseError());
    ASSERT_FALSE(registration.IsNull());
    ASSERT_STREQ("RegisterFailure",
                 GetWithDefault<const char*>(RegistrationSuccess::METHOD(), registration, ""));
}

TEST_F(AplMetricsExtensionTest, RegistrationWithNullApplicationId) {
    Document settings(kObjectType);
    settings.AddMember("applicationId", Value(), settings.GetAllocator());
    settings.AddMember("experienceId", "TestExperience", settings.GetAllocator());
    Document regReq = RegistrationRequest("1.0")
        .uri(URI)
        .settings(settings);
    auto registration = mExtension->createRegistration(createActivityDescriptor(), regReq);
    ASSERT_FALSE(registration.HasParseError());
    ASSERT_FALSE(registration.IsNull());
    ASSERT_STREQ("RegisterFailure",
                 GetWithDefault<const char*>(RegistrationSuccess::METHOD(), registration, ""));
}

TEST_F(AplMetricsExtensionTest, RegistrationWithoutExperienceId) {
    Document settings(kObjectType);
    settings.AddMember("applicationId", "TestApplication", settings.GetAllocator());
    Document regReq = RegistrationRequest("1.0")
        .uri(URI)
        .settings(settings);
    auto registration = mExtension->createRegistration(createActivityDescriptor(), regReq);
    ASSERT_FALSE(registration.HasParseError());
    ASSERT_FALSE(registration.IsNull());
    ASSERT_STREQ("RegisterSuccess",
                 GetWithDefault<const char*>(RegistrationSuccess::METHOD(), registration, ""));
}

TEST_F(AplMetricsExtensionTest, RegistrationWithEmptyExperienceId) {
    Document settings(kObjectType);
    settings.AddMember("applicationId", "TestApplication", settings.GetAllocator());
    settings.AddMember("experienceId", "", settings.GetAllocator());
    Document regReq = RegistrationRequest("1.0")
        .uri(URI)
        .settings(settings);
    auto registration = mExtension->createRegistration(createActivityDescriptor(), regReq);
    ASSERT_FALSE(registration.HasParseError());
    ASSERT_FALSE(registration.IsNull());
    ASSERT_STREQ("RegisterSuccess",
                 GetWithDefault<const char*>(RegistrationSuccess::METHOD(), registration, ""));
}

TEST_F(AplMetricsExtensionTest, RegistrationWithNullExperienceId) {
    Document settings(kObjectType);
    settings.AddMember("applicationId", "TestApplication", settings.GetAllocator());
    settings.AddMember("experienceId", Value(), settings.GetAllocator());
    Document regReq = RegistrationRequest("1.0")
        .uri(URI)
        .settings(settings);
    auto registration = mExtension->createRegistration(createActivityDescriptor(), regReq);
    ASSERT_FALSE(registration.HasParseError());
    ASSERT_FALSE(registration.IsNull());
    ASSERT_STREQ("RegisterSuccess",
                 GetWithDefault<const char*>(RegistrationSuccess::METHOD(), registration, ""));
}

TEST_F(AplMetricsExtensionTest, RegistrationCommands)
{
    Document settings(kObjectType);
    settings.AddMember("applicationId", "TestApplication", settings.GetAllocator());
    settings.AddMember("experienceId", "TestExperience", settings.GetAllocator());
    Document regReq = RegistrationRequest("1.0")
        .uri(URI)
        .settings(settings);

    auto registration = mExtension->createRegistration(createActivityDescriptor(), regReq);
    Value *schema = RegistrationSuccess::SCHEMA().Get(registration);
    ASSERT_TRUE(schema);

    Value *commands = ExtensionSchema::COMMANDS().Get(*schema);
    ASSERT_TRUE(commands);

    auto expectedCommandSet = std::set<std::string>();
    expectedCommandSet.insert("IncrementCounter");
    expectedCommandSet.insert("StartTimer");
    expectedCommandSet.insert("StopTimer");
    ASSERT_TRUE(commands->IsArray() && commands->Size() == expectedCommandSet.size());

    for (const Value &com : commands->GetArray()) {
        ASSERT_TRUE(com.IsObject());
        auto name = GetWithDefault<const char *>(Command::NAME(), com, "MissingName");
        ASSERT_TRUE(expectedCommandSet.count(name) == 1) << "Unknown Command:" << name;
        expectedCommandSet.erase(name);
    }
    ASSERT_TRUE(expectedCommandSet.empty());
}

TEST_F(AplMetricsExtensionTest, RegistrationEvents)
{
    Document settings(kObjectType);
    settings.AddMember("applicationId", "TestApplication", settings.GetAllocator());
    settings.AddMember("experienceId", "TestExperience", settings.GetAllocator());
    Document regReq = RegistrationRequest("1.0")
        .uri(URI)
        .settings(settings);

    auto registration = mExtension->createRegistration(createActivityDescriptor(), regReq);
    Value *schema = RegistrationSuccess::SCHEMA().Get(registration);
    ASSERT_TRUE(schema);

    Value *events = ExtensionSchema::EVENTS().Get(*schema);
    ASSERT_TRUE(events);

    ASSERT_TRUE(events->IsArray() && events->Size() == 0);
}

TEST_F(AplMetricsExtensionTest, TestCommands) {
    auto activity = createActivityDescriptor();
    ASSERT_TRUE(registerExtension(activity));

    mObserver->mCommand = NONE;
    auto command = Command("1.0")
                       .uri(URI)
                       .name("IncrementCounter")
                       .property(METRIC_ID, "TestId")
                       .property(AMOUNT, 3);
    auto invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);

    // Test IncrementCounter without amount as float
    mObserver->mCommand = NONE;
    command = Command("1.0")
        .uri(URI)
        .name("IncrementCounter")
        .property(AMOUNT, 2.0)
        .property(METRIC_ID, "TestId");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);

    // Test IncrementCounter without amount property
    mObserver->mCommand = NONE;
    command = Command("1.0")
        .uri(URI)
        .name("IncrementCounter")
        .property(METRIC_ID, "TestId");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);

    // Test IncrementCounter with amount as int string
    mObserver->mCommand = NONE;
    command = Command("1.0")
        .uri(URI)
        .name("IncrementCounter")
        .property(METRIC_ID, "TestId")
        .property(AMOUNT, "2");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);

    // Test IncrementCounter with amount as double string
    mObserver->mCommand = NONE;
    command = Command("1.0")
        .uri(URI)
        .name("IncrementCounter")
        .property(METRIC_ID, "TestId")
        .property(AMOUNT, "2.53");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);

    mObserver->mCommand = NONE;
    command = Command("1.0")
        .uri(URI)
        .name("StartTimer")
        .property(METRIC_ID, "TestId");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    ASSERT_EQ(NONE, mObserver->mCommand);

    command = Command("1.0")
        .uri(URI)
        .name("StopTimer")
        .property(METRIC_ID, "TestId");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    ASSERT_EQ(RECORD_TIMER, mObserver->mCommand);

    mExtension->onSessionEnded(*activity.getSession());
    ASSERT_EQ(10, mObserver->mRecordedCounter);
    ASSERT_EQ(RECORD_COUNTER, mObserver->mCommand);
}

TEST_F(AplMetricsExtensionTest, TestMetricIdLimit) {
    auto activity = createActivityDescriptor();
    ASSERT_TRUE(registerExtension(activity));

    for (int i = 0; i < MAX_METRIC_ID_ALLOWED; i++) {
        auto command = Command("1.0")
                           .uri(URI)
                           .name("IncrementCounter")
                           .property(METRIC_ID, "TestId" + std::to_string(i))
                           .property(AMOUNT, 1);
        auto invoke = mExtension->invokeCommand(activity, command);
        ASSERT_TRUE(invoke);
    }

    auto command = Command("1.0")
        .uri(URI)
        .name("IncrementCounter")
        .property(METRIC_ID, "NewTestId")
        .property(AMOUNT, 1);
    auto invoke = mExtension->invokeCommand(activity, command);
    ASSERT_FALSE(invoke);

    command = Command("1.0")
        .uri(URI)
        .name("StartTimer")
        .property(METRIC_ID, "NewTestId");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_FALSE(invoke);
}

TEST_F(AplMetricsExtensionTest, TestCommandsWithInvalidActivity) {
    auto activity = createActivityDescriptor();
    ASSERT_TRUE(registerExtension(activity));

    mObserver->mCommand = NONE;
    auto command = Command("1.0")
        .uri(URI)
        .name("IncrementCounter")
        .property(METRIC_ID, "TestId");
    auto session = SessionDescriptor::create("TestSessionId");
    auto invalidActivity = createActivityDescriptor(session, "aplext:metrics:INVALID");
    auto invoke = mExtension->invokeCommand(invalidActivity, command);
    ASSERT_FALSE(invoke);

    command = Command("1.0")
        .uri(URI)
        .name("StartTimer")
        .property(METRIC_ID, "TestId");
    invoke = mExtension->invokeCommand(invalidActivity, command);
    ASSERT_FALSE(invoke);

    command = Command("1.0")
        .uri(URI)
        .name("StopTimer")
        .property(METRIC_ID, "TestId");
    invoke = mExtension->invokeCommand(invalidActivity, command);
    ASSERT_FALSE(invoke);
}

TEST_F(AplMetricsExtensionTest, TestCommandsWithInvalidSession) {
    auto activity = createActivityDescriptor();
    ASSERT_TRUE(registerExtension(activity));

    mObserver->mCommand = NONE;
    auto command = Command("1.0")
                       .uri(URI)
                       .name("IncrementCounter")
                       .property(METRIC_ID, "TestId");
    auto session = SessionDescriptor::create("Session1");
    auto invoke = mExtension->invokeCommand(createActivityDescriptor(session), command);
    ASSERT_FALSE(invoke);
}

TEST_F(AplMetricsExtensionTest, TestInvalidCommands) {
    auto activity = createActivityDescriptor();
    ASSERT_TRUE(registerExtension(activity));

    // Invalid command name
    auto command = Command("1.0")
                       .uri(URI)
                       .name("InvalidCommand")
                       .property(METRIC_ID, "TestId")
                       .property(AMOUNT, 1);
    auto invoke = mExtension->invokeCommand(activity, command);
    ASSERT_FALSE(invoke);

    // MetricId property missing
    command = Command("1.0")
        .uri(URI)
        .name("IncrementCounter")
        .property(AMOUNT, 1);
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_FALSE(invoke);

    command = Command("1.0")
        .uri(URI)
        .name("StartTimer");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_FALSE(invoke);

    command = Command("1.0")
        .uri(URI)
        .name("StopTimer");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_FALSE(invoke);

    // MetricId is empty
    command = Command("1.0")
        .uri(URI)
        .name("IncrementCounter")
        .property(METRIC_ID, "")
        .property(AMOUNT, 1);
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_FALSE(invoke);

    command = Command("1.0")
        .uri(URI)
        .property(METRIC_ID, "")
        .name("StartTimer");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_FALSE(invoke);

    command = Command("1.0")
        .uri(URI)
        .property(METRIC_ID, "")
        .name("StopTimer");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_FALSE(invoke);

}

TEST_F(AplMetricsExtensionTest, TestTimerMetricCommand) {
    auto activity = createActivityDescriptor();
    ASSERT_TRUE(registerExtension(activity));

    // Test stop without start
    mObserver->mCommand = NONE;
    auto command = Command("1.0")
        .uri(URI)
        .name("StopTimer")
        .property(METRIC_ID, "TestId");
    auto invoke = mExtension->invokeCommand(activity, command);
    ASSERT_FALSE(invoke);
    ASSERT_EQ(NONE, mObserver->mCommand);

    mObserver->mCommand = NONE;
    command = Command("1.0")
        .uri(URI)
        .name("StartTimer")
        .property(METRIC_ID, "TestId");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    ASSERT_EQ(NONE, mObserver->mCommand);

    mObserver->mCommand = NONE;
    command = Command("1.0")
        .uri(URI)
        .name("StopTimer")
        .property(METRIC_ID, "TestId");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    ASSERT_EQ(RECORD_TIMER, mObserver->mCommand);

    // Stop again
    mObserver->mCommand = NONE;
    command = Command("1.0")
        .uri(URI)
        .name("StopTimer")
        .property(METRIC_ID, "TestId");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_FALSE(invoke);
    ASSERT_EQ(NONE, mObserver->mCommand);

    mObserver->mCommand = NONE;
    command = Command("1.0")
        .uri(URI)
        .name("StartTimer")
        .property(METRIC_ID, "TestId");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    ASSERT_EQ(NONE, mObserver->mCommand);
}

TEST_F(AplMetricsExtensionTest, TestTimerMetricWithinSession) {
    auto session1 = SessionDescriptor::create("Session1");
    auto activity1 = createActivityDescriptor(session1);
    ASSERT_TRUE(registerExtension(activity1));

    mObserver->mCommand = NONE;
    auto command = Command("1.0")
        .uri(URI)
        .name("StartTimer")
        .property(METRIC_ID, "TestId1");
    auto invoke = mExtension->invokeCommand(activity1, command);
    ASSERT_TRUE(invoke);
    ASSERT_EQ(NONE, mObserver->mCommand);

    // Register another activity with same session
    auto activity2 = createActivityDescriptor(session1);
    ASSERT_TRUE(registerExtension(activity2));

    mObserver->mCommand = NONE;
    command = Command("1.0")
        .uri(URI)
        .name("StopTimer")
        .property(METRIC_ID, "TestId1");
    invoke = mExtension->invokeCommand(activity2, command);
    ASSERT_TRUE(invoke);
    ASSERT_EQ(RECORD_TIMER, mObserver->mCommand);

    // Start another timer in activity1
    mObserver->mCommand = NONE;
    command = Command("1.0")
        .uri(URI)
        .name("StartTimer")
        .property(METRIC_ID, "TestId2");
    invoke = mExtension->invokeCommand(activity1, command);
    ASSERT_TRUE(invoke);
    ASSERT_EQ(NONE, mObserver->mCommand);

    // Register another activity with different session and try to stop timer
    auto activity3 = createActivityDescriptor(SessionDescriptor::create("Session2"));
    ASSERT_TRUE(registerExtension(activity3));
    command = Command("1.0")
        .uri(URI)
        .name("StopTimer")
        .property(METRIC_ID, "TestId2");
    invoke = mExtension->invokeCommand(activity3, command);
    ASSERT_FALSE(invoke);
    ASSERT_EQ(NONE, mObserver->mCommand);
}

TEST_F(AplMetricsExtensionTest, TestCounterMetricWithinSession) {
    auto session1 = SessionDescriptor::create("Session1");
    auto activity1 = createActivityDescriptor(session1);
    ASSERT_TRUE(registerExtension(activity1));

    // Increment counter in activity1
    mObserver->mCommand = NONE;
    auto command = Command("1.0")
        .uri(URI)
        .name("IncrementCounter")
        .property(METRIC_ID, "TestId");
    auto invoke = mExtension->invokeCommand(activity1, command);
    ASSERT_TRUE(invoke);

    // Register activity2 with same session
    auto activity2 = createActivityDescriptor(session1);
    ASSERT_TRUE(registerExtension(activity2));

    // Increment counter in activity2
    command = Command("1.0")
        .uri(URI)
        .name("IncrementCounter")
        .property(METRIC_ID, "TestId");
    invoke = mExtension->invokeCommand(activity2, command);
    ASSERT_TRUE(invoke);

    // Increment counter again in activity1 by amount 2.
    command = Command("1.0")
        .uri(URI)
        .name("IncrementCounter")
        .property(METRIC_ID, "TestId")
        .property(AMOUNT, 2);
    invoke = mExtension->invokeCommand(activity1, command);
    ASSERT_TRUE(invoke);

    // Register another activity with different session and increment counter
    auto session2 = SessionDescriptor::create("Session2");
    auto activity3 = createActivityDescriptor(session2);
    ASSERT_TRUE(registerExtension(activity3));
    command = Command("1.0")
        .uri(URI)
        .name("IncrementCounter")
        .property(METRIC_ID, "TestId")
        .property(AMOUNT, 10);
    invoke = mExtension->invokeCommand(activity3, command);
    ASSERT_TRUE(invoke);

    // Observer not invoked before session end
    ASSERT_EQ(NONE, mObserver->mCommand);

    // End session1
    mExtension->onSessionEnded(*session1);
    ASSERT_EQ(RECORD_COUNTER, mObserver->mCommand);
    ASSERT_EQ(4, mObserver->mRecordedCounter);

    mObserver->mCommand = NONE;

    // End session2
    mExtension->onSessionEnded(*session2);
    ASSERT_EQ(RECORD_COUNTER, mObserver->mCommand);
    ASSERT_EQ(10, mObserver->mRecordedCounter);
}

TEST_F(AplMetricsExtensionTest, TestCommandWithInvalidActivity)
{
    auto activity = createActivityDescriptor();
    ASSERT_TRUE(registerExtension(activity));

    auto command = Command("1.0")
        .uri(URI)
        .name("IncrementCounter")
        .property(AMOUNT, 1);
    auto invoke = mExtension->invokeCommand(createActivityDescriptor(), command);
    ASSERT_FALSE(invoke);
}

TEST_F(AplMetricsExtensionTest, TestCommandWithInvalidURI)
{
    auto activity = createActivityDescriptor();
    ASSERT_TRUE(registerExtension(activity));

    auto command = Command("1.0")
        .uri("aplext:metrics:INVALID")
        .name("IncrementCounter")
        .property(AMOUNT, 1);
    auto invoke = mExtension->invokeCommand(activity, command);
    ASSERT_FALSE(invoke);
}