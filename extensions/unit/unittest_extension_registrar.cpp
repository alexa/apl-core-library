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
using namespace rapidjson;

/**
 * Test class;
 */
class ExtensionRegistrarTest : public ::testing::Test {
public:
    void SetUp() override {
        extRegistrar = std::make_shared<ExtensionRegistrar>();
    }

    void TearDown() override {
        extRegistrar = nullptr;
        ::testing::Test::TearDown();
    }

    ExtensionRegistrarPtr extRegistrar;
};


class TextExtensionProxy : public ExtensionProxy {
public:
    TextExtensionProxy(const std::string& uri) { mURIs.emplace(uri); }

    std::set<std::string> getURIs() const override { return mURIs; }

    bool initializeExtension(const std::string &uri) override { return true; }
    bool isInitialized(const std::string &uri) const override { return true; }
    bool getRegistration(const std::string &uri, const rapidjson::Value &registrationRequest,
                         RegistrationSuccessCallback success, RegistrationFailureCallback error) override { return false; }
    bool invokeCommand(const std::string &uri, const rapidjson::Value &command,
                       CommandSuccessCallback success, CommandFailureCallback error) override { return false; }
    bool sendMessage(const std::string &uri, const rapidjson::Value &message) override { return false; }
    void registerEventCallback(Extension::EventCallback callback) override {}
    void registerLiveDataUpdateCallback(Extension::LiveDataUpdateCallback callback) override {}
    void onRegistered(const std::string &uri, const std::string &token) override {}
    void onUnregistered(const std::string &uri, const std::string &token) override {}
    void onResourceReady( const std::string& uri, const ResourceHolderPtr& resourceHolder) override {}

private:
    std::set<std::string> mURIs;
};

/**
 * Nothing will happen really, for coverage only.
 */
TEST_F(ExtensionRegistrarTest, EmptyAdds) {
    extRegistrar->registerExtension(nullptr);
    extRegistrar->addProvider(nullptr);
}

TEST_F(ExtensionRegistrarTest, BasicLocallyRegisteredProxy) {
    auto test1 = std::make_shared<TextExtensionProxy>("test1");
    auto test2 = std::make_shared<TextExtensionProxy>("test2");

    extRegistrar->registerExtension(test1);
    extRegistrar->registerExtension(test2);

    ASSERT_TRUE(extRegistrar->hasExtension("test1"));
    ASSERT_TRUE(extRegistrar->hasExtension("test2"));
    ASSERT_FALSE(extRegistrar->hasExtension("test3"));

    ASSERT_EQ(test1, extRegistrar->getExtension("test1"));
    ASSERT_EQ(test2, extRegistrar->getExtension("test2"));
    ASSERT_EQ(nullptr, extRegistrar->getExtension("test3"));
}

class TestProvider : public ExtensionProvider {
public:
    TestProvider(const std::string& prefix) {
        mExtensions.emplace(prefix + "::test1");
        mExtensions.emplace(prefix + "::test2");
    }

    bool hasExtension(const std::string& uri) override {
        return mExtensions.count(uri);
    }

    ExtensionProxyPtr getExtension(const std::string& uri) override {
        if (mExtensions.count(uri) > 0) {
            return std::make_shared<TextExtensionProxy>(uri);
        }
        return nullptr;
    }

private:
    std::set<std::string> mExtensions;
};

TEST_F(ExtensionRegistrarTest, MultipleProviders) {
    auto test1 = std::make_shared<TextExtensionProxy>("test1");
    auto test2 = std::make_shared<TextExtensionProxy>("test2");

    auto tp1 = std::make_shared<TestProvider>("provider1");
    auto tp2 = std::make_shared<TestProvider>("provider2");

    extRegistrar->addProvider(tp1);
    extRegistrar->addProvider(tp2);
    extRegistrar->registerExtension(test1);
    extRegistrar->registerExtension(test2);

    ASSERT_TRUE(extRegistrar->hasExtension("test1"));
    ASSERT_TRUE(extRegistrar->hasExtension("test2"));
    ASSERT_FALSE(extRegistrar->hasExtension("test3"));

    ASSERT_TRUE(extRegistrar->hasExtension("provider1::test1"));
    ASSERT_TRUE(extRegistrar->hasExtension("provider1::test2"));
    ASSERT_FALSE(extRegistrar->hasExtension("provider1::test3"));

    ASSERT_TRUE(extRegistrar->hasExtension("provider2::test1"));
    ASSERT_TRUE(extRegistrar->hasExtension("provider2::test2"));
    ASSERT_FALSE(extRegistrar->hasExtension("provider2::test3"));

    ASSERT_EQ(test1, extRegistrar->getExtension("test1"));
    ASSERT_EQ(test2, extRegistrar->getExtension("test2"));
    ASSERT_EQ(nullptr, extRegistrar->getExtension("test3"));

    ASSERT_NE(nullptr, extRegistrar->getExtension("provider1::test1"));
    ASSERT_NE(nullptr, extRegistrar->getExtension("provider1::test2"));
    ASSERT_EQ(nullptr, extRegistrar->getExtension("provider1::test3"));

    ASSERT_NE(nullptr, extRegistrar->getExtension("provider2::test1"));
    ASSERT_NE(nullptr, extRegistrar->getExtension("provider2::test2"));
    ASSERT_EQ(nullptr, extRegistrar->getExtension("provider2::test3"));
}

TEST_F(ExtensionRegistrarTest, ReturnsSame) {
    auto test1 = std::make_shared<TextExtensionProxy>("test1");

    auto tp1 = std::make_shared<TestProvider>("provider1");

    extRegistrar->addProvider(tp1);
    extRegistrar->registerExtension(test1);

    ASSERT_TRUE(extRegistrar->hasExtension("test1"));
    ASSERT_TRUE(extRegistrar->hasExtension("provider1::test1"));
    ASSERT_EQ(test1, extRegistrar->getExtension("test1"));

    ASSERT_EQ(extRegistrar->getExtension("test1"), extRegistrar->getExtension("test1"));
    ASSERT_EQ(extRegistrar->getExtension("provider1::test1"), extRegistrar->getExtension("provider1::test1"));
}