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

#ifndef _APL_COMPONENT_EVENT_WRAPPER_H
#define _APL_COMPONENT_EVENT_WRAPPER_H

#include "apl/common.h"
#include "apl/primitives/objectdata.h"

namespace apl {

class ComponentEventSourceWrapper;
class ComponentEventTargetWrapper;

/**
 * This class holds a weak reference to a CoreComponent and retrieves object properties that are
 * exposed by the "event.source" or "event.target" bindings in data-binding contexts.
 *
 * All components a set of event properties as returned by CoreComponent::eventPropertyMap().
 * The methods in this object retrieve those properties from the referenced Corecomponent.
 */
class ComponentEventWrapper : public ObjectData {
public:
    explicit ComponentEventWrapper(const std::shared_ptr<const CoreComponent>& component);

    Object get(const std::string& key) const override;
    Object opt(const std::string& key, const Object& def) const override;
    bool has(const std::string& key) const override;
    std::uint64_t size() const override;

    const ObjectMap& getMap() const override;

    std::shared_ptr<const CoreComponent> getComponent() const { return mComponent.lock(); }

    virtual bool operator==(const ComponentEventWrapper& rhs) const = 0;
    virtual bool operator==(const ComponentEventSourceWrapper& rhs) const { return false; }
    virtual bool operator==(const ComponentEventTargetWrapper& rhs) const { return false; }

protected:
    std::weak_ptr<const CoreComponent> mComponent;
};

} // namespace apl

#endif // _APL_COMPONENT_EVENT_WRAPPER_H
