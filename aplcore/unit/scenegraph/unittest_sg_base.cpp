/*
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
 *
 */

#include "../testeventloop.h"
#include "test_sg.h"
#include "apl/scenegraph/builder.h"

using namespace apl;

class SGBaseTest : public DocumentWrapper {};

static const char* DEFAULT_DOC = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Frame"
    }
  }
})";

/**
 * A trivial scene graph should still give the correct size
 */
TEST_F(SGBaseTest, Simple)
{
    metrics.size(200,300);
    loadDocument(DEFAULT_DOC);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(IsEqual(Size(200, 300), sg->getViewportSize()));
}


static const char* MUTATING_DOC = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "id": "TARGET",
      "width": 200,
      "height": 200
    }
  }
})";

/**
 * A mutating document in a variable viewport will change the viewport size
 */
TEST_F(SGBaseTest, Mutating)
{
    metrics.size(300,300).minAndMaxWidth(100,400).minAndMaxHeight(150,350);
    loadDocument(MUTATING_DOC);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(IsEqual(Size(200, 200), sg->getViewportSize()));  // Wrapped to component

    executeCommand("SetValue", {{"componentId", "TARGET"}, {"property", "width"}, {"value", 50}}, false);
    sg = root->getSceneGraph();
    ASSERT_TRUE(IsEqual(Size(100, 200), sg->getViewportSize()));  // Wrapped to component, but clipped to minWidth

    executeCommand("SetValue", {{"componentId", "TARGET"}, {"property", "height"}, {"value", 600}}, false);
    sg = root->getSceneGraph();
    ASSERT_TRUE(IsEqual(Size(100, 350), sg->getViewportSize()));  // Maxes out viewport height
}
