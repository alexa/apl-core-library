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

#include <iostream>
#include <memory>

#include "gtest/gtest.h"

#include "testeventloop.h"

#include "apl/primitives/dimension.h"
#include "apl/primitives/filter.h"
#include "apl/engine/context.h"
#include "apl/content/metrics.h"
#include "apl/content/jsondata.h"
#include "apl/engine/rootcontext.h"

using namespace apl;

TEST(FilterTest, Basic)
{
    auto context = Context::create(Metrics(), makeDefaultSession());

    JsonData json(R"({"type":"Blur", "radius": 10})");
    auto f = Filter::create(*context, json.get());

    ASSERT_TRUE(f.isFilter());
    ASSERT_EQ(kFilterTypeBlur, f.getFilter().getType());
    ASSERT_TRUE(IsEqual(Dimension(10), f.getFilter().getValue(kFilterPropertyRadius)));
}

TEST(FilterTest, BadFilter)
{
    auto context = Context::create(Metrics(), makeDefaultSession());

    JsonData json(R"({"type":"Blurry", "radius": 10})");
    auto f = Filter::create(*context, json.get());

    ASSERT_FALSE(f.isFilter());
    ASSERT_EQ(Object::NULL_OBJECT(), f);
}

static std::vector<std::pair<std::string, Dimension>> sBlurTestCases = {
    {R"({"type":"Blur", "radius": 6.5})",    Dimension(6.5)},
    {R"({"type":"Blur", "radius": "10vh"})", Dimension(100)},
    {R"({"type":"Blur", "radius": "10vw"})", Dimension(200)},
    {R"({"type":"Blur", "radius": 0})",      Dimension(0)},
    {R"({"type":"Blur"})",                   Dimension(0)},
    {R"({"type":"Blur", "radius": -1})",     Dimension(0)},  // Illegal radius
    {R"({"type":"Blur", "radius": "10%"})",  Dimension(0)},  // Illegal radius
    {R"({"type":"Blur", "radius": "auto"})", Dimension(0)},  // Illegal radius
};

TEST(FilterTest, BlurFilter)
{
    auto context = Context::create(Metrics().size(2000,1000), makeDefaultSession());

    for (auto& m : sBlurTestCases) {
        JsonData json(m.first);
        auto f = Filter::create(*context, json.get());
        ASSERT_TRUE(f.isFilter()) << m.first;
        ASSERT_EQ(kFilterTypeBlur, f.getFilter().getType()) << m.first;
        ASSERT_EQ(Object(m.second), f.getFilter().getValue(kFilterPropertyRadius)) << m.first;
    }
}

TEST(FilterTest, NoiseFilter)
{
    auto context = Context::create(Metrics().size(2000,1000), makeDefaultSession());

    std::vector<std::tuple<std::string, bool, NoiseFilterKind, double>> noiseTestCases;

    noiseTestCases.push_back(std::tuple<std::string, bool, NoiseFilterKind, double>(
        R"({"type":"Noise", "useColor": true})",
        true,  kFilterNoiseKindGaussian, 10));
    noiseTestCases.push_back(std::tuple<std::string, bool, NoiseFilterKind, double>(
        R"({"type":"Noise", "kind": "uniform"})",
        false, kFilterNoiseKindUniform, 10));
    noiseTestCases.push_back(std::tuple<std::string, bool, NoiseFilterKind, double>(
        R"({"type":"Noise", "useColor": false, "sigma": 6.5})",
        false, kFilterNoiseKindGaussian, 6.5));
    noiseTestCases.push_back(std::tuple<std::string, bool, NoiseFilterKind, double>(
        R"({"type":"Noise", "useColor": 0, "sigma": -1})",
        false, kFilterNoiseKindGaussian, 0));

    for (auto& m : noiseTestCases) {
        std::string raw = std::get<0>(m);
        JsonData json(raw);
        auto f = Filter::create(*context, json.get());
        ASSERT_TRUE(f.isFilter()) << raw;
        ASSERT_EQ(kFilterTypeNoise, f.getFilter().getType()) << raw;
        ASSERT_TRUE(IsEqual(std::get<1>(m), f.getFilter().getValue(kFilterPropertyUseColor))) << raw;
        ASSERT_TRUE(IsEqual(std::get<2>(m), f.getFilter().getValue(kFilterPropertyKind))) << raw;
        ASSERT_TRUE(IsEqual(std::get<3>(m), f.getFilter().getValue(kFilterPropertySigma))) << raw;
    }
}

TEST(FilterTest, ResourceSubstitution)
{
    auto context = Context::create(Metrics().size(2000,1000), makeDefaultSession());
    context->putConstant("@filterSize", Object(Dimension(10)));

    JsonData json(R"({"type": "Blur", "radius": "${@filterSize * 2}"})");
    auto f = Filter::create(*context, json.get());
    ASSERT_TRUE(f.isFilter());
    ASSERT_EQ(Object(Dimension(20)), f.getFilter().getValue(kFilterPropertyRadius));
}

static const char *COMPONENT_FILTER =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Image\","
    "      \"filters\": ["
    "        {"
    "          \"type\": \"Blur\","
    "          \"radius\": 20"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

class FilterTestDocument : public DocumentWrapper {};

TEST_F(FilterTestDocument, InComponent)
{
    loadDocument(COMPONENT_FILTER);

    auto filters = component->getCalculated(kPropertyFilters);
    ASSERT_EQ(1, filters.size());
    ASSERT_EQ(kFilterTypeBlur, filters.at(0).getFilter().getType());
    ASSERT_EQ(Object(Dimension(20)), filters.at(0).getFilter().getValue(kFilterPropertyRadius));
}

static const char *COMPONENT_MIXED_FILTERS =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Image\","
    "      \"filters\": ["
    "        {"
    "          \"type\": \"Noise\","
    "          \"useColor\": true"
    "        },"
    "        {"
    "          \"type\": \"Blurry\","
    "          \"radius\": 10"
    "        },"
    "        {"
    "          \"type\": \"Blur\","
    "          \"radius\": 10"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(FilterTestDocument, InComponentMixed)
{
    loadDocument(COMPONENT_MIXED_FILTERS);

    auto filters = component->getCalculated(kPropertyFilters);
    ASSERT_EQ(2, filters.size());

    ASSERT_EQ(kFilterTypeNoise, filters.at(0).getFilter().getType());
    ASSERT_TRUE(IsEqual(true, filters.at(0).getFilter().getValue(kFilterPropertyUseColor)));
    ASSERT_TRUE(IsEqual(kFilterNoiseKindGaussian, filters.at(0).getFilter().getValue(kFilterPropertyKind)));
    ASSERT_TRUE(IsEqual(10, filters.at(0).getFilter().getValue(kFilterPropertySigma)));

    ASSERT_EQ(kFilterTypeBlur, filters.at(1).getFilter().getType());
    ASSERT_TRUE(IsEqual(Dimension(10), filters.at(1).getFilter().getValue(kFilterPropertyRadius)));

    ASSERT_TRUE(ConsoleMessage()); // The Blurry filter should have generated a console message
}
