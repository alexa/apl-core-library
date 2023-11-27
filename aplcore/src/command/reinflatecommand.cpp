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

#include "apl/command/reinflatecommand.h"
#include "apl/component/hostcomponent.h"
#include "apl/time/sequencer.h"

namespace apl {

const CommandPropDefSet&
ReinflateCommand::propDefSet() const
{
    static CommandPropDefSet sReinflateCommandProperties(CoreCommand::propDefSet(), {
        { kCommandPropertyPreservedSequencers, Object::EMPTY_ARRAY(), asArray }
    });

    return sReinflateCommandProperties;
}

ActionPtr
ReinflateCommand::execute(const TimersPtr& timers, bool fastMode)
{
    if (!calculateProperties())
        return nullptr;

    auto preservedSequencersList = mValues.find(kCommandPropertyPreservedSequencers);
    if (preservedSequencersList->second.isArray()) {
        auto sequencers = std::set<std::string>();
        for (auto& so : preservedSequencersList->second.getArray()) {
            if (!so.empty()) {
                sequencers.emplace(so.asString());
            }
        }
        mContext->sequencer().setPreservedSequencers(sequencers);
    }

    if (mContext->embedded()) {
        // Directly initiate reinflate on relevant document.
        std::static_pointer_cast<HostComponent>(mContext->topComponent()->getParent())->reinflate();
        return nullptr;
    }

    if (CoreDocumentContext::cast(mContext->documentContext())->refreshContent())
        LOG(LogLevel::kDebug) << "Content re-resolution required after reinflate";

    // Return a simple action that pushes the event and does nothing else.  The view host must
    // resolve this event to allow further events in the sequencer to execute.
    return Action::make(timers, [this](ActionRef ref) {
        mContext->pushEvent(Event(kEventTypeReinflate, EventBag(), nullptr, ref));
    });
}

} // namespace apl