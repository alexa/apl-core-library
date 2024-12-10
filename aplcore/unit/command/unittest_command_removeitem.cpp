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
#include "../audio/audiotest.h"

#include "apl/component/textcomponent.h"
#include "apl/engine/event.h"
#include "apl/focus/focusmanager.h"


using namespace apl;

class CommandRemoveItemTest : public AudioTest {
public:
    ActionPtr executeRemoveItem(const std::string& component) {
        std::map<std::string, Object> properties = {};

        if (!component.empty()) {
            properties.emplace("componentId", component);
        }

        return executeCommand("RemoveItem", properties, false);
    }

    ActionPtr executeSetPage(const std::string& component, int value) {
        std::map<std::string, Object> properties = {};
        properties.emplace("componentId", component);
        properties.emplace("position", "relative");
        properties.emplace("value", value);

        return executeCommand("SetPage", properties, false);
    }
};

static const char *REMOVE_ITEM = R"({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "id": "main",
      "type": "Container",
      "items": [
        {
          "type": "Text",
          "id": "unique",
          "text": "Goodbye, World!"
        },
        {
          "type": "Text",
          "id": "nonUnique",
          "text": "first nonUnique"
        },
        {
          "type": "Text",
          "id": "nonUnique",
          "text": "second nonUnique"
        }
      ]
    }
  }
})";

TEST_F(CommandRemoveItemTest, RemoveItemMissingComponentId)
{
    loadDocument(REMOVE_ITEM);
    root->clearPending();

    executeRemoveItem("");

    ASSERT_TRUE(session->checkAndClear("Missing required property 'componentId' for RemoveItem"));
    ASSERT_FALSE(root->isDirty());
}

TEST_F(CommandRemoveItemTest, RemoveItemWithNonExistentComponentId)
{
    loadDocument(REMOVE_ITEM);
    root->clearPending();
    auto id = "missing";
    auto toRemove = root->findComponentById(id);
    ASSERT_EQ(toRemove, nullptr);

    executeRemoveItem(id);

    ASSERT_TRUE(session->checkAndClear("Illegal command RemoveItem: Could not resolve target 'missing'. Need to specify a valid target componentId"));
    ASSERT_FALSE(root->isDirty());
}

TEST_F(CommandRemoveItemTest, RemoveItemWithNoParent)
{
    loadDocument(REMOVE_ITEM);
    root->clearPending();
    auto id = "main";
    auto toRemove = root->findComponentById(id);
    ASSERT_NE(toRemove, nullptr);

    executeRemoveItem(id);

    ASSERT_TRUE(session->checkAndClear("Component 'main' cannot be removed"));
    ASSERT_FALSE(root->isDirty());
}

TEST_F(CommandRemoveItemTest, RemoveOnlyComponentWithGivenComponentId)
{
    loadDocument(REMOVE_ITEM);
    root->clearPending();
    auto id = "unique";
    auto toRemove = root->findComponentById(id);
    auto parent = toRemove->getParent();

    executeRemoveItem(id);

    ASSERT_EQ(toRemove->getParent(), nullptr);
    ASSERT_EQ(root->findComponentById(id), nullptr);
    ASSERT_TRUE(CheckDirtyDoNotClear(parent, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(root->isDirty());
}

TEST_F(CommandRemoveItemTest, RemoveFirstComponentWithGivenComponentId)
{
    loadDocument(REMOVE_ITEM);
    root->clearPending();
    auto id = "nonUnique";
    auto toRemove = root->findComponentById(id);
    auto parent = toRemove->getParent();

    executeRemoveItem(id);

    ASSERT_EQ(toRemove->getParent(), nullptr);
    ASSERT_TRUE(CheckDirtyDoNotClear(parent, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(root->isDirty());

    auto componentWithSameId = root->findComponentById(id);
    ASSERT_NE(componentWithSameId, nullptr);
    ASSERT_EQ(TextComponent::cast(componentWithSameId)->getValue(), "second nonUnique");
}

static const char *REMOVE_LIVEDATA = R"({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "id": "main",
      "type": "Container",
      "data": "${UnRemovableArray}",
      "items": [
        {
          "type": "Text",
          "id": "text${data}",
          "text": "${data}"
        }
      ]
    }
  }
})";

TEST_F(CommandRemoveItemTest, RemoveItemWithLiveData)
{
    auto myArray = LiveArray::create(ObjectArray{1, 2});
    config->liveData("UnRemovableArray", myArray);

    loadDocument(REMOVE_LIVEDATA);

    executeRemoveItem("text1");
    root->clearPending();

    ASSERT_TRUE(session->checkAndClear("Component 'text1' cannot be removed"));
    ASSERT_FALSE(root->isDirty());
}

static const char *REMOVE_SHRINK = R"({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": "100%",
      "height": "100%",
      "direction": "row",
      "items": [
        {
          "type": "Container",
          "id": "parent",
          "width": "auto",
          "height": "auto",
          "direction": "row",
          "shrink": 1,
          "items": [
            {
              "type": "Frame",
              "id": "frame1",
              "width": 100,
              "height": 100
            },
            {
              "type": "Frame",
              "id": "frame2",
              "width": 100,
              "height": 100
            }
          ]
        }
      ]
    }
  }
})";

TEST_F(CommandRemoveItemTest, RemoveChildCausesLayout)
{
    loadDocument(REMOVE_SHRINK);
    auto parent = root->findComponentById("parent");

    ASSERT_TRUE(parent);
    ASSERT_EQ(Rect(0,0,200,800), parent->getCalculated(apl::kPropertyBounds).get<Rect>());

    executeRemoveItem("frame1");
    root->clearPending();

    ASSERT_EQ(Rect(0,0,100,800), parent->getCalculated(apl::kPropertyBounds).get<Rect>());
}

static const char *REMOVE_MEDIA = R"apl({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": "100%",
      "height": "100%",
      "items": [
        {
          "type": "Video",
          "id": "MyVideo",
          "autoplay": true,
          "source": "track1"
        }
      ]
    }
  }
})apl";

class SinglePlayerMediaFactory : public MediaPlayerFactory {
public:
    class SingleMediaPlayer : public MediaPlayer {
    public:
        SingleMediaPlayer(MediaPlayerCallback callback) : MediaPlayer(std::move(callback)) {}

        void release() override { mReleased = true; }
        void halt() override { mHalted = true; }
        void setTrackList(std::vector<MediaTrack> tracks) override { mTracks = tracks; }
        void play(ActionRef actionRef) override { mPlaying = true; }
        void pause() override { mPlaying = false; }
        void next() override {}
        void previous() override {}
        void rewind() override {}
        void seek( int offset ) override {}
        void seekTo( int offset ) override {}
        void setTrackIndex( int trackIndex ) override {}
        void setAudioTrack( AudioTrack audioTrack ) override {}
        void setMute( bool mute ) override {}

        bool released() const { return mReleased; }
        bool halted() const { return mHalted ; }
        bool playing() const { return mPlaying; }

    private:
        bool mReleased = false;
        bool mHalted = false;
        bool mPlaying = false;
        std::vector<MediaTrack> mTracks;
    };

    MediaPlayerPtr createPlayer(MediaPlayerCallback callback) override {
        if (!mPlayer) mPlayer = std::make_shared<SingleMediaPlayer>(std::move(callback));
        return mPlayer;
    }

    MediaPlayerPtr player() const { return mPlayer; }


private:
    MediaPlayerPtr mPlayer;
};

TEST_F(CommandRemoveItemTest, RemoveStopsMediaPlayback)
{
    std::shared_ptr<SinglePlayerMediaFactory> mediaPlayerFactory = std::make_shared<SinglePlayerMediaFactory>();

    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureManageMediaRequests);
    config->mediaPlayerFactory(mediaPlayerFactory);

    loadDocument(REMOVE_MEDIA);

    ASSERT_TRUE(mediaPlayerFactory->player());
    auto player = std::static_pointer_cast<SinglePlayerMediaFactory::SingleMediaPlayer>(mediaPlayerFactory->player());

    ASSERT_TRUE(player->playing());
    ASSERT_FALSE(player->halted());

    executeRemoveItem("MyVideo");
    root->clearPending();

    ASSERT_TRUE(player->halted());
}

static const char *REMOVE_MEDIA_CHILD = R"apl({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": "100%",
      "height": "100%",
      "items": [
        {
          "type": "Container",
          "id": "MyVideoContainer",
          "width": "100%",
          "height": "100%",
          "items": [
            {
              "type": "Video",
              "autoplay": true,
              "source": "track1"
            }
          ]
        }
      ]
    }
  }
})apl";

TEST_F(CommandRemoveItemTest, RemoveStopsChildMediaPlayback)
{
    std::shared_ptr<SinglePlayerMediaFactory> mediaPlayerFactory = std::make_shared<SinglePlayerMediaFactory>();

    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureManageMediaRequests);
    config->mediaPlayerFactory(mediaPlayerFactory);

    loadDocument(REMOVE_MEDIA_CHILD);

    ASSERT_TRUE(mediaPlayerFactory->player());
    auto player = std::static_pointer_cast<SinglePlayerMediaFactory::SingleMediaPlayer>(mediaPlayerFactory->player());

    ASSERT_TRUE(player->playing());
    ASSERT_FALSE(player->halted());

    executeRemoveItem("MyVideoContainer");
    root->clearPending();

    ASSERT_TRUE(player->halted());
}

static const char *REMOVE_SPEAK_ITEM = R"apl(
{
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": "100%",
      "height": "100%",
      "items": [
        {
          "type": "Text",
          "id": "MyText",
          "speech": "URL"
        }
      ]
    }
  }
})apl";

TEST_F(CommandRemoveItemTest, RemoveStopsKaraokePlayback)
{
    factory->addFakeContent({
        {"URL", 100, 100, -1, {}} // 100 ms duration, 100 ms initial delay
    });

    loadDocument(REMOVE_SPEAK_ITEM);

    executeSpeakItem("MyText", apl::kCommandScrollAlignCenter, apl::kCommandHighlightModeLine, 230);
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    // Advance until the preroll has finished
    advanceTime(100); // This should take us to the start of speech
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kPlay));
    ASSERT_FALSE(factory->hasEvent());

    executeRemoveItem("MyText");
    root->clearPending();

    // The audio gets stopped and released.
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kPause));
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());
}

static const char *REMOVE_SPEAK_ITEM_CHILD = R"apl(
{
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "width": "100%",
      "height": "100%",
      "items": [
        {
          "type": "Container",
          "id": "MyTextContainer",
          "width": "100%",
          "height": "100%",
          "items": [
            {
              "type": "Text",
              "id": "MyText",
              "speech": "URL"
            }
          ]
        }
      ]
    }
  }
})apl";

TEST_F(CommandRemoveItemTest, RemoveStopsKaraokeChildPlayback)
{
    factory->addFakeContent({
        {"URL", 100, 100, -1, {}} // 100 ms duration, 100 ms initial delay
    });

    loadDocument(REMOVE_SPEAK_ITEM_CHILD);

    executeSpeakItem("MyText", apl::kCommandScrollAlignCenter, apl::kCommandHighlightModeLine, 230);
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    // Advance until the preroll has finished
    advanceTime(100); // This should take us to the start of speech
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kPlay));
    ASSERT_FALSE(factory->hasEvent());

    executeRemoveItem("MyTextContainer");
    root->clearPending();

    // The audio gets stopped and released.
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kPause));
    ASSERT_TRUE(CheckPlayer("URL", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());
}

static const char *FOCUS_TEST = R"({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Sequence",
      "width": "100%",
      "height": "100%",
      "items": [
        {
          "type": "TouchWrapper",
          "id": "thing1",
          "width": 100,
          "height": 100
        },
        {
          "type": "TouchWrapper",
          "id": "thing2",
          "width": 100,
          "height": 100
        }
      ]
    }
  }
})";

TEST_F(CommandRemoveItemTest, RemoveFocusedDoesStuff)
{
    loadDocument(FOCUS_TEST);
    auto thing1 = CoreComponent::cast(root->context().findComponentById("thing1"));
    auto thing2 = CoreComponent::cast(root->context().findComponentById("thing2"));
    ASSERT_TRUE(thing1);
    ASSERT_TRUE(thing2);

    ASSERT_FALSE(thing1->getState().get(kStateFocused));
    ASSERT_FALSE(thing2->getState().get(kStateFocused));

    auto& fm = root->context().focusManager();
    ASSERT_FALSE(fm.getFocus());

    fm.setFocus(thing2, true);
    ASSERT_FALSE(thing1->getState().get(kStateFocused));
    ASSERT_TRUE(thing2->getState().get(kStateFocused));
    ASSERT_EQ(thing2, fm.getFocus());

    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(thing2, event.getComponent());

    executeRemoveItem("thing2");
    root->clearPending();

    event = root->popEvent();
    ASSERT_EQ(kEventTypeFocus, event.getType());
    ASSERT_EQ(nullptr, event.getComponent());

    ASSERT_FALSE(fm.getFocus());
}

/**
 * Test using a custom MediaManager.  This one assumes that all media objects are immediately
 * available.
 */
class StaticMediaObject : public MediaObject {
public:
    StaticMediaObject(std::string url, EventMediaType type, const HeaderArray& headers)
        : mURL(std::move(url)), mType(type), mHeaders(headers) {}

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

class WeakHoldingMediaManager : public MediaManager {
public:
    MediaObjectPtr request(
        const std::string& url,
        EventMediaType type,
        const HeaderArray& headers) override {
        auto result = std::make_shared<StaticMediaObject>(url, type, headers);
        weakReferences.emplace(url, result);
        return result;
    }

    std::map<std::string, std::weak_ptr<StaticMediaObject>> weakReferences;
};

static const char* REMOVABLE_MEDIA_ELEMENTS = R"({
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

TEST_F(CommandRemoveItemTest, RemoveClearsMediaResource)
{
    auto testManager = std::make_shared<WeakHoldingMediaManager>();
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureManageMediaRequests);
    config->mediaManager(testManager);

    loadDocument(REMOVABLE_MEDIA_ELEMENTS);

    ASSERT_EQ(2, testManager->weakReferences.size());

    ASSERT_EQ(kMediaStateReady, root->findComponentById("myImage")->getCalculated(kPropertyMediaState).getInteger());
    ASSERT_EQ(kMediaStateReady, root->findComponentById("myAVG")->getCalculated(kPropertyMediaState).getInteger());

    ASSERT_TRUE(CheckDirty(root));

    executeRemoveItem("myImage");
    root->clearPending();

    // Check that image got freed
    ASSERT_FALSE(testManager->weakReferences.at("http://myImage").lock());

    executeRemoveItem("myAVG");
    root->clearPending();

    // Check that image got freed
    ASSERT_FALSE(testManager->weakReferences.at("http://myAVG").lock());
}

static const char *PAGER_TEST = R"({
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "items": {
      "type": "Container",
      "width": 500,
      "height": 500,
      "items": [
        {
          "type": "Pager",
          "id": "PapaPager",
          "width": "100%",
          "height": "100%",
          "items": [
            {
              "type": "Frame",
              "id": "frame1",
              "width": "100%",
              "height": "100%",
              "backgroundColor": "red"
            },
            {
              "type": "Frame",
              "id": "frame2",
              "width": "100%",
              "height": "100%",
              "backgroundColor": "green"
            },
            {
              "type": "Frame",
              "id": "frame3",
              "width": "100%",
              "height": "100%",
              "backgroundColor": "yellow"
            }
          ]
        }
      ]
    }
  }
})";

TEST_F(CommandRemoveItemTest, RemovePagerClearsPageAnimation)
{
    loadDocument(PAGER_TEST);

    auto actionRef = executeSetPage("PapaPager", 1);

    advanceTime(100);

    ASSERT_TRUE(actionRef->isPending());

    executeRemoveItem("PapaPager");
    root->clearPending();

    advanceTime(500);

    ASSERT_TRUE(actionRef->isTerminated());
}

TEST_F(CommandRemoveItemTest, RemovePagerChildMovedPage)
{
    loadDocument(PAGER_TEST);
    auto pager = component->getChildAt(0);

    ASSERT_EQ(0, pager->pagePosition());
    ASSERT_EQ("frame1", pager->getChildAt(0)->getId());

    executeRemoveItem("frame1");
    root->clearPending();

    ASSERT_EQ(0, pager->pagePosition());
    ASSERT_EQ("frame2", pager->getChildAt(0)->getId());
}

TEST_F(CommandRemoveItemTest, RemovePagerLastChildMovedPage)
{
    loadDocument(PAGER_TEST);
    auto actionRef = executeSetPage("PapaPager", 2);
    advanceTime(600);

    auto pager = component->getChildAt(0);

    ASSERT_EQ(2, pager->pagePosition());
    ASSERT_EQ("frame3", pager->getChildAt(2)->getId());

    executeRemoveItem("frame3");
    root->clearPending();

    ASSERT_EQ(1, pager->pagePosition());
    ASSERT_EQ("frame2", pager->getChildAt(1)->getId());
}
