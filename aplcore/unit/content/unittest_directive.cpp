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

#include <iostream>

#include "rapidjson/document.h"
#include "gtest/gtest.h"

#include "apl/engine/evaluate.h"
#include "apl/content/metrics.h"
#include "apl/component/component.h"
#include "apl/content/directive.h"

#include "../testeventloop.h"

using namespace apl;

class DirectiveTest : public MemoryWrapper {};

static const char *DIRECTIVE =
    "{"
    "  \"name\": \"RenderDocument\","
    "  \"namespace\": \"Alexa.Presentation.APL\","
    "  \"payload\": {"
    "    \"document\": {"
    "      \"type\": \"APL\","
    "      \"version\": \"1.0\","
    "      \"import\": [],"
    "      \"resources\": ["
    "        {"
    "          \"strings\": {"
    "            \"label\": \"My\""
    "          }"
    "        }"
    "      ],"
    "      \"mainTemplate\": {"
    "        \"parameters\": ["
    "          \"payload\""
    "        ],"
    "        \"item\": {"
    "          \"type\": \"Text\","
    "          \"text\": \"${@label} ${payload.title}\""
    "        }"
    "      }"
    "    },"
    "    \"datasources\": {"
    "      \"title\": \"Pecan Pie V\""
    "    }"
    "  }"
    "}";


TEST_F(DirectiveTest, SimpleDocument)
{
    auto doc = Directive::create(DIRECTIVE, session);
    ASSERT_TRUE(doc);

    auto m = Metrics().size(1024,800).theme("dark");
    auto root = doc->build(m);
    ComponentPtr component = root->topComponent();
    ASSERT_TRUE(component);

    auto map = component->getCalculated();
    ASSERT_EQ("My Pecan Pie V", map["text"].asString());
}


static const char *BAD_DIRECTIVE = "{"
"  \"name\": \"RenderDocument\","
"  \"document\": {"
"    \"type\": \"APL\","
"}";

TEST_F(DirectiveTest, BadDocument)
{
    auto doc = Directive::create(BAD_DIRECTIVE, session);
    ASSERT_FALSE(doc);
    ASSERT_TRUE(ConsoleMessage());
}
