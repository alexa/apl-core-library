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

Component::Component(const ContextPtr& context, const std::string& id)
    : UIDObject(context, UIDObject::UIDObjectType::COMPONENT),
      mId(id)
{
}

std::string
Component::name() const
{
    return sComponentTypeBimap.at(getType());
}

bool
Component::updateGraphic(const GraphicContentPtr& json) {
    LOG(LogLevel::kError).session(mContext) << "updateGraphic called for component that does not support it.";
    return false;
}

void
Component::updateResourceState(const ExtensionComponentResourceState& state, int errorCode, const std::string& error)
{
    LOG(LogLevel::kError).session(mContext) << "updateResourceState called for component that does not support it.";
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

const Object&
Component::getCalculated(apl::PropertyKey key) const
{
    if (mFlags.isSet(kComponentFlagIsReleased)) {
        LOG(LogLevel::kWarn) << "Trying to access released component: " << getUniqueId()
                             << ", property: " << sComponentPropertyBimap.at(key);
        return Object::NULL_OBJECT();
    }

    return mCalculated.get(key);
}

bool
Component::getBoundsInParent(const ComponentPtr& ancestor, Rect& out) const
{
    out = mCalculated.get(kPropertyBounds).get<Rect>();

    ComponentPtr parent = getParent();
    while (parent && parent != ancestor) {
        out.offset(parent->getCalculated(kPropertyBounds).get<Rect>().getTopLeft() - parent->scrollPosition());
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
    LOG(LogLevel::kError).session(mContext) << "isCharacterValid called for component that does not support it.";
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