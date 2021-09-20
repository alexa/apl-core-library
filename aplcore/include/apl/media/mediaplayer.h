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

#ifndef _APL_MEDIA_PLAYER_H
#define _APL_MEDIA_PLAYER_H

#include <vector>
#include "apl/media/mediaobject.h"
#include "apl/primitives/mediastate.h"

namespace apl {

/**
 * A description of a media track to be played by the media player.
 */
struct MediaTrack {
    std::string url;  // Source of the video clip
    int offset;       // Starting offset within the media object, in milliseconds
    int duration;     // Duration from the starting offset to play.  Set this to a large number to play the whole track.
    int repeatCount;  // Number of times to repeat this track before moving to the next. Negative numbers repeat forever.
};

enum MediaPlayerEventType {
    kMediaPlayerEventEnd,
    kMediaPlayerEventPause,
    kMediaPlayerEventPlay,
    kMediaPlayerEventTimeUpdate,
    kMediaPlayerEventTrackUpdate,
    kMediaPlayerEventTrackReady,
    kMediaPlayerEventTrackFail
};

extern std::map<MediaPlayerEventType, std::string> sMediaPlayerEventTypeMap;

/**
 * The media player callback should be executed by the view host in a thread-safe manner.
 * Pass in the event type, the current state of the media object, and a fast mode flag.
 */
using MediaPlayerCallback = std::function<void(MediaPlayerEventType, const MediaState&)>;

/**
 * The public interface to the media player.
 *
 * This is an abstract class.  The view host should implement this class in a thread-safe manner.
 * These methods are intended to be used by the core engine and should not be called by the view host.
 */
class MediaPlayer {
public:
    explicit MediaPlayer(MediaPlayerCallback callback) : mCallback(std::move(callback)) {}

    virtual ~MediaPlayer() = default;

    /**
     * Release this media player and associated resources.  After this method is called, the
     * media player should not respond to commands from the core or from the view host.  In other
     * words, it should just stop.
     */
    virtual void release() = 0;

    /**
     * Halt all activity on this media player, but keep the track list and current position.
     * This method is used when a VideoComponent is detached from the DOM.  It should not invoke
     * the MediaPlayerCallback.
     */
    virtual void halt() = 0;

    /**
     * Pause video playback and set new video tracks.
     *
     * Events:  onPause, onTrackUpdate
     *
     * @param tracks An array of media tracks
     */
    virtual void setTrackList(std::vector<MediaTrack>) = 0;

    /**
     * Start or resume playing at the current track and offset
     * This is ignored if: There are no defined tracks or the player is not playing or the
     * player is at the end of the final track and has no repeats or the player has been released.
     *
     * Events: onPlay
     * @param action An optional action reference to resolve when finished.  Resolve this immediately
     *               if not using foreground audio.
     */
    virtual void play(ActionRef actionRef) = 0;

    /**
     * Pause video playback.
     * This is ignored if there are no defined tracks or the player is not currently playing or
     * the player has been released
     *
     * Events: onPause
     */
    virtual void pause() = 0;

    /**
     * Pause video playback and move to the start of the next video track.  If the video player
     * is already on the last video track, the video seek position will be moved to the end
     * of the last video track and the repeat counter will be zeroed.
     *
     * If the media is playing, we issue the "onPause" event.
     * If the track changes, we issue the "onTrackUpdate" event
     * If the track doesn't change but we jump to the end, we issue "onTimeUpdate"
     */
    virtual void next() = 0;

    /**
     * Pause video playback and move to the start of the previous video track.  If the video
     * player is already on the first video track, the video seek position will be moved to the
     * start of the first video track and the repeat counter will be reloaded.
     *
     * If the media is playing, we issue the "onPause" event.
     * If the track changes, we issue the "onTrackUpdate" event
     * If the track doesn't change but we jump to the start, we issue "onTimeUpdate"
     */
    virtual void previous() = 0;

    /**
     * Pause video playback and reload the current video track.  The repeat counter for the video
     * track is reloaded.
     *
     * Events: onPause, onTimeUpdate
     */
    virtual void rewind() = 0;

    /**
     * Pause video playback and change the position of the player.  The offset is relative to the
     * current track offset.  The final position will be clipped to the duration of the track.
     * The repeat counter is not changed.
     *
     * Events: onPause, onTimeUpdate
     *
     * @param offset Offset in milliseconds
     */
    virtual void seek( int offset ) = 0;

    /**
     * Pause video playback and change the current track.  This command is ignored if the trackIndex
     * is not valid (but the video will pause).  The repeat counter for the track is reloaded even
     * if the track doesn't change.
     *
     * Events: onPause, onTrackUpdate, onTimeUpdate(?)
     *
     * @param trackIndex
     */
    virtual void setTrackIndex( int trackIndex ) = 0;

    /**
     * Configure the audio output to one of foreground audio (overlaps speech), background audio,
     * or no audio.
     * @param audioTrack The audio track to play on.
     */
    virtual void setAudioTrack( AudioTrack audioTrack ) = 0;

protected:
    MediaPlayerCallback mCallback;
};

} // namespace apl

#endif // _APL_MEDIA_PLAYER_H
