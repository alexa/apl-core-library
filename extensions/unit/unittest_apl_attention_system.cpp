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
#include "alexaext/extensionmessage.h"
#include "alexaext/APLAttentionSystemExtension/AplAttentionSystemExtension.h"

#include "gtest/gtest.h"

using namespace alexaext;
using namespace alexaext::attention;
using namespace rapidjson;


// Inject the UUID generator so we can reproduce tests
static int uuidValue = 0;

class TestAttentionSystemExtension : public AplAttentionSystemExtension {
public:
    explicit TestAttentionSystemExtension()
            : AplAttentionSystemExtension(Executor::getSynchronousExecutor(), testUuid)
    {}

    void updateLiveData(const ActivityDescriptor& activity) {
        AplAttentionSystemExtension::publishLiveData(activity);
    }

    static std::string testUuid() {
        return "AplAttentionSystemUuid-" + std::to_string(uuidValue);
    }
};

class AplAttentionSystemExtensionTest : public ::testing::Test {
public:

    void SetUp() override
    {
        mExtension = std::make_shared<TestAttentionSystemExtension>();
    }

    static
    std::shared_ptr<ActivityDescriptor> createActivityDescriptor(std::string uri = "aplext:attentionsystem:10") {
        auto session = SessionDescriptor::create();
        return ActivityDescriptor::create(uri, session, uri);
    }

    /**
     * Simple registration for testing event/command/data.
     */
    ::testing::AssertionResult registerExtension(std::shared_ptr<ActivityDescriptor> activity = createActivityDescriptor())
    {
        Document settings(kObjectType);
        settings.AddMember("attentionSystemStateName", Value("MyAttentionState"), settings.GetAllocator());

        Document regReq = RegistrationRequest("1.0").uri("aplext:attentionsystem:10").settings(settings);

        auto registration = mExtension->createRegistration(*activity, regReq);
        auto method = GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, "Fail");
        if (std::strcmp("RegisterSuccess", method) != 0)
            return testing::AssertionFailure() << "Failed Registration:" << method;
        mClientToken = GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, "");
        if (mClientToken.length() == 0)
            return testing::AssertionFailure() << "Failed Token:" << mClientToken;

        return ::testing::AssertionSuccess();
    }

    inline
    ::testing::AssertionResult CheckLiveData(const Value &update, const std::string &operation,
                                             const std::string &key)
    {
        using namespace testing;

        if (!update.IsObject())
            return AssertionFailure() << "Invalid json object" << update.GetType();

        std::string op = GetWithDefault<const char *>(LiveDataMapOperation::TYPE(), update, "");
        if (op != operation)
            return AssertionFailure() << "Invalid operation - expected:" << operation << " actual:" << op;

        std::string kk = GetWithDefault<const char *>(LiveDataMapOperation::KEY(), update, "");
        if (kk != key)
            return AssertionFailure() << "Invalid key - expected:" << key << " actual:" << kk;

        return AssertionSuccess();
    }

    ::testing::AssertionResult CheckLiveData(const Value &update, const std::string &operation,
                                             const std::string &key, const char *item)
    {
        using namespace testing;

        const auto &preCheck = CheckLiveData(update, operation, key);
        if (!preCheck)
            return preCheck;

        const rapidjson::Value *itm = LiveDataMapOperation::ITEM().Get(update);
        if (!itm || !itm->IsString())
            return AssertionFailure() << "Invalid item type";

        // string compare
        const char *value = itm->GetString();
        if (std::strcmp(value, item) != 0)
            return AssertionFailure() << "Invalid item - expected:" << item << " actual:" << value;

        return AssertionSuccess();
    }

    template<typename T>
    ::testing::AssertionResult CheckLiveData(const Value &update, const std::string &operation,
                                             const std::string &key, T item)
    {

        using namespace testing;

        const auto &preCheck = CheckLiveData(update, operation, key);
        if (!preCheck)
            return preCheck;

        const rapidjson::Value *itm = LiveDataMapOperation::ITEM().Get(update);
        if (!itm || itm->IsNull() || !itm->Is<T>())
            return AssertionFailure() << "Invalid item type";

        T value = itm->Get<T>();
        if (item != value)
            return AssertionFailure() << "Invalid item - extected:" << item << " actual:" << value;

        return AssertionSuccess();
    }

    Value* findDataType (Value *types, const std::string& typeName) {
        for (auto& v : types->GetArray()) {
            auto name = GetWithDefault<const char *>(TypePropertySchema::NAME(), v, "");
            if (typeName.compare(name) == 0) {
                return &v;
            }
        }
        return nullptr;
    }

    inline
    ::testing::AssertionResult IsEqual(const Value &lhs, const Value &rhs)
    {

        if (lhs != rhs) {
            return ::testing::AssertionFailure() << "Documents not equal\n"
                                                 << "lhs:\n" << AsPrettyString(lhs)
                                                 << "\nrhs:\n" << AsPrettyString(rhs) << "\n";
        }
        return ::testing::AssertionSuccess();
    }

    std::shared_ptr<TestAttentionSystemExtension> mExtension;
    std::string mClientToken;
};

/**
 * Simple create test for sanity.
 */
TEST_F(AplAttentionSystemExtensionTest, CreateExtension)
{
    ASSERT_TRUE(mExtension);
    auto supported = mExtension->getURIs();
    ASSERT_EQ(1, supported.size());
    ASSERT_NE(supported.end(), supported.find("aplext:attentionsystem:10"));
}

/**
 * Registration request with bad URI.
 */
TEST_F(AplAttentionSystemExtensionTest, RegistrationURIBad)
{
    Document regReq = RegistrationRequest("aplext:attentionsystem:BAD");
    auto activity = createActivityDescriptor("aplext:attentionsystem:BAD");

    auto registration = mExtension->createRegistration(*activity, regReq);
    ASSERT_FALSE(registration.HasParseError());
    ASSERT_FALSE(registration.IsNull());
    ASSERT_STREQ("RegisterFailure",
                 GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, ""));
}

/**
 * Registration Success has required fields
 */
TEST_F(AplAttentionSystemExtensionTest, RegistrationSuccess)
{
    Document regReq = RegistrationRequest("1.0").uri("aplext:attentionsystem:10");
    auto activity = createActivityDescriptor();

    auto registration = mExtension->createRegistration(*activity, regReq);
    ASSERT_STREQ("RegisterSuccess",
                 GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, ""));
    ASSERT_STREQ("aplext:attentionsystem:10",
                 GetWithDefault<const char *>(RegistrationSuccess::URI(), registration, ""));
    auto schema = RegistrationSuccess::SCHEMA().Get(registration);
    ASSERT_TRUE(schema);
    ASSERT_STREQ("aplext:attentionsystem:10", GetWithDefault<const char *>("uri", *schema, ""));
}

/**
 * Environment registration has best practice of version
 */
TEST_F(AplAttentionSystemExtensionTest, RegistrationEnvironmentVersion)
{
    Document regReq = RegistrationRequest("1.0").uri("aplext:attentionsystem:10");
    auto activity = createActivityDescriptor();

    auto registration = mExtension->createRegistration(*activity, regReq);
    ASSERT_STREQ("RegisterSuccess",
                 GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, ""));
    Value *environment = RegistrationSuccess::ENVIRONMENT().Get(registration);
    ASSERT_TRUE(environment);
    ASSERT_STREQ("APLAttentionSystemExtension-1.0",
                 GetWithDefault<const char *>(Environment::VERSION(), *environment, ""));
}

/**
 * Events are defined
 */
TEST_F(AplAttentionSystemExtensionTest, RegistrationEvents)
{
    Document regReq = RegistrationRequest("1.0").uri("aplext:attentionsystem:10");
    auto activity = createActivityDescriptor();

    auto registration = mExtension->createRegistration(*activity, regReq);
    ASSERT_STREQ("RegisterSuccess",
                 GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, ""));
    Value *schema = RegistrationSuccess::SCHEMA().Get(registration);
    ASSERT_TRUE(schema);

    Value *events = ExtensionSchema::EVENTS().Get(*schema);
    ASSERT_TRUE(events);

    auto expectedHandlerSet = std::set<std::string>();
    expectedHandlerSet.insert("OnAttentionStateChanged");
    ASSERT_TRUE(events->IsArray() && events->Size() == expectedHandlerSet.size());

    // should have all event handlers defined
    for (const Value &evt : events->GetArray()) {
        ASSERT_TRUE(evt.IsObject());
        auto name = GetWithDefault<const char *>(Event::NAME(), evt, "attentionState");
        ASSERT_TRUE(expectedHandlerSet.count(name) == 1);
        expectedHandlerSet.erase(name);
    }
    ASSERT_TRUE(expectedHandlerSet.empty());
}

/**
 * LiveData registration is not defined without settings.
 */
TEST_F(AplAttentionSystemExtensionTest, RegistrationSettingsEmpty)
{
    Document regReq = RegistrationRequest("1.0").uri("aplext:attentionsystem:10");
    auto activity = createActivityDescriptor();

    auto registration = mExtension->createRegistration(*activity, regReq);
    ASSERT_STREQ("RegisterSuccess",
                 GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, ""));
    Value *schema = RegistrationSuccess::SCHEMA().Get(registration);
    ASSERT_TRUE(schema);

    // no live data
    Value *liveData = ExtensionSchema::LIVE_DATA().Get(*schema);
    ASSERT_TRUE(liveData);
    ASSERT_TRUE(liveData->IsArray() && liveData->Empty());
}

/**
 * LiveData registration is defined with settings.
 */
TEST_F(AplAttentionSystemExtensionTest, RegistrationSettingsHasLiveData)
{
    Document settings(kObjectType);
    settings.AddMember("attentionSystemStateName", Value("MyAttentionState"), settings.GetAllocator());

    Document regReq = RegistrationRequest("1.0").uri("aplext:attentionsystem:10").settings(settings);
    auto activity = createActivityDescriptor();

    auto registration = mExtension->createRegistration(*activity, regReq);

    ASSERT_STREQ("RegisterSuccess",
                 GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, ""));
    Value *schema = RegistrationSuccess::SCHEMA().Get(registration);
    ASSERT_TRUE(schema);

    // live data defined
    Value *liveData = ExtensionSchema::LIVE_DATA().Get(*schema);
    ASSERT_TRUE(liveData);
    ASSERT_TRUE(liveData->IsArray() && liveData->Size() == 1);

    const Value& data = (*liveData)[0];
    ASSERT_TRUE(data.IsObject());
    auto name = GetWithDefault<const char *>(LiveDataSchema::NAME(), data, "");
    ASSERT_STREQ("MyAttentionState", name);
    auto type = GetWithDefault<const char *>(LiveDataSchema::DATA_TYPE(), data, "");
    ASSERT_STREQ("AttentionSystemState", type);

    Value *types = ExtensionSchema::TYPES().Get(*schema);
    ASSERT_TRUE(types);
    ASSERT_TRUE(liveData->IsArray());

    Value *stateType = findDataType(types, "AttentionSystemState");
    ASSERT_NE(nullptr, stateType);
    ASSERT_TRUE(stateType->IsObject());

    rapidjson::Document expected;
    expected.Parse(R"(
        {
            "name": "AttentionSystemState",
            "properties": {
                "attentionState": "string"
            }
        }
    )");
    ASSERT_FALSE(expected.HasParseError());
    ASSERT_TRUE(IsEqual(expected, *stateType));

}

/**
* Invalid settings on registration are handled and defaults are used.
**/
TEST_F(AplAttentionSystemExtensionTest, RegistrationSettingsBad)
{
    Document regReq = RegistrationRequest("1.0").uri("aplext:attentionsystem:10").settings(Value());
    auto activity = createActivityDescriptor();

    auto registration = mExtension->createRegistration(*activity, regReq);
    ASSERT_STREQ("RegisterSuccess",
                 GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration, ""));
    Value *schema = RegistrationSuccess::SCHEMA().Get(registration);
    ASSERT_TRUE(schema);
    // live data available
    Value *liveData = ExtensionSchema::LIVE_DATA().Get(*schema);
    ASSERT_TRUE(liveData);
    ASSERT_TRUE(liveData->IsArray() && liveData->Empty());
}

/**
 * LiveData is published when settings assigned.
 */
TEST_F(AplAttentionSystemExtensionTest, GetLiveDataObjectsSuccess)
{
    auto activity = createActivityDescriptor();
    ASSERT_TRUE(registerExtension(activity));

    bool gotUpdate = false;
    mExtension->registerLiveDataUpdateCallback(
            [&](const ActivityDescriptor& activity, const rapidjson::Value &liveDataUpdate) {
                gotUpdate = true;
                ASSERT_STREQ("LiveDataUpdate",
                             GetWithDefault<const char *>(RegistrationSuccess::METHOD(), liveDataUpdate, ""));
                const Value *ops = LiveDataUpdate::OPERATIONS().Get(liveDataUpdate);
                ASSERT_TRUE(ops);
                ASSERT_TRUE(ops->IsArray() && ops->Size() == 1);
                ASSERT_TRUE(CheckLiveData(ops->GetArray()[0], "Set", "attentionState", "IDLE"));
            });

    mExtension->updateLiveData(*activity);
    ASSERT_TRUE(gotUpdate);
}

/**
 * Attention state change updates live data.
 */
TEST_F(AplAttentionSystemExtensionTest, UpdateAttentionState)
{
    auto activity = createActivityDescriptor();
    ASSERT_TRUE(registerExtension(activity));

    bool gotUpdate = false;
    mExtension->registerLiveDataUpdateCallback(
            [&](const ActivityDescriptor& activity, const rapidjson::Value &liveDataUpdate) {
                gotUpdate = true;
                ASSERT_STREQ("LiveDataUpdate",
                             GetWithDefault<const char *>(RegistrationSuccess::METHOD(), liveDataUpdate, ""));
                ASSERT_STREQ("aplext:attentionsystem:10",
                             GetWithDefault<const char *>(RegistrationSuccess::TARGET(), liveDataUpdate, ""));
                const Value *ops = LiveDataUpdate::OPERATIONS().Get(liveDataUpdate);
                ASSERT_TRUE(ops);
                ASSERT_TRUE(ops->IsArray() && ops->Size() == 1);
                ASSERT_TRUE(CheckLiveData(ops->GetArray()[0], "Set", "attentionState", "LISTENING"));
            });

    mExtension->updateAttentionState(AttentionState::LISTENING);
    ASSERT_TRUE(gotUpdate);
}

/**
 * Extension instance can handle multiple concurrent activities.
 */
TEST_F(AplAttentionSystemExtensionTest, MultipleActivitiesLiveData)
{
    // Set up two activities with different attentionSystemStateName values
    Document settings1(kObjectType);
    settings1.AddMember("attentionSystemStateName", Value("FirstAttentionState"), settings1.GetAllocator());
    Document settings2(kObjectType);
    settings2.AddMember("attentionSystemStateName", Value("SecondAttentionState"), settings2.GetAllocator());

    Document regReq1 = RegistrationRequest("1.0").uri("aplext:attentionsystem:10").settings(settings1);
    auto activity1 = createActivityDescriptor();

    Document regReq2 = RegistrationRequest("1.0").uri("aplext:attentionsystem:10").settings(settings2);
    auto activity2 = createActivityDescriptor();

    auto registration1 = mExtension->createRegistration(*activity1, regReq1);
    auto registration2 = mExtension->createRegistration(*activity2, regReq2);

    ASSERT_STREQ("RegisterSuccess", GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration1, ""));
    Value *schema1 = RegistrationSuccess::SCHEMA().Get(registration1);
    ASSERT_TRUE(schema1);

    ASSERT_STREQ("RegisterSuccess", GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registration2, ""));
    Value *schema2 = RegistrationSuccess::SCHEMA().Get(registration2);
    ASSERT_TRUE(schema2);

    // Check LiveData schema exists and matches the correct name
    Value *liveData1 = ExtensionSchema::LIVE_DATA().Get(*schema1);
    const Value& data1 = (*liveData1)[0];
    ASSERT_TRUE(data1.IsObject());

    auto name1 = GetWithDefault<const char *>(LiveDataSchema::NAME(), data1, "");
    ASSERT_STREQ("FirstAttentionState", name1);

    Value *liveData2 = ExtensionSchema::LIVE_DATA().Get(*schema2);
    const Value& data2 = (*liveData2)[0];
    ASSERT_TRUE(data2.IsObject());

    auto name2 = GetWithDefault<const char *>(LiveDataSchema::NAME(), data2, "");
    ASSERT_STREQ("SecondAttentionState", name2);

    // Update attention state and see both are updated
    bool gotUpdate1 = false;
    bool gotUpdate2 = false;
    mExtension->registerLiveDataUpdateCallback(
            [&](const ActivityDescriptor& activity, const rapidjson::Value &liveDataUpdate) {
                if (activity == *activity1) {
                    gotUpdate1 = true;
                } else if (activity == *activity2) {
                    gotUpdate2 = true;
                }

                ASSERT_STREQ("LiveDataUpdate",
                    GetWithDefault<const char *>(RegistrationSuccess::METHOD(), liveDataUpdate, ""));
                ASSERT_STREQ("aplext:attentionsystem:10",
                    GetWithDefault<const char *>(RegistrationSuccess::TARGET(), liveDataUpdate, ""));
                const Value *ops = LiveDataUpdate::OPERATIONS().Get(liveDataUpdate);
                ASSERT_TRUE(ops);
                ASSERT_TRUE(ops->IsArray() && ops->Size() == 1);
                ASSERT_TRUE(CheckLiveData(ops->GetArray()[0], "Set", "attentionState", "THINKING"));
        });

    mExtension->updateAttentionState(AttentionState::THINKING);
    ASSERT_TRUE(gotUpdate1);
    ASSERT_TRUE(gotUpdate2);
}

/**
 * Once an activity is unregistered, ensure it does not get new updates.
 */
TEST_F(AplAttentionSystemExtensionTest, StateDoesNotUpdateAfterUnregister)
{
    auto activity = createActivityDescriptor();
    ASSERT_TRUE(registerExtension(activity));

    int numUpdates = 0;
    mExtension->registerLiveDataUpdateCallback(
        [&](const ActivityDescriptor& activity, const rapidjson::Value &liveDataUpdate) {
            numUpdates++;
            ASSERT_STREQ("LiveDataUpdate",
                         GetWithDefault<const char *>(RegistrationSuccess::METHOD(), liveDataUpdate, ""));
            ASSERT_STREQ("aplext:attentionsystem:10",
                         GetWithDefault<const char *>(RegistrationSuccess::TARGET(), liveDataUpdate, ""));
        });

    mExtension->updateAttentionState(AttentionState::LISTENING);
    mExtension->updateAttentionState(AttentionState::SPEAKING);
    ASSERT_EQ(numUpdates, 2);

    mExtension->onActivityUnregistered(*activity);
    mExtension->updateAttentionState(AttentionState::THINKING);

    // should not have updated again
    ASSERT_EQ(numUpdates, 2);
}
