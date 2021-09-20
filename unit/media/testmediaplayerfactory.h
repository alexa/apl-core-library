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

#ifndef _APL_TEST_MEDIA_PLAYER_FACTORY_H
#define _APL_TEST_MEDIA_PLAYER_FACTORY_H

#include "testmediaplayer.h"
#include "apl/media/mediaplayerfactory.h"

namespace apl {

/**
 * Fake information about a video track.
 */
struct FakeContent {
    std::string url;
    int actualDuration; // May be -1 for infinite duration
    int initialDelay;   // Initial buffering delay in milliseconds.  This applies to failed tracks as well.
    int failAfter;      // Fail after this many milliseconds.  May be 0.  Negative numbers never fail.
};

/**
 * A simulated media player factory.  This returns a simulated media player when requested.
 * It also stores information about the actual duration, buffering delay, and failure times
 * for fake video content.
 */
class TestMediaPlayerFactory : public MediaPlayerFactory,
                               public std::enable_shared_from_this<TestMediaPlayerFactory>
{
public:
    /**
     * MediaPlayerFactory interface
     */
    MediaPlayerPtr createPlayer( MediaPlayerCallback callback ) override {
        auto self = std::dynamic_pointer_cast<TestMediaPlayerFactory>(shared_from_this());
        auto player = std::make_shared<TestMediaPlayer>(std::move(callback), std::move(self));
        if (mEventCallback) player->setEventCallback(mEventCallback);
        mPlayers.emplace_back(player);
        return player;
    }

    /**
     * The test media player calls this method to retrieve the fake content information for each track.
     */
    FakeContent findContent(const std::string& url) {
        auto it = mFakeContent.find(url);
        if (it != mFakeContent.end())
            return it->second;

        // If the URL is not recognized, return fake content that fails immediately.
        return { url, 1000, 100, 0 };
    }

    /**
     * Call this method from your unit tests to add information about media tracks that the
     * test player will simulate.
     */
    void addFakeContent(const std::vector<FakeContent>& fakeContent) {
        for (const auto& m : fakeContent)
            mFakeContent.emplace(m.url, m);
    }

    /**
     * Advance the media player time.  Note that this is not the same as advancing the time
     * for normal event handling - this only affects the test media players.
     * @param milliseconds
     */
    void advanceTime(apl_duration_t milliseconds) {
        auto it = mPlayers.begin();
        while (it != mPlayers.end()) {
            auto player = it->lock();
            if (player) {
                for (auto remaining = milliseconds ; remaining > 0 ; )
                    remaining -= player->advanceByUpTo(remaining);
                it++;
            }
            else {
                it = mPlayers.erase(it);
            }
        }
    }

    void setEventCallback(TestMediaPlayer::EventCallback callback) { mEventCallback = callback; }

private:
    std::vector<std::weak_ptr<TestMediaPlayer>> mPlayers;
    std::map<std::string, FakeContent> mFakeContent;
    TestMediaPlayer::EventCallback mEventCallback;
};

} // namespace apl

#endif // _APL_TEST_MEDIA_PLAYER_FACTORY_H
