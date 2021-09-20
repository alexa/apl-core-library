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


#include "apl/focus/focusvisitor.h"
#include "apl/component/corecomponent.h"

namespace apl {

void
FocusVisitor::visit(const CoreComponent& component)
{
    bool inTheViewport = true;

    auto parent = std::static_pointer_cast<CoreComponent>(component.getParent());
    // Defer visibility to scrollable itself
    if (parent && !parent->scrollable()) {
        inTheViewport = !parent || parent->isDisplayedChild(component);
    }
    if (component.isFocusable() && component.getUniqueId() != mRootId && inTheViewport
        && !component.getInheritParentState()) {
        mFocusables.emplace_back(std::const_pointer_cast<CoreComponent>(component.shared_from_corecomponent()));
        // Sequences will define if focus should go to the child, usually
        if (!mFullTree && (component.scrollable() || component.isTouchable())) mPruneBranch = true;
    }
}

void
FocusVisitor::pop() {
    mPruneBranch = false;
}

bool
FocusVisitor::isAborted() const {
    return mPruneBranch;
}

}
