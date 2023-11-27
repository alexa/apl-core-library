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
#include <unordered_map>

#include <gtest/gtest.h>

#include "alexaext/APLAudioNormalizationExtension/AplAudioNormalizationExtension.h"

namespace alexaext {
namespace audionormalization {

static auto sListenerCreateCount = 0;
static auto sListenerDestroyCount = 0;

class TestListener : public Listener {
public:
    TestListener() { sListenerCreateCount++; }

    TestListener(const TestListener& listener) { sListenerCreateCount++; }

    ~TestListener() override { sListenerDestroyCount++; }

    void onAudioNormalizationEnabled(const ActivityDescriptor& activity) override {
        auto it = mState.find(activity);
        if (it != mState.end()) {
            it->second = true;
        } else {
            mState.emplace(activity, true);
        }
    };

    void onAudioNormalizationDisabled(const ActivityDescriptor& activity) override {
        auto it = mState.find(activity);
        if (it != mState.end()) {
            it->second = false;
        } else {
            mState.emplace(activity, false);
        }
    }

public:
    std::unordered_map<ActivityDescriptor, bool, ActivityDescriptor::Hash> mState;
};


class AplAudioNormalizationTest : public ::testing::Test {
public:
    void TearDown() override {
        testListener.reset();
        ASSERT_EQ(sListenerCreateCount, sListenerDestroyCount);
    }

public:
    ActivityDescriptorPtr createActivity() {
        return ActivityDescriptor::create(AplAudioNormalizationExtension::URI, SessionDescriptor::create());
    }

    std::shared_ptr<AplAudioNormalizationExtension> extension = AplAudioNormalizationExtension::getInstance();

    std::shared_ptr<TestListener> testListener = std::make_shared<TestListener>();

    ActivityDescriptorPtr activity = createActivity();

    Command enable() { return Command("1.0").name("Enable"); }

    Command disable() { return Command("1.0").name("Disable"); }
};

TEST_F(AplAudioNormalizationTest, TestRegister)
{
    auto registerSuccess = extension->createRegistration(*createActivity(), RegistrationRequest("1.0").uri(AplAudioNormalizationExtension::URI));
    ASSERT_STREQ("RegisterSuccess", GetWithDefault<const char *>(RegistrationSuccess::METHOD(), registerSuccess, ""));
    ASSERT_STREQ(AplAudioNormalizationExtension::URI, GetWithDefault<const char*>(RegistrationSuccess::URI(), registerSuccess, ""));

    rapidjson::Value *schema = RegistrationSuccess::SCHEMA().Get(registerSuccess);
    ASSERT_TRUE(schema);

    rapidjson::Value *commands = ExtensionSchema::COMMANDS().Get(*schema);
    ASSERT_TRUE(commands);

    auto expectedCommandSet = std::set<std::string>{ "Enable", "Disable" };
    ASSERT_TRUE(commands->IsArray() && commands->Size() == expectedCommandSet.size());
    for (const rapidjson::Value &com : commands->GetArray()) {
        ASSERT_TRUE(com.IsObject());
        auto name = GetWithDefault<const char *>(Command::NAME(), com, "MissingName");
        ASSERT_TRUE(expectedCommandSet.count(name) == 1) << "Unknown Command:" << name;
        expectedCommandSet.erase(name);
    }
    ASSERT_TRUE(expectedCommandSet.empty());
}

TEST_F(AplAudioNormalizationTest, TestCommands)
{
    extension->registerListener(testListener);

    extension->invokeCommand(*activity, enable());
    ASSERT_TRUE(testListener->mState.at(*activity));

    extension->invokeCommand(*activity, disable());
    ASSERT_FALSE(testListener->mState.at(*activity));

    extension->unregisterListener(testListener);
}

TEST_F(AplAudioNormalizationTest, TestUnregisteredListenerNotUpdated)
{
    extension->registerListener(testListener);

    extension->invokeCommand(*activity, enable());
    ASSERT_TRUE(testListener->mState.at(*activity));

    extension->unregisterListener(testListener);

    extension->invokeCommand(*activity, disable());
    ASSERT_TRUE(testListener->mState.at(*activity));
}

TEST_F(AplAudioNormalizationTest, TestMultipleListeners)
{
    auto listener2 = std::make_shared<TestListener>();
    extension->registerListener(testListener);
    extension->registerListener(listener2);
    
    extension->invokeCommand(*activity, enable());
    ASSERT_TRUE(testListener->mState.at(*activity));
    ASSERT_TRUE(listener2->mState.at(*activity));

    extension->invokeCommand(*activity, disable());
    ASSERT_FALSE(testListener->mState.at(*activity));
    ASSERT_FALSE(listener2->mState.at(*activity));
}

TEST_F(AplAudioNormalizationTest, MultipleListenersUnregister)
{
    auto listener2 = std::make_shared<TestListener>();
    extension->registerListener(testListener);
    extension->registerListener(listener2);

    extension->invokeCommand(*activity, enable());
    ASSERT_TRUE(testListener->mState.at(*activity));
    ASSERT_TRUE(listener2->mState.at(*activity));

    extension->unregisterListener(listener2);

    extension->invokeCommand(*activity, disable());
    ASSERT_FALSE(testListener->mState.at(*activity));
    ASSERT_TRUE(listener2->mState.at(*activity));
}

TEST_F(AplAudioNormalizationTest, TestMultipleListenersMultipleActivities)
{
    auto activity2 = createActivity();
    auto listener2 = std::make_shared<TestListener>();

    extension->registerListener(testListener);
    extension->registerListener(listener2);

    extension->invokeCommand(*activity2, enable());
    ASSERT_TRUE(testListener->mState.at(*activity2));
    ASSERT_TRUE(listener2->mState.at(*activity2));

    extension->invokeCommand(*activity2, disable());
    ASSERT_FALSE(testListener->mState.at(*activity2));
    ASSERT_FALSE(listener2->mState.at(*activity2));

    extension->invokeCommand(*activity, enable());
    ASSERT_TRUE(testListener->mState.at(*activity));
    ASSERT_TRUE(listener2->mState.at(*activity));
}

TEST_F(AplAudioNormalizationTest, NullListenerDoesntCrash)
{
    extension->registerListener(testListener);

    testListener.reset();

    extension->invokeCommand(*activity, enable());
}

TEST_F(AplAudioNormalizationTest, NullListenersRemovedOnSessionEnded)
{
    extension->registerListener(testListener);

    testListener.reset();

    extension->onSessionEnded(*activity->getSession());
}

TEST_F(AplAudioNormalizationTest, NeverRegisteredListenerDoesntThrow)
{
    extension->unregisterListener(testListener);
}

TEST_F(AplAudioNormalizationTest, NullListenerNotRegistered)
{
    extension->registerListener(nullptr);
    extension->registerListener(testListener);

    extension->invokeCommand(*activity, enable());
    ASSERT_TRUE(testListener->mState.at(*activity));
}

} // namespace audionormalization
} // namespace alexaext