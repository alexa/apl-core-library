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
#include "apl/component/containercomponent.h"
#include "apl/component/yogaproperties.h"

namespace apl {

CoreComponentPtr
ContainerComponent::create(const ContextPtr& context,
                           Properties&& properties,
                           const Path& path)
{
    auto ptr = std::make_shared<ContainerComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

ContainerComponent::ContainerComponent(const ContextPtr& context,
                                       Properties&& properties,
                                       const Path& path)
    : CoreComponent(context, std::move(properties), path)
{
}

void
ContainerComponent::adjustSpacing()
{
    if (!mChildren.empty()) {
        for (int i = 0; i < mChildren.size(); i++) {
            // Spacing is attached to "spaced out" child. First child is not spaces from anything.
            getCoreChildAt(i)->fixSpacing(i == 0);
        }
    }
}

void
ContainerComponent::handleLayoutDirectionChange(bool useDirtyFlag)
{
    adjustSpacing();
}

void
ContainerComponent::processLayoutChanges(bool useDirtyFlag, bool first)
{
    // TODO: FIX THIS - IT CAN CHANGE THE SIZE OF THE CONTAINER
    // Quite brute-forcy to do that, though it will handle any strange changes to layout direction in the future.
    adjustSpacing();

    CoreComponent::processLayoutChanges(useDirtyFlag, first);
}

const ComponentPropDefSet&
ContainerComponent::propDefSet() const
{
    static ComponentPropDefSet sContainerComponentProperties(CoreComponent::propDefSet(), {
        {kPropertyAlignItems,       kFlexboxAlignStretch,        sFlexboxAlignMap,          kPropIn | kPropStyled | kPropDynamic, yn::setAlignItems },
        {kPropertyDirection,        kContainerDirectionColumn,   sContainerDirectionMap,    kPropIn | kPropStyled | kPropDynamic, yn::setFlexDirection },
        {kPropertyJustifyContent,   kFlexboxJustifyContentStart, sFlexboxJustifyContentMap, kPropIn | kPropStyled | kPropDynamic, yn::setJustifyContent },
        {kPropertyNumbered,         false,                       asBoolean,                 kPropIn |
                                                                                            kPropVisualContext},
        {kPropertyWrap,             kFlexboxWrapNoWrap,          sFlexboxWrapMap,           kPropIn | kPropStyled | kPropDynamic, yn::setWrap },
    });

    return sContainerComponentProperties;
}

const ComponentPropDefSet*
ContainerComponent::layoutPropDefSet() const
{
    // TODO: Break these into two sets so we don't calculate everything all of the time.
    static ComponentPropDefSet sContainerChildProperties = ComponentPropDefSet().add({
         {kPropertyAlignSelf,     kFlexboxAlignAuto,                 sFlexboxAlignMap,    kPropIn | kPropStyled | kPropDynamic | kPropResetOnRemove, yn::setAlignSelf },
         {kPropertyBottom,        Dimension(DimensionType::Auto, 0), asDimension,         kPropIn | kPropStyled | kPropDynamic | kPropResetOnRemove, yn::setPosition<YGEdgeBottom> },
         {kPropertyEnd,           Object::NULL_OBJECT(),             asNonAutoDimension,  kPropIn | kPropStyled | kPropDynamic | kPropResetOnRemove, yn::setPosition<YGEdgeEnd> },
         {kPropertyGrow,          0,                                 asNonNegativeNumber, kPropIn | kPropStyled | kPropDynamic | kPropResetOnRemove, yn::setPropertyGrow },
         {kPropertyLeft,          Dimension(DimensionType::Auto, 0), asDimension,         kPropIn | kPropStyled | kPropDynamic | kPropResetOnRemove, yn::setPosition<YGEdgeLeft> },
         {kPropertyNumbering,     kNumberingNormal,                  sNumberingMap,       kPropIn },
         {kPropertyPosition,      kPositionRelative,                 sPositionMap,        kPropIn | kPropStyled | kPropDynamic | kPropResetOnRemove, yn::setPositionType },
         {kPropertyRight,         Dimension(DimensionType::Auto, 0), asDimension,         kPropIn | kPropStyled | kPropDynamic | kPropResetOnRemove, yn::setPosition<YGEdgeRight> },
         {kPropertyShrink,        0,                                 asNonNegativeNumber, kPropIn | kPropStyled | kPropDynamic | kPropResetOnRemove, yn::setPropertyShrink },
         {kPropertySpacing,       Dimension(0),                      asAbsoluteDimension, kPropIn | kPropStyled | kPropDynamic | kPropNeedsNode | kPropResetOnRemove, yn::setSpacing },
         {kPropertyStart,         Object::NULL_OBJECT(),             asNonAutoDimension,  kPropIn | kPropStyled | kPropDynamic | kPropResetOnRemove, yn::setPosition<YGEdgeStart> },
         {kPropertyTop,           Dimension(DimensionType::Auto, 0), asDimension,         kPropIn | kPropStyled | kPropDynamic | kPropResetOnRemove, yn::setPosition<YGEdgeTop> }
     });
    return &sContainerChildProperties;
}

std::map<int,int>
ContainerComponent::calculateChildrenVisualLayer(const std::map<int, float>& visibleIndexes,
                                                 const Rect& visibleRect, int visualLayer)
{
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