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

namespace {

static const char *URI = "test:extension:1.0";

class LocalExtension final : public ExtensionBase {
public:
    explicit LocalExtension() : ExtensionBase(URI) {};

    bool invokeCommand(const std::string& uri, const rapidjson::Value& command) override {
        return false;
    }

    rapidjson::Document createRegistration(const std::string& uri,
                                           const rapidjson::Value& registerRequest) override {
        const auto *settings = RegistrationRequest::SETTINGS().Get(registerRequest);
        bool shouldSucceed = GetWithDefault<bool>("succeed", settings, false);

        if (!shouldSucceed) {
            return RegistrationFailure::forInvalidMessage(uri);
        }

        return RegistrationSuccess("1.0")
                .uri(uri)
                .token("SessionToken1")
                .schema("1.0", [uri](ExtensionSchema schema) { schema.uri(uri); });
    }

    void onRegistered(const std::string& uri, const std::string& token) override {
        registered = true;
    }

    bool registered = false;
};

}

class LocalExtensionTest : public ::testing::Test {
public:
    void SetUp() override {
        extension = std::make_shared<LocalExtension>();
        proxy = std::make_shared<LocalExtensionProxy>(extension);
    }

    std::shared_ptr<LocalExtension> extension;
    LocalExtensionProxyPtr proxy;
};

TEST_F(LocalExtensionTest, SuccessfulRegistration) {
    Document settings;
    settings.Parse(R"(
        {
            "succeed": true
        }
    )");

    auto req = RegistrationRequest("1.0")
        .uri("aplext:foo:10")
        .settings(settings);

    ASSERT_TRUE(proxy->initializeExtension(URI));
    bool successCallbackWasCalled = false;
    bool registered = proxy->getRegistration(URI, req,
            [&](const std::string &uri, const rapidjson::Value &response) {
                successCallbackWasCalled = true;
            },
            [](const std::string &uri, const rapidjson::Value &error) {
                FAIL();
            });
    proxy->onRegistered(URI, "<token>");
    ASSERT_TRUE(registered);
    ASSERT_TRUE(successCallbackWasCalled);
    ASSERT_TRUE(extension->registered);
}

TEST_F(LocalExtensionTest, FailedRegistration) {
    Document settings;
    settings.Parse(R"(
        {
            "succeed": false
        }
    )");

    auto req = RegistrationRequest("1.0")
        .uri("aplext:foo:10")
        .settings(settings);

    ASSERT_TRUE(proxy->initializeExtension(URI));
    bool errorCallbackWasCalled = false;
    bool registered = proxy->getRegistration(URI, req,
            [](const std::string &uri, const rapidjson::Value &response) {
                FAIL();
            },
            [&](const std::string &uri, const rapidjson::Value &error) {
                errorCallbackWasCalled = true;
            });
    ASSERT_TRUE(registered);
    ASSERT_TRUE(errorCallbackWasCalled);
    ASSERT_FALSE(extension->registered);
}