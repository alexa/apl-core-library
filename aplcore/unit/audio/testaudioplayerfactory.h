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

#ifndef _APL_TEST_AUDIO_PLAYER_FACTORY_H
#define _APL_TEST_AUDIO_PLAYER_FACTORY_H

#include <queue>

#include "testaudioplayer.h"
#include "apl/audio/audioplayerfactory.h"
#include "apl/audio/speechmark.h"

namespace apl {

/**
 * Fake information about an audio track
 */
struct FakeAudioContent {
    std::string url;
    int actualDuration; // May be -1 for infinite duration
    int initialDelay; // Initial buffering delay in milliseconds.  This applies to failed tracks as well.
    int failAfter;    // Fail after this many milliseconds.  May be 0.  Negative numbers never fail.
    std::vector<SpeechMark> speechMarks;  // Ordered series of speech marks to send out
    TextTrackArray trackArray; // TextTrack (Caption) data associated with the Track
};

class TestAudioPlayerFactory : public AudioPlayerFactory,
                               public std::enable_shared_from_this<TestAudioPlayerFactory>
{
public:
    TestAudioPlayerFactory(const TimersPtr& timers) : mTimers(timers) {}

    AudioPlayerPtr createPlayer(AudioPlayerCallback playerCallback,
                                SpeechMarkCallback speechMarkCallback) override {
        auto self = std::static_pointer_cast<TestAudioPlayerFactory>(shared_from_this());
        auto player = std::make_shared<TestAudioPlayer>(std::move(playerCallback),
                                                        std::move(speechMarkCallback),
                                                        std::move(self));
        mPlayers.emplace_back(player);
        return player;
    }

    /**
     * The test media player calls this method to retrieve the fake content information for each track.
     */
    FakeAudioContent findContent(const std::string& url) {
        auto it = mFakeContent.find(url);
        if (it != mFakeContent.end())
            return it->second;

        // If the URL is not recognized, return fake content that fails immediately.
        return {url, 1000, 100, 0, {}};
    }

    /**
     * Call this method from your unit tests to add information about media tracks that the
     * test player will simulate.
     */
    void addFakeContent(const std::vector<FakeAudioContent>& fakeContent) {
        for (const auto& m : fakeContent)
            mFakeContent.emplace(m.url, m);
    }

    const TimersPtr& timers() const { return mTimers; }

    // ******* Event recording *********

    using EventType = TestAudioPlayer::EventType;

    struct Event {
        std::string url;
        EventType eventType;
    };

    void record(const TestAudioPlayer& player, const std::string& url, EventType eventType) {
        mEvents.push(Event{url, eventType});

        // Verify that we are in the list
        auto ptr = player.shared_from_this();
        auto it = std::find_if(mPlayers.begin(), mPlayers.end(), [&](const std::weak_ptr<TestAudioPlayer>& other) {
            return !ptr.owner_before(other) && !other.owner_before(ptr);
        });
        assert(it != mPlayers.end());

        // Remove the player from the list on a release
        if (eventType == TestAudioPlayer::kRelease)
            mPlayers.erase(it);
    }

    bool hasEvent() const { return !mEvents.empty(); }

    Event popEvent() {
        assert(!mEvents.empty());
        auto result = mEvents.front();
        mEvents.pop();
        return result;
    }

    size_t playerCount() const { return mPlayers.size(); }

    const std::vector<std::weak_ptr<TestAudioPlayer>>& getPlayers() const { return mPlayers; }

private:
    std::vector<std::weak_ptr<TestAudioPlayer>> mPlayers;
    std::map<std::string, FakeAudioContent> mFakeContent;
    std::queue<Event> mEvents;
    TimersPtr mTimers;
};

} // namespace apl

#endif // _APL_TEST_AUDIO_PLAYER_FACTORY_H
