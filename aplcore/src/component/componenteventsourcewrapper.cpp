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
ComponentEventSourceWrapper::create(const ConstCoreComponentPtr &component,
                                    std::string handler,
                                    const Object &value) {
    assert(!handler.empty());
    auto result = std::make_shared<ComponentEventSourceWrapper>(component);
    result->mHandler = handler;
    result->mValue = value;
    if (component)
        result->mSource = sComponentTypeBimap.at(component->getType());
    return result;
}

Object
ComponentEventSourceWrapper::get(const std::string& key) const
{
    if (key == "handler")
        return mHandler;

    if (key == "value")
        return mValue;

    if (key == "source")
        return mSource;

    return ComponentEventWrapper::get(key);
}

Object
ComponentEventSourceWrapper::opt(const std::string& key, const Object& def) const
{
    if (key == "handler")
        return mHandler.empty() ? def : mHandler;

    if (key == "value")
        return mHandler.empty() ? def : mValue;

    if (key == "source")
        return mSource.empty() ? def : mSource;

    return ComponentEventWrapper::opt(key, def);
}

bool
ComponentEventSourceWrapper::has(const std::string& key) const
{
    if (key == "handler" || key == "value" || key == "source")
        return true;

    return ComponentEventWrapper::has(key);
}

std::pair<std::string, Object>
ComponentEventSourceWrapper::keyAt(std::size_t offset) const
{
    auto basicSize = ComponentEventWrapper::size();
    if (offset < basicSize)
        return ComponentEventWrapper::keyAt(offset);

    offset -= basicSize;

    // Provide a consistent ordering of "value", "handler", and "source" properties.
    switch (offset) {
        case 0: return { "value", mValue };
        case 1: return { "handler", mHandler };
        case 2: return { "source", mSource };
        default: return { "", Object::NULL_OBJECT() };
    }
}

std::uint64_t
ComponentEventSourceWrapper::size() const
{
    // We add three properties to the size of the component wrapper.
    return ComponentEventWrapper::size() + 3;
}

rapidjson::Value
ComponentEventSourceWrapper::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    rapidjson::Value m(rapidjson::kObjectType);
    auto component = mComponent.lock();
    if (component) {
        component->serializeEvent(m, allocator);
        // Note that the source properties are assigned AFTER serialization.  This ensures we overwrite.
    }

    m.AddMember("value", mValue.serialize(allocator), allocator);
    m.AddMember("handler", rapidjson::Value(mHandler.c_str(), allocator), allocator);
    m.AddMember("source", rapidjson::Value(mSource.c_str(), allocator), allocator);

    return m;
}

} // namespace apl
