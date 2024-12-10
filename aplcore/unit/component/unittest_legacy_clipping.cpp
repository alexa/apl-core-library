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

#include "gtest/gtest.h"

#include "apl/engine/evaluate.h"
#include "apl/engine/builder.h"
#include "apl/component/component.h"

#include "../testeventloop.h"

using namespace apl;

class LegacyClipping : public DocumentWrapper {};

static const char* TOP_FRAME_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "Frame"
    }
  }
})";

/**
 * Top components always clip.
 */
TEST_F(LegacyClipping, TopFrameShouldClip) {
    loadDocument(TOP_FRAME_DOC);

    auto frame = std::static_pointer_cast<CoreComponent>(root->topComponent());
    ASSERT_TRUE(frame->shouldClip());
}

static const char* TOP_CONTAINER_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "item": {
      "type": "Container"
    }
  }
})";

/**
 * Top components always clip.
 */
TEST_F(LegacyClipping, TopContainerShouldClip) {
    loadDocument(TOP_CONTAINER_DOC);

    auto container = std::static_pointer_cast<CoreComponent>(root->topComponent());
    ASSERT_TRUE(container->shouldClip());
}

static const char* CONTAINER_IMAGE_DOC_15 = R"({
    "type": "APL",
    "version": "1.5",
    "mainTemplate": {
      "items": {
        "type": "Container",
        "width": 100,
        "height": 100,
        "items": {
          "type": "Image",
          "width": 200,
          "height": 200
        }
      }
    }
})";

/**
 * Test that container children do not clipping in legacy versions (< 1.6)
 */
TEST_F(LegacyClipping, LegacyContainerChildrenShouldNotClip) {
    loadDocument(CONTAINER_IMAGE_DOC_15);

    auto image = std::static_pointer_cast<CoreComponent>(component->getChildAt(0));
    ASSERT_FALSE(image->shouldClip());
}

static const char* CONTAINER_PAGER_DOC_15 = R"({
    "type": "APL",
    "version": "1.5",
    "mainTemplate": {
      "items": {
        "type": "Container",
        "width": 100,
        "height": 100,
        "items": {
          "type": "Pager",
          "width": 200,
          "height": 200
        }
      }
    }
})";

/**
 * Test that Pagers do clip in legacy versions (< 1.6)
 */
TEST_F(LegacyClipping, LegacyPagerChildrenShouldClip) {
    loadDocument(CONTAINER_PAGER_DOC_15);

    auto pager = std::static_pointer_cast<CoreComponent>(component->getChildAt(0));
    ASSERT_TRUE(pager->shouldClip());
}

static const char* CONTAINER_FRAME_DOC_15 = R"({
    "type": "APL",
    "version": "1.5",
    "mainTemplate": {
      "items": {
        "type": "Container",
        "width": 100,
        "height": 100,
        "items": {
          "type": "Frame",
          "width": 200,
          "height": 200
        }
      }
    }
})";

/**
 * Test that Frames do clip in legacy versions (< 1.6)
 */
TEST_F(LegacyClipping, LegacyFrameShouldClip) {
    loadDocument(CONTAINER_FRAME_DOC_15);

    auto frame = std::static_pointer_cast<CoreComponent>(component->getChildAt(0));
    ASSERT_TRUE(frame->shouldClip());
}

static const char* CONTAINER_SEQUENCE_DOC_15 = R"({
    "type": "APL",
    "version": "1.5",
    "mainTemplate": {
      "items": {
        "type": "Container",
        "width": 100,
        "height": 100,
        "items": {
          "type": "Sequence",
          "width": 200,
          "height": 200
        }
      }
    }
})";

/**
 * Test that Sequences do clip in legacy versions (< 1.6)
 */
TEST_F(LegacyClipping, LegacySequenceShouldClip) {
    loadDocument(CONTAINER_SEQUENCE_DOC_15);

    auto sequence = std::static_pointer_cast<CoreComponent>(component->getChildAt(0));
    ASSERT_TRUE(sequence->shouldClip());
}

static const char* CONTAINER_IMAGE_DOC_16 = R"({
    "type": "APL",
    "version": "1.6",
    "mainTemplate": {
      "items": {
        "type": "Container",
        "width": 100,
        "height": 100,
        "items": {
          "type": "Image",
          "width": 200,
          "height": 200
        }
      }
    }
})";

/**
 * Test that container children do clip
 */
TEST_F(LegacyClipping, ContainerChildrenShouldClip) {
    loadDocument(CONTAINER_IMAGE_DOC_16);

    auto image = std::static_pointer_cast<CoreComponent>(component->getChildAt(0));
    ASSERT_TRUE(image->shouldClip());
}