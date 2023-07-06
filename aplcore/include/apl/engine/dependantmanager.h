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

#ifndef _APL_DEPENDANT_MANAGER_H
#define _APL_DEPENDANT_MANAGER_H

#include <vector>

#include "apl/common.h"

namespace apl {

/**
 * Manage topological sorting and dependency propagation for a single root context.  All
 * documents sharing one view port use the same manager because dependencies can propagate
 * across documents.
 *
 * The manager is responsible for assigning topological sort IDs as the dependencies are
 * generated and for processing the dependencies in sort order as they are triggered.
 */
class DependantManager {
public:
    DependantManager() = default;

    /**
     * @return The next absolute sort order to use for this root context
     */
    id_type getNextSortOrder() { return mSortOrderGenerator++; }

    /**
     * Add a dependency to the "to-be-processed" list.
     * @param dependant The dependant to add to the list
     */
    void enqueueDependency(const DependantPtr& dependant);

    /**
     * Process the list of dependencies until it is empty.
     */
    void processDependencies(bool useDirtyFlag);

private:
    id_type mSortOrderGenerator = 10;  // Start at a non-zero value to help debugging
    std::vector<DependantPtr> mProcessList;  // Sorted list of dependants.  Could be a deque
};

} // namespace apl

#endif