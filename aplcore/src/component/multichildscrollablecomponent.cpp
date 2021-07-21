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

#include "apl/apl.h"
#include "apl/component/componentpropdef.h"
#include "apl/component/multichildscrollablecomponent.h"
#include "apl/component/yogaproperties.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/layoutmanager.h"
#include "apl/livedata/layoutrebuilder.h"
#include "apl/time/sequencer.h"
#include "apl/time/timemanager.h"
#include "apl/utils/tracing.h"

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

/**
 * The following templated functions are used to save and restore the scroll position during reinflation.
 */
using VisChildResult = std::pair<CoreComponentPtr, float>;

template<typename TOwner, VisChildResult(TOwner::*func)() const>
Object getScrollAlignId(const CoreComponent& component)
{
    const auto& m = dynamic_cast<const TOwner&>(component);
    auto p = (m.*func)();
    return ObjectArray{p.first ? p.first->getId() : "", p.second};
}

template<typename TOwner, VisChildResult(TOwner::*func)() const>
Object getScrollAlignIndex(const CoreComponent& component)
{
    const auto& m = dynamic_cast<const TOwner&>(component);
    auto p = (m.*func)();
    return ObjectArray{component.getChildIndex(p.first), p.second};
};

template<ScrollableAlign align>
void setScrollAlignId(CoreComponent& component, const Object& value)
{
    if (!value.isArray() || value.size() != 2) {
        CONSOLE_CTP(component.getContext()) << "Invalid " << value.toDebugString();
        return;
    }

    auto id = value.at(0).asString();
    auto child = std::dynamic_pointer_cast<CoreComponent>(component.findComponentById(id));
    if (!child) {
        CONSOLE_CTP(component.getContext()) << "Unable to find child with id " << id;
        return;
    }

    auto& self = dynamic_cast<MultiChildScrollableComponent&>(component);
    self.setScrollPositionDirectlyByChild(child, align, value.at(1).asNumber());
};

template<ScrollableAlign align>
void setScrollAlignIndex(CoreComponent& component, const Object& value)
{
    if (!value.isArray() || value.size() != 2) {
        CONSOLE_CTP(component.getContext()) << "Invalid " << value.toDebugString();
        return;
    }

    auto index = value.at(0).asInt();
    if (index < 0 || index >= component.getChildCount()) {
        CONSOLE_CTP(component.getContext()) << "Child index out of range " << index;
        return;
    }

    auto child = std::dynamic_pointer_cast<CoreComponent>(component.getChildAt(index));
    auto& self = dynamic_cast<MultiChildScrollableComponent&>(component);
    self.setScrollPositionDirectlyByChild(child, align, value.at(1).asNumber());
};

const ComponentPropDefSet&
MultiChildScrollableComponent::propDefSet() const
{
    static auto getCenterId = &getScrollAlignId<MultiChildScrollableComponent,
        &MultiChildScrollableComponent::getCenterChildInViewInternal>;
    static auto setCenterId = &setScrollAlignId<kScrollableAlignCenter>;
    static auto getCenterIndex = &getScrollAlignIndex<MultiChildScrollableComponent,
        &MultiChildScrollableComponent::getCenterChildInViewInternal>;
    static auto setCenterIndex = &setScrollAlignIndex<kScrollableAlignCenter>;

    static auto getFirstId = &getScrollAlignId<MultiChildScrollableComponent,
        &MultiChildScrollableComponent::getFirstChildInViewInternal>;
    static auto setFirstId = &setScrollAlignId<kScrollableAlignFirst>;
    static auto getFirstIndex = &getScrollAlignIndex<MultiChildScrollableComponent,
        &MultiChildScrollableComponent::getFirstChildInViewInternal>;
    static auto setFirstIndex = &setScrollAlignIndex<kScrollableAlignFirst>;

    static ComponentPropDefSet sSequenceComponentProperties(ScrollableComponent::propDefSet(), {
        {kPropertyHeight,      Dimension(),           asDimension,    kPropIn | kPropDynamic | kPropStyled, yn::setHeight, defaultSequenceHeight},
        {kPropertyWidth,       Dimension(),           asDimension,    kPropIn | kPropDynamic | kPropStyled, yn::setWidth,  defaultSequenceWidth},
        {kPropertySnap,        kSnapNone,             sSnapMap,       kPropInOut | kPropStyled},
        {kPropertyNumbered,    false,                 asBoolean,      kPropIn | kPropVisualContext},
        {kPropertyOnScroll,    Object::EMPTY_ARRAY(), asCommand,      kPropIn},
        {kPropertyCenterId,    getCenterId,           setCenterId,    kPropDynamic | kPropSetAfterLayout},
        {kPropertyCenterIndex, getCenterIndex,        setCenterIndex, kPropDynamic | kPropSetAfterLayout},
        {kPropertyFirstId,     getFirstId,            setFirstId,     kPropDynamic | kPropSetAfterLayout},
        {kPropertyFirstIndex,  getFirstIndex,         setFirstIndex,  kPropDynamic | kPropSetAfterLayout},
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
    bool isLTR = getCalculated(kPropertyLayoutDirection) == kLayoutDirectionLTR;
    double scrollSize = vertical
                        ? innerBounds.getBottom()
                        : (isLTR ? innerBounds.getRight() : innerBounds.getLeft());
    double innerScrollSize = vertical
                             ? lastChildBounds.getBottom()
                             : (isLTR ? lastChildBounds.getRight() : lastChildBounds.getLeft());
    if (isHorizontal() && !isLTR) {
        return currentPosition + scrollSize > innerScrollSize;
    }
    return currentPosition + scrollSize < innerScrollSize;
}

/*
 * The child position is specified by the alignment and the percentage offset.  A child
 * with alignment "First" has a base position of it's top/left side aligned with the top/left of the
 * innerBounds of the parent.  A child with alignment "Center" has a base position of the center
 * of the child placed in the center of the innerBounds.
 *
 * The percentage offset shifts the child by a percentage of its width/height, where a positive
 * offset shifts the child to the right/down.
 *
 * Parent Bounds
 * +----------------------------------------+
 * |  First        Center (of inner bounds) |
 * |    |            |                      | First   Center
 * |    +-------------------------+         |   |       |
 * |    | Parent Inner Bounds     |         |   +--------------+
 * |    |                         |         |   | Child Bounds |
 * |    |                         |         |   +--------------+
 * |    +-------------------------+         |
 * +----------------------------------------+
 */
void
MultiChildScrollableComponent::setScrollPositionDirectlyByChild(const CoreComponentPtr& child,
                                                                ScrollableAlign align,
                                                                float offset)
{
    assert(child);

    ensureChildLayout(child, true); // TODO: Should this set dirty?  With the initial expansion this should not be set...
    const auto childBounds = child->getCalculated(kPropertyBounds).getRect();
    const auto innerBounds = mCalculated.get(kPropertyInnerBounds).getRect();

    const auto horiz = isHorizontal();
    const auto childSize = horiz ? childBounds.getWidth() : childBounds.getHeight();
    const auto parentSize = horiz ? innerBounds.getWidth() : innerBounds.getHeight();

    // Calculate the current offset of the child from the desired position
    float scrollPosition = horiz ? childBounds.getLeft() - innerBounds.getLeft() : childBounds.getTop() - innerBounds.getTop();

    // Adjust for centered positions
    switch (align) {
        case kScrollableAlignCenter:
            scrollPosition += childSize / 2 - parentSize / 2;
            break;
        case kScrollableAlignFirst:
            break;
    }

    // Add in the percentage offset.  Note that a negative value increases the scroll position
    scrollPosition -= childSize * offset;
    setScrollPositionDirectly(scrollPosition);
}

void
MultiChildScrollableComponent::ensureChildLayout(const CoreComponentPtr& child, bool useDirtyFlag)
{
    // TODO: Search for child and if direct - attach if required.
    auto it = std::find(mChildren.begin(), mChildren.end(), child);
    if (it != mChildren.end()) {
        auto index = std::distance(mChildren.begin(), it);
        layoutChildIfRequired(child, index, true, false);
        child->markDisplayedChildrenStale(true);
    } else {
        child->ensureLayoutInternal(useDirtyFlag);
    }
}

bool
MultiChildScrollableComponent::allowBackwards() const {
    auto currentPosition = mCalculated.get(kPropertyScrollPosition).asNumber();
    bool isRTL = getCalculated(kPropertyLayoutDirection) == kLayoutDirectionRTL;
    return (isHorizontal() && isRTL) ? currentPosition < 0 : currentPosition > 0;
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

std::pair<CoreComponentPtr, float>
MultiChildScrollableComponent::getFirstChildInViewInternal() const
{
    auto& mutableSelf = const_cast<MultiChildScrollableComponent&>(*this);
    mutableSelf.ensureChildrenVisibilityUpdated();

    if (mFirstChildInView == -1)
        return {nullptr, 0.0};

    // Calculate the percentage shift in the child from the top-left corner of the inner bounds
    auto child = mChildren.at(mFirstChildInView);
    auto childBounds = child->getCalculated(kPropertyBounds).getRect();
    auto topLeft = mCalculated.get(kPropertyInnerBounds).getRect().getTopLeft();
    auto scrollPosition = mCalculated.get(kPropertyScrollPosition).asNumber();

    float offset = 0.0;
    if (!childBounds.empty()) {
        if (isHorizontal())
            offset = (childBounds.getLeft() - topLeft.getX() - scrollPosition) / childBounds.getWidth();
        else
            offset = (childBounds.getTop() - topLeft.getY() - scrollPosition) / childBounds.getHeight();
    }

    return { child, offset };
}

std::pair<CoreComponentPtr, float>
MultiChildScrollableComponent::getCenterChildInViewInternal() const
{
    auto& mutableSelf = const_cast<MultiChildScrollableComponent&>(*this);
    mutableSelf.ensureChildrenVisibilityUpdated();

    if (mFirstChildInView == -1)
        return {nullptr, 0.0};

    // Find the closest child by distance to the center point
    auto center = mCalculated.get(kPropertyInnerBounds).getRect().getCenter() + scrollPosition();
    auto bestDistance = std::numeric_limits<float>::max();
    CoreComponentPtr bestChild = nullptr;

    for (size_t index = mFirstChildInView; index <= mLastChildInView; index++) {
        auto child = mChildren.at(index);
        auto distance = child->getCalculated(kPropertyBounds).getRect().distanceTo(center);
        if (distance < bestDistance) {
            bestChild = child;
            bestDistance = distance;
        }
    }

    // Calculate the percentage shift of the center of the child from the center of the innerBounds
    auto childBounds = bestChild->getCalculated(kPropertyBounds).getRect();
    center = mCalculated.get(kPropertyInnerBounds).getRect().getCenter() + scrollPosition();  // Switch to innerBounds

    float offset = 0.0;
    if (!childBounds.empty()) {
        if (isHorizontal())
            offset = (childBounds.getCenterX() - center.getX()) / childBounds.getWidth();
        else
            offset = (childBounds.getCenterY() - center.getY()) / childBounds.getHeight();
    }

    return { bestChild, offset };
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

inline double
getSpacing(const CoreComponent& component) {
    auto spacing = component.getCalculated(kPropertySpacing).asDimension(*component.getContext());
    if (spacing.isAbsolute()) {
        return spacing.getValue();
    }
    return 0;
}

void
MultiChildScrollableComponent::fixScrollPosition(const Rect& oldAnchorRect, const Rect& anchorRect)
{
    if (anchorRect != oldAnchorRect) {
        auto layoutDirection = static_cast<LayoutDirection>(getCalculated(kPropertyLayoutDirection).asInt());
        auto currentPosition = getCalculated(kPropertyScrollPosition).asNumber();
        float offset;
        if (isHorizontal()) {
            offset = layoutDirection == kLayoutDirectionLTR
                     ? anchorRect.getLeft() - oldAnchorRect.getLeft()
                     : anchorRect.getRight() - oldAnchorRect.getRight();
        } else {
            offset = anchorRect.getTop() - oldAnchorRect.getTop();
        }
        currentPosition += offset;
        mCalculated.set(kPropertyScrollPosition, Dimension(DimensionType::Absolute, currentPosition));
        setDirty(kPropertyScrollPosition);
    }
}

Point
MultiChildScrollableComponent::trimScroll(const Point& point)
{
    // Break out early. If there are no children - no scrolling possible
    if (shouldNotPropagateLayoutChanges() ||
        getCalculated(kPropertyBounds).empty())
        return Point();

    if (mDelayLayoutAction && mDelayLayoutAction->isPending()) {
        mDelayLayoutAction->terminate();
        mDelayLayoutAction = nullptr;
    }

    auto innerBounds = mCalculated.get(kPropertyInnerBounds).getRect();
    // We treat this component as 0 point of sequence. All calculation happens in relation to it.
    auto zeroAnchorIdx = mEnsuredChildren.empty() ? 0 : mEnsuredChildren.lowerBound();
    auto zeroAnchor = mChildren.at(zeroAnchorIdx);
    if (!zeroAnchor->isAttached()) layoutChildIfRequired(zeroAnchor, zeroAnchorIdx, true, false);
    auto oldZeroAnchorPos = zeroAnchor->getCalculated(kPropertyBounds).getRect();
    auto zeroAnchorPos = oldZeroAnchorPos.getTopLeft() - innerBounds.getTopLeft();

    // We should disregard spacing in limit calculation as it's not part of inner bounds really.
    auto spacing = mEnsuredChildren.lowerBound() > 0 ? getSpacing(*zeroAnchor) : 0;

    // We've guaranteed that at least one child is non-empty
    assert(!mEnsuredChildren.empty());

    if (isVertical()) {
        auto y = point.getY();
        // Cover in back direction. Current code ensures from index 0 to requested component and any updates
        // asynchronous so no way it will happen twice.
        while (y + zeroAnchorPos.getY() < 0 && mEnsuredChildren.lowerBound() > 0) {
            auto idx = mEnsuredChildren.lowerBound() - 1;
            layoutChildIfRequired(mChildren.at(idx), idx, true, false);
            zeroAnchorPos = zeroAnchor->getCalculated(kPropertyBounds).getRect().getTopLeft() - innerBounds.getTopLeft();
        }
        fixScrollPosition(oldZeroAnchorPos, zeroAnchor->getCalculated(kPropertyBounds).getRect());

        y += zeroAnchorPos.getY() - spacing;

        if (y <= 0)
            return Point();

        float bottom = innerBounds.getBottom();
        float maxY = 0;

        // Ensure children until they cover the sequence.
        int startingChild = std::max(mEnsuredChildren.upperBound(), 0);
        for (int i = startingChild ; i<mChildren.size() ; i++) {
            const auto& child = mChildren.at(i);
            layoutChildIfRequired(child, i, false, false);
            maxY = nonNegative(child->getCalculated(kPropertyBounds).getRect().getBottom() - bottom);
            if (y <= maxY)
                return Point(0,y);
        }

        return Point(0, maxY);
    }
    else if (getCalculated(kPropertyLayoutDirection) == kLayoutDirectionLTR) {  // Horizontal LTR
        auto x = point.getX();
        while (x + zeroAnchorPos.getX() < 0 && mEnsuredChildren.lowerBound() > 0) {
            auto idx = mEnsuredChildren.lowerBound() - 1;
            layoutChildIfRequired(mChildren.at(idx), idx, true, false);
            zeroAnchorPos = zeroAnchor->getCalculated(kPropertyBounds).getRect().getTopLeft() - innerBounds.getTopLeft();
        }
        fixScrollPosition(oldZeroAnchorPos, zeroAnchor->getCalculated(kPropertyBounds).getRect());

        x += zeroAnchorPos.getX() - spacing;

        if (x <= 0)
            return Point();

        float right = innerBounds.getRight();
        float maxX = 0;

        // Ensure children until they cover the sequence.
        int startingChild = std::max(mEnsuredChildren.upperBound(), 0);
        for (int i = startingChild ; i<mChildren.size(); i++) {
            const auto& child = mChildren.at(i);
            layoutChildIfRequired(child, i, true, false);
            maxX = nonNegative(child->getCalculated(kPropertyBounds).getRect().getRight() - right);
            if (x <= maxX)
                return Point(x,0);
        }

        return Point(maxX, 0);
    } else { // Horizontal RTL
        auto x = point.getX();
        zeroAnchorPos = zeroAnchor->getCalculated(kPropertyBounds).getRect().getTopRight() - innerBounds.getTopRight();

        while (x > zeroAnchorPos.getX() && mEnsuredChildren.lowerBound() > 0) {
            auto idx = mEnsuredChildren.lowerBound() - 1;
            layoutChildIfRequired(mChildren.at(idx), idx, true, false);
            zeroAnchorPos = zeroAnchor->getCalculated(kPropertyBounds).getRect().getTopRight() - innerBounds.getTopRight();
        }
        fixScrollPosition(oldZeroAnchorPos, zeroAnchor->getCalculated(kPropertyBounds).getRect());

        x -= zeroAnchorPos.getX() - spacing;

        if (x >= 0)
            return Point();

        float left = innerBounds.getLeft();
        float maxX = 0;

        // Ensure children until they cover the sequence.
        int startingChild = std::max(mEnsuredChildren.upperBound(), 0);
        for (int i = startingChild ; i<mChildren.size(); i++) {
            const auto& child = mChildren.at(i);
            layoutChildIfRequired(child, i, true, false);
            maxX = nonPositive(child->getCalculated(kPropertyBounds).getRect().getLeft() - left);
            if (x >= maxX)
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
MultiChildScrollableComponent::ensureChildAttached(const CoreComponentPtr& child, int targetIdx)
{
    if (mEnsuredChildren.empty() || mEnsuredChildren.above(targetIdx)) {
        // Ensure from upperBound to target
        for (int index = mEnsuredChildren.empty() ? 0 : mEnsuredChildren.upperBound() + 1; index <= targetIdx ; index++) {
            const auto& c = mChildren.at(index);
            if (mRebuilder) {
                mRebuilder->inflateIfRequired(c);
            }
            if (attachChild(c, mEnsuredChildren.size())) {
                mEnsuredChildren.expandTo(index);
            }
        }
    } else if (mEnsuredChildren.below(targetIdx)) {
        // Ensure from lowerBound down to target
        for (int index = mEnsuredChildren.lowerBound() - 1; index >= targetIdx ; index--) {
            const auto& c = mChildren.at(index);
            if (mRebuilder) {
                mRebuilder->inflateIfRequired(c);
            }
            if (attachChild(c, 0)) {
                mEnsuredChildren.expandTo(index);
            }
        }
    } else {
        if (mRebuilder) {
            mRebuilder->inflateIfRequired(child);
        }
        // Just attach single one inside of ensured range if needed.
        attachChild(child, targetIdx - mEnsuredChildren.lowerBound());
    }
}

bool
MultiChildScrollableComponent::attachChild(const CoreComponentPtr& child, size_t index)
{
    if (child->isAttached()) {
        return false;
    }

    YGNodeInsertChild(mYGNodeRef, child->getNode(), index);
    child->updateNodeProperties();
    return true;
}

void
MultiChildScrollableComponent::attachYogaNode(const CoreComponentPtr& child)
{
    auto it = std::find(mChildren.begin(), mChildren.end(), child);
    assert(it != mChildren.end());
    auto index = std::distance(mChildren.begin(), it);

    // The child should not already be attached and it should not be in the ensured range
    assert(!child->isAttached());
    assert(!mEnsuredChildren.contains(index));

    // Extend the ensured children range one child at a time
    while (!mEnsuredChildren.contains(index)) {
        auto childIndex = mEnsuredChildren.extendTowards(index);
        auto& c = mChildren.at(childIndex);
        assert(!c->isAttached());
        YGNodeInsertChild(mYGNodeRef, c->getNode(), childIndex - mEnsuredChildren.lowerBound());
        c->updateNodeProperties();
    }
}

void
MultiChildScrollableComponent::layoutChildIfRequired(const CoreComponentPtr& child, size_t childIdx, bool useDirtyFlag, bool first)
{
    APL_TRACE_BLOCK("MultiChildScrollableComponent:layoutChildIfRequired");
    if (!child->isAttached() || child->getCalculated(kPropertyBounds).empty()) {
        ensureChildAttached(child, childIdx);
        relayoutInPlace(useDirtyFlag, first);
    }
}

void
MultiChildScrollableComponent::relayoutInPlace(bool useDirtyFlag, bool first)
{
    APL_TRACE_BLOCK("MultiChildScrollableComponent:relayoutInPlace");
    auto root = getRootComponent();
    auto rootBounds = root->getCalculated(kPropertyBounds).getRect();
    APL_TRACE_BEGIN("MultiChildScrollableComponent:YGNodeCalculateLayout:root");
    YGNodeCalculateLayout(root->getNode(), rootBounds.getWidth(), rootBounds.getHeight(), getLayoutDirection());
    APL_TRACE_END("MultiChildScrollableComponent:YGNodeCalculateLayout:root");
    auto oldBounds = getCalculated(kPropertyBounds);
    CoreComponent::processLayoutChanges(useDirtyFlag, first);
    if (oldBounds != getCalculated(kPropertyBounds)) {
        mContext->layoutManager().needToReProcessLayoutChanges();
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
MultiChildScrollableComponent::insertChild(const CoreComponentPtr& child, size_t index, bool useDirtyFlag)
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

    if (mEnsuredChildren.contains(index)) {
        mEnsuredChildren.remove(index);

        // The mEnsuredChildren code makes the assumption that if mChildren is not empty then
        // mEnsuredChildren is not empty. However if mEnsuredChildren is empty we have no anchor
        // to update the scroll position against so we reset it to 0
        if (mEnsuredChildren.empty() && !mChildren.empty()) {
            mEnsuredChildren.insert(0);
            mCalculated.set(kPropertyScrollPosition, Dimension(DimensionType::Absolute, 0));
            setDirty(kPropertyScrollPosition);
        }
    } else if (!mEnsuredChildren.empty() && (int)index < mEnsuredChildren.lowerBound())
        mEnsuredChildren.shift(-1);

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

/**
 * Relatively simple heuristics: take laid-out anchor component and estimate how many components will be required to
 * cover child cache region. Precise calculation is still up to proper layout pass, but (especially for cases when
 * children are uniform) number of layouts is decreased significantly (for up to 2 instead of n where n is number of
 * required children).
 */
void
MultiChildScrollableComponent::runLayoutHeuristics(size_t anchorIdx, float childCache, float pageSize, bool useDirtyFlag, bool first)
{
    // Estimate how many children is actually required based on available anchor dimensions
    auto toCover = estimateChildrenToCover(first ? pageSize : (childCache + 1) * pageSize, anchorIdx);
    auto attached = false;
    for (int i = mEnsuredChildren.upperBound(); i < std::min(anchorIdx + toCover, mChildren.size()); i++) {
        auto child = mChildren.at(i);
        if (!child->isAttached() || child->getCalculated(kPropertyBounds).empty()) {
            attached = true;
            ensureChildAttached(child, i);
            if (i > 0 && childrenUseSpacingProperty()) {
                child->fixSpacing();
            }
        }
    }

    if (!first) {
        toCover = estimateChildrenToCover(childCache * pageSize, anchorIdx);
        for (int i = mEnsuredChildren.lowerBound(); i >= std::max(0, static_cast<int>(anchorIdx - toCover)); i--) {
            auto child = mChildren.at(i);
            if (!child->isAttached() || child->getCalculated(kPropertyBounds).empty()) {
                attached = true;
                ensureChildAttached(child, i);
                if (i > 0 && childrenUseSpacingProperty()) {
                    child->fixSpacing();
                }
            }
        }
    }

    if (attached) relayoutInPlace(useDirtyFlag, first);
}

void
MultiChildScrollableComponent::processLayoutChanges(bool useDirtyFlag, bool first) {
    APL_TRACE_BLOCK("MultiChildScrollableComponent:processLayoutChanges");
    // We need to account for padding as find function don't do that automatically.
    bool horizontal = isHorizontal();
    auto oldLayoutDirection = static_cast<LayoutDirection>(mCalculated.get(kPropertyLayoutDirection).asInt());
    auto startPoint = oldLayoutDirection == kLayoutDirectionLTR
                          ? getCalculated(kPropertyInnerBounds).getRect().getTopLeft()
                          : getCalculated(kPropertyInnerBounds).getRect().getTopRight();
    auto paddedScrollPosition = scrollPosition() + startPoint;
    auto anchor = std::static_pointer_cast<CoreComponent>(findDirectChildAtPosition(paddedScrollPosition));

    Rect oldAnchorBounds;
    if (anchor) {
        oldAnchorBounds = anchor->getCalculated(kPropertyBounds).getRect();
    }

    CoreComponent::processLayoutChanges(useDirtyFlag, first);

    // If starting empty ask for loading for conditional cases
    if (mChildren.empty() && mRebuilder) {
        mRebuilder->notifyStartEdgeReached();
        mRebuilder->notifyEndEdgeReached();
    }

    if (shouldNotPropagateLayoutChanges()) {
        // Starting with empty or invalid sequence
        return;
    }

    auto layoutDirection = static_cast<LayoutDirection>(getCalculated(kPropertyLayoutDirection).asInt());

    // We have not laid-out anything before. Refer to first available attached child. Possible only on initial layout.
    size_t anchorIdx = 0;
    if (!anchor) {
        anchorIdx = mEnsuredChildren.empty() ? 0 : mEnsuredChildren.lowerBound();
        anchor = mChildren.at(anchorIdx);
        layoutChildIfRequired(anchor, anchorIdx, useDirtyFlag, first);
        oldAnchorBounds = anchor->getCalculated(kPropertyBounds).getRect();
    } else {
        auto it = std::find(mChildren.begin(), mChildren.end(), anchor);
        anchorIdx = std::distance(mChildren.begin(), it);
        if (oldLayoutDirection != layoutDirection) {
            oldAnchorBounds = anchor->getCalculated(kPropertyBounds).getRect();
            auto mirroredScrollPosition = getCalculated(kPropertyScrollPosition).asNumber() * -1.0f;
            mCalculated.set(kPropertyScrollPosition, Dimension(DimensionType::Absolute, mirroredScrollPosition));
            setDirty(kPropertyScrollPosition);
        }
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

    // Try to figure majority of layout as a bulk
    runLayoutHeuristics(anchorIdx, childCache, pageSize, useDirtyFlag, first);
    // Anchor bounds may have shifted
    Rect anchorBounds = anchor->getCalculated(kPropertyBounds).getRect();
    float anchorPosition = horizontal
                           ? (layoutDirection == kLayoutDirectionLTR ? anchorBounds.getX() : anchorBounds.getRight())
                           : anchorBounds.getY();

    // Lay out children in positive order until we hit cache limit.
    auto distanceToCover = first ? pageSize : (childCache + 1) * pageSize;
    float positionToCover = layoutDirection == kLayoutDirectionLTR
                            ? anchorPosition + distanceToCover
                            : anchorPosition - distanceToCover;
    bool targetCovered = false;
    int lastLoaded = mEnsuredChildren.lowerBound();
    for (; lastLoaded < mChildren.size(); lastLoaded++) {
        auto child = mChildren.at(lastLoaded);
        layoutChildIfRequired(child, lastLoaded, useDirtyFlag, first);
        auto childBounds = child->getCalculated(kPropertyBounds).getRect();
        float childCoveredPosition = horizontal
                               ? (layoutDirection == kLayoutDirectionLTR ? childBounds.getRight()
                                                                         : childBounds.getLeft())
                               : childBounds.getBottom();
        targetCovered = layoutDirection == kLayoutDirectionLTR ? childCoveredPosition > positionToCover
                                                                    : childCoveredPosition < positionToCover;
        if (targetCovered) {
            break;
        }
    }

    // In case if children of sequence highly conditional we need to ask for more data regardless of position - for
    // cases when long distance exists between valid data indexes.
    if ((mEnsuredChildren.upperBound() + 1 >= mChildren.size()) && !targetCovered && mRebuilder) {
        mRebuilder->notifyEndEdgeReached();
    } else {
        reportLoaded(std::min(lastLoaded, mEnsuredChildren.upperBound()));
    }

    // Lay out children in negative order until we hit cache limit.
    positionToCover = layoutDirection == kLayoutDirectionLTR
        ? childCache * pageSize
        : childCache * pageSize * -1.0f;
    int firstLoaded = mEnsuredChildren.upperBound();
    targetCovered = false;
    for (; firstLoaded >= 0; firstLoaded--) {
        auto child = mChildren.at(firstLoaded);
        layoutChildIfRequired(child, firstLoaded, useDirtyFlag, first);
        auto childBounds = child->getCalculated(kPropertyBounds).getRect();
        anchorBounds = anchor->getCalculated(kPropertyBounds).getRect();
        float distance = (horizontal ? anchorBounds.getLeft() : anchorBounds.getTop())
                       - (horizontal ? childBounds.getLeft() : childBounds.getTop());
        targetCovered = layoutDirection == kLayoutDirectionLTR ? distance > positionToCover : distance < positionToCover;
        if (targetCovered) {
            break;
        }
    }

    if ((mEnsuredChildren.lowerBound() == 0) && !targetCovered && mRebuilder) {
        mRebuilder->notifyStartEdgeReached();
    } else {
        reportLoaded(std::max(firstLoaded, mEnsuredChildren.lowerBound()));
    }

    if (anchor) {
        // Adjust scroll position if changed at the beginning of sequence
        fixScrollPosition(oldAnchorBounds, anchorBounds);
    }

    ensureChildrenVisibilityUpdated();

    if (first) {
        // Avoid yoga initiated re-layout that may be caused by attaching components that were already laid-out
        mContext->layoutManager().remove(getRootComponent());

        // Postpone to the next frame, if any
        if (mEnsuredChildren.upperBound() + 1 >= mChildren.size()) return;
        mDelayLayoutAction = Action::makeDelayed(getRootConfig().getTimeManager(), 1);
        auto weak_self = std::weak_ptr<MultiChildScrollableComponent>(
                std::static_pointer_cast<MultiChildScrollableComponent>(shared_from_corecomponent()));
        mDelayLayoutAction->then([weak_self](const ActionPtr &) {
            auto self = weak_self.lock();
            if (self) {
                self->processLayoutChanges(true, false);
                self->mDelayLayoutAction = nullptr;
            }
        });
    }
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

    // Force figuring out what is on screen.
    processLayoutChanges(true, false);
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
    bool isLTR = getCalculated(kPropertyLayoutDirection) == kLayoutDirectionLTR;

    if (isVertical()) {
        childStart = childBoundsInParent.getTop();
        childEnd = childBoundsInParent.getBottom();
        currentPosition = scrollPosition().getY();
    } else {
        childStart = isLTR ? childBoundsInParent.getLeft() : childBoundsInParent.getRight();
        childEnd = isLTR ? childBoundsInParent.getRight() : childBoundsInParent.getLeft();
        currentPosition = scrollPosition().getX();
    }

    switch (snap) {
        case kSnapStart:
        case kSnapForceStart:
            scrollTo = childStart - parentStart;
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

ComponentPtr
MultiChildScrollableComponent::findChildCloseToPosition(const Point& position) const
{
    if (mEnsuredChildren.empty())
        return nullptr;

    auto vertical = isVertical();
    auto directionalOffset = vertical ? position.getY() : position.getX();
    auto bestCandidate = mChildren.at(mEnsuredChildren.upperBound());
    auto bestDistance = std::numeric_limits<float>::max();

    for (int i = mEnsuredChildren.upperBound(); i >= mEnsuredChildren.lowerBound(); i--) {
        auto child = mChildren.at(i);
        auto bounds = child->getCalculated(kPropertyBounds).getRect();
        auto referencePosition = vertical ? bounds.getCenterY() : bounds.getCenterX();
        auto distance = std::abs(referencePosition - directionalOffset);

        if (distance <= bestDistance) {
            bestCandidate = child;
            bestDistance = distance;
        } else {
            break;
        }
    }

    return bestCandidate;
}

Point
MultiChildScrollableComponent::getSnapOffset() const
{
    auto snap = static_cast<Snap>(getCalculated(kPropertySnap).getInteger());
    if (snap == kSnapNone) return {};

    // It's unidirectional on purpose, snap on multidirectional scrolling is quite unclear
    const auto vertical = isVertical();
    Rect parentInnerBounds = getCalculated(kPropertyInnerBounds).getRect();
    auto lastChildBounds = mChildren.at(mEnsuredChildren.upperBound())->getCalculated(kPropertyBounds).getRect();
    bool isRTL = getCalculated(kPropertyLayoutDirection) == kLayoutDirectionRTL;
    auto lastChildBottom = isRTL ? lastChildBounds.getBottomLeft() : lastChildBounds.getBottomRight();

    // Figure to which position we should snap
    float parentStart, parentEnd, parentOffset, forwardScrollLimit, scrollOffset;
    if (vertical) {
        parentStart = parentInnerBounds.getTop();
        parentEnd = parentInnerBounds.getBottom();
        parentOffset = parentEnd - parentStart;
        forwardScrollLimit = lastChildBottom.getY() - parentOffset;
        scrollOffset = scrollPosition().getY();
    } else {
        parentStart = isRTL ? parentInnerBounds.getRight() : parentInnerBounds.getLeft();
        parentEnd = isRTL ? parentInnerBounds.getLeft() : parentInnerBounds.getRight();
        parentOffset = parentEnd - parentStart;
        forwardScrollLimit = isRTL ? lastChildBottom.getX() : lastChildBottom.getX() - parentOffset;
        scrollOffset =  scrollPosition().getX();
    }

    // If we currently at a limit - do not snap, we already snapped to limit.
    bool atLimit = (isHorizontal() && isRTL)
                       ? (scrollOffset >= 0 || scrollOffset <= forwardScrollLimit)
                       : (scrollOffset <= 0 || scrollOffset >= forwardScrollLimit);
    if (atLimit) return {};

    float parentSnapOffset = 0;
    switch (snap) {
        case kSnapCenter:
        case kSnapForceCenter:
            parentSnapOffset = parentOffset / 2;
            break;
        case kSnapEnd:
        case kSnapForceEnd:
            parentSnapOffset = parentOffset;
            break;
        default:
            break;
    }

    Point referencePoint;
    if (vertical) {
        referencePoint = Point(parentInnerBounds.getX(), scrollOffset + parentSnapOffset);
    } else {
        auto startPoint = isRTL ? parentInnerBounds.getTopRight() : parentInnerBounds.getTopLeft();
        referencePoint = Point(scrollOffset + parentSnapOffset + startPoint.getX(), parentInnerBounds.getY());
    }

    auto targetChild = findDirectChildAtPosition(referencePoint);
    // May have hit spacing here. Need to find child "close" to position, not directly at it
    if (!targetChild) {
        targetChild = findChildCloseToPosition(referencePoint);
    }
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
                if (targetIndex >= itemsPerCourse) {
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
        LOG(LogLevel::kWarn) << "Can't snap on scroll offset " << scrollOffset;
        return {};
    }

    auto offset = getSnapOffsetForChild(targetChild, snap, parentStart, parentEnd);

    // Check if adjustment will overshoot us over scrolling limits. If so - clip to scroll limit.
    bool exceededBackwardScrollLimit = (isHorizontal() && isRTL)
                                     ? scrollOffset + offset >= 0
                                     : scrollOffset + offset <= 0;
    if (exceededBackwardScrollLimit) {
        offset = -scrollOffset;
    }

    bool exceededForwardScrollLimit = (isHorizontal() && isRTL)
                                    ? scrollOffset + offset <= forwardScrollLimit
                                    : scrollOffset + offset >= forwardScrollLimit;
    if (exceededForwardScrollLimit) {
        offset = forwardScrollLimit - scrollOffset;
    }

    return (vertical ? Point(0, offset) : Point(offset, 0));
}

void
MultiChildScrollableComponent::attachYogaNodeIfRequired(const CoreComponentPtr& coreChild, int index)
{
    // For most layouts we just attach the Yoga node.
    // For sequences we don't attach until ensureLayout is called if outside of existing ensured range.
    if (shouldAttachChildYogaNode(index)
        || (mEnsuredChildren.contains(index) && index > mEnsuredChildren.lowerBound())) {

        auto offset = mEnsuredChildren.insert(index);
        YGNodeInsertChild(mYGNodeRef, coreChild->getNode(), offset);
    } else if (!mEnsuredChildren.empty() && index <= mEnsuredChildren.lowerBound()) {
        mEnsuredChildren.shift(1);
    }
}

bool
MultiChildScrollableComponent::shouldBeFullyInflated(int index) const
{
    return shouldAttachChildYogaNode(index) ||
           (mChildren.empty() && index == 0) ||
           (mEnsuredChildren.contains(index) && index > mEnsuredChildren.lowerBound());
}

} // namespace apl
