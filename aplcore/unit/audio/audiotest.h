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

#ifndef _APL_AUDIO_TEST_H
#define _APL_AUDIO_TEST_H

#include "../testeventloop.h"

namespace apl {

/**
 * Subclass of DocumentWrapper for handling audio player tests
 */
class AudioTest : public DocumentWrapper {
public:
    AudioTest() {
        // Alias to not change all existing tests.
        factory = audioPlayerFactory;
    }

    ActionPtr executeSpeakItem(const std::string& item, CommandScrollAlign align,
                          CommandHighlightMode highlightMode, int minimumDwell,
                          std::string sequencer = "") {
        return executeCommand("SpeakItem",
                       {{"componentId", item},
                        {"align", sCommandAlignMap.at(align)},
                        {"highlightMode", sHighlightModeMap.at(highlightMode)},
                        {"minimumDwellTime", minimumDwell},
                        {"sequencer", sequencer}},
                       false);
    }

    void executeSpeakItem(const ComponentPtr& component, CommandScrollAlign align,
                          CommandHighlightMode highlightMode, int minimumDwell) {
        executeSpeakItem(component->getUniqueId(), align, highlightMode, minimumDwell);
    }

public:
    std::shared_ptr<TestAudioPlayerFactory> factory;
};

} // namespace apl

#endif // _APL_AUDIO_TEST_H
