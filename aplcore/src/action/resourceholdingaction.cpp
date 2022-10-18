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

#include "apl/action/resourceholdingaction.h"
#include "apl/command/corecommand.h"
#include "apl/time/sequencer.h"
#include "apl/engine/rootcontext.h"

namespace apl {

ResourceHoldingAction::ResourceHoldingAction(const TimersPtr& timers,
                                             const ContextPtr& context)
        : Action(timers),
          mContext(context)
{}

void
ResourceHoldingAction::onFinish()
{
    mContext->sequencer().releaseRelatedResources(shared_from_this());
}

void
ResourceHoldingAction::freeze()
{
    mContext = nullptr;
    Action::freeze();
}

bool
ResourceHoldingAction::rehydrate(const RootContext& context)
{
    if (!Action::rehydrate(context)) return false;
    mContext = context.contextPtr();
    return true;
}

} // namespace apl