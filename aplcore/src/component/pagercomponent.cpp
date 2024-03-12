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

#include "apl/component/pagercomponent.h"

#include "apl/component/componentpropdef.h"
#include "apl/component/yogaproperties.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/layoutmanager.h"
#include "apl/focus/focusmanager.h"
#include "apl/livedata/layoutrebuilder.h"
#include "apl/livedata/livearrayobject.h"
#include "apl/primitives/accessibilityaction.h"
#include "apl/time/sequencer.h"
#include "apl/time/timemanager.h"
#include "apl/touch/gestures/pagerflinggesture.h"
#include "apl/touch/utils/pagemovehandler.h"

namespace apl {

static const bool DEBUG_PAGER = false;

CoreComponentPtr
PagerComponent::create(const ContextPtr& context,
                       Properties&& properties,
                       const Path& path) {
    auto ptr = std::make_shared<PagerComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

PagerComponent::PagerComponent(const ContextPtr& context,
                               Properties&& properties,
                               const Path& path)
        : ActionableComponent(context, std::move(properties), path)
{}

void
PagerComponent::releaseSelf()
{
    clearActiveStateSelf();
    ActionableComponent::releaseSelf();
}

void
PagerComponent::clearActiveStateSelf()
{
    ActionableComponent::clearActiveStateSelf();
    mPageMoveHandler = nullptr;
    mCurrentAnimation = nullptr;
    mDelayLayoutAction = nullptr;
}

void
PagerComponent::getSupportedStandardAccessibilityActions(std::map<std::string, bool>& result) const
{
    ActionableComponent::getSupportedStandardAccessibilityActions(result);
    if (getRootConfig().experimentalFeatureEnabled(RootConfig::kExperimentalFeatureDynamicAccessibilityActions)) {
        auto availableDirection = pageDirection();
        switch (availableDirection) {
            case kPageDirectionNone:
                break;
            case kPageDirectionForward:
                result.emplace(AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLFORWARD, true);
                break;
            case kPageDirectionBack:
                result.emplace(AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLBACKWARD, true);
                break;
            case kPageDirectionBoth:
                result.emplace(AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLFORWARD, true);
                result.emplace(AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLBACKWARD, true);
                break;
        }
    } else {
        result.emplace(AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLFORWARD, true);
        result.emplace(AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLBACKWARD, true);
    }
}

void
PagerComponent::invokeStandardAccessibilityAction(const std::string& name)
{
    bool pageChange = false;
    if (name == AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLFORWARD) {
        setPageImmediate(kPageDirectionForward);
        pageChange = true;
    } else if (name == AccessibilityAction::ACCESSIBILITY_ACTION_SCROLLBACKWARD) {
        setPageImmediate(kPageDirectionBack);
        pageChange = true;
    } else
        ActionableComponent::invokeStandardAccessibilityAction(name);

    if (pageChange &&
        getRootConfig().experimentalFeatureEnabled(RootConfig::kExperimentalFeatureDynamicAccessibilityActions)) {
        // If we have focus set to a CHILD of pager it should go to the next child, or to pager if
        // no such.
        auto focused = getContext()->focusManager().getFocus();
        auto self = shared_from_corecomponent();
        if (focused && focused != self && isParentOf(focused)) {
            auto next = getContext()->focusManager().find(kFocusDirectionForward, nullptr, Rect(), self);
            if (!next) next = self;

            getContext()->focusManager().setFocus(next, true, false);
        }
    }
}

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

    // Static Getter and Setter functions.  By defining these inside of PagerComponent::propDefSet, they have
    // access to protected and private members of PagerComponent.

    static auto getPageId = [](const CoreComponent& component) -> Object {
        auto currentPage = component.pagePosition();
        if (currentPage >= 0 && currentPage < component.getChildCount())
            return component.getChildAt(currentPage)->getId();

        return Object::NULL_OBJECT();
    };

    static auto setPageId = [](CoreComponent& component, const Object& value) -> void {
        if (value.empty())
            return;

        auto id = value.asString();
        for (size_t index = 0 ; index < component.getChildCount() ; index++) {
            auto child = component.getChildAt(index);
            if (child->getId() == id || child->getUniqueId() == id) {
                ((PagerComponent&)component).setPageImmediate((int)index);
                return;
            }
        }
    };

    static auto getPageIndex = [](const CoreComponent& component) -> Object {
        return component.pagePosition();
    };

    static auto setPageIndex = [](CoreComponent& component, const Object& value) -> void {
        ((PagerComponent&)component).setPageImmediate(value.asInt());
    };

    static ComponentPropDefSet sPagerComponentProperties(ActionableComponent::propDefSet(), {
        {kPropertyHeight,         Dimension(100),             asNonAutoDimension,  kPropIn, yn::setHeight, defaultHeight},
        {kPropertyWidth,          Dimension(100),             asNonAutoDimension,  kPropIn, yn::setWidth,  defaultWidth},
        {kPropertyInitialPage,    0,                          asInteger,           kPropIn },
        {kPropertyNavigation,     kNavigationWrap,            sNavigationMap,      kPropInOut | kPropDynamic | kPropVisualContext },
        {kPropertyPageDirection,  kScrollDirectionHorizontal, sScrollDirectionMap, kPropIn | kPropDynamic },
        {kPropertyHandlePageMove, Object::EMPTY_ARRAY(),      asArray,             kPropIn },
        {kPropertyOnPageChanged,  Object::EMPTY_ARRAY(),      asCommand,           kPropIn },
        {kPropertyCurrentPage,    0,                          asInteger,           kPropRuntimeState | kPropVisualContext | kPropVisibility | kPropAccessibility },
        {kPropertyPageId,         getPageId,                  setPageId,           kPropDynamic },
        {kPropertyPageIndex,      getPageIndex,               setPageIndex,        kPropDynamic },
    });

    return sPagerComponentProperties;
}

std::shared_ptr<PagerComponent>
PagerComponent::cast(const ComponentPtr& component) {
    return component && component->getType() == ComponentType::kComponentTypePager
               ? std::static_pointer_cast<PagerComponent>(component) : nullptr;
}

void
PagerComponent::initialize() {
    CoreComponent::initialize();

    // Set current page. It will be clipped, if required, later.
    int currentPage = getCalculated(kPropertyInitialPage).asInt();
    mCalculated.set(kPropertyCurrentPage, currentPage);

    // If native gestures enabled - register them.
    mGestureHandlers.emplace_back(PagerFlingGesture::create(ActionableComponent::cast(shared_from_this())));
}

void
PagerComponent::update(UpdateType type, float value) {
    if (type == kUpdatePagerPosition || type == kUpdatePagerByEvent) {
        // Update is used only in case if change of the page was performed by viewhost implementation (non-native gesture
        // processing). Animations is up to viewhost so we only update core internal state.
        int currentPage = pagePosition();
        if (static_cast<int>(value) != currentPage) {
            setPage(static_cast<int>(value));
            markDisplayedChildrenStale(true);
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
    setVisibilityDirty();
    attachPageAndReportLoaded(page);

    setDirty(kPropertyCurrentPage);
}

/**
 * Immediately change the current page in the pager.  This method can be invoked using "SetValue" on pageIndex or pageId.
 * @param pageIndex The index of the page to change to
 */
void
PagerComponent::setPageImmediate(int pageIndex)
{
    if (pageIndex < 0 || pageIndex >= mChildren.size() || pageIndex == pagePosition())
        return;

    mContext->sequencer().releaseResource({kExecutionResourcePosition, shared_from_this()});
    mCalculated.set(kPropertyCurrentPage, pageIndex);
    setVisualContextDirty();
    setVisibilityDirty();
    attachPageAndReportLoaded(pageIndex);
    setDirty(kPropertyCurrentPage);

    if (allowEventHandlers())
        executePageChangeEvent(true);

    markDisplayedChildrenStale(true);
}

void
PagerComponent::setPageUtil(
        const apl::ComponentPtr& target,
        int index,
        PageDirection direction,
        const ActionRef& ref,
        bool skipDefaultAnimation,
        apl_duration_t transitionDuration)
{
    auto pager = PagerComponent::cast(target);
    if (pager)
        pager->handleSetPage(index, direction, ref, skipDefaultAnimation, transitionDuration);
}

void
PagerComponent::setPageImmediate(PageDirection direction)
{
    auto index = direction == kPageDirectionForward ? pagePosition() + 1 : pagePosition() - 1;
    auto pages = static_cast<int>(getChildCount());
    int result = index % pages;
    setPageImmediate(result >= 0 ? result : result + pages);
}

void
PagerComponent::handleSetPage(int index, PageDirection direction, const ActionRef& ref, bool skipDefaultAnimation, apl_duration_t transitionDuration)
{
    auto currentPage = pagePosition();

    // Have to do that here for now in order to give viewhost possibility to load the next page if not there
    attachPageAndReportLoaded(index);

    // We have current animation in progress, do what it asks and finish it up preemptively.
    // Only ever can happen on <= 1.3 as resources will prevent it otherwise.
    if (mPageMoveHandler && mCurrentAnimation) {
        int targetPageIndex = mPageMoveHandler->getTargetPageIndex(shared_from_corecomponent());
        if (targetPageIndex >= 0) {
            setPage(targetPageIndex);
        }
        mCurrentAnimation->resolve();
        if (!ref.empty() && ref.isPending()) ref.resolve();
        return;
    }

    // Set initial state
    startPageMove(direction, currentPage, index);

    if (!mPageMoveHandler) {
        // Created in the previous step, should not happen
        assert(false);
        return;
    }

    // Animate if required.
    std::weak_ptr<PagerComponent> weak_ptr(std::static_pointer_cast<PagerComponent>(shared_from_this()));
    disableGestures();

    // We skip animation if asked to do so, on default handler and duration was not explicitly set
    if (mPageMoveHandler->isDefault() && skipDefaultAnimation && transitionDuration <= 0) {
        mCurrentAnimation = Action::makeDelayed(getRootConfig().getTimeManager(), 0);
    } else {
        if (transitionDuration < 0)
            transitionDuration = getRootConfig().getProperty(RootProperty::kDefaultPagerAnimationDuration).getDouble();
        mCurrentAnimation = Action::makeAnimation(getRootConfig().getTimeManager(),
          transitionDuration, [weak_ptr, transitionDuration](apl_duration_t offset){
              auto self = weak_ptr.lock();
              if (self) self->executePageMove(offset/transitionDuration);
          });
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

    mCurrentAnimation->addTerminateCallback([weak_ptr](const TimersPtr&) {
      auto self = weak_ptr.lock();
      if (self) {
          self->enableGestures();
      }
    });

    if (!ref.empty() && ref.isPending()) {
        ref.addTerminateCallback([weak_ptr](const TimersPtr&) {
            auto self = weak_ptr.lock();
            if (self) {
                if (self->mCurrentAnimation) {
                    self->mCurrentAnimation->terminate();
                    self->mCurrentAnimation = nullptr;
                }
                if (self->mPageMoveHandler) {
                    self->mPageMoveHandler->reset();
                    self->mPageMoveHandler = nullptr;
                }
            }
        });
    }
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
PagerComponent::startPageMove(PageDirection direction, int currentPage, int targetPage)
{
    auto swipeDirection = isHorizontal() ?
        (direction == kPageDirectionForward ? kSwipeDirectionLeft : kSwipeDirectionRight) :
        (direction == kPageDirectionForward ? kSwipeDirectionUp : kSwipeDirectionDown);

    if (isHorizontal() && getCalculated(kPropertyLayoutDirection) == kLayoutDirectionRTL) {
        // Switch direction for RTL layouts
        swipeDirection = (swipeDirection == kSwipeDirectionLeft ? kSwipeDirectionRight : kSwipeDirectionLeft);
    }

    auto handlerObject = getCalculated(kPropertyHandlePageMove);

    // Reset state of pages involved in any previous move, so that transforms are not left over
    if (mPageMoveHandler) {
       mPageMoveHandler->reset();
    }

    // We cache it here. It makes no sense to change handler while it's in animation .
    mPageMoveHandler = PageMoveHandler::create(shared_from_corecomponent(), handlerObject,
        swipeDirection, direction, currentPage, targetPage);
    markDisplayedChildrenStale(true);
}

void
PagerComponent::executePageMove(float amount)
{
    // If no handler exist - ignore it. Called out of order.
    if (mPageMoveHandler)
        mPageMoveHandler->execute(shared_from_corecomponent(), amount);
}

/*
 * Check if we have focus on the given page
 */
static bool
isOnPage(const CoreComponentPtr& pager, const CoreComponentPtr& child, const CoreComponentPtr& targetComponent) {
    auto comp = targetComponent;
    while (comp && comp != pager) {
        if (comp == child) return true;
        comp = CoreComponent::cast(comp->getParent());
    }
    return false;
}

void
PagerComponent::endPageMove(bool fulfilled, const ActionRef& ref, bool fast)
{
    // If no handler exist - ignore it. Called out of order.
    if (!mPageMoveHandler) return;

    if (fulfilled) {
        int targetPageIndex = mPageMoveHandler->getTargetPageIndex(shared_from_corecomponent());
        if (targetPageIndex >= 0) {
            // Ensure internal state for lazy loading.
            setPage(targetPageIndex);
        }

        // Check if we are focusing on something in the previous page
        if (auto currentPage = mPageMoveHandler->getCurrentPage().lock()) {
            auto& fm = mContext->focusManager();
            if (isOnPage(shared_from_corecomponent(), currentPage, fm.getFocus()))
                fm.setFocus(shared_from_corecomponent(), true);
        }

        auto event = executePageChangeEvent(fast);
        if (!ref.empty()) {
            if (event && event->isPending()) {
                event->then([ref](const ActionPtr& ptr) { ref.resolve(); });
            } else {
                ref.resolve();
            }
        }
    } else {
        if (!ref.empty()) {
            ref.resolve();
        }
    }

    markDisplayedChildrenStale(true);
    if (mPageMoveHandler) {
        mPageMoveHandler->reset();
        mPageMoveHandler = nullptr;
    }
    if (mCurrentAnimation) {
        mCurrentAnimation->terminate();
        mCurrentAnimation = nullptr;
    }
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
    return getCalculated(kPropertyPageDirection) == kScrollDirectionVertical ? kScrollTypeVerticalPager
                                                                             : kScrollTypeHorizontalPager;
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

bool
PagerComponent::allowForward() const
{
    auto allowedDirection = pageDirection();
    return allowedDirection == kPageDirectionBoth || allowedDirection == kPageDirectionForward;
}

bool
PagerComponent::allowBackwards() const
{
    auto allowedDirection = pageDirection();
    return allowedDirection == kPageDirectionBoth || allowedDirection == kPageDirectionBack;
}

void
PagerComponent::ensureDisplayedChildren() {

    if (mChildren.empty() || !mDisplayedChildrenStale)
        return;

    mDisplayedChildrenStale = false;

    // clear previous calculations
    mDisplayedChildren.clear();

    // current page is in the viewport
    CoreComponentPtr currentPage = nullptr;
    if (mPageMoveHandler) { 
        currentPage = mPageMoveHandler->getCheckedCurrentPage(shared_from_corecomponent());
    } else {
        currentPage = mChildren.at(pagePosition());
    }
    if (currentPage && currentPage->isDisplayable()) {
        mDisplayedChildren.emplace_back(currentPage);
    }

    // when in a page move transition, add the target next page
    if (mPageMoveHandler) {
        // current page
        auto nextPage = mPageMoveHandler->getCheckedTargetPage(shared_from_corecomponent());
        if (nextPage && nextPage->isDisplayable()) {

            // get the gesture direction by working backwards from swipe direction
            // see startPageMove() for inverse
            auto swipeDirection = mPageMoveHandler->getSwipeDirection();
            auto direction = isHorizontal()
                            ? (swipeDirection == kSwipeDirectionLeft? kPageDirectionForward : kPageDirectionBack)
                            : (swipeDirection == kSwipeDirectionUp? kPageDirectionForward : kPageDirectionBack);

            if (isHorizontal() && getCalculated(kPropertyLayoutDirection) == kLayoutDirectionRTL) {
                // swipe directions are flipped for RTL layout
                direction = (direction == kPageDirectionForward) ? kPageDirectionBack : kPageDirectionForward;
            }
            // compare user assigned draw order to direction to calculate
            // relative "ZOrder" of next page
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

            auto idx =  (nextAbove) ? mDisplayedChildren.end(): mDisplayedChildren.begin();
            mDisplayedChildren.emplace(idx, nextPage);
        }
    }
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
PagerComponent::insertChild(const CoreComponentPtr& child, size_t index, bool useDirtyFlag)
{
    size_t initialSize = mChildren.size();
    bool result = CoreComponent::insertChild(child, index, useDirtyFlag);
    if (result) {
        mContext->layoutManager().setAsTopNode(child);
        int currentPage = getCalculated(kPropertyCurrentPage).asInt();
        if (currentPage >= index && index < initialSize) {
            mCalculated.set(kPropertyCurrentPage, currentPage + 1);
            setDirty(kPropertyCurrentPage);
        }
    }
    return result;
}

void
PagerComponent::removeChildAfterMarkedRemoved(const CoreComponentPtr& child, size_t index, bool useDirtyFlag)
{
    CoreComponent::removeChildAfterMarkedRemoved(child, index, useDirtyFlag);
    mContext->layoutManager().removeAsTopNode(child);
    int currentPage = getCalculated(kPropertyCurrentPage).asInt();
    if (currentPage >= index && currentPage != 0) {
        mCalculated.set(kPropertyCurrentPage, currentPage - 1);
        setDirty(kPropertyCurrentPage);
    }
}

void
PagerComponent::reportLoadedInternal(size_t index)
{
    if (index == 0 && mRebuilder) mRebuilder->notifyStartEdgeReached();
    else if (index >= mChildren.size() - 1 && mRebuilder) mRebuilder->notifyEndEdgeReached();
    else reportLoaded(index);
}

void
PagerComponent::processLayoutChanges(bool useDirtyFlag, bool first)
{
    auto currentPage = getCalculated(kPropertyCurrentPage).asInt();
    if (first && !mChildren.empty()) {
        const auto& c = mChildren.at(currentPage);
        if (mRebuilder) {
            mRebuilder->inflateIfRequired(c);
        }
        mContext->layoutManager().requestLayout(c, false);
        reportLoadedInternal(currentPage);

        mDelayLayoutAction = Action::makeDelayed(getRootConfig().getTimeManager(), 1);
        auto weak_self = std::weak_ptr<PagerComponent>(
                std::static_pointer_cast<PagerComponent>(shared_from_corecomponent()));
        mDelayLayoutAction->then([weak_self](const ActionPtr &) {
            auto self = weak_self.lock();
            if (self) {
                self->processLayoutChanges(true, false);
                self->mDelayLayoutAction = nullptr;
            }
        });
    } else {
        attachPageAndReportLoaded(currentPage);
    }
    CoreComponent::processLayoutChanges(useDirtyFlag, first);
}

bool
PagerComponent::shouldAttachChildYogaNode(int index) const {
    return false;
}

void
PagerComponent::finalizePopulate()
{
    // Take this chance to set initial page as visible (and clip it if required).
    auto initialPage = std::max(0,
                                std::min(getCalculated(kPropertyInitialPage).getInteger(),
                                         static_cast<int>(mChildren.size()) - 1));
    mCalculated.set(kPropertyCurrentPage, initialPage);

    // One using it should be careful in cases when DDS is infinite - it may lead to excessive load requests.
    if (mContext->getRequestedAPLVersion().compare("1.7") < 0) {
        auto navigation = static_cast<Navigation>(mCalculated.get(kPropertyNavigation).asInt());

        // Override wrap to normal if populated by DynamicDatasource. LiveArray != DDS.
        if (mRebuilder && navigation == kNavigationWrap) {
            auto rebuilderArray = mRebuilder->getBackingArray();
            if (rebuilderArray && rebuilderArray->isPaginating()) {
                mCalculated.set(kPropertyNavigation, kNavigationNormal);
            }
        }
    }
    markAccessibilityDirty();
}

void
PagerComponent::attachPageAndReportLoaded(int page) {
    LOG_IF(DEBUG_PAGER).session(getContext()) << this->toDebugSimpleString();
    if (mChildren.empty() && mRebuilder) {
        // Force loading if possible
        mRebuilder->notifyStartEdgeReached();
        mRebuilder->notifyEndEdgeReached();
        return;
    }

    /**
     * Ensure that the requested page and some number of pages about it
     * the have been laid out.  This avoids stutters when switching pages
     * in case the next page needs to lay out complicated text blocks.
     */
    const auto childCount = static_cast<int>(mChildren.size());
    const auto pagerChildCache = mContext->getRootConfig().getProperty(RootProperty::kPagerChildCache).getInteger();
    const auto navigation = static_cast<Navigation>(getCalculated(kPropertyNavigation).getInteger());

    int start = 0;
    int count = 0;

    switch (navigation) {
        case kNavigationNormal:  // Allow forwards and backwards; bound to (0, childCount)
        case kNavigationNone:    // We don't know how it will behave, so use the "normal" page-cache algorithm
            start = std::max(0, page - pagerChildCache);
            count = std::min(page + pagerChildCache + 1, childCount) - start;
            break;
        case kNavigationWrap:  // Allow forwards and backwards; may wrap around
            if (pagerChildCache * 2 + 1 >= childCount) {  // All pages should be cached
                count = childCount;
            }
            else {  // Some pages won't be cached
                start = (page - pagerChildCache + childCount) % childCount;
                count = pagerChildCache * 2 + 1;
            }
            break;
        case kNavigationForwardOnly:  // Allow forward only; don't allow wrapping
            start = page;
            count = std::min(page + pagerChildCache + 1, childCount) - start;
            break;
    }

    LOG_IF(DEBUG_PAGER).session(getContext()) << "   start=" << start << " count=" << count;
    auto& lm = mContext->layoutManager();
    for (int i = 0 ; i < count ; i++) {
        auto index = (start + i) % childCount;
        const auto& c = mChildren.at(index);
        if (mRebuilder) {
            mRebuilder->inflateIfRequired(c);
        }
        lm.requestLayout(c, false);
        reportLoadedInternal(index);
    }
}

PageDirection
PagerComponent::focusDirectionToPage(FocusDirection direction)
{
    auto sType = scrollType();
    if (sType == kScrollTypeVerticalPager) {
        if (direction == kFocusDirectionDown) {
            return kPageDirectionForward;
        } else if (direction == kFocusDirectionUp) {
            return kPageDirectionBack;
        }
    } else {
        if (direction == kFocusDirectionRight) {
            return kPageDirectionForward;
        } else if (direction == kFocusDirectionLeft) {
            return kPageDirectionBack;
        }
    }

    if (direction == kFocusDirectionForward) return kPageDirectionForward;
    if (direction == kFocusDirectionBackwards) return kPageDirectionBack;

    return kPageDirectionNone;
}

bool
PagerComponent::canConsumeFocusDirectionEvent(FocusDirection direction, bool fromInside)
{
    if (!fromInside) return true;

    auto targetDirection = focusDirectionToPage(direction);
    if (targetDirection == kPageDirectionNone) return false;
    auto allowedPageDirection = pageDirection();
    return (allowedPageDirection == kPageDirectionBoth || targetDirection == allowedPageDirection);
}

CoreComponentPtr
PagerComponent::takeFocusFromChild(FocusDirection direction, const Rect& origin)
{
    auto targetDirection = focusDirectionToPage(direction);
    if (targetDirection != kPageDirectionNone) {
        const auto& bounds = getCalculated(kPropertyBounds).get<Rect>();
        Rect offsetRect = origin.empty() ? Rect(0, 0, bounds.getWidth(), bounds.getHeight()) : origin;
        switch(direction) {
            case kFocusDirectionLeft:
                offsetRect.offset(Point(bounds.getWidth(), 0));
                break;
            case kFocusDirectionRight:
                offsetRect.offset(Point(-bounds.getWidth(), 0));
                break;
            case kFocusDirectionUp:
                offsetRect.offset(Point(0, bounds.getHeight()));
                break;
            case kFocusDirectionDown:
                offsetRect.offset(Point(0, -bounds.getHeight()));
                break;
            case kFocusDirectionForward:
            case kFocusDirectionBackwards:
                break;
            default:
                return nullptr;
        }

        auto allowedPageDirection = pageDirection();
        if (allowedPageDirection == kPageDirectionBoth || targetDirection == allowedPageDirection) {
            const auto childCount = getChildCount();
            const auto delta = targetDirection == kPageDirectionForward ? 1 : -1;
            const auto targetPage = (int)((pagePosition() + delta + childCount) % childCount);
            // Reset any running commands that may affect page position.
            getContext()->sequencer().releaseResource({kExecutionResourcePosition, shared_from_this()});
            setPageUtil(shared_from_corecomponent(), targetPage, targetDirection, ActionRef(nullptr));
            // Need to have that to base search on what it will be.
            setPage(targetPage);
            auto pager = shared_from_corecomponent();
            auto next = getContext()->focusManager().find(direction, pager, offsetRect, pager);
            if (next) {
                return next;
            } else {
                return pager;
            }
        }
    }
    return nullptr;
}


} // namespace apl
