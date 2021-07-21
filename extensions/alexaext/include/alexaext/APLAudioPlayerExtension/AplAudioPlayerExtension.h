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

#ifndef ALEXA_SMART_SCREEN_SDK_APPLICATIONUTILITIES_APL_EXTENSIONS_AUDIOPLAYER_APLAUDIOPLAYEREXTENSION_H
#define ALEXA_SMART_SCREEN_SDK_APPLICATIONUTILITIES_APL_EXTENSIONS_AUDIOPLAYER_APLAUDIOPLAYEREXTENSION_H

#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>

#include <alexaext/alexaext.h>

#include <rapidjson/document.h>

#include "AplAudioPlayerExtensionObserverInterface.h"

namespace AudioPlayer {

static const std::string URI = "aplext:audioplayer:10";
static const std::string ENVIRONMENT_VERSION = "APLAudioPlayerExtension-1.0";

/**
 * An APL Extension designed for bi-directional communication between an @c AudioPlayer and APL document
 * to allow for control and command of audio stream and APL UI.
 *
 * TODO : Add link to public spec when available -
 * https://aplspec.aka.corp.amazon.com/apl-extensions-release/html/extensions/audioplayer/audioplayer_extension_10.html
 */
class AplAudioPlayerExtension
        : public alexaext::ExtensionBase, public std::enable_shared_from_this<AplAudioPlayerExtension> {
public:
    /**
     * Constructor
     */
    explicit AplAudioPlayerExtension(std::shared_ptr<AplAudioPlayerExtensionObserverInterface> observer);

    virtual ~AplAudioPlayerExtension() = default;

    /// @name alexaext::Extension Functions
    /// @{

    rapidjson::Document createRegistration(const std::string &uri,
                                           const rapidjson::Value &registrationRequest) override;

    bool invokeCommand(const std::string &uri, const rapidjson::Value &command) override;

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
     * Used to inform the extension of the active @c AudioPlayer.Presentation.APL presentationSession.
     * @deprecated The extension generates it's own token on extension registration.
     * @param id The identifier of the active presentation session.
     * @param skillId The identifiery of the active Skill / Speechlet who owns the session.
     */
    void setActivePresentationSession(const std::string& id, const std::string& skillId);

    /**
     * Utility object for tracking lyrics viewed data.
     */
    struct LyricsViewedData {
        /**
         * Default Constructor.
         */
        LyricsViewedData() = default;

        /**
         * Constructor.
         *
         * @param token the identifier of the track displaying lyrics.
         */
        explicit LyricsViewedData(std::string token) : token{std::move(token)}
        {
            lyricData = std::make_shared<rapidjson::Document>();
            lyricData->SetArray();
        };

        /// The identifier of the track displaying lyrics.
        std::string token;

        /// The total time in milliseconds that lyrics were viewed.
        long durationInMilliseconds{};

        /// The lyrics viewed data array.
        std::shared_ptr<rapidjson::Document> lyricData;

        /**
         * Add Lyric lines to the data array.
         * @param lines The lines of lyrics to append.
         */
        void addLyricLinesData(const rapidjson::Value &lyricLines)
        {
            using namespace rapidjson;
            if (!lyricLines.IsArray())
                return;

            auto &alloc = lyricData->GetAllocator();
            for (auto &line: lyricLines.GetArray()) {
                // verify data integrity and adjust
                // received line data from double format, store the int values
                std::string text = alexaext::GetWithDefault<const char *>("text", line, "");
                auto startTime = alexaext::GetWithDefault<int>("startTime", line, -1);
                auto endTime = alexaext::GetWithDefault<int>("endTime", line, -1);
                if (text.empty() || startTime < 0 || endTime < 0 || endTime < startTime)
                    continue;
                Value value(kObjectType);
                value.AddMember("text", Value(text.c_str(), alloc).Move(), alloc)
                     .AddMember("startTime", startTime, alloc)
                     .AddMember("endTime", endTime, alloc);
                lyricData->PushBack(value.Move(), alloc);
            }
        }

        /**
         * Resets the LyricsData object
         */
        void reset()
        {
            token = "";
            durationInMilliseconds = 0;
            lyricData = std::make_shared<rapidjson::Document>();
            lyricData->SetArray();
        }

        /**
         * Returns string payload of the lyricData object.
         * @return the lyricData object payload.
         */
        std::string getLyricDataPayload() const
        {
            return alexaext::AsString(*lyricData);
        }
    };

    /**
     * An internal function to retrieve the active @c LyricsViewedData object from the m_lyricsViewedData map
     * based on the m_activeSkillId;
     * @param initIfNull If true this will initialize a @c LyricsViewedData object for the m_activeSkillId if none exists.
     * @param token - The token for the track actively displaying lyrics.
     */
    std::shared_ptr<LyricsViewedData> getActiveLyricsViewedData(bool initIfNull = false, const std::string &token = "");

    /**
     * Flushes the provided @c LyricsViewedData and notifies the observer.
     * @param lyricsViewedData The @c LyricsViewedData to flush.
     */
    void flushLyricData(const std::shared_ptr<LyricsViewedData> &lyricsViewedData);

protected:

    // Applies the settings from a RegistrationRequest
    void applySettings(const rapidjson::Value &settings);

    // Publishes a LiveDataUpdate
    void publishLiveData();

protected:

    /// The @c AplAudioPlayerExtensionObserverInterface observer
    std::shared_ptr<AplAudioPlayerExtensionObserverInterface> mObserver;

    /// The document settings defined 'name' for the playbackState data object
    std::string mPlaybackStateName;

    /// The @c apl::LiveMap ativity and offset for AudioPlayer playbackState data.
    std::string mPlaybackStateActivity;
    int mPlaybackStateOffset;

    /// The id of the active skill in session.
    std::string mActiveClientToken;

    /// The map of @c LyricsViewedData objects per skill Id.
    std::unordered_map<std::string, std::shared_ptr<LyricsViewedData>> mLyricsViewedData;
};

using AplAudioPlayerExtensionPtr = std::shared_ptr<AplAudioPlayerExtension>;

}  // namespace AudioPlayer


#endif  // ALEXA_SMART_SCREEN_SDK_APPLICATIONUTILITIES_APL_EXTENSIONS_AUDIOPLAYER_APLAUDIOPLAYEREXTENSION_H