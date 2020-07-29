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

#ifndef _APL_COMPONENT_EVENT_TARGET_WRAPPER_H
#define _APL_COMPONENT_EVENT_TARGET_WRAPPER_H

#include "apl/component/componenteventwrapper.h"

namespace apl {

/**
 * Holds the "event.target" property defined in event handlers.  The serialize() method
 * in this class generates an appropriate object for the "event.target" property.
 *
 * The "event.target" properties are a subset of the "event.source" properties,
 * so the retrieval of "event.target" properties is delegated to the common parent class.
 */
class ComponentEventTargetWrapper : public ComponentEventWrapper {
public:
    static std::shared_ptr<ComponentEventTargetWrapper> create(const std::shared_ptr<const CoreComponent>& component);

    explicit ComponentEventTargetWrapper(const std::shared_ptr<const CoreComponent>& component)
            : ComponentEventWrapper(component) {}

    std::string toDebugString() const override { return "ComponentEventTargetWrapper<>"; }

    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const override;

    bool operator==(const ComponentEventWrapper& rhs) const override { return rhs == *this; };
    bool operator==(const ComponentEventTargetWrapper& rhs) const override {
        return !mComponent.owner_before(rhs.mComponent) && !rhs.mComponent.owner_before(mComponent);
    }
};

} // namespace apl

#endif // _APL_COMPONENT_EVENT_TARGET_WRAPPER_H
