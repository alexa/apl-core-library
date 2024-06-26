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
#include <chrono>
#include <future>
#include <rapidjson/document.h>
#include <thread>

#include "alexaext/APLMetricsExtensionV2/AplMetricsExtensionV2.h"
#include "alexaext/APLMetricsExtensionV2/DestinationInterface.h"
#include "alexaext/extensionmessage.h"

using namespace alexaext;
using namespace alexaext::metricsExtensionV2;
using namespace rapidjson;

const char* const DESTINATION = "destination";
const char* const DIMENSIONS = "dimensions";

static const char* METRIC_ID = "metricId";
static const char* AMOUNT = "amount";

bool shouldCreateDestinationSucceed;
bool shouldCreateDestinationBeCalled;

#define AssertPublishMetricForDestination(isQueued, isPublished, destination)                      \
    {                                                                                              \
        ASSERT_EQ(isQueued, mExecutor->taskQueued);                                                \
        if (destination) {                                                                         \
            ASSERT_EQ(isPublished, (destination)->metricPublished());                              \
        }                                                                                          \
    }

#define AssertLastPublishMetric(isQueued, isPublished)                                             \
    { AssertPublishMetricForDestination(isQueued, isPublished, mDestFactory->lastDestinationMock); }

#define resetPublishedFlagsForDestination(destination)                                             \
    {                                                                                              \
        (destination)->publishMetricsCalled = false;                                               \
        (destination)->publishAllMetricsCalled = false;                                            \
    }

#define resetLastPublishedFlags()                                                                  \
    { resetPublishedFlagsForDestination(mDestFactory->lastDestinationMock) }

class DestinationInterfaceMock : public DestinationInterface {
public:
    DestinationInterfaceMock() : publishMetricsCalled(false), publishAllMetricsCalled(false) {}
    void publish(Metric metric) {
        publishMetricsCalled = true;
        lastPublishedMetric = metric;
    }

    void publish(std::vector<Metric> metric) {
        publishAllMetricsCalled = true;
        lastPublishedMetricList = metric;
    }

    bool metricPublished() { return publishMetricsCalled || publishAllMetricsCalled; }

    bool publishMetricsCalled;
    bool publishAllMetricsCalled;
    Metric lastPublishedMetric;
    std::vector<Metric> lastPublishedMetricList;
};

class DestinationFactoryInterfaceMock : public DestinationFactoryInterface {
public:
    DestinationFactoryInterfaceMock() : createDestinationCalled(false) {}

    std::shared_ptr<DestinationInterface> createDestination(const rapidjson::Value& mSettings) {
        createDestinationCalled = true;
        if (!shouldCreateDestinationSucceed) {
            return nullptr;
        }

        lastDestinationMock = std::make_shared<DestinationInterfaceMock>();
        return lastDestinationMock;
    }

    std::shared_ptr<DestinationInterfaceMock> lastDestinationMock;
    bool createDestinationCalled;
};

class MockExecutor : public Executor {
public:
    MockExecutor() {
        taskQueued = false;
        mExecutorImpl = Executor::getSynchronousExecutor();
    }
    bool enqueueTask(Task task) {
        taskQueued = true;
        return mExecutorImpl->enqueueTask(std::move(task));
    }

    void resetFlag() { taskQueued = false; }

    ExecutorPtr mExecutorImpl;
    bool taskQueued;
};

class AplMetricsExtension2Test : public ::testing::Test {
public:
    void SetUp() override {
        shouldCreateDestinationSucceed = true;
        shouldCreateDestinationBeCalled = true;
        mDestFactory = std::make_shared<DestinationFactoryInterfaceMock>();

        mExecutor = std::make_shared<MockExecutor>();
        mExtension = std::make_shared<AplMetricsExtensionV2>(mDestFactory, mExecutor);
    }

    void TearDown() override {
        if (shouldCreateDestinationBeCalled) {
            ASSERT_TRUE(mDestFactory->createDestinationCalled);
        }
    }

    /**
     * Simple registration for testing event/command/data.
     */
    rapidjson::Document registerExtension(const alexaext::ActivityDescriptor& activity,
                                          const Dimensions& dimensions = Dimensions(),
                                          bool withDestination = true, bool withDimension = true,
                                          bool withDestinationType = true) {
        Document metricsSettings(kObjectType);
        rapidjson::Value destination(kObjectType);

        if (withDestination) {
            metricsSettings.AddMember(
                rapidjson::Value(DESTINATION, metricsSettings.GetAllocator()).Move(),
                destination.Move(), metricsSettings.GetAllocator());

            metricsSettings[DESTINATION].AddMember("groupId", "gid",
                                                   metricsSettings.GetAllocator());
            metricsSettings[DESTINATION].AddMember("schemaId", "schemaId",
                                                   metricsSettings.GetAllocator());
            if (withDestinationType) {
                metricsSettings[DESTINATION].AddMember("type", "anyDestinationType",
                                                       metricsSettings.GetAllocator());
            }
        }

        if (withDimension) {
            rapidjson::Value dimension(kObjectType);
            metricsSettings.AddMember(
                rapidjson::Value(DIMENSIONS, metricsSettings.GetAllocator()).Move(),
                dimension.Move(), metricsSettings.GetAllocator());

            for (auto& dimension : dimensions) {
                metricsSettings[DIMENSIONS].AddMember(
                    rapidjson::Value(dimension.first.c_str(), metricsSettings.GetAllocator())
                        .Move(),
                    rapidjson::Value(dimension.second.c_str(), metricsSettings.GetAllocator())
                        .Move(),
                    metricsSettings.GetAllocator());
            }
        }

        Document regReq = RegistrationRequest("2.0").uri(URI_V2).settings(metricsSettings);
        return mExtension->createRegistration(activity, regReq);
    }

    /**
     * Simple utility to create activity descriptors accross
     * the tests
     */
    ActivityDescriptor createActivityDescriptor(std::string uri = URI_V2) {
        // Create Activity
        SessionDescriptorPtr sessionPtr = SessionDescriptor::create("TestSessionId");
        ActivityDescriptor activityDescriptor(uri, sessionPtr);
        return activityDescriptor;
    }

    /**
     * Create activity descriptors with a specific session
     */
    ActivityDescriptor createActivityDescriptor(SessionDescriptorPtr session,
                                                std::string uri = URI_V2) {
        ActivityDescriptor activityDescriptor(uri, session);
        return activityDescriptor;
    }

    std::shared_ptr<MockExecutor> mExecutor;
    std::shared_ptr<AplMetricsExtensionV2> mExtension;
    std::shared_ptr<DestinationFactoryInterfaceMock> mDestFactory;
};

TEST_F(AplMetricsExtension2Test, RegistrationTest) {
    Dimensions dim;
    dim["experienceId"] = "photos";

    auto activity = createActivityDescriptor();
    auto registration = registerExtension(activity, dim);

    auto method = GetWithDefault<const char*>(RegistrationSuccess::METHOD(), registration, "Fail");
    if (std::strcmp("RegisterSuccess", method) != 0) {
        FAIL() << "Failed Registration:" << method;
    }
}

TEST_F(AplMetricsExtension2Test, InvalidURI) {
    Document regReq = RegistrationRequest("aplext:metrics:INVALID");
    auto reg2 =
        mExtension->createRegistration(createActivityDescriptor("aplext:metrics:INVALID"), regReq);
    auto method = GetWithDefault<const char*>(RegistrationSuccess::METHOD(), reg2, "Fail");
    if (std::strcmp("RegisterSuccess", method) != 0) {
        SUCCEED() << "Invalid URI not accepted" << method;
    }
    else {
        FAIL() << "Registration succeeded" << method;
    }

    shouldCreateDestinationBeCalled = false;
}

TEST_F(AplMetricsExtension2Test, RegistrationWithoutSettings) {
    Document regReq = RegistrationRequest(URI_V2);
    auto registration = mExtension->createRegistration(createActivityDescriptor(), regReq);
    ASSERT_FALSE(registration.HasParseError());
    ASSERT_FALSE(registration.IsNull());
    ASSERT_STREQ("RegisterFailure",
                 GetWithDefault<const char*>(RegistrationSuccess::METHOD(), registration, ""));
    shouldCreateDestinationBeCalled = false;
}

TEST_F(AplMetricsExtension2Test, RegistrationWithoutDestination) {
    Dimensions dim;
    dim["experienceId"] = "photos";

    auto registration = registerExtension(createActivityDescriptor(), dim, false);
    auto method = GetWithDefault<const char*>(RegistrationSuccess::METHOD(), registration, "Fail");
    if (std::strcmp("RegisterSuccess", method) != 0) {
        SUCCEED() << "Destination should be present" << method;
    }

    shouldCreateDestinationBeCalled = false;
}

TEST_F(AplMetricsExtension2Test, ReRegistrationTest) {
    Dimensions dim;
    dim["experienceId"] = "photos";

    auto activity = createActivityDescriptor();
    auto reg1 = registerExtension(activity, dim);
    auto method = GetWithDefault<const char*>(RegistrationSuccess::METHOD(), reg1, "Fail");
    if (std::strcmp("RegisterSuccess", method) != 0) {
        FAIL() << "Failed Registration for " << method;
    }
    ASSERT_TRUE(mDestFactory->createDestinationCalled);
    mDestFactory->createDestinationCalled = false;

    auto reg2 = registerExtension(activity, dim);
    method = GetWithDefault<const char*>(RegistrationSuccess::METHOD(), reg2, "Fail");
    if (std::strcmp("RegisterSuccess", method) != 0) {
        SUCCEED() << "Failed Registration is correct for: " << method;
    }
}

TEST_F(AplMetricsExtension2Test, RegistrationWithEmptyDimensions) {
    auto reg1 = registerExtension(createActivityDescriptor());

    auto method = GetWithDefault<const char*>(RegistrationSuccess::METHOD(), reg1, "Fail");
    if (std::strcmp("RegisterSuccess", method) != 0) {
        FAIL() << "Failed Registration:" << method;
    }
}

TEST_F(AplMetricsExtension2Test, RegistrationWithNoDimensions) {
    Dimensions dim;
    auto reg1 = registerExtension(createActivityDescriptor(), dim, true, false);

    auto method = GetWithDefault<const char*>(RegistrationSuccess::METHOD(), reg1, "Fail");
    if (std::strcmp("RegisterSuccess", method) != 0) {
        SUCCEED() << "Empty dimensions are allowed : " << method;
    }

    shouldCreateDestinationBeCalled = false;
}

TEST_F(AplMetricsExtension2Test, RegistrationWithCreateDestinationFailed) {
    shouldCreateDestinationSucceed = false;
    auto reg1 = registerExtension(createActivityDescriptor());

    auto method = GetWithDefault<const char*>(RegistrationSuccess::METHOD(), reg1, "Fail");
    if (std::strcmp("RegisterSuccess", method) != 0) {
        SUCCEED() << "Failed Registration:" << method;
    }
}

TEST_F(AplMetricsExtension2Test, TestCreateCounter100Dim) {
    auto activity = createActivityDescriptor();
    auto reg = registerExtension(activity);

    std::string _1000Dimensions = "{";
    for (int i = 0; i < 100; i++) {
        _1000Dimensions.append("\"key\"=\"so value for the key\"");
    }
    _1000Dimensions += "}";
    auto command = Command("1.0")
                       .uri(URI_V2)
                       .name("CreateCounter")
                       .property(METRIC_ID, "TestId")
                       .property("metricName", "testName")
                       .property("initialValue", 101)
                       .property("dimensions", _1000Dimensions.c_str());
    auto invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    mExtension->onActivityUnregistered(activity);
    auto metricList = mDestFactory->lastDestinationMock->lastPublishedMetricList;
    ASSERT_EQ(metricList.size(), 1);
    ASSERT_EQ(101, metricList[0].value);
}

TEST_F(AplMetricsExtension2Test, TestCreateCounter) {
    auto activity = createActivityDescriptor();
    auto reg = registerExtension(activity);

    auto command = Command("1.0")
                       .uri(URI_V2)
                       .name("CreateCounter")
                       .property(METRIC_ID, "TestId")
                       .property("metricName", "testName")
                       .property("initialValue", 101);
    auto invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    mExtension->onActivityUnregistered(activity);
    auto metricList = mDestFactory->lastDestinationMock->lastPublishedMetricList;
    ASSERT_EQ(metricList.size(), 1);
    ASSERT_EQ(101, metricList[0].value);
}

TEST_F(AplMetricsExtension2Test, TestIncrementCounter) {
    auto activity = createActivityDescriptor();
    registerExtension(activity);

    // Creates counter if not present
    auto command = Command("1.0")
                       .uri(URI_V2)
                       .name("IncrementCounter")
                       .property(METRIC_ID, "TestId")
                       .property(AMOUNT, 3);
    auto invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    // Test IncrementCounter without amount property
    command = Command("1.0")
                  .uri(URI_V2)
                  .name("IncrementCounter")
                  .property(METRIC_ID, "TestId")
                  .property(AMOUNT, 2);
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);

    // Default Increment by 1
    command = Command("1.0").uri(URI_V2).name("IncrementCounter").property(METRIC_ID, "TestId");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);

    mExtension->onActivityUnregistered(activity);

    auto metricList = mDestFactory->lastDestinationMock->lastPublishedMetricList;
    ASSERT_EQ(6, metricList[0].value);
}

TEST_F(AplMetricsExtension2Test, TestTimerMetric) {
    auto activity = createActivityDescriptor();
    auto reg = registerExtension(activity);

    Document dimensionsDoc(kObjectType);
    rapidjson::Value dimension(kObjectType);
    dimension.AddMember("dim1", "dimVal1", dimensionsDoc.GetAllocator());
    dimension.AddMember("dim2", "dimVal2", dimensionsDoc.GetAllocator());
    dimension.AddMember("dim3", "dimVal2", dimensionsDoc.GetAllocator());
    dimension.AddMember("dim4", "dimVal2", dimensionsDoc.GetAllocator());

    auto command = Command("1.0")
                       .uri(URI_V2)
                       .name("StartTimer")
                       .property(METRIC_ID, "TestId")
                       .property("metricName", "testName")
                       .property("dimensions", dimension);
    auto invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    command = Command("1.0").uri(URI_V2).name("StopTimer").property(METRIC_ID, "TestId");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);

    auto metric = mDestFactory->lastDestinationMock->lastPublishedMetric;
    ASSERT_TRUE(0 == metric.name.compare("testName"));
    ASSERT_TRUE(4 == metric.dimensions.size());
}

TEST_F(AplMetricsExtension2Test, TestRecordValueMetric) {
    auto activity = createActivityDescriptor();
    auto reg = registerExtension(activity);

    Document dimensionsDoc(kObjectType);
    rapidjson::Value dimension(kObjectType);
    dimension.AddMember("dim1", "dimVal1", dimensionsDoc.GetAllocator());

    auto command = Command("1.0")
                       .uri(URI_V2)
                       .name("RecordValue")
                       .property("metricName", "valueName")
                       .property("value", 563)
                       .property("dimensions", dimension);
    auto invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);

    auto metric = mDestFactory->lastDestinationMock->lastPublishedMetric;
    ASSERT_TRUE(0 == metric.name.compare("valueName"));
    ASSERT_TRUE(563 == metric.value);
    ASSERT_TRUE(1 == metric.dimensions.size());
}

TEST_F(AplMetricsExtension2Test, TestDimensionsParsing) {
    auto activity = createActivityDescriptor();
    auto reg = registerExtension(activity);

    Document dimensionsDoc(kObjectType);
    rapidjson::Value dimension(kObjectType);
    dimension.AddMember("dim1", "dimVal1", dimensionsDoc.GetAllocator());
    dimension.AddMember("dim2", "dimVal2", dimensionsDoc.GetAllocator());
    dimension.AddMember("dim3", "dimVal3", dimensionsDoc.GetAllocator());
    dimension.AddMember("dim4", "dimVal4", dimensionsDoc.GetAllocator());

    auto command = Command("1.0")
                       .uri(URI_V2)
                       .name("RecordValue")
                       .property("metricName", "valueName")
                       .property("value", 563)
                       .property("dimensions", dimension);
    auto invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);

    auto metric = mDestFactory->lastDestinationMock->lastPublishedMetric;
    ASSERT_TRUE(4 == metric.dimensions.size());

    for (auto dim : metric.dimensions) {
        if (dim.first == "dim1") {
            ASSERT_STRCASEEQ(dim.second.c_str(), "dimVal1");
        }
        else if (dim.first == "dim2") {
            ASSERT_STRCASEEQ(dim.second.c_str(), "dimVal2");
        }
        else if (dim.first == "dim3") {
            ASSERT_STRCASEEQ(dim.second.c_str(), "dimVal3");
        }
        else if (dim.first == "dim4") {
            ASSERT_STRCASEEQ(dim.second.c_str(), "dimVal4");
        }
        else {
            FAIL() << "Dimension has value that was not part of command";
        }
    }
}

TEST_F(AplMetricsExtension2Test, RegistrationCommands) {
    auto registration = registerExtension(createActivityDescriptor());
    Value* schema = RegistrationSuccess::SCHEMA().Get(registration);
    ASSERT_TRUE(schema);

    Value* commands = ExtensionSchema::COMMANDS().Get(*schema);
    ASSERT_TRUE(commands);

    auto expectedCommandSet = std::set<std::string>();
    expectedCommandSet.insert("IncrementCounter");
    expectedCommandSet.insert("StartTimer");
    expectedCommandSet.insert("StopTimer");
    expectedCommandSet.insert("CreateCounter");
    expectedCommandSet.insert("RecordValue");
    ASSERT_TRUE(commands->IsArray() && commands->Size() == expectedCommandSet.size());

    for (const Value& com : commands->GetArray()) {
        ASSERT_TRUE(com.IsObject());
        auto name = GetWithDefault<const char*>(Command::NAME(), com, "MissingName");
        ASSERT_TRUE(expectedCommandSet.count(name) == 1) << "Unknown Command:" << name;
        expectedCommandSet.erase(name);
    }
    ASSERT_TRUE(expectedCommandSet.empty());
}

TEST_F(AplMetricsExtension2Test, TestCommandsWithInvalidActivity) {
    auto session = SessionDescriptor::create("TestSessionId");
    auto reg = registerExtension(createActivityDescriptor(session));

    auto command =
        Command("1.0").uri(URI_V2).name("IncrementCounter").property(METRIC_ID, "TestId");
    auto invalidActivity = createActivityDescriptor(session, "aplext:metrics:INVALID");
    auto invoke = mExtension->invokeCommand(invalidActivity, command);
    ASSERT_TRUE(invoke);
    AssertLastPublishMetric(true, false);

    command = Command("1.0").uri(URI_V2).name("StopTimer").property(METRIC_ID, "TestId");
    invoke = mExtension->invokeCommand(invalidActivity, command);
    ASSERT_TRUE(invoke);
    AssertLastPublishMetric(true, false);
}

TEST_F(AplMetricsExtension2Test, TestCommandsWithInvalidSession) {
    auto reg = registerExtension(createActivityDescriptor());

    auto command =
        Command("1.0").uri(URI_V2).name("IncrementCounter").property(METRIC_ID, "TestId");
    auto session = SessionDescriptor::create("Session1");
    auto invoke = mExtension->invokeCommand(createActivityDescriptor(session), command);
    ASSERT_TRUE(invoke);
    AssertLastPublishMetric(true, false);
}

TEST_F(AplMetricsExtension2Test, TestInvalidCommands) {
    auto activity = createActivityDescriptor();
    registerExtension(activity);

    mExecutor->resetFlag();
    // Invalid command name
    auto command = Command("1.0")
                       .uri(URI_V2)
                       .name("InvalidCommand")
                       .property(METRIC_ID, "TestId")
                       .property(AMOUNT, 1);
    auto invoke = mExtension->invokeCommand(activity, command);
    ASSERT_FALSE(invoke);

    //========================
    //  MetricId property missing
    command = Command("1.0").uri(URI_V2).name("IncrementCounter").property(AMOUNT, 1);
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke); // fail soft
    AssertLastPublishMetric(false, false);

    command = Command("1.0").uri(URI_V2).name("StartTimer");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    AssertLastPublishMetric(false, false);

    command = Command("1.0").uri(URI_V2).name("StopTimer");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    AssertLastPublishMetric(false, false);

    command = Command("1.0").uri(URI_V2).name("CreateCounter");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    AssertLastPublishMetric(false, false);
    //
    //========================

    //========================
    // MetricId is empty
    command = Command("1.0")
                  .uri(URI_V2)
                  .name("IncrementCounter")
                  .property(METRIC_ID, "")
                  .property(AMOUNT, 1);
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    AssertLastPublishMetric(false, false);

    command = Command("1.0").uri(URI_V2).property(METRIC_ID, "").name("StartTimer");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    AssertLastPublishMetric(false, false);

    command = Command("1.0").uri(URI_V2).property(METRIC_ID, "").name("StopTimer");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    AssertLastPublishMetric(false, false);

    command = Command("1.0").uri(URI_V2).property(METRIC_ID, "").name("CreateCounter");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    AssertLastPublishMetric(false, false);
    //
    //========================
}

TEST_F(AplMetricsExtension2Test, TestTimerMetricCommand) {
    auto activity = createActivityDescriptor();
    registerExtension(activity);

    // Test stop without start
    mExecutor->resetFlag();
    auto command = Command("1.0").uri(URI_V2).name("StopTimer").property(METRIC_ID, "TestId");
    auto invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    AssertLastPublishMetric(true, false);

    mExecutor->resetFlag();
    command = Command("1.0").uri(URI_V2).name("StartTimer").property(METRIC_ID, "TestId");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    AssertLastPublishMetric(true, false);

    mExecutor->resetFlag();
    command = Command("1.0").uri(URI_V2).name("StopTimer").property(METRIC_ID, "TestId");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    AssertLastPublishMetric(true, true);

    auto metric = mDestFactory->lastDestinationMock->lastPublishedMetric;
    ASSERT_TRUE(0 == metric.name.compare("TestId")); // Metric name should be MeticId
    ASSERT_TRUE(0 == metric.dimensions.size());

    // Stop again
    mExecutor->resetFlag();
    resetLastPublishedFlags();
    command = Command("1.0").uri(URI_V2).name("StopTimer").property(METRIC_ID, "TestId");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    AssertLastPublishMetric(true, false);

    mExecutor->resetFlag();
    command = Command("1.0").uri(URI_V2).name("StartTimer").property(METRIC_ID, "TestId");
    invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    AssertLastPublishMetric(true, false);
}

TEST_F(AplMetricsExtension2Test, TestMultipleTimerMetricWithinSession) {
    auto session1 = SessionDescriptor::create("Session1");
    auto activity1 = createActivityDescriptor(session1);
    registerExtension(activity1);
    auto destination1 = mDestFactory->lastDestinationMock;

    mExecutor->resetFlag();
    auto command = Command("1.0").uri(URI_V2).name("StartTimer").property(METRIC_ID, "TestId1");
    auto invoke = mExtension->invokeCommand(activity1, command);
    ASSERT_TRUE(invoke);

    // Register another activity with same session
    auto activity2 = createActivityDescriptor(session1);
    registerExtension(activity2);
    auto destination2 = mDestFactory->lastDestinationMock;

    // Start another timer in activity1
    mExecutor->resetFlag();
    command = Command("1.0").uri(URI_V2).name("StartTimer").property(METRIC_ID, "TestId2");
    invoke = mExtension->invokeCommand(activity2, command);
    ASSERT_TRUE(invoke);

    // Stop timer with id: TestId1, but with activity2. It should fail as `TestId1` was started
    // by activity1
    mExecutor->resetFlag();
    command = Command("1.0").uri(URI_V2).name("StopTimer").property(METRIC_ID, "TestId1");
    invoke = mExtension->invokeCommand(activity2, command);
    ASSERT_TRUE(invoke);
    AssertPublishMetricForDestination(true, false, destination2);

    // Stop first timer with id: TestId1
    mExecutor->resetFlag();
    resetPublishedFlagsForDestination(destination1);
    command = Command("1.0").uri(URI_V2).name("StopTimer").property(METRIC_ID, "TestId1");
    invoke = mExtension->invokeCommand(activity1, command);
    ASSERT_TRUE(invoke);
    AssertPublishMetricForDestination(true, true, destination1);

    // Register another activity with different session and try to stop timer
    auto activity3 = createActivityDescriptor(session1);
    registerExtension(activity3);
    auto destination3 = mDestFactory->lastDestinationMock;

    mExecutor->resetFlag();
    resetPublishedFlagsForDestination(destination3);
    command = Command("1.0").uri(URI_V2).name("StopTimer").property(METRIC_ID, "TestId2");
    invoke = mExtension->invokeCommand(activity3, command);
    ASSERT_TRUE(invoke);
    AssertPublishMetricForDestination(true, false, destination3);

    // Stop second timer with id: TestId2
    mExecutor->resetFlag();
    resetPublishedFlagsForDestination(destination2);
    command = Command("1.0").uri(URI_V2).name("StopTimer").property(METRIC_ID, "TestId2");
    invoke = mExtension->invokeCommand(activity2, command);
    ASSERT_TRUE(invoke);
    AssertPublishMetricForDestination(true, true, destination2);
}

TEST_F(AplMetricsExtension2Test, TestMultipleCounterMetricWithinSession) {
    auto session1 = SessionDescriptor::create("Session1");
    auto activity1 = createActivityDescriptor(session1);
    registerExtension(activity1);
    auto destination1 = mDestFactory->lastDestinationMock;

    // Increment counter in activity1
    auto command =
        Command("1.0").uri(URI_V2).name("IncrementCounter").property(METRIC_ID, "TestId");
    auto invoke = mExtension->invokeCommand(activity1, command);
    ASSERT_TRUE(invoke);

    // Register activity2 with same session
    auto activity2 = createActivityDescriptor(session1);
    registerExtension(activity2);
    auto destination2 = mDestFactory->lastDestinationMock;

    // Increment counter in activity2
    // Should create a new counter with same ID for this activity
    invoke = mExtension->invokeCommand(activity2, command);
    ASSERT_TRUE(invoke);

    // Increment counter again in activity1 by amount 2.
    command = Command("1.0")
                  .uri(URI_V2)
                  .name("IncrementCounter")
                  .property(METRIC_ID, "TestId")
                  .property(AMOUNT, 2);
    invoke = mExtension->invokeCommand(activity1, command);
    ASSERT_TRUE(invoke);

    // Increment counter activity2 by amount 100.
    command = Command("1.0")
                  .uri(URI_V2)
                  .name("IncrementCounter")
                  .property(METRIC_ID, "TestId")
                  .property(AMOUNT, 100);
    invoke = mExtension->invokeCommand(activity2, command);
    ASSERT_TRUE(invoke);

    // Register another activity with different session and increment counter
    auto activity3 = createActivityDescriptor(session1);
    registerExtension(activity3);
    auto destination3 = mDestFactory->lastDestinationMock;

    command = Command("1.0")
                  .uri(URI_V2)
                  .name("IncrementCounter")
                  .property(METRIC_ID, "TestId")
                  .property(AMOUNT, 45);
    invoke = mExtension->invokeCommand(activity3, command);
    ASSERT_TRUE(invoke);

    // End activity
    mExtension->onActivityUnregistered(activity1);
    mExtension->onActivityUnregistered(activity2);
    mExtension->onActivityUnregistered(activity3);

    ASSERT_EQ(1, destination1->lastPublishedMetricList.size());
    ASSERT_EQ(destination1->lastPublishedMetricList[0].name.compare("TestId"), 0);
    ASSERT_EQ(destination1->lastPublishedMetricList[0].value, 3);
    ASSERT_NE(destination1, destination2);

    ASSERT_EQ(1, destination2->lastPublishedMetricList.size());
    ASSERT_EQ(destination2->lastPublishedMetricList[0].name.compare("TestId"), 0);
    ASSERT_EQ(destination2->lastPublishedMetricList[0].value, 101);
    ASSERT_NE(destination2, destination3);

    ASSERT_EQ(1, destination3->lastPublishedMetricList.size());
    ASSERT_EQ(destination3->lastPublishedMetricList[0].name.compare("TestId"), 0);
    ASSERT_EQ(destination3->lastPublishedMetricList[0].value, 45);
}

TEST_F(AplMetricsExtension2Test, TestCommandUnRegisteredActivity) {
    registerExtension(createActivityDescriptor());

    auto command = Command("1.0").uri(URI_V2).name("IncrementCounter").property(AMOUNT, 1);
    auto invoke = mExtension->invokeCommand(createActivityDescriptor(), command);
    ASSERT_TRUE(invoke);
    AssertLastPublishMetric(true, false);
}

TEST_F(AplMetricsExtension2Test, TestEmptyCommandName) {
    registerExtension(createActivityDescriptor());

    auto command = Command("1.0").uri(URI_V2).name("");
    auto invoke = mExtension->invokeCommand(createActivityDescriptor(), command);
    ASSERT_FALSE(invoke);
}

TEST_F(AplMetricsExtension2Test, TestCreateCounterEmptyMetricName) {
    auto activity = createActivityDescriptor();
    auto reg = registerExtension(activity);

    auto command = Command("1.0")
                       .uri(URI_V2)
                       .name("CreateCounter")
                       .property(METRIC_ID, "TestId")
                       .property("metricName", "")
                       .property("initialValue", 101);
    auto invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    mExtension->onActivityUnregistered(activity);
    auto metricList = mDestFactory->lastDestinationMock->lastPublishedMetricList;
    ASSERT_EQ(metricList.size(), 1);
    ASSERT_EQ(101, metricList[0].value);
}

TEST_F(AplMetricsExtension2Test, TestRecordMetricWithNoRegisteredActivity) {
    shouldCreateDestinationBeCalled = false;
    auto activity = createActivityDescriptor();

    auto command = Command("1.0")
                       .uri(URI_V2)
                       .name("RecordValue")
                       .property("metricName", "valueName")
                       .property("value", 563);
    auto invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    AssertLastPublishMetric(true, false);
}

TEST_F(AplMetricsExtension2Test, TestCreateCounterMetricWithNoRegisteredActivity) {
    shouldCreateDestinationBeCalled = false;
    auto activity = createActivityDescriptor();

    auto command = Command("1.0")
                       .uri(URI_V2)
                       .name("CreateCounter")
                       .property(METRIC_ID, "TestId")
                       .property("metricName", "valueName")
                       .property("initialValue", 101);
    auto invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    AssertLastPublishMetric(true, false);
}

TEST_F(AplMetricsExtension2Test, TestStartTimerMetricWithNoRegisteredActivity) {
    shouldCreateDestinationBeCalled = false;
    auto activity = createActivityDescriptor();

    auto command = Command("1.0")
                       .uri(URI_V2)
                       .name("StartTimer")
                       .property(METRIC_ID, "TestId")
                       .property("metricName", "testName");
    auto invoke = mExtension->invokeCommand(activity, command);
    ASSERT_TRUE(invoke);
    AssertLastPublishMetric(true, false);
}

TEST_F(AplMetricsExtension2Test, TestOnActivityUnregisteredWithNoRegisteredActivity) {
    shouldCreateDestinationBeCalled = false;
    auto activity = createActivityDescriptor();
    mExtension->onActivityUnregistered(activity);
    AssertLastPublishMetric(true, false);
}

TEST_F(AplMetricsExtension2Test, TestCreateCounterDestroyedExecutor) {
    shouldCreateDestinationBeCalled = false;
    auto activity = createActivityDescriptor();
    registerExtension(activity);

    auto command = Command("1.0")
                       .uri(URI_V2)
                       .name("CreateCounter")
                       .property(METRIC_ID, "TestId")
                       .property("metricName", "valueName")
                       .property("initialValue", 101);

    std::shared_ptr<AplMetricsExtensionV2> extension;
    {
        auto executor = std::make_shared<MockExecutor>();
        extension = std::make_shared<AplMetricsExtensionV2>(mDestFactory, executor);
    }
    auto invoke = extension->invokeCommand(activity, command);
    ASSERT_FALSE(invoke);
    AssertLastPublishMetric(true, false);
}

TEST_F(AplMetricsExtension2Test, TestAlreadyRegisteredActivity) {
    shouldCreateDestinationBeCalled = false;
    auto activity = createActivityDescriptor();
    registerExtension(activity);
    registerExtension(activity);

    AssertLastPublishMetric(true, false);
}

TEST_F(AplMetricsExtension2Test, TestNoDestinationType) {
    shouldCreateDestinationBeCalled = false;
    auto activity = createActivityDescriptor();
    registerExtension(activity, Dimensions(), true, true, false);

    AssertLastPublishMetric(false, false);
}