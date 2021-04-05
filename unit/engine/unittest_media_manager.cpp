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
#include "apl/engine/event.h"
#include "apl/primitives/object.h"

using namespace apl;

class MediaManagerTest : public DocumentWrapper {
public:
    MediaManagerTest() : DocumentWrapper() {
        config->enableExperimentalFeature(RootConfig::kExperimentalFeatureManageMediaRequests);
    }

    template<class... Args>
    ::testing::AssertionResult
    MediaRequested(Args... args) {
        if (!root->hasEvent())
            return ::testing::AssertionFailure() << "No event.";

        // Event should be fired that requests media to be loaded.
        auto event = root->popEvent();
        if (kEventTypeMediaRequest != event.getType())
            return ::testing::AssertionFailure() << "Wrong event type.";

        auto sources = event.getValue(kEventPropertySource);
        if (!sources.isArray())
            return ::testing::AssertionFailure() << "Not a string as expected.";

        std::vector<std::string> expectedSources = {args...};
        auto& actualSources = sources.getArray();

        if (expectedSources.size() != actualSources.size())
            return ::testing::AssertionFailure() << "Wrong number of sources requested.";

        for (int i = 0; i<actualSources.size(); i++) {
            if (expectedSources.at(i) != actualSources.at(i).getString())
                return ::testing::AssertionFailure()
                        << "Wrong media, expected: " << expectedSources.at(i)
                        << ", actual: " << actualSources.at(i).getString();
        }

        return ::testing::AssertionSuccess();
    }

    template<class... Args>
    ::testing::AssertionResult
    CheckLoadedMedia(const ComponentPtr& comp, Args... args) {
        std::vector<std::string> sources = {args...};
        for (auto& source : sources) {
            root->mediaLoaded(source);
        }

        auto dirty = CheckDirty(comp, kPropertyMediaState);
        if (!dirty)
            return dirty;

        auto state = component->getCalculated(kPropertyMediaState).getInteger();
        if (kMediaStateReady != state)
            return ::testing::AssertionFailure()
                    << "Wrong media state, expected: " << kMediaStateReady
                    << ", actual: " << state;

        return ::testing::AssertionSuccess();
    }
};

static const char* SINGLE_IMAGE = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Image",
      "source": "universe"
    }
  }
})";

TEST_F(MediaManagerTest, SingleImage) {
    loadDocument(SINGLE_IMAGE);

    ASSERT_FALSE(root->isDirty());

    // Event should be fired that requests media to be loaded.
    ASSERT_TRUE(MediaRequested("universe"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(component, "universe"));
}

static const char* MULTIPLE_IMAGES_WITHOUT_FILTERS = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Image",
      "sources": ["universe0", "universe1", "universe2", "universe3"]
    }
  }
})";

TEST_F(MediaManagerTest, MultipleImagesWithoutFilter) {
    loadDocument(MULTIPLE_IMAGES_WITHOUT_FILTERS);

    ASSERT_FALSE(root->isDirty());

    // Event should be fired that requests media to be loaded.
    // Should request only 1 image (last one), as per spec.
    ASSERT_TRUE(MediaRequested("universe3"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(component, "universe3"));
}

static const char* MULTIPLE_IMAGES_WITH_FILTERS = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Image",
      "sources": ["universe0", "universe1", "universe2", "universe3"],
      "filters": {
        "type": "Blend",
        "mode": "normal"
      }
    }
  }
})";

TEST_F(MediaManagerTest, MultipleImagesWithFilters) {
    loadDocument(MULTIPLE_IMAGES_WITH_FILTERS);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested("universe0", "universe1", "universe2", "universe3"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(component, "universe0", "universe1", "universe2", "universe3"));
}

TEST_F(MediaManagerTest, MultipleImagesWithFiltersPartialLoad) {
    loadDocument(MULTIPLE_IMAGES_WITH_FILTERS);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested("universe0", "universe1", "universe2", "universe3"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    root->mediaLoaded("universe0");
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(component, "universe1", "universe2", "universe3"));
}

TEST_F(MediaManagerTest, MultipleImagesWithFiltersLoadFail) {
    loadDocument(MULTIPLE_IMAGES_WITH_FILTERS);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested("universe0", "universe1", "universe2", "universe3"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    root->mediaLoadFailed("universe0");
    ASSERT_EQ(kMediaStateError, component->getCalculated(kPropertyMediaState).getInteger());
    root->mediaLoaded("universe1");
    ASSERT_EQ(kMediaStateError, component->getCalculated(kPropertyMediaState).getInteger());
}

TEST_F(MediaManagerTest, MultipleImagesWithFiltersLoadFailAfterOneLoad) {
    loadDocument(MULTIPLE_IMAGES_WITH_FILTERS);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested("universe0", "universe1", "universe2", "universe3"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    root->mediaLoaded("universe1");
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    root->mediaLoadFailed("universe0");
    ASSERT_EQ(kMediaStateError, component->getCalculated(kPropertyMediaState).getInteger());
    root->mediaLoaded("universe2");
    ASSERT_EQ(kMediaStateError, component->getCalculated(kPropertyMediaState).getInteger());
}

TEST_F(MediaManagerTest, MultipleImagesWithFiltersLoadFailAfterAllLoadedIgnored) {
    loadDocument(MULTIPLE_IMAGES_WITH_FILTERS);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested("universe0", "universe1", "universe2", "universe3"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(component, "universe0", "universe1", "universe2", "universe3"));

    root->mediaLoadFailed("universe0");
    ASSERT_EQ(kMediaStateReady, component->getCalculated(kPropertyMediaState).getInteger());
}

static const char* MULTIPLE_IMAGES_WITH_FILTERS_DUPLICATE = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Image",
      "sources": ["universe0", "universe0", "universe1", "universe1"],
      "filters": {
        "type": "Blend",
        "mode": "normal"
      }
    }
  }
})";

TEST_F(MediaManagerTest, MultipleImagesWithFiltersDuplicate) {
    loadDocument(MULTIPLE_IMAGES_WITH_FILTERS_DUPLICATE);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested("universe0", "universe1"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(component, "universe0", "universe1"));
}

TEST_F(MediaManagerTest, SingleImageUpdate) {
    loadDocument(SINGLE_IMAGE);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested("universe"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(component, "universe"));

    component->setProperty(kPropertySource, "sample");

    ASSERT_TRUE(CheckDirty(component, kPropertySource, kPropertyMediaState));

    ASSERT_TRUE(MediaRequested("sample"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(component, "sample"));
}

static const char* SIMPLE_SEQUENCE = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "height": 200,
      "width": 200,
      "data": [0, 1, 2, 3, 4, 5, 6],
      "item": {
        "type": "Image",
        "source": "universe${data}",
        "height": 100,
        "width": 200
      }
    }
  }
})";

TEST_F(MediaManagerTest, SimpleSequence) {
    loadDocument(SIMPLE_SEQUENCE);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested("universe0", "universe1", "universe2", "universe3", "universe4"));
    ASSERT_FALSE(root->hasEvent());

    component->update(kUpdateScrollPosition, 100);
    root->clearPending();

    ASSERT_TRUE(MediaRequested("universe5"));
    ASSERT_FALSE(root->hasEvent());
}

static const char* SIMPLE_PAGER = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Pager",
      "navigation": "normal",
      "height": 200,
      "width": 200,
      "data": [0, 1, 2, 3, 4, 5, 6],
      "item": {
        "type": "Image",
        "source": "universe${data}"
      }
    }
  }
})";

TEST_F(MediaManagerTest, SimplePager) {
    loadDocument(SIMPLE_PAGER);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested("universe0", "universe1"));
    ASSERT_FALSE(root->hasEvent());

    component->update(kUpdatePagerPosition, 1);
    root->clearPending();

    ASSERT_TRUE(MediaRequested("universe2"));
    ASSERT_FALSE(root->hasEvent());
}

static const char *LIVE_CHANGES = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "data": "${TestArray}",
      "item": {
        "type": "Image",
        "source": "universe${data}",
        "height": 100,
        "width": 100
      }
    }
  }
})";

TEST_F(MediaManagerTest, ComponentClear) {
    auto myArray = LiveArray::create(ObjectArray{0, 1, 2, 3});
    config->liveData("TestArray", myArray);

    loadDocument(LIVE_CHANGES);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested("universe0", "universe1", "universe2", "universe3"));
    ASSERT_FALSE(root->hasEvent());

    myArray->push_back(4);
    root->clearPending();

    ASSERT_TRUE(MediaRequested("universe4"));
    ASSERT_FALSE(root->hasEvent());
}

static const char* SINGLE_VIDEO = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Video",
      "source": "universe"
    }
  }
})";

TEST_F(MediaManagerTest, SingleVideo) {
    loadDocument(SINGLE_VIDEO);

    ASSERT_FALSE(root->isDirty());

    // Event should be fired that requests media to be loaded.
    ASSERT_TRUE(MediaRequested("universe"));
    ASSERT_TRUE(CheckLoadedMedia(component, "universe"));
}

static const char* MULTIPLE_VIDEO_SOURCES = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Video",
      "sources": ["universe0", "universe1", "universe2", "universe3"]
    }
  }
})";

TEST_F(MediaManagerTest, MultipleVideoSources) {
    loadDocument(MULTIPLE_VIDEO_SOURCES);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested("universe0", "universe1", "universe2", "universe3"));

    // Video can be rendered with just current video loaded usually, so should be marked dirty when current index was
    // loaded.
    root->mediaLoaded("universe0");
    ASSERT_TRUE(CheckDirty(component, kPropertyMediaState));
}

TEST_F(MediaManagerTest, MultipleVideoSourcesFailureAfterCurrentLoaded) {
    loadDocument(MULTIPLE_VIDEO_SOURCES);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested("universe0", "universe1", "universe2", "universe3"));

    // Video can be rendered with just current video loaded usually, so should be marked dirty when current index was
    // loaded.
    root->mediaLoaded("universe0");
    ASSERT_TRUE(CheckDirty(component, kPropertyMediaState));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());

    root->mediaLoadFailed("universe1");
    ASSERT_EQ(kMediaStateError, component->getCalculated(kPropertyMediaState).getInteger());
    root->mediaLoaded("universe2");
    ASSERT_EQ(kMediaStateError, component->getCalculated(kPropertyMediaState).getInteger());
}