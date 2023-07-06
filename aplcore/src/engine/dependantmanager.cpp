/*
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
 *
 */

#include "apl/engine/dependant.h"
#include "apl/engine/dependantmanager.h"
#include "apl/utils/log.h"

namespace apl {

const bool DEBUG_DEPENDANT_MANAGER = false;

/**
 * We enqueue dependencies into the list sorted in topological order.
 * For now we store them in normal order in a vector - which means we are generally
 * adding to the _back_ of the vector, but popping elements from the _front.  Arguably
 * we could store them in reverse order (since popping from the back is fast), but that
 * would mean most add operations are in the front.  Or we could try a dequeue or linked-list
 * approach.  We'll keep it simple until there is time to do performance measurements.
 */
void
DependantManager::enqueueDependency(const DependantPtr& dependant)
{
    assert(dependant);
    LOG_IF(DEBUG_DEPENDANT_MANAGER) << "Enqueue dependant: " << dependant->toDebugString();

    // Find the first element in the queue that is greater-than or equal-to dependant
    auto it = std::lower_bound(mProcessList.begin(),
                               mProcessList.end(),
                               dependant,
                               [](const DependantPtr& lhs,
                                  const DependantPtr& rhs){
                                   return *lhs < *rhs;
                               });

    // Check if it is already in the queue
    if (it != mProcessList.end() && *it == dependant)
        return;

    mProcessList.insert(it, dependant);
}

void
DependantManager::processDependencies(bool useDirtyFlag)
{
    while (!mProcessList.empty()) {
        // Pop the dependency off the front
        auto dependant = mProcessList.front();
        LOG_IF(DEBUG_DEPENDANT_MANAGER) << "Processing dependant: " << dependant->toDebugString();
        mProcessList.erase(mProcessList.begin());

        dependant->recalculate(useDirtyFlag);
    }
}

} // namespace apl