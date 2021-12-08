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

static const char* URI = "test:extension:1.0";
static const char* RESOURCE_ID = "SURFACE42";
static const int ERROR_CODE = -64;
static const char* ERROR = "error message";

class TestResourceProvider final: public ExtensionResourceProvider {
public:
    // Simple resource provider supporting a single resource for a single uri;
    bool requestResource(const std::string& uri, const std::string& resourceId,
                         ExtensionResourceSuccessCallback success,
                         ExtensionResourceFailureCallback error) override {
        // fail call if bad URI - artificial failure not representing real world use case
        if (uri.compare(URI) != 0)
            return false;

        if (resourceId.compare(RESOURCE_ID) != 0) {
            // error callback if bad resource ID
            error(uri, resourceId, ERROR_CODE, ERROR);
        }
        else {
            // success callback if resource supported
            auto resource = std::make_shared<ResourceHolder>(RESOURCE_ID);
            success(uri, resource);
        }
        return true;
    };
};

} // namespace

class ExtensionResourceProviderTest : public ::testing::Test {
public:
    void SetUp() override { provider = std::make_shared<TestResourceProvider>(); }

    ExtensionResourceProviderPtr provider;
};


/**
 * The resource request cannot be handled (for example an IPC error)
 */
TEST_F(ExtensionResourceProviderTest, RequestNotHandled) {
    auto requested = provider->requestResource(
        "potato", RESOURCE_ID,
        [&](const std::string& uri, const ResourceHolderPtr& resourceHolder) { FAIL(); },
        [](const std::string& uri, const std::string& resourceId, int errorCode,
           const std::string& error) { FAIL(); });
    ASSERT_FALSE(requested);
}

/**
 * The resource request was handled and a resource was successfully provided.
 */
TEST_F(ExtensionResourceProviderTest, RequestResourceSuccess) {

    bool successCalled = false;
    ResourceHolderPtr result;
    auto requested = provider->requestResource(
        URI, RESOURCE_ID,
        [&](const std::string& uri, const ResourceHolderPtr& resourceHolder) {
            successCalled = true;
            result = resourceHolder;
        },
        [](const std::string& uri, const std::string& resourceId, int errorCode,
           const std::string& error) { FAIL(); });

    ASSERT_TRUE(requested);
    ASSERT_TRUE(successCalled);
    ASSERT_EQ(RESOURCE_ID, result->resourceId());
}

/**
 * The resource request was handled and failed.
 */
TEST_F(ExtensionResourceProviderTest, RequestResourceFailure) {

    bool failureCalled = false;
    int code = 0;
    std::string message = "";

    ResourceHolderPtr result;
    auto requested = provider->requestResource(
        URI, "potato",
        [](const std::string& uri, const ResourceHolderPtr& resourceHolder) { FAIL(); },
        [&](const std::string& uri, const std::string& resourceId, int errorCode,
            const std::string& error) {
            failureCalled = true;
            code = errorCode;
            message = error;
        });

    ASSERT_TRUE(requested);
    ASSERT_TRUE(failureCalled);
    ASSERT_EQ(ERROR_CODE, code);
    ASSERT_EQ(ERROR, message);
}

