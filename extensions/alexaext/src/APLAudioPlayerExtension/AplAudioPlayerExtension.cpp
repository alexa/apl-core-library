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

/**
 * Utility object for tracking ActivityState
 */
struct AplAudioPlayerExtension::ActivityState {
    /**
     * Constructor.
     *
     * @param token the identifier of the track displaying lyrics.
     */
    explicit ActivityState(std::string token = "") : token{std::move(token)}
    {
        lyricData = std::make_shared<rapidjson::Document>();
        lyricData->SetArray();
    };

    /// The playback state name
    std::string playbackStateName;

    /// The identifier of the track displaying lyrics.
    std::string token;

    /// The total time in milliseconds that lyrics were viewed.
    long durationInMilliseconds = 0;

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
     * Clear the Lyrics object
     */
    void clearLyrics()
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

// Registration token for unique client identifier
// TODO use UID gen
static std::atomic_int sToken(53);

AplAudioPlayerExtension::AplAudioPlayerExtension(std::shared_ptr<AplAudioPlayerExtensionObserverInterface> observer)
        : alexaext::ExtensionBase(URI), mObserver(std::move(observer))
{
    mPlaybackStateActivity = "STOPPED";
    mPlaybackStateOffset = 0;
}

void
AplAudioPlayerExtension::applySettings(const ActivityDescriptor &activity, const rapidjson::Value &settings)
{
    if (!settings.IsObject())
        return;

    /// Apply document assigned settings
    auto playbackStateName = settings.FindMember(SETTING_PLAYBACK_STATE_NAME);
    if (playbackStateName != settings.MemberEnd() && playbackStateName->value.IsString()) {
        getOrCreateActivityState(activity)->playbackStateName = settings[SETTING_PLAYBACK_STATE_NAME].GetString();
    }
}

rapidjson::Document
AplAudioPlayerExtension::createRegistration(const ActivityDescriptor& activity,
                                            const rapidjson::Value &registrationRequest)
{
    auto uri = activity.getURI();
    if (uri != URI) {
        return  RegistrationFailure::forUnknownURI(uri);
    }

    // extract document assigned settings
    const auto *settingsValue = RegistrationRequest::SETTINGS().Get(registrationRequest);
    if (settingsValue)
        applySettings(activity, *settingsValue);

    // session token
    auto clientToken = TAG + std::to_string(sToken++);

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
                auto playbackStateName = getOrCreateActivityState(activity)->playbackStateName;
                if (!playbackStateName.empty()) {
                    schema.liveDataMap(playbackStateName, [](LiveDataSchema &liveDataSchema) {
                        liveDataSchema.dataType(DATA_TYPE_PLAYBACK_STATE);
                    });
                }
            });
}

bool
AplAudioPlayerExtension::invokeCommand(const ActivityDescriptor& activity, const rapidjson::Value &command)
{
    auto uri = activity.getURI();

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
        updatePlaybackProgress(offset);
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
        auto state = getOrCreateActivityState(activity);
        auto &activeToken = state->token;
        // Flush lyric data if tokens are changing
        if (token != activeToken) {
            flushLyricData(state);
        }
        state->token = token;
        // Validation of lines is handled by lyricData, invalid values are ignored
        const auto &lines = (*params)[PROPERTY_LINES];
        state->addLyricLinesData(lines);
        return true;
    }

    if (COMMAND_ADD_LYRICS_DURATION_IN_MILLISECONDS == name) {
        const rapidjson::Value *params = Command::PAYLOAD().Get(command);
        if (!params || !params->HasMember(PROPERTY_TOKEN) || !params->HasMember(PROPERTY_DURATION_IN_MILLISECONDS))
            return false;
        std::string token = GetWithDefault<const char *>(PROPERTY_TOKEN, *params, "");
        if (token.empty())
            return false;
        auto duration = GetWithDefault<int>(PROPERTY_DURATION_IN_MILLISECONDS, *params, -1);
        if (duration < 0)
            return false;
        getOrCreateActivityState(activity)->durationInMilliseconds += duration;
        return true;
    }

    if (COMMAND_FLUSH_LYRIC_DATA == name) {
        flushLyricData(getOrCreateActivityState(activity));
        return true;
    }

    return false;
}

void
AplAudioPlayerExtension::onActivityRegistered(const ActivityDescriptor &activity)
{
    getOrCreateActivityState(activity);
}

void
AplAudioPlayerExtension::onActivityUnregistered(const ActivityDescriptor &activity)
{
    flushLyricData(getOrCreateActivityState(activity));
    std::lock_guard<std::mutex> lock(mStateMutex);
    mActivityStateMap.erase(activity);
}

std::shared_ptr<AplAudioPlayerExtension::ActivityState>
AplAudioPlayerExtension::getOrCreateActivityState(const ActivityDescriptor &activity)
{
    std::lock_guard<std::mutex> lock(mStateMutex);
    auto it = mActivityStateMap.find(activity);
    if (it != mActivityStateMap.end()) {
        return it->second;
    } else {
        auto state = std::make_shared<ActivityState>();
        mActivityStateMap.emplace(activity, state);
        return state;
    }
}

void
AplAudioPlayerExtension::flushLyricData(const std::shared_ptr<ActivityState> &activityState)
{
    if (!activityState->lyricData->Empty()) {
        mObserver->onAudioPlayerLyricDataFlushed(activityState->token,
                                                 activityState->durationInMilliseconds,
                                                 activityState->getLyricDataPayload());
    }
    activityState->clearLyrics();
}

void
AplAudioPlayerExtension::updatePlayerActivity(const std::string &state, int offset)
{
    if (std::find(PLAYER_ACTIVITY.begin(), PLAYER_ACTIVITY.end(), state) == PLAYER_ACTIVITY.end()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mStateMutex);
        mPlaybackStateActivity = state;
        mPlaybackStateOffset = offset;
    }

    auto event = Event("1.0").uri(URI).target(URI)
                             .name(EVENTHANDLER_ON_PLAYER_ACTIVITY_UPDATED_NAME)
                             .property(PROPERTY_PLAYER_ACTIVITY, state)
                             .property(PROPERTY_OFFSET, offset);
    publishLiveData();

    // Make a list of activities to update with the lock
    std::vector<ActivityDescriptor> activitiesToUpdate;
    {
        std::lock_guard<std::mutex> lock(mStateMutex);
        for (const auto &it: mActivityStateMap) {
            activitiesToUpdate.emplace_back(it.first);
        }
    }

    for (const auto &activity: activitiesToUpdate) {
        invokeExtensionEventHandler(activity, event);
    }
}

void
AplAudioPlayerExtension::updatePlaybackProgress(int offset)
{
    {
        std::lock_guard<std::mutex> lock(mStateMutex);
        mPlaybackStateOffset = offset;
    }
    publishLiveData();
}

void
AplAudioPlayerExtension::setActivePresentationSession(const std::string &id, const std::string &skillId)
{
    // no-op
}

void
AplAudioPlayerExtension::publishLiveData()
{
    // Make a list of updates with the lock
    std::unordered_map<ActivityDescriptor, std::shared_ptr<LiveDataUpdate>, ActivityDescriptor::Hash> updates;
    {
        std::lock_guard<std::mutex> lock(mStateMutex);
        for (const auto &it: mActivityStateMap) {
            const auto playbackStateName = it.second->playbackStateName;
            // Publish live data for activities that set the playback state name
            if (!playbackStateName.empty()) {
                auto liveDataUpdate = std::make_shared<LiveDataUpdate>("1.0");
                liveDataUpdate->uri(URI)
                        .objectName(playbackStateName)
                        .target(URI)
                        .liveDataMapUpdate([&](LiveDataMapOperation& operation) {
                            operation.type("Set")
                                .key(PROPERTY_PLAYER_ACTIVITY)
                                .item(mPlaybackStateActivity);
                        })
                        .liveDataMapUpdate([&](LiveDataMapOperation& operation) {
                            operation.type("Set").key(PROPERTY_OFFSET).item(mPlaybackStateOffset);
                        });

                updates.emplace(it.first, liveDataUpdate);
            }
        }
    }

    for (const auto& it: updates) {
        invokeLiveDataUpdate(it.first, it.second->getDocument());
    }
}
