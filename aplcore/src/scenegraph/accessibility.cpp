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

#include "apl/scenegraph/accessibility.h"

#include "apl/primitives/object.h"

namespace apl {
namespace sg {

void
Accessibility::executeCallback(const std::string& name) const
{
    if (mActionCallback)
        mActionCallback(name);
}

bool
Accessibility::setLabel(const std::string& label)
{
    if (label == mLabel)
        return false;

    mLabel = label;
    return true;
}

bool
Accessibility::setRole(Role role)
{
    if (role == mRole)
        return false;

    mRole = role;
    return true;
}

void
Accessibility::appendAction(const std::string& name, const std::string& label, bool enabled)
{
    mActions.emplace_back(Action{name, label, enabled});
}

bool
Accessibility::setAdjustableValue(const std::string& adjustableValue) {
    if (adjustableValue == mAdjustableValue)
        return false;

    mAdjustableValue = adjustableValue;
    return true;
}

bool
Accessibility::setAdjustableRange(const apl::Object& object) {
    if (object.isNull())
        return false;
    mAdjustableRange = AdjustableRange{object.get("minValue").asNumber(), object.get("maxValue").asNumber(), object.get("currentValue").asNumber()};
    return true;
}

rapidjson::Value
Accessibility::serialize(rapidjson::Document::AllocatorType& allocator) const {
    auto out = rapidjson::Value(rapidjson::kObjectType);
    out.AddMember("label", rapidjson::Value(mLabel.c_str(), allocator), allocator);
    out.AddMember("role", rapidjson::Value(sRoleMap.at(mRole).c_str(), allocator), allocator);

    auto actionArray = rapidjson::Value(rapidjson::kArrayType);
    for (const auto& action : mActions) {
        auto actionItem = rapidjson::Value(rapidjson::kObjectType);
        actionItem.AddMember("name", rapidjson::Value(action.name.c_str(), allocator), allocator);
        actionItem.AddMember("label", rapidjson::Value(action.label.c_str(), allocator), allocator);
        actionItem.AddMember("enabled", rapidjson::Value(action.enabled), allocator);
        actionArray.PushBack(actionItem, allocator);
    }
    out.AddMember("actions", actionArray, allocator);
    auto range = rapidjson::Value(rapidjson::kObjectType);
    range.AddMember("minValue", rapidjson::Value(mAdjustableRange.minValue), allocator);
    range.AddMember("maxValue", rapidjson::Value(mAdjustableRange.maxValue), allocator);
    range.AddMember("currentValue", rapidjson::Value(mAdjustableRange.currentValue), allocator);
    out.AddMember("adjustableRange", range, allocator);
    out.AddMember("adjustableValue", rapidjson::Value(mAdjustableValue.c_str(), allocator), allocator);

    return out;
}

} // namespace sg
} // namespace apl