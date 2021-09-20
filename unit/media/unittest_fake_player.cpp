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
#include "fakeplayer.h"

using namespace apl;

/**
 * The FakePlayer mock class is complicated enough that it deserves a few unit tests of its own.
 */
class FakePlayerTest : public ::testing::Test {};

::testing::AssertionResult
CheckAdvance( FakePlayer::FakeEvent event, int advanceTime, std::pair<FakePlayer::FakeEvent, int> result )
{
    if (event != result.first)
        return ::testing::AssertionFailure()
               << "event mismatch was=" << result.first << " expected=" << event;

    if (advanceTime != result.second)
        return ::testing::AssertionFailure()
                << "advance time mismatch was=" << result.second << " expected=" << advanceTime;

    return ::testing::AssertionSuccess();
}

TEST_F(FakePlayerTest, Basic)
{
    auto fakePlayer = FakePlayer::create(MediaTrack{"https://foo.com",
                                                    0,  // offset
                                                    0,  // duration
                                                    0}, // repeat count
                                         1000,          // Actual duration
                                         100,           // Initial delay
                                         -1);           // Fail after

    ASSERT_EQ(0, fakePlayer->getPosition());
    ASSERT_EQ(0, fakePlayer->getDuration());
    ASSERT_EQ(FakePlayer::IDLE, fakePlayer->getState());
    ASSERT_TRUE(fakePlayer->active());
    ASSERT_FALSE(fakePlayer->isPlaying());
    ASSERT_TRUE(fakePlayer->atStart());

    // We are not playing yet.  Advance time - this should finish buffering and stop.
    ASSERT_TRUE(CheckAdvance(FakePlayer::TRACK_READY, 100, fakePlayer->advanceTime(1000)));
    ASSERT_EQ(0, fakePlayer->getPosition());

    // Do it again.  Time passes, but nothing gets reported
    ASSERT_TRUE(CheckAdvance(FakePlayer::NO_REPORT, 1000, fakePlayer->advanceTime(1000)));
    ASSERT_EQ(0, fakePlayer->getPosition());

    // Now start playing
    fakePlayer->play();
    ASSERT_TRUE(fakePlayer->isPlaying());

    // Move ahead by 100 milliseconds
    ASSERT_TRUE(CheckAdvance(FakePlayer::TIME_UPDATE, 100, fakePlayer->advanceTime(100)));
    ASSERT_EQ(100, fakePlayer->getPosition());

    // Move ahead by another 100 milliseconds
    ASSERT_TRUE(CheckAdvance(FakePlayer::TIME_UPDATE, 100, fakePlayer->advanceTime(100)));
    ASSERT_EQ(200, fakePlayer->getPosition());

    // Move the final 800 milliseconds
    ASSERT_TRUE(CheckAdvance(FakePlayer::TRACK_DONE, 800, fakePlayer->advanceTime(800)));
    ASSERT_EQ(1000, fakePlayer->getPosition());
    ASSERT_EQ(FakePlayer::DONE, fakePlayer->getState());
    ASSERT_FALSE(fakePlayer->active());
    ASSERT_FALSE(fakePlayer->isPlaying());
    ASSERT_FALSE(fakePlayer->atStart());
}

TEST_F(FakePlayerTest, BasicWithPause)
{
    auto fakePlayer = FakePlayer::create(MediaTrack{"https://foo.com",
                                                    0,  // offset
                                                    0,  // duration
                                                    0}, // repeat count
                                         1000,          // Actual duration
                                         100,           // Initial delay
                                         -1);           // Fail after

    // Start playing immediately (before the buffering time has passed)
    fakePlayer->play();

    // We get a TRACK_READY message after things have buffered
    ASSERT_TRUE(CheckAdvance(FakePlayer::TRACK_READY, 100, fakePlayer->advanceTime(1000)));
    ASSERT_EQ(0, fakePlayer->getPosition());

    // Run for another 500 ms
    ASSERT_TRUE(CheckAdvance(FakePlayer::TIME_UPDATE, 500, fakePlayer->advanceTime(500)));
    ASSERT_EQ(500, fakePlayer->getPosition());

    // Pause playback
    fakePlayer->pause();

    // Run for another 500 ms - nothing should happen
    ASSERT_TRUE(CheckAdvance(FakePlayer::NO_REPORT, 500, fakePlayer->advanceTime(500)));
    ASSERT_EQ(500, fakePlayer->getPosition());

    // Start playing again and finish
    fakePlayer->play();
    ASSERT_TRUE(CheckAdvance(FakePlayer::TRACK_DONE, 500, fakePlayer->advanceTime(1000)));
    ASSERT_EQ(1000, fakePlayer->getPosition());
}

TEST_F(FakePlayerTest, Complex)
{
    auto fakePlayer = FakePlayer::create(MediaTrack{"https://foo.com",
                                                    150, // offset
                                                    0,   // duration
                                                    2},  // repeat count
                                         1000, // Actual duration. Note that we start offset by 150
                                         100,  // Initial delay
                                         -1);  // Fail after

    // Start playing immediately (before the buffering time has passed)
    fakePlayer->play();

    // We get a TRACK_READY message after things have buffered
    ASSERT_TRUE(CheckAdvance(FakePlayer::TRACK_READY, 100, fakePlayer->advanceTime(1000)));
    ASSERT_EQ(150, fakePlayer->getPosition()); // Start at the offset

    // Run for 500 ms
    ASSERT_TRUE(CheckAdvance(FakePlayer::TIME_UPDATE, 500, fakePlayer->advanceTime(500)));
    ASSERT_EQ(650, fakePlayer->getPosition());

    // Run for another 500 ms.  This should wrap once
    ASSERT_TRUE(CheckAdvance(FakePlayer::TIME_UPDATE, 500, fakePlayer->advanceTime(500)));
    ASSERT_EQ(300, fakePlayer->getPosition());

    // Run out the clock
    ASSERT_TRUE(
        CheckAdvance(FakePlayer::TRACK_DONE, 850 * 3 - 1000, fakePlayer->advanceTime(2000)));
    ASSERT_EQ(1000, fakePlayer->getPosition());

    // Rewind back to the beginning
    fakePlayer->rewind();
    ASSERT_EQ(FakePlayer::IDLE, fakePlayer->getState());
    ASSERT_EQ(150, fakePlayer->getPosition());

    fakePlayer->play();

    // This time there is no buffering required
    ASSERT_TRUE(CheckAdvance(FakePlayer::TIME_UPDATE, 1000, fakePlayer->advanceTime(1000)));
    ASSERT_EQ(300, fakePlayer->getPosition());

    // Run out the clock
    ASSERT_TRUE(
        CheckAdvance(FakePlayer::TRACK_DONE, 850 * 3 - 1000, fakePlayer->advanceTime(2000)));
    ASSERT_EQ(1000, fakePlayer->getPosition());
}

TEST_F(FakePlayerTest, Fail)
{
    auto fakePlayer = FakePlayer::create(MediaTrack{"https://foo.com",
                                                    500, // offset
                                                    500, // duration
                                                    -1}, // repeat count
                                         -1,             // Actual duration = infinite
                                         0,              // Initial delay
                                         1200);          // Fail after

    // Start playing immediately (before the buffering time has passed)
    fakePlayer->play();

    // We get a TRACK_READY message after things have buffered
    ASSERT_TRUE(CheckAdvance(FakePlayer::TRACK_READY, 0, fakePlayer->advanceTime(1000)));
    ASSERT_EQ(500, fakePlayer->getPosition()); // Start at the offset

    // Run for 500 ms.  This loops immediately
    ASSERT_TRUE(CheckAdvance(FakePlayer::TIME_UPDATE, 500, fakePlayer->advanceTime(500)));
    ASSERT_EQ(500, fakePlayer->getPosition());

    // Run for 500 ms.  This loops a second time
    ASSERT_TRUE(CheckAdvance(FakePlayer::TIME_UPDATE, 500, fakePlayer->advanceTime(500)));
    ASSERT_EQ(500, fakePlayer->getPosition());

    // The third time fails
    ASSERT_TRUE(CheckAdvance(FakePlayer::TRACK_FAIL, 200, fakePlayer->advanceTime(500)));
    ASSERT_EQ(700, fakePlayer->getPosition());
    ASSERT_FALSE(fakePlayer->active());

    // Rewinding doesn't do anything
    fakePlayer->rewind();
    ASSERT_EQ(700, fakePlayer->getPosition());
    ASSERT_FALSE(fakePlayer->active());
}

TEST_F(FakePlayerTest, FailImmediately)
{
    auto fakePlayer = FakePlayer::create(MediaTrack{"https://foo.com",
                                                    500, // offset
                                                    500, // duration
                                                    -1}, // repeat count
                                         -1,             // Actual duration = infinite
                                         100,            // Initial delay
                                         0);             // Fail immediately

    // Start playing immediately (before the buffering time has passed)
    fakePlayer->play();

    // We get a TRACK_FAIL message after the buffering time
    ASSERT_TRUE(CheckAdvance(FakePlayer::TRACK_FAIL, 100, fakePlayer->advanceTime(1000)));

    // Once failed, nothing works
    ASSERT_FALSE(fakePlayer->play());
    ASSERT_FALSE(fakePlayer->pause());
    ASSERT_FALSE(fakePlayer->rewind());
    ASSERT_FALSE(fakePlayer->finish());
    ASSERT_FALSE(fakePlayer->seek(0));
    ASSERT_FALSE(fakePlayer->clearRepeat());
}

TEST_F(FakePlayerTest, ZeroDuration)
{
    auto fakePlayer = FakePlayer::create(MediaTrack{"https://foo.com",
                                                    500, // offset
                                                    500, // duration
                                                    -1}, // repeat count
                                         500,            // Actual duration <= offset
                                         0,              // Initial delay
                                         -1);            // Fail immediately

    ASSERT_TRUE(fakePlayer->play());

    // We get a TRACK_DONE immediately
    ASSERT_TRUE(CheckAdvance(FakePlayer::TRACK_READY, 0, fakePlayer->advanceTime(1000)));
    ASSERT_TRUE(CheckAdvance(FakePlayer::TRACK_DONE, 0, fakePlayer->advanceTime(1000)));

    // Once done, many things don't work
    ASSERT_FALSE(fakePlayer->play());
    ASSERT_FALSE(fakePlayer->pause());
    ASSERT_FALSE(fakePlayer->rewind());
    ASSERT_FALSE(fakePlayer->finish());
    ASSERT_FALSE(fakePlayer->seek(0));
    ASSERT_FALSE(fakePlayer->seek(100));
    ASSERT_FALSE(fakePlayer->clearRepeat());
}

TEST_F(FakePlayerTest, BothDurationsClippedToActual)
{
    auto fakePlayer = FakePlayer::create(MediaTrack{"https://foo.com",
                                                    500, // offset
                                                    500, // duration
                                                    0},  // repeat count
                                         750,            // Actual duration
                                         0,              // Initial delay
                                         -1);            // Fail after

    // Start playing immediately (before the buffering time has passed)
    fakePlayer->play();

    // TRACK_READY immediately
    ASSERT_TRUE(CheckAdvance(FakePlayer::TRACK_READY, 0, fakePlayer->advanceTime(1000)));
    ASSERT_EQ(500, fakePlayer->getPosition()); // Start at the offset

    // Run for 1000 ms.  This should finish
    ASSERT_TRUE(CheckAdvance(FakePlayer::TRACK_DONE, 250, fakePlayer->advanceTime(1000)));
    ASSERT_EQ(750, fakePlayer->getPosition());
}

TEST_F(FakePlayerTest, BothDurationsClippedToRequested)
{
    auto fakePlayer = FakePlayer::create(MediaTrack{"https://foo.com",
                                                    500, // offset
                                                    500, // duration
                                                    0},  // repeat count
                                         1250,           // Actual duration
                                         0,              // Initial delay
                                         -1);            // Fail after

    // Start playing immediately (before the buffering time has passed)
    fakePlayer->play();

    // TRACK_READY immediately
    ASSERT_TRUE(CheckAdvance(FakePlayer::TRACK_READY, 0, fakePlayer->advanceTime(1000)));
    ASSERT_EQ(500, fakePlayer->getPosition()); // Start at the offset

    // Run for 1000 ms.  This should finish
    ASSERT_TRUE(CheckAdvance(FakePlayer::TRACK_DONE, 500, fakePlayer->advanceTime(1000)));
    ASSERT_EQ(1000, fakePlayer->getPosition());
}

TEST_F(FakePlayerTest, BothDurationsInfinite)
{
    auto fakePlayer = FakePlayer::create(MediaTrack{"https://foo.com",
                                                    500, // offset
                                                    0,   // duration
                                                    0},  // repeat count
                                         -1,             // Actual duration
                                         0,              // Initial delay
                                         -1);            // Fail after

    // Start playing immediately (before the buffering time has passed)
    fakePlayer->play();

    // TRACK_READY immediately
    ASSERT_TRUE(CheckAdvance(FakePlayer::TRACK_READY, 0, fakePlayer->advanceTime(1000)));
    ASSERT_EQ(500, fakePlayer->getPosition()); // Start at the offset

    // Run for 1000 ms.  This should update
    ASSERT_TRUE(CheckAdvance(FakePlayer::TIME_UPDATE, 1000, fakePlayer->advanceTime(1000)));
    ASSERT_EQ(1500, fakePlayer->getPosition());

    // Run for 1000 ms.  This should update
    ASSERT_TRUE(CheckAdvance(FakePlayer::TIME_UPDATE, 1000, fakePlayer->advanceTime(1000)));
    ASSERT_EQ(2500, fakePlayer->getPosition());

    // Run for 1000 ms.  This should update
    ASSERT_TRUE(CheckAdvance(FakePlayer::TIME_UPDATE, 1000, fakePlayer->advanceTime(1000)));
    ASSERT_EQ(3500, fakePlayer->getPosition());
}

TEST_F(FakePlayerTest, ClippedTrack)
{
    auto fakePlayer = FakePlayer::create(MediaTrack{"https://foo.com",
                                                    500, // offset
                                                    0,   // duration
                                                    0},  // repeat count
                                         200,            // Actual duration
                                         0,              // Initial delay
                                         -1);            // Fail after

    // Start playing immediately (before the buffering time has passed)
    fakePlayer->play();

    // TRACK_READY immediately
    ASSERT_TRUE(CheckAdvance(FakePlayer::TRACK_READY, 0, fakePlayer->advanceTime(1000)));
    ASSERT_EQ(200, fakePlayer->getPosition()); // Start at the end of the video

    // Run for 1000 ms.  This should immediately return DONE
    ASSERT_TRUE(CheckAdvance(FakePlayer::TRACK_DONE, 0, fakePlayer->advanceTime(1000)));
    ASSERT_EQ(200, fakePlayer->getPosition());
}

TEST_F(FakePlayerTest, SeekTests)
{
    auto fakePlayer = FakePlayer::create(MediaTrack{"https://foo.com",
                                                    0,  // offset
                                                    0,  // duration
                                                    2}, // repeat count
                                         1000,          // Actual duration
                                         0,             // Initial delay
                                         -1);           // Fail after

    ASSERT_TRUE(fakePlayer->play());

    // The track is ready immediately
    ASSERT_TRUE(CheckAdvance(FakePlayer::TRACK_READY, 0, fakePlayer->advanceTime(1000)));
    ASSERT_TRUE(CheckAdvance(FakePlayer::TIME_UPDATE, 500, fakePlayer->advanceTime(500)));

    // Seek to the same location
    ASSERT_FALSE(fakePlayer->seek(500));
    ASSERT_TRUE(!fakePlayer->isPlaying());
    ASSERT_EQ(500, fakePlayer->getPosition());

    // Start playing again and hit the first repeat
    ASSERT_TRUE(fakePlayer->play());
    ASSERT_TRUE(CheckAdvance(FakePlayer::TIME_UPDATE, 500, fakePlayer->advanceTime(500)));

    // Seek earlier than the beginning
    fakePlayer->seek(-1000);
    ASSERT_EQ(0, fakePlayer->getPosition());

    // Seek past the end.  This clips to the length of the track, but leaves the play head at the
    // end.
    fakePlayer->seek(2000);
    ASSERT_EQ(1000, fakePlayer->getPosition());

    // Start playing again and wrap around just a little bit.
    fakePlayer->play();
    ASSERT_TRUE(CheckAdvance(FakePlayer::TIME_UPDATE, 100, fakePlayer->advanceTime(100)));

    // Now seek to the end - we should be marked as done
    fakePlayer->seek(1000);
    ASSERT_EQ(1000, fakePlayer->getPosition());
    ASSERT_TRUE(fakePlayer->getState() == FakePlayer::DONE);
}

TEST_F(FakePlayerTest, InfiniteZero)
{
    auto fakePlayer = FakePlayer::create(MediaTrack{"https://foo.com",
                                                    2000, // offset
                                                    0,    // duration
                                                    -1},  // repeat count
                                         1000,            // Actual duration
                                         0,               // Initial delay
                                         -1);             // Fail after

    ASSERT_TRUE(fakePlayer->play());
    ASSERT_TRUE(CheckAdvance(FakePlayer::TRACK_READY, 0, fakePlayer->advanceTime(1000)));
    ASSERT_TRUE(CheckAdvance(FakePlayer::TRACK_DONE, 0, fakePlayer->advanceTime(500)));
}
