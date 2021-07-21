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

#include "apl/command/speaklistcommand.h"
#include "apl/action/speaklistaction.h"
#include "apl/utils/session.h"

namespace apl {

const CommandPropDefSet&
SpeakListCommand::propDefSet() const {
    static CommandPropDefSet sSpeakListCommandProperties(CoreCommand::propDefSet(), {
            {kCommandPropertyAlign,            kCommandScrollAlignVisible, sCommandAlignMap},
            {kCommandPropertyComponentId,      "",                         asString,             kPropRequiredId},
            {kCommandPropertyCount,            0,                          asNonNegativeInteger, kPropRequired},
            {kCommandPropertyHighlightMode,    kCommandHighlightModeBlock, sHighlightModeMap},
            {kCommandPropertyMinimumDwellTime, 0,                          asNonNegativeInteger},
            {kCommandPropertyStart,            0,                          asInteger,            kPropRequired}
    });

    return sSpeakListCommandProperties;
}

ActionPtr
SpeakListCommand::execute(const TimersPtr& timers, bool fastMode) {
    if (fastMode) {
        CONSOLE_CTP(mContext) << "Ignoring SpeakList command in fast mode";
        return nullptr;
    }

    if (!calculateProperties())
        return nullptr;

    return SpeakListAction::make(timers, std::static_pointer_cast<CoreCommand>(shared_from_this()));
}

} // namespace apl