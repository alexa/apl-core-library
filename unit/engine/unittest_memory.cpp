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
#include "apl/apl.h"

using namespace apl;

class MemoryTest : public MemoryWrapper {
public:
    ComponentPtr component;
    RootContextPtr root;
};

static const char *BASIC_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "item": {
        "type": "Frame"
      }
    }
  }
})";

TEST_F(MemoryTest, Basic)
{
    auto content = Content::create(BASIC_DOC, makeDefaultSession());

    ASSERT_TRUE(content->isReady());
    ASSERT_FALSE(content->isWaiting());
    ASSERT_FALSE(content->isError());

    auto m = Metrics().size(1024,800).theme("dark");
    auto config = RootConfig().defaultIdleTimeout(15000);
    root = RootContext::create(m, content, config);

    ASSERT_TRUE(root);
    component = root->topComponent();
    ASSERT_EQ(kComponentTypeContainer, component->getType());

    component = nullptr;
    root = nullptr;
}

class MemTextMeasure : public TextMeasurement {
public:
    LayoutSize measure(Component *component, float width, MeasureMode widthMode, float height,
                   MeasureMode heightMode) override {
        return LayoutSize({5, 5});
    }

    float baseline(Component *component, float width, float height) override {
        return height;
    }
};

static const char *TEXT_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "item": {
        "type": "Text"
      }
    }
  }
})";

TEST_F(MemoryTest, Text)
{
    auto content = Content::create(TEXT_DOC, makeDefaultSession());

    ASSERT_TRUE(content->isReady());
    ASSERT_FALSE(content->isWaiting());
    ASSERT_FALSE(content->isError());

    auto m = Metrics().size(1024,800).theme("dark");
    auto config = RootConfig().defaultIdleTimeout(15000).measure(std::make_shared<MemTextMeasure>());
    root = RootContext::create(m, content, config);

    ASSERT_TRUE(root);
    component = root->topComponent();
    ASSERT_EQ(kComponentTypeContainer, component->getType());

    component = nullptr;
    root = nullptr;
}
