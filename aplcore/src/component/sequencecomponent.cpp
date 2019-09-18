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
    : ScrollableComponent(context, std::move(properties), path),
      mHighestIndexSeen(-1),
      mFirstUnensuredChild(0)
{
}

void
SequenceComponent::update(UpdateType type, float value)
{
    ScrollableComponent::update(type, value);
    if (type == kUpdateScrollPosition) {
        updateSeen();
    }
}

ComponentPtr
SequenceComponent::findChildAtPosition(const Point& position) const
{
    // Not all children may be ensured.  We start at the end of the list of ensured children
    // TODO: Given the serial nature of layout in a Sequence, a binary search would be faster

    if (mChildren.empty())
        return nullptr;

    for (int i = mFirstUnensuredChild - 1 ; i >= 0 ; i-- ) {
        auto child = mChildren.at(i)->findComponentAtPosition(position);
        if (child != nullptr)
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
    return scrollSize != 0 ? mCurrentPosition / scrollSize : 0;
}

ScrollType
SequenceComponent::scrollType() const
{
    return getCalculated(kPropertyScrollDirection) == kScrollDirectionVertical ? kScrollTypeVertical : kScrollTypeHorizontal;

}
Point
SequenceComponent::scrollPosition() const
{
    return getCalculated(kPropertyScrollDirection) == kScrollDirectionVertical ?
           Point(0, mCurrentPosition) :
           Point(mCurrentPosition, 0);
}

Point
SequenceComponent::trimScroll(const Point& point) const
{
    auto innerBounds = mCalculated.get(kPropertyInnerBounds).getRect();

    if (scrollType() == kScrollTypeVertical) {
        auto y = point.getY();
        if (y <= 0)
            return Point();

        float bottom = innerBounds.getBottom();
        float maxY = 0;

        // Ensure children until they cover the sequence.
        int startingChild = std::max(mFirstUnensuredChild - 1, 0);
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
        if (x <= 0)
            return Point();

        float right = innerBounds.getRight();
        float maxX = 0;

        // Ensure children until they cover the sequence.
        int startingChild = std::max(mFirstUnensuredChild - 1, 0);
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

        updateSeen();
        auto lowestOrdinalSeen = INT_MAX;
        auto highestOrdinalSeen = 0;

        for(int i = 0; i<= mHighestIndexSeen; i++) {
            auto ordinal = mChildren.at(i)->getContext()->opt("ordinal");
            if(ordinal.isNull())
                continue;

            highestOrdinalSeen = std::max(ordinal.asInt(), highestOrdinalSeen);
            lowestOrdinalSeen = std::min(ordinal.asInt(), lowestOrdinalSeen);
        }

        list.AddMember("lowestIndexSeen", 0, allocator);
        list.AddMember("highestIndexSeen", mHighestIndexSeen >= 0 ? mHighestIndexSeen : 0, allocator);

        if (getCalculated(kPropertyNumbered).truthy() && lowestOrdinalSeen <= highestOrdinalSeen) {
            list.AddMember("lowestOrdinalSeen", lowestOrdinalSeen, allocator);
            list.AddMember("highestOrdinalSeen", highestOrdinalSeen, allocator);
        }

        outMap.AddMember("list", list, allocator);
    }
    return actionable;
}

bool
SequenceComponent::allowForward() const {
    if(getChildCount() == 0)
        return false;
    // If the last element has not had ensureLayout called on it,
    // then the viewhost still has room to scroll.
    if(mFirstUnensuredChild < getChildCount())
        return true;

    // otherwise get the last child and calculate the bounds of
    // all children
    auto lastChild = getChildAt(getChildCount() - 1);
    auto lastChildBounds = lastChild->getCalculated(kPropertyBounds).getRect();
    auto innerBounds = mCalculated.get(kPropertyInnerBounds).getRect();
    double scrollSize = scrollType() == kScrollTypeVertical
                        ? innerBounds.getBottom()
                        : innerBounds.getRight();
    double innerScrollSize = scrollType() == kScrollTypeVertical
                             ? lastChildBounds.getBottom()
                             : lastChildBounds.getRight();
    return mCurrentPosition + scrollSize < innerScrollSize;
}


std::map<int, float>
SequenceComponent::getChildrenVisibility(float realOpacity, const Rect &visibleRect) {
    std::map<int, float> visibleIndexes;
    bool visibleMet = false;

    for (int index = 0; index < mFirstUnensuredChild; index++) {
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
        mHighestIndexSeen = std::max(visibleIndexes.rbegin()->first, mHighestIndexSeen);
    }
}

void
SequenceComponent::ensureChildAttached(const ComponentPtr& child)
{
    CoreComponent::ensureChildAttached(child);
    auto it = std::find(mChildren.begin(), mChildren.end(), child);
    mFirstUnensuredChild = std::distance(mChildren.begin(), it) + 1;
}

} // namespace apl
