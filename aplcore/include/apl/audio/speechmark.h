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

#ifndef _APL_SPEECH_MARK_H
#define _APL_SPEECH_MARK_H

#include <string>
#include <vector>

#include "apl/utils/bimap.h"

namespace apl {

enum SpeechMarkType {
    kSpeechMarkWord,
    kSpeechMarkSentence,
    kSpeechMarkSSML,
    kSpeechMarkViseme,
    kSpeechMarkUnknown
};

/**
 * Store a single Polly speech mark.
 *
 * Refer to https://docs.aws.amazon.com/polly/latest/dg/speechmarks.html
 * for a discussion of the speech marks.
 */
struct SpeechMark {
    SpeechMarkType type;
    unsigned int start;
    unsigned int end;
    unsigned long time;
    std::string value;
};


std::vector<SpeechMark> parsePollySpeechMarks(const char *data, unsigned long length);

extern Bimap<int, std::string> sSpeechMarkTypeMap;

} // namespace apl

#endif // _APL_SPEECH_MARK_H
