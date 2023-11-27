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

#include "../testeventloop.h"

using namespace apl;

class DocumentBackgroundTest : public ::testing::Test {
public:
    DocumentBackgroundTest() : Test()
    {
        metrics.theme("black").size(1000, 1000).dpi(160).mode(kViewportModeHub);
        config.set({{RootProperty::kAgentName, "backgroundTest"}, {RootProperty::kAgentVersion, "0.1"}});
    }

    Object load(const char *document) {
        auto content = Content::create(document, makeDefaultSession());
        return content->getBackground(metrics, config);
    }

    Metrics metrics;
    RootConfig config;
};


/**
 * Test cases for the "background" property of the document object.  The background may be either
 * a color or a gradient.  If it is poorly defined, it will be returned as the TRANSPARENT color.
 */

static const char *NO_BACKGROUND = R"({
  "type": "APL",
  "version": "1.1",
  "mainTemplate": {
    "items": {
      "type": "Text"
    }
  }
})";

TEST_F(DocumentBackgroundTest, NoBackground)
{
    auto background = load(NO_BACKGROUND);

    ASSERT_TRUE(background.is<Color>());
    ASSERT_TRUE(IsEqual(Color(Color::TRANSPARENT), background));
}

static const char *COLOR_BACKGROUND = R"({
  "type": "APL",
  "version": "1.1",
  "background": "blue",
  "mainTemplate": {
    "items": {
      "type": "Text"
    }
  }
})";

TEST_F(DocumentBackgroundTest, ColorBackground)
{
    auto background = load(COLOR_BACKGROUND);

    ASSERT_TRUE(background.is<Color>());
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), background));

    // New API will fail
    auto content = Content::create(COLOR_BACKGROUND, makeDefaultSession());
    ASSERT_TRUE(IsEqual(Color(Color::TRANSPARENT), content->getBackground()));
}

static const char *GRADIENT_BACKGROUND = R"({
  "type": "APL",
  "version": "1.1",
  "background": {
    "type": "linear",
    "colorRange": [
      "darkgreen",
      "white"
    ],
    "inputRange": [
      0,
      0.25
    ],
    "angle": 90
  },
  "mainTemplate": {
    "items": {
      "type": "Text"
    }
  }
})";

TEST_F(DocumentBackgroundTest, GradientBackground)
{
    auto background = load(GRADIENT_BACKGROUND);

    ASSERT_TRUE(background.is<Gradient>());

    auto gradient = background.get<Gradient>();
    ASSERT_EQ(Gradient::GradientType::LINEAR, gradient.getType());
    ASSERT_EQ(90, gradient.getProperty(kGradientPropertyAngle).getInteger());
    ASSERT_EQ(std::vector<Object>({Color(0x006400ff), Color(0xffffffff)}), gradient.getProperty(kGradientPropertyColorRange).getArray());
    ASSERT_EQ(std::vector<Object>({0.0, 0.25}), gradient.getProperty(kGradientPropertyInputRange).getArray());
}

static const char *BAD_BACKGROUND_MAP = R"({
  "type": "APL",
  "version": "1.1",
  "background": {
    "type": "Foo"
  },
  "mainTemplate": {
    "items": {
      "type": "Text"
    }
  }
})";

TEST_F(DocumentBackgroundTest, BadBackgroundMap)
{
    auto background = load(BAD_BACKGROUND_MAP);

    ASSERT_TRUE(background.is<Color>());
    ASSERT_TRUE(IsEqual(Color(Color::TRANSPARENT), background));
}

static const char *BAD_BACKGROUND_COLOR = R"({
  "type": "APL",
  "version": "1.1",
  "background": "bluish",
  "mainTemplate": {
    "items": {
      "type": "Text"
    }
  }
})";

TEST_F(DocumentBackgroundTest, BadBackgroundColor)
{
    auto background = load(BAD_BACKGROUND_COLOR);

    ASSERT_TRUE(background.is<Color>());
    ASSERT_TRUE(IsEqual(Color(Color::TRANSPARENT), background));
}

static const char *DATA_BINDING_TEST = R"({
  "type": "APL",
  "version": "1.1",
  "background": "${viewport.width > 500 ? 'blue' : 'red'}",
  "mainTemplate": {
    "items": {
      "type": "Text"
    }
  }
})";

TEST_F(DocumentBackgroundTest, DataBindingTest)
{
    // Small screens get a red background
    metrics.size(100, 100);
    auto background = load(DATA_BINDING_TEST);
    ASSERT_TRUE(background.is<Color>());
    ASSERT_TRUE(IsEqual(Color(Color::RED), background));

    // Large screens get a blue background
    metrics.size(1000,1000);
    background = load(DATA_BINDING_TEST);
    ASSERT_TRUE(background.is<Color>());
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), background));
}

/*
 * Check to see that a data-binding expression can use the system theme
 */
static const char *DATA_BOUND_THEME = R"({
  "type": "APL",
  "version": "1.1",
  "background": "${viewport.theme == 'dark' ? 'rgb(16,32,64)' : 'rgb(224, 224, 192)'}",
  "mainTemplate": {
    "items": {
      "type": "Text"
    }
  }
})";

TEST_F(DocumentBackgroundTest, DataBoundTheme)
{
    metrics.theme("dark");
    auto background = load(DATA_BOUND_THEME);
    ASSERT_TRUE(background.is<Color>());
    ASSERT_TRUE(IsEqual(Color(0x102040ff), background));

    metrics.theme("light");
    background = load(DATA_BOUND_THEME);
    ASSERT_TRUE(background.is<Color>());
    ASSERT_TRUE(IsEqual(Color(0xe0e0c0ff), background));
}

/*
 * Check that a data-binding expression using a theme can be overridden by the document-supplied theme
 */
static const char *DATA_BOUND_THEME_OVERRIDE = R"({
  "type": "APL",
  "version": "1.1",
  "theme": "light",
  "background": "${viewport.theme == 'dark' ? 'rgb(16,32,64)' : 'rgb(224, 224, 192)'}",
  "mainTemplate": {
    "items": {
      "type": "Text"
    }
  }
})";

TEST_F(DocumentBackgroundTest, DataBoundThemeOverride)
{
    metrics.theme("dark");
    auto background = load(DATA_BOUND_THEME_OVERRIDE);
    ASSERT_TRUE(background.is<Color>());
    ASSERT_TRUE(IsEqual(Color(0xe0e0c0ff), background));

    metrics.theme("light");
    background = load(DATA_BOUND_THEME_OVERRIDE);
    ASSERT_TRUE(background.is<Color>());
    ASSERT_TRUE(IsEqual(Color(0xe0e0c0ff), background));
}

TEST_F(DocumentBackgroundTest, NewContentApi)
{
    metrics.theme("dark");
    auto content = Content::create(DATA_BOUND_THEME_OVERRIDE, makeDefaultSession(), metrics, config);
    auto background = content->getBackground();
    ASSERT_TRUE(background.is<Color>());
    ASSERT_TRUE(IsEqual(Color(0xe0e0c0ff), background));
}
