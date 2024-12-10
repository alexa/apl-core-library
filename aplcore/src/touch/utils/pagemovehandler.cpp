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

#include "apl/touch/utils/pagemovehandler.h"

#include "apl/animation/easing.h"
#include "apl/component/component.h"
#include "apl/component/pagercomponent.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/arrayify.h"
#include "apl/engine/evaluate.h"
#include "apl/primitives/transform.h"
#include "apl/time/sequencer.h"
#include "apl/utils/bimap.h"
#include "apl/utils/make_unique.h"

namespace apl {

static const Bimap<PageMoveDrawOrder, std::string> sPageMoveDrawOrderBimap = {
    {kPageMoveDrawOrderNextAbove,   "nextAbove"},
    {kPageMoveDrawOrderNextAbove,   "next-above"},
    {kPageMoveDrawOrderNextBelow,   "nextBelow"},
    {kPageMoveDrawOrderNextBelow,   "next-below"},
    {kPageMoveDrawOrderHigherAbove, "higherAbove"},
    {kPageMoveDrawOrderHigherAbove, "higher-above"},
    {kPageMoveDrawOrderHigherBelow, "higherBelow"},
    {kPageMoveDrawOrderHigherBelow, "higher-below"},
};

std::unique_ptr<PageMoveHandler>
PageMoveHandler::create(
    const CoreComponentPtr& component,
    const Object& array,
    SwipeDirection swipeDirection,
    PageDirection pageDirection,
    int currentPage,
    int targetPage)
{
    auto currentChild = component->getCoreChildAt(currentPage);
    auto nextChild = component->getCoreChildAt(targetPage);

    if (!currentChild || !nextChild) {
        // Fail miserably. Should not happen.
        return nullptr;
    }

    // Evaluating against event context to allow "when" to work properly
    auto eventContext = createPageMoveContext(0.0f, swipeDirection, pageDirection, component, currentChild, nextChild);
    for (const auto& object : array.getArray()) {
        if (!propertyAsBoolean(*eventContext, object, "when", true)) {
            continue;
        }

        auto commands = arrayifyPropertyAsObject(*eventContext, object, "commands");
        auto action = optionalMappedProperty<PageMoveDrawOrder>(*eventContext, object, "drawOrder",
                                                                kPageMoveDrawOrderHigherAbove,
                                                                sPageMoveDrawOrderBimap);

        return std::make_unique<PageMoveHandler>(std::move(commands), action,
            swipeDirection, pageDirection, currentChild, nextChild);
    }

    // Go for default handling.
    auto pager = PagerComponent::cast(component);
    auto layoutDireciton = static_cast<LayoutDirection>(component->getCalculated(kPropertyLayoutDirection).asInt());
    bool fromLeft = pageDirection == PageDirection::kPageDirectionForward;
    // Flip direction for RTL layout
    if (pager->isHorizontal() && layoutDireciton == kLayoutDirectionRTL) {
        fromLeft = !fromLeft;
    }

    auto currentChildTransform = getPageTransformation(pager, false, fromLeft);
    auto targetChildTransform = getPageTransformation(pager, true, fromLeft);
    auto handler = std::make_unique<PageMoveHandler>(swipeDirection, pageDirection, currentChild,
        nextChild, std::move(currentChildTransform), std::move(targetChildTransform));
    handler->reset();
    return handler;
}

PageMoveHandler::PageMoveHandler(
    Object&& commands,
    PageMoveDrawOrder drawOrder,
    SwipeDirection swipeDirection,
    PageDirection pageDirection,
    const CoreComponentPtr& currentPage,
    const CoreComponentPtr& targetPage) :
    mCommands(std::move(commands)),
    mDrawOrder(drawOrder),
    mSwipeDirection(swipeDirection),
    mPageDirection(pageDirection),
    mCurrentPage(currentPage),
    mTargetPage(targetPage)
{}

PageMoveHandler::PageMoveHandler(
    SwipeDirection swipeDirection,
    PageDirection pageDirection,
    const CoreComponentPtr& currentPage,
    const CoreComponentPtr& targetPage,
    ITPtr&& currentPageTransform,
    ITPtr&& targetPageTransform) :
    mCommands(Object::NULL_OBJECT()),
    mDrawOrder(kPageMoveDrawOrderHigherAbove),
    mSwipeDirection(swipeDirection),
    mPageDirection(pageDirection),
    mCurrentPage(currentPage),
    mTargetPage(targetPage),
    mCurrentPageTransform(std::move(currentPageTransform)),
    mTargetPageTransform(std::move(targetPageTransform))
{}

void
PageMoveHandler::reset()
{
    // Reset opacity and transforms
    auto currentPage = mCurrentPage.lock();
    if (currentPage && currentPage->isValid()) {
        currentPage->setProperty(kPropertyOpacity, 1.0f);
        currentPage->setProperty(kPropertyTransformAssigned, Object::EMPTY_ARRAY());
    }
    auto targetPage = mTargetPage.lock();
    if (targetPage && targetPage->isValid()) {
        targetPage->setProperty(kPropertyOpacity, 1.0f);
        targetPage->setProperty(kPropertyTransformAssigned, Object::EMPTY_ARRAY());
    }
}

void
PageMoveHandler::execute(const CoreComponentPtr& component, float amount)
{
    auto currentPage = mCurrentPage.lock();
    auto targetPage = mTargetPage.lock();

    if (!currentPage || !targetPage) {
        return;
    }

    if (mCommands.isNull()) {
        auto animationEasing = component->getRootConfig().getProperty(RootProperty::kDefaultPagerAnimationEasing).get<Easing>();
        executeDefaultPagingAnimation(animationEasing->calc(amount), currentPage, targetPage);
    } else {
        component->getContext()->sequencer().executeCommands(mCommands,
            createPageMoveContext(amount, mSwipeDirection, mPageDirection, component, currentPage, targetPage), component, true);
    }
}

int
PageMoveHandler::getTargetPageIndex(const CoreComponentPtr& component) const {
    if (auto targetPage = mTargetPage.lock()) {
        return component->getChildIndex(targetPage);
    }
    return -1;
}

CoreComponentPtr
PageMoveHandler::getCheckedCurrentPage(const CoreComponentPtr& component) const {
    if (auto currentPage = mCurrentPage.lock()) {
        if (component->getChildIndex(currentPage) >= 0) {
            return currentPage;
        }
    }
    return nullptr;
}

CoreComponentPtr
PageMoveHandler::getCheckedTargetPage(const CoreComponentPtr& component) const {
    if (auto targetPage = mTargetPage.lock()) {
        if (component->getChildIndex(targetPage) >= 0) {
            return targetPage;
        }
    }
    return nullptr;
}

ContextPtr
PageMoveHandler::createPageMoveContext(float amount, SwipeDirection direction, PageDirection pageDirection,
                                       const CoreComponentPtr& self, const CoreComponentPtr& currentChild,
                                       const CoreComponentPtr& nextChild)
{
    auto eventProps = std::make_shared<ObjectMap>();
    eventProps->emplace("amount", amount);
    eventProps->emplace("direction", sSwipeDirectionMap.at(direction));
    eventProps->emplace("forward", pageDirection == kPageDirectionForward);

    auto current = std::make_shared<ObjectMap>();
    current->emplace("id", currentChild->getId());
    current->emplace("uid", currentChild->getUniqueId());

    auto next = std::make_shared<ObjectMap>();
    next->emplace("id", nextChild->getId());
    next->emplace("uid", nextChild->getUniqueId());

    eventProps->emplace("currentChild", current);
    eventProps->emplace("nextChild", next);

    return self->createEventContext("Move", eventProps);
}

void
PageMoveHandler::executeDefaultPagingAnimation(
    float amount,
    const CoreComponentPtr& currentChild,
    const CoreComponentPtr& nextChild)
{
    if (mCurrentPageTransform && currentChild->isValid()) {
        mCurrentPageTransform->interpolate(amount);
        currentChild->setProperty(kPropertyTransformAssigned, Object(mCurrentPageTransform));
        currentChild->markProperty(kPropertyTransformAssigned);
    }

    if (mTargetPageTransform && nextChild->isValid()) {
        mTargetPageTransform->interpolate(amount);
        nextChild->setProperty(kPropertyTransformAssigned, Object(mTargetPageTransform));
        nextChild->markProperty(kPropertyTransformAssigned);
    }
}

std::shared_ptr<InterpolatedTransformation>
PageMoveHandler::getPageTransformation(const PagerPtr& pager, bool comeIn, bool fromLeft)
{
    const auto& bounds = pager->getCalculated(kPropertyInnerBounds).get<Rect>();
    auto from = std::make_shared<ObjectMap>();
    auto to = std::make_shared<ObjectMap>();

    auto targetTranslate = pager->isHorizontal() ? "translateX" : "translateY";
    auto startShift = pager->isHorizontal() ? bounds.getWidth() : bounds.getHeight();

    if (comeIn) {
        from->emplace(targetTranslate, Object(fromLeft ? startShift : -startShift));
        to->emplace(targetTranslate, Object(0));
    } else {
        from->emplace(targetTranslate, Object(0));
        to->emplace(targetTranslate, Object(fromLeft ? -startShift : startShift));
    }

    return InterpolatedTransformation::create(*pager->getContext(), {from}, {to});
}

} // namespace apl
