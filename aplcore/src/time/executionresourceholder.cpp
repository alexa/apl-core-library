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

#include "apl/time/executionresourceholder.h"
#include "apl/component/corecomponent.h"
#include "apl/time/sequencer.h"

namespace apl {

static const bool DEBUG_RESOURCE_HOLDER = false;

std::shared_ptr<ExecutionResourceHolder>
ExecutionResourceHolder::create(ExecutionResourceKey resourceKey,
                                const CoreComponentPtr& component,
                                std::function<void()>&& callback,
                                PropertyKey propKey)
{
    assert(component);
    return std::make_shared<ExecutionResourceHolder>(resourceKey, component, std::move(callback), propKey);
}

ExecutionResourceHolder::ExecutionResourceHolder(ExecutionResourceKey resourceKey,
                                                 const CoreComponentPtr& component,
                                                 std::function<void()>&& callback,
                                                 PropertyKey propKey)
    : mResource(resourceKey, component, propKey),
      mCallback(std::move(callback)),
      mContext(component->getContext())
{
}

ExecutionResourceHolder::~ExecutionResourceHolder()
{
    release();
}

void
ExecutionResourceHolder::release()
{
    // The component is no longer reliable, so we drop our callback.
    mCallback = nullptr;
}

void
ExecutionResourceHolder::takeResource()
{
    LOG_IF(DEBUG_RESOURCE_HOLDER) << mResource.toString() << " was holding=" << mHoldingResources;

    if (mHoldingResources)
        return;

    auto sequencer = getSequencer();
    if (sequencer) {
        sequencer->claimResource(mResource, shared_from_this());
        mHoldingResources = true;
    }
}

void
ExecutionResourceHolder::releaseResource()
{
    LOG_IF(DEBUG_RESOURCE_HOLDER) << mResource.toString() << " was holding=" << mHoldingResources;

    if (!mHoldingResources)
        return;

    auto sequencer = getSequencer();
    if (sequencer)
        sequencer->releaseRelatedResources(shared_from_this());

    mHoldingResources = false;
}

void
ExecutionResourceHolder::onResourceLoss()
{
    LOG_IF(DEBUG_RESOURCE_HOLDER) << mResource.toString() << " was holding=" << mHoldingResources;

    mHoldingResources = false;
    if (mCallback)
        mCallback();
}

Sequencer *
ExecutionResourceHolder::getSequencer()
{
    auto context = mContext.lock();
    return context ? &context->sequencer() : nullptr;
}

} // namespace apl