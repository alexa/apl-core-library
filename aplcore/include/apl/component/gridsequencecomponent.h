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

#ifndef _APL_GRIDSEQUENCE_COMPONENT_H
#define _APL_GRIDSEQUENCE_COMPONENT_H

#include <yoga/YGNode.h>

#include "apl/component/multichildscrollablecomponent.h"

namespace apl {

class GridSequenceComponent : public MultiChildScrollableComponent {
public:

    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const Path& path);
    GridSequenceComponent(const ContextPtr& context,
                          Properties&& properties,
                          const Path& path)
            : MultiChildScrollableComponent(context, std::move(properties), path),
              mItemsPerCourse(0),
              mCrossAxisDimensionIsAuto(false) {}

    ComponentType getType() const override { return kComponentTypeGridSequence; };
    void initialize() override;
    void processLayoutChanges(bool useDirtyFlag, bool first) override;

protected:
    const ComponentPropDefSet& propDefSet() const override;
    const ComponentPropDefSet* layoutPropDefSet() const override;
    void layoutChildIfRequired(const CoreComponentPtr& child,
                               size_t childIdx,
                               bool useDirtyFlag,
                               bool first) override;
    void ensureChildAttached(const CoreComponentPtr& child, int targetIdx) override;
    const EventPropertyMap & eventPropertyMap() const override;

    void handlePropertyChange(const ComponentPropDef& def, const Object& value) override;

    bool childrenUseSpacingProperty() const override { return false; }

    size_t getItemsPerCourse() const override { return mItemsPerCourse; }

    size_t estimateChildrenToCover(float distance, size_t baseChild) override;

private:
    std::pair<float, std::vector<float>> adjustChildDimensions(
        const Dimension& transAxisChildDimension,
        float transAxisSize,
        const ObjectArray& crossAxisArray,
        float crossAxisSize);

    Size getChildSize(size_t index) const;
    void applyChildSize(const CoreComponentPtr& coreChild, size_t index) const;
    void calculateAbsoluteChildSizes(float gridWidth, float gridHeight);
    void calculateItemsPerCourse();
    void adjustAutoCrossAxisSize();

    std::vector<float> mAdjustedChildHeights;
    std::vector<float> mAdjustedChildWidths;

    int mItemsPerCourse; /// Number of Rows for horizontal scroll, number of Columns for vertical scroll
    bool mCrossAxisDimensionIsAuto; /// Flag to identify that "auto" size was used on cross axis.

    /// Used to check if child sizes recalculation required.
    Object mLastChildHeight;
    Object mLastChildWidth;
    Rect mLastParentBounds;
};

} // namespace apl

#endif //_APL_GRID_SEQUENCE_COMPONENT_H
