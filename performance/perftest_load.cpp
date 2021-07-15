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

class PerftestLoad : public PerftestBase {};

TEST_F(PerftestLoad, DocumentLoad) {
    auto thresholds = std::map<std::string, double>();
    thresholds.emplace("hello-world", 1);
    thresholds.emplace("complex-list", 15);
    thresholds.emplace("fn", 10);
    thresholds.emplace("kayak", 15);

    for(int i = 0; i<100; i++) {
        for(auto& doct : thresholds) {
            load(doct.first);
        }
    }

    for(auto& doct : thresholds) {
        EXPECT_GT(doct.second, extractCounter(doct.first));
    }
}
