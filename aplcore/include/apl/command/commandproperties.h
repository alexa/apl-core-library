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

#ifndef _APL_COMMAND_PROPERTIES_H
#define _APL_COMMAND_PROPERTIES_H

#include "apl/component/componentproperties.h"
#include "apl/primitives/objectbag.h"

namespace apl {

enum CommandType {
    kCommandTypeArray,
    kCommandTypeIdle,
    kCommandTypeSequential,
    kCommandTypeParallel,
    kCommandTypeSendEvent,
    kCommandTypeSetValue,
    kCommandTypeSetState,
    kCommandTypeSpeakItem,
    kCommandTypeSpeakList,
    kCommandTypeScroll,
    kCommandTypeScrollToIndex,
    kCommandTypeScrollToComponent,
    kCommandTypeSelect,
    kCommandTypeSetPage,
    kCommandTypeAutoPage,
    kCommandTypePlayMedia,
    kCommandTypeControlMedia,
    kCommandTypeOpenURL,
    kCommandTypeAnimateItem,
    kCommandTypeSetFocus,
    kCommandTypeClearFocus,
    kCommandTypeFinish,
    kCommandTypeReinflate,
    kCommandTypeCustomEvent,
};

enum CommandScrollAlign {
    kCommandScrollAlignFirst = 0,
    kCommandScrollAlignCenter = 1,
    kCommandScrollAlignLast = 2,
    kCommandScrollAlignVisible = 3
};

enum CommandHighlightMode {
    kCommandHighlightModeLine = 0,
    kCommandHighlightModeBlock = 1
};

enum CommandPosition {
    kCommandPositionRelative = 0,
    kCommandPositionAbsolute = 1
};

enum CommandAudioTrack {
    kCommandAudioTrackBackground = AudioTrack::kAudioTrackBackground,
    kCommandAudioTrackForeground = AudioTrack::kAudioTrackForeground,
    kCommandAudioTrackNone = AudioTrack::kAudioTrackNone
};

enum CommandControlMedia {
    kCommandControlMediaPlay,
    kCommandControlMediaPause,
    kCommandControlMediaNext,
    kCommandControlMediaPrevious,
    kCommandControlMediaRewind,
    kCommandControlMediaSeek,
    kCommandControlMediaSetTrack
};

enum CommandRepeatMode {
    kCommandRepeatModeRestart,
    kCommandRepeatModeReverse
};

enum CommandReason {
    kCommandReasonBack,
    kCommandReasonExit
};

enum CommandPropertyKey {
    kCommandPropertyAlign,
    kCommandPropertyArguments,
    kCommandPropertyAudioTrack,
    kCommandPropertyCatch,
    kCommandPropertyCommand,
    kCommandPropertyCommands,
    kCommandPropertyComponents,
    kCommandPropertyComponentId,
    kCommandPropertyCount,
    kCommandPropertyData,
    kCommandPropertyDelay,
    kCommandPropertyDistance,
    kCommandPropertyDuration,
    kCommandPropertyEasing,
    kCommandPropertyExtension,
    kCommandPropertyFinally,
    kCommandPropertyHighlightMode,
    kCommandPropertyIndex,
    kCommandPropertyMinimumDwellTime,
    kCommandPropertyOnFail,
    kCommandPropertyOtherwise,
    kCommandPropertyPosition,
    kCommandPropertyProperty,
    kCommandPropertyReason,
    kCommandPropertyRepeatCount,
    kCommandPropertyRepeatMode,
    kCommandPropertyScreenLock,
    kCommandPropertySequencer,
    kCommandPropertySource,
    kCommandPropertyStart,
    kCommandPropertyState,
    kCommandPropertyValue,
};

extern Bimap<int, std::string> sCommandNameBimap;
extern Bimap<int, std::string> sCommandPropertyBimap;
extern Bimap<int, std::string> sCommandAlignMap;
extern Bimap<int, std::string> sHighlightModeMap;
extern Bimap<int, std::string> sCommandPositionMap;
extern Bimap<int, std::string> sCommandAudioTrackMap;
extern Bimap<int, std::string> sControlMediaMap;
extern Bimap<int, std::string> sCommandRepeatModeMap;
extern Bimap<int, std::string> sCommandReasonMap;

using CommandBag = ObjectBag<sCommandPropertyBimap>;

} // namespace apl

#endif //_APL_COMMAND_PROPERTIES_H
