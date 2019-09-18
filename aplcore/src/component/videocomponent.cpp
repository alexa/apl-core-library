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
    });

    return sVideoComponentProperties;
}

void
VideoComponent::updateMediaState(const MediaState& state, bool fromEvent)
{
    MediaState previousState = mMediaState;
    mMediaState = state;

    if (state.isPaused() != previousState.isPaused()) {
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

    if (state.isEnded() != previousState.isEnded() && state.isEnded()) {
        auto& commands = getCalculated(kPropertyOnEnd);
        if (!commands.empty()) {
            mContext->sequencer().executeCommands(
                commands,
                createEventContext("End", Object::NULL_OBJECT()),
                shared_from_this(),
                fromEvent);
        }
    }

    if (state.getTrackIndex() != previousState.getTrackIndex()) {
        auto& commands = getCalculated(kPropertyOnTrackUpdate);
        if (!commands.empty()) {
            mContext->sequencer().executeCommands(
                commands,
                createEventContext("TrackUpdate", state.getTrackIndex()),
                shared_from_this(),
                fromEvent);
        }
    }

    if (state.getCurrentTime() != previousState.getCurrentTime()) {
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
    event.emplace("trackIndex", mMediaState.getTrackIndex());
    event.emplace("trackCount", mMediaState.getTrackCount());
    event.emplace("currentTime", mMediaState.getCurrentTime());
    event.emplace("duration", mMediaState.getDuration());
    event.emplace("paused", mMediaState.isPaused());
    event.emplace("ended", mMediaState.isEnded());
}

std::shared_ptr<ObjectMap>
VideoComponent::getEventTargetProperties() const
{
    auto target = CoreComponent::getEventTargetProperties();
    target->emplace("trackIndex", mMediaState.getTrackIndex());
    target->emplace("trackCount", mMediaState.getTrackCount());
    target->emplace("currentTime", mMediaState.getCurrentTime());
    target->emplace("duration", mMediaState.getDuration());
    target->emplace("paused", mMediaState.isPaused());
    target->emplace("ended", mMediaState.isEnded());
    return target;
}

bool
VideoComponent::getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator) {
    bool actionable = CoreComponent::getTags(outMap, allocator);
    auto sources = getCalculated(kPropertySource);
    if(sources.size() > 0) {
        bool allowAdjustSeekPositionForward = mMediaState.getCurrentTime() < mMediaState.getDuration();
        bool allowAdjustSeekPositionBackwards = mMediaState.getCurrentTime() > 0;
        bool allowNext = mMediaState.getTrackIndex() < (mMediaState.getTrackCount() - 1);
        bool allowPrevious = mMediaState.getTrackIndex() > 0;

        std::string state = "idle";
        if (mMediaState.isPaused() && !mMediaState.isEnded() && mMediaState.getCurrentTime() != 0) {
            state = "paused";
        } else if (!mMediaState.isPaused()) {
            state = "playing";
        }

        auto currentSource = sources.getArray().at(mMediaState.getTrackIndex()).getMediaSource();

        rapidjson::Value media(rapidjson::kObjectType);
        media.AddMember("allowAdjustSeekPositionForward", allowAdjustSeekPositionForward, allocator);
        media.AddMember("allowAdjustSeekPositionBackwards", allowAdjustSeekPositionBackwards, allocator);
        media.AddMember("allowNext", allowNext, allocator);
        media.AddMember("allowPrevious", allowPrevious, allocator);
        media.AddMember("entities", rapidjson::Value(currentSource.getEntities().serialize(allocator),
                allocator).Move(), allocator);
        media.AddMember("positionInMilliseconds", mMediaState.getCurrentTime(), allocator);
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