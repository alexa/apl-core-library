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

#include "rapidjson/document.h"
#include "gtest/gtest.h"

#include "apl/apl.h"

using namespace apl;

class MyTextMeasure : public TextMeasurement {
public:
    YGSize measure(TextComponent *component, float width, YGMeasureMode widthMode, float height,
                   YGMeasureMode heightMode) override {
        return YGSize({.width=120.0, .height=60.0});
    }

    float baseline(TextComponent *component, float width, float height) override {
        return 0;
    }
};

using Packages = std::map<std::string, std::string>;

class PerftestBase : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

private:
    RootContextPtr loadInternal(const std::string& document, const std::string& data);
    std::string loadFile(const std::string& file);

    std::shared_ptr<Packages> packages;
    std::shared_ptr<Telemetry> telemetry;

protected:
    RootContextPtr load(const std::string& document);
    std::shared_ptr<Telemetry> getTelemetry();
    double extractCounter(const std::string& doc);
};
