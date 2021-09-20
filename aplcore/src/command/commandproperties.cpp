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

#include "apl/command/commandproperties.h"

namespace apl {

Bimap<int, std::string> sCommandNameBimap = {
    {kCommandTypeIdle,              "Idle"},
    {kCommandTypeSequential,        "Sequential"},
    {kCommandTypeParallel,          "Parallel"},
    {kCommandTypeSendEvent,         "SendEvent"},
    {kCommandTypeSendEvent,         "sendEvent"},
    {kCommandTypeSetValue,          "SetValue"},
    {kCommandTypeSetState,          "SetState"},
    {kCommandTypeSpeakItem,         "SpeakItem"},
    {kCommandTypeSpeakList,         "SpeakList"},
    {kCommandTypeScroll,            "Scroll"},
    {kCommandTypeScrollToIndex,     "ScrollToIndex"},
    {kCommandTypeScrollToComponent, "ScrollToComponent"},
    {kCommandTypeSetPage,           "SetPage"},
    {kCommandTypeSelect,            "Select"},
    {kCommandTypeAutoPage,          "AutoPage"},
    {kCommandTypePlayMedia,         "PlayMedia"},
    {kCommandTypeControlMedia,      "ControlMedia"},
    {kCommandTypeOpenURL,           "OpenURL"},
    {kCommandTypeAnimateItem,       "AnimateItem"},
    {kCommandTypeSetFocus,          "SetFocus"},
    {kCommandTypeClearFocus,        "ClearFocus"},
    {kCommandTypeFinish,            "Finish"},
    {kCommandTypeReinflate,         "Reinflate"},
};

Bimap<int, std::string> sCommandPropertyBimap = {
    {kCommandPropertyAlign,            "align"},
    {kCommandPropertyArguments,        "arguments"},
    {kCommandPropertyAudioTrack,       "audioTrack"},
    {kCommandPropertyCatch,            "catch"},
    {kCommandPropertyCommand,          "command"},
    {kCommandPropertyCommands,         "commands"},
    {kCommandPropertyComponents,       "components"},
    {kCommandPropertyComponentId,      "componentId"},
    {kCommandPropertyCount,            "count"},
    {kCommandPropertyData,             "data"},
    {kCommandPropertyDelay,            "delay"},
    {kCommandPropertyDistance,         "distance"},
    {kCommandPropertyDuration,         "duration"},
    {kCommandPropertyEasing,           "easing"},
    {kCommandPropertyExtension,        "extension"},
    {kCommandPropertyFinally,          "finally"},
    {kCommandPropertyFlags,            "flags"},
    {kCommandPropertyHighlightMode,    "highlightMode"},
    {kCommandPropertyIndex,            "index"},
    {kCommandPropertyMinimumDwellTime, "minimumDwellTime"},
    {kCommandPropertyOnFail,           "onFail"},
    {kCommandPropertyOtherwise,        "otherwise"},
    {kCommandPropertyPosition,         "position"},
    {kCommandPropertyProperty,         "property"},
    {kCommandPropertyReason,           "reason"},
    {kCommandPropertyRepeatCount,      "repeatCount"},
    {kCommandPropertyRepeatMode,       "repeatMode"},
    {kCommandPropertyScreenLock,       "screenLock"},
    {kCommandPropertySequencer,        "sequencer"},
    {kCommandPropertySource,           "source"},
    {kCommandPropertyStart,            "start"},
    {kCommandPropertyState,            "state"},
    {kCommandPropertyValue,            "value"},
};

Bimap<int, std::string> sCommandAlignMap = {
    {kCommandScrollAlignFirst,   "first"},
    {kCommandScrollAlignCenter,  "center"},
    {kCommandScrollAlignLast,    "last"},
    {kCommandScrollAlignVisible, "visible"}
};

Bimap<int, std::string> sHighlightModeMap = {
    {kCommandHighlightModeLine,  "line"},
    {kCommandHighlightModeBlock, "block"}
};

Bimap<int, std::string> sCommandPositionMap = {
    {kCommandPositionRelative, "relative"},
    {kCommandPositionAbsolute, "absolute"}
};

Bimap<int, std::string> sCommandAudioTrackMap = {
    {kCommandAudioTrackBackground, "background"},
    {kCommandAudioTrackForeground, "foreground"},
    {kCommandAudioTrackNone,       "none"},
    {kCommandAudioTrackNone,       "mute"},  // "mute" is a synonym for "none"
};

Bimap<int, std::string> sControlMediaMap = {
    {kCommandControlMediaPlay,     "play"},
    {kCommandControlMediaPause,    "pause"},
    {kCommandControlMediaNext,     "next"},
    {kCommandControlMediaPrevious, "previous"},
    {kCommandControlMediaRewind,   "rewind"},
    {kCommandControlMediaSeek,     "seek"},
    {kCommandControlMediaSetTrack, "setTrack"}
};

Bimap<int, std::string> sCommandRepeatModeMap = {
    {kCommandRepeatModeRestart, "restart"},
    {kCommandRepeatModeReverse, "reverse"}
};

Bimap<int, std::string> sCommandReasonMap = {
        {kCommandReasonBack, "back"},
        {kCommandReasonExit, "exit"}
};

} // namespace apl