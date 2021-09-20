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

#include "apl/component/component.h"
#include "apl/engine/context.h"
#include "apl/utils/log.h"

namespace apl {

// Start a little offset to catch errors
id_type Component::sUniqueIdGenerator = 1000;

Component::Component(const ContextPtr& context, const std::string& id)
    : mContext(context),
      mUniqueId(':'+std::to_string(Component::sUniqueIdGenerator++)),
      mId(id)
{
}

std::string
Component::name() const
{
    return sComponentTypeBimap.at(getType());
}


void
Component::updateMediaState(const MediaState& state, bool fromEvent)
{
    LOG(LogLevel::kError) << "updateMediaState called for component that does not support it.";
}

bool
Component::updateGraphic(const GraphicContentPtr& json) {
    LOG(LogLevel::kError) << "updateGraphic called for component that does not support it.";
    return false;
}

void
Component::updateResourceState(const ExtensionComponentResourceState& state)
{
    LOG(LogLevel::kError) << "updateResourceState called for component that does not support it.";
}

void
Component::clearDirty() {
    mDirty.clear();
    // Clean out changed children property.
    auto changedChildren = mCalculated.get(kPropertyNotifyChildrenChanged);
    if (!changedChildren.empty() && changedChildren.isArray()) {
        changedChildren.getMutableArray().clear();
    }
}

bool
Component::getBoundsInParent(const ComponentPtr& ancestor, Rect& out) const
{
    out = mCalculated.get(kPropertyBounds).getRect();

    ComponentPtr parent = getParent();
    while (parent && parent != ancestor) {
        out.offset(parent->getCalculated(kPropertyBounds).getRect().getTopLeft() - parent->scrollPosition());
        parent = parent->getParent();
    }
    return parent == ancestor;
}

std::string
Component::toDebugString() const
{
    std::string result ="Component<" + name() + mUniqueId;
    if (!mId.empty())
        result += "(" + mId + ")";

    for (const auto& m : mCalculated) {
        if (m.second.empty())
            continue;

        result += std::string(" ") + sComponentPropertyBimap.at(m.first) + ":" + m.second.toDebugString();
    }
    result += ">";
    return result;
}

std::string
Component::toDebugSimpleString() const
{
    std::string result ="Component<" + name() + mUniqueId;
    if (!mId.empty())
        result += "(" + mId + ")";
    result += ">";
    return result;
}

bool
Component::isCharacterValid(const wchar_t wc) const {
    LOG(LogLevel::kError) << "isCharacterValid called for component that does not support it.";
    return false;
}

streamer&
operator<<(streamer& os, const Component& component)
{
    os << component.toDebugString();
    return os;
}

ComponentPtr
Component::inflateChildAt(const rapidjson::Value& component, size_t index)
{
    auto childPtr =  mContext->inflate(component);
    if(childPtr) {
        return insertChild(childPtr, index) ? childPtr : nullptr;
    }
    return nullptr;
}


}  // namespace apl