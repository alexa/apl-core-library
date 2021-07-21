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

#include "apl/action/scrolltoaction.h"
#include "apl/command/corecommand.h"
#include "apl/time/sequencer.h"
#include "apl/component/pagercomponent.h"
#include "apl/content/rootconfig.h"

namespace apl {

static const bool DEBUG_SCROLL_TO = false;

ScrollToAction::ScrollToAction(const TimersPtr& timers,
                               const CommandScrollAlign& align,
                               const Rect& subBounds,
                               const ContextPtr& context,
                               bool scrollToSubBounds,
                               const CoreComponentPtr& target,
                               const CoreComponentPtr& scrollableParent,
                               apl_duration_t duration)
        : AnimatedScrollAction(timers, context, scrollableParent, duration),
          mAlign(align),
          mSubBounds(subBounds),
          mScrollToSubBounds(scrollToSubBounds),
          mTarget(target)
{}

std::shared_ptr<ScrollToAction>
ScrollToAction::make(const TimersPtr& timers,
                     const std::shared_ptr<CoreCommand>& command,
                     const Rect& subBounds,
                     const CoreComponentPtr& target)
{
    auto t = target ? target : command->target();
    if (!t)
        return nullptr;
    auto align = static_cast<CommandScrollAlign>(command->getValue(kCommandPropertyAlign).getInteger());
    return make(timers, align, subBounds, command->context(), true, t);
}

std::shared_ptr<ScrollToAction>
ScrollToAction::make(const TimersPtr& timers,
                     const std::shared_ptr<CoreCommand>& command,
                     const CoreComponentPtr& target)
{
    auto t = target ? target : command->target();
    if (!t)
        return nullptr;
    auto align = static_cast<CommandScrollAlign>(command->getValue(kCommandPropertyAlign).getInteger());
    return make(timers, align, Rect(), command->context(), false, t);
}

std::shared_ptr<ScrollToAction>
ScrollToAction::makeUsingSnap(const TimersPtr& timers,
                              const CoreComponentPtr& target,
                              apl_duration_t duration)
{
    return make(timers, kCommandScrollAlignVisible, Rect(), target->getContext(), false,
                target, duration, true);
}

std::shared_ptr<ScrollToAction>
ScrollToAction::make(const TimersPtr& timers,
                     const CommandScrollAlign& align,
                     const Rect& subBounds,
                     const ContextPtr& context,
                     const CoreComponentPtr& target) {
    return make(timers, align, subBounds, context, true, target);
}

std::shared_ptr<ScrollToAction>
ScrollToAction::make(const TimersPtr& timers,
                     const CommandScrollAlign& align,
                     const Rect& subBounds,
                     const ContextPtr& context,
                     bool scrollToSubBounds,
                     const CoreComponentPtr& target,
                     apl_duration_t duration,
                     bool useSnap) {
    if (!target)
        return nullptr;

    // Find a scrollable or page-able parent
    auto container = target->getParent();
    while (container && container->scrollType() == kScrollTypeNone)
        container = container->getParent();

    if (!container)
        return nullptr;

    auto resultingAlign = align;

    if (useSnap) {
        LOG_IF(DEBUG_SCROLL_TO) << "Ignoring provided align and using component defined snap.";
        auto snapObject = container->getCalculated(kPropertySnap);
        if (!snapObject.isNull()) {
            auto snap = static_cast<Snap>(snapObject.getInteger());
            switch (snap) {
                case kSnapStart:
                case kSnapForceStart:
                    resultingAlign = kCommandScrollAlignFirst;
                    break;
                case kSnapCenter:
                case kSnapForceCenter:
                    resultingAlign = kCommandScrollAlignCenter;
                    break;
                case kSnapEnd:
                case kSnapForceEnd:
                    resultingAlign = kCommandScrollAlignLast;
                    break;
                default:
                    resultingAlign = kCommandScrollAlignVisible;
                    break;
            }
        }
    }

    auto ptr = std::make_shared<ScrollToAction>(
            timers,
            resultingAlign,
            subBounds,
            context,
            scrollToSubBounds,
            target,
            std::dynamic_pointer_cast<CoreComponent>(container),
            duration > 0 ? duration : context->getRootConfig().getScrollCommandDuration());

    context->sequencer().claimResource({kExecutionResourcePosition, container}, ptr);

    ptr->start();
    return ptr;
}

void
ScrollToAction::start() {
    // Find a scrollable or page-able parent
    mContainer->ensureChildLayout(mTarget, true);

    switch (mContainer->scrollType()) {
        case kScrollTypeNone:
            resolve();
            break;

        case kScrollTypeVertical:
        case kScrollTypeHorizontal:
            scrollTo();
            break;

        case kScrollTypeVerticalPager:
        case kScrollTypeHorizontalPager:
            pageTo();
            break;
    }
}

void
ScrollToAction::scrollTo()
{
    LOG_IF(DEBUG_SCROLL_TO) << "Constructing scroll to action";

    // Calculate how far we need to scroll
    Rect childBoundsInParent;
    if (!mTarget->getBoundsInParent(mContainer, childBoundsInParent)) {
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

    Rect parentInnerBounds = mContainer->getCalculated(kPropertyInnerBounds).getRect();
    bool vertical = (mContainer->scrollType() == kScrollTypeVertical);
    bool isLTR = mContainer->getCalculated(kPropertyLayoutDirection) == kLayoutDirectionLTR;

    float parentStart, parentEnd;
    float childStart, childEnd;
    float scrollTo;
    bool beforeParentStart, afterParentEnd;

    if (vertical) {
        parentStart = parentInnerBounds.getTop();
        parentEnd = parentInnerBounds.getBottom();
        childStart = childBoundsInParent.getTop();
        childEnd = childBoundsInParent.getBottom();
        scrollTo = mContainer->scrollPosition().getY();
        beforeParentStart = childStart - scrollTo < parentStart;
        afterParentEnd = childEnd - scrollTo > parentEnd;
    } else if (isLTR) { // Horizontal LTR
        parentStart = parentInnerBounds.getLeft();
        parentEnd = parentInnerBounds.getRight();
        childStart = childBoundsInParent.getLeft();
        childEnd = childBoundsInParent.getRight();
        scrollTo = mContainer->scrollPosition().getX();
        beforeParentStart = childStart - scrollTo < parentStart;
        afterParentEnd = childEnd - scrollTo > parentEnd;
    } else { // Horizontal RTL
        parentStart = parentInnerBounds.getRight();
        parentEnd = parentInnerBounds.getLeft();
        childStart = childBoundsInParent.getRight();
        childEnd = childBoundsInParent.getLeft();
        scrollTo = mContainer->scrollPosition().getX();
        beforeParentStart = childStart - scrollTo > parentStart;
        afterParentEnd = childEnd - scrollTo < parentEnd;
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
            if (beforeParentStart)
                scrollTo = childStart - parentStart;
            else if (afterParentEnd)
                scrollTo = childEnd - parentEnd;
            break;
    }

    // Calculate the new position by trimming the old position plus the distance
    auto p = mContainer->trimScroll(Point(scrollTo, scrollTo));

    LOG_IF(DEBUG_SCROLL_TO) << "...distance=" << scrollTo << " position=" << p;

    scroll(vertical, p);
}

void
ScrollToAction::pageTo()
{
    LOG_IF(DEBUG_SCROLL_TO) << mContainer;

    // We have a target component to show and a pager component that (eventually) holds the target.
    // First, we need to figure out which page the target component is on.  This requires finding
    // the component WITHIN the pager that is either the target or the ancestor of the target.
    auto component = mTarget;
    while (component->getParent() != mContainer)
        component = std::static_pointer_cast<CoreComponent>(component->getParent());

    int targetPage = -1;
    for (int i = 0 ; i < mContainer->getChildCount() ; i++) {
        if (mContainer->getChildAt(i) == component) {
            targetPage = i;
            break;
        }
    }

    if (targetPage == -1) {
        LOG(LogLevel::kError) << "Unrecoverable error in pageTo";
        resolve();
        return;
    }

    // Check if we're already on the correct page
    int currentPage = mContainer->pagePosition();
    if (targetPage == currentPage) {
        resolve();
        return;
    }

    // We assume we were invoked from a ScrollToComponent/Index command.  We use absolute
    // positioning.
    PagerComponent::setPageUtil(mContext, mContainer, targetPage,
        targetPage < currentPage ? kPageDirectionBack : kPageDirectionForward, shared_from_this(),
        mContext->getRequestedAPLVersion().compare("1.6") < 0);
}

} // namespace apl