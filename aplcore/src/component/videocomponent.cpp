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

#include "apl/apl.h"
#include "apl/component/componentpropdef.h"
#include "apl/component/videocomponent.h"
#include "apl/component/yogaproperties.h"
#include "apl/media/mediaplayerfactory.h"
#include "apl/media/mediautils.h"
#include "apl/time/sequencer.h"

namespace apl {

/**
 * Convert a media state object into event properties that will be passed to the event handler.
 * Note that this method copies more properties than are strictly needed according to the APL
 * documentation.
 * @param mediaState
 * @return Shared object map
 */
ObjectMapPtr
mediaStateToEventProperties(const MediaState& mediaState)
{
    auto eventProps = std::make_shared<ObjectMap>();
    eventProps->emplace("trackIndex", mediaState.getTrackIndex());
    eventProps->emplace("trackCount", mediaState.getTrackCount());
    eventProps->emplace("trackState", sTrackStateMap.at(mediaState.getTrackState()));
    eventProps->emplace("currentTime", mediaState.getCurrentTime());
    eventProps->emplace("duration", mediaState.getDuration());
    eventProps->emplace("paused", mediaState.isPaused());
    eventProps->emplace("ended", mediaState.isEnded());
    eventProps->emplace("errorCode", mediaState.getErrorCode());
    return eventProps;
}


CoreComponentPtr
VideoComponent::create(const ContextPtr& context,
                       Properties&& properties,
                       const Path& path)
{
    auto ptr = std::make_shared<VideoComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

VideoComponent::VideoComponent(const ContextPtr& context,
                               Properties&& properties,
                               const Path& path)
    : CoreComponent(context, std::move(properties), path),
      mMediaSequencer("VIDEO"+getUniqueId())
{
    mMediaPlayer = mContext->mediaPlayerFactory().createPlayer([this](
                                                                   MediaPlayerEventType eventType,
                                                                   const MediaState& mediaState) {
        if (!mMediaPlayer)
            return;

        auto self = shared_from_corecomponent();
        auto& sequencer = mContext->sequencer();

        saveMediaState(mediaState);

        // The event handlers are invoked using new sequencer logic.  In this version, the
        // onEnd/onPause/onPlay handlers are always invoked on a sequencer unique to the video
        // player. The onTimeUpdate/onTrackUpdate/onTrackReady/onTrackFail event handlers are always
        // invoked in fast mode
        switch (eventType) {
            case kMediaPlayerEventEnd: {
                auto& commands = getCalculated(kPropertyOnEnd);
                if (!commands.empty()) {
                    sequencer.executeCommandsOnSequencer(
                        commands,
                        createEventContext("End", mediaStateToEventProperties(mediaState)), self,
                        mMediaSequencer);
                }
            } break;
            case kMediaPlayerEventPause: {
                auto& commands = getCalculated(kPropertyOnPause);
                if (!commands.empty()) {
                    sequencer.executeCommandsOnSequencer(
                        commands,
                        createEventContext("Pause", mediaStateToEventProperties(mediaState)), self,
                        mMediaSequencer);
                }
            } break;
            case kMediaPlayerEventPlay: {
                auto& commands = getCalculated(kPropertyOnPlay);
                if (!commands.empty())
                    sequencer.executeCommandsOnSequencer(
                        commands,
                        createEventContext("Play", mediaStateToEventProperties(mediaState)), self,
                        mMediaSequencer);
            } break;
            case kMediaPlayerEventTimeUpdate: {
                auto& commands = getCalculated(kPropertyOnTimeUpdate);
                if (!commands.empty()) {
                    sequencer.executeCommands(
                        commands,
                        createEventContext("TimeUpdate", mediaStateToEventProperties(mediaState),
                                           mediaState.getCurrentTime()),
                        self, true);
                }
            } break;
            case kMediaPlayerEventTrackUpdate: {
                auto& commands = getCalculated(kPropertyOnTrackUpdate);
                if (!commands.empty()) {
                    sequencer.executeCommands(
                        commands,
                        createEventContext("TrackUpdate", mediaStateToEventProperties(mediaState),
                                           mediaState.getTrackIndex()),
                        self, true);
                }
            } break;
            case kMediaPlayerEventTrackReady: {
                auto& commands = getCalculated(kPropertyOnTrackReady);
                if (!commands.empty()) {
                    sequencer.executeCommands(
                        commands,
                        createEventContext("TrackReady", mediaStateToEventProperties(mediaState)),
                        self, true);
                }
            } break;
            case kMediaPlayerEventTrackFail: {
                auto& commands = getCalculated(kPropertyOnTrackFail);
                if (!commands.empty()) {
                    sequencer.executeCommands(
                        commands,
                        createEventContext("TrackFail", mediaStateToEventProperties(mediaState)),
                        self, true);
                }
                break;
            }
        }
    });
}

VideoComponent::~VideoComponent() noexcept
{
    if (mMediaPlayer) {
        mMediaPlayer->release();
        mMediaPlayer = nullptr;
    }
}

bool
VideoComponent::remove()
{
    if (mMediaPlayer)
        mMediaPlayer->halt();

    return CoreComponent::remove();
}

const ComponentPropDefSet&
VideoComponent::propDefSet() const
{
    static const std::array<PropertyKey, 6> PLAYING_STATE = {
        kPropertyTrackCount,
        kPropertyTrackCurrentTime,
        kPropertyTrackDuration,
        kPropertyTrackIndex,
        kPropertyTrackPaused,
        kPropertyTrackEnded
    };

    // Save the current playing state of the component
    static auto getPlayingState = [](const CoreComponent& component) -> Object {
        auto array = ObjectArray();
        for (auto key : PLAYING_STATE)
            array.emplace_back(component.getCalculated(key));
        return Object(std::move(array));
    };

    // Restore the current playing state of the component. Note that this is NOT guaranteed to
    // be correct; the view host video component must check the current properties at load time
    // and decide how to handle them.
    static auto setPlayingState = [](CoreComponent& component, const Object& value ) -> void {
        if (!value.isArray() || value.size() != PLAYING_STATE.size()) {
            CONSOLE_CTP(component.getContext()) << "setPlayingState: Invalid " << value.toDebugString();
            return;
        }

        auto& self = dynamic_cast<VideoComponent&>(component);
        for (size_t index = 0 ; index < PLAYING_STATE.size() ; index++)
            self.mCalculated.set(PLAYING_STATE.at(index), value.at(index));

        // TODO: Could we arrange to use the same MediaPlayer when reinflating?
    };

    static auto resetMediaState = [](Component& component) {
        auto& comp = dynamic_cast<VideoComponent&>(component);
        auto mediaPlayer = comp.getMediaPlayer();
        if (mediaPlayer)
            mediaPlayer->setTrackList(mediaSourcesToTracks(comp.getCalculated(kPropertySource)));
    };

    static ComponentPropDefSet sVideoComponentProperties = ComponentPropDefSet(
        CoreComponent::propDefSet(), MediaComponentTrait::propDefList()).add({
        { kPropertyAudioTrack,      kAudioTrackForeground,  sAudioTrackMap,     kPropInOut },
        { kPropertyAutoplay,        false,                  asOldBoolean,       kPropInOut },
        { kPropertyScale,           kVideoScaleBestFit,     sVideoScaleMap,     kPropInOut },
        { kPropertySource,          Object::EMPTY_ARRAY(),  asMediaSourceArray, kPropDynamic | kPropInOut | kPropVisualContext | kPropVisualHash, resetMediaState },
        { kPropertyOnEnd,           Object::EMPTY_ARRAY(),  asCommand,          kPropIn },
        { kPropertyOnPause,         Object::EMPTY_ARRAY(),  asCommand,          kPropIn },
        { kPropertyOnPlay,          Object::EMPTY_ARRAY(),  asCommand,          kPropIn },
        { kPropertyOnTrackReady,    Object::EMPTY_ARRAY(),  asCommand,          kPropIn },
        { kPropertyOnTimeUpdate,    Object::EMPTY_ARRAY(),  asCommand,          kPropIn },
        { kPropertyOnTrackUpdate,   Object::EMPTY_ARRAY(),  asCommand,          kPropIn },
        { kPropertyOnTrackFail,     Object::EMPTY_ARRAY(),  asCommand,          kPropIn },
        // TODO: When MediaPlayer integration is complete, these properties may be removed or act solely as accessors
        { kPropertyTrackCount,      0,                      asInteger,          kPropRuntimeState | kPropVisualContext },
        { kPropertyTrackCurrentTime,0,                      asInteger,          kPropRuntimeState | kPropVisualContext },
        { kPropertyTrackDuration,   0,                      asInteger,          kPropRuntimeState | kPropVisualContext },
        { kPropertyTrackIndex,      0,                      asInteger,          kPropRuntimeState | kPropVisualContext },
        { kPropertyTrackPaused,     true,                   asBoolean,          kPropRuntimeState | kPropVisualContext },
        { kPropertyTrackEnded,      false,                  asBoolean,          kPropRuntimeState | kPropVisualContext },
        { kPropertyTrackState,      kTrackNotReady,         sTrackStateMap,     kPropRuntimeState},
        { kPropertyPlayingState,    getPlayingState,        setPlayingState,    kPropDynamic },
    });

    return sVideoComponentProperties;
}

void
VideoComponent::assignProperties(const ComponentPropDefSet &propDefSet)
{
    CoreComponent::assignProperties(propDefSet);

    auto mediaSourceArray = getCalculated(kPropertySource);
    if (mediaSourceArray.isArray())
        mCalculated.set(kPropertyTrackCount, mediaSourceArray.size());

    if (mMediaPlayer) {
        if (propDefSet.find(kPropertyAudioTrack) != propDefSet.end()) {
            mMediaPlayer->setAudioTrack(
                static_cast<AudioTrack>(getCalculated(kPropertyAudioTrack).getInteger()));
        }
        if (propDefSet.find(kPropertySource) != propDefSet.end()) {
            mMediaPlayer->setTrackList(mediaSourcesToTracks(mediaSourceArray));

            // We start auto playing here, but viewhosts may need to defer until the video player
            // has been given a surface.
            if (getCalculated(kPropertyAutoplay).asBoolean())
                mMediaPlayer->play(ActionRef(nullptr));
        }
    }
}

void
VideoComponent::saveMediaState(const MediaState& state)
{
    if (!mMediaPlayer)
        setVisualContextDirty();

    mCalculated.set(kPropertyTrackCount, state.getTrackCount());
    mCalculated.set(kPropertyTrackCurrentTime, state.getCurrentTime());
    mCalculated.set(kPropertyTrackDuration, state.getDuration());
    mCalculated.set(kPropertyTrackIndex, state.getTrackIndex());
    mCalculated.set(kPropertyTrackPaused, state.isPaused());
    mCalculated.set(kPropertyTrackEnded, state.isEnded());
    mCalculated.set(kPropertyTrackState, state.getTrackState());
}

void
VideoComponent::updateMediaState(const MediaState& state, bool fromEvent)
{
    // This method should only be called if we don't have a media player
    assert(!mMediaPlayer);

    auto previousStatePaused = getCalculated(kPropertyTrackPaused).asBoolean();
    auto previousStateEnded = getCalculated(kPropertyTrackEnded).asBoolean();
    auto previousTrackIndex = getCalculated(kPropertyTrackIndex).asInt();
    auto previousCurrentTime = getCalculated(kPropertyTrackCurrentTime).asInt();
    auto previousTrackState = getCalculated(kPropertyTrackState).asInt();
    saveMediaState(state);

    if (state.getTrackIndex() != previousTrackIndex) {
        auto& commands = getCalculated(kPropertyOnTrackUpdate);
        if (!commands.empty()) {
            mContext->sequencer().executeCommands(
                commands,
                createEventContext("TrackUpdate", createDefaultEventProperties(), state.getTrackIndex()),
                shared_from_corecomponent(),
                fromEvent);
        }
    }

    if(state.isError() &&
        ( kTrackFailed != previousTrackState || state.getTrackIndex() != previousTrackIndex)) {
        auto& commands = getCalculated(kPropertyOnTrackFail);
        if (!commands.empty()) {
            mContext->sequencer().executeCommands(
                commands,
                createEventContext("TrackFail", createErrorEventProperties(state.getErrorCode())),
                shared_from_corecomponent(), fromEvent);
        }
        return;
    }

    if(state.isReady() && previousTrackState == kTrackNotReady &&
        previousTrackIndex == state.getTrackIndex()) {
        auto& commands = getCalculated(kPropertyOnTrackReady);
        if (!commands.empty()) {
            mContext->sequencer().executeCommands(
                commands,
                createEventContext("TrackReady", createReadyEventProperties()),
                shared_from_corecomponent(), fromEvent);
        }
    }

    if (state.isPaused() != previousStatePaused) {
        if (!state.isPaused()) {
            auto& commands = getCalculated(kPropertyOnPlay);
            if (!commands.empty())
                mContext->sequencer().executeCommands(
                    commands,
                    createEventContext("Play", createDefaultEventProperties()),
                    shared_from_corecomponent(),
                    fromEvent);
        }
        else {
            auto& commands = getCalculated(kPropertyOnPause);
            if (!commands.empty()) {
                mContext->sequencer().executeCommands(
                    commands,
                    createEventContext("Pause", createDefaultEventProperties()),
                    shared_from_corecomponent(),
                    fromEvent);
            }
        }
    }

    if (state.isEnded() != previousStateEnded && state.isEnded()) {
        auto& commands = getCalculated(kPropertyOnEnd);
        if (!commands.empty()) {
            mContext->sequencer().executeCommands(
                commands,
                createEventContext("End", createDefaultEventProperties()),
                shared_from_corecomponent(),
                fromEvent);
        }
    }

    if (state.getCurrentTime() != previousCurrentTime) {
        auto& commands = getCalculated(kPropertyOnTimeUpdate);
        if (!commands.empty()) {
            mContext->sequencer().executeCommands(
                commands,
                createEventContext("TimeUpdate", createDefaultEventProperties(), state.getCurrentTime()),
                shared_from_corecomponent(),
                true);
        }
    }
}

std::shared_ptr<ObjectMap>
VideoComponent::createDefaultEventProperties()
{
    // This method should only be called if we don't have a media player
    assert(!mMediaPlayer);

    auto eventProps = std::make_shared<ObjectMap>();
    eventProps->emplace("trackIndex", getCalculated(kPropertyTrackIndex).asInt());
    eventProps->emplace("trackCount", getCalculated(kPropertyTrackCount).asInt());
    eventProps->emplace("trackState", sTrackStateMap.at(getCalculated(kPropertyTrackState).asInt()));
    eventProps->emplace("currentTime", getCalculated(kPropertyTrackCurrentTime).asInt());
    eventProps->emplace("duration", getCalculated(kPropertyTrackDuration).asInt());
    eventProps->emplace("paused", getCalculated(kPropertyTrackPaused).asBoolean());
    eventProps->emplace("ended", getCalculated(kPropertyTrackEnded).asBoolean());

    return eventProps;
}

std::shared_ptr<ObjectMap>
VideoComponent::createErrorEventProperties(const int error)
{
    // This method should only be called if we don't have a media player
    assert(!mMediaPlayer);

    auto eventProps = std::make_shared<ObjectMap>();
    eventProps->emplace("trackIndex", getCalculated(kPropertyTrackIndex).asInt());
    eventProps->emplace("trackState", sTrackStateMap.at(getCalculated(kPropertyTrackState).asInt()));
    eventProps->emplace("currentTime", getCalculated(kPropertyTrackCurrentTime).asInt());
    eventProps->emplace("errorCode", error);
    return eventProps;
}

std::shared_ptr<ObjectMap>
VideoComponent::createReadyEventProperties()
{
    // This method should only be called if we don't have a media player
    assert(!mMediaPlayer);

    auto eventProps = std::make_shared<ObjectMap>();
    eventProps->emplace("trackIndex", getCalculated(kPropertyTrackIndex).asInt());
    eventProps->emplace("trackState", sTrackStateMap.at(getCalculated(kPropertyTrackState).asInt()));
    return eventProps;
}

std::string
VideoComponent::getCurrentUrl() const
{
    auto& source = getCalculated(kPropertySource);
    auto trackIndex = getCalculated(kPropertyTrackIndex).asInt();
    if (trackIndex < 0 || trackIndex >= source.size())
        return "";
    return source.at(trackIndex).getMediaSource().getUrl();
}

static inline Object
inlineGetCurrentURL(const CoreComponent *c) {
    auto comp = dynamic_cast<const VideoComponent*>(c);
    return comp->getCurrentUrl();
}

const EventPropertyMap&
VideoComponent::eventPropertyMap() const
{
    static EventPropertyMap sVideoEventProperties = eventPropertyMerge(
        CoreComponent::eventPropertyMap(),
        {
            {"trackCount",  [](const CoreComponent* c) { return c->getCalculated(kPropertyTrackCount); }},
            {"trackIndex",  [](const CoreComponent* c) { return c->getCalculated(kPropertyTrackIndex); }},
            {"currentTime", [](const CoreComponent* c) { return c->getCalculated(kPropertyTrackCurrentTime); }},
            {"duration",    [](const CoreComponent* c) { return c->getCalculated(kPropertyTrackDuration); }},
            {"paused",      [](const CoreComponent* c) { return c->getCalculated(kPropertyTrackPaused); }},
            {"ended",       [](const CoreComponent* c) { return c->getCalculated(kPropertyTrackEnded); }},
            {"source",      &inlineGetCurrentURL},
            {"url",         &inlineGetCurrentURL},
            {"trackState",  [](const CoreComponent* c) { return sTrackStateMap.at(c->getCalculated(kPropertyTrackState).asInt()); }},
        });

    return sVideoEventProperties;
}

bool
VideoComponent::getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator) {
    bool actionable = CoreComponent::getTags(outMap, allocator);
    auto sources = getCalculated(kPropertySource);
    if (sources.size() > 0) {
        bool isPaused = getCalculated(kPropertyTrackPaused).asBoolean();
        bool isEnded = getCalculated(kPropertyTrackEnded).asBoolean();
        int trackIndex = getCalculated(kPropertyTrackIndex).asInt();
        int trackCount = getCalculated(kPropertyTrackCount).asInt();
        int currentTime = getCalculated(kPropertyTrackCurrentTime).asInt();
        int duration = getCalculated(kPropertyTrackDuration).asInt();

        bool allowAdjustSeekPositionForward = currentTime < duration;
        bool allowAdjustSeekPositionBackwards = currentTime > 0;
        bool allowNext = trackIndex < (trackCount - 1);
        bool allowPrevious = trackIndex > 0;

        std::string state = "idle";
        if (isPaused && !isEnded && currentTime != 0) {
            state = "paused";
        } else if (!isPaused) {
            state = "playing";
        }

        auto currentSource = sources.getArray().at(trackIndex).getMediaSource();

        rapidjson::Value media(rapidjson::kObjectType);
        media.AddMember("allowAdjustSeekPositionForward", allowAdjustSeekPositionForward, allocator);
        media.AddMember("allowAdjustSeekPositionBackwards", allowAdjustSeekPositionBackwards, allocator);
        media.AddMember("allowNext", allowNext, allocator);
        media.AddMember("allowPrevious", allowPrevious, allocator);
        media.AddMember("entities", rapidjson::Value(currentSource.getEntities().serialize(allocator),
                allocator).Move(), allocator);
        media.AddMember("positionInMilliseconds", currentTime, allocator);
        media.AddMember("state", rapidjson::Value(state.c_str(), allocator).Move(), allocator);
        media.AddMember("url", rapidjson::Value(currentSource.getUrl().c_str(), allocator).Move(), allocator);
        outMap.AddMember("media", media, allocator);
        actionable = true;
    }
    return actionable;
}

std::string
VideoComponent::getVisualContextType() const
{
    return getCalculated(kPropertySource).empty() ? VISUAL_CONTEXT_TYPE_EMPTY : VISUAL_CONTEXT_TYPE_VIDEO;
}

} // namespace apl