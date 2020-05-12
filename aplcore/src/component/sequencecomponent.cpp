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

#include "apl/component/componentpropdef.h"
#include "apl/component/sequencecomponent.h"
#include "apl/component/yogaproperties.h"
#include "apl/content/rootconfig.h"

namespace apl {

CoreComponentPtr
SequenceComponent::create(const ContextPtr& context,
                          Properties&& properties,
                          const std::string& path)
{
    auto ptr = std::make_shared<SequenceComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

SequenceComponent::SequenceComponent(const ContextPtr& context,
                                     Properties&& properties,
                                     const std::string& path)
    : ScrollableComponent(context, std::move(properties), path)
{
}

void
SequenceComponent::update(UpdateType type, float value)
{
    ScrollableComponent::update(type, value);
    if (type == kUpdateScrollPosition) {
        // Force figuring out what is on screen.
        processLayoutChanges(true);
        updateSeen();
    }
}

ComponentPtr
SequenceComponent::findChildAtPosition(const Point& position) const
{
    // Not all children may be ensured.  We start at the end of the list of ensured children
    // TODO: Given the serial nature of layout in a Sequence, a binary search would be faster

    if (mChildren.empty() || mEnsuredChildren.empty())
        return nullptr;

    for (int i = mEnsuredChildren.upperBound() ; i >= mEnsuredChildren.lowerBound() ; i--) {
        auto child = std::dynamic_pointer_cast<CoreComponent>(mChildren.at(i)->findComponentAtPosition(position));
        if (child != nullptr && child->isAttached() && !child->getCalculated(kPropertyBounds).getRect().isEmpty())
            return child;
    }

    return nullptr;
}

ComponentPtr
SequenceComponent::findDirectChildAtPosition(const Point& position) const
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
SequenceComponent::propDefSet() const
{
    static ComponentPropDefSet sSequenceComponentProperties(ScrollableComponent::propDefSet(), {
        {kPropertyHeight,           Dimension(),              asDimension,          kPropIn,    yn::setHeight, defaultSequenceHeight},
        {kPropertyWidth,            Dimension(),              asDimension,          kPropIn,    yn::setWidth,  defaultSequenceWidth},
        {kPropertyScrollDirection,  kScrollDirectionVertical, sScrollDirectionMap,  kPropInOut, yn::setScrollDirection},
        {kPropertySnap,             kSnapNone,                sSnapMap,             kPropInOut | kPropStyled },
        {kPropertyNumbered,         false,                    asBoolean,            kPropIn},
        {kPropertyOnScroll,         Object::EMPTY_ARRAY(),    asCommand,            kPropIn},
        {kPropertyFastScrollScale,  1.0,                      asNonNegativeNumber,  kPropInOut | kPropStyled },
    });

    return sSequenceComponentProperties;
}

Object
SequenceComponent::getValue() const {
    double scrollSize = getCalculated(kPropertyScrollDirection) == kScrollDirectionVertical
                        ? YGNodeLayoutGetHeight(mYGNodeRef)
                        : YGNodeLayoutGetWidth(mYGNodeRef);
    auto currentPosition = mCalculated.get(kPropertyScrollPosition).asNumber();
    return scrollSize != 0 ? currentPosition / scrollSize : 0;
}

ScrollType
SequenceComponent::scrollType() const
{
    return getCalculated(kPropertyScrollDirection) == kScrollDirectionVertical ? kScrollTypeVertical : kScrollTypeHorizontal;

}
Point
SequenceComponent::scrollPosition() const
{
    auto currentPosition = mCalculated.get(kPropertyScrollPosition).asNumber();
    return getCalculated(kPropertyScrollDirection) == kScrollDirectionVertical ?
           Point(0, currentPosition) :
           Point(currentPosition, 0);
}

Point
SequenceComponent::trimScroll(const Point& point) const
{
    // Break out early. If there are no children - no scrolling possible
    if (mChildren.empty())
        return Point();

    auto innerBounds = mCalculated.get(kPropertyInnerBounds).getRect();
    // We treat this component as 0 point of sequence. All calculation happens in relation to it.
    auto zeroAnchor = mChildren.at(mEnsuredChildren.empty() ? 0 : mEnsuredChildren.lowerBound());
    if (!zeroAnchor->isAttached()) zeroAnchor->ensureLayout(false);
    auto zeroAnchorPos = zeroAnchor->getCalculated(kPropertyBounds).getRect().getTopLeft() - innerBounds.getTopLeft();

    if (scrollType() == kScrollTypeVertical) {
        auto y = point.getY();
        // Cover in back direction. Current code ensures from index 0 to requested component and any updates
        // asynchronous so no way it will happen twice.
        while (y < zeroAnchorPos.getY() && !mEnsuredChildren.empty() && mEnsuredChildren.lowerBound() > 0) {
            mChildren.at(mEnsuredChildren.lowerBound() - 1)->ensureLayout(false);
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
            mChildren.at(mEnsuredChildren.lowerBound() - 1)->ensureLayout(false);
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
            child->ensureLayout(false);
            maxX = nonNegative(child->getCalculated(kPropertyBounds).getRect().getRight() - right);
            if (x <= maxX)
                return Point(x,0);
        }

        return Point(maxX, 0);
    }
}

const ComponentPropDefSet*
SequenceComponent::layoutPropDefSet() const
{
    static ComponentPropDefSet sSequenceChildProperties = ComponentPropDefSet().add({
        { kPropertyNumbering, kNumberingNormal, sNumberingMap,       kPropIn },
        { kPropertySpacing,   Dimension(0),     asAbsoluteDimension, kPropIn | kPropNeedsNode | kPropResetOnRemove, yn::setSpacing }
    });

    return &sSequenceChildProperties;
}

bool
SequenceComponent::getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator) {
    bool actionable = ScrollableComponent::getTags(outMap, allocator);
    if(!mChildren.empty()) {
        rapidjson::Value list(rapidjson::kObjectType);
        list.AddMember("itemCount", static_cast<int>(mChildren.size()), allocator);

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

bool
SequenceComponent::allowForward() const {
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
    double scrollSize = scrollType() == kScrollTypeVertical
                        ? innerBounds.getBottom()
                        : innerBounds.getRight();
    double innerScrollSize = scrollType() == kScrollTypeVertical
                             ? lastChildBounds.getBottom()
                             : lastChildBounds.getRight();
    return currentPosition + scrollSize < innerScrollSize;
}


std::map<int, float>
SequenceComponent::getChildrenVisibility(float realOpacity, const Rect &visibleRect) {
    std::map<int, float> visibleIndexes;
    bool visibleMet = false;

    if (mEnsuredChildren.empty()) {
        return visibleIndexes;
    }

    for (int index = mEnsuredChildren.lowerBound(); index <= mEnsuredChildren.upperBound(); index++) {
        const auto& child = getCoreChildAt(index);
        float childVisibility = child->calculateVisibility(realOpacity, visibleRect);
        if(childVisibility > 0.0) {
            visibleMet = true;
            visibleIndexes.emplace(index, childVisibility);
        }
        else if(visibleMet) {
            // Check if we have element outside of sequence viewport. If so - break out the loop.
            break;
        }
    }

    return visibleIndexes;
}

void
SequenceComponent::updateSeen() {
    // We don't always go from parent to child here (update case) so calculate opacity and visible rect recursively.
    auto visibleIndexes = getChildrenVisibility(calculateRealOpacity(), calculateVisibleRect());
    if(!visibleIndexes.empty()) {
        mIndexesSeen.expandTo(visibleIndexes.begin()->first);
        mIndexesSeen.expandTo(visibleIndexes.rbegin()->first);
    }
}

void
SequenceComponent::layoutChildIfRequired(
        const Rect& parentBounds, const CoreComponentPtr& child, size_t childIdx, bool useDirtyFlag) {
    if (!child->isAttached() || child->getCalculated(kPropertyBounds).empty()) {
        ensureChildAttached(child, childIdx);
        // Override standard behavior if child is not "really" first.
        if (childIdx > 0) {
            child->fixSpacing();
        }
        YGNodeCalculateLayout(
                mYGNodeRef,
                parentBounds.getWidth(),
                parentBounds.getHeight(),
                YGDirection::YGDirectionLTR);
        CoreComponent::processLayoutChanges(useDirtyFlag);
    }
}

void
SequenceComponent::processLayoutChanges(bool useDirtyFlag)
{
    // We need to account for padding as find function don't do that automatically.
    bool horizontal = scrollType() == kScrollTypeHorizontal;
    auto paddedScrollPosition = scrollPosition() + getCalculated(kPropertyInnerBounds).getRect().getTopLeft();
    auto anchor = std::static_pointer_cast<CoreComponent>(findDirectChildAtPosition(paddedScrollPosition));

    Rect oldAnchorBounds;
    if (anchor) {
        oldAnchorBounds = anchor->getCalculated(kPropertyBounds).getRect();
    }

    if (!mChildren.empty()) {
        // Reset very first child to not have spacing
        mChildren.at(0)->fixSpacing(true);

        // First ensured should not ignore spacing if it's not very first one overall
        if (mEnsuredChildren.lowerBound() > 0) {
            auto child = mChildren.at(mEnsuredChildren.lowerBound());
            child->fixSpacing();
        }
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

    auto sequenceBounds = mCalculated.get(kPropertyBounds).getRect();
    float childCache = mContext->getRootConfig().getSequenceChildCache();
    float pageSize = horizontal ? sequenceBounds.getWidth() : sequenceBounds.getHeight();
    float anchorPosition = horizontal ? oldAnchorBounds.getX() : oldAnchorBounds.getY();

    // Lay out children in positive direction until we hit cache limit.
    float distanceToCover = (childCache + 1) * pageSize + anchorPosition;
    int lastLoaded = mEnsuredChildren.lowerBound();
    for (; lastLoaded < mChildren.size(); lastLoaded++) {
        auto child = mChildren.at(lastLoaded);
        layoutChildIfRequired(sequenceBounds, child, lastLoaded, useDirtyFlag);
        auto childBounds = child->getCalculated(kPropertyBounds).getRect();
        float distance = horizontal ? childBounds.getRight() : childBounds.getBottom();
        if (distance >= distanceToCover) {
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
        layoutChildIfRequired(sequenceBounds, child, firstLoaded, useDirtyFlag);
        auto childBounds = child->getCalculated(kPropertyBounds).getRect();
        anchorBounds = anchor->getCalculated(kPropertyBounds).getRect();
        float distance = (horizontal ? anchorBounds.getLeft() : anchorBounds.getTop())
                       - (horizontal ? childBounds.getLeft() : childBounds.getTop());
        if (distance >= distanceToCover) {
            break;
        }
    }

    reportLoaded(std::max(firstLoaded, mEnsuredChildren.lowerBound()));

    if (anchor) {
        // Adjust scroll position if changed at the beginning of sequence
        if (anchorBounds != oldAnchorBounds) {
            auto currentPosition = getCalculated(kPropertyScrollPosition).asNumber();
            currentPosition += scrollType() == kScrollTypeHorizontal ? anchorBounds.getLeft() - oldAnchorBounds.getLeft()
                                                                     : anchorBounds.getTop() - oldAnchorBounds.getTop();
            mCalculated.set(kPropertyScrollPosition, Dimension(DimensionType::Absolute, currentPosition));
            setDirty(kPropertyScrollPosition);
        }
    }

    updateSeen();
}

bool
SequenceComponent::insertChild(const ComponentPtr& child, size_t index, bool useDirtyFlag)
{
    bool result = CoreComponent::insertChild(child, index, useDirtyFlag);
    if (result && !mIndexesSeen.empty() && (int)index <= mIndexesSeen.lowerBound()) {
        mIndexesSeen.shift(1);
    }
    return result;
}

void
SequenceComponent::removeChild(const CoreComponentPtr& child, size_t index, bool useDirtyFlag)
{
    CoreComponent::removeChild(child, index, useDirtyFlag);
    if (!mIndexesSeen.empty()) {
        if ((int)index < mIndexesSeen.lowerBound()) {
            mIndexesSeen.shift(-1);
        } else if (mIndexesSeen.contains(index)) {
            mIndexesSeen.remove(index);
        }
    }
}

bool
SequenceComponent::shouldAttachChildYogaNode(int index) const {
    // We can't predict the order in which new indexes attached. So just attach first one initially to have anchor
    // in place.
    return (mEnsuredChildren.empty() && index == 0);
}

float
SequenceComponent::maxScroll() const {
    if (mChildren.empty()) {
        return 0;
    }
    bool horizontal = scrollType() == kScrollTypeHorizontal;
    auto sequenceBounds = mCalculated.get(kPropertyInnerBounds).getRect();
    float end = horizontal ? sequenceBounds.getRight() : sequenceBounds.getBottom();
    auto lastChildBounds = mChildren.at(mEnsuredChildren.upperBound())->getCalculated(kPropertyBounds).getRect();
    float childEnd = horizontal ? lastChildBounds.getRight() : lastChildBounds.getBottom();
    return nonNegative(childEnd - end);
}

} // namespace apl
