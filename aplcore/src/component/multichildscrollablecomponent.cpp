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
    double scrollSize = vertical
                        ? innerBounds.getBottom()
                        : innerBounds.getRight();
    double innerScrollSize = vertical
                             ? lastChildBounds.getBottom()
                             : lastChildBounds.getRight();
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

    child->ensureLayout(true);  // TODO: Should this set dirty?  With the initial expansion this should not be set...
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

bool
MultiChildScrollableComponent::allowBackwards() const {
    auto currentPosition = mCalculated.get(kPropertyScrollPosition).asNumber();
    return (currentPosition > 0);
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

    // We should disregard spacing in limit calculation as it's not part of inner bounds really.
    auto spacing = mEnsuredChildren.lowerBound() > 0 ? getSpacing(*zeroAnchor) : 0;

    // We've guaranteed that at least one child is non-empty
    assert(!mEnsuredChildren.empty());

    if (isVertical()) {
        auto y = point.getY();
        // Cover in back direction. Current code ensures from index 0 to requested component and any updates
        // asynchronous so no way it will happen twice.
        while (y < zeroAnchorPos.getY() && mEnsuredChildren.lowerBound() > 0) {
            mChildren.at(mEnsuredChildren.lowerBound() - 1)->ensureLayout(true);
            zeroAnchorPos = zeroAnchor->getCalculated(kPropertyBounds).getRect().getTopLeft() - innerBounds.getTopLeft();
        }

        y += zeroAnchorPos.getY() - spacing;

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
        while (x < zeroAnchorPos.getX() && mEnsuredChildren.lowerBound() > 0) {
            mChildren.at(mEnsuredChildren.lowerBound() - 1)->ensureLayout(true);
            zeroAnchorPos = zeroAnchor->getCalculated(kPropertyBounds).getRect().getTopLeft() - innerBounds.getTopLeft();
        }

        x += zeroAnchorPos.getX() - spacing;

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
MultiChildScrollableComponent::ensureChildAttached(const CoreComponentPtr& child, int targetIdx)
{
    if (mEnsuredChildren.empty() || mEnsuredChildren.above(targetIdx)) {
        // Ensure from upperBound to target
        for (int index = mEnsuredChildren.empty() ? 0 : mEnsuredChildren.upperBound() + 1; index <= targetIdx ; index++) {
            const auto& c = mChildren.at(index);
            if (attachChild(c, mEnsuredChildren.size())) {
                mEnsuredChildren.expandTo(index);
            }
        }
    } else if (mEnsuredChildren.below(targetIdx)) {
        // Ensure from lowerBound down to target
        for (int index = mEnsuredChildren.lowerBound() - 1; index >= targetIdx ; index--) {
            const auto& c = mChildren.at(index);
            if (attachChild(c, 0)) {
                mEnsuredChildren.expandTo(index);
            }
        }
    } else {
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
MultiChildScrollableComponent::layoutChildIfRequired(const CoreComponentPtr& child, size_t childIdx, bool useDirtyFlag)
{
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
        layoutChildIfRequired(anchor, mEnsuredChildren.lowerBound(), useDirtyFlag);
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

    // Force figuring out what is on screen.
    processLayoutChanges(true);
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

        if (distance < bestDistance) {
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
    if (scrollOffset + offset <= 0) {
        offset = -scrollOffset;
    }

    if (scrollOffset + offset >= forwardScrollLimit) {
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

} // namespace apl
