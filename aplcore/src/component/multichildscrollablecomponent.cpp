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
#include "apl/component/multichildscrollablecomponent.h"
#include "apl/component/yogaproperties.h"
#include "apl/content/rootconfig.h"

namespace apl {

inline Object
defaultSequenceWidth(Component& component, const RootConfig& rootConfig)
{
    auto scrollDirection = static_cast<ScrollDirection>(component.getCalculated(kPropertyScrollDirection).asInt());
    return rootConfig.getDefaultComponentWidth(component.getType(), scrollDirection == kScrollDirectionVertical);
}

inline Object
defaultSequenceHeight(Component& component, const RootConfig& rootConfig)
{
    auto scrollDirection = static_cast<ScrollDirection>(component.getCalculated(kPropertyScrollDirection).asInt());
    return rootConfig.getDefaultComponentHeight(component.getType(), scrollDirection == kScrollDirectionVertical);
}

const ComponentPropDefSet&
MultiChildScrollableComponent::propDefSet() const
{
    static ComponentPropDefSet sSequenceComponentProperties(ScrollableComponent::propDefSet(), {
            {kPropertyHeight,           Dimension(),              asDimension,          kPropIn,    yn::setHeight, defaultSequenceHeight},
            {kPropertyWidth,            Dimension(),              asDimension,          kPropIn,    yn::setWidth,  defaultSequenceWidth},
            {kPropertySnap,             kSnapNone,                sSnapMap,             kPropInOut | kPropStyled },
            {kPropertyNumbered,         false,                    asBoolean,            kPropIn | kPropVisualContext},
            {kPropertyOnScroll,         Object::EMPTY_ARRAY(),    asCommand,            kPropIn}
    });

    return sSequenceComponentProperties;
}
bool
MultiChildScrollableComponent::allowForward() const {
    if(getChildCount() == 0 || mEnsuredChildren.empty())
        return false;
    // If the last element has not had ensureLayout called on it,
    // then the viewhost still has room to scroll.
    if(mEnsuredChildren.upperBound() + 1 < getChildCount())
        return true;

    // otherwise get the last child and calculate the bounds of
    // all children
    auto lastChild = getChildAt(getChildCount() - 1);
    auto lastChildBounds = lastChild->getCalculated(kPropertyBounds).getRect();
    auto innerBounds = mCalculated.get(kPropertyInnerBounds).getRect();
    auto currentPosition = mCalculated.get(kPropertyScrollPosition).asNumber();
    auto vertical = isVertical();
    double scrollSize = vertical
                        ? innerBounds.getBottom()
                        : innerBounds.getRight();
    double innerScrollSize = vertical
                             ? lastChildBounds.getBottom()
                             : lastChildBounds.getRight();
    return currentPosition + scrollSize < innerScrollSize;
}

bool
MultiChildScrollableComponent::allowBackwards() const {
    auto currentPosition = mCalculated.get(kPropertyScrollPosition).asNumber();
    return (currentPosition > 0);
}

void
MultiChildScrollableComponent::update(UpdateType type, float value) {
    ScrollableComponent::update(type, value);

    if (type == kUpdateScrollPosition) {
        // Force figuring out what is on screen.
        processLayoutChanges(true);
    }
}

void
MultiChildScrollableComponent::ensureChildrenVisibilityUpdated()
{
    if(!mChildrenVisibilityStale)
        return;

    // We don't always go from parent to child here (update case) so calculate opacity and visible rect recursively.
    auto visibleIndexes = getChildrenVisibility(calculateRealOpacity(), calculateVisibleRect());
    if (!visibleIndexes.empty()) {
        mFirstChildInView = visibleIndexes.begin()->first;
        mLastChildInView = visibleIndexes.rbegin()->first;

        mIndexesSeen.expandTo(mFirstChildInView);
        mIndexesSeen.expandTo(mLastChildInView);

        auto firstFullyVisibleItr = std::find_if(visibleIndexes.begin(), visibleIndexes.end(),
                                                 [](const std::pair<int, float> &item) { return item.second == 1.0; });
        mFirstChildFullyInView = firstFullyVisibleItr != visibleIndexes.end() ? firstFullyVisibleItr->first : -1;

        auto lastFullyVisibleItr = std::find_if(visibleIndexes.rbegin(), visibleIndexes.rend(),
                                                [](const std::pair<int, float> &item) { return item.second == 1.0; });
        mLastChildFullyInView = lastFullyVisibleItr != visibleIndexes.rend() ? lastFullyVisibleItr->first : -1;
    } else {
        // reset all properties
        mFirstChildInView = -1;
        mFirstChildFullyInView = -1;
        mLastChildFullyInView = -1;
        mLastChildInView = -1;
    }

    // reset property changed flag to prevent recalculation
    mChildrenVisibilityStale = false;
}

void
MultiChildScrollableComponent::handlePropertyChange(const ComponentPropDef& def, const Object& value)
{
    CoreComponent::handlePropertyChange(def, value);

    if (def.key == kPropertyOpacity || def.key == kPropertyDisplay) {
        mChildrenVisibilityStale = true;
    }
}

void
MultiChildScrollableComponent::accept(Visitor<CoreComponent>& visitor) const
{
    visitor.visit(*this);
    visitor.push();
    if (!mEnsuredChildren.empty()) {
        for (int i = mEnsuredChildren.lowerBound();
             i <= mEnsuredChildren.upperBound() && !visitor.isAborted(); i++) {
            auto child = std::dynamic_pointer_cast<CoreComponent>(mChildren.at(i));
            if (child != nullptr && child->isAttached() &&
                !child->getCalculated(kPropertyBounds).getRect().isEmpty())
                child->accept(visitor);
        }
    }
    visitor.pop();
}

void
MultiChildScrollableComponent::raccept(Visitor<CoreComponent>& visitor) const
{
    visitor.visit(*this);
    visitor.push();
    if (!mEnsuredChildren.empty()) {
        for (int i = mEnsuredChildren.upperBound();
             i >= mEnsuredChildren.lowerBound() && !visitor.isAborted(); i--) {
            auto child = std::dynamic_pointer_cast<CoreComponent>(mChildren.at(i));
            if (child != nullptr && child->isAttached() &&
                !child->getCalculated(kPropertyBounds).getRect().isEmpty())
                child->raccept(visitor);
        }
    }
    visitor.pop();
}

ComponentPtr
MultiChildScrollableComponent::findDirectChildAtPosition(const Point& position) const
{
    if (mEnsuredChildren.empty())
        return nullptr;

    for (int i = mEnsuredChildren.upperBound(); i >= mEnsuredChildren.lowerBound(); i--) {
        auto child = mChildren.at(i);
        auto bounds = child->getCalculated(kPropertyBounds).getRect();
        if (bounds.contains(position))
            return child;
    }

    return nullptr;
}

std::map<int, float>
MultiChildScrollableComponent::getChildrenVisibility(float realOpacity, const Rect &visibleRect) const {
    std::map<int, float> visibleIndexes;
    bool visibleMet = false;

    if (mEnsuredChildren.empty())
        return visibleIndexes;

    for (int index = mEnsuredChildren.lowerBound(); index <= mEnsuredChildren.upperBound(); index++) {
        const auto& child = getCoreChildAt(index);
        if (!child->inParentViewport()) {
            // Check if we have element outside of sequence viewport. If so - break out the loop.
            if (visibleMet) {
                break;
            } else {
                continue;
            }
        }

        visibleMet = true;

        float childVisibility = child->calculateVisibility(realOpacity, visibleRect);
        if(childVisibility > 0.0) {
            visibleIndexes.emplace(index, childVisibility);
        }
    }

    return visibleIndexes;
}

const EventPropertyMap&
MultiChildScrollableComponent::eventPropertyMap() const
{
    static EventPropertyMap sMultiScrollEventProperties = eventPropertyMerge(
        ScrollableComponent::eventPropertyMap(),
        {
            {"firstVisibleChild", [](const CoreComponent* c) { return static_cast<const MultiChildScrollableComponent*>(c)->getFirstChildInView(); }},
            {"firstFullyVisibleChild", [](const CoreComponent* c) { return static_cast<const MultiChildScrollableComponent*>(c)->getFirstChildFullyInView(); }},
            {"lastFullyVisibleChild", [](const CoreComponent* c) { return static_cast<const MultiChildScrollableComponent*>(c)->getLastChildFullyInView(); }},
            {"lastVisibleChild", [](const CoreComponent* c) { return static_cast<const MultiChildScrollableComponent*>(c)->getLastChildInView(); }},
        });

    return sMultiScrollEventProperties;
}

Object
MultiChildScrollableComponent::getValue() const {
    double scrollSize = isVertical()
                        ? YGNodeLayoutGetHeight(mYGNodeRef)
                        : YGNodeLayoutGetWidth(mYGNodeRef);
    auto currentPosition = mCalculated.get(kPropertyScrollPosition).asNumber();
    return scrollSize != 0 ? currentPosition / scrollSize : 0;
}

Point
MultiChildScrollableComponent::scrollPosition() const {
    auto currentPosition = mCalculated.get(kPropertyScrollPosition).asNumber();
    return isVertical() ?
           Point(0, currentPosition) :
           Point(currentPosition, 0);
}

Point
MultiChildScrollableComponent::trimScroll(const Point& point) const
{
    // Break out early. If there are no children - no scrolling possible
    if (mChildren.empty())
        return Point();

    auto innerBounds = mCalculated.get(kPropertyInnerBounds).getRect();
    // We treat this component as 0 point of sequence. All calculation happens in relation to it.
    auto zeroAnchor = mChildren.at(mEnsuredChildren.empty() ? 0 : mEnsuredChildren.lowerBound());
    if (!zeroAnchor->isAttached()) zeroAnchor->ensureLayout(true);
    auto zeroAnchorPos = zeroAnchor->getCalculated(kPropertyBounds).getRect().getTopLeft() - innerBounds.getTopLeft();

    if (isVertical()) {
        auto y = point.getY();
        // Cover in back direction. Current code ensures from index 0 to requested component and any updates
        // asynchronous so no way it will happen twice.
        while (y < zeroAnchorPos.getY() && !mEnsuredChildren.empty() && mEnsuredChildren.lowerBound() > 0) {
            mChildren.at(mEnsuredChildren.lowerBound() - 1)->ensureLayout(true);
            zeroAnchorPos = zeroAnchor->getCalculated(kPropertyBounds).getRect().getTopLeft() - innerBounds.getTopLeft();
        }

        y += zeroAnchorPos.getY();

        if (y <= 0)
            return Point();

        float bottom = innerBounds.getBottom();
        float maxY = 0;

        // Ensure children until they cover the sequence.
        int startingChild = std::max(mEnsuredChildren.upperBound(), 0);
        for (int i = startingChild ; i<mChildren.size() ; i++) {
            const auto& child = mChildren.at(i);
            child->ensureLayout(false);
            maxY = nonNegative(child->getCalculated(kPropertyBounds).getRect().getBottom() - bottom);
            if (y <= maxY)
                return Point(0,y);
        }

        return Point(0, maxY);
    }
    else {  // Horizontal
        auto x = point.getX();
        while (x < zeroAnchorPos.getX() && !mEnsuredChildren.empty() && mEnsuredChildren.lowerBound() > 0) {
            mChildren.at(mEnsuredChildren.lowerBound() - 1)->ensureLayout(true);
            zeroAnchorPos = zeroAnchor->getCalculated(kPropertyBounds).getRect().getTopLeft() - innerBounds.getTopLeft();
        }

        x += zeroAnchorPos.getX();

        if (x <= 0)
            return Point();

        float right = innerBounds.getRight();
        float maxX = 0;

        // Ensure children until they cover the sequence.
        int startingChild = std::max(mEnsuredChildren.upperBound(), 0);
        for (int i = startingChild ; i<mChildren.size(); i++) {
            const auto& child = mChildren.at(i);
            child->ensureLayout(true);
            maxX = nonNegative(child->getCalculated(kPropertyBounds).getRect().getRight() - right);
            if (x <= maxX)
                return Point(x,0);
        }

        return Point(maxX, 0);
    }
}

bool
MultiChildScrollableComponent::getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator) {
    bool actionable = ScrollableComponent::getTags(outMap, allocator);
    if(!mChildren.empty()) {
        rapidjson::Value list(rapidjson::kObjectType);
        list.AddMember("itemCount", static_cast<int>(mChildren.size()), allocator);

        ensureChildrenVisibilityUpdated();

        if (!mIndexesSeen.empty()) {
            auto lowestOrdinalSeen = INT_MAX;
            auto highestOrdinalSeen = -1;

            for(int i = mIndexesSeen.lowerBound(); i<= mIndexesSeen.upperBound(); i++) {
                auto ordinal = mChildren.at(i)->getContext()->opt("ordinal");
                if(ordinal.isNull())
                    continue;

                highestOrdinalSeen = std::max(ordinal.asInt(), highestOrdinalSeen);
                lowestOrdinalSeen = std::min(ordinal.asInt(), lowestOrdinalSeen);
            }

            list.AddMember("lowestIndexSeen", mIndexesSeen.lowerBound(), allocator);
            list.AddMember("highestIndexSeen", mIndexesSeen.upperBound(), allocator);

            if (getCalculated(kPropertyNumbered).truthy() && lowestOrdinalSeen <= highestOrdinalSeen) {
                list.AddMember("lowestOrdinalSeen", lowestOrdinalSeen, allocator);
                list.AddMember("highestOrdinalSeen", highestOrdinalSeen, allocator);
            }
        } else {
            list.AddMember("lowestIndexSeen", 0, allocator);
            list.AddMember("highestIndexSeen", 0, allocator);
        }

        outMap.AddMember("list", list, allocator);
    }
    return actionable;
}

void
MultiChildScrollableComponent::layoutChildIfRequired(
        CoreComponentPtr& child, size_t childIdx, bool useDirtyFlag) {
    if (!child->isAttached() || child->getCalculated(kPropertyBounds).empty()) {
        ensureChildAttached(child, childIdx);
        if (childIdx > 0 && childrenUseSpacingProperty()) {
            child->fixSpacing();
        }

        // First calculate the layout for this component (if dirty)
        auto bounds = mCalculated.get(kPropertyBounds).getRect();
        YGNodeCalculateLayout(
                mYGNodeRef,
                bounds.getWidth(),
                bounds.getHeight(),
                YGDirection::YGDirectionLTR);

        // Then re-layout from the root if necessary. We don't expect any of components higher in
        // hierarchy to change when we attach to multichild scrollable. We need to call to root
        // here to avoid any problems with propagation of positions for "lazy" components in the
        // chain. Assumed to be optimal from yoga side. It could happen only after initial layout
        // so we have parent bounds.
        auto root = getRootComponent();
        if (root && root != shared_from_corecomponent()) {
            auto rootBounds = root->getCalculated(kPropertyBounds).getRect();
            YGNodeCalculateLayout(root->getNode(), rootBounds.getWidth(), rootBounds.getHeight(),
                                  YGDirection::YGDirectionLTR);
        }

        CoreComponent::processLayoutChanges(useDirtyFlag);
    }
}

float
MultiChildScrollableComponent::maxScroll() const {
    if (mChildren.empty()) {
        return 0;
    }
    bool horizontal = isHorizontal();
    auto sequenceBounds = mCalculated.get(kPropertyInnerBounds).getRect();
    float end = horizontal ? sequenceBounds.getRight() : sequenceBounds.getBottom();
    auto lastChildBounds = mChildren.at(mEnsuredChildren.upperBound())->getCalculated(kPropertyBounds).getRect();
    float childEnd = horizontal ? lastChildBounds.getRight() : lastChildBounds.getBottom();
    return nonNegative(childEnd - end);
}

bool
MultiChildScrollableComponent::shouldAttachChildYogaNode(int index) const {
    // We can't predict the order in which new indexes attached. So just attach first one initially to have anchor
    // in place.
    return (mEnsuredChildren.empty() && index == 0);
}

bool
MultiChildScrollableComponent::insertChild(const ComponentPtr& child, size_t index, bool useDirtyFlag)
{
    bool result = CoreComponent::insertChild(child, index, useDirtyFlag);

    if (result && !mIndexesSeen.empty() && (int)index <= mIndexesSeen.lowerBound()) {
        mIndexesSeen.shift(1);
    }
    // updating flag to trigger recalculate visibility update
    mChildrenVisibilityStale = true;

    return result;
}

void
MultiChildScrollableComponent::removeChild(const CoreComponentPtr& child, size_t index, bool useDirtyFlag)
{
    CoreComponent::removeChild(child, index, useDirtyFlag);

    if (!mIndexesSeen.empty()) {
        if ((int)index < mIndexesSeen.lowerBound()) {
            mIndexesSeen.shift(-1);
        } else if (mIndexesSeen.contains(index)) {
            mIndexesSeen.remove(index);
        }
    }
    // updating flag to trigger recalculate visibility update
    mChildrenVisibilityStale = true;
}

void
MultiChildScrollableComponent::processLayoutChanges(bool useDirtyFlag) {
    // We need to account for padding as find function don't do that automatically.
    bool horizontal = isHorizontal();
    auto paddedScrollPosition = scrollPosition() + getCalculated(kPropertyInnerBounds).getRect().getTopLeft();
    auto anchor = std::static_pointer_cast<CoreComponent>(findDirectChildAtPosition(paddedScrollPosition));

    Rect oldAnchorBounds;
    if (anchor) {
        oldAnchorBounds = anchor->getCalculated(kPropertyBounds).getRect();
    }

    CoreComponent::processLayoutChanges(useDirtyFlag);

    if (mChildren.empty()) {
        // Starting with empty sequence
        return;
    }

    // We have not laid-out anything before. Refer to first available attached child. Possible only on initial layout.
    if (!anchor) {
        anchor = mChildren.at(mEnsuredChildren.lowerBound());
        oldAnchorBounds = anchor->getCalculated(kPropertyBounds).getRect();
    }

    if (!mChildren.empty() && childrenUseSpacingProperty()) {
        // Reset very first child to not have spacing
        mChildren.at(0)->fixSpacing(true);

        // First ensured should not ignore spacing if it's not very first one overall
        auto child = mChildren.at(mEnsuredChildren.lowerBound());
        if (mEnsuredChildren.lowerBound() == 0) {
            // Reset very first child to not have spacing
            child->fixSpacing(true);
        } else {
            child->fixSpacing();
        }
    }

    auto sequenceBounds = mCalculated.get(kPropertyBounds).getRect();
    float childCache = mContext->getRootConfig().getSequenceChildCache();
    float pageSize = horizontal ? sequenceBounds.getWidth() : sequenceBounds.getHeight();
    float anchorPosition = horizontal ? oldAnchorBounds.getX() : oldAnchorBounds.getY();

    // Lay out children in positive direction until we hit cache limit.
    float distanceToCover = (childCache + 1) * pageSize + anchorPosition;
    int lastLoaded = mEnsuredChildren.lowerBound();
    for (; lastLoaded < mChildren.size(); lastLoaded++) {
        auto child = mChildren.at(lastLoaded);
        layoutChildIfRequired(child, lastLoaded, useDirtyFlag);
        auto childBounds = child->getCalculated(kPropertyBounds).getRect();
        float distance = horizontal ? childBounds.getRight() : childBounds.getBottom();
        if (distance > distanceToCover) {
            break;
        }
    }

    reportLoaded(std::min(lastLoaded, mEnsuredChildren.upperBound()));

    Rect anchorBounds;
    // Lay out children in negative direction until we hit cache limit.
    distanceToCover = childCache * pageSize;
    int firstLoaded = mEnsuredChildren.upperBound();
    for (; firstLoaded >= 0; firstLoaded--) {
        auto child = mChildren.at(firstLoaded);
        layoutChildIfRequired(child, firstLoaded, useDirtyFlag);
        auto childBounds = child->getCalculated(kPropertyBounds).getRect();
        anchorBounds = anchor->getCalculated(kPropertyBounds).getRect();
        float distance = (horizontal ? anchorBounds.getLeft() : anchorBounds.getTop())
                       - (horizontal ? childBounds.getLeft() : childBounds.getTop());
        if (distance > distanceToCover) {
            break;
        }
    }

    reportLoaded(std::max(firstLoaded, mEnsuredChildren.lowerBound()));

    if (anchor) {
        // Adjust scroll position if changed at the beginning of sequence
        if (anchorBounds != oldAnchorBounds) {
            auto currentPosition = getCalculated(kPropertyScrollPosition).asNumber();
            currentPosition += horizontal ? anchorBounds.getLeft() - oldAnchorBounds.getLeft()
                                          : anchorBounds.getTop() - oldAnchorBounds.getTop();
            mCalculated.set(kPropertyScrollPosition, Dimension(DimensionType::Absolute, currentPosition));
            setDirty(kPropertyScrollPosition);
        }
    }

    ensureChildrenVisibilityUpdated();
}

void
MultiChildScrollableComponent::onScrollPositionUpdated() {
    ScrollableComponent::onScrollPositionUpdated();

    for (int i = mEnsuredChildren.lowerBound(); i <= mEnsuredChildren.upperBound(); i++) {
        auto child = std::dynamic_pointer_cast<CoreComponent>(mChildren.at(i));
        if (child != nullptr && child->isAttached()) {
            child->markGlobalToLocalTransformStale();
        }
    }

    mChildrenVisibilityStale = true;
}

float
MultiChildScrollableComponent::getSnapOffsetForChild(
    const ComponentPtr& child,
    Snap snap,
    float parentStart,
    float parentEnd) const
{
    float scrollTo = 0;
    float childStart, childEnd, currentPosition;
    auto childBoundsInParent = child->getCalculated(kPropertyBounds).getRect();

    if (isVertical()) {
        childStart = childBoundsInParent.getTop();
        childEnd = childBoundsInParent.getBottom();
        currentPosition = scrollPosition().getY();
    } else {
        childStart = childBoundsInParent.getLeft();
        childEnd = childBoundsInParent.getRight();
        currentPosition = scrollPosition().getX();
    }

    switch (snap) {
        case kSnapStart:
        case kSnapForceStart:
            scrollTo = childStart;
            break;

        case kSnapCenter:
        case kSnapForceCenter:
            scrollTo = ((childStart + childEnd) - (parentStart + parentEnd)) / 2;
            break;

        case kSnapEnd:
        case kSnapForceEnd:
            scrollTo = childEnd - parentEnd;
            break;

        default:
            break;
    }

    return scrollTo - currentPosition;
}

float
MultiChildScrollableComponent::childFractionOnScreenWithProposedScrollOffset(const ComponentPtr& child, float scrollOffset) const
{
    const auto vertical = isVertical();
    auto childBounds = child->getCalculated(kPropertyBounds).getRect();
    auto parentBounds = getCalculated(kPropertyInnerBounds).getRect();
    parentBounds.offset(vertical ? Point(0, scrollOffset) : Point(scrollOffset, 0));

    auto intersectionRect = parentBounds.intersect(childBounds);
    if (vertical) {
        return intersectionRect.getHeight() / std::min(parentBounds.getHeight(), childBounds.getHeight());
    } else {
        return intersectionRect.getWidth() / std::min(parentBounds.getWidth(), childBounds.getWidth());
    }
}

bool
MultiChildScrollableComponent::shouldForceSnap() const {
    auto snap = static_cast<Snap>(getCalculated(kPropertySnap).getInteger());
    return snap == kSnapForceStart || snap == kSnapForceCenter || snap == kSnapForceEnd;
}

Point
MultiChildScrollableComponent::getSnapOffset() const
{
    auto snap = static_cast<Snap>(getCalculated(kPropertySnap).getInteger());
    if (snap == kSnapNone) return {};

    // It's unidirectional on purpose, snap on multidirectional scrolling is quite unclear
    const auto vertical = isVertical();
    Rect parentInnerBounds = getCalculated(kPropertyInnerBounds).getRect();
    auto lastChildBottom =
        mChildren.at(mEnsuredChildren.upperBound())->getCalculated(kPropertyBounds).getRect().getBottomRight();

    // Figure to which position we should snap
    float parentStart, parentEnd, parentSize, forwardScrollLimit, scrollOffset;
    if (vertical) {
        parentStart = parentInnerBounds.getTop();
        parentEnd = parentInnerBounds.getBottom();
        parentSize = parentEnd - parentStart;
        forwardScrollLimit = lastChildBottom.getY() - parentSize;
        scrollOffset = scrollPosition().getY();
    } else {
        parentStart = parentInnerBounds.getLeft();
        parentEnd = parentInnerBounds.getRight();
        parentSize = parentEnd - parentStart;
        forwardScrollLimit = lastChildBottom.getX() - parentSize;
        scrollOffset =  scrollPosition().getX();
    }

    // If we currently at a limit - do not snap, we already snapped to limit. Unless forced to.
    if (!shouldForceSnap() && (scrollOffset <= 0 || scrollOffset >= forwardScrollLimit)) return {};

    float parentSnapOffset = 0;
    switch (snap) {
        case kSnapCenter:
        case kSnapForceCenter:
            parentSnapOffset = parentSize / 2;
            break;
        case kSnapEnd:
        case kSnapForceEnd:
            parentSnapOffset = parentSize;
            break;
        default:
            break;
    }

    Point referencePoint;
    if (vertical) {
        referencePoint = Point(parentInnerBounds.getX(), scrollOffset + parentSnapOffset);
    } else {
        referencePoint = Point(scrollOffset + parentSnapOffset, parentInnerBounds.getY());
    }

    auto targetChild = findDirectChildAtPosition(referencePoint);
    if (!targetChild) return {};

    auto it = std::find(mChildren.begin(), mChildren.end(), targetChild);
    size_t targetIndex = std::distance(mChildren.begin(), it);

    auto itemsPerCourse = getItemsPerCourse();

    switch (snap) {
        case kSnapStart:
        case kSnapForceStart:
            // If < 50% visible -> snap to next.
            if (childFractionOnScreenWithProposedScrollOffset(targetChild, scrollOffset) < 0.5) {
                if (targetIndex + itemsPerCourse < mChildren.size()) {
                    targetIndex += itemsPerCourse;
                    targetChild = getChildAt(targetIndex);
                }
            }
            break;
        case kSnapEnd:
        case kSnapForceEnd:
            // If < 50% visible -> snap to previous.
            if (childFractionOnScreenWithProposedScrollOffset(targetChild, scrollOffset) < 0.5) {
                if (targetIndex - itemsPerCourse >= 0) {
                    targetIndex -= itemsPerCourse;
                    targetChild = getChildAt(targetIndex);
                }
            }
            break;
        case kSnapCenter:
        case kSnapForceCenter:
            // Just snap to component we hit.
            break;
        default:
            break;
    }

    if (!targetChild) {
        LOG(LogLevel::WARN) << "Can't snap on scroll offset " << scrollOffset;
        return {};
    }

    auto offset = getSnapOffsetForChild(targetChild, snap, parentStart, parentEnd);

    // Check if adjustment will overshoot us over scrolling limits. If so - clip to scroll limit.
    if (scrollOffset + offset <= 0) {
        offset = -scrollOffset;
    }

    if (scrollOffset + offset >= forwardScrollLimit) {
        offset = forwardScrollLimit - scrollOffset;
    }

    return (vertical ? Point(0, offset) : Point(offset, 0));
}

} // namespace apl
