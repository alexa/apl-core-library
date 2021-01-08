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

#ifndef _APL_ACTIONABLE_COMPONENT_H
#define _APL_ACTIONABLE_COMPONENT_H

#include "corecomponent.h"

namespace apl {

class Gesture;

/**
 * An actionable component is one that accepts interaction focus.
 */
class ActionableComponent : public CoreComponent {
public:
    /**
     * Disable gestures processing. Some of the events could lead to gestures not having any sense,
     * or require to pause processing for time of animation.
     */
    void disableGestures() { mGesturesDisabled = true; }

    /**
     * Enable gestures processing. State of all supported gestures will be reset.
     */
    void enableGestures();

    /**
     * Get the touch event specific properties
     * @param point Properties of the component segment of the event.
     * @return The event data-binding context.
     */
    virtual ObjectMapPtr createTouchEventProperties(const Point& localPoint) const;

    /**
     * @return true if component supports horizontal movement.
     */
    virtual bool isHorizontal() const { return false; }

    /**
     * @return true if component supports vertical movement.
     */
    virtual bool isVertical() const { return false; }

    /// CoreComponent overrides
    bool isActionable() const override { return true; }
    bool isTouchable() const override;

protected:
    bool isFocusable() const override { return true; }
    void executeOnBlur() override;
    void executeOnFocus() override;
    bool executeKeyHandlers(KeyHandlerType type, const ObjectMapPtr& keyboard)  override;
    const ComponentPropDefSet& propDefSet() const override;
    void release() override;

    ActionableComponent(const ContextPtr& context, Properties&& properties, const std::string& path) :
            CoreComponent(context, std::move(properties), path), mGesturesDisabled(false) {}

    bool processGestures(const PointerEvent& event, apl_time_t timestamp, bool topComponent) override;
    void invokeStandardAccessibilityAction(const std::string& name) override;

    std::vector<std::shared_ptr<Gesture>> mGestureHandlers;
    std::shared_ptr<Gesture> mActiveGesture;

private:
    bool mGesturesDisabled;
};

} // namespace apl

#endif // _APL_ACTIONABLE_COMPONENT_H
