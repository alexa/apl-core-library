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

#include "apl/component/imagecomponent.h"
#include "apl/component/textcomponent.h"
#include "apl/engine/event.h"
#include "apl/media/coremediamanager.h"
#include "apl/media/mediaobject.h"
#include "apl/primitives/object.h"

using namespace apl;

class MediaManagerTest : public DocumentWrapper
{
public:
    MediaManagerTest() : DocumentWrapper() {
        config->enableExperimentalFeature(RootConfig::kExperimentalFeatureManageMediaRequests);
    }

    template <class... Args>
    ::testing::AssertionResult MediaRequested(EventMediaType mediaType, Args... args) {
        if (!root->hasEvent())
            return ::testing::AssertionFailure() << "No event.";

        // Event should be fired that requests media to be loaded.
        auto event = root->popEvent();
        auto type = event.getType();
        if (kEventTypeMediaRequest != type)
            return ::testing::AssertionFailure() << "Wrong event type. Expected: " << kEventTypeMediaRequest
                                                 << ", actual: "  << type;

        if (event.getValue(kEventPropertyMediaType).asInt() != mediaType) {
            return ::testing::AssertionFailure() << "Wrong media type.";
        }

        auto sources = event.getValue(kEventPropertySource);
        if (!sources.isArray())
            return ::testing::AssertionFailure() << "Not an array as expected.";

        std::set<std::string> expectedSources = {args...};
        std::set<std::string> actualSources;
        for (const auto& m : sources.getArray())
            actualSources.emplace(m.getString());

        if (expectedSources.size() != actualSources.size())
            return ::testing::AssertionFailure()
                << "Source list size mismatch. Expected: " << expectedSources.size()
                << ", actual: " << actualSources.size();

        std::string es;
        std::string as;

        auto first = true;
        for (auto const& s : expectedSources) {
            if (!first) es += ',';
            first = false;
            es += s;
        }

        first = true;
        for (auto const& s : actualSources) {
            if (!first) as += ',';
            first = false;
            as += s;
        }

        if (expectedSources != actualSources)
            return ::testing::AssertionFailure() << "Source mismatch. Expected: [" << es << "], actual: [" << as << "]";

        return ::testing::AssertionSuccess();
    }

    template <class... Args>
    ::testing::AssertionResult CheckLoadedMedia(const ComponentPtr& comp, Args... args) {
        std::vector<std::string> sources = {args...};
        for (auto& source : sources) {
            root->mediaLoaded(source);
        }

        auto dirty = CheckDirty(comp, kPropertyMediaState);
        if (!dirty)
            return dirty;

        auto state = comp->getCalculated(kPropertyMediaState).getInteger();
        if (kMediaStateReady != state)
            return ::testing::AssertionFailure()
                   << "Wrong media state, expected: " << kMediaStateReady << ", actual: " << state;

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
    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe"));
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
    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe3"));
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

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe0", "universe1", "universe2", "universe3"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(component, "universe0", "universe1", "universe2", "universe3"));
}

TEST_F(MediaManagerTest, MultipleImagesWithFiltersPartialLoad) {
    loadDocument(MULTIPLE_IMAGES_WITH_FILTERS);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe0", "universe1", "universe2", "universe3"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    root->mediaLoaded("universe0");
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(component, "universe1", "universe2", "universe3"));
}

TEST_F(MediaManagerTest, MultipleImagesWithFiltersLoadFail) {
    loadDocument(MULTIPLE_IMAGES_WITH_FILTERS);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe0", "universe1", "universe2", "universe3"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    root->mediaLoadFailed("universe0", 2, "Other error");
    ASSERT_EQ(kMediaStateError, component->getCalculated(kPropertyMediaState).getInteger());
    root->mediaLoaded("universe1");
    ASSERT_EQ(kMediaStateError, component->getCalculated(kPropertyMediaState).getInteger());
}

TEST_F(MediaManagerTest, MultipleImagesWithFiltersLoadFailAfterOneLoad) {
    loadDocument(MULTIPLE_IMAGES_WITH_FILTERS);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe0", "universe1", "universe2", "universe3"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    root->mediaLoaded("universe1");
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    root->mediaLoadFailed("universe0", 2, "Other error");
    ASSERT_EQ(kMediaStateError, component->getCalculated(kPropertyMediaState).getInteger());
    root->mediaLoaded("universe2");
    ASSERT_EQ(kMediaStateError, component->getCalculated(kPropertyMediaState).getInteger());
}

TEST_F(MediaManagerTest, MultipleImagesWithFiltersLoadFailAfterAllLoadedIgnored) {
    loadDocument(MULTIPLE_IMAGES_WITH_FILTERS);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe0", "universe1", "universe2", "universe3"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(component, "universe0", "universe1", "universe2", "universe3"));

    root->mediaLoadFailed("universe0", 2, "Other error");
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

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe0", "universe1"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(component, "universe0", "universe1"));
}

TEST_F(MediaManagerTest, SingleImageUpdate) {
    loadDocument(SINGLE_IMAGE);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(component, "universe"));

    component->setProperty(kPropertySource, "sample");

    ASSERT_TRUE(CheckDirty(component, kPropertySource, kPropertyMediaState, kPropertyVisualHash));

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "sample"));
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
    advanceTime(10);
    root->clearDirty();

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe0", "universe1", "universe2"));
    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe3", "universe4"));
    ASSERT_FALSE(root->hasEvent());

    component->update(kUpdateScrollPosition, 100);
    advanceTime(10);
    root->clearPending();

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe5"));
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
    advanceTime(10);
    root->clearDirty();

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe0"));
    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe1"));
    ASSERT_FALSE(root->hasEvent());

    component->update(kUpdatePagerPosition, 1);
    root->clearPending();

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe2"));
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

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe0", "universe1", "universe2", "universe3"));
    ASSERT_FALSE(root->hasEvent());

    myArray->push_back(4);
    root->clearPending();

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe4"));
    ASSERT_FALSE(root->hasEvent());
}


static const char* VECTOR_GRAPHIC_DOCUMENT = R"(
    {
        "type": "APL",
        "version": "1.5",
        "mainTemplate": {
            "item": {
                "type": "VectorGraphic",
                "source": "http://myPillShape",
                "width": 100,
                "height": 200,
                "scale": "fill",
                "id": "avg"
            }
        }
    }
)";

TEST_F(MediaManagerTest, VectorGraphic) {
    loadDocument(VECTOR_GRAPHIC_DOCUMENT);

    ASSERT_FALSE(root->isDirty());

    // Event should be fired that requests media to be loaded.
    ASSERT_TRUE(MediaRequested(kEventMediaTypeVectorGraphic, "http://myPillShape"));

    ASSERT_TRUE(CheckLoadedMedia(component, "http://myPillShape"));
}

TEST_F(MediaManagerTest, VectorGraphicIgnoresNonRequestedURIs) {
    loadDocument(VECTOR_GRAPHIC_DOCUMENT);

    ASSERT_FALSE(root->isDirty());

    // Event should be fired that requests media to be loaded.
    ASSERT_TRUE(MediaRequested(kEventMediaTypeVectorGraphic, "http://myPillShape"));

    root->mediaLoaded("http://myOtherShape");
    ASSERT_EQ(0, root->getDirty().size());
    ASSERT_TRUE(CheckLoadedMedia(component, "http://myPillShape"));
}

TEST_F(MediaManagerTest, VectorGraphicFailure) {
    loadDocument(VECTOR_GRAPHIC_DOCUMENT);

    ASSERT_FALSE(root->isDirty());

    // Event should be fired that requests media to be loaded.
    ASSERT_TRUE(MediaRequested(kEventMediaTypeVectorGraphic, "http://myPillShape"));

    root->mediaLoadFailed("http://myPillShape", 2, "Other error");

    ASSERT_TRUE(CheckDirty(component, kPropertyMediaState));
    ASSERT_EQ(kMediaStateError, component->getCalculated(kPropertyMediaState).getInteger());
}

TEST_F(MediaManagerTest, VectorGraphicChange)
{
    loadDocument(VECTOR_GRAPHIC_DOCUMENT);

    ASSERT_FALSE(root->isDirty());

    // Let the first one load
    ASSERT_TRUE(MediaRequested(kEventMediaTypeVectorGraphic, "http://myPillShape"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(apl::kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(component, "http://myPillShape"));
    ASSERT_EQ(kMediaStateReady, component->getCalculated(apl::kPropertyMediaState).getInteger());

    // Change the source property
    component->setProperty(kPropertySource, "http://foobar");
    ASSERT_TRUE(MediaRequested(kEventMediaTypeVectorGraphic, "http://foobar"));
    ASSERT_TRUE(CheckDirty(component, kPropertySource, kPropertyMediaState, kPropertyVisualHash));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(apl::kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(component, "http://foobar"));
    ASSERT_EQ(kMediaStateReady, component->getCalculated(apl::kPropertyMediaState).getInteger());
}


static const char* VECTOR_GRAPHIC_LOCAL_SOURCE_DOCUMENT = R"(
    {
        "type": "APL",
        "version": "1.5",
        "mainTemplate": {
            "item": {
                "type": "VectorGraphic",
                "source": "box",
                "width": 100,
                "height": 200,
                "scale": "fill",
                "id": "avg"
            }
        },
        "graphics": {
            "box": {
                "type": "AVG",
                "version": "1.0",
                "height": 100,
                "width": 100,
                "parameters": [
                    {
                        "name": "myColor",
                        "type": "color",
                        "default": "red"
                    }
                ],
                "items": {
                    "type": "path",
                    "pathData": "M0,0 h100 v100 h-100 z",
                    "fill": "${myColor}"
                }
            }
        }
    }
)";

TEST_F(MediaManagerTest, VectorGraphicLocalSource) {
    loadDocument(VECTOR_GRAPHIC_LOCAL_SOURCE_DOCUMENT);

    ASSERT_FALSE(root->isDirty());

    // Event should be fired that requests media to be loaded.
    ASSERT_FALSE(MediaRequested(kEventMediaTypeVectorGraphic, "box"));
}

static const char* MIXED_MEDIA_DOCUMENT = R"(
    {
        "type": "APL",
        "version": "1.5",
        "mainTemplate": {
            "item": {
                "type": "Container",
                "items": [
                    {
                        "type": "VectorGraphic",
                        "source": "http://myAVG",
                        "width": 100,
                        "height": 200,
                        "scale": "fill",
                        "id": "myAVG"
                    },

                    {
                        "type": "Image",
                        "source": "http://myImage",
                        "id": "myImage"
                    }
                ]
            }
        }
    }
)";

TEST_F(MediaManagerTest, MixedMediaRequests) {
    loadDocument(MIXED_MEDIA_DOCUMENT);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "http://myImage"));
    ASSERT_TRUE(MediaRequested(kEventMediaTypeVectorGraphic, "http://myAVG"));

    ASSERT_TRUE(CheckLoadedMedia(root->findComponentById("myImage"), "http://myImage"));
    ASSERT_TRUE(CheckLoadedMedia(root->findComponentById("myAVG"), "http://myAVG"));
}

/**
 * Test using a custom MediaManager.  This one assumes that all media objects are immediately
 * available.
 */
class TestMediaObject : public MediaObject {
public:
    TestMediaObject(std::string url,
                    EventMediaType type, HeaderArray headers = HeaderArray())
        : mURL(std::move(url)), mType(type), mHeaders(std::move(headers)) {}

    std::string url() const override { return mURL; }
    State state() const override { return kReady; }
    const HeaderArray& headers() const override { return mHeaders; }
    EventMediaType type() const override { return mType; }
    Size size() const override { return apl::Size(20,20); }
    CallbackID addCallback(MediaObjectCallback callback) override { return 0; }
    void removeCallback(CallbackID callbackToken) override {}
    int errorCode() const override { return 0; }
    std::string errorDescription() const override { return std::string(); }

    std::string mURL;
    EventMediaType mType;
    HeaderArray mHeaders;
};

class TestManager : public MediaManager {
public:
    MediaObjectPtr request(
        const std::string& url,
        EventMediaType type,
        const HeaderArray& headers) override {
        counter++;
        return std::make_shared<TestMediaObject>(url, type, headers);
    }

    MediaObjectPtr request(const std::string& url, EventMediaType type) override {
        return std::make_shared<TestMediaObject>(url, type, HeaderArray());
    }

    void processMediaRequests(const ContextPtr& context) override {}
    void mediaLoadComplete(const std::string& source,
                           bool isReady = 0,
                           int errorCode = 0,
                           const std::string& errorReason = std::string()) override {}

    int counter = 0;
};

TEST_F(MediaManagerTest, OverrideManager)
{
    auto testManager = std::make_shared<TestManager>();
    config->mediaManager(testManager);

    loadDocument(MIXED_MEDIA_DOCUMENT);
    ASSERT_EQ(2, testManager->counter);

    ASSERT_EQ(kMediaStateReady, root->findComponentById("myImage")->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_EQ(kMediaStateReady, root->findComponentById("myAVG")->getCalculated(kPropertyMediaState).getInteger());

    ASSERT_TRUE(CheckDirty(root));   // Nothing should be dirty because we loaded them immediately
}


/**
 *  Verify that the core media manager correctly releases objects
 */
class C2MediaManager : public CoreMediaManager {
public:
    size_t objectMapSize() const { return mObjectMap.size(); }
    size_t pendingSize() const { return mPending.size(); }
};

static const char* TEMP_DOC = R"(
    {
        "type": "APL",
        "version": "1.6",
        "mainTemplate": {
            "item": {
                "type": "Frame"
            }
        }
    }
)";
TEST_F(MediaManagerTest, CoreMemoryCheck)
{
    auto manager = std::make_shared<C2MediaManager>();
    config->mediaManager(manager);

    loadDocument(TEMP_DOC);

    // STEP #1: Request five objects from the MediaManager
    static std::vector<std::string> URL_LIST = { "test1", "test2", "test3", "test4", "test5" };

    std::vector<MediaObjectPtr> objects;
    std::map<std::string, MediaObject::State> callbackState;

    for (const auto& m : URL_LIST) {
        objects.emplace_back(manager->request(m, kEventMediaTypeImage, HeaderArray()));
        objects.back()->addCallback([&callbackState](const MediaObjectPtr& mediaObject) {
            callbackState[mediaObject->url()] = mediaObject->state();
        });
    }
    ASSERT_EQ(5, objects.size());
    ASSERT_EQ(5, manager->pendingSize());
    ASSERT_EQ(5, manager->objectMapSize());

    // STEP #2: Use the "processMediaRequests" method to remove them from the "pending" queue
    manager->processMediaRequests(context);
    ASSERT_EQ(0, manager->pendingSize());
    ASSERT_EQ(5, manager->objectMapSize());
    ASSERT_TRUE(root->hasEvent());
    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "test1", "test2", "test3", "test4", "test5"));

    // STEP #3: Drop the last two objects.  These are still being processed, but are no longer referenced here
    objects.pop_back();
    objects.pop_back();
    ASSERT_EQ(3, objects.size());
    ASSERT_EQ(0, manager->pendingSize());
    ASSERT_EQ(3, manager->objectMapSize());

    // STEP #4: The media loader loads or fails to load 4 of the 5 objects
    manager->mediaLoadComplete("test1", true);
    manager->mediaLoadComplete("test2", false);
    manager->mediaLoadComplete("test4", false);  // This should be ignored because the object was dropped
    manager->mediaLoadComplete("test5", true);   // This should be ignored because the object was dropped

    ASSERT_EQ(0, manager->pendingSize());
    ASSERT_EQ(3, manager->objectMapSize());

    static std::map<std::string, MediaObject::State> EXPECTED = {
        {"test1", MediaObject::kReady},
        {"test2", MediaObject::kError},
    };
    ASSERT_EQ(EXPECTED, callbackState);

    static std::vector<MediaObject::State> EXPECTED_STATE = {
        MediaObject::kReady, MediaObject::kError, MediaObject::kPending
    };
    for (int i = 0 ; i < EXPECTED_STATE.size() ; i++)
        ASSERT_EQ(EXPECTED_STATE.at(i), objects.at(i)->state());

    // STEP #5: Ask for all five objects again.  Three of them are already known to the media manager
    for (const auto& m : URL_LIST) {
        objects.emplace_back(manager->request(m, kEventMediaTypeImage, HeaderArray()));
        objects.back()->addCallback([&callbackState](const MediaObjectPtr& mediaObject) {
            callbackState[mediaObject->url()] = mediaObject->state();
        });
    }
    ASSERT_EQ(8, objects.size());
    ASSERT_EQ(2, manager->pendingSize());
    ASSERT_EQ(5, manager->objectMapSize());

    // Check the state of all objects
    static std::vector<MediaObject::State> EXPECTED_STATE_2 = {
        MediaObject::kReady,   // "test1"
        MediaObject::kError,   // "test2"
        MediaObject::kPending, // "test3"
        MediaObject::kReady,   // "test1"
        MediaObject::kError,   // "test2"
        MediaObject::kPending, // "test3"
        MediaObject::kPending, // "test4",
        MediaObject::kPending  // "test5"
    };
    for (int i = 0 ; i < EXPECTED_STATE_2.size() ; i++)
        ASSERT_EQ(EXPECTED_STATE_2.at(i), objects.at(i)->state());

    // STEP #6: Use the "processMediaRequests" method to clear out the "pending queue"
    manager->processMediaRequests(context);
    ASSERT_EQ(0, manager->pendingSize());
    ASSERT_EQ(5, manager->objectMapSize());
    ASSERT_TRUE(root->hasEvent());
    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "test4", "test5"));

    // STEP #7: One of the original objects requested finally appears.  Two separate media objects are waiting on it.
    manager->mediaLoadComplete("test3", true);

    static std::map<std::string, MediaObject::State> EXPECTED_2 = {
        {"test1", MediaObject::kReady},
        {"test2", MediaObject::kError},
        {"test3", MediaObject::kReady},
    };
    ASSERT_EQ(EXPECTED_2, callbackState);

    static std::vector<MediaObject::State> EXPECTED_STATE_3 = {
        MediaObject::kReady,   // "test1"
        MediaObject::kError,   // "test2"
        MediaObject::kReady,   // "test3"
        MediaObject::kReady,   // "test1"
        MediaObject::kError,   // "test2"
        MediaObject::kReady,   // "test3"
        MediaObject::kPending, // "test4",
        MediaObject::kPending  // "test5"
    };
    for (int i = 0 ; i < EXPECTED_STATE_2.size() ; i++)
        ASSERT_EQ(EXPECTED_STATE_3.at(i), objects.at(i)->state());
}

static const char* NO_IMAGE_FAIL_LOAD = R"({
    "type": "APL",
    "version": "1.7",
    "mainTemplate": {
        "items": {
            "type": "Container",
            "items": [
                {
                    "type": "Image",
                    "id": "myImage",
                    "source": [],
                    "onLoad": {
                        "type": "SetValue",
                        "componentId": "textComp",
                        "property": "text",
                        "value": "tango"
                    },
                    "onFail": {
                        "type": "SetValue",
                        "componentId": "textComp",
                        "property": "text",
                        "value": "bravo"
                    }
                },
                {
                    "type": "Text",
                    "id": "textComp",
                    "text": "tiger"
                }
            ]
        }
    }
})";
TEST_F(MediaManagerTest, NoSourceImageNoLoad)
{
    loadDocument(NO_IMAGE_FAIL_LOAD);

    ASSERT_FALSE(root->isDirty());

    auto textComponent = root->findComponentById("textComp");
    ASSERT_TRUE(textComponent);
    ASSERT_EQ(kComponentTypeText, textComponent->getType());
    ASSERT_EQ("tiger", textComponent->getCalculated(kPropertyText).asString());
}

static const char* SINGLE_IMAGE_ON_LOAD = R"({
    "type": "APL",
    "version": "1.7",
    "mainTemplate": {
        "items": {
            "type": "Container",
            "items": [
                {
                    "type": "Image",
                    "id": "myImage",
                    "source": "universe",
                    "onLoad": {
                        "type": "SetValue",
                        "componentId": "textComp",
                        "property": "text",
                        "value": "tango"
                    },
                    "onFail": {
                        "type": "SetValue",
                        "componentId": "textComp",
                        "property": "text",
                        "value": "bravo"
                    }
                },
                {
                    "type": "Text",
                    "id": "textComp",
                    "text": "tiger"
                }
            ]
        }
    }
})";
TEST_F(MediaManagerTest, SingleImageLoad)
{
    loadDocument(SINGLE_IMAGE_ON_LOAD);

    ASSERT_FALSE(root->isDirty());

    auto textComponent = root->findComponentById("textComp");
    ASSERT_TRUE(textComponent);
    ASSERT_EQ(kComponentTypeText, textComponent->getType());
    ASSERT_EQ("tiger", textComponent->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe"));
    ASSERT_TRUE(CheckLoadedMedia(root->findComponentById("myImage"), "universe"));

    ASSERT_EQ("tango", textComponent->getCalculated(kPropertyText).asString());
}

TEST_F(MediaManagerTest, SingleImageFail)
{
    loadDocument(SINGLE_IMAGE_ON_LOAD);

    ASSERT_FALSE(root->isDirty());

    auto textComponent = root->findComponentById("textComp");
    ASSERT_TRUE(textComponent);
    ASSERT_EQ(kComponentTypeText, textComponent->getType());
    ASSERT_EQ("tiger", textComponent->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe"));
    root->mediaLoadFailed("universe", 2, "Other error");

    ASSERT_EQ("bravo", textComponent->getCalculated(kPropertyText).asString());
}

TEST_F(MediaManagerTest, SingleImageLoadChangeSourceTriggersOnLoad)
{
    loadDocument(SINGLE_IMAGE_ON_LOAD);

    ASSERT_FALSE(root->isDirty());

    auto textComponent = root->findComponentById("textComp");
    ASSERT_TRUE(textComponent);
    ASSERT_EQ(kComponentTypeText, textComponent->getType());
    ASSERT_EQ("tiger", textComponent->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe"));
    root->mediaLoadFailed("universe", 2, "Other error");

    ASSERT_EQ("bravo", textComponent->getCalculated(kPropertyText).asString());

    auto textComponentMedia = TextComponent::cast(textComponent);
    textComponentMedia->setProperty(kPropertyText, "torpedo");

    ASSERT_EQ("torpedo", textComponent->getCalculated(kPropertyText).asString());

    auto imageComponent = ImageComponent::cast(root->findComponentById("myImage"));
    imageComponent->setProperty(kPropertySource, "universe1");
    ASSERT_TRUE(CheckDirty(imageComponent, kPropertySource, kPropertyMediaState, kPropertyVisualHash));
    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe1"));
    ASSERT_EQ(kMediaStatePending, imageComponent->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(imageComponent, "universe1"));

    ASSERT_EQ("tango", textComponent->getCalculated(kPropertyText).asString());
}

static const char* MULTIPLE_IMAGES_ON_LOAD_ON_FAIL_NO_FILTERS = R"({
    "type": "APL",
    "version": "1.7",
    "mainTemplate": {
        "items": {
            "type": "Container",
            "items": [
                {
                    "type": "Image",
                    "id": "myImage",
                    "sources": ["universe0", "universe1"],
                    "onLoad": {
                        "type": "SetValue",
                        "componentId": "textComp",
                        "property": "text",
                        "value": "tango"
                    },
                    "onFail": {
                        "type": "SetValue",
                        "componentId": "textComp",
                        "property": "text",
                        "value": "bravo"
                    }
                },
                {
                    "type": "Text",
                    "id": "textComp",
                    "text": "tiger"
                }
            ]
        }
    }
})";

TEST_F(MediaManagerTest, MultipleImagesNoFilterLoad)
{
    loadDocument(MULTIPLE_IMAGES_ON_LOAD_ON_FAIL_NO_FILTERS);

    ASSERT_FALSE(root->isDirty());

    auto textComponent = root->findComponentById("textComp");
    ASSERT_TRUE(textComponent);
    ASSERT_EQ(kComponentTypeText, textComponent->getType());
    ASSERT_EQ("tiger", textComponent->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe1"));
    ASSERT_FALSE(MediaRequested(kEventMediaTypeImage, "universe0"));
    ASSERT_TRUE(CheckLoadedMedia(root->findComponentById("myImage"), "universe1"));
    ASSERT_FALSE(CheckLoadedMedia(root->findComponentById("myImage"), "universe0"));

    ASSERT_EQ("tango", textComponent->getCalculated(kPropertyText).asString());
}

TEST_F(MediaManagerTest, MultipleImagesNoFilterOnFail)
{
    loadDocument(MULTIPLE_IMAGES_ON_LOAD_ON_FAIL_NO_FILTERS);

    ASSERT_FALSE(root->isDirty());

    auto textComponent = root->findComponentById("textComp");
    ASSERT_TRUE(textComponent);
    ASSERT_EQ(kComponentTypeText, textComponent->getType());
    ASSERT_EQ("tiger", textComponent->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe1"));
    ASSERT_FALSE(MediaRequested(kEventMediaTypeImage, "universe0"));
    root->mediaLoadFailed("universe1", 2, "Other error");

    ASSERT_EQ("bravo", textComponent->getCalculated(kPropertyText).asString());
}

static const char* MULTIPLE_IMAGES_ON_FAIL_ON_LOAD_FILTERS = R"({
    "type": "APL",
    "version": "1.7",
    "mainTemplate": {
        "items": {
            "type": "Container",
            "items": [
                {
                    "type": "Image",
                    "id": "myImage",
                    "sources": ["universe0", "universe1", "universe2", "universe3"],
                    "filters": {
                        "type": "Blend",
                        "mode": "normal"
                    },
                    "onLoad": {
                        "type": "SetValue",
                        "componentId": "textComp",
                        "property": "text",
                        "value": "tango"
                    },
                    "onFail": {
                        "type": "SetValue",
                        "componentId": "textComp",
                        "property": "text",
                        "value": "bravo"
                    }
                },
                {
                    "type": "Text",
                    "id": "textComp",
                    "text": "tiger"
                }
            ]
        }
    }
})";

TEST_F(MediaManagerTest, MultipleImagesFilterOnLoad)
{
    loadDocument(MULTIPLE_IMAGES_ON_FAIL_ON_LOAD_FILTERS);

    ASSERT_FALSE(root->isDirty());

    auto textComponent = root->findComponentById("textComp");
    ASSERT_TRUE(textComponent);
    ASSERT_EQ(kComponentTypeText, textComponent->getType());
    ASSERT_EQ("tiger", textComponent->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe0", "universe1", "universe2", "universe3"));
    ASSERT_TRUE(CheckLoadedMedia(root->findComponentById("myImage"), "universe0", "universe1", "universe2", "universe3"));

    ASSERT_EQ("tango", textComponent->getCalculated(kPropertyText).asString());
}

TEST_F(MediaManagerTest, MultipleImagesFilterPartialOnLoad)
{
    loadDocument(MULTIPLE_IMAGES_ON_FAIL_ON_LOAD_FILTERS);

    ASSERT_FALSE(root->isDirty());

    auto textComponent = root->findComponentById("textComp");
    ASSERT_TRUE(textComponent);
    ASSERT_EQ(kComponentTypeText, textComponent->getType());
    ASSERT_EQ("tiger", textComponent->getCalculated(kPropertyText).asString());

    auto imageComponent = root->findComponentById("myImage");
    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe0", "universe1", "universe2", "universe3"));
    ASSERT_EQ(kMediaStatePending, imageComponent->getCalculated(kPropertyMediaState).getInteger());
    root->mediaLoaded("universe0");
    ASSERT_EQ("tiger", textComponent->getCalculated(kPropertyText).asString());
    ASSERT_EQ(kMediaStatePending, imageComponent->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(imageComponent, "universe1", "universe2", "universe3"));

    ASSERT_EQ("tango", textComponent->getCalculated(kPropertyText).asString());
}

TEST_F(MediaManagerTest, MultipleImagesFilterOnFail)
{
    loadDocument(MULTIPLE_IMAGES_ON_FAIL_ON_LOAD_FILTERS);

    ASSERT_FALSE(root->isDirty());

    auto textComponent = root->findComponentById("textComp");
    ASSERT_TRUE(textComponent);
    ASSERT_EQ(kComponentTypeText, textComponent->getType());
    ASSERT_EQ("tiger", textComponent->getCalculated(kPropertyText).asString());

    auto imageComponent = root->findComponentById("myImage");
    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe0", "universe1", "universe2", "universe3"));
    ASSERT_EQ(kMediaStatePending, imageComponent->getCalculated(kPropertyMediaState).getInteger());
    root->mediaLoadFailed("universe0", 2, "Other error");
    ASSERT_EQ(kMediaStateError, imageComponent->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_EQ("bravo", textComponent->getCalculated(kPropertyText).asString());
}

static const char* MULTIPLE_IMAGES_ON_LOAD_ON_FAIL_NO_FILTERS_SET_VALUE = R"({
    "type": "APL",
    "version": "1.7",
    "mainTemplate": {
        "items": {
            "type": "Container",
            "items": [
                {
                    "type": "Image",
                    "id": "myImage",
                    "sources": ["universe0", "universe1", "universe2"],
                    "onLoad": {
                        "type": "SetValue",
                        "componentId": "textComp",
                        "property": "text",
                        "value": "tango"
                    },
                    "onFail": {
                        "type": "SetValue",
                        "componentId": "textComp",
                        "property": "text",
                        "value": "${event.value}"
                    }
                },
                {
                    "type": "Text",
                    "id": "textComp",
                    "text": "tiger"
                }
            ]
        }
    }
})";

TEST_F(MediaManagerTest, MultipleImagesNoFilterReadyFailSetValue)
{
    loadDocument(MULTIPLE_IMAGES_ON_LOAD_ON_FAIL_NO_FILTERS_SET_VALUE);

    ASSERT_FALSE(root->isDirty());

    auto textComponent = root->findComponentById("textComp");
    ASSERT_TRUE(textComponent);
    ASSERT_EQ(kComponentTypeText, textComponent->getType());
    ASSERT_EQ("tiger", textComponent->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe2"));
    ASSERT_FALSE(MediaRequested(kEventMediaTypeImage, "universe1"));
    ASSERT_FALSE(MediaRequested(kEventMediaTypeImage, "universe0"));
    root->mediaLoadFailed("universe1", 2, "Other error");
    root->mediaLoadFailed("universe2", 2, "Other error");

    ASSERT_EQ("universe2", textComponent->getCalculated(kPropertyText).asString());
}

static const char* MULTIPLE_IMAGES_ON_LOAD_ON_FAIL_ERROR_MESSAGE = R"({
    "type": "APL",
    "version": "1.7",
    "mainTemplate": {
        "items": {
            "type": "Container",
            "items": [
                {
                    "type": "Image",
                    "id": "myImage",
                    "sources": ["universe0", "universe1", "universe2"],
                    "onLoad": {
                        "type": "SetValue",
                        "componentId": "textComp",
                        "property": "text",
                        "value": "tango"
                    },
                    "onFail": {
                        "type": "SetValue",
                        "componentId": "textComp",
                        "property": "text",
                        "value": "${event.error}"
                    }
                },
                {
                    "type": "Text",
                    "id": "textComp",
                    "text": "tiger"
                }
            ]
        }
    }
})";

TEST_F(MediaManagerTest, MultipleImagesNoFilterReadyFailSetErrorMessage)
{
    loadDocument(MULTIPLE_IMAGES_ON_LOAD_ON_FAIL_ERROR_MESSAGE);

    ASSERT_FALSE(root->isDirty());

    auto textComponent = root->findComponentById("textComp");
    ASSERT_TRUE(textComponent);
    ASSERT_EQ(kComponentTypeText, textComponent->getType());
    ASSERT_EQ("tiger", textComponent->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe2"));
    ASSERT_FALSE(MediaRequested(kEventMediaTypeImage, "universe1"));
    ASSERT_FALSE(MediaRequested(kEventMediaTypeImage, "universe0"));
    root->mediaLoadFailed("universe1", 2, "Other error");
    root->mediaLoadFailed("universe2", 3, "Not found");

    ASSERT_EQ("Not found", textComponent->getCalculated(kPropertyText).asString());
}

static const char* MULTIPLE_IMAGES_ON_LOAD_ON_FAIL_ERROR_CODE = R"({
    "type": "APL",
    "version": "1.7",
    "mainTemplate": {
        "items": {
            "type": "Container",
            "items": [
                {
                    "type": "Image",
                    "id": "myImage",
                    "sources": ["universe0", "universe1", "universe2"],
                    "onLoad": {
                        "type": "SetValue",
                        "componentId": "textComp",
                        "property": "text",
                        "value": "tango"
                    },
                    "onFail": {
                        "type": "SetValue",
                        "componentId": "textComp",
                        "property": "text",
                        "value": "${event.errorCode}"
                    }
                },
                {
                    "type": "Text",
                    "id": "textComp",
                    "text": "tiger"
                }
            ]
        }
    }
})";

TEST_F(MediaManagerTest, MultipleImagesNoFilterReadyFailSetErrorCode)
{
    loadDocument(MULTIPLE_IMAGES_ON_LOAD_ON_FAIL_ERROR_CODE);

    ASSERT_FALSE(root->isDirty());

    auto textComponent = root->findComponentById("textComp");
    ASSERT_TRUE(textComponent);
    ASSERT_EQ(kComponentTypeText, textComponent->getType());
    ASSERT_EQ("tiger", textComponent->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe2"));
    ASSERT_FALSE(MediaRequested(kEventMediaTypeImage, "universe1"));
    ASSERT_FALSE(MediaRequested(kEventMediaTypeImage, "universe0"));
    root->mediaLoadFailed("universe1", 2, "Other error");
    root->mediaLoadFailed("universe2", 3, "Not found");

    ASSERT_EQ("3", textComponent->getCalculated(kPropertyText).asString());
}

static const char* MULTIPLE_IMAGES_ON_LOAD_ON_FAIL_FILTERS_SET_VALUE = R"({
    "type": "APL",
    "version": "1.7",
    "mainTemplate": {
        "items": {
            "type": "Container",
            "items": [
                {
                    "type": "Image",
                    "id": "myImage",
                    "sources": ["universe0", "universe1", "universe2"],
                    "filters": {
                        "type": "Blend",
                        "mode": "normal"
                    },
                    "onLoad": {
                        "type": "SetValue",
                        "componentId": "textComp",
                        "property": "text",
                        "value": "tango"
                    },
                    "onFail": {
                        "type": "SetValue",
                        "componentId": "textComp",
                        "property": "text",
                        "value": "${event.value}"
                    }
                },
                {
                    "type": "Text",
                    "id": "textComp",
                    "text": "tiger"
                }
            ]
        }
    }
})";

TEST_F(MediaManagerTest, MultipleImagesFilterReadyFailSetValue)
{
    loadDocument(MULTIPLE_IMAGES_ON_LOAD_ON_FAIL_FILTERS_SET_VALUE);

    ASSERT_FALSE(root->isDirty());

    auto textComponent = root->findComponentById("textComp");
    ASSERT_TRUE(textComponent);
    ASSERT_EQ(kComponentTypeText, textComponent->getType());
    ASSERT_EQ("tiger", textComponent->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe0", "universe1", "universe2"));
    root->mediaLoadFailed("universe1", 2, "Other error");
    root->mediaLoadFailed("universe2", 2, "Other error");

    // We call it on the first one unless we change the sources
    ASSERT_EQ("universe1", textComponent->getCalculated(kPropertyText).asString());
}

TEST_F(MediaManagerTest, MultipleImagesFilterReadySetValueSetSource)
{
    loadDocument(MULTIPLE_IMAGES_ON_LOAD_ON_FAIL_FILTERS_SET_VALUE);

    ASSERT_FALSE(root->isDirty());

    auto textComponent = root->findComponentById("textComp");
    ASSERT_TRUE(textComponent);
    ASSERT_EQ(kComponentTypeText, textComponent->getType());
    ASSERT_EQ("tiger", textComponent->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe0", "universe1", "universe2"));
    root->mediaLoadFailed("universe1", 2, "Other error");

    ASSERT_EQ("universe1", textComponent->getCalculated(kPropertyText).asString());
    auto imageComponent = std::static_pointer_cast<ImageComponent>(root->findComponentById("myImage"));

    auto newSources = Object(ObjectArray{"universe3", "universe1", "universe2"});
    imageComponent->setProperty(kPropertySource, newSources);

    ASSERT_TRUE(CheckDirty(imageComponent, kPropertySource, kPropertyMediaState, kPropertyVisualHash));

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe3", "universe1", "universe2"));
    ASSERT_TRUE(CheckLoadedMedia(imageComponent, "universe3", "universe1", "universe2"));

    ASSERT_EQ("tango", textComponent->getCalculated(kPropertyText).asString());
}

TEST_F(MediaManagerTest, MultipleImagesFilterReadyFailSetValueSetSource)
{
    loadDocument(MULTIPLE_IMAGES_ON_LOAD_ON_FAIL_FILTERS_SET_VALUE);

    ASSERT_FALSE(root->isDirty());

    auto textComponent = root->findComponentById("textComp");
    ASSERT_TRUE(textComponent);
    ASSERT_EQ(kComponentTypeText, textComponent->getType());
    ASSERT_EQ("tiger", textComponent->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe0", "universe1", "universe2"));
    root->mediaLoadFailed("universe1", 2, "Other error");

    ASSERT_EQ("universe1", textComponent->getCalculated(kPropertyText).asString());
    auto imageComponent = std::static_pointer_cast<ImageComponent>(root->findComponentById("myImage"));

    auto newSources = Object(ObjectArray{"universe3", "universe1", "universe2"});
    imageComponent->setProperty(kPropertySource, newSources);

    ASSERT_TRUE(CheckDirty(imageComponent, kPropertySource, kPropertyMediaState, kPropertyVisualHash));

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe3", "universe1", "universe2"));
    root->mediaLoadFailed("universe3", 2, "Other error");

    ASSERT_EQ("universe3", textComponent->getCalculated(kPropertyText).asString());
}

static const char* VECTOR_GRAPHIC_ON_LOAD_ON_FAIL = R"apl(
{
    "type": "APL",
    "version": "1.7",
    "graphics": {
        "MyIcon": {
          "type": "AVG",
          "version": "1.0",
          "height": 100,
          "width": 100,
          "items": {
            "type": "path",
            "pathData": "M0,0 h100 v100 h-100 z",
            "fill": "red"
          }
        }
      },
    "mainTemplate": {
        "items": {
            "type": "Container",
            "items": [
                {
                    "type": "VectorGraphic",
                    "source": "myIcon",
                    "width": "100%",
                    "height": "100%",
                    "scale": "best-fit",
                    "align": "center",
                    "onLoad": {
                        "type": "SetValue",
                        "componentId": "textComp",
                        "property": "text",
                        "value": "tango"
                    },
                    "onFail": {
                        "type": "SetValue",
                        "componentId": "textComp",
                        "property": "text",
                        "value": "bravo"
                    }
                },
                {
                    "type": "Text",
                    "id": "textComp",
                    "text": "tiger"
                }
            ]
        }
    }
}
)apl";

TEST_F(MediaManagerTest, VectorGraphicOnLoadSuccess)
{
    loadDocument(VECTOR_GRAPHIC_ON_LOAD_ON_FAIL);

    ASSERT_FALSE(root->isDirty());

    auto textComponent = root->findComponentById("textComp");
    ASSERT_TRUE(textComponent);
    ASSERT_EQ(kComponentTypeText, textComponent->getType());
    ASSERT_EQ("tiger", textComponent->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeVectorGraphic, "myIcon"));

    root->mediaLoaded("myIcon");
    ASSERT_EQ("tango", textComponent->getCalculated(kPropertyText).asString());
}

TEST_F(MediaManagerTest, VectorGraphicOnFailFailure)
{
    loadDocument(VECTOR_GRAPHIC_ON_LOAD_ON_FAIL);

    ASSERT_FALSE(root->isDirty());

    auto textComponent = root->findComponentById("textComp");
    ASSERT_TRUE(textComponent);
    ASSERT_EQ(kComponentTypeText, textComponent->getType());
    ASSERT_EQ("tiger", textComponent->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeVectorGraphic, "myIcon"));

    root->mediaLoadFailed("myIcon", 2, "Other error");
    ASSERT_EQ("bravo", textComponent->getCalculated(kPropertyText).asString());
}


static const char* VECTOR_GRAPHIC_ON_FAIL_ONCE = R"apl(
{
    "type": "APL",
    "version": "1.7",
    "graphics": {
        "MyIcon": {
          "type": "AVG",
          "version": "1.0",
          "height": 100,
          "width": 100,
          "items": {
            "type": "path",
            "pathData": "M0,0 h100 v100 h-100 z",
            "fill": "red"
          }
        }
      },
    "mainTemplate": {
        "items": {
            "type": "Container",
            "items": [
                {
                    "type": "VectorGraphic",
                    "source": "myIcon",
                    "width": "100%",
                    "height": "100%",
                    "scale": "best-fit",
                    "align": "center",
                    "onLoad": {
                        "type": "SetValue",
                        "componentId": "textComp",
                        "property": "text",
                        "value": "tango"
                    },
                    "onFail": {
                        "type": "SetValue",
                        "componentId": "textComp",
                        "property": "text",
                        "value": "${event.error}"
                    }
                },
                {
                    "type": "Text",
                    "id": "textComp",
                    "text": "tiger"
                }
            ]
        }
    }
}
)apl";

TEST_F(MediaManagerTest, VectorGraphicMultipleFailuresOnlyOneIsReported)
{
    loadDocument(VECTOR_GRAPHIC_ON_FAIL_ONCE);

    ASSERT_FALSE(root->isDirty());

    auto textComponent = root->findComponentById("textComp");
    ASSERT_TRUE(textComponent);
    ASSERT_EQ(kComponentTypeText, textComponent->getType());
    ASSERT_EQ("tiger", textComponent->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeVectorGraphic, "myIcon"));

    root->mediaLoadFailed("myIcon", 2, "Other error");
    root->mediaLoadFailed("myIcon", 3, "Tornado");
    ASSERT_EQ("Other error", textComponent->getCalculated(kPropertyText).asString());
}

static const char* SINGLE_IMAGE_ONLOAD_REINFLATE = R"({
    "type": "APL",
    "version": "1.8",
    "onConfigChange": {
      "type": "Reinflate"
    },
    "mainTemplate": {
        "items": {
            "type": "Image",
            "id": "myImage",
            "sources": ["${viewport.width > viewport.height ? 'source0' : 'source1'}"],
            "onLoad": {
                "type": "SendEvent",
                "sequencer": "SENDER",
                "arguments": ["${viewport.width > viewport.height ? 'loaded0' : 'loaded1'}"]
            },
            "onFail": {
                "type": "SendEvent",
                "sequencer": "SENDER",
                "arguments": ["${viewport.width > viewport.height ? 'failed0' : 'failed1'}"]
            }
        }
    }
})";

TEST_F(MediaManagerTest, SingleImageOnLoadReinflate)
{
    metrics.size(1000,500);
    loadDocument(SINGLE_IMAGE_ONLOAD_REINFLATE);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "source0"));
    advanceTime(100);

    root->mediaLoaded("source0");
    advanceTime(100);

    ASSERT_TRUE(CheckSendEvent(root, "loaded0"));
    advanceTime(100);


    configChangeReinflate(ConfigurationChange(500, 1000));

    component = CoreComponent::cast(root->topComponent());
    ASSERT_TRUE(component);

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "source1"));
    advanceTime(100);

    root->mediaLoaded("source1");
    advanceTime(100);

    ASSERT_TRUE(CheckSendEvent(root, "loaded1"));
}

static const char* SINGLE_IMAGE_REINFLATE = R"({
    "type": "APL",
    "version": "1.8",
    "onConfigChange": {
      "type": "Reinflate"
    },
    "mainTemplate": {
        "items": {
            "type": "Image",
            "id": "myImage",
            "sources": ["${viewport.width > viewport.height ? 'source0' : 'source1'}"]
        }
    }
})";

TEST_F(MediaManagerTest, SingleImageReinflate)
{
    metrics.size(1000,500);
    loadDocument(SINGLE_IMAGE_REINFLATE);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "source0"));
    advanceTime(100);

    root->mediaLoaded("source0");
    advanceTime(100);

    configChangeReinflate(ConfigurationChange(500, 1000));

    component = CoreComponent::cast(root->topComponent());
    ASSERT_TRUE(component);

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "source1"));
    advanceTime(100);

    root->mediaLoaded("source1");
    advanceTime(100);
}

static const char* SINGLE_IMAGE_ONLOAD_REINFLATE_SAME = R"({
    "type": "APL",
    "version": "1.8",
    "onConfigChange": {
      "type": "Reinflate"
    },
    "mainTemplate": {
        "items": {
            "type": "Image",
            "id": "myImage",
            "sources": ["source0"],
            "onLoad": {
                "type": "SendEvent",
                "sequencer": "SENDER",
                "arguments": ["loaded0"]
            },
            "onFail": {
                "type": "SendEvent",
                "sequencer": "SENDER",
                "arguments": ["failed0"]
            }
        }
    }
})";

TEST_F(MediaManagerTest, SingleImageOnLoadReinflateSame)
{
    metrics.size(1000,500);
    loadDocument(SINGLE_IMAGE_ONLOAD_REINFLATE_SAME);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "source0"));
    advanceTime(100);

    root->mediaLoaded("source0");
    advanceTime(100);

    ASSERT_TRUE(CheckSendEvent(root, "loaded0"));
    advanceTime(100);


    configChangeReinflate(ConfigurationChange(500, 1000));

    component = CoreComponent::cast(root->topComponent());
    ASSERT_TRUE(component);

    ASSERT_TRUE(CheckSendEvent(root, "loaded0"));

    // Try to load what was loaded
    root->mediaLoaded("source0");
    advanceTime(100);
}

static const char* SINGLE_IMAGE_REINFLATE_SAME = R"({
  "type": "APL",
  "version": "1.8",
  "onConfigChange": {
    "type": "Reinflate"
  },
  "mainTemplate": {
    "items": {
      "type": "Container",
      "items": {
        "type": "Image",
        "sources": ["source0"]
      }
    }
  }
})";

TEST_F(MediaManagerTest, SingleImageReinflateSame)
{
    metrics.size(1000,500);
    loadDocument(SINGLE_IMAGE_REINFLATE_SAME);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "source0"));
    advanceTime(100);

    root->mediaLoaded("source0");
    advanceTime(100);

    configChangeReinflate(ConfigurationChange(500, 1000));

    component = CoreComponent::cast(root->topComponent());
    ASSERT_TRUE(component);

    root->mediaLoaded("source0");
    advanceTime(100);
}

TEST_F(MediaManagerTest, SingleImageReinflateSameHoldComponent)
{
    metrics.size(1000,500);
    loadDocument(SINGLE_IMAGE_REINFLATE_SAME);
    ASSERT_TRUE(IsEqual("initial", evaluate(*context, "${environment.reason}")));

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "source0"));
    advanceTime(100);

    root->mediaLoaded("source0");
    advanceTime(100);

    auto image = component->getCoreChildAt(0);

    configChangeReinflate(ConfigurationChange(500, 1000));

    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual("reinflation", evaluate(*context, "${environment.reason}")));

    root->mediaLoaded("source0");
    advanceTime(100);
}

TEST_F(MediaManagerTest, SingleImageReinflateSameNotLoadedFirstHoldComponent)
{
    metrics.size(1000,500);
    loadDocument(SINGLE_IMAGE_REINFLATE_SAME);
    ASSERT_TRUE(IsEqual("initial", evaluate(*context, "${environment.reason}")));

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "source0"));
    advanceTime(100);

    auto image = component->getCoreChildAt(0);

    configChangeReinflate(ConfigurationChange(500, 1000));

    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual("reinflation", evaluate(*context, "${environment.reason}")));

    root->mediaLoaded("source0");
    advanceTime(100);
}

static const char* IMAGE_SOURCES_ARRAY_WITH_HEADERS = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Image",
      "sources": [
        {
          "url": "universe0",
          "description": "milky way",
          "headers": [
              "A: header"
          ]
        }
      ]
    }
  }
})";

TEST_F(MediaManagerTest, ImageWithSourcesAsArraryWithHeaders) {
    loadDocument(IMAGE_SOURCES_ARRAY_WITH_HEADERS);

    ASSERT_FALSE(root->isDirty());

    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe0"));
    ASSERT_TRUE(CheckLoadedMedia(component, "universe0"));

    auto sources = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(sources.size(), 1);
    auto asSource = sources.at(0).get<URLRequest>();
    auto headers = asSource.getHeaders();
    ASSERT_EQ(headers.size(), 1);
    ASSERT_EQ(headers.at(0), "A: header");
}

static const char* IMAGE_SOURCES_ARRAY_WITH_HEADERS_AS_STRING = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Image",
      "sources": [
        {
          "url": "universe0",
          "description": "milky way",
          "headers": "A: Let me in"
        }
      ]
    }
  }
})";

TEST_F(MediaManagerTest, ImageWithSourcesAsArraryWithHeadersAsString) {
    loadDocument(IMAGE_SOURCES_ARRAY_WITH_HEADERS_AS_STRING);

    ASSERT_FALSE(root->isDirty());

    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe0"));
    ASSERT_TRUE(CheckLoadedMedia(component, "universe0"));

    auto sources = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(sources.size(), 1);
    auto asSource = sources.at(0).get<URLRequest>();
    auto headers = asSource.getHeaders();
    ASSERT_EQ(headers.size(), 1);
    ASSERT_EQ(headers.at(0), "A: Let me in");
}

static const char* IMAGE_SOURCES_OBJECT_WITH_HEADERS = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Image",
      "sources":
        {
          "url": "universe0",
          "description": "Andromeda",
          "headers": [
              "A: Let me in please"
          ]
        }
    }
  }
})";

TEST_F(MediaManagerTest, ImageSourceAsObjectWithHeaders) {
    loadDocument(IMAGE_SOURCES_OBJECT_WITH_HEADERS);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe0"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(component, "universe0"));

    // Based on the spec, we will "array-fy" the property
    auto sources = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(sources.size(), 1);
    auto asSource = sources.at(0).get<URLRequest>();
    auto headers = asSource.getHeaders();
    ASSERT_EQ(headers.size(), 1);
    ASSERT_EQ(headers.at(0), "A: Let me in please");
}

static const char* IMAGE_SOURCES_ARRAY_WITH_MULTIPLE_IMAGES_AND_HEADERS = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Image",
      "sources": [
        {
          "url": "universe0",
          "description": "milky way",
          "headers": [
              "A: Let me in"
          ]
        },
        "universe1",
        {
          "url": "universe2"
        },
        {
          "url": "universe3",
          "description": "IC 1101",
          "headers": [
              "A: Did you know?",
              "B: I'm the largest galaxy!"
          ]
        }
      ]
    }
  }
})";

TEST_F(MediaManagerTest, MultipleImagesWithHeaders) {
    loadDocument(IMAGE_SOURCES_ARRAY_WITH_MULTIPLE_IMAGES_AND_HEADERS);

    ASSERT_FALSE(root->isDirty());

    // No filters, we only load the last source
    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe3"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(component, "universe3"));

    // Check each of the sources
    auto sources = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(sources.size(), 4);

    ASSERT_TRUE(sources.at(0).is<URLRequest>());
    auto asSource0 = sources.at(0).get<URLRequest>();
    ASSERT_EQ(asSource0.getUrl(), "universe0");
    auto headers0 = asSource0.getHeaders();
    ASSERT_EQ(headers0.size(), 1);
    ASSERT_EQ(headers0.at(0), "A: Let me in");

    ASSERT_TRUE(sources.at(1).isString());
    auto asSource1 = sources.at(1).getString();
    ASSERT_EQ(asSource1, "universe1");

    auto asSource2 = sources.at(2).get<URLRequest>();
    ASSERT_EQ(asSource2.getUrl(), "universe2");
    auto headers2 = asSource2.getHeaders();
    ASSERT_EQ(headers2.size(), 0);

    auto asSource3 = sources.at(3).get<URLRequest>();
    ASSERT_EQ(asSource3.getUrl(), "universe3");
    auto headers3 = asSource3.getHeaders();
    ASSERT_EQ(headers3.size(), 2);
    ASSERT_EQ(headers3.at(0), "A: Did you know?");
    ASSERT_EQ(headers3.at(1), "B: I'm the largest galaxy!");
}

static const char* MULTIPLE_IMAGES_WITH_FILTERS_AND_HEADERS = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Image",
      "sources": [
        {
          "url": "universe0",
          "description": "milky way",
          "headers": [
              "A: Let me in"
          ]
        },
        "universe1",
        {
          "url": "universe2"
        },
        {
          "url": "universe3",
          "description": "IC 1101",
          "headers": [
              "A: Did you know?",
              "B: I'm the largest galaxy!"
          ]
        }
      ],
      "filters": {
        "type": "Blend",
        "mode": "normal"
      }
    }
  }
})";

TEST_F(MediaManagerTest, MultipleImagesWithFiltersAndHeaders) {
    loadDocument(MULTIPLE_IMAGES_WITH_FILTERS_AND_HEADERS);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe0", "universe1", "universe2", "universe3"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(component, "universe0", "universe1", "universe2", "universe3"));

    // Check each of the sources
    auto sources = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(sources.size(), 4);

    ASSERT_TRUE(sources.at(0).is<URLRequest>());
    auto asSoource0 = sources.at(0).get<URLRequest>();
    ASSERT_EQ(asSoource0.getUrl(), "universe0");
    auto headers0 = asSoource0.getHeaders();
    ASSERT_EQ(headers0.size(), 1);
    ASSERT_EQ(headers0.at(0), "A: Let me in");

    ASSERT_TRUE(sources.at(1).isString());
    auto asSource1 = sources.at(1).getString();
    ASSERT_EQ(asSource1, "universe1");

    auto asSource2 = sources.at(2).get<URLRequest>();
    ASSERT_EQ(asSource2.getUrl(), "universe2");
    auto headers2 = asSource2.getHeaders();
    ASSERT_EQ(headers2.size(), 0);

    auto asSource3 = sources.at(3).get<URLRequest>();
    ASSERT_EQ(asSource3.getUrl(), "universe3");
    auto headers3 = asSource3.getHeaders();
    ASSERT_EQ(headers3.size(), 2);
    ASSERT_EQ(headers3.at(0), "A: Did you know?");
    ASSERT_EQ(headers3.at(1), "B: I'm the largest galaxy!");
}

static const char* VECTOR_GRAPHIC_MEDIA_OBJECT = R"({
    "type": "APL",
    "version": "1.6",
    "mainTemplate": {
        "item": {
            "type": "VectorGraphic",
            "source": {
                "url": "universe0",
                "description": "milky way",
                "headers": [
                    "A: Let me in"
                ]
            }
        }
    }
})";

TEST_F(MediaManagerTest, VectorGraphicWithHeaders) {
    loadDocument(VECTOR_GRAPHIC_MEDIA_OBJECT);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeVectorGraphic, "universe0"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(component, "universe0"));

    auto sourceProp = component->getCalculated(kPropertySource);
    ASSERT_TRUE(sourceProp.is<URLRequest>());
    auto asSource = sourceProp.get<URLRequest>();
    auto headers = asSource.getHeaders();
    ASSERT_EQ(headers.size(), 1);
    ASSERT_EQ(headers.at(0), "A: Let me in");
}

static const char* SINGLE_VIDEO_WITH_WRONG_HEADER = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Video",
      "source": {
        "url": "universe",
        "description": "milky way",
        "headers": [ " A Let me in" ]
      }
    }
  }
})";
TEST_F(MediaManagerTest, VideoWithWrongHeaders) {
    loadDocument(SINGLE_VIDEO_WITH_WRONG_HEADER);

    ASSERT_FALSE(root->isDirty());

    // Based on the spec, sources get transformed into an array
    auto mediaSources = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(mediaSources.size(), 1);
    ASSERT_TRUE(mediaSources.at(0).is<MediaSource>());

    auto asMediaSource = mediaSources.at(0).get<MediaSource>();
    ASSERT_EQ(asMediaSource.getDescription(), "milky way");
    auto headers = asMediaSource.getHeaders();
    ASSERT_EQ(headers.size(), 0);
}

static const char* SINGLE_VIDEO_WITH_HEADERS = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Video",
      "source": {
        "url": "universe",
        "description": "milky way",
        "headers": [ " A: Let me in" ]
      }
    }
  }
})";

TEST_F(MediaManagerTest, VideoWithHeaders) {
    loadDocument(SINGLE_VIDEO_WITH_HEADERS);

    ASSERT_FALSE(root->isDirty());

    // Based on the spec, sources get transformed into an array
    auto mediaSources = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(mediaSources.size(), 1);
    ASSERT_TRUE(mediaSources.at(0).is<MediaSource>());

    auto asMediaSource = mediaSources.at(0).get<MediaSource>();
    ASSERT_EQ(asMediaSource.getDescription(), "milky way");
    auto headers = asMediaSource.getHeaders();
    ASSERT_EQ(headers.size(), 1);
    ASSERT_EQ(headers.at(0), "A: Let me in");
}

TEST_F(MediaManagerTest, VideoWithHeadersSetsDirtyAfterHeaderChange) {
    loadDocument(SINGLE_VIDEO_WITH_HEADERS);

    ASSERT_FALSE(root->isDirty());

    // Based on the spec, sources get transformed into an array
    auto mediaSources = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(mediaSources.size(), 1);
    ASSERT_TRUE(mediaSources.at(0).is<MediaSource>());

    auto asMediaSource = mediaSources.at(0).get<MediaSource>();
    ASSERT_EQ(asMediaSource.getDescription(), "milky way");
    auto headers = asMediaSource.getHeaders();
    ASSERT_EQ(headers.size(), 1);
    ASSERT_EQ(headers.at(0), "A: Let me in");

    // Change only the headers
    ObjectArrayPtr newHeaders = std::make_shared<ObjectArray>();
    newHeaders->emplace_back("A: Let me out");
    ObjectMapPtr newSource = std::make_shared<ObjectMap>();
    newSource->emplace("url", "universe");
    newSource->emplace("description", "milky way");
    newSource->emplace("headers", newHeaders);
    ObjectArrayPtr newMedia = std::make_shared<ObjectArray>();
    newMedia->emplace_back(MediaSource::create(*context, newSource));

    component->setProperty(kPropertySource, newMedia);

    ASSERT_TRUE(CheckDirty(component, kPropertySource, kPropertyVisualHash));

    mediaSources = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(mediaSources.size(), 1);
    ASSERT_TRUE(mediaSources.at(0).is<MediaSource>());
    asMediaSource = mediaSources.at(0).get<MediaSource>();
    ASSERT_EQ(asMediaSource.getDescription(), "milky way");
    headers = asMediaSource.getHeaders();
    ASSERT_EQ(headers.size(), 1);
    ASSERT_EQ(headers.at(0), "A: Let me out");

    // Update again
    ObjectArrayPtr newHeaders2 = std::make_shared<ObjectArray>();
    newHeaders2->emplace_back("D: Let me in");
    ObjectMapPtr newSource2 = std::make_shared<ObjectMap>();
    newSource2->emplace("url", "universe");
    newSource2->emplace("description", "milky way");
    newSource2->emplace("headers", newHeaders2);
    ObjectArrayPtr newMedia2 = std::make_shared<ObjectArray>();
    newMedia2->emplace_back(MediaSource::create(*context, newSource2));

    component->setProperty(kPropertySource, newMedia2);

    ASSERT_TRUE(CheckDirty(component, kPropertySource, kPropertyVisualHash));
    mediaSources = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(mediaSources.size(), 1);
    ASSERT_TRUE(mediaSources.at(0).is<MediaSource>());
    asMediaSource = mediaSources.at(0).get<MediaSource>();
    ASSERT_EQ(asMediaSource.getDescription(), "milky way");
    headers = asMediaSource.getHeaders();
    ASSERT_EQ(headers.size(), 1);
    ASSERT_EQ(headers.at(0), "D: Let me in");
}

TEST_F(MediaManagerTest, VideoWithHeadersDenyUppercase) {
    config->filterHeaders(std::vector<std::pair<std::regex, bool>>{{std::regex("A", std::regex_constants::icase), false}});
    loadDocument(SINGLE_VIDEO_WITH_HEADERS);

    ASSERT_FALSE(root->isDirty());

    // Based on the spec, sources get transformed into an array
    auto mediaSources = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(mediaSources.size(), 1);
    ASSERT_TRUE(mediaSources.at(0).is<MediaSource>());

    auto asMediaSource = mediaSources.at(0).get<MediaSource>();
    ASSERT_EQ(asMediaSource.getDescription(), "milky way");
    auto headers = asMediaSource.getHeaders();
    ASSERT_EQ(headers.size(), 0);
}

TEST_F(MediaManagerTest, VideoWithHeadersDenyLowercase) {
    config->filterHeaders(std::vector<std::pair<std::regex, bool>>{{std::regex("a", std::regex_constants::icase), false}});
    loadDocument(SINGLE_VIDEO_WITH_HEADERS);

    ASSERT_FALSE(root->isDirty());

    // Based on the spec, sources get transformed into an array
    auto mediaSources = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(mediaSources.size(), 1);
    ASSERT_TRUE(mediaSources.at(0).is<MediaSource>());

    auto asMediaSource = mediaSources.at(0).get<MediaSource>();
    ASSERT_EQ(asMediaSource.getDescription(), "milky way");
    auto headers = asMediaSource.getHeaders();
    ASSERT_EQ(headers.size(), 0);
}

TEST_F(MediaManagerTest, VideoWithHeadersAllowListUppercase) {
    config->filterHeaders(std::vector<std::pair<std::regex, bool>>{{std::regex("A", std::regex_constants::icase), true}});
    loadDocument(SINGLE_VIDEO_WITH_HEADERS);

    ASSERT_FALSE(root->isDirty());

    // Based on the spec, sources get transformed into an array
    auto mediaSources = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(mediaSources.size(), 1);
    ASSERT_TRUE(mediaSources.at(0).is<MediaSource>());

    auto asMediaSource = mediaSources.at(0).get<MediaSource>();
    ASSERT_EQ(asMediaSource.getDescription(), "milky way");
    auto headers = asMediaSource.getHeaders();
    ASSERT_EQ(headers.size(), 1);
    ASSERT_EQ(headers.at(0), "A: Let me in");
}

TEST_F(MediaManagerTest, VideoWithHeadersAllowListLowercase) {
    config->filterHeaders(std::vector<std::pair<std::regex, bool>>{{std::regex("a", std::regex_constants::icase), true}});
    loadDocument(SINGLE_VIDEO_WITH_HEADERS);

    ASSERT_FALSE(root->isDirty());

    // Based on the spec, sources get transformed into an array
    auto mediaSources = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(mediaSources.size(), 1);
    ASSERT_TRUE(mediaSources.at(0).is<MediaSource>());

    auto asMediaSource = mediaSources.at(0).get<MediaSource>();
    ASSERT_EQ(asMediaSource.getDescription(), "milky way");
    auto headers = asMediaSource.getHeaders();
    ASSERT_EQ(headers.size(), 1);
    ASSERT_EQ(headers.at(0), "A: Let me in");
}

TEST_F(MediaManagerTest, VideoWithHeadersAllowListNotPresent) {
    config->filterHeaders(std::vector<std::pair<std::regex, bool>>{{std::regex("B", std::regex_constants::icase), true}});
    loadDocument(SINGLE_VIDEO_WITH_HEADERS);

    ASSERT_FALSE(root->isDirty());

    // Based on the spec, sources get transformed into an array
    auto mediaSources = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(mediaSources.size(), 1);
    ASSERT_TRUE(mediaSources.at(0).is<MediaSource>());

    auto asMediaSource = mediaSources.at(0).get<MediaSource>();
    ASSERT_EQ(asMediaSource.getDescription(), "milky way");
    auto headers = asMediaSource.getHeaders();
    ASSERT_EQ(headers.size(), 1);
    ASSERT_EQ(headers.at(0), "A: Let me in");
}

static const char* SINGLE_VIDEO_WITH_AMAZON_HEADERS = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Video",
      "source": {
        "url": "universe",
        "description": "milky way",
        "headers": [ "X-amzn-test: Let me in",  "X-amzn-test2: Let me in2", "C: other"]
      }
    }
  }
})";

TEST_F(MediaManagerTest, VideoWithHeadersAllowRegex) {
    config->filterHeaders(std::vector<std::pair<std::regex, bool>>{
        {std::regex("(x-amzn-)(.*)", std::regex_constants::icase), true},
        {std::regex("(x-amz-)(.*)", std::regex_constants::icase), true},
        {std::regex("C", std::regex_constants::icase), false}
    });

    loadDocument(SINGLE_VIDEO_WITH_AMAZON_HEADERS);

    ASSERT_FALSE(root->isDirty());

    // Based on the spec, sources get transformed into an array
    auto mediaSources = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(mediaSources.size(), 1);
    ASSERT_TRUE(mediaSources.at(0).is<MediaSource>());

    auto asMediaSource = mediaSources.at(0).get<MediaSource>();
    ASSERT_EQ(asMediaSource.getDescription(), "milky way");
    auto headers = asMediaSource.getHeaders();
    ASSERT_EQ(headers.size(), 2);
    ASSERT_EQ(headers.at(0), "X-amzn-test: Let me in");
    ASSERT_EQ(headers.at(1), "X-amzn-test2: Let me in2");
}

TEST_F(MediaManagerTest, VideoWithHeadersDenyRegex) {
    config->filterHeaders(std::vector<std::pair<std::regex, bool>>{
        {std::regex("(x-amzn-)(.*)", std::regex_constants::icase), false},
        {std::regex("(x-amz-)(.*)", std::regex_constants::icase), false}
    });

    loadDocument(SINGLE_VIDEO_WITH_AMAZON_HEADERS);

    ASSERT_FALSE(root->isDirty());

    // Based on the spec, sources get transformed into an array
    auto mediaSources = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(mediaSources.size(), 1);
    ASSERT_TRUE(mediaSources.at(0).is<MediaSource>());

    auto asMediaSource = mediaSources.at(0).get<MediaSource>();
    ASSERT_EQ(asMediaSource.getDescription(), "milky way");
    auto headers = asMediaSource.getHeaders();
    ASSERT_EQ(headers.size(), 1);
    ASSERT_EQ(headers.at(0), "C: other");
}

TEST_F(MediaManagerTest, VideoWithHeadersDenyAllRegex) {
    config->filterHeaders(std::vector<std::pair<std::regex, bool>>{
        {std::regex("(X-)(.*)", std::regex_constants::icase), false}
    });

    loadDocument(SINGLE_VIDEO_WITH_AMAZON_HEADERS);

    ASSERT_FALSE(root->isDirty());

    // Based on the spec, sources get transformed into an array
    auto mediaSources = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(mediaSources.size(), 1);
    ASSERT_TRUE(mediaSources.at(0).is<MediaSource>());

    auto asMediaSource = mediaSources.at(0).get<MediaSource>();
    ASSERT_EQ(asMediaSource.getDescription(), "milky way");
    auto headers = asMediaSource.getHeaders();
    ASSERT_EQ(headers.size(), 1);
    ASSERT_EQ(headers.at(0), "C: other");
}

static const char* SINGLE_VIDEO_WITH_CONTENT_TYPE = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Video",
      "source": {
        "url": "universe",
        "description": "milky way",
        "headers": [ "X-amzn-test: Let me in",  "X-amzn-test2: Let me in2", "Content-Type: other"]
      }
    }
  }
})";

TEST_F(MediaManagerTest, VideoWithHeadersAcceptContentTypeDenyAllRegex) {
    config->filterHeaders(std::vector<std::pair<std::regex, bool>>{
        {std::regex("Content-Type", std::regex_constants::icase), true},
        {std::regex(".*"), false}
    });

    loadDocument(SINGLE_VIDEO_WITH_CONTENT_TYPE);

    ASSERT_FALSE(root->isDirty());

    // Based on the spec, sources get transformed into an array
    auto mediaSources = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(mediaSources.size(), 1);
    ASSERT_TRUE(mediaSources.at(0).is<MediaSource>());

    auto asMediaSource = mediaSources.at(0).get<MediaSource>();
    ASSERT_EQ(asMediaSource.getDescription(), "milky way");
    auto headers = asMediaSource.getHeaders();
    ASSERT_EQ(headers.size(), 1);
    ASSERT_EQ(headers.at(0), "Content-Type: other");
}

static const char* DUPLICATED_HEADERS = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Video",
      "source": {
        "url": "universe",
        "description": "milky way",
        "headers": [
            "Cache-Control: no-cache",
            "Cache-Control: no-store"
        ]
      }
    }
  }
})";

TEST_F(MediaManagerTest, VideoWithDuplicatedHeaders) {
    config->filterHeaders(std::vector<std::pair<std::regex, bool>>{
        {std::regex("Cache-Control", std::regex_constants::icase), true},
        {std::regex(".*"), false}
    });

    loadDocument(DUPLICATED_HEADERS);

    ASSERT_FALSE(root->isDirty());

    // Based on the spec, sources get transformed into an array
    auto mediaSources = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(mediaSources.size(), 1);
    ASSERT_TRUE(mediaSources.at(0).is<MediaSource>());

    auto asMediaSource = mediaSources.at(0).get<MediaSource>();
    ASSERT_EQ(asMediaSource.getDescription(), "milky way");
    auto headers = asMediaSource.getHeaders();
    ASSERT_EQ(headers.size(), 2);
    ASSERT_EQ(headers.at(0), "Cache-Control: no-cache");
    ASSERT_EQ(headers.at(1), "Cache-Control: no-store");
}

static const char* MULTIPLE_IMAGES_WITH_ERRORS = R"({
  "type": "APL",
  "version": "1.8",
  "mainTemplate": {
    "item": {
      "type": "Image",
      "sources": [
        [],
        {
          "url": ""
        }
      ]
    }
  }
})";

TEST_F(MediaManagerTest, SourceWithErrorsAreEmptyString) {
    loadDocument(MULTIPLE_IMAGES_WITH_ERRORS);

    ASSERT_FALSE(root->isDirty());

    auto sourceProp = component->getCalculated(kPropertySource);
    ASSERT_TRUE(sourceProp.isString());
    ASSERT_EQ(sourceProp.asString().length(), 0);
    // We don't care about the session messages on this test, clean it
    session->clear();
}

static const char* SINGLE_VIDEO_WITH_MULTIPLE_HEADERS = R"({
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "item": {
      "type": "Video",
      "source": {
        "url": "universe",
        "description": "milky way",
        "headers": [ " A: Let me in", "B: A ", " C: A ", "   D:A     ", "   E: F   " ]
      }
    }
  }
})";

TEST_F(MediaManagerTest, VideoWithMultipleHeaders) {
    loadDocument(SINGLE_VIDEO_WITH_MULTIPLE_HEADERS);

    ASSERT_FALSE(root->isDirty());

    // Based on the spec, sources get transformed into an array
    auto mediaSources = component->getCalculated(kPropertySource).getArray();
    ASSERT_EQ(mediaSources.size(), 1);
    ASSERT_TRUE(mediaSources.at(0).is<MediaSource>());

    auto asMediaSource = mediaSources.at(0).get<MediaSource>();
    ASSERT_EQ(asMediaSource.getDescription(), "milky way");
    auto headers = asMediaSource.getHeaders();
    ASSERT_EQ(headers.size(), 5);
    ASSERT_EQ(headers.at(0), "A: Let me in");
    ASSERT_EQ(headers.at(1), "B: A");
    ASSERT_EQ(headers.at(2), "C: A");
    ASSERT_EQ(headers.at(3), "D: A");
    ASSERT_EQ(headers.at(4), "E: F");
}

static const char* CHANGING_IMAGES = R"({
  "type": "APL",
  "version": "1.8",
  "mainTemplate": {
    "item": {
      "type": "Image",
      "id": "IMAGE",
      "source": "duck.png"
    }
  }
})";

TEST_F(MediaManagerTest, ChangingImages) {
    loadDocument(CHANGING_IMAGES);
    ASSERT_FALSE(root->isDirty());

    // Event should be fired that requests media to be loaded.
    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "duck.png"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(component, "duck.png"));

    executeCommand("SetValue",
                   {{"componentId", "IMAGE"}, {"property", "source"}, {"value", "duck2.png"}},
                   true);
    ASSERT_TRUE(CheckDirty(component, kPropertySource, kPropertyMediaState, kPropertyVisualHash));

    // Event should be fired that requests media to be loaded.
    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "duck2.png"));
    ASSERT_EQ(kMediaStatePending, component->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_TRUE(CheckLoadedMedia(component, "duck2.png"));
}


static const char* FIRST_LAST_SEQUENCE = R"apl({
  "type": "APL",
  "version": "1.10",
  "theme": "dark",
  "mainTemplate": {
    "items": [
      {
        "type": "Sequence",
        "width": 500,
        "height": 500,
        "data": [0,1,2,3,4,5,6,7,8,9],
        "firstItem": {
          "type": "Image",
          "width": 200,
          "height": 100,
          "source": "universe_first"
        },
        "items": [
          {
            "type": "Image",
            "width": 200,
            "height": 200,
            "source": "universe_${data}"
          }
        ],
        "lastItem": {
          "type": "Image",
          "width": 200,
          "height": 100,
          "source": "universe_last"
        }
      }
    ]
  }
})apl";

TEST_F(MediaManagerTest, FirstLastSequence) {
    config->set(RootProperty::kSequenceChildCache, 1);
    loadDocument(FIRST_LAST_SEQUENCE);

    ASSERT_FALSE(root->isDirty());

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe_first", "universe_0", "universe_1", "universe_2"));

    // Two more will be requested to cover cache position here.
    advanceTime(10);

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "universe_3", "universe_4"));

    ASSERT_FALSE(root->hasEvent());
}


static const char* DEEP_EVALUATE_SOURCE = R"apl({
  "type": "APL",
  "version": "1.5",
  "theme": "dark",
  "mainTemplate": {
    "parameters": [
      "payload"
    ],
    "items": [
      {
        "type": "Container",
        "items": [
          {
            "type": "AlexaMusicAttributionLogoImage",
            "imageMap": {
              "largeUrl": "${payload.url}"
            },
            "largeUrl": "${payload.url}"
          }
        ]
      }
    ]
  },
  "layouts": {
    "AlexaMusicAttributionLogoImage": {
      "parameters": [
        {
          "name": "imageMap",
          "type": "map"
        },
        {
          "name": "largeUrl"
        }
      ],
      "item": [
        {
          "type": "Container",
          "items": [
            {
              "type": "Text",
              "id": "text1",
              "text": "${imageMap.largeUrl}"
            },
            {
              "type": "Text",
              "id": "text2",
              "text": "${largeUrl}"
            },
            {
              "type": "Image",
              "id": "image1",
              "source": "${imageMap.largeUrl}"
            },
            {
              "type": "Image",
              "id": "image2",
              "source": "https://image.png"
            }
          ]
        }
      ]
    }
  }
})apl";

static const char* DEEP_EVALUATE_SOURCE_DATA = R"apl({
  "url": "https://image.png"
})apl";

TEST_F(MediaManagerTest, DeepEvaluationSource) {
    loadDocument(DEEP_EVALUATE_SOURCE, DEEP_EVALUATE_SOURCE_DATA);

    auto text1 = root->findComponentById("text1");
    auto text2 = root->findComponentById("text2");
    auto image1 = root->findComponentById("image1");
    auto image2 = root->findComponentById("image2");

    root->clearPending();

    auto text1Content = text1->getCalculated(kPropertyText).asString();
    ASSERT_EQ(text1Content, "${payload.url}");
    auto text2Content = text2->getCalculated(kPropertyText).asString();
    ASSERT_EQ(text2Content, "https://image.png");
    auto image1content = image1->getCalculated(apl::kPropertySource).asString();
    ASSERT_EQ(image1content, "https://image.png");
    auto image2content = image2->getCalculated(apl::kPropertySource).asString();
    ASSERT_EQ(image1content, "https://image.png");

    // We don't care about events on this test
    root.reset();
}

static const char* IMAGE_ON_FAIL_CHANGES_SOURCE = R"({
  "type": "APL",
  "version": "2023.1",
  "mainTemplate": {
    "items": {
      "type": "Image",
      "id": "myImageWithFallback",
      "sources": [
        "original"
      ],
      "onFail": {
        "type": "SetValue",
        "componentId": "myImageWithFallback",
        "property": "source",
        "value": "fallback"
      }
    }
  }
}
)";

TEST_F(MediaManagerTest, ImageOnLoadFailChangesSource)
{
    metrics.size(800, 600);
    loadDocument(IMAGE_ON_FAIL_CHANGES_SOURCE);

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "original"));
    advanceTime(100);

    root->mediaLoadFailed("original", 2, "Invalid image");

    ASSERT_TRUE(MediaRequested(kEventMediaTypeImage, "fallback"));
    advanceTime(100);
}
