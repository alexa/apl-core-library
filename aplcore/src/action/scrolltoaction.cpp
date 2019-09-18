/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "apl/action/scrolltoaction.h"
#include "apl/command/corecommand.h"

namespace apl {

static const bool DEBUG_SCROLL_TO = false;


std::shared_ptr<ScrollToAction>
ScrollToAction::make(const TimersPtr& timers,
                     const std::shared_ptr<CoreCommand>& command,
                     const Rect& subBounds,
                     const ComponentPtr& target)
{
    auto t = target ? target : std::static_pointer_cast<Component>(command->target());
    if (!t)
        return nullptr;
    auto align = static_cast<CommandScrollAlign>(command->getValue(kCommandPropertyAlign).getInteger());
    auto ptr = std::make_shared<ScrollToAction>(timers, align, subBounds, command->context(), t);
    ptr->start();
    return ptr;
}

std::shared_ptr<ScrollToAction>
ScrollToAction::make(const TimersPtr& timers,
                     const std::shared_ptr<CoreCommand>& command,
                     const ComponentPtr& target)
{
    auto t = target ? target : std::static_pointer_cast<Component>(command->target());
    if (!t)
        return nullptr;
    auto align = static_cast<CommandScrollAlign>(command->getValue(kCommandPropertyAlign).getInteger());
    auto ptr = std::make_shared<ScrollToAction>(timers, align, command->context(), t);
    ptr->start();
    return ptr;
}

std::shared_ptr<ScrollToAction>
ScrollToAction::make(const TimersPtr& timers,
                     const CommandScrollAlign& align,
                     const Rect& subBounds,
                     const ContextPtr& context,
                     const ComponentPtr& target) {
    if (!target)
        return nullptr;
    auto ptr = std::make_shared<ScrollToAction>(timers, align, subBounds, context, target);
    ptr->start();
    return ptr;
}

void
ScrollToAction::start() {
    // Find a scrollable or page-able parent
    auto container = mTarget->getParent();
    while (container && container->scrollType() == kScrollTypeNone)
        container = container->getParent();

    if (!container) {
        resolve();
        return;
    }

    mTarget->ensureLayout(true);

    switch (container->scrollType()) {
        case kScrollTypeNone:
            resolve();
            break;

        case kScrollTypeVertical:
        case kScrollTypeHorizontal:
            scrollTo(container);
            break;

        case kScrollTypeHorizontalPager:
            pageTo(container);
            break;
    }
}

void
ScrollToAction::scrollTo(const std::shared_ptr<apl::Component>& scrollable)
{
    LOG_IF(DEBUG_SCROLL_TO) << "Constructing scroll to action";

    // Calculate how far we need to scroll
    Rect childBoundsInParent;
    if (!mTarget->getBoundsInParent(scrollable, childBoundsInParent)) {
        resolve();
        return;
    }

    // In the case of line highlight mode karaoke, we'll need to scroll to a bounds
    // within our target. If that's the case, adjust child bounds here.
    if(mScrollToSubBounds) {
        childBoundsInParent = Rect(
                childBoundsInParent.getX() + mSubBounds.getX(),
                childBoundsInParent.getY() + mSubBounds.getY(),
                mSubBounds.getWidth(),
                mSubBounds.getHeight());
    }

    Rect parentInnerBounds = scrollable->getCalculated(kPropertyInnerBounds).getRect();
    bool vertical = (scrollable->scrollType() == kScrollTypeVertical);

    float parentStart, parentEnd;
    float childStart, childEnd;
    float scrollTo;

    if (vertical) {
        parentStart = parentInnerBounds.getTop();
        parentEnd = parentInnerBounds.getBottom();
        childStart = childBoundsInParent.getTop();
        childEnd = childBoundsInParent.getBottom();
        scrollTo = scrollable->scrollPosition().getY();
    } else {
        parentStart = parentInnerBounds.getLeft();
        parentEnd = parentInnerBounds.getRight();
        childStart = childBoundsInParent.getLeft();
        childEnd = childBoundsInParent.getRight();
        scrollTo = scrollable->scrollPosition().getX();
    }

    LOG_IF(DEBUG_SCROLL_TO) << "parent start=" << parentStart << " end=" << parentEnd;
    LOG_IF(DEBUG_SCROLL_TO) << "child start=" << childStart << " end=" << childEnd;
    LOG_IF(DEBUG_SCROLL_TO) << "scrollPosition=" << scrollTo;

    switch (mAlign) {
        case kCommandScrollAlignFirst:
            scrollTo = childStart - parentStart;
            break;

        case kCommandScrollAlignCenter:
            scrollTo = ((childStart + childEnd) - (parentStart + parentEnd)) / 2;
            break;

        case kCommandScrollAlignLast:
            scrollTo = childEnd - parentEnd;
            break;

        case kCommandScrollAlignVisible:
            if (childStart - scrollTo < parentStart)
                scrollTo = childStart - parentStart;
            else if (childEnd - scrollTo > parentEnd)
                scrollTo = childEnd - parentEnd;
            break;
    }

    // Calculate the new position by trimming the old position plus the distance
    auto p = scrollable->trimScroll(Point(scrollTo, scrollTo));

    LOG_IF(DEBUG_SCROLL_TO) << "...distance=" << scrollTo << " position=" << p;

    // Copy properties into the command bag
    EventBag bag;
    bag.emplace(kEventPropertyPosition, Dimension(vertical ? p.getY() : p.getX()));

    auto self = shared_from_this();
    mContext->pushEvent(Event(kEventTypeScrollTo, std::move(bag), scrollable, self));
}

void
ScrollToAction::pageTo(const std::shared_ptr<apl::Component>& pager)
{
    LOG_IF(DEBUG_SCROLL_TO) << pager;

    // We have a target component to show and a pager component that (eventually) holds the target.
    // First, we need to figure out which page the target component is on.  This requires finding
    // the component WITHIN the pager that is either the target or the ancestor of the target.
    auto component = mTarget;
    while (component->getParent() != pager)
        component = component->getParent();

    int targetPage = -1;
    for (int i = 0 ; i < pager->getChildCount() ; i++) {
        if (pager->getChildAt(i) == component) {
            targetPage = i;
            break;
        }
    }

    if (targetPage == -1) {
        LOG(LogLevel::ERROR) << "Unrecoverable error in pageTo";
        resolve();
        return;
    }

    // Check if we're already on the correct page
    int currentPage = pager->pagePosition();
    if (targetPage == currentPage) {
        resolve();
        return;
    }

    // We assume we were invoked from a ScrollToComponent/Index command.  We use absolute
    // positioning.
    EventBag bag;
    bag.emplace(kEventPropertyPosition, targetPage);
    bag.emplace(kEventPropertyDirection,
        targetPage < currentPage ? kEventDirectionBackward : kEventDirectionForward);

    auto self = shared_from_this();
    mContext->pushEvent(Event(kEventTypeSetPage, std::move(bag), pager, self));
}

} // namespace apl