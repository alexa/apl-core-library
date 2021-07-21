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

class ViewhostTest : public DocumentWrapper {};

static const char *BASIC =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"This is ${viewport.theme}\""
    "    }"
    "  }"
    "}";

TEST_F(ViewhostTest, Basic)
{
    metrics.dpi(320)
        .theme("brilliant")
        .size(1000,1000)
        .shape(ROUND)
        .mode(kViewportModeAuto);

    loadDocument(BASIC);
    ASSERT_TRUE(component);

    auto viewport = context->opt("viewport");
    ASSERT_TRUE(IsEqual(500, viewport.get("width")));
    ASSERT_TRUE(IsEqual(500, viewport.get("height")));
    ASSERT_TRUE(IsEqual("round", viewport.get("shape")));
    ASSERT_TRUE(IsEqual(1000, viewport.get("pixelWidth")));
    ASSERT_TRUE(IsEqual(1000, viewport.get("pixelHeight")));
    ASSERT_TRUE(IsEqual(320, viewport.get("dpi")));
    ASSERT_TRUE(IsEqual("brilliant", viewport.get("theme")));
    ASSERT_TRUE(IsEqual("auto", viewport.get("mode")));

    ASSERT_TRUE(IsEqual("This is brilliant", component->getCalculated(kPropertyText).asString()));
}

static const char *OVERRIDE_THEME =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"theme\": \"fuzzy\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"This is ${viewport.theme}\""
    "    }"
    "  }"
    "}";

TEST_F(ViewhostTest, OverrideTheme)
{
    metrics.dpi(480)
        .theme("brilliant")
        .size(3000,900)
        .shape(RECTANGLE)
        .mode(kViewportModeMobile);

    loadDocument(OVERRIDE_THEME);
    ASSERT_TRUE(component);

    auto viewport = context->opt("viewport");
    ASSERT_TRUE(IsEqual(1000, viewport.get("width")));
    ASSERT_TRUE(IsEqual(300, viewport.get("height")));
    ASSERT_TRUE(IsEqual("rectangle", viewport.get("shape")));
    ASSERT_TRUE(IsEqual(3000, viewport.get("pixelWidth")));
    ASSERT_TRUE(IsEqual(900, viewport.get("pixelHeight")));
    ASSERT_TRUE(IsEqual(480, viewport.get("dpi")));
    ASSERT_TRUE(IsEqual("fuzzy", viewport.get("theme")));
    ASSERT_TRUE(IsEqual("mobile", viewport.get("mode")));

    ASSERT_TRUE(IsEqual("This is fuzzy", component->getCalculated(kPropertyText).asString()));
}
