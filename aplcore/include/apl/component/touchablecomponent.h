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
     */
    void executePointerEventHandler(PropertyKey handlerKey, const Point& point) override;

    /**
     * @return A boolean representing the checked state.
     */
    Object getValue() const override;

    /// CoreComponent overrides
    void initialize() override;
    bool executeIntrinsicKeyHandlers(KeyHandlerType type, const Keyboard& keyboard) override;

protected:
    TouchableComponent(const ContextPtr &context, Properties &&properties, const Path& path) :
        ActionableComponent(context, std::move(properties), path) {}

    const ComponentPropDefSet &propDefSet() const override;
    bool getTags(rapidjson::Value &outMap, rapidjson::Document::AllocatorType &allocator) override;
    bool isTouchable() const override { return true; }
    // TODO: override to be removed once we support handing intrinsic/reserved keys
    void update(UpdateType type, float value) override;
    void setGestureHandlers();
    PointerCaptureStatus processPointerEvent(const PointerEvent& event, apl_time_t timestamp) override;
    void invokeStandardAccessibilityAction(const std::string& name) override;

private:
    void executePreEventActions(PropertyKey handlerKey);
    void executePostEventActions(PropertyKey handlerKey, const Point &point);
    void addHandlerEventProperties(PropertyKey handlerKey,
                                   const Point& point,
                                   const ObjectMapPtr& eventProperties) const;

    void handleEnterKey(PropertyKey type);

    /// Track if pointer started in this component for purpose of handling onPress handler. It's different from pressed
    /// state.
    bool mReceivedOnDown = false;
};
} // namespace apl

#endif //_APL_TOUCHABLE_COMPONENT_H
