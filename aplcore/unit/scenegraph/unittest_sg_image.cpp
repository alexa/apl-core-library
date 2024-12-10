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
#include "test_sg.h"
#include "apl/scenegraph/builder.h"
#include "apl/media/mediaobject.h"

using namespace apl;

// Custom media manager that has some pre-defined media objects
class SGMediaManager : public MediaManager {
private:
    class MO : public MediaObject {
    public:
        MO(std::string url, EventMediaType type, MediaObject::State state)
            : mURL(std::move(url)),
              mType(type),
              mState(state)
        {}

        std::string url() const override { return mURL; }
        State state() const override { return mState; }
        EventMediaType type() const override { return mType; }
        Size size() const override { return mSize; }
        int errorCode() const override { return mErrorCode; }
        std::string errorDescription() const override { return mErrorDescription; }
        const HeaderArray& headers() const override { return mHeaders; }

        CallbackID addCallback(MediaObjectCallback callback) override {
            if (mState == kPending) {
                mCallbackCounter += 1;
                mCallbacks.emplace(mCallbackCounter, callback);
                return mCallbackCounter;
            }
            return false;
        }

        void removeCallback(CallbackID callbackId) override {
            mCallbacks.erase(callbackId);
        }

        std::string mURL;
        EventMediaType mType;
        Size mSize;
        State mState;
        std::map<CallbackID, MediaObjectCallback> mCallbacks;
        int mErrorCode = 0;
        std::string mErrorDescription;
        int mCallbackCounter = 0;
        HeaderArray mHeaders;
    };

public:
    MediaObjectPtr request(const std::string& url, EventMediaType type) override {
        auto it = mObjectMap.find(url);
        if (it != mObjectMap.end())
            return it->second;

        auto result = std::make_shared<MO>(url, kEventMediaTypeImage, MediaObject::State::kPending);
        mObjectMap.emplace(url, result);
        return result;
    }

    MediaObjectPtr request(const std::string& url, EventMediaType type, const HeaderArray& headers) override {
        return request(url, type);
    }

    void addMedia(const std::string& url, Size size) {
        auto it = mObjectMap.find(url);
        if (it != mObjectMap.end()) {
            it->second->mSize = size;
            it->second->mState = MediaObject::State::kReady;
            for (const auto& m : it->second->mCallbacks)
                m.second(it->second);
            it->second->mCallbacks.clear();
            return;
        }

        auto ptr = std::make_shared<MO>(url, kEventMediaTypeImage, MediaObject::State::kReady);
        ptr->mSize = size;
        mObjectMap.emplace(url, ptr);
    }

    void failMedia(const std::string& url, int code, const std::string& description) {
        auto it = mObjectMap.find(url);
        if (it != mObjectMap.end()) {
            it->second->mState = MediaObject::State::kError;
            for (const auto& m : it->second->mCallbacks)
                m.second(it->second);
            it->second->mCallbacks.clear();
            return;
        }

        auto ptr = std::make_shared<MO>(url, kEventMediaTypeImage, MediaObject::State::kError);
        mObjectMap.emplace(url, ptr);
    }

    std::vector<std::string> pendingMediaRequests() const {
        auto result = std::vector<std::string>{};
        for (const auto& m : mObjectMap)
            if (m.second->mState == MediaObject::State::kPending)
                result.emplace_back(m.first);
        return result;
    }

    std::map<std::string, std::shared_ptr<MO>> mObjectMap;
};

class SGImageTest : public DocumentWrapper {
public:
    SGImageTest() : DocumentWrapper() {
        config->enableExperimentalFeature(RootConfig::kExperimentalFeatureManageMediaRequests);
        mediaManager = std::make_shared<SGMediaManager>();
        config->mediaManager(mediaManager);
    }

    void addMedia(const std::string& url, Size size) {
        mediaManager->addMedia(url, size);
    }

    void failMedia(const std::string& url) {
        mediaManager->failMedia(url, 99, "Something went wrong");
    }

    void TearDown() override {
        mediaManager.reset();
        DocumentWrapper::TearDown();
    }

    std::vector<std::string> pendingMediaRequests() const {
        return mediaManager->pendingMediaRequests();
    }

    std::shared_ptr<SGMediaManager> mediaManager;
};




static const char * BASIC_TEST = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "items": {
          "type": "Image",
          "width": 200,
          "height": 200,
          "source": "http://fake.url"
        }
      }
    }
)apl";

TEST_F(SGImageTest, Preloaded) {
    addMedia("http://fake.url", Size{100, 200});

    metrics.size(300, 300);
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(component);

    // Note: Image defaults to "center", "best-fit"
    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 200})
                .characteristic(sg::Layer::kCharacteristicHasMedia)
                .content(IsClipNode()
                             .path(IsRoundRectPath(50, 0, 100, 200, 0)) // Clip to target region
                             .child(IsImageNode()
                                        .filterTest(IsMediaObjectFilter("http://fake.url"))
                                        .target(Rect{50, 0, 100, 200})
                                        .source(Rect{0, 0, 100, 200})))));
}

TEST_F(SGImageTest, FailedLoad)
{
    metrics.size(300, 300);
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(component);

    // Note: Image defaults to "center", "best-fit"
    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 200})
                .characteristic(sg::Layer::kCharacteristicHasMedia)
                .content(IsClipNode()
                             .path(IsRoundRectPath(RoundedRect{})) // Clip to target region
                             .child(IsImageNode().filterTest(IsMediaObjectFilter(
                                 "http://fake.url", MediaObject::State::kPending))))));

    ASSERT_EQ(1, pendingMediaRequests().size());

    failMedia("http://fake.url");
    ASSERT_EQ(0, pendingMediaRequests().size());

    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 200})
                .dirty(sg::Layer::kFlagRedrawContent) // The image node will have changed
                .characteristic(sg::Layer::kCharacteristicHasMedia)
                .content(IsClipNode(".clip")
                             .path(IsRoundRectPath(RoundedRect{})) // Clip to target region
                             .child(IsImageNode(".image")
                                        .filterTest(IsMediaObjectFilter(
                                            "http://fake.url", MediaObject::State::kError))))));
}

TEST_F(SGImageTest, DelayedLoad) {
    metrics.size(300, 300);
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(component);

    // Note: Image defaults to "center", "best-fit"
    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 200})
                .characteristic(sg::Layer::kCharacteristicHasMedia)
                .content(IsClipNode()
                             .path(IsRoundRectPath(RoundedRect{})) // Clip to target region
                             .child(IsImageNode().filterTest(IsMediaObjectFilter(
                                 "http://fake.url", MediaObject::State::kPending))))));

    ASSERT_EQ(1, pendingMediaRequests().size());

    addMedia("http://fake.url", Size{100, 200});
    ASSERT_EQ(0, pendingMediaRequests().size());

    sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 200, 200})
                .characteristic(sg::Layer::kCharacteristicHasMedia)
                .dirty(sg::Layer::kFlagRedrawContent)     // The image content changed
                .content(IsClipNode()
                             .path(IsRoundRectPath(50, 0, 100, 200, 0))
                             .child(IsImageNode()
                                        .filterTest(IsMediaObjectFilter("http://fake.url"))
                                        .target(Rect{50, 0, 100, 200})
                                        .source(Rect{0, 0, 100, 200})))));
}

static const char * LEGACY_IMAGE = R"apl(
    {
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
            "height": 200,
            "source": "http://fake.url"
          }
        }
      }
    }
)apl";

TEST_F(SGImageTest, LegacyNoCroppedImage)
{
    addMedia("http://fake.url", Size{100, 200});

    metrics.size(300, 300);
    loadDocument(LEGACY_IMAGE);
    ASSERT_TRUE(component);

    // Note: Image defaults to "center", "best-fit"
    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 100, 100}, "...Container")
            .child(IsLayer(Rect{0, 0, 200, 200})
                       .characteristic(sg::Layer::kCharacteristicHasMedia | sg::Layer::kCharacteristicDoNotClipChildren)
                       .content(IsClipNode()
                                    .path(IsRoundRectPath(50, 0, 100, 200,
                                                          0)) // Target bounds
                                    .child(IsImageNode()
                                               .filterTest(IsMediaObjectFilter("http://fake.url"))
                                               .target(Rect{50, 0, 100, 200})
                                               .source(Rect{0, 0, 100, 200}))))));
}


static const char * FRAMED_IMAGE = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "items": {
          "type": "Frame",
          "width": 200,
          "height": 200,
          "borderWidth": 10,
          "borderColor": "red",
          "items": {
            "type": "Image",
            "width": 200,
            "height": 200,
            "source": "http://fake.url"
          }
        }
      }
    }
)apl";

TEST_F(SGImageTest, FramedImage)
{
    addMedia("http://fake.url", Size{100, 200});

    metrics.size(300, 300);
    loadDocument(FRAMED_IMAGE);
    ASSERT_TRUE(component);

    // Note: Image defaults to "center", "best-fit"
    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 200, 200}, "...Frame")
            .content(IsDrawNode() // Draw the border of the frame
                        .path(IsFramePath(RoundedRect({0, 0, 200, 200}, 0), 10))
                        .pathOp(IsFillOp(IsColorPaint(Color::RED), apl::sg::kFillTypeEvenOdd)))
            .childClip(IsRoundRectPath(10, 10, 180, 180, 0))
            .child(IsLayer(Rect{10, 10, 200, 200})
                       .characteristic(sg::Layer::kCharacteristicHasMedia)
                       .content(IsClipNode()
                                    .path(IsRoundRectPath(50, 0, 100, 200,
                                                          0)) // Target bounds
                                    .child(IsImageNode()
                                               .filterTest(IsMediaObjectFilter("http://fake.url"))
                                               .target(Rect{50, 0, 100, 200})
                                               .source(Rect{0, 0, 100, 200}))))));
}


static const char *COLOR_OVERLAY = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "items": {
          "type": "Image",
          "width": 400,
          "height": 400,
          "source": "ALPHA",
          "overlayColor": "blue"
        }
      }
    }
)apl";

TEST_F(SGImageTest, ColorOverlay) {
    addMedia("ALPHA", Size{100, 200});

    metrics.size(300, 300);
    loadDocument(COLOR_OVERLAY);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();

    // Default is "center", "best-fit"
    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 400, 400})
            .characteristic(sg::Layer::kCharacteristicHasMedia)
            .content(IsClipNode()
                         .path(IsRoundRectPath(100, 0, 200, 400,
                                               0)) // Clip to the image size (is this needed?)
                         .child(IsImageNode()
                                    .filterTest(IsBlendFilter(
                                        IsMediaObjectFilter("ALPHA"),
                                        IsSolidFilter(IsColorPaint(Color::BLUE)), kBlendModeNormal))
                                    .target(Rect{100, 0, 200, 400})
                                    .source(Rect{0, 0, 100, 200})))));
}


static const char *GRADIENT_OVERLAY = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "items": {
          "type": "Image",
          "width": 400,
          "height": 400,
          "source": "ALPHA",
          "overlayGradient": { "colorRange": ["blue", "red"] }
        }
      }
    }
)apl";

TEST_F(SGImageTest, GradientOverlay) {
    addMedia("ALPHA", Size{100, 200});

    metrics.size(300, 300);
    loadDocument(GRADIENT_OVERLAY);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();

    // Default is "center", "best-fit"
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 400, 400})
                .characteristic(sg::Layer::kCharacteristicHasMedia)
                .content(IsClipNode()
                             .path(IsRoundRectPath(100, 0, 200, 400,
                                                   0)) // Clip to the image size (is this needed?)
                             .child(IsImageNode()
                                        .filterTest(IsBlendFilter(
                                            IsMediaObjectFilter("ALPHA"),
                                            IsSolidFilter(IsLinearGradientPaint(
                                                {0, 1}, {Color::BLUE, Color::RED}, Gradient::PAD,
                                                true, {0.5, 1}, {0.5, 0})),
                                            kBlendModeNormal))
                                        .target(Rect{100, 0, 200, 400})
                                        .source(Rect{0, 0, 100, 200})))));
}


static const char *GRADIENT_COLOR_OVERLAY = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "items": {
          "type": "Image",
          "width": 400,
          "height": 400,
          "source": "ALPHA",
          "overlayGradient": { "type": "radial", "colorRange": ["blue", "red"] },
          "overlayColor": "green"
        }
      }
    }
)apl";

TEST_F(SGImageTest, GradientColorOverlay) {
    addMedia("ALPHA", Size{100, 200});

    metrics.size(300, 300);
    loadDocument(GRADIENT_COLOR_OVERLAY);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();

    // Default is "center", "best-fit"
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 400, 400})
                .characteristic(sg::Layer::kCharacteristicHasMedia)
                .content(IsClipNode()
                             .path(IsRoundRectPath(100, 0, 200, 400,
                                                   0)) // Clip to the image size (is this needed?)
                             .child(IsImageNode()
                                        .filterTest(IsBlendFilter(
                                            IsBlendFilter(IsMediaObjectFilter("ALPHA"),
                                                          IsSolidFilter(IsColorPaint(Color::GREEN)),
                                                          kBlendModeNormal),
                                            IsSolidFilter(IsRadialGradientPaint(
                                                {0, 1}, {Color::BLUE, Color::RED}, Gradient::PAD,
                                                true, {0.5, 0.5}, M_SQRT1_2)),
                                            kBlendModeNormal))
                                        .target(Rect{100, 0, 200, 400})
                                        .source(Rect{0, 0, 100, 200})))));
}


static const char *TWO_IMAGES = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "items": {
          "type": "Image",
          "width": 250,
          "height": 310,
          "align": "left",
          "scale": "none",
          "source": [
            "ALPHA",
            "BETA"
          ]
        }
      }
    }
)apl";

TEST_F(SGImageTest, TwoImages) {
    addMedia("ALPHA", Size{100, 200});
    addMedia("BETA", Size{40, 50});

    metrics.size(300, 300);
    loadDocument(TWO_IMAGES);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 250, 310})
                .characteristic(sg::Layer::kCharacteristicHasMedia)
                .content(IsClipNode()
                             .path(IsRoundRectPath(0, 130, 40, 50,
                                                   0)) // Clip to the image size (is this needed?)
                             .child(IsImageNode()
                                        .filterTest(IsMediaObjectFilter("BETA"))
                                        .target(Rect{0, 130, 40, 50})
                                        .source(Rect{0, 0, 40, 50})))));
}


static const char *TWO_IMAGES_BLEND = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "items": {
          "type": "Image",
          "width": 250,
          "height": 310,
          "align": "left",
          "scale": "none",
          "source": [
            "ALPHA",
            "BETA"
          ],
          "filters": { "type": "Blend", "mode": "darken" }
        }
      }
    }
)apl";

TEST_F(SGImageTest, TwoImagesBlend) {
    addMedia("ALPHA", Size{100, 200});
    addMedia("BETA", Size{40, 50});

    metrics.size(300, 300);
    loadDocument(TWO_IMAGES_BLEND);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 250, 310})
                .characteristic(sg::Layer::kCharacteristicHasMedia)
                .content(IsClipNode()
                             .path(IsRoundRectPath(0, 55, 100, 200, 0)) // Clip to the image size
                             .child(IsImageNode()
                                        .filterTest(IsBlendFilter(IsMediaObjectFilter("ALPHA"),
                                                                  IsMediaObjectFilter("BETA"),
                                                                  kBlendModeDarken))
                                        .target(Rect{0, 55, 100, 200})
                                        .source(Rect{0, 0, 100, 200})))));
}

static const char *INVALID_FILTER_SOURCE = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "items": {
          "type": "Image",
          "width": 250,
          "height": 310,
          "align": "left",
          "scale": "none",
          "source": [
            "ALPHA"
          ],
          "filters": [
            { "type": "Saturate", "source": 5 }
          ]
        }
      }
    }
)apl";

TEST_F(SGImageTest, InvalidFilterSource) {
    addMedia("ALPHA", Size{100, 200});

    metrics.size(300, 300);
    loadDocument(INVALID_FILTER_SOURCE);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 250, 310})
                .characteristic(sg::Layer::kCharacteristicHasMedia)
                .content(IsClipNode()
                             .path(IsRoundRectPath(0, 55, 100, 200, 0)) // Clip to the image size
                             .child(IsImageNode()
                                        .filterTest(IsMediaObjectFilter("ALPHA"))
                                        .target(Rect{0, 55, 100, 200})
                                        .source(Rect{0, 0, 100, 200})))));
}

static const char *TRANSPARENT_COLOR_BLEND_BACK = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "items": {
          "type": "Image",
          "width": 250,
          "height": 310,
          "align": "left",
          "scale": "none",
          "source": [
            "ALPHA"
          ],
          "filters": [
            { "type": "Color" },
            { "type": "Blend", "mode": "darken" }
          ]
        }
      }
    }
)apl";

TEST_F(SGImageTest, TransparentColorBlendBack) {
    addMedia("ALPHA", Size{100, 200});

    metrics.size(300, 300);
    loadDocument(TRANSPARENT_COLOR_BLEND_BACK);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 250, 310})
                .characteristic(sg::Layer::kCharacteristicHasMedia)
                .content(IsClipNode()
                             .path(IsRoundRectPath(0, 55, 100, 200, 0)) // Clip to the image size
                             .child(IsImageNode()
                                        .filterTest(IsMediaObjectFilter("ALPHA"))
                                        .target(Rect{0, 55, 100, 200})
                                        .source(Rect{0, 0, 100, 200})))));
}

static const char *TRANSPARENT_COLOR_BLEND_FRONT = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "items": {
          "type": "Image",
          "width": 250,
          "height": 310,
          "align": "left",
          "scale": "none",
          "source": [
            "ALPHA"
          ],
          "filters": [
            { "type": "Color" },
            { "type": "Blend", "mode": "darken", "source": -2, "destination": -1 }
          ]
        }
      }
    }
)apl";

TEST_F(SGImageTest, TransparentColorBlendFront) {
    addMedia("ALPHA", Size{100, 200});

    metrics.size(300, 300);
    loadDocument(TRANSPARENT_COLOR_BLEND_FRONT);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 250, 310})
                .characteristic(sg::Layer::kCharacteristicHasMedia)
                .content(IsClipNode()
                             .path(IsRoundRectPath(0, 55, 100, 200, 0)) // Clip to the image size
                             .child(IsImageNode()
                                        .filterTest(IsMediaObjectFilter("ALPHA"))
                                        .target(Rect{0, 55, 100, 200})
                                        .source(Rect{0, 0, 100, 200})))));
}


static const char *MANY_FILTERS = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "items": {
          "type": "Image",
          "width": 400,
          "height": 400,
          "source": "ALPHA",
          "filters": [
            { "type": "Blur", "radius": 0.5 },
            { "type": "Grayscale", "amount": 0.5 },
            { "type": "Noise" },
            { "type": "Saturate" }
          ]
        }
      }
    }
)apl";

TEST_F(SGImageTest, ManyFilters) {
    addMedia("ALPHA", Size{100, 200});

    metrics.size(300, 300);
    loadDocument(MANY_FILTERS);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();

    // Default is "center", "best-fit"
    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 400, 400})
            .characteristic(sg::Layer::kCharacteristicHasMedia)
            .content(
                IsClipNode()
                    .path(IsRoundRectPath(100, 0, 200, 400,
                                          0)) // Clip to the image size (is this needed?)
                    .child(IsImageNode()
                               .filterTest(IsSaturateFilter(
                                   IsNoiseFilter(
                                       IsGrayscaleFilter(
                                           IsBlurFilter(IsMediaObjectFilter("ALPHA"), 0.5), 0.5),
                                       kFilterNoiseKindGaussian, false, 10.0),
                                   1.0))
                               .target(Rect{100, 0, 200, 400})
                               .source(Rect{0, 0, 100, 200})))));
}
