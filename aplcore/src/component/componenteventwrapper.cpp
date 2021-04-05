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

#include "apl/component/componenteventwrapper.h"
#include "apl/component/corecomponent.h"

namespace apl {

ComponentEventWrapper::ComponentEventWrapper(const std::shared_ptr<const CoreComponent>& component)
    : mComponent(component)
{
}

Object
ComponentEventWrapper::get(const std::string& key) const
{
    auto component = mComponent.lock();
    if (component)
        return component->getEventProperty(key).second;

    return Object::NULL_OBJECT();
}

Object
ComponentEventWrapper::opt(const std::string& key, const Object& def) const
{
    auto component = mComponent.lock();
    if (component) {
        auto result = component->getEventProperty(key);
        if (result.first)
            return result.second;
    }

    return def;
}

bool
ComponentEventWrapper::has(const std::string& key) const
{
    auto component = mComponent.lock();
    if (component)
        return component->getEventProperty(key).first;

    return false;
}

std::uint64_t
ComponentEventWrapper::size() const
{
    auto component = mComponent.lock();
    if (component)
        return component->getEventPropertySize();

    return 0;
}

// This routine returns an empty map to avoid crashing the system in complicated ways
const ObjectMap&
ComponentEventWrapper::getMap() const
{
    static auto empty = ObjectMap();
    return empty;
}

}  // apl
