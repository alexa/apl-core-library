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

#include "apl/utils/stickyfunctions.h"
#include "apl/component/corecomponent.h"
#include "apl/component/yogaproperties.h"

namespace apl {

namespace stickyfunctions {

std::pair<CoreComponentPtr, CoreComponentPtr>
getAncestorHorizontalAndVerticalScrollable(const CoreComponentPtr &component) {
    auto ancestor = std::dynamic_pointer_cast<CoreComponent>(component->getParent());
    CoreComponentPtr horizontal, vertical;
    while (ancestor != nullptr) {
        if (horizontal != nullptr && vertical != nullptr)
            break;

        if (horizontal == nullptr && ancestor->scrollable() && ancestor->scrollType() == kScrollTypeHorizontal)
            horizontal = ancestor;

        if (vertical == nullptr && ancestor->scrollable() && ancestor->scrollType() == kScrollTypeVertical)
            vertical = ancestor;

        ancestor = std::dynamic_pointer_cast<CoreComponent>(ancestor->getParent());
    }
    return {horizontal, vertical};
}

void
updateStickyOffset(const CoreComponentPtr &component) {
    auto offset = calculateStickyOffset(component);
    // The offset for nested sticky components will be calculated in order from ancestor to descendant.
    // We update the bounds early so any sticky descendants have up to date bounds to work with
    auto currentStickyOffset = component->getStickyOffset();
    auto b = component->getCalculated(kPropertyBounds).getRect();
    b.offset(-currentStickyOffset);
    b.offset(offset);
    component->setCalculated(kPropertyBounds, std::move(b));
    component->setDirty(kPropertyBounds);
    component->setStickyOffset(offset);
}

static Point
calculateVerticalOffset(const CoreComponentPtr& component, Point& currentStickyOffset,
               const CoreComponentPtr& verticalScrollable) {
    Point calculatedOffset;
    float scrollableHeight = verticalScrollable->getCalculated(kPropertyBounds).getRect().getHeight();
    float scrollY = verticalScrollable->scrollPosition().getY();
    auto topStyleOffset = component->getCalculated(kPropertyTop);
    auto bottomStyleOffset = component->getCalculated(kPropertyBottom);
    // bounds used for computing the sticky offset are relative to the ancestor scrollable
    Rect stickyBounds;
    if (!component->getBoundsInParent(verticalScrollable, stickyBounds)) {
        assert(false);// This should never be reached.
    }
    // Remove any applied sticky offsets
    stickyBounds.offset(-currentStickyOffset);

    Rect stickyParentBounds;
    if (!component->getParent()->getBoundsInParent(verticalScrollable, stickyParentBounds)) {
        assert(false);// This should never be reached.
    }

    // If the top and bottom offsets are bigger than the scrollable height then we skip the
    // bottom offset. The same
    // applies for left and right offsets.
    bool skipBottom = false;
    if (!topStyleOffset.isAutoDimension() && !bottomStyleOffset.isAutoDimension()) {
        skipBottom = topStyleOffset.asNumber() + stickyBounds.getHeight() + bottomStyleOffset.asNumber() >
                     scrollableHeight;
    }

    if (!bottomStyleOffset.isAutoDimension() && !skipBottom) {
        float bottomToTopOffset = scrollableHeight - bottomStyleOffset.asNumber();
        float bottomOffset = bottomToTopOffset - (stickyBounds.getBottom() - scrollY);
        // The bottom offset can only decrease the Y position of our component
        bottomOffset = std::min(0.0f, bottomOffset);

        // Check if the component bounds are still inside the parents bounds
        float maxNegativeOffset = std::min(0.0f, stickyParentBounds.getY() - stickyBounds.getY());
        if (bottomOffset < maxNegativeOffset)
            bottomOffset = maxNegativeOffset;

        calculatedOffset += Point(0.0f, bottomOffset);
    }

    if (!topStyleOffset.isAutoDimension()) {
        float topOffset = topStyleOffset.asNumber() - (stickyBounds.getY() - scrollY);
        // The top offset can only increase the Y position of our component
        topOffset = std::max(0.0f, topOffset);

        // Check if the component bounds are still inside the parents bounds
        float maxOffset = std::max(0.0f, stickyParentBounds.getBottom() - stickyBounds.getBottom());
        if (topOffset > maxOffset)
            topOffset = maxOffset;

        calculatedOffset += Point(0.0f, topOffset);
    }
    return calculatedOffset;
}

Point
calculateHorizontalOffset(const CoreComponentPtr& component, const Point& currentStickyOffset,
                          const CoreComponentPtr& horizontalScrollable) {
    Point calculatedOffset;
    float scrollableWidth = horizontalScrollable->getCalculated(kPropertyBounds).getRect().getWidth();
    float scrollX = horizontalScrollable->scrollPosition().getX();
    auto leftStyleOffset = component->getCalculated(kPropertyLeft);
    auto rightStyleOffset = component->getCalculated(kPropertyRight);
    // bounds used for computing the sticky offset are relative to the ancestor scrollable
    Rect stickyBounds;
    if (!component->getBoundsInParent(horizontalScrollable, stickyBounds)) {
        assert(false);// This should never be reached.
    }
    // Remove any applied sticky offsets
    stickyBounds.offset(-currentStickyOffset);

    Rect stickyParentBounds;
    if (!component->getParent()->getBoundsInParent(horizontalScrollable, stickyParentBounds)) {
        assert(false);// This should never be reached.
    }

    bool skipRight = false;
    if (!leftStyleOffset.isAutoDimension() && !rightStyleOffset.isAutoDimension()) {
        skipRight = leftStyleOffset.asNumber() + stickyBounds.getWidth() + rightStyleOffset.asNumber() >
                    scrollableWidth;
    }

    if (!rightStyleOffset.isAutoDimension() && !skipRight) {
        float rightToLeftOffset = scrollableWidth - rightStyleOffset.asNumber();
        float rightOffset = rightToLeftOffset - (stickyBounds.getRight() - scrollX);
        // The right offset can only decrease the X position of our component
        rightOffset = std::min(0.0f, rightOffset);

        // Check if the component bounds are still inside the parents bounds
        float maxNegativeOffset = std::min(0.0f, stickyParentBounds.getX() - stickyBounds.getX());
        if (rightOffset < maxNegativeOffset)
            rightOffset = maxNegativeOffset;

        calculatedOffset += Point(rightOffset, 0.0f);
    }

    if (!leftStyleOffset.isAutoDimension()) {
        float leftOffset = leftStyleOffset.asNumber() - (stickyBounds.getX() - scrollX);
        // The left offset can only increase the X position of our component
        leftOffset = std::max(0.0f, leftOffset);

        // Check if the component bounds are still inside the parents bounds
        float maxOffset = std::max(0.0f, stickyParentBounds.getRight() - stickyBounds.getRight());
        if (leftOffset > maxOffset)
            leftOffset = maxOffset;

        calculatedOffset += Point(leftOffset, 0.0f);
    }
    return calculatedOffset;
}
Point
calculateStickyOffset(const CoreComponentPtr &component) {

    auto currentStickyOffset = component->getStickyOffset();
    Point calculatedOffset;
    auto p = getAncestorHorizontalAndVerticalScrollable(component);
    auto horizontalScrollable = std::get<0>(p);
    auto verticalScrollable   = std::get<1>(p);

    if (verticalScrollable != nullptr) {
        calculatedOffset +=
            calculateVerticalOffset(component, currentStickyOffset, verticalScrollable);
    }

    if (horizontalScrollable != nullptr) {
        calculatedOffset +=
            calculateHorizontalOffset(component, currentStickyOffset, horizontalScrollable);
    }

    return calculatedOffset;
}

}  // namespace stickyfunctions

}  // namespace apl
