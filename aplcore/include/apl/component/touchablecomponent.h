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

#ifndef _APL_TOUCHABLE_COMPONENT_H
#define _APL_TOUCHABLE_COMPONENT_H

#include "apl/component/actionablecomponent.h"

namespace apl {

class Gesture;

/**
 * Touchable components can be interacted with by pointers, e.g. mice and touches and then invoke handlers defined for the
 * component.
 */
class TouchableComponent : public ActionableComponent {
public:

    /**
     * Executes a given handler by name with a specific position.
     * @param handlerKey The handler to execute.
     * @param point The Point at which the event that triggered this was created, in component
     *              coordinates.
     * @return Whether or not the event was handled.
     */
    bool executePointerEventHandler(PropertyKey handlerKey, const Point& point);

    /**
     * Executes a given handler by name with a specific position.
     * @param handlerKey The handler to execute.
     * @param event The pointer event.
     * @return Whether or not the event was handled.
     */
    bool executePointerEventHandler(PropertyKey handlerKey, const PointerEvent& event);

    /**
     * Get the touch event specific properties
     * @param point Properties of the component segment of the event, in component coordinates.
     * @return The event data-binding context.
     */
    virtual ObjectMapPtr createTouchEventProperties(const Point& point) const;

    /**
     * Get the touch event specific properties
     * @param event The pointer event.
     * @return The event data-binding context.
     */
    virtual ObjectMapPtr createTouchEventProperties(const PointerEvent& event) const;

    /**
     * Execute a given handler in specified mode with any additional parameters required.
     * @param event Event handler name.
     * @param commands Array of commands to execute.
     * @param fastMode true for fast mode, false for normal.
     * @param optional Optional key->value map to include on event context.
     */
    void executeEventHandler(const std::string& event, const Object& commands, bool fastMode = true,
                             const ObjectMapPtr& optional = nullptr);

    /**
     * @return A boolean representing the checked state.
     */
    Object getValue() const override;

    /**
     * Pass event to any gesture requiring it.
     * @param event pointer event.
     * @param timestamp event timestamp.
     * @return true if event is locked in gesture. False otherwise.
     */
    bool processGesture(const PointerEvent& event, apl_time_t timestamp);

    /**
     * Disable gestures processing. Some of the events could lead to gestures not having any sense any more.
     */
    void disableGestures() { mGesturesDisabled = true; }

    /// CoreComponent override s
    void release() override;
    void initialize() override;
    bool executeIntrinsicKeyHandlers(KeyHandlerType type, const ObjectMapPtr& keyboard) override;

protected:
    TouchableComponent(const ContextPtr &context, Properties &&properties, const std::string &path) :
        ActionableComponent(context, std::move(properties), path), mGesturesDisabled(false) {}

    const ComponentPropDefSet &propDefSet() const override;
    bool getTags(rapidjson::Value &outMap, rapidjson::Document::AllocatorType &allocator) override;
    bool isTouchable() const override { return true; }
    // override to be removed once we support handing intrinsic/reserved keys
    void update(UpdateType type, float value) override;

private:
    void executePreEventActions(PropertyKey handlerKey);
    void executePostEventActions(PropertyKey handlerKey, const Point &point);
    void addHandlerEventProperties(PropertyKey handlerKey,
                                   const Point& point,
                                   const ObjectMapPtr& eventProperties) const;

    void handleEnterKey(PropertyKey type);

    void setGestureHandlers();

    std::vector<std::shared_ptr<Gesture>> mGestureHandlers;
    std::shared_ptr<Gesture> mActiveGesture;
    bool mGesturesDisabled;
};
} // namespace apl

#endif //_APL_TOUCHABLE_COMPONENT_H
