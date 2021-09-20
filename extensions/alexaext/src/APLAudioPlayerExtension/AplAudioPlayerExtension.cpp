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

#include <algorithm>
#include <string>
#include <list>
#include <utility>

#include <rapidjson/document.h>

#include "alexaext/APLAudioPlayerExtension/AplAudioPlayerExtension.h"

using namespace AudioPlayer;
using namespace alexaext;

/// String to identify log entries originating from this file.
static const char *TAG("AplAudioPlayerExtension");

// version of the extension definition Schema
static const std::string SCHEMA_VERSION = "1.0";

// Settings read on registration
static const char *SETTING_PLAYBACK_STATE_NAME = "playbackStateName";

// Commands
static const char *COMMAND_PLAY_NAME = "Play";
static const char *COMMAND_PAUSE_NAME = "Pause";
static const char *COMMAND_PREVIOUS_NAME = "Previous";
static const char *COMMAND_NEXT_NAME = "Next";
static const char *COMMAND_SEEK_TO_POSITION_NAME = "SeekToPosition";
static const char *COMMAND_TOGGLE_NAME = "Toggle";
static const char *COMMAND_ADD_LYRICS_VIEWED = "AddLyricsViewed";
static const char *COMMAND_ADD_LYRICS_DURATION_IN_MILLISECONDS = "AddLyricsDurationInMilliseconds";
static const char *COMMAND_FLUSH_LYRIC_DATA = "FlushLyricData";
static const char *COMMAND_SKIP_FORWARD_NAME = "SkipForward";
static const char *COMMAND_SKIP_BACKWARD_NAME = "SkipBackward";

// Events
static const char *EVENTHANDLER_ON_PLAYER_ACTIVITY_UPDATED_NAME = "OnPlayerActivityUpdated";

// Data Types
static const char *DATA_TYPE_PLAYBACK_STATE = "playbackState";   // spec named
static const char *PROPERTY_PLAYER_ACTIVITY = "playerActivity";
static const char *PROPERTY_OFFSET = "offset";
static const char *DATA_TYPE_SEEK_POSITION = "SeekToPositionType";
static const char *DATA_TYPE_TOGGLE = "ToggleType";
static const char *PROPERTY_TOGGLE_NAME = "name";
static const char *PROPERTY_TOGGLE_CHECKED = "checked";
static const char *DATA_TYPE_LYRIC = "Lyric";
static const char *PROPERTY_LYRIC_TEXT = "text";
static const char *PROPERTY_LYRIC_START_TIME = "startTime";
static const char *PROPERTY_LYRIC_END_TIME = "endTime";
static const char *DATA_TYPE_ADD_LYRICS_VIEWED = "AddLyricsViewedType";
static const char *PROPERTY_TOKEN = "token";
static const char *PROPERTY_LINES = "lines";
static const char *DATA_TYPE_ADD_LYRICS_DURATION = "AddLyricsDurationType";
static const char *PROPERTY_DURATION_IN_MILLISECONDS = "durationInMilliseconds";

/// List of accepted toggle command names.
static const std::vector<std::string> TOGGLE_COMMAND_NAMES = {
        "thumbsUp",
        "thumbsDown",
        "shuffle",
        "repeat"
};

/// List of accepted player activity.
static const std::vector<std::string> PLAYER_ACTIVITY = {
        "PLAYING",
        "STOPPED",
        "PAUSED",
        "BUFFER_UNDERRUN"
};

// Registration token for unique client identifier
// TODO use UID gen
static std::atomic_int sToken(53);

AplAudioPlayerExtension::AplAudioPlayerExtension(std::shared_ptr<AplAudioPlayerExtensionObserverInterface> observer)
        : alexaext::ExtensionBase(URI), mObserver(std::move(observer))
{
    mPlaybackStateName = "";
    mActiveClientToken = "";
    mPlaybackStateActivity = "STOPPED";
    mPlaybackStateOffset = 0;
}

void
AplAudioPlayerExtension::applySettings(const rapidjson::Value &settings)
{
    // Reset to defaults
    mPlaybackStateName = "";
    if (!settings.IsObject())
        return;

    /// Apply document assigned settings
    auto playbackStateName = settings.FindMember(SETTING_PLAYBACK_STATE_NAME);
    if (playbackStateName != settings.MemberEnd() && playbackStateName->value.IsString()) {
        mPlaybackStateName = settings[SETTING_PLAYBACK_STATE_NAME].GetString();
    }
}

rapidjson::Document
AplAudioPlayerExtension::createRegistration(const std::string &uri,
                                            const rapidjson::Value &registrationRequest)
{
    if (uri != URI) {
        return  RegistrationFailure::forUnknownURI(uri);
    }

    // extract document assigned settings
    const auto *settingsValue = RegistrationRequest::SETTINGS().Get(registrationRequest);
    if (settingsValue)
        applySettings(*settingsValue);

    // session token
    auto clientToken = TAG + std::to_string(sToken++);
    setActivePresentationSession(clientToken, clientToken);

    // return success with the schema and environment
    return RegistrationSuccess(SCHEMA_VERSION)
            .uri(URI)
            .token(clientToken)
            .environment([](Environment &environment) {
                environment.version(ENVIRONMENT_VERSION);
            })
            .schema("1.0", [&](ExtensionSchema &schema) {
                schema.uri(URI)
                      .dataType(DATA_TYPE_PLAYBACK_STATE, [](TypeSchema &dataTypeSchema) {
                          dataTypeSchema
                            .property(PROPERTY_PLAYER_ACTIVITY, "string")
                            .property(PROPERTY_OFFSET, "number");
                      })
                      .dataType(DATA_TYPE_SEEK_POSITION, [](TypeSchema &dataTypeSchema) {
                          dataTypeSchema.property(PROPERTY_OFFSET, [](TypePropertySchema &propertySchema) {
                              propertySchema.type("number")
                                            .required(true)
                                            .defaultValue(0);
                          });
                      })
                      .dataType(DATA_TYPE_TOGGLE, [](TypeSchema &dataTypeSchema) {
                          dataTypeSchema.property(PROPERTY_TOGGLE_NAME, [](TypePropertySchema &propertySchema) {
                                            propertySchema.type("string")
                                                          .required(true)
                                                          .defaultValue("");
                                        })
                                        .property(PROPERTY_TOGGLE_CHECKED, [](TypePropertySchema &propertySchema) {
                                            propertySchema.type("bool")
                                                          .required(true)
                                                          .defaultValue(false);
                                        });
                      })
                      .dataType(DATA_TYPE_LYRIC, [](TypeSchema &dataTypeSchema) {
                          dataTypeSchema.property(PROPERTY_LYRIC_TEXT, [](TypePropertySchema &propertySchema) {
                                            propertySchema.type("string")
                                                          .required(true)
                                                          .defaultValue("");
                                        })
                                        .property(PROPERTY_LYRIC_START_TIME, [](TypePropertySchema &propertySchema) {
                                            propertySchema.type("number")
                                                          .required(true)
                                                          .defaultValue(0);
                                        })
                                        .property(PROPERTY_LYRIC_END_TIME, [](TypePropertySchema &propertySchema) {
                                            propertySchema.type("number")
                                                          .required(true)
                                                          .defaultValue(0);
                                        });
                      })
                      .dataType(DATA_TYPE_ADD_LYRICS_VIEWED, [](TypeSchema &dataTypeSchema) {
                          dataTypeSchema.property(PROPERTY_TOKEN, [](TypePropertySchema &propertySchema) {
                                            propertySchema.type("string")
                                                          .required(true)
                                                          .defaultValue("");
                                        })
                                        .property(PROPERTY_LINES, [](TypePropertySchema &propertySchema) {
                                            propertySchema.type("array");
                                        });
                      })
                      .dataType(DATA_TYPE_ADD_LYRICS_DURATION, [](TypeSchema &dataTypeSchema) {
                          dataTypeSchema.property(PROPERTY_TOKEN, [](TypePropertySchema &propertySchema) {
                                            propertySchema.type("string")
                                                          .required(true)
                                                          .defaultValue("");
                                        })
                                        .property(PROPERTY_DURATION_IN_MILLISECONDS,
                                                  [](TypePropertySchema &propertySchema) {
                                                      propertySchema.type("number")
                                                                    .required(true)
                                                                    .defaultValue(0);
                                                  });
                      })
                      .event(EVENTHANDLER_ON_PLAYER_ACTIVITY_UPDATED_NAME)
                      .command(COMMAND_PLAY_NAME)
                      .command(COMMAND_PAUSE_NAME)
                      .command(COMMAND_PREVIOUS_NAME)
                      .command(COMMAND_NEXT_NAME)
                      .command(COMMAND_SEEK_TO_POSITION_NAME, [](CommandSchema &commandSchema) {
                          commandSchema.allowFastMode(true)
                                       .dataType(DATA_TYPE_SEEK_POSITION);
                      })
                      .command(COMMAND_TOGGLE_NAME, [](CommandSchema &commandSchema) {
                          commandSchema.allowFastMode(true)
                                       .dataType(DATA_TYPE_TOGGLE);
                      })
                      .command(COMMAND_SKIP_FORWARD_NAME)
                      .command(COMMAND_SKIP_BACKWARD_NAME)
                      .command(COMMAND_ADD_LYRICS_VIEWED, [](CommandSchema &commandSchema) {
                          commandSchema.allowFastMode(true)
                                       .dataType(DATA_TYPE_ADD_LYRICS_VIEWED);
                      })
                      .command(COMMAND_ADD_LYRICS_DURATION_IN_MILLISECONDS, [](CommandSchema &commandSchema) {
                          commandSchema.allowFastMode(true)
                                       .dataType(DATA_TYPE_ADD_LYRICS_DURATION);
                      })
                      .command(COMMAND_FLUSH_LYRIC_DATA);
                // live data is not used if it has not been named
                if (!mPlaybackStateName.empty()) {
                    schema.liveDataMap(mPlaybackStateName, [](LiveDataSchema &liveDataSchema) {
                        liveDataSchema.dataType(DATA_TYPE_PLAYBACK_STATE);
                    });
                }
            });
}

bool
AplAudioPlayerExtension::invokeCommand(const std::string &uri, const rapidjson::Value &command)
{
    // unknown URI
    if (uri != URI)
        return false;
    // no player attached
    if (!mObserver)
        return false;

    const std::string &name = GetWithDefault(Command::NAME(), command, "");

    if (COMMAND_PLAY_NAME == name) {
        mObserver->onAudioPlayerPlay();
        return true;
    }

    if (COMMAND_PAUSE_NAME == name) {
        mObserver->onAudioPlayerPause();
        return true;
    }

    if (COMMAND_PREVIOUS_NAME == name) {
        mObserver->onAudioPlayerPrevious();
        return true;
    }

    if (COMMAND_NEXT_NAME == name) {
        mObserver->onAudioPlayerNext();
        return true;
    }

    if (COMMAND_SEEK_TO_POSITION_NAME == name) {
        const rapidjson::Value *params = Command::PAYLOAD().Get(command);
        if (!params || !params->HasMember(PROPERTY_OFFSET))
            return false;
        // require a non-negative offset
        auto offset = GetWithDefault<int>(PROPERTY_OFFSET, *params, -1);
        if (offset < 0)
            return false;
        mObserver->onAudioPlayerSeekToPosition(offset);
        return true;
    }

    if (COMMAND_SKIP_FORWARD_NAME == name) {
        mObserver->onAudioPlayerSkipForward();
        return true;
    }

    if (COMMAND_SKIP_BACKWARD_NAME == name) {
        mObserver->onAudioPlayerSkipBackward();
        return true;
    }

    if (COMMAND_TOGGLE_NAME == name) {
        const rapidjson::Value *params = Command::PAYLOAD().Get(command);
        if (!params || !params->HasMember(PROPERTY_TOGGLE_NAME) || !params->HasMember(PROPERTY_TOGGLE_CHECKED))
            return false;
        auto toggleName = GetWithDefault<const char *>(PROPERTY_TOGGLE_NAME, *params, "");
        if (std::find(TOGGLE_COMMAND_NAMES.begin(), TOGGLE_COMMAND_NAMES.end(), toggleName) ==
            TOGGLE_COMMAND_NAMES.end())
            return false;
        // getting checked as int to detect missing value
        auto &checked = (*params)[PROPERTY_TOGGLE_CHECKED];
        if (!checked.IsBool())
            return false;
        mObserver->onAudioPlayerToggle(toggleName, checked.GetBool());
        return true;
    }

    if (COMMAND_ADD_LYRICS_VIEWED == name) {
        const rapidjson::Value *params = Command::PAYLOAD().Get(command);
        if (!params || !params->HasMember(PROPERTY_TOKEN) || !params->HasMember(PROPERTY_LINES))
            return false;
        std::string token = GetWithDefault<const char *>(PROPERTY_TOKEN, *params, "");
        if (token.empty())
            return false;
        auto lyricData = getActiveLyricsViewedData(true, token);
        if (!lyricData)
            return false;
        // Validation of lines is handled by lyricData, invalid values are ignored
        const auto &lines = (*params)[PROPERTY_LINES];
        lyricData->addLyricLinesData(lines);
        return true;
    }

    if (COMMAND_ADD_LYRICS_DURATION_IN_MILLISECONDS == name) {
        const rapidjson::Value *params = Command::PAYLOAD().Get(command);
        if (!params || !params->HasMember(PROPERTY_TOKEN) || !params->HasMember(PROPERTY_DURATION_IN_MILLISECONDS))
            return false;
        std::string token = GetWithDefault<const char *>(PROPERTY_TOKEN, *params, "");
        if (token.empty())
            return false;
        auto lyricData = getActiveLyricsViewedData(true, token);
        if (!lyricData)
            return false;
        auto duration = GetWithDefault<int>(PROPERTY_DURATION_IN_MILLISECONDS, *params, -1);
        if (duration < 0)
            return false;
        lyricData->durationInMilliseconds += duration;
        return true;
    }

    if (COMMAND_FLUSH_LYRIC_DATA == name) {
        if (auto lyricData = getActiveLyricsViewedData()) {
            flushLyricData(lyricData);
        }
        return true;
    }

    return false;
}

std::shared_ptr<AplAudioPlayerExtension::LyricsViewedData>
AplAudioPlayerExtension::getActiveLyricsViewedData(bool initIfNull, const std::string &token)
{
    if (!mActiveClientToken.empty()) {
        auto lvdi = mLyricsViewedData.find(mActiveClientToken);
        if (lvdi != mLyricsViewedData.end()) {
            auto lyricsViewedData = lvdi->second;
            // If token has changed for the active skill's lyric data, flush the data and set the new token.
            if (!token.empty() && lyricsViewedData->token != token) {
                flushLyricData(lyricsViewedData);
                lyricsViewedData->token = token;
            }
            return lyricsViewedData;
        }
    }

    if (initIfNull) {
        mLyricsViewedData[mActiveClientToken] = std::make_shared<LyricsViewedData>(token);
        return mLyricsViewedData[mActiveClientToken];
    }

    return nullptr;
}

void
AplAudioPlayerExtension::flushLyricData(const std::shared_ptr<LyricsViewedData> &lyricsViewedData)
{
    if (!lyricsViewedData->lyricData->Empty()) {
        mObserver->onAudioPlayerLyricDataFlushed(lyricsViewedData->token,
                                                 lyricsViewedData->durationInMilliseconds,
                                                 lyricsViewedData->getLyricDataPayload());
    }
    lyricsViewedData->reset();
}

void
AplAudioPlayerExtension::updatePlayerActivity(const std::string &state, int offset)
{
    if (std::find(PLAYER_ACTIVITY.begin(), PLAYER_ACTIVITY.end(), state) == PLAYER_ACTIVITY.end()) {
        return;
    }
    if (offset < 100)
        return;

    mPlaybackStateActivity = state;
    mPlaybackStateOffset = offset;

    auto event = Event("1.0").uri(URI).target(URI)
                             .name(EVENTHANDLER_ON_PLAYER_ACTIVITY_UPDATED_NAME)
                             .property(PROPERTY_PLAYER_ACTIVITY, state)
                             .property(PROPERTY_OFFSET, offset);
    publishLiveData();
    invokeExtensionEventHandler(URI, event);
}

void
AplAudioPlayerExtension::updatePlaybackProgress(int offset)
{
    mPlaybackStateOffset = offset;
    publishLiveData();
}

void
AplAudioPlayerExtension::setActivePresentationSession(const std::string &id, const std::string &skillId)
{
    mActiveClientToken = skillId;
    /// If there's available lyricsViewedData for the newly active skillId, report it immediately
    if (auto lyricsViewedData = getActiveLyricsViewedData()) {
        flushLyricData(lyricsViewedData);
    }
}

void
AplAudioPlayerExtension::publishLiveData()
{
    // live data is not used if it has not been named
    if (mPlaybackStateName.empty())
        return;

    //  Publish playback state
    auto liveDataUpdate = LiveDataUpdate("1.0")
            .uri(URI)
            .objectName(mPlaybackStateName)
            .target(URI)
            .liveDataMapUpdate([&](LiveDataMapOperation &operation) {
                operation
                        .type("Set")
                        .key(PROPERTY_PLAYER_ACTIVITY)
                        .item(mPlaybackStateActivity);
            })
            .liveDataMapUpdate([&](LiveDataMapOperation &operation) {
                operation
                        .type("Set")
                        .key(PROPERTY_OFFSET)
                        .item(mPlaybackStateOffset);
            });
    invokeLiveDataUpdate(URI, liveDataUpdate);
}
