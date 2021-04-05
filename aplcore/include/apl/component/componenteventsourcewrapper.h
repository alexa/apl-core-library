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

#ifndef _APL_COMPONENT_EVENT_SOURCE_WRAPPER_H
#define _APL_COMPONENT_EVENT_SOURCE_WRAPPER_H

#include "apl/component/componenteventwrapper.h"

namespace apl {

/**
 * Holds the "event.source" property defined in event handlers.  This extends the base class
 * and adds properties for "event.source.handler", "event.source.value", and "event.source.source".
 * Note that "event.source.source" is a backward-compatibility hack for APL 1.3 and earlier.
 *
 * The serialize method generates an appropriate JSON object for the "event.source" property.
 */
class ComponentEventSourceWrapper : public ComponentEventWrapper {
public:
    static std::shared_ptr<ComponentEventSourceWrapper> create(const std::shared_ptr<const CoreComponent>& component,
                                                      std::string handler,
                                                      const Object& value);

    explicit ComponentEventSourceWrapper(const std::shared_ptr<const CoreComponent>& component)
            : ComponentEventWrapper(component) {}

    std::string toDebugString() const override { return "ComponentEventSourceWrapper<>"; }

    Object get(const std::string& key) const override;
    Object opt(const std::string& key, const Object& def) const override;
    bool has(const std::string& key) const override;
    std::uint64_t size() const override;

    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const override;

    bool operator==(const ComponentEventWrapper& rhs) const override { return rhs == *this; };
    bool operator==(const ComponentEventSourceWrapper& rhs) const override {
        return mHandler == rhs.mHandler &&
               mValue == rhs.mValue &&
               !mComponent.owner_before(rhs.mComponent) &&
               !rhs.mComponent.owner_before(mComponent);
    }

private:
    std::string mHandler;
    Object mValue;
};

} // namespace apl

#endif //_APL_COMPONENT_EVENT_SOURCE_WRAPPER_H
