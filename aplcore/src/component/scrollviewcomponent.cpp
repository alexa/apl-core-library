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

#include "apl/command/commandproperties.h"
#include "apl/command/scrollcommand.h"
#include "apl/component/componentpropdef.h"
#include "apl/component/scrollviewcomponent.h"
#include "apl/component/yogaproperties.h"
#include "apl/time/sequencer.h"

namespace apl {

CoreComponentPtr
ScrollViewComponent::create(const ContextPtr& context,
                            Properties&& properties,
                            const Path& path) {
    auto ptr = std::make_shared<ScrollViewComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

ScrollViewComponent::ScrollViewComponent(const ContextPtr& context,
                                         Properties&& properties,
                                         const Path& path)
        : ScrollableComponent(context, std::move(properties), path) {
    YGNodeStyleSetOverflow(mYGNodeRef, YGOverflowScroll);
}

const ComponentPropDefSet&
ScrollViewComponent::propDefSet() const {
    static ComponentPropDefSet sScrollViewComponentProperties(ScrollableComponent::propDefSet(), {
            {kPropertyOnScroll, Object::EMPTY_ARRAY(), asCommand, kPropIn}
    });

    return sScrollViewComponentProperties;
}

Object
ScrollViewComponent::getValue() const {
    auto height = YGNodeLayoutGetHeight(mYGNodeRef);
    auto currentPosition = mCalculated.get(kPropertyScrollPosition).asNumber();
    return height > 0 ? currentPosition / height : 0;
}

Point
ScrollViewComponent::scrollPosition() const
{
    auto currentPosition = mCalculated.get(kPropertyScrollPosition).asNumber();
    return Point(0, currentPosition);
}

Point
ScrollViewComponent::trimScroll(const Point& point) {
    auto y = point.getY();
    if (y <= 0 || mChildren.empty())
        return Point();

    float maxY = maxScroll();

    return Point(0, y <= maxY ? y : maxY);
}

float
ScrollViewComponent::maxScroll() const {
    float bottom = mCalculated.get(kPropertyInnerBounds).getRect().getBottom();
    if (mChildren.empty()) {
        return bottom;
    }
    auto child = mChildren.at(mChildren.size() - 1);
    return nonNegative(child->getCalculated(kPropertyBounds).getRect().getBottom() - bottom);
}

bool
ScrollViewComponent::allowForward() const {
    if (getChildCount() == 0)
        return false;

    auto child = getChildAt(0);
    auto innerScrollSize = child->getCalculated(kPropertyBounds).getRect().getHeight();
    auto scrollSize = mCalculated.get(kPropertyInnerBounds).getRect().getHeight();
    auto currentPosition = mCalculated.get(kPropertyScrollPosition).asNumber();
    return currentPosition + scrollSize < innerScrollSize;
}

bool
ScrollViewComponent::allowBackwards() const {
    auto currentPosition = mCalculated.get(kPropertyScrollPosition).asNumber();
    return (currentPosition > 0);
}

void
ScrollViewComponent::onScrollPositionUpdated() {
    ScrollableComponent::onScrollPositionUpdated();
    markDisplayedChildrenStale(true);

    if (getChildCount() > 0) {
        auto child = getCoreChildAt(0);
        child->markGlobalToLocalTransformStale();
    }
}

} // namespace apl
