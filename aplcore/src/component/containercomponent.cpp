/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include "apl/component/containercomponent.h"
#include "apl/component/yogaproperties.h"

namespace apl {

CoreComponentPtr
ContainerComponent::create(const ContextPtr& context,
                           Properties&& properties,
                           const std::string& path)
{
    auto ptr = std::make_shared<ContainerComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

ContainerComponent::ContainerComponent(const ContextPtr& context,
                                       Properties&& properties,
                                       const std::string& path)
    : CoreComponent(context, std::move(properties), path)
{
}

void
ContainerComponent::processLayoutChanges(bool useDirtyFlag) {
    // Quite brute-forcy to do that, though it will handle any strange changes to layout direction in the future.
    if (!mChildren.empty()) {
        for (int i = 0; i < mChildren.size(); i++) {
            getCoreChildAt(i)->fixSpacing(i == 0);
        }
    }

    CoreComponent::processLayoutChanges(useDirtyFlag);
}

const ComponentPropDefSet&
ContainerComponent::propDefSet() const
{
    static ComponentPropDefSet sContainerComponentProperties(CoreComponent::propDefSet(), {
        {kPropertyAlignItems,       kFlexboxAlignStretch,        sFlexboxAlignMap,          kPropIn, yn::setAlignItems },
        {kPropertyDirection,        kContainerDirectionColumn,   sContainerDirectionMap,    kPropIn, yn::setFlexDirection },
        {kPropertyJustifyContent,   kFlexboxJustifyContentStart, sFlexboxJustifyContentMap, kPropIn, yn::setJustifyContent },
        {kPropertyNumbered,         false,                       asBoolean,                 kPropIn },
        {kPropertyWrap,             kFlexboxWrapNoWrap,          sFlexboxWrapMap,           kPropIn, yn::setWrap },
    });

    return sContainerComponentProperties;
}

const ComponentPropDefSet*
ContainerComponent::layoutPropDefSet() const
{
    // TODO: Break these into two sets so we don't calculate everything all of the time.
    static ComponentPropDefSet sContainerChildProperties = ComponentPropDefSet().add({
         {kPropertyAlignSelf, kFlexboxAlignAuto,     sFlexboxAlignMap,    kPropIn | kPropResetOnRemove, yn::setAlignSelf },
         {kPropertyBottom,    Object::NULL_OBJECT(), asNonAutoDimension,  kPropIn | kPropResetOnRemove, yn::setPosition<YGEdgeBottom> },
         {kPropertyGrow,      0,                     asNonNegativeNumber, kPropIn | kPropResetOnRemove, yn::setPropertyGrow },
         {kPropertyLeft,      Object::NULL_OBJECT(), asNonAutoDimension,  kPropIn | kPropResetOnRemove, yn::setPosition<YGEdgeLeft> },
         {kPropertyNumbering, kNumberingNormal,      sNumberingMap,       kPropIn },
         {kPropertyPosition,  kPositionRelative,     sPositionMap,        kPropIn | kPropResetOnRemove, yn::setPositionType },
         {kPropertyRight,     Object::NULL_OBJECT(), asNonAutoDimension,  kPropIn | kPropResetOnRemove, yn::setPosition<YGEdgeRight> },
         {kPropertyShrink,    0,                     asNonNegativeNumber, kPropIn | kPropResetOnRemove, yn::setPropertyShrink },
         {kPropertySpacing,   Dimension(0),          asAbsoluteDimension, kPropIn | kPropNeedsNode | kPropResetOnRemove, yn::setSpacing },
         {kPropertyTop,       Object::NULL_OBJECT(), asNonAutoDimension,  kPropIn | kPropResetOnRemove, yn::setPosition<YGEdgeTop> }
     });
    return &sContainerChildProperties;
}

std::map<int,int>
ContainerComponent::calculateChildrenVisualLayer(const std::map<int, float>& visibleIndexes,
                                                 const Rect& visibleRect, int visualLayer) {
    std::map<int,int> result;

    if(visibleIndexes.empty()) {
        return result;
    }

    std::vector<int> indexes;
    indexes.reserve(visibleIndexes.size());
    for(auto const& vi : visibleIndexes) {
        indexes.emplace_back(vi.first);
        result.emplace(vi.first, visualLayer);
    }

    for(int i=0; i<indexes.size(); i++) {
        auto& childi = mChildren.at(indexes.at(i));
        for(int j=i+1; j<visibleIndexes.size(); j++) {
            auto childj = mChildren.at(indexes.at(j));
            bool intersects = childi->calculateVisibleRect(visibleRect)
                    .intersect(childj->calculateVisibleRect(visibleRect)).area() > 0;
            if(intersects) {
                result.at(indexes.at(j)) = result.at(indexes.at(i)) + 1;
            }
        }
    }

    return result;
}

} // namespace apl