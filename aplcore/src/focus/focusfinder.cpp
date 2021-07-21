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

#include "apl/focus/focusfinder.h"

#include "apl/focus/beamintersect.h"
#include "apl/focus/focusvisitor.h"
#include "apl/engine/rootcontextdata.h"
#include "apl/component/corecomponent.h"

namespace apl {

const bool DEBUG_FOCUS_FINDER = false;

std::vector<CoreComponentPtr>
FocusFinder::getFocusables(const CoreComponentPtr& root, bool ignoreVisitorPruning)
{
    auto focusVisitor = FocusVisitor(root->getUniqueId(), ignoreVisitorPruning);
    root->accept(focusVisitor);
    return focusVisitor.getResult();
}

CoreComponentPtr
FocusFinder::findNext(const CoreComponentPtr& focused, FocusDirection direction)
{
    assert(focused);

    auto next = focused->getUserSpecifiedNextFocus(direction);
    if (next) {
        return next;
    }

    auto root = getImplicitFocusRoot(focused, direction);
    if (direction == kFocusDirectionForward || direction == kFocusDirectionBackwards) {
        return findNextByTabOrder(focused, root, direction);
    }
    Rect focusedRect;
    focused->getBoundsInParent(root, focusedRect);
    next = findNextInternal(root, focusedRect, direction);
    if (next != nullptr && next != focused) {
        return next;
    }

    if (!focused && root->isFocusable()) {
        return root;
    }

    if (root != focused && root->isFocusable()) {
        LOG_IF(DEBUG_FOCUS_FINDER) << "going with root";
        return root->takeFocusFromChild(direction, focusedRect);
    }
    return nullptr;
}

CoreComponentPtr
FocusFinder::findNext(
        const CoreComponentPtr& focused,
        const Rect& focusedRect,
        FocusDirection direction,
        const CoreComponentPtr& root)
{
    if (direction == kFocusDirectionForward || direction == kFocusDirectionBackwards) {
        return findNextByTabOrder(focused, root, direction);
    }
    auto next = findNextInternal(root, focusedRect, direction);
    if (next != nullptr && next != focused) {
        return next;
    }

    if (!focused && root->isFocusable()) {
        return root;
    }

    if (root != focused && root->isFocusable()) {
        LOG_IF(DEBUG_FOCUS_FINDER) << "going with root";
        return root->takeFocusFromChild(direction, focusedRect);
    }
    return nullptr;
}

CoreComponentPtr
FocusFinder::findNextInternal(const CoreComponentPtr& root, const Rect& focusedRect, FocusDirection direction)
{
    LOG_IF(DEBUG_FOCUS_FINDER) << "Root:" << root->toDebugSimpleString() << " focusedRect:" <<
        focusedRect.toDebugString() << " direction:" << direction;
    auto focusables = getFocusables(root, false);
    CoreComponentPtr bestCandidate;
    // TODO: Really simple Android-like BeamIntersect scorer. Likely needs tweaking.
    BeamIntersect bestIntersect;

    for (auto& focusable : focusables) {
        Rect candidateRect;
        focusable->getBoundsInParent(root, candidateRect);
        if (isValidCandidate(root, focusedRect, focusable, candidateRect, direction)) {
            auto candidateIntersect = BeamIntersect::build(focusedRect, candidateRect, direction);
            LOG_IF(DEBUG_FOCUS_FINDER) << "Candidate: " << focusable->toDebugSimpleString()
                                       << " intersect: " << candidateIntersect;
            if (bestIntersect.empty() || candidateIntersect > bestIntersect) {
                bestIntersect = candidateIntersect;
                bestCandidate = focusable;
            }
        }
    }

    if (bestCandidate) {
        LOG_IF(DEBUG_FOCUS_FINDER) << "Best: " << bestCandidate->toDebugSimpleString()
                                   << " intersect: " << bestIntersect;
        auto offsetFocusRect = focusedRect;
        offsetFocusRect.offset(-bestIntersect.getCandidate().getTopLeft());
        // In case if we deal with scrollable as root - we need to offset by scroll position to get to proper place in
        // relation to it's children.
        offsetFocusRect.offset(bestCandidate->scrollPosition());
        auto better = findNextInternal(bestCandidate, offsetFocusRect, direction);
        if (better && !(bestCandidate->isTouchable() && better->isTouchable())) {
            return better;
        }
        return bestCandidate;
    }

    LOG_IF(DEBUG_FOCUS_FINDER) << "Nothing to focus.";

    return nullptr;
}

CoreComponentPtr
FocusFinder::findNextByTabOrder(const CoreComponentPtr& focused, const CoreComponentPtr& root, FocusDirection direction)
{
    LOG_IF(DEBUG_FOCUS_FINDER) << "Root:" << root->getUniqueId() << " focused:"
                               << (focused ?  focused->getUniqueId() : "N/A");
    auto walkRoot = root;
    auto current = focused;

    while (walkRoot != current) {
        auto focusables = getFocusables(walkRoot, true);

        if (direction == kFocusDirectionBackwards) {
            std::reverse(focusables.begin(), focusables.end());
        }

        auto startIt = std::find(focusables.begin(), focusables.end(), focused);
        if (startIt != focusables.end()) startIt++;

        auto resultIt = std::find_if(startIt, focusables.end(), [this, root, direction](const CoreComponentPtr& comp) {
            Rect candidateRect;
            comp->getBoundsInParent(root, candidateRect);
            return isValidCandidate(root, Rect(), comp, candidateRect, direction);
        });
        if (resultIt != focusables.end()) return *resultIt;

        // Have no previously focused component and have not found any in root.
        if (!current) {
            if (!focusables.empty())
                return *focusables.begin();
            if (!focused && root->isFocusable())
                return root;
            return nullptr;
        }

        // Exiting from last available component in root.
        if (walkRoot->canConsumeFocusDirectionEvent(direction, true))
            return walkRoot->takeFocusFromChild(direction, Rect());

        // Go up in hierarchy to extend the search
        current = walkRoot;
        walkRoot = getImplicitFocusRoot(current, direction);
    }

    return nullptr;
}

CoreComponentPtr
FocusFinder::getImplicitFocusRoot(const CoreComponentPtr& focused, FocusDirection direction)
{
    assert(focused);

    CoreComponentPtr parent = focused;
    CoreComponentPtr prevParent = parent;
    do {
        prevParent = parent;
        parent = std::dynamic_pointer_cast<CoreComponent>(prevParent->getParent());
        // If parent can't take focus in direction we want - expand the search.
        if(parent && parent->isFocusable() && parent->canConsumeFocusDirectionEvent(direction, true)) {
            return parent;
        }
    } while (parent);

    return prevParent;
}

// Really simple and assumes root being fully visible. May need adjustments in future.
static bool
childIsVisible(const CoreComponentPtr& root, const CoreComponentPtr& child)
{
    CoreComponentPtr current = child;
    while (current && current != root) {
        if (current->getCalculated(kPropertyDisplay).asInt() != kDisplayNormal ||
            current->getCalculated(kPropertyOpacity).getDouble() <= 0)
            return false;
        current = std::dynamic_pointer_cast<CoreComponent>(current->getParent());
    }
    return true;
}

bool
FocusFinder::isValidCandidate(
    const CoreComponentPtr& root,
    const Rect& origin,
    const CoreComponentPtr& candidateComponent,
    const Rect& candidateRect,
    FocusDirection direction)
{
    if (candidateRect.empty())
        return false;

    // TODO: Keep in mind that checks below are quite simple. It may change in future in case when we
    //  define something not focusable/invisible but treated as one due to accessibility setting.
    if (!childIsVisible(root, candidateComponent))
        return false;

    if (candidateComponent->getState().get(kStateDisabled)) {
        return false;
    }

    // Check if child is actually in parent's viewport.
    auto rootSize = root->getCalculated(kPropertyBounds).getRect().getSize();
    auto scrollPosition = root->scrollPosition();
    auto rootBounds = Rect(scrollPosition.getX(), scrollPosition.getY(), rootSize.getWidth(), rootSize.getHeight());
    if (candidateRect.intersect(rootBounds).empty()) {
        if (!root->scrollable()) return false;
        // Scrollables as scrollables is more interesting in this extent - you may focus something which is not on
        // screen and bring it in view.
        if (!root->canConsumeFocusDirectionEvent(direction, true)) return false;
    }

    switch (direction) {
        case kFocusDirectionLeft:
            return origin.getRight() > candidateRect.getRight() && origin.getLeft() > candidateRect.getLeft();
        case kFocusDirectionRight:
            return origin.getLeft() < candidateRect.getLeft() && origin.getRight() < candidateRect.getRight();
        case kFocusDirectionUp:
            return origin.getBottom() > candidateRect.getBottom() && origin.getTop() > candidateRect.getTop();
        case kFocusDirectionDown:
            return origin.getTop() < candidateRect.getTop() && origin.getBottom() < candidateRect.getBottom();
        case kFocusDirectionForward:
        case kFocusDirectionBackwards:
            return true;
        default:
            return false;
    }
}

}