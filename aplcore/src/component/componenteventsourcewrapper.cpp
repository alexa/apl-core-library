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

#include "apl/component/componenteventsourcewrapper.h"
#include "apl/component/corecomponent.h"

namespace apl {

std::shared_ptr<ComponentEventSourceWrapper>
ComponentEventSourceWrapper::create(const std::shared_ptr<const CoreComponent> &component,
                                       std::string handler,
                                       const Object &value) {
    auto result = std::make_shared<ComponentEventSourceWrapper>(component);
    result->mHandler = handler;
    result->mValue = value;
    return result;
}

Object
ComponentEventSourceWrapper::get(const std::string& key) const
{
    if (key == "handler")
        return mHandler;

    if (key == "value")
        return mValue;

    if (key == "source") {
        auto component = mComponent.lock();
        if (component)
            return sComponentTypeBimap.at(component->getType());
        return "";
    }

    return ComponentEventWrapper::get(key);
}

Object
ComponentEventSourceWrapper::opt(const std::string& key, const Object& def) const
{
    if (key == "handler")
        return mHandler.empty() ? def : mHandler;

    if (key == "value")
        return mHandler.empty() ? def : mValue;

    if (key == "source") {
        auto component = mComponent.lock();
        if (component)
            return sComponentTypeBimap.at(component->getType());
        return def;
    }

    return ComponentEventWrapper::opt(key, def);
}

bool
ComponentEventSourceWrapper::has(const std::string& key) const
{
    if (key == "handler")
        return !mHandler.empty();

    if (key == "value")
        return true;

    if (key == "source")
        return mComponent.lock() != nullptr;

    return ComponentEventWrapper::has(key);
}

std::uint64_t
ComponentEventSourceWrapper::size() const
{
    // The number of properties in the source wrapper will be size of the parent class, one
    // for the "value" property, and one each for the "handler" and "source" properties if
    // they are present.
    std::uint64_t result = ComponentEventWrapper::size() + 1;   // The "value" property is always present.

    if (!mHandler.empty())  // The "handler" property may be not be set
        result += 1;

    if (mComponent.lock() != nullptr)  // The "source" property may not be available.
        result += 1;

    return result;
}

rapidjson::Value
ComponentEventSourceWrapper::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    rapidjson::Value m(rapidjson::kObjectType);
    auto component = mComponent.lock();
    if (component) {
        component->serializeEvent(m, allocator);
        // Note that the source properties are assigned AFTER serialization.  This ensures we overwrite.
        m.AddMember("source", rapidjson::StringRef(sComponentTypeBimap.at(component->getType()).c_str()), allocator);
    }

    m.AddMember("value", mValue.serialize(allocator), allocator);
    if (!mHandler.empty())
        m.AddMember("handler", rapidjson::Value(mHandler.c_str(), allocator), allocator);

    return m;
}

} // namespace apl
