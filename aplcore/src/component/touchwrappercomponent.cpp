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

#include "apl/component/componentpropdef.h"
#include "apl/component/touchwrappercomponent.h"
#include "apl/component/yogaproperties.h"
#include "apl/primitives/keyboard.h"
#include "apl/time/sequencer.h"

namespace apl {

CoreComponentPtr
TouchWrapperComponent::create(const ContextPtr& context,
                              Properties&& properties,
                              const Path& path) {
    auto ptr = std::make_shared<TouchWrapperComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

TouchWrapperComponent::TouchWrapperComponent(const ContextPtr& context,
                                             Properties&& properties,
                                             const Path& path)
        : TouchableComponent(context, std::move(properties), path) {
}

std::shared_ptr<TouchWrapperComponent>
TouchWrapperComponent::cast(const std::shared_ptr<Component>& component) {
    return component && component->getType() == ComponentType::kComponentTypeTouchWrapper
        ? std::static_pointer_cast<TouchWrapperComponent>(component) : nullptr;
}

const ComponentPropDefSet&
TouchWrapperComponent::propDefSet() const {
    static ComponentPropDefSet sTouchWrapperComponentProperties(TouchableComponent::propDefSet());
    return sTouchWrapperComponentProperties;
}

void
TouchWrapperComponent::injectReplaceComponent(const CoreComponentPtr& child, bool above) {
    const auto& bounds = getCalculated(kPropertyInnerBounds).get<Rect>();

    insertChild(child, above ? 1 : 0, true);

    yn::setPositionType(child->getNode(), static_cast<int>(kPositionAbsolute), *mContext);
    yn::setPosition<YGEdgeLeft>(child->getNode(), bounds.getLeft(), *mContext);
    yn::setPosition<YGEdgeTop>(child->getNode(), bounds.getTop(), *mContext);
    yn::setWidth(child->getNode(), bounds.getWidth(), *mContext);
    yn::setHeight(child->getNode(), bounds.getHeight(), *mContext);
}


void
TouchWrapperComponent::resetChildPositionType() {
    if (!mChildren.empty()) {
        // TouchWrapper only supports relative positioning, we force absolute and inner bounds
        // coordinates to position overlay in case of Swipe/Replace
        yn::setPositionType(getCoreChildAt(0)->getNode(), static_cast<int>(kPositionRelative), *mContext);
        yn::setPosition<YGEdgeLeft>(getCoreChildAt(0)->getNode(), 0, *mContext);
        yn::setPosition<YGEdgeTop>(getCoreChildAt(0)->getNode(), 0, *mContext);
    }
}

}  // namespace apl