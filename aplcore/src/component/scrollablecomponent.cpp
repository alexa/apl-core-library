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

#include "apl/component/scrollablecomponent.h"

#include "apl/action/scrollaction.h"
#include "apl/component/componentpropdef.h"
#include "apl/component/yogaproperties.h"
#include "apl/content/rootconfig.h"
#include "apl/focus/focusmanager.h"
#include "apl/time/sequencer.h"
#include "apl/time/timemanager.h"
#include "apl/touch/gestures/scrollgesture.h"
#include "apl/utils/stickychildrentree.h"
#ifdef SCENEGRAPH
#include "apl/scenegraph/builder.h"
#include "apl/scenegraph/scenegraph.h"
#endif // SCENEGRAPH

namespace apl {

ScrollableComponent::ScrollableComponent(const ContextPtr& context, Properties&& properties,
                                         const Path& path) :
    ActionableComponent(context, std::move(properties), path),
    mStickyTree(std::make_shared<StickyChildrenTree>(*this)) {}

std::shared_ptr<ScrollableComponent>
ScrollableComponent::cast(const std::shared_ptr<Component>& component) {
    return component && CoreComponent::cast(component)->scrollable()
               ? std::static_pointer_cast<ScrollableComponent>(component) : nullptr;
}

const ComponentPropDefSet&
ScrollableComponent::propDefSet() const
{
    static auto getScrollOffset = [](const CoreComponent& component) -> Object {
        return component.getCalculated(kPropertyScrollPosition);
    };

    static auto setScrollOffset = [](CoreComponent& component, const Object& value) -> void {
        ((ScrollableComponent&)component).setScrollPositionDirectly((float)value.asNumber());
    };

    static auto getScrollPercent = [](const CoreComponent& component) -> Object {
        return component.getValue();
    };

    static auto setScrollPercent = [](CoreComponent& component, const Object& value) -> void {
        float scrollSize;
        if (component.scrollType() == kScrollTypeHorizontal) {
            scrollSize = YGNodeLayoutGetWidth(component.getNode());
        } else {
            scrollSize = YGNodeLayoutGetHeight(component.getNode());
        }
        ((ScrollableComponent&)component).setScrollPositionDirectly(scrollSize * (float)value.asNumber());
    };

    static ComponentPropDefSet sScrollableComponentProperties(ActionableComponent::propDefSet(), {
        {kPropertyScrollOffset,   getScrollOffset,  setScrollOffset,     kPropDynamic | kPropSetAfterLayout },
        {kPropertyScrollPercent,  getScrollPercent, setScrollPercent,    kPropDynamic | kPropSetAfterLayout },
        {kPropertyScrollPosition, Dimension(0),     asAbsoluteDimension, kPropRuntimeState | kPropVisualContext},
    });
    return sScrollableComponentProperties;
}

bool
ScrollableComponent::setScrollPositionInternal(float value)
{
    auto currentPosition = mCalculated.get(kPropertyScrollPosition).asNumber();
    auto target = trimScroll(isHorizontal() ? Point(value, 0) : Point(0, value));
    auto trimmedValue = isHorizontal() ? target.getX() : target.getY();

    if (trimmedValue == currentPosition)
        return false;

    mCalculated.set(kPropertyScrollPosition, Dimension(DimensionType::Absolute, trimmedValue));
    onScrollPositionUpdated();

    // Only run the onScroll event handler once the component has been fully laid out.  This prevents the handler
    // from being run if the component was created with a scroll offset.
    auto onScrollHandler = getCalculated(kPropertyOnScroll);
    if (allowEventHandlers() && !onScrollHandler.empty()) {
        ContextPtr eventContext = createEventContext("Scroll");
        mContext->sequencer().executeCommands(onScrollHandler,
                                              eventContext,
                                              shared_from_corecomponent(),
                                              true);
    }
    return true;
}

void
ScrollableComponent::setScrollPositionDirectly(float value)
{
    mContext->sequencer().releaseResource({kExecutionResourcePosition, shared_from_this()});
    setScrollPositionInternal(value);
}

void
ScrollableComponent::update(UpdateType type, float value)
{
    if (type == kUpdateScrollPosition)
        setScrollPositionInternal(value);
    else
        CoreComponent::update(type, value);
}

const EventPropertyMap&
ScrollableComponent::eventPropertyMap() const
{
    static EventPropertyMap sScrollableEventProperties = eventPropertyMerge(
            CoreComponent::eventPropertyMap(),
            {
                    {"position", [](const CoreComponent *c) { return c->getValue(); }},
            });

    return sScrollableEventProperties;
}

bool
ScrollableComponent::getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator)
{
    bool actionable = CoreComponent::getTags(outMap, allocator);

    std::string direction = scrollType() == kScrollTypeHorizontal ? "horizontal" : "vertical";
    bool shouldAllowForward = allowForward();
    bool shouldAllowBackwards = allowBackwards();

    if(!mChildren.empty() && (shouldAllowBackwards || shouldAllowForward)) {
        rapidjson::Value scrollable(rapidjson::kObjectType);
        scrollable.AddMember("direction", rapidjson::Value(direction.c_str(), allocator).Move(), allocator);
        scrollable.AddMember("allowForward", shouldAllowForward, allocator);
        scrollable.AddMember("allowBackwards", shouldAllowBackwards, allocator);
        outMap.AddMember("scrollable", scrollable, allocator);

        actionable = true;
    }

    return actionable;
}

void
ScrollableComponent::initialize()
{
    ActionableComponent::initialize();
    mGestureHandlers.emplace_back(ScrollGesture::create(ScrollableComponent::cast(shared_from_this())));
}

void
ScrollableComponent::onScrollPositionUpdated()
{
    setVisualContextDirty();
    markDisplayedChildrenStale(true);
    setDirty(kPropertyScrollPosition);

    mStickyTree->updateStickyOffsets();
}

bool
ScrollableComponent::canConsumeFocusDirectionEvent(FocusDirection direction, bool fromInside)
{
    if (!fromInside) return true;
    return (direction == kFocusDirectionRight || direction == kFocusDirectionLeft) ||
           (direction == kFocusDirectionDown || direction == kFocusDirectionUp) ||
           (direction == kFocusDirectionBackwards && allowBackwards()) ||
           (direction == kFocusDirectionForward && allowForward());
}

bool
ScrollableComponent::canScroll(FocusDirection direction)
{
    auto sp = scrollType();
    bool isLTR = getCalculated(kPropertyLayoutDirection) == kLayoutDirectionLTR;
    bool horizontalBackwards = isLTR
              ? (direction == kFocusDirectionLeft && sp == kScrollTypeHorizontal)
              : (direction == kFocusDirectionRight && sp == kScrollTypeHorizontal);
    bool horizontalForwards = isLTR
              ? (direction == kFocusDirectionRight && sp == kScrollTypeHorizontal)
              : (direction == kFocusDirectionLeft && sp == kScrollTypeHorizontal);
    return (((direction == kFocusDirectionUp && sp == kScrollTypeVertical) || horizontalBackwards ||
             (direction == kFocusDirectionBackwards)) && allowBackwards()) ||
           (((direction == kFocusDirectionDown && sp == kScrollTypeVertical) || horizontalForwards ||
             (direction == kFocusDirectionForward)) && allowForward());
}

CoreComponentPtr
ScrollableComponent::takeFocusFromChild(FocusDirection direction, const Rect& origin)
{
    const auto& bounds = getCalculated(kPropertyBounds).get<Rect>();
    Rect offsetRect = origin;
    if (origin.empty()) {
        // If empty - we simulate entrance from edge opposite to movement direction.
        switch(direction) {
            case kFocusDirectionLeft:
                offsetRect = Rect(bounds.getWidth(), 0, 1, bounds.getHeight());
                break;
            case kFocusDirectionRight:
                offsetRect = Rect(-1, 0, 1, bounds.getHeight());
                break;
            case kFocusDirectionUp:
                offsetRect = Rect(0, bounds.getHeight(), bounds.getWidth(), 1);
                break;
            case kFocusDirectionDown:
                offsetRect = Rect(0, -1, bounds.getWidth(), 1);
                break;
            case kFocusDirectionForward:
            case kFocusDirectionBackwards:
                break;
            default:
                return nullptr;
        }
        offsetRect.offset(scrollPosition());
    }

    auto canTravel = canScroll(direction);
    auto scrollable = shared_from_corecomponent();
    if (canTravel) {
        auto next = getContext()->focusManager().find(direction, scrollable, offsetRect, scrollable);
        if (next) return next;
    }

    if (canTravel) {
        // Shift in %
        float targetShift = 0;
        bool horizontalForward = getCalculated(kPropertyLayoutDirection) == kLayoutDirectionLTR
                                     ? (scrollType() == kScrollTypeHorizontal && direction == kFocusDirectionRight)
                                     : (scrollType() == kScrollTypeHorizontal && direction == kFocusDirectionLeft);
        if (horizontalForward ||
            direction == kFocusDirectionDown ||
            direction == kFocusDirectionForward) {
            targetShift = 100;
        } else {
            targetShift = -100;
        }

        auto duration = getContext()->getRootConfig().getProperty(RootProperty::kScrollOnFocusDuration).getDouble();
        auto action = ScrollAction::make(
            getRootConfig().getTimeManager(),
            getContext(),
            scrollable,
            Dimension(DimensionType::Relative, targetShift),
            duration);
        mContext->sequencer().attachToSequencer(action, FOCUS_SEQUENCER);
        return scrollable;
    } else if (mParent && direction != kFocusDirectionForward && direction != kFocusDirectionBackwards) {
        auto focusRoot = FocusFinder::getImplicitFocusRoot(shared_from_corecomponent(), direction);
        Rect globalBounds;
        getBoundsInParent(focusRoot, globalBounds);
        if (origin.empty()) {
            offsetRect = globalBounds;
        } else {
            offsetRect = origin;
            offsetRect.offset(globalBounds.getTopLeft() - scrollPosition());
        }

        switch (direction) {
            case kFocusDirectionDown:
                offsetRect = Rect(offsetRect.getLeft(), globalBounds.getBottom() - 1, offsetRect.getWidth(), 1);
                break;
            case kFocusDirectionUp:
                offsetRect = Rect(offsetRect.getLeft(), globalBounds.getTop(), offsetRect.getWidth(), 1);
                break;
            case kFocusDirectionLeft:
                offsetRect = Rect(globalBounds.getLeft(), offsetRect.getTop(), 1, offsetRect.getHeight());
                break;
            case kFocusDirectionRight:
                offsetRect = Rect(globalBounds.getRight() - 1, offsetRect.getTop(), 1, offsetRect.getHeight());
            default:
                break;
        }

        return getContext()->focusManager().find(direction, scrollable, offsetRect, focusRoot);
    }

    return nullptr;
}

#ifdef SCENEGRAPH
sg::LayerPtr
ScrollableComponent::constructSceneGraphLayer(sg::SceneGraphUpdates& sceneGraph) {
    auto layer = ActionableComponent::constructSceneGraphLayer(sceneGraph);
    assert(layer);

    layer->setChildOffset(scrollPosition());
    return layer;
}

bool
ScrollableComponent::updateSceneGraphInternal(sg::SceneGraphUpdates& sceneGraph)
{
    if (isDirty(kPropertyScrollPosition)) {
        mSceneGraphLayer->setChildOffset(scrollPosition());
        sceneGraph.changed(mSceneGraphLayer);
    }

    return false;
}
#endif // SCENEGRAPH

} // namespace apl
