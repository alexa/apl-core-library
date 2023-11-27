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
#include "apl/media/mediaobject.h"

using namespace apl;
// Custom media manager that has some pre-defined media objects
class SGAVGManager : public MediaManager {
private:
    class MO : public MediaObject {
    public:
        MO(std::string url, MediaObject::State state)
            : mURL(std::move(url)),
              mState(state)
        {}

        std::string url() const override { return mURL; }
        State state() const override { return mState; }
        EventMediaType type() const override { return apl::kEventMediaTypeVectorGraphic; }
        Size size() const override { return {10,10}; }
        int errorCode() const override { return mErrorCode; }
        std::string errorDescription() const override { return mErrorDescription; }
        const HeaderArray& headers() const override { return mHeaders; }
        GraphicContentPtr graphic() override { return mGraphic; }

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
        State mState;
        std::map<CallbackID, MediaObjectCallback> mCallbacks;
        int mErrorCode = 0;
        std::string mErrorDescription;
        int mCallbackCounter = 0;
        HeaderArray mHeaders;
        GraphicContentPtr mGraphic;
    };

public:
    SGAVGManager(const SessionPtr& session) : mSession(session) {}

    MediaObjectPtr request(const std::string& url, EventMediaType type) override {
        assert(type == apl::kEventMediaTypeVectorGraphic);

        auto it = mObjectMap.find(url);
        if (it != mObjectMap.end())
            return it->second;

        auto result = std::make_shared<MO>(url, MediaObject::State::kPending);
        mObjectMap.emplace(url, result);
        return result;
    }

    MediaObjectPtr request(const std::string& url, EventMediaType type, const HeaderArray& headers) override {
        return request(url, type);
    }

    void addMedia(const std::string& url, const char *data) {
        auto it = mObjectMap.find(url);
        if (it != mObjectMap.end()) {
            it->second->mGraphic = GraphicContent::create(mSession, data);
            it->second->mState = MediaObject::State::kReady;
            for (const auto& m : it->second->mCallbacks)
                m.second(it->second);
            it->second->mCallbacks.clear();
            return;
        }

        auto ptr = std::make_shared<MO>(url, MediaObject::State::kReady);
        ptr->mGraphic = GraphicContent::create(mSession, data);
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

        auto ptr = std::make_shared<MO>(url, MediaObject::State::kError);
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
    SessionPtr mSession;
};

class SGGraphicLoadingTest : public DocumentWrapper {
public:
    SGGraphicLoadingTest() : DocumentWrapper() {
        config->enableExperimentalFeature(RootConfig::kExperimentalFeatureManageMediaRequests);
        mediaManager = std::make_shared<SGAVGManager>(session);
        config->mediaManager(mediaManager);
    }

    void addMedia(const std::string& url, const char *data) {
        mediaManager->addMedia(url, data);
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

    std::shared_ptr<SGAVGManager> mediaManager;
};


static const char *BLUE_BOX = R"apl(
{
  "type": "AVG",
  "version": "1.2",
  "width": 200,
  "height": 200,
  "items": {
    "type": "path",
    "fill": "blue",
    "pathData": "h200 v200 h-200 z"
  }
}
)apl";

static const char *RED_BOX = R"apl(
{
  "type": "AVG",
  "version": "1.2",
  "width": 200,
  "height": 200,
  "items": {
    "type": "path",
    "fill": "red",
    "pathData": "h200 v200 h-200 z"
  }
}
)apl";

static const char * BASIC_TEST = R"apl(
{
  "type": "APL",
  "version": "1.6",
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "id": "TestVG",
      "width": 200,
      "height": 200,
      "source": "http://bluebox"
    }
  }
}
)apl";

TEST_F(SGGraphicLoadingTest, Preloaded)
{
    addMedia("http://bluebox", BLUE_BOX);
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(component);
    ASSERT_TRUE(pendingMediaRequests().empty());

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect(0, 0, 200, 200))
                .child(IsLayer(Rect(0, 0, 200, 200))
                           .characteristic(sg::Layer::kCharacteristicRenderOnly)
                           .content(IsTransformNode().child(
                               IsDrawNode()
                                   .path(IsGeneralPath("MLLLZ", {0, 0, 200, 0, 200, 200, 0, 200}))
                                   .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))))));
}


TEST_F(SGGraphicLoadingTest, Postloaded)
{
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(component);
    ASSERT_EQ(1, pendingMediaRequests().size());

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect(0, 0, 200, 200))
                .child(IsLayer(Rect(0, 0, 1, 1))
                        .characteristic(sg::Layer::kCharacteristicRenderOnly))));

    addMedia("http://bluebox", BLUE_BOX);
    ASSERT_TRUE(pendingMediaRequests().empty());

    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect(0, 0, 200, 200))
                .child(IsLayer(Rect(0, 0, 200, 200))
                           .characteristic(sg::Layer::kCharacteristicRenderOnly)
                           .dirty(sg::Layer::kFlagRedrawContent | sg::Layer::kFlagSizeChanged)
                           .content(IsTransformNode().child(
                               IsDrawNode()
                                   .path(IsGeneralPath("MLLLZ", {0, 0, 200, 0, 200, 200, 0, 200}))
                                   .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))))));
}


TEST_F(SGGraphicLoadingTest, Change)
{
    loadDocument(BASIC_TEST);
    ASSERT_TRUE(component);
    ASSERT_EQ(1, pendingMediaRequests().size());

    // The initial VectorGraph is looking for "http://bluebox", which hasn't been received
    auto sg = root->getSceneGraph();
    ASSERT_TRUE(
        CheckSceneGraph(sg, IsLayer(Rect(0, 0, 200, 200)).child(IsLayer(Rect(0, 0, 1, 1))
                                                                    .characteristic(sg::Layer::kCharacteristicRenderOnly))));

    // Change the source to "http://redbox", add it, and verify that the VG inflates correctly
    executeCommand("SetValue",
                   {{"componentId", "TestVG"}, {"property", "source"}, {"value", "http://redbox"}},
                   true);
    ASSERT_EQ(2, pendingMediaRequests().size());

    addMedia("http://redbox", RED_BOX);
    ASSERT_EQ(1, pendingMediaRequests().size());
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect(0, 0, 200, 200))
                .child(IsLayer(Rect(0, 0, 200, 200))
                           .characteristic(sg::Layer::kCharacteristicRenderOnly)
                           .dirty(sg::Layer::kFlagRedrawContent | sg::Layer::kFlagSizeChanged)
                           .content(IsTransformNode().child(
                               IsDrawNode()
                                   .path(IsGeneralPath("MLLLZ", {0, 0, 200, 0, 200, 200, 0, 200}))
                                   .pathOp(IsFillOp(IsColorPaint(Color::RED))))))));

    // Add in "http://bluebox"
    addMedia("http://bluebox", BLUE_BOX);
    ASSERT_TRUE(pendingMediaRequests().empty());

    // Now change back to "http://bluebox".  That should fire immediately
    executeCommand("SetValue",
                   {{"componentId", "TestVG"}, {"property", "source"}, {"value", "http://bluebox"}},
                   true);
    ASSERT_TRUE(pendingMediaRequests().empty());
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect(0, 0, 200, 200))
                .child(IsLayer(Rect(0, 0, 200, 200))
                           .dirty(sg::Layer::kFlagRedrawContent)
                           .characteristic(sg::Layer::kCharacteristicRenderOnly)
                           .content(IsTransformNode().child(
                               IsDrawNode()
                                   .path(IsGeneralPath("MLLLZ", {0, 0, 200, 0, 200, 200, 0, 200}))
                                   .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))))));

    // Finally, swap it to empty
    executeCommand(
        "SetValue",
        {{"componentId", "TestVG"}, {"property", "source"}, {"value", "http://missing_box"}}, true);
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect(0, 0, 200, 200))
                .child(IsLayer(Rect(0, 0, 200, 200)).characteristic(sg::Layer::kCharacteristicRenderOnly).dirty(sg::Layer::kFlagRedrawContent))));
}



static const char * LOCAL_TEST = R"apl(
{
  "type": "APL",
  "version": "1.6",
  "graphics": {
    "yellowBox": {
      "type": "AVG",
      "version": "1.2",
      "width": 200,
      "height": 200,
      "items": {
        "type": "path",
        "fill": "yellow",
        "pathData": "h200 v200 h-200 z"
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "id": "TestVG",
      "width": 200,
      "height": 200,
      "source": "http://bluebox"
    }
  }
}
)apl";


TEST_F(SGGraphicLoadingTest, LocalGraphic)
{
    loadDocument(LOCAL_TEST);
    ASSERT_TRUE(component);
    ASSERT_EQ(1, pendingMediaRequests().size());

    // The initial VectorGraph is looking for "http://bluebox", which hasn't been received
    auto sg = root->getSceneGraph();
    ASSERT_TRUE(
        CheckSceneGraph(sg, IsLayer(Rect(0, 0, 200, 200)).child(IsLayer(Rect(0, 0, 1, 1)).characteristic(sg::Layer::kCharacteristicRenderOnly))));

    // Change the source to "yellowBox", add it, and verify that the VG inflates correctly
    executeCommand("SetValue",
                   {{"componentId", "TestVG"}, {"property", "source"}, {"value", "yellowBox"}},
                   true);
    ASSERT_EQ(1, pendingMediaRequests().size());  // Immediate graphic load
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect(0, 0, 200, 200))
                .child(IsLayer(Rect(0, 0, 200, 200))
                           .dirty(sg::Layer::kFlagRedrawContent | sg::Layer::kFlagSizeChanged)
                           .characteristic(sg::Layer::kCharacteristicRenderOnly)
                           .content(IsTransformNode().child(
                               IsDrawNode()
                                   .path(IsGeneralPath("MLLLZ", {0, 0, 200, 0, 200, 200, 0, 200}))
                                   .pathOp(IsFillOp(IsColorPaint(Color::YELLOW))))))));

    // Empty it
    executeCommand(
        "SetValue",
        {{"componentId", "TestVG"}, {"property", "source"}, {"value", "http://bluebox"}}, true);
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect(0, 0, 200, 200))
                .child(IsLayer(Rect(0, 0, 200, 200)).characteristic(sg::Layer::kCharacteristicRenderOnly).dirty(sg::Layer::kFlagRedrawContent))));

    // Set it back
    executeCommand("SetValue",
                   {{"componentId", "TestVG"}, {"property", "source"}, {"value", "yellowBox"}},
                   true);
    ASSERT_EQ(1, pendingMediaRequests().size());  // Immediate graphic load
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect(0, 0, 200, 200))
                .child(IsLayer(Rect(0, 0, 200, 200))
                           .characteristic(sg::Layer::kCharacteristicRenderOnly)
                           .dirty(sg::Layer::kFlagRedrawContent)
                           .content(IsTransformNode().child(
                               IsDrawNode()
                                   .path(IsGeneralPath("MLLLZ", {0, 0, 200, 0, 200, 200, 0, 200}))
                                   .pathOp(IsFillOp(IsColorPaint(Color::YELLOW))))))));
}
