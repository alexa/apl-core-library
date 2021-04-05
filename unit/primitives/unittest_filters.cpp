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

#include <memory>

#include "gtest/gtest.h"

#include "../testeventloop.h"

#include "apl/primitives/dimension.h"
#include "apl/primitives/filter.h"
#include "apl/engine/context.h"
#include "apl/content/metrics.h"
#include "apl/content/jsondata.h"
#include "apl/engine/rootcontext.h"

using namespace apl;

TEST(FilterTest, Basic)
{
    auto context = Context::createTestContext(Metrics(), makeDefaultSession());

    JsonData json(R"({"type":"Blur", "radius": 10})");
    auto f = Filter::create(*context, json.get());

    ASSERT_TRUE(f.isFilter());
    ASSERT_EQ(kFilterTypeBlur, f.getFilter().getType());
    ASSERT_TRUE(IsEqual(Dimension(10), f.getFilter().getValue(kFilterPropertyRadius)));
    ASSERT_TRUE(IsEqual(-1, f.getFilter().getValue(kFilterPropertySource)));
}

TEST(FilterTest, BadFilter)
{
    auto context = Context::createTestContext(Metrics(), makeDefaultSession());

    JsonData json(R"({"type":"Blurry", "radius": 10})");
    auto f = Filter::create(*context, json.get());

    ASSERT_FALSE(f.isFilter());
    ASSERT_EQ(Object::NULL_OBJECT(), f);
}

TEST(FilterTest, Equality)
{
    auto context = Context::createTestContext(Metrics().size(2000,1000), makeDefaultSession());

    JsonData blend1(R"( {"type": "Blend", "mode": "multiply"} )");
    JsonData blend2(R"( {"type": "Blend"} )");

    ASSERT_EQ(Filter::create(*context, blend1.get()), Filter::create(*context, blend1.get()));
    ASSERT_NE(Filter::create(*context, blend1.get()), Filter::create(*context, blend2.get()));
}

namespace {
struct BlendFilterTest {
    std::string json;
    int destination;
    BlendMode mode;
    int source;
};

std::vector<BlendFilterTest> BLEND_TESTS = {
    {R"({"type":"Blend"})",                                                -2, kBlendModeNormal,  -1},
    {R"({"type":"Blend", "source":0, "destination":1, "mode":"overlay"})", 1,  kBlendModeOverlay, 0},
    {R"({"type":"Blend", "mode":"fuzzy"})",                                -2, kBlendModeNormal,  -1},
    {R"({"type":"Blend", "source":"fuzzy", "destination": "v"})",          0,  kBlendModeNormal,  0},
};
}

TEST(FilterTest, BlendFilter)
{
    auto context = Context::createTestContext(Metrics().size(2000,1000), makeDefaultSession());

    for (auto& m : BLEND_TESTS) {
        JsonData json(m.json);
        auto filterObject = Filter::create(*context, json.get());
        ASSERT_TRUE(filterObject.isFilter()) << m.json;
        const auto& filter = filterObject.getFilter();
        ASSERT_EQ(kFilterTypeBlend, filter.getType()) << m.json;
        ASSERT_TRUE(IsEqual(m.destination, filter.getValue(kFilterPropertyDestination))) << m.json;
        ASSERT_TRUE(IsEqual(m.mode, filter.getValue(kFilterPropertyMode))) << m.json;
        ASSERT_TRUE(IsEqual(m.source, filter.getValue(kFilterPropertySource))) << m.json;
    }
}

namespace {
struct BlurFilterTest {
    std::string json;
    Dimension radius;
    int source;
};

std::vector<BlurFilterTest> BLUR_TESTS = {
    {R"({"type":"Blur", "radius": 6.5, "source": 2})",      Dimension(6.5), 2},
    {R"({"type":"Blur", "radius": "10vh", "source": 0})",   Dimension(100), 0},
    {R"({"type":"Blur", "radius": "10vw"})",                Dimension(200), -1},
    {R"({"type":"Blur", "radius": 0})",                     Dimension(0),   -1},
    {R"({"type":"Blur"})",                                  Dimension(0),   -1},
    // Illegal values for radius and/or source
    {R"({"type":"Blur", "radius": -1})",                    Dimension(0),   -1},
    {R"({"type":"Blur", "radius": "10%"})",                 Dimension(0),   -1},
    {R"({"type":"Blur", "radius": "auto", "source": "b"})", Dimension(0),   0},
};
}

TEST(FilterTest, BlurFilter)
{
    auto context = Context::createTestContext(Metrics().size(2000,1000), makeDefaultSession());

    for (auto& m : BLUR_TESTS) {
        JsonData json(m.json);
        auto filterObject = Filter::create(*context, json.get());
        ASSERT_TRUE(filterObject.isFilter()) << m.json;
        const auto& filter = filterObject.getFilter();
        ASSERT_EQ(kFilterTypeBlur, filter.getType()) << m.json;
        ASSERT_TRUE(IsEqual(m.radius, filter.getValue(kFilterPropertyRadius))) << m.json;
        ASSERT_TRUE(IsEqual(m.source, filter.getValue(kFilterPropertySource))) << m.json;
    }
}

namespace {
struct ColorFilterTest {
    std::string json;
    Color color;
};

std::vector<ColorFilterTest> COLOR_TESTS = {
    {R"({"type":"Color", "color": "blue"})",  Color::BLUE},
    {R"({"type":"Color" })",                  Color::TRANSPARENT},
    {R"({"type":"Color", "color": [1,2,3]})", Color::TRANSPARENT},
};
}

TEST(FilterTest, ColorFilter) {
    auto context = Context::createTestContext(Metrics().size(2000,1000), makeDefaultSession());

    for (auto& m : COLOR_TESTS) {
        JsonData json(m.json);
        auto filterObject = Filter::create(*context, json.get());
        ASSERT_TRUE(filterObject.isFilter()) << m.json;
        const auto& filter = filterObject.getFilter();
        ASSERT_EQ(kFilterTypeColor, filter.getType()) << m.json;
        ASSERT_TRUE(IsEqual(m.color, filter.getValue(kFilterPropertyColor))) << m.json;
    }
}

namespace {
struct GradientFilterTest {
    std::string json;
    Gradient::GradientType type;
    std::vector<Color> colorRange;
    std::vector<double> inputRange;
};

std::vector<GradientFilterTest> GRADIENT_TESTS = {
    { // Minimal gradient
        R"({"type":"Gradient", "gradient": {"type": "linear", "colorRange":["blue", "red"]}})",
        Gradient::GradientType::LINEAR,
        std::vector<Color>{Color(Color::BLUE), Color(Color::RED)},
        std::vector<double>{0, 1},
    },
    { // Bad gradient - need to specify an actual gradient
        R"({"type": "Gradient"})",
        static_cast<Gradient::GradientType>(-1),  // Mark a bad gradient
        {},
        {}
    },
    {
        R"({"type":"Gradient", "gradient": {"type": "radial", "colorRange":["green", "red"]}})",
        Gradient::GradientType::RADIAL,
        std::vector<Color>{Color(Color::GREEN), Color(Color::RED)},
        std::vector<double>{0, 1},
    },
    {  // Invalid gradient - one that does not have a type
        R"({"type":"Gradient", "gradient": {"type": "odd", "colorRange":["green", "red"]}})",
        static_cast<Gradient::GradientType>(-1),  // Mark a bad gradient
        {},
        {}
    }
};
}

TEST(FilterTest, GradientFilter) {
    auto context = Context::createTestContext(Metrics().size(2000,1000), makeDefaultSession());

    for (auto& m : GRADIENT_TESTS) {
        JsonData json(m.json);
        auto filterObject = Filter::create(*context, json.get());
        ASSERT_TRUE(filterObject.isFilter()) << m.json;
        const auto& filter = filterObject.getFilter();
        ASSERT_EQ(kFilterTypeGradient, filter.getType()) << m.json;

        const auto& gradient = filter.getValue(kFilterPropertyGradient);
        if (m.type == static_cast<Gradient::GradientType>(-1)) {
            ASSERT_TRUE(gradient.isNull()) << m.json;
        } else {
            const auto& g = gradient.getGradient();
            ASSERT_EQ(m.type, g.getType()) << m.json;
            ASSERT_EQ(m.colorRange, g.getColorRange()) << m.json;
            ASSERT_EQ(m.inputRange, g.getInputRange()) << m.json;
        }
    }
}

namespace {
struct GrayscaleFilterTest {
    std::string json;
    double amount;
    int source;
};

std::vector<GrayscaleFilterTest> GRAYSCALE_TESTS = {
    {R"({"type":"Grayscale"})",                               0,  -1},
    {R"({"type":"Grayscale", "amount": 0.25, "source": -2})", 0.25, -2},
    {R"({"type":"Grayscale", "amount": 2.5, "source": 0})",   1.0,  0},
    {R"({"type":"Grayscale", "amount": -3, "source": 2.2})",  0,    2},
};
}

TEST(FilterTest, GrayscaleFilter)
{
    auto context = Context::createTestContext(Metrics().size(2000,1000), makeDefaultSession());

    for (auto& m : GRAYSCALE_TESTS) {
        JsonData json(m.json);
        auto f = Filter::create(*context, json.get());
        ASSERT_TRUE(f.isFilter()) << m.json;
        ASSERT_EQ(kFilterTypeGrayscale, f.getFilter().getType()) << m.json;
        ASSERT_TRUE(IsEqual(m.amount, f.getFilter().getValue(kFilterPropertyAmount))) << m.json;
        ASSERT_TRUE(IsEqual(m.source, f.getFilter().getValue(kFilterPropertySource))) << m.json;
    }
}


namespace {
struct NoiseFilterTest {
    std::string json;
    bool useColor;
    NoiseFilterKind kind;
    double sigma;
    int source;
};

std::vector<NoiseFilterTest> NOISE_TESTS = {
    {R"({"type":"Noise", "useColor": true})",                true,  kFilterNoiseKindGaussian, 10,  -1},
    {R"({"type":"Noise", "kind": "uniform", "source": 2})",  false, kFilterNoiseKindUniform,  10,  2},
    {R"({"type":"Noise", "useColor": false, "sigma": 6.5})", false, kFilterNoiseKindGaussian, 6.5, -1},
    {R"({"type":"Noise", "useColor": 0, "sigma": -1})",      false, kFilterNoiseKindGaussian, 0,   -1},
};
}

TEST(FilterTest, NoiseFilter)
{
    auto context = Context::createTestContext(Metrics().size(2000,1000), makeDefaultSession());

    for (auto& m : NOISE_TESTS) {
        JsonData json(m.json);
        auto f = Filter::create(*context, json.get());
        ASSERT_TRUE(f.isFilter()) << m.json;
        ASSERT_EQ(kFilterTypeNoise, f.getFilter().getType()) << m.json;
        ASSERT_TRUE(IsEqual(m.useColor, f.getFilter().getValue(kFilterPropertyUseColor))) << m.json;
        ASSERT_TRUE(IsEqual(m.kind, f.getFilter().getValue(kFilterPropertyKind))) << m.json;
        ASSERT_TRUE(IsEqual(m.sigma, f.getFilter().getValue(kFilterPropertySigma))) << m.json;
        ASSERT_TRUE(IsEqual(m.source, f.getFilter().getValue(kFilterPropertySource))) << m.json;
    }
}

namespace {
struct SaturateFilterTest {
    std::string json;
    double amount;
    int source;
};

std::vector<SaturateFilterTest> SATURATE_TESTS = {
    {R"({"type":"Saturate"})",                              1.0, -1},
    {R"({"type":"Saturate", "amount": 2.5, "source": 0})",  2.5, 0},
    {R"({"type":"Saturate", "amount": -3, "source": 2.2})", 0,   2},
};
}

TEST(FilterTest, SaturateFilter)
{
    auto context = Context::createTestContext(Metrics().size(2000,1000), makeDefaultSession());

    for (auto& m : SATURATE_TESTS) {
        JsonData json(m.json);
        auto f = Filter::create(*context, json.get());
        ASSERT_TRUE(f.isFilter()) << m.json;
        ASSERT_EQ(kFilterTypeSaturate, f.getFilter().getType()) << m.json;
        ASSERT_TRUE(IsEqual(m.amount, f.getFilter().getValue(kFilterPropertyAmount))) << m.json;
        ASSERT_TRUE(IsEqual(m.source, f.getFilter().getValue(kFilterPropertySource))) << m.json;
    }
}


TEST(FilterTest, ResourceSubstitution)
{
    auto context = Context::createTestContext(Metrics().size(2000,1000), makeDefaultSession());
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


static const char *EXTENSION_FILTER = R"apl(
    {
      "type": "APL",
      "version": "1.4",
      "extensions": {
        "name": "Canny",
        "uri": "aplext:CannyEdgeFilters:10"
      },
      "mainTemplate": {
        "items": {
          "type": "Image",
          "filters": {
            "type": "Canny:FindEdges",
            "min": 0.2,
            "max": 0.8
          }
        }
      }
    }
)apl";

/**
 * Test an extension that operates on a single image from the source array.
 * The kFilterPropertySource property will be generated with a default value of -1.
 */
TEST_F(FilterTestDocument, ExtensionWithSource)
{
    config->registerExtensionFilter(ExtensionFilterDefinition("aplext:CannyEdgeFilters:10",
                                                             "FindEdges",
                                                             ExtensionFilterDefinition::ONE)
                        .property("min", 0.1, kBindingTypeNumber)
                        .property("max", 0.9, kBindingTypeNumber));

    loadDocument(EXTENSION_FILTER);

    ASSERT_TRUE(component);
    auto filters = component->getCalculated(kPropertyFilters);
    ASSERT_TRUE(filters.isArray());
    ASSERT_EQ(1, filters.size());

    auto filterObject = filters.at(0);
    ASSERT_TRUE(filterObject.isFilter());

    const auto& filter = filterObject.getFilter();
    ASSERT_EQ(kFilterTypeExtension, filter.getType());
    ASSERT_TRUE(IsEqual("aplext:CannyEdgeFilters:10", filter.getValue(kFilterPropertyExtensionURI)));
    ASSERT_TRUE(IsEqual("FindEdges", filter.getValue(kFilterPropertyName)));
    ASSERT_TRUE(IsEqual(-1, filter.getValue(kFilterPropertySource)));
    ASSERT_TRUE(filter.getValue(kFilterPropertyDestination).isNull());
    auto bag = filter.getValue(kFilterPropertyExtension);
    ASSERT_TRUE(bag.isMap());
    ASSERT_TRUE(IsEqual(0.2, bag.get("min")));
    ASSERT_TRUE(IsEqual(0.8, bag.get("max")));
}

static const char *EXTENSION_TWO_IMAGES_FILTER = R"apl(
    {
      "type": "APL",
      "version": "1.4",
      "extensions": {
        "name": "Morph",
        "uri": "aplext:MorphingFilters:10"
      },
      "mainTemplate": {
        "items": {
          "type": "Image",
          "filters": {
            "type": "Morph:MergeTwo",
            "amount": 0.25,
            "source": 1
          }
        }
      }
    }
)apl";

/**
 * Test an extension that combines two images from the source array.
 * The kFilterPropertySource property will be generated with a default value of -1.
 * The kFilterPropertyDestination property will be generated with a default value of -2.
 */
TEST_F(FilterTestDocument, ExtensionWithSourceAndDestination)
{
    config->registerExtensionFilter(ExtensionFilterDefinition("aplext:MorphingFilters:10",
                                                             "MergeTwo",
                                                             ExtensionFilterDefinition::TWO)
                                       .property("amount", 0.5, kBindingTypeNumber));

    loadDocument(EXTENSION_TWO_IMAGES_FILTER);

    ASSERT_TRUE(component);
    auto filters = component->getCalculated(kPropertyFilters);
    ASSERT_TRUE(filters.isArray());
    ASSERT_EQ(1, filters.size());

    auto filterObject = filters.at(0);
    ASSERT_TRUE(filterObject.isFilter());

    const auto& filter = filterObject.getFilter();
    ASSERT_EQ(kFilterTypeExtension, filter.getType());
    ASSERT_TRUE(IsEqual("aplext:MorphingFilters:10", filter.getValue(kFilterPropertyExtensionURI)));
    ASSERT_TRUE(IsEqual("MergeTwo", filter.getValue(kFilterPropertyName)));
    ASSERT_TRUE(IsEqual(1, filter.getValue(kFilterPropertySource)));
    ASSERT_TRUE(IsEqual(-2, filter.getValue(kFilterPropertyDestination)));
    auto bag = filter.getValue(kFilterPropertyExtension);
    ASSERT_TRUE(bag.isMap());
    ASSERT_TRUE(IsEqual(0.25, bag.get("amount")));
}


static const char *EXTENSION_ZERO_IMAGES_FILTER = R"apl(
    {
      "type": "APL",
      "version": "1.4",
      "extensions": {
        "name": "Foo",
        "uri": "aplext:NoiseGeneration:10"
      },
      "mainTemplate": {
        "items": {
          "type": "Image",
          "filters": {
            "type": "Foo:Perlin",
            "width": 256,
            "height": 256,
            "cellSize": 12.2,
            "attenuation": 3.2,
            "color": true
          }
        }
      }
    }
)apl";

/**
 * This extension does not take any input images; it only generates an output image.
 */
TEST_F(FilterTestDocument, ExtensionNoInputImages)
{
    config->registerExtensionFilter(ExtensionFilterDefinition("aplext:NoiseGeneration:10",
                                                             "Perlin",
                                                             ExtensionFilterDefinition::ZERO)
                                       .property("width", 128, kBindingTypeInteger)
                                       .property("height", 128, kBindingTypeInteger)
                                       .property("cellSize", 8, kBindingTypeInteger)
                                       .property("attenuation", 0.4, kBindingTypeNumber)
                                       .property("color", false, kBindingTypeBoolean));

    loadDocument(EXTENSION_ZERO_IMAGES_FILTER);

    ASSERT_TRUE(component);
    auto filters = component->getCalculated(kPropertyFilters);
    ASSERT_TRUE(filters.isArray());
    ASSERT_EQ(1, filters.size());

    auto filterObject = filters.at(0);
    ASSERT_TRUE(filterObject.isFilter());

    const auto& filter = filterObject.getFilter();
    ASSERT_EQ(kFilterTypeExtension, filter.getType());
    ASSERT_TRUE(IsEqual("aplext:NoiseGeneration:10", filter.getValue(kFilterPropertyExtensionURI)));
    ASSERT_TRUE(IsEqual("Perlin", filter.getValue(kFilterPropertyName)));
    ASSERT_TRUE(filter.getValue(kFilterPropertySource).isNull());
    ASSERT_TRUE(filter.getValue(kFilterPropertyDestination).isNull());
    auto bag = filter.getValue(kFilterPropertyExtension);
    ASSERT_TRUE(bag.isMap());
    ASSERT_TRUE(IsEqual(256, bag.get("width")));
    ASSERT_TRUE(IsEqual(256, bag.get("height")));
    ASSERT_TRUE(IsEqual(12, bag.get("cellSize")));
    ASSERT_TRUE(IsEqual(3.2, bag.get("attenuation")));
    ASSERT_TRUE(IsEqual(true, bag.get("color")));
}


static const char *EXTENSION_EQUALITY = R"apl(
    {
      "type": "APL",
      "version": "1.4",
      "extensions": [
        { "name": "A", "uri": "TestURI" },
        { "name": "B", "uri": "OtherURI" }
      ],
      "mainTemplate": {
        "items": {
          "type": "Image",
          "filters": [
            { "type": "A:afilter" },
            { "type": "A:afilter", "a": 0 },
            { "type": "A:afilter", "a": 10 },
            { "type": "A:afilter", "source": 0 },
            { "type": "A:bfilter" },
            { "type": "B:afilter" }
          ]
        }
      }
    }
)apl";

// Extension filters have a slightly richer equality test
TEST_F(FilterTestDocument, ExtensionEquality)
{
    auto context = Context::createTestContext(Metrics().size(2000, 1000), makeDefaultSession());
    config->registerExtensionFilter(
        ExtensionFilterDefinition("TestURI", "afilter", ExtensionFilterDefinition::ONE)
            .property("a", 0)
    );
    config->registerExtensionFilter(
        ExtensionFilterDefinition("TestURI", "bfilter", ExtensionFilterDefinition::ONE)
            .property("a", 0)
    );
    config->registerExtensionFilter(
        ExtensionFilterDefinition("OtherURI", "afilter", ExtensionFilterDefinition::ONE)
            .property("a", 0)
    );

    loadDocument(EXTENSION_EQUALITY);
    ASSERT_TRUE(component);
    auto filters = component->getCalculated(kPropertyFilters);
    ASSERT_TRUE(filters.isArray());
    ASSERT_EQ(6, filters.size());

    ASSERT_TRUE( filters.at(0) == filters.at(1) );   // The first two filters are the same
    ASSERT_FALSE( filters.at(0) == filters.at(2) );  // Mismatched 'a'
    ASSERT_FALSE( filters.at(0) == filters.at(3) );  // Mismatched 'source'
    ASSERT_FALSE( filters.at(0) == filters.at(4) );  // Different filter name
    ASSERT_FALSE( filters.at(0) == filters.at(5) );  // Different filter URI
}


static const char *SERIALIZE_FILTERS = R"apl(
    {
      "type": "APL",
      "version": "1.4",
      "extensions": {
        "name": "Morph",
        "uri": "aplext:MorphingFilters:10"
      },
      "mainTemplate": {
        "items": {
          "type": "Image",
          "filters": [
            {
              "type": "Morph:MergeTwo",
              "amount": 0.25,
              "source": 1
            },
            {
              "type": "Noise",
              "kind": "uniform"
            }
          ]
        }
      }
    }
)apl";


// Verify that filters serialize correctly
TEST_F(FilterTestDocument, Serialize)
{
    config->registerExtensionFilter(ExtensionFilterDefinition("aplext:MorphingFilters:10",
                                                             "MergeTwo",
                                                             ExtensionFilterDefinition::TWO)
                                       .property("amount", 0.5, kBindingTypeNumber)
                                       .property("hue", Color(Color::BLUE), kBindingTypeColor));

    loadDocument(SERIALIZE_FILTERS);

    ASSERT_TRUE(component);
    auto filters = component->getCalculated(kPropertyFilters);
    ASSERT_TRUE(filters.isArray());
    ASSERT_EQ(2, filters.size());

    rapidjson::Document doc;
    auto json = filters.serialize(doc.GetAllocator());

    ASSERT_TRUE(json.IsArray());
    ASSERT_EQ(2, json.GetArray().Size());

    // Check the first filter - this is an extension filter with source and destination
    ASSERT_EQ(6, json[0].MemberCount());  // Six members: type, destination, source, extension, extensionURI, name
    ASSERT_EQ(kFilterTypeExtension, json[0]["type"].GetDouble());
    ASSERT_STREQ("MergeTwo", json[0]["name"].GetString());
    ASSERT_STREQ("aplext:MorphingFilters:10", json[0]["extensionURI"].GetString());
    ASSERT_EQ(1, json[0]["source"].GetDouble());
    ASSERT_EQ(-2, json[0]["destination"].GetDouble());
    ASSERT_EQ(0.25, json[0]["extension"]["amount"].GetDouble());
    ASSERT_STREQ("#0000ffff", json[0]["extension"]["hue"].GetString());

    // Check the second filter - this is a noise filter with type, kind, sigma, source, and useColor
    ASSERT_EQ(5, json[1].MemberCount());
    ASSERT_EQ(kFilterTypeNoise, json[1]["type"].GetDouble());
    ASSERT_EQ(-1, json[1]["source"].GetDouble());
    ASSERT_EQ(kFilterNoiseKindUniform, json[1]["kind"].GetDouble());
    ASSERT_EQ(10.0, json[1]["sigma"].GetDouble());
    ASSERT_FALSE(json[1]["useColor"].GetBool());
}
