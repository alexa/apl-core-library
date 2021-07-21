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

#include "apl/component/componentpropdef.h"
#include "apl/component/pagercomponent.h"
#include "apl/component/yogaproperties.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/layoutmanager.h"
#include "apl/focus/focusmanager.h"
#include "apl/livedata/layoutrebuilder.h"
#include "apl/livedata/livearrayobject.h"
#include "apl/primitives/keyboard.h"
#include "apl/time/sequencer.h"

#include "apl/touch/gestures/pagerflinggesture.h"
#include "apl/touch/utils/pagemovehandler.h"
#include "apl/time/timemanager.h"

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
PagerComponent::release()
{
    mCurrentAnimation = nullptr;
    ActionableComponent::release();
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
                dynamic_cast<PagerComponent&>(component).setPageImmediate(index);
                return;
            }
        }
    };

    static auto getPageIndex = [](const CoreComponent& component) -> Object {
        return component.pagePosition();
    };

    static auto setPageIndex = [](CoreComponent& component, const Object& value) -> void {
        dynamic_cast<PagerComponent&>(component).setPageImmediate(value.asInt());
    };

    static ComponentPropDefSet sPagerComponentProperties(ActionableComponent::propDefSet(), {
        {kPropertyHeight,         Dimension(100),             asNonAutoDimension,  kPropIn, yn::setHeight, defaultHeight},
        {kPropertyWidth,          Dimension(100),             asNonAutoDimension,  kPropIn, yn::setWidth,  defaultWidth},
        {kPropertyInitialPage,    0,                          asInteger,           kPropIn},
        {kPropertyNavigation,     kNavigationWrap,            sNavigationMap,      kPropInOut | kPropDynamic | kPropVisualContext},
        {kPropertyPageDirection,  kScrollDirectionHorizontal, sScrollDirectionMap, kPropIn | kPropDynamic},
        {kPropertyHandlePageMove, Object::EMPTY_ARRAY(),      asArray,             kPropIn},
        {kPropertyOnPageChanged,  Object::EMPTY_ARRAY(),      asCommand,           kPropIn},
        {kPropertyCurrentPage,    0,                          asInteger,           kPropRuntimeState | kPropVisualContext},
        {kPropertyPageId,         getPageId,                  setPageId,           kPropDynamic },
        {kPropertyPageIndex,      getPageIndex,               setPageIndex,        kPropDynamic },
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
    mGestureHandlers.emplace_back(PagerFlingGesture::create(std::static_pointer_cast<ActionableComponent>(shared_from_this())));
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
    attachPageAndReportLoaded(page);

    setDirty(kPropertyCurrentPage);
}

/**
 * Immediately change the current page in the pager.  This method can be invoked using "SetValue" on pageIndex or pageId.
 * @param page The index of the page to change to
 */
void
PagerComponent::setPageImmediate(int pageIndex)
{
    if (pageIndex < 0 || pageIndex >= mChildren.size() || pageIndex == pagePosition())
        return;

    mContext->sequencer().releaseResource({kExecutionResourcePosition, shared_from_this()});
    mCalculated.set(kPropertyCurrentPage, pageIndex);
    setVisualContextDirty();
    attachPageAndReportLoaded(pageIndex);
    setDirty(kPropertyCurrentPage);

    if (allowEventHandlers())
        executePageChangeEvent(true);
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
   std::dynamic_pointer_cast<PagerComponent>(target)->handleSetPage(index, direction, ref, skipDefaultAnimation);
}

void
PagerComponent::handleSetPage(int index, PageDirection direction, const ActionRef& ref, bool skipDefaultAnimation)
{
    auto currentPage = pagePosition();

    // Have to do that here for now in order to give viewhost possibility to load the next page if not there
    attachPageAndReportLoaded(index);

    // We have current animation in progress, do what it asks and finish it up preemptively.
    // Only ever can happen on <= 1.3 as resources will prevent it otherwise.
    if (mPageMoveHandler && mCurrentAnimation) {
        setPage(mPageMoveHandler->getTargetPage());
        mCurrentAnimation->resolve();
        if (!ref.isEmpty() && ref.isPending()) ref.resolve();
        return;
    }

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

    if (!ref.isEmpty() && ref.isPending()) {
        ref.addTerminateCallback([weak_ptr](const TimersPtr&) {
            auto self = weak_ptr.lock();
            if (self) {
                if (self->mCurrentAnimation) {
                    self->mCurrentAnimation->terminate();
                    self->mCurrentAnimation = nullptr;
                }
                if (self->mPageMoveHandler) {
                    self->mPageMoveHandler->reset(*self);
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

void
PagerComponent::endPageMove(bool fulfilled, const ActionRef& ref, bool fast)
{
    // If no handler exist - ignore it. Called out of order.
    if (!mPageMoveHandler) return;

    auto targetPage = mPageMoveHandler->getTargetPage();

    if (fulfilled) {
        // Ensure internal state for lazy loading.
        setPage(targetPage);

        auto event = executePageChangeEvent(fast);
        if (!ref.isEmpty()) {
            if (event && event->isPending()) {
                event->then([ref](const ActionPtr& ptr) { ref.resolve(); });
            } else {
                ref.resolve();
            }
        }
    } else {
        if (!ref.isEmpty()) {
            ref.resolve();
        }
    }

    markDisplayedChildrenStale(true);
    if (mPageMoveHandler) {
        mPageMoveHandler->reset(*this);
        mPageMoveHandler = nullptr;
    }
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
    auto currentPageIndex = mPageMoveHandler ? mPageMoveHandler->getCurrentPage() : pagePosition();
    auto currentPage = mChildren.at(currentPageIndex);
    if (currentPage->isDisplayable()) {
        mDisplayedChildren.emplace_back(currentPage);
    }

    // when in a page move transition, add the target next page
    if (mPageMoveHandler) {
        // current page
        auto nextPage = mChildren.at(mPageMoveHandler->getTargetPage());
        if (nextPage->isDisplayable()) {

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
PagerComponent::removeChild(const CoreComponentPtr& child, size_t index, bool useDirtyFlag)
{
    CoreComponent::removeChild(child, index, useDirtyFlag);
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
}

void
PagerComponent::attachPageAndReportLoaded(int page) {
    LOG_IF(DEBUG_PAGER) << this->toDebugSimpleString();
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
    const auto pagerChildCache = mContext->getRootConfig().getPagerChildCache();
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

    LOG_IF(DEBUG_PAGER) << "   start=" << start << " count=" << count;
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
        auto bounds = getCalculated(kPropertyBounds).getRect();
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
            const int childCount = getChildCount();
            const auto delta = targetDirection == kPageDirectionForward ? 1 : -1;
            const auto targetPage = (pagePosition() + delta + childCount) % childCount;
            // Reset any running commands that may affect page position.
            getContext()->sequencer().releaseResource({kExecutionResourcePosition, shared_from_this()});
            setPageUtil(getContext(), shared_from_corecomponent(), targetPage, targetDirection, ActionRef(nullptr));
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
