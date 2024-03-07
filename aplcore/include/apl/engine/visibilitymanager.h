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

#ifndef _APL_VISIBILITY_MANAGER_H
#define _APL_VISIBILITY_MANAGER_H

#include <map>
#include <set>

#include "apl/common.h"

namespace apl {

/**
 * Simple manager class to take care of any tracked visibility change propagation and processing.
 */
class VisibilityManager {
public:
    VisibilityManager() = default;

    /**
     * Register component for visibility updates.
     * @param component target component.
     */
    void registerForUpdates(const CoreComponentPtr& component);

    /**
     * De-register component from visibility updates.
     * @param component target component.
     */
    void deregister(const CoreComponentPtr& component);

    /**
     * Mark component's visibility as dirty. Only directly registered components or their ancestors
     * will be marked.
     * @param component target component.
     */
    void markDirty(const CoreComponentPtr& component);

    /**
     * Process list of dirty components and report visibility changes if required.
     * Happens once per frame.
     */
    void processVisibilityChanges();

private:
    struct VisibilityState {
        double visibleRegionPercentage;
        double cumulativeOpacity;
    };

    WeakPtrMap<CoreComponent, VisibilityState> mTrackedComponentVisibility;
    WeakPtrSet<CoreComponent> mDirtyVisibility;
};

} // namespace apl

#endif // _APL_VISIBILITY_MANAGER_H