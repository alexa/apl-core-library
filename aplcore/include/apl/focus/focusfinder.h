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

#ifndef _APL_FOCUS_FINDER_H
#define _APL_FOCUS_FINDER_H

#include "apl/common.h"
#include "apl/focus/focusdirection.h"
#include "apl/primitives/rect.h"

namespace apl {

class FocusManager;

/**
 * Top level focus finder that based on comparison between next focus candidates.
 * APL Core engine specifics:
 *  - Considering clear hierarchy we operate in segment's defined by current
 *    focused component's focusable parent.
 *  - We expand wider only if no targets available in current root search.
 */
class FocusFinder {
public:
    /**
     * Find next focusable component.
     * @param focused currently focused component.
     * @param direction direction of search.
     * @return Found component, nullptr otherwise.
     */
    CoreComponentPtr findNext(const CoreComponentPtr& focused, FocusDirection direction);

    /**
     * Find next focusable component.
     * @param focused currently focused component or nullptr if none.
     * @param focusedRect currently focused rectangle.
     * @param direction direction of search.
     * @param root root of search hierarchy.
     * @return Found component, nullptr otherwise.
     */
    CoreComponentPtr findNext(
            const CoreComponentPtr& focused,
            const Rect& focusedRect,
            FocusDirection direction,
            const CoreComponentPtr& root);

    /**
     * Get all focusable components in provided root.
     * @param root top component to start search from.
     * @param ignoreVisitorPruning ignore any search tree pruning rules.
     * @return list of focusable components.
     */
    std::vector<CoreComponentPtr> getFocusables(const CoreComponentPtr& root, bool ignoreVisitorPruning);

    /**
     * Search upwards through the component's hierarchy to find the first ancestor that is focusable
     * and can accept focus in the target focus direction from a child. If no such ancestor is found,
     * return the hierarchy root.
     * @param focused currently focused component.
     * @param direction direction of search.
     */
    static CoreComponentPtr getImplicitFocusRoot(const CoreComponentPtr& focused, FocusDirection direction);

private:
    CoreComponentPtr findNextInternal(const CoreComponentPtr& root, const Rect& focusedRect, FocusDirection direction);
    CoreComponentPtr findNextByTabOrder(const CoreComponentPtr& focused, const CoreComponentPtr& root, FocusDirection direction);

    /**
     * Verify that candidate is valid in regard to origin and provided direction.
     */
    bool isValidCandidate(
            const CoreComponentPtr& root,
            const Rect& origin,
            const CoreComponentPtr& candidateComponent,
            const Rect& candidateRect,
            FocusDirection direction);
};

} // namespace apl

#endif //_APL_FOCUS_FINDER_H
