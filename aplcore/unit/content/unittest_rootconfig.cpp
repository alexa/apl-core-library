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

#include "gtest/gtest.h"

#include "apl/apl.h"

using namespace apl;

TEST(RootConfigTest, CustomEnvironmentProperties)
{
    RootConfig rootConfig;

    ASSERT_TRUE(rootConfig.getEnvironmentValues().empty());

    rootConfig.setEnvironmentValue("number", 42);
    ASSERT_EQ(42, rootConfig.getEnvironmentValues().at("number").asInt());

    rootConfig.setEnvironmentValue("string", "all your base");
    ASSERT_EQ("all your base", rootConfig.getEnvironmentValues().at("string").asString());

    // Wrong env property.
    rootConfig.setEnvironmentValue("environment", "oops");
    ASSERT_TRUE(rootConfig.getEnvironmentValues().find("environment") == rootConfig.getEnvironmentValues().end());
}

TEST(RootConfigTest, ApplyConfigurationChange)
{
    RootConfig rootConfig;
    ASSERT_FALSE(rootConfig.getProperty(RootProperty::kDisallowVideo).asBoolean());

    ConfigurationChange configurationChange;
    configurationChange.environmentValue("number", 42);
    configurationChange.disallowVideo(true);

    configurationChange.applyToRootConfig(rootConfig);

    ASSERT_EQ(42, rootConfig.getEnvironmentValues().at("number").asInt());
    ASSERT_TRUE(rootConfig.getProperty(RootProperty::kDisallowVideo).asBoolean());
}

TEST(RootConfigTest, CannotShadowExistingNames)
{
    RootConfig rootConfig;

    rootConfig.setEnvironmentValue("rotated", true)         // synthesized ConfigurationChange property
              .setEnvironmentValue("environment", {})     // top-level name
              .setEnvironmentValue("viewport", {})        // top-level name
              .setEnvironmentValue("agentName", "tests")  // part of default env
              .setEnvironmentValue("width", 42)           // part of default viewport
              .setEnvironmentValue("height", 42)          // part of default viewport
              .setEnvironmentValue("theme", "night");     // part of default viewport

    // Check that all invalid names have been rejected, so the environment still appears empty
    ASSERT_TRUE(rootConfig.getEnvironmentValues().empty());
}

TEST(RootConfigTest, RootPropertyBimapFullySynced)
{
    for (int rootProperty = RootProperty::kRootPropertySetBegin + 1; rootProperty != RootProperty::kRootPropertySetEnd; rootProperty++) {
        auto it = sRootPropertyBimap.find(static_cast<RootProperty>(rootProperty));
        if (it == sRootPropertyBimap.end()) {
            LOG(apl::LogLevel::kError) << "Key " << static_cast<RootProperty>(rootProperty) << " has not been assigned a name";
        }
        ASSERT_TRUE(it != sRootPropertyBimap.end());
    }
}