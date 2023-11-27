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

#include "../media/testmediaplayerfactory.h"
#include "apl/component/videocomponent.h"
#include "apl/focus/focusmanager.h"
#include "apl/primitives/object.h"

using namespace apl;

class VideoComponentTest : public DocumentWrapper {
public:
    VideoComponentTest() : DocumentWrapper() {
        mediaPlayerFactory = std::make_shared<TestMediaPlayerFactory>();
        config->mediaPlayerFactory(mediaPlayerFactory);
    }

    std::shared_ptr<TestMediaPlayerFactory> mediaPlayerFactory;
};

static const char* VIDEO_IN_CONTAINER = R"({
  "type": "APL",
  "version": "2023.1",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "width": 200,
      "height": 200,
      "items": {
        "type": "Video",
        "id": "VIDEO",
        "width": "100%",
        "height": "100%"
      }
    }
  }
})";

TEST_F(VideoComponentTest, DisallowVideoTrueDisallowsComponent) {
    config->set(RootProperty::kDisallowVideo, true);
    loadDocument(VIDEO_IN_CONTAINER);

    ASSERT_TRUE(component);
    auto v = std::static_pointer_cast<CoreComponent>(root->findComponentById("VIDEO"));
    ASSERT_EQ(v->isDisallowed(), true);
    // No media player when disallow is true
    ASSERT_EQ(v->getMediaPlayer(), nullptr);
}

TEST_F(VideoComponentTest, DisallowVideoFalseAllowsComponent) {
    config->set(RootProperty::kDisallowVideo, false);
    loadDocument(VIDEO_IN_CONTAINER);

    ASSERT_TRUE(component);
    auto v = std::static_pointer_cast<CoreComponent>(root->findComponentById("VIDEO"));
    ASSERT_EQ(v->isDisallowed(), false);
    // Has media player when disallow is false
    ASSERT_TRUE(v->getMediaPlayer());
}

static const char* VIDEO_IN_CONTAINER_WITH_REINFLATE = R"({
  "type": "APL",
  "version": "2023.1",
  "onConfigChange": {
      "type": "Reinflate"
  },
  "mainTemplate": {
    "items": {
      "type": "Container",
      "width": 200,
      "height": 200,
      "items": {
        "type": "Video",
        "id": "VIDEO",
        "width": "100%",
        "height": "100%"
      }
    }
  }
})";

TEST_F(VideoComponentTest, ConfigChangeDisallowVideoTrueToFalseWillAllowComponent) {
    // Initial configuration
    config->set(RootProperty::kDisallowVideo, true);
    loadDocument(VIDEO_IN_CONTAINER_WITH_REINFLATE);
    ASSERT_TRUE(component);

    auto v = std::static_pointer_cast<CoreComponent>(root->findComponentById("VIDEO"));
    ASSERT_EQ(v->isDisallowed(), true);
    // No media player when disallow is true
    ASSERT_EQ(v->getMediaPlayer(), nullptr);

    // Trigger config change
    auto configChange = ConfigurationChange().disallowVideo(false);
    root->configurationChange(configChange);
    processReinflate();
    advanceTime(100);

    v = std::static_pointer_cast<CoreComponent>(root->findComponentById("VIDEO"));
    ASSERT_EQ(v->isDisallowed(), false);
    // Has media player when disallow is false
    ASSERT_TRUE(v->getMediaPlayer());
}

TEST_F(VideoComponentTest, ConfigChangeDisallowVideoFalseToTrueWillDisallowComponent) {
    // Initial configuration
    config->set(RootProperty::kDisallowVideo, false);
    loadDocument(VIDEO_IN_CONTAINER_WITH_REINFLATE);

    ASSERT_TRUE(component);
    auto v = std::static_pointer_cast<CoreComponent>(root->findComponentById("VIDEO"));
    ASSERT_EQ(v->isDisallowed(), false);
    // Has media player when disallow is false
    ASSERT_TRUE(v->getMediaPlayer());

    // Trigger config change
    auto configChange = ConfigurationChange().disallowVideo(true);
    root->configurationChange(configChange);
    processReinflate();
    advanceTime(100);

    v = std::static_pointer_cast<CoreComponent>(root->findComponentById("VIDEO"));
    ASSERT_EQ(v->isDisallowed(), true);
    // No media player when disallow is true
    ASSERT_EQ(v->getMediaPlayer(), nullptr);
}

TEST_F(VideoComponentTest, ConfigChangeDisallowVideoFalseToFalseDoesntDisallowComponent) {
    // Initial configuration
    config->set(RootProperty::kDisallowVideo, false);
    loadDocument(VIDEO_IN_CONTAINER_WITH_REINFLATE);

    ASSERT_TRUE(component);
    auto v = std::static_pointer_cast<CoreComponent>(root->findComponentById("VIDEO"));
    ASSERT_EQ(v->isDisallowed(), false);
    // Has media player when disallow is false
    ASSERT_TRUE(v->getMediaPlayer());

    // Trigger config change
    auto configChange = ConfigurationChange().disallowVideo(false);
    root->configurationChange(configChange);
    processReinflate();
    advanceTime(100);

    v = std::static_pointer_cast<CoreComponent>(root->findComponentById("VIDEO"));
    ASSERT_EQ(v->isDisallowed(), false);
    // Has media player when disallow is false
    ASSERT_TRUE(v->getMediaPlayer());
}

TEST_F(VideoComponentTest, ConfigChangeDisallowVideoTrueToTrueDoesntAllowComponent) {
    // Initial configuration
    config->set(RootProperty::kDisallowVideo, true);
    loadDocument(VIDEO_IN_CONTAINER_WITH_REINFLATE);

    ASSERT_TRUE(component);
    auto v = std::static_pointer_cast<CoreComponent>(root->findComponentById("VIDEO"));
    ASSERT_EQ(v->isDisallowed(), true);
    // No media player when disallow is true
    ASSERT_EQ(v->getMediaPlayer(), nullptr);

    // Trigger config change
    auto configChange = ConfigurationChange().disallowVideo(true);
    root->configurationChange(configChange);
    processReinflate();
    advanceTime(100);

    v = std::static_pointer_cast<CoreComponent>(root->findComponentById("VIDEO"));
    ASSERT_EQ(v->isDisallowed(), true);
    // No media player when disallow is true
    ASSERT_EQ(v->getMediaPlayer(), nullptr);
}

TEST_F(VideoComponentTest, ComponentNotDisplayedWhenDisallowVideoTrue) {
    config->set(RootProperty::kDisallowVideo, true);

    loadDocument(VIDEO_IN_CONTAINER);

    ASSERT_TRUE(component);
    // Inflated as expected
    ASSERT_EQ(1, component->getChildCount());
    ASSERT_EQ(kComponentTypeVideo, component->getCoreChildAt(0)->getType());
    // Not displayed
    ASSERT_EQ(0, component->getDisplayedChildCount());
}

TEST_F(VideoComponentTest, ComponentDisplayedWhenDisallowVideoFalse) {
    config->set(RootProperty::kDisallowVideo, false);

    loadDocument(VIDEO_IN_CONTAINER);

    ASSERT_TRUE(component);
    // Inflated as expected
    ASSERT_EQ(1, component->getChildCount());
    ASSERT_EQ(kComponentTypeVideo, component->getCoreChildAt(0)->getType());
    // Displayed
    ASSERT_EQ(1, component->getDisplayedChildCount());
    ASSERT_EQ(kComponentTypeVideo, component->getDisplayedChildAt(0)->getType());
}