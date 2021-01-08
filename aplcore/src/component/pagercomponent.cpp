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

#include "apl/command/setpagecommand.h"
#include "apl/component/componentpropdef.h"
#include "apl/component/pagercomponent.h"
#include "apl/component/yogaproperties.h"
#include "apl/content/rootconfig.h"
#include "apl/primitives/keyboard.h"
#include "apl/time/sequencer.h"

#include "apl/touch/gestures/pagerflinggesture.h"
#include "apl/time/timemanager.h"
#include "apl/utils/pagemovehandler.h"

namespace apl {

CoreComponentPtr
PagerComponent::create(const ContextPtr& context,
                       Properties&& properties,
                       const std::string& path) {
    auto ptr = std::make_shared<PagerComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

PagerComponent::PagerComponent(const ContextPtr& context,
                               Properties&& properties,
                               const std::string& path)
        : ActionableComponent(context, std::move(properties), path)
{}

inline Object
defaultWidth(Component& component, const RootConfig& rootConfig) {
    return rootConfig.getDefaultComponentWidth(component.getType());
}

inline Object
defaultHeight(Component& component, const RootConfig& rootConfig) {
    return rootConfig.getDefaultComponentHeight(component.getType());
}

const ComponentPropDefSet&
PagerComponent::propDefSet() const {
    static ComponentPropDefSet sPagerComponentProperties(ActionableComponent::propDefSet(), {
        {kPropertyHeight,         Dimension(100),             asNonAutoDimension,  kPropIn, yn::setHeight, defaultHeight},
        {kPropertyWidth,          Dimension(100),             asNonAutoDimension,  kPropIn, yn::setWidth,  defaultWidth},
        {kPropertyInitialPage,    0,                          asInteger,           kPropIn},
        {kPropertyNavigation,     kNavigationWrap,            sNavigationMap,      kPropInOut | kPropVisualContext},
        {kPropertyPageDirection,  kScrollDirectionHorizontal, sScrollDirectionMap, kPropIn},
        {kPropertyHandlePageMove, Object::EMPTY_ARRAY(),      asArray,             kPropIn},
        {kPropertyOnPageChanged,  Object::EMPTY_ARRAY(),      asCommand,           kPropIn},
        {kPropertyCurrentPage,    0,                          asInteger,           kPropRuntimeState | kPropVisualContext}
    });

    return sPagerComponentProperties;
}

void
PagerComponent::initialize() {
    CoreComponent::initialize();

    // Set current page. It will be clipped, if required, later.
    int currentPage = getCalculated(kPropertyInitialPage).asInt();
    mCalculated.set(kPropertyCurrentPage, currentPage);

    // If native gestures enabled - register them.
    if (getRootConfig().experimentalFeatureEnabled(RootConfig::kExperimentalFeatureHandleScrollingAndPagingInCore)) {
        mGestureHandlers.emplace_back(PagerFlingGesture::create(std::static_pointer_cast<ActionableComponent>(shared_from_this())));
    }
}

void
PagerComponent::update(UpdateType type, float value) {
    if (type == kUpdatePagerPosition || type == kUpdatePagerByEvent) {
        // Update is used only in case if change of the page was performed by viewhost implementation (non-native gesture
        // processing). Animations is up to viewhost so we only update core internal state.
        int currentPage = pagePosition();
        if (value != currentPage) {
            setPage(value);
            executePageChangeEvent(type == kUpdatePagerByEvent);
        }
    } else
        CoreComponent::update(type, value);
}

void
PagerComponent::setPage(int page)
{
    mCalculated.set(kPropertyCurrentPage, page);
    setVisualContextDirty();
    if (attachCurrentAndReportLoaded()) {
        auto width = YGNodeLayoutGetWidth(mYGNodeRef);
        auto height = YGNodeLayoutGetHeight(mYGNodeRef);
        layout(width, height, true);
    }
    if (getRootConfig().experimentalFeatureEnabled(RootConfig::kExperimentalFeatureHandleScrollingAndPagingInCore))
        setDirty(kPropertyCurrentPage);
}

void
PagerComponent::setPageUtil(
        const apl::ContextPtr& context,
        const apl::ComponentPtr& target,
        int index,
        PageDirection direction,
        const ActionRef& ref,
        bool skipDefaultAnimation)
{
    if (context->getRootConfig().experimentalFeatureEnabled(RootConfig::kExperimentalFeatureHandleScrollingAndPagingInCore)) {
        auto pager = std::dynamic_pointer_cast<PagerComponent>(target);
        pager->handleSetPage(index, direction, ref, skipDefaultAnimation);
    } else {
        EventBag bag;
        bag.emplace(kEventPropertyPosition, index);
        bag.emplace(kEventPropertyDirection, direction == kPageDirectionForward ?
            kEventDirectionForward : kEventDirectionBackward);
        context->pushEvent(Event(kEventTypeSetPage, std::move(bag), target, ref));
    }
}

void
PagerComponent::handleSetPage(int index, PageDirection direction, const ActionRef& ref, bool skipDefaultAnimation)
{
    auto currentPage = pagePosition();

    // Have to do that here for now in order to give viewhost possibility to load the next page
    // if not there
    setPage(index);

    // Set initial state
    startPageMove(direction, currentPage, index);

    // Animate if required.
    std::weak_ptr<PagerComponent> weak_ptr(std::dynamic_pointer_cast<PagerComponent>(shared_from_this()));
    disableGestures();
    if (mPageMoveHandler && !(mPageMoveHandler->isDefault() && skipDefaultAnimation)) {
        auto duration = getRootConfig().getDefaultPagerAnimationDuration();
        mCurrentAnimation = Action::makeAnimation(getRootConfig().getTimeManager(),
            duration, [weak_ptr, duration](apl_duration_t offset){
              auto self = weak_ptr.lock();
              if (self) {
                  self->executePageMove(offset/duration);
              }
            });
    } else {
        mCurrentAnimation = Action::makeDelayed(getRootConfig().getTimeManager(), 0);
    }

    mCurrentAnimation->then([weak_ptr, ref](const ActionPtr& actionPtr){
      auto self = weak_ptr.lock();
      if (self) {
          self->enableGestures();
          self->endPageMove(true, ref);
      } else {
          ref.resolve();
      }
    });
}

ActionPtr
PagerComponent::executePageChangeEvent(bool fast)
{
    ContextPtr eventContext = createEventContext("Page");
    return mContext->sequencer().executeCommands(
            getCalculated(kPropertyOnPageChanged),
            eventContext,
            shared_from_corecomponent(),
            fast);  // If the user set the pager, run in fast mode.
}

void
PagerComponent::resetPage(int page)
{
    // Currently only ZOrder. May be more in future.
    auto child = getCoreChildAt(page);
    if (page == pagePosition()) {
        child->setZOrder(1);
    } else {
        child->setZOrder(0);
    }
}

void
PagerComponent::startPageMove(PageDirection direction, int currentPage, int targetPage)
{
    auto swipeDirection = isHorizontal() ?
        (direction == kPageDirectionForward ? kSwipeDirectionLeft : kSwipeDirectionRight) :
        (direction == kPageDirectionForward ? kSwipeDirectionUp : kSwipeDirectionDown);

    auto currentChild = getCoreChildAt(currentPage);
    auto nextChild = getCoreChildAt(targetPage);

    // Reset opacity, transforms and interactivity here
    currentChild->setProperty(kPropertyOpacity, 1.0f);
    currentChild->setProperty(kPropertyTransformAssigned, Object::EMPTY_ARRAY());
    nextChild->setProperty(kPropertyOpacity, 1.0f);
    nextChild->setProperty(kPropertyTransformAssigned, Object::EMPTY_ARRAY());

    auto handlerObject = getCalculated(kPropertyHandlePageMove);

    // We cache it here. It makes no sense to change handler while it's in animation .
    mPageMoveHandler = PageMoveHandler::create(shared_from_corecomponent(), handlerObject,
        swipeDirection, direction, currentPage, targetPage);

    if (mPageMoveHandler) {
        // Ok, based on ordering setting we need to set z-order in proper way
        bool nextAbove = false;
        switch (mPageMoveHandler->getDrawOrder()) {
            case kPageMoveDrawOrderNextAbove:
                nextAbove = true;
                break;
            case kPageMoveDrawOrderHigherAbove:
                nextAbove = (direction == kPageDirectionForward);
                break;
            case kPageMoveDrawOrderHigherBelow:
                nextAbove = (direction == kPageDirectionBack);
                break;
            default:
                break;
        }

        currentChild->setZOrder( nextAbove ? 1 : 2 );
        nextChild->setZOrder( nextAbove ? 2 : 1 );
    }
}

void
PagerComponent::executePageMove(float amount)
{
    // If no handler exist - ignore it. Called out of order.
    if (mPageMoveHandler)
        mPageMoveHandler->execute(shared_from_corecomponent(), amount);
}

void
PagerComponent::endPageMove(bool fulfilled, const ActionRef& ref, bool fast)
{
    // If no handler exist - ignore it. Called out of order.
    if (!mPageMoveHandler) return;

    auto targetPage = mPageMoveHandler->getTargetPage();
    auto currentPage = mPageMoveHandler->getCurrentPage();

    if (fulfilled) {
        // Ensure internal state for lazy loading.
        setPage(targetPage);

        // Clear page states
        resetPage(currentPage);
        resetPage(targetPage);

        auto event = executePageChangeEvent(fast);
        if (event && event->isPending()) {
            event->then([ref](const ActionPtr& ptr) { ref.resolve(); });
        }
        else if (!ref.isEmpty()) {
            ref.resolve();
        }
    } else {
        resetPage(currentPage);
        resetPage(targetPage);
        if (!ref.isEmpty()) {
            ref.resolve();
        }
    }
    mPageMoveHandler = nullptr;
    mCurrentAnimation = nullptr;
}

const ComponentPropDefSet*
PagerComponent::layoutPropDefSet() const {
    static ComponentPropDefSet sPagerChildProperties = ComponentPropDefSet().add(
        {
            // Force absolute position because the pager children each occupy the entire space of their parent
            {kPropertyPosition, kPositionAbsolute, sPositionMap, kPropOut | kPropResetOnRemove, yn::setPositionType},

            // The width and height of the children of a pager are set to 100%
            {kPropertyWidth, Dimension(DimensionType::Relative, 100), asNonAutoDimension, kPropNone, yn::setWidth },
            {kPropertyHeight, Dimension(DimensionType::Relative, 100), asNonAutoDimension, kPropNone, yn::setHeight}
        });

    return &sPagerChildProperties;
}

const EventPropertyMap &
PagerComponent::eventPropertyMap() const
{
    static EventPropertyMap sPagerEventProperties = eventPropertyMerge(
            CoreComponent::eventPropertyMap(),
            {
                    {"page", [](const CoreComponent *c) { return c->pagePosition(); }},
            });

    return sPagerEventProperties;
}

ScrollType
PagerComponent::scrollType() const
{
    if (getRootConfig().experimentalFeatureEnabled(RootConfig::kExperimentalFeatureHandleScrollingAndPagingInCore)) {
        return getCalculated(kPropertyPageDirection) == kScrollDirectionVertical ? kScrollTypeVerticalPager
                                                                                 : kScrollTypeHorizontalPager;
    }
    return kScrollTypeHorizontalPager;
}

PageDirection
PagerComponent::pageDirection() const {
    // If we don't have children, there is no navigation
    auto len = mChildren.size();
    if (len <= 1)
        return kPageDirectionNone;

    auto navigation = static_cast<Navigation>(mCalculated.get(kPropertyNavigation).asInt());
    int currentPage = pagePosition();
    switch (navigation) {
        case kNavigationNormal:  // No wrapping, forward or back supported
            if (currentPage == 0)
                return kPageDirectionForward;
            if (currentPage == len - 1)
                return kPageDirectionBack;
            return kPageDirectionBoth;

        case kNavigationNone:
            return kPageDirectionNone;

        case kNavigationWrap:
            return kPageDirectionBoth;

        case kNavigationForwardOnly:
            if (currentPage == len - 1)
                return kPageDirectionNone;

            return kPageDirectionForward;
    }
    return kPageDirectionNone;
}

std::map<int, float>
PagerComponent::getChildrenVisibility(float realOpacity, const Rect& visibleRect) const {
    std::map<int, float> result;

    if (!mChildren.empty()) {
        int currentPage = pagePosition();
        auto child = mChildren.at(currentPage);
        auto childVisibility = child->calculateVisibility(realOpacity, visibleRect);
        if (childVisibility > 0.0) {
            result.emplace(currentPage, childVisibility);
        }
    }

    return result;
}

bool
PagerComponent::getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator) {
    bool actionable = CoreComponent::getTags(outMap, allocator);
    if (mChildren.size() > 1) {
        bool allowForward = false;
        bool allowBackwards = false;

        switch (pageDirection()) {
            case kPageDirectionBoth:
                allowForward = true;
                allowBackwards = true;
                break;
            case kPageDirectionBack:
                allowBackwards = true;
                break;
            case kPageDirectionForward:
                allowForward = true;
                break;
            case kPageDirectionNone:
                break;
            default:
                break;
        }

        rapidjson::Value pager(rapidjson::kObjectType);
        pager.AddMember("index", pagePosition(), allocator);
        pager.AddMember("pageCount", (int) mChildren.size(), allocator);
        pager.AddMember("allowForward", allowForward, allocator);
        pager.AddMember("allowBackwards", allowBackwards, allocator);
        outMap.AddMember("pager", pager, allocator);
        actionable = true;
    }

    return actionable;
}

void
PagerComponent::accept(Visitor<CoreComponent>& visitor) const {
    visitor.visit(*this);
    visitor.push();
    int currentPage = pagePosition();
    if (!visitor.isAborted() && currentPage >= 0 && currentPage < getChildCount()) {
        auto child = mChildren.at(currentPage);
        if (child != nullptr)
            child->accept(visitor);
    }
    visitor.pop();
}

void
PagerComponent::raccept(Visitor<CoreComponent>& visitor) const {
    visitor.visit(*this);
    visitor.push();
    int currentPage = pagePosition();
    if (!visitor.isAborted() && currentPage >= 0 && currentPage < getChildCount()) {
        auto child = mChildren.at(currentPage);
        if (child != nullptr)
            child->raccept(visitor);
    }
    visitor.pop();
}

bool
PagerComponent::insertChild(const ComponentPtr& child, size_t index, bool useDirtyFlag) {
    size_t initialSize = mChildren.size();
    bool result = CoreComponent::insertChild(child, index, useDirtyFlag);
    if (result) {
        int currentPage = getCalculated(kPropertyCurrentPage).asInt();
        if (currentPage >= index && index < initialSize) {
            mCalculated.set(kPropertyCurrentPage, currentPage + 1);
            setDirty(kPropertyCurrentPage);
        }
    }
    return result;
}

void
PagerComponent::removeChild(const CoreComponentPtr& child, size_t index, bool useDirtyFlag) {
    CoreComponent::removeChild(child, index, useDirtyFlag);
    int currentPage = getCalculated(kPropertyCurrentPage).asInt();
    if (currentPage >= index && currentPage != 0) {
        mCalculated.set(kPropertyCurrentPage, currentPage - 1);
        setDirty(kPropertyCurrentPage);
    }
}

void
PagerComponent::processLayoutChanges(bool useDirtyFlag) {
    attachCurrentAndReportLoaded();
    CoreComponent::processLayoutChanges(useDirtyFlag);
}

bool
PagerComponent::shouldAttachChildYogaNode(int index) const {
    auto navigation = static_cast<Navigation>(mCalculated.get(kPropertyNavigation).asInt());

    // Always attach for wrapping case as otherwise it will not actually be possible.
    // Do not do that for dynamic source though as if it's not fully loaded wrap is unclear.
    if (!mRebuilder && kNavigationWrap == navigation) {
        return true;
    }

    // Only attach initial page as any cache required will be attached later.
    int currentPage = getCalculated(kPropertyCurrentPage).asInt();
    return mEnsuredChildren.empty() && index == currentPage;
}

void
PagerComponent::finalizePopulate() {
    // Take this chance to set initial page as visible (and clip it if required).
    int initialPage = getCalculated(kPropertyInitialPage).asInt();
    initialPage = std::max(initialPage, 0);

    if (!mChildren.empty()) {
        initialPage = std::min(initialPage, static_cast<int>(mChildren.size()) - 1);
    } else {
        initialPage = 0;
    }

    mCalculated.set(kPropertyCurrentPage, initialPage);
    attachCurrentAndReportLoaded();

    auto navigation = static_cast<Navigation>(mCalculated.get(kPropertyNavigation).asInt());

    // Override wrap to normal if dynamic data.
    if (mRebuilder && navigation == kNavigationWrap) {
        mCalculated.set(kPropertyNavigation, kNavigationNormal);
    }

    if (getRootConfig().experimentalFeatureEnabled(RootConfig::kExperimentalFeatureHandleScrollingAndPagingInCore)) {
        auto currentChild = mChildren.at(initialPage);
        currentChild->setZOrder(1);
    }
}

bool
PagerComponent::attachCurrentAndReportLoaded() {
    if (mChildren.empty()) {
        return false;
    }

    int currentPage = getCalculated(kPropertyCurrentPage).asInt();
    int childCache = mContext->getRootConfig().getPagerChildCache();
    int lowerBound = currentPage - childCache;
    int upperBound = currentPage + childCache;
    size_t size = mChildren.size();
    bool needsLayoutCalculation = false;

    if (!mChildren.at(currentPage)->isAttached()) {
        ensureChildAttached(mChildren.at(currentPage), currentPage);
        needsLayoutCalculation = true;
    }
    if (lowerBound >= 0 && !mChildren.at(lowerBound)->isAttached()) {
        ensureChildAttached(mChildren.at(lowerBound), lowerBound);
        needsLayoutCalculation = true;
    }
    if (upperBound < size && !mChildren.at(upperBound)->isAttached()) {
        ensureChildAttached(mChildren.at(upperBound), upperBound);
        needsLayoutCalculation = true;
    }

    reportLoaded(currentPage);

    return needsLayoutCalculation;
}


} // namespace apl
