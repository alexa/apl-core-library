/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "perftest_base.h"
#include "rapidjson/document.h"

using namespace apl;

class PerftestVisualContext : public PerftestBase {};

TEST_F(PerftestVisualContext, Basic) {
    auto telemetry = getTelemetry();

    for(int i = 0; i<100; i++) {
        auto root = load("basic");

        telemetry->startTime("visual_context");

        rapidjson::Document document(rapidjson::kObjectType);
        auto context = root->topComponent()->serializeVisualContext(document.GetAllocator());

        ASSERT_TRUE(context.HasMember("children"));
        ASSERT_TRUE(context.HasMember("tags"));
        ASSERT_FALSE(context.HasMember("id"));
        auto& tags = context["tags"];
        ASSERT_TRUE(tags.HasMember("viewport"));

        auto& children = context["children"];
        ASSERT_EQ(2, children.Size());
        ASSERT_EQ("touchWrapper", children[0]["id"]);
        ASSERT_EQ("sequence", children[1]["id"]);

        telemetry->endTime("visual_context");
    }

    EXPECT_GT(10, extractCounter("basic"));
    EXPECT_GT(1, extractCounter("visual_context"));
}