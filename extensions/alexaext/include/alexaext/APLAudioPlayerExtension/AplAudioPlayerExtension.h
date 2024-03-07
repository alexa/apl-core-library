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
 */

#ifndef APL_APLAUDIOPLAYEREXTENSION_H
#define APL_APLAUDIOPLAYEREXTENSION_H

#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <alexaext/alexaext.h>

#include <rapidjson/document.h>

#include "AplAudioPlayerExtensionObserverInterface.h"

namespace alexaext {
namespace audioplayer {

static const std::string URI = "aplext:audioplayer:10";
static const std::string ENVIRONMENT_VERSION = "APLAudioPlayerExtension-1.0";

/**
 * An APL Extension designed for bi-directional communication between an @c AudioPlayer and APL document
 * to allow for control and command of audio stream and APL UI.
 */
class AplAudioPlayerExtension
    : public alexaext::ExtensionBase,
      public std::enable_shared_from_this<AplAudioPlayerExtension> {
public:
    // forward declare
    struct ActivityState;
    /**
     * Constructor
     */
    explicit AplAudioPlayerExtension(std::shared_ptr<AplAudioPlayerExtensionObserverInterface> observer);

    ~AplAudioPlayerExtension() override = default;

    /// @name alexaext::Extension Functions
    /// @{

    rapidjson::Document createRegistration(const ActivityDescriptor& activity,
                                           const rapidjson::Value &registrationRequest) override;

    bool invokeCommand(const ActivityDescriptor& activity, const rapidjson::Value &command) override;

    void onActivityRegistered(const ActivityDescriptor& activity) override;
    void onActivityUnregistered(const ActivityDescriptor& activity) override;

    /// @}


    /**
     * Call to invoke the OnPlayerActivityUpdated ExtensionEventHandler and update the playbackState apl::LiveMap.
     * It is expected that this is called on every change in the AudioPlayer's PlayerActivity state.
     *
     * @param state The player activity state as defined in
     * https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/audioplayer.html#context
     * @param offset The current offsetInMilliseconds for the active audioItem received from
     * https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/audioplayer.html#play
     */
    void updatePlayerActivity(const std::string &state, int offset);

    /**
     * Call to update the audioItem offset property of the playbackState apl::LiveMap.
     * It is expected that this is called on every offset change (tick) from the AudioPlayer's audioItem to consistently
     * update playback progress.
     *
     * @param offset The current offsetInMilliseconds for the active audioItem received from
     * https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/audioplayer.html#play
     */
    void updatePlaybackProgress(int offset);

    /**
     * Call to update the audioItemId apl::LiveMap.
     * It is expected that this is called on every "Play" directive.
     * 
     * @param audioItemId Unique id to identify audioItem
     */
    void updateCurrentAudioItemId(const std::string& audioItemId);

    /**
     * This method will do nothing
     * @deprecated The extension generates it's own token on extension registration.
     * @param id The identifier of the active presentation session.
     * @param skillId The identifiery of the active Skill / Speechlet who owns the session.
     */
    void setActivePresentationSession(const std::string& id, const std::string& skillId);



protected:

    // Applies the settings from a RegistrationRequest
    void applySettings(const ActivityDescriptor &activity, const rapidjson::Value &settings);

    // Publishes a LiveDataUpdate
    void publishLiveData();

protected:

    /// The @c AplAudioPlayerExtensionObserverInterface observer
    std::shared_ptr<AplAudioPlayerExtensionObserverInterface> mObserver;

    std::mutex mStateMutex;
    /// The @c apl::LiveMap activity and offset for AudioPlayer playbackState data.
    std::string mPlaybackStateActivity;
    int mPlaybackStateOffset = 0;
    std::string mAudioItemId{""};
    /// The map of activity to activity state
    std::unordered_map<ActivityDescriptor,
                       std::shared_ptr<ActivityState>,
                       ActivityDescriptor::Hash> mActivityStateMap;

private:
    /**
     * Flushes the provided @c ActivityState and notifies the observer.
     * @param activityState The @c ActivityState to flush.
     */
    void flushLyricData(const std::shared_ptr<ActivityState> &activityState);

    /**
     * An internal function to retrieve the @c ActivityState object from the mActivityStateMap
     * based on the @c ActivityDescriptor. Creates a new ActivityState object if not already created.
     * @param descriptor The activity descriptor
     */
    std::shared_ptr<ActivityState> getOrCreateActivityState(const ActivityDescriptor& activity);
};

using AplAudioPlayerExtensionPtr = std::shared_ptr<AplAudioPlayerExtension>;

}  // namespace audioplayer
}  // namespace alexaext

#endif  // APL_APLAUDIOPLAYEREXTENSION_H