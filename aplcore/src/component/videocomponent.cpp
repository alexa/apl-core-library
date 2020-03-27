/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "apl/component/componentpropdef.h"
#include "apl/component/videocomponent.h"
#include "apl/component/yogaproperties.h"
#include "apl/time/sequencer.h"

namespace apl {


CoreComponentPtr
VideoComponent::create(const ContextPtr& context,
                       Properties&& properties,
                       const std::string& path)
{
    auto ptr = std::make_shared<VideoComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

VideoComponent::VideoComponent(const ContextPtr& context,
                               Properties&& properties,
                               const std::string& path)
    : CoreComponent(context, std::move(properties), path)
{
}

const ComponentPropDefSet&
VideoComponent::propDefSet() const
{
    static ComponentPropDefSet sVideoComponentProperties(CoreComponent::propDefSet(), {
        { kPropertyAudioTrack,      kAudioTrackForeground,  sAudioTrackMap,     kPropInOut },
        { kPropertyAutoplay,        false,                  asOldBoolean,       kPropInOut },
        { kPropertyScale,           kVideoScaleBestFit,     sVideoScaleMap,     kPropInOut },
        { kPropertySource,          Object::EMPTY_ARRAY(),  asMediaSourceArray, kPropDynamic | kPropInOut },
        { kPropertyOnEnd,           Object::EMPTY_ARRAY(),  asCommand,          kPropIn },
        { kPropertyOnPause,         Object::EMPTY_ARRAY(),  asCommand,          kPropIn },
        { kPropertyOnPlay,          Object::EMPTY_ARRAY(),  asCommand,          kPropIn },
        { kPropertyOnTimeUpdate,    Object::EMPTY_ARRAY(),  asCommand,          kPropIn },
        { kPropertyOnTrackUpdate,   Object::EMPTY_ARRAY(),  asCommand,          kPropIn },
        { kPropertyTrackCount,      0,                      asInteger,          kPropRuntimeState },
        { kPropertyTrackCurrentTime,0,                      asInteger,          kPropRuntimeState },
        { kPropertyTrackDuration,   0,                      asInteger,          kPropRuntimeState },
        { kPropertyTrackIndex,      0,                      asInteger,          kPropRuntimeState },
        { kPropertyTrackPaused,     true,                   asBoolean,          kPropRuntimeState },
        { kPropertyTrackEnded,      false,                  asBoolean,          kPropRuntimeState }
    });

    return sVideoComponentProperties;
}

void
VideoComponent::saveMediaState(const MediaState& state)
{
    mCalculated.set(kPropertyTrackCount, state.getTrackCount());
    mCalculated.set(kPropertyTrackCurrentTime, state.getCurrentTime());
    mCalculated.set(kPropertyTrackDuration, state.getDuration());
    mCalculated.set(kPropertyTrackIndex, state.getTrackIndex());
    mCalculated.set(kPropertyTrackPaused, state.isPaused());
    mCalculated.set(kPropertyTrackEnded, state.isEnded());
}

void
VideoComponent::updateMediaState(const MediaState& state, bool fromEvent)
{
    auto previousStatePaused = getCalculated(kPropertyTrackPaused).asBoolean();
    auto previousStateEnded = getCalculated(kPropertyTrackEnded).asBoolean();
    auto previousTrackIndex = getCalculated(kPropertyTrackIndex).asInt();
    auto previousCurrentTime = getCalculated(kPropertyTrackCurrentTime).asInt();
    saveMediaState(state);

    if (state.isPaused() != previousStatePaused) {
        if (!state.isPaused()) {
            auto& commands = getCalculated(kPropertyOnPlay);
            if (!commands.empty())
                mContext->sequencer().executeCommands(
                    commands,
                    createEventContext("Play", Object::NULL_OBJECT()),
                    shared_from_this(),
                    fromEvent);
        }
        else {
            auto& commands = getCalculated(kPropertyOnPause);
            if (!commands.empty()) {
                mContext->sequencer().executeCommands(
                    commands,
                    createEventContext("Pause", Object::NULL_OBJECT()),
                    shared_from_this(),
                    fromEvent);
            }
        }
    }

    if (state.isEnded() != previousStateEnded && state.isEnded()) {
        auto& commands = getCalculated(kPropertyOnEnd);
        if (!commands.empty()) {
            mContext->sequencer().executeCommands(
                commands,
                createEventContext("End", Object::NULL_OBJECT()),
                shared_from_this(),
                fromEvent);
        }
    }

    if (state.getTrackIndex() != previousTrackIndex) {
        auto& commands = getCalculated(kPropertyOnTrackUpdate);
        if (!commands.empty()) {
            mContext->sequencer().executeCommands(
                commands,
                createEventContext("TrackUpdate", state.getTrackIndex()),
                shared_from_this(),
                fromEvent);
        }
    }

    if (state.getCurrentTime() != previousCurrentTime) {
        auto& commands = getCalculated(kPropertyOnTimeUpdate);
        if (!commands.empty()) {
            mContext->sequencer().executeCommands(
                commands,
                createEventContext("TimeUpdate", state.getCurrentTime()),
                shared_from_this(),
                true);
        }
    }
}

void
VideoComponent::addEventSourceProperties(apl::ObjectMap& event) const
{
    event.emplace("trackIndex", getCalculated(kPropertyTrackIndex).asInt());
    event.emplace("trackCount", getCalculated(kPropertyTrackCount).asInt());
    event.emplace("currentTime", getCalculated(kPropertyTrackCurrentTime).asInt());
    event.emplace("duration", getCalculated(kPropertyTrackDuration).asInt());
    event.emplace("paused", getCalculated(kPropertyTrackPaused).asBoolean());
    event.emplace("ended", getCalculated(kPropertyTrackEnded).asBoolean());
}

std::shared_ptr<ObjectMap>
VideoComponent::getEventTargetProperties() const
{
    auto target = CoreComponent::getEventTargetProperties();
    target->emplace("trackIndex", getCalculated(kPropertyTrackIndex).asInt());
    target->emplace("trackCount", getCalculated(kPropertyTrackCount).asInt());
    target->emplace("currentTime", getCalculated(kPropertyTrackCurrentTime).asInt());
    target->emplace("duration", getCalculated(kPropertyTrackDuration).asInt());
    target->emplace("paused", getCalculated(kPropertyTrackPaused).asBoolean());
    target->emplace("ended", getCalculated(kPropertyTrackEnded).asBoolean());
    return target;
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
VideoComponent::getVisualContextType() {
    return getCalculated(kPropertySource).empty() ? VISUAL_CONTEXT_TYPE_EMPTY : VISUAL_CONTEXT_TYPE_VIDEO;
}


} // namespace apl