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
#include <math.h>

#include "apl/component/componentpropdef.h"
#include "apl/component/gridsequencecomponent.h"
#include "apl/component/yogaproperties.h"
#include "apl/content/rootconfig.h"

namespace apl {

CoreComponentPtr
GridSequenceComponent::create(const ContextPtr& context,
                          Properties&& properties,
                          const std::string& path)
{
    auto ptr = std::make_shared<GridSequenceComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

void
GridSequenceComponent::initialize() {
    MultiChildScrollableComponent::initialize();

    // Figure out if we need to adjust cross axis.
    mCrossAxisDimensionIsAuto = mCalculated[isHorizontal() ? kPropertyHeight : kPropertyWidth].isAutoDimension();
    if (mCrossAxisDimensionIsAuto) {
        adjustAutoCrossAxisSize();
    }
}

void
GridSequenceComponent::adjustAutoCrossAxisSize() {
    auto horizontal = isHorizontal();
    auto crossAxisProp = isHorizontal() ? kPropertyHeight : kPropertyWidth;
    float adjustedCrossAxisSize = 0.0f;

    auto crossAxisArray = horizontal ? getCalculated(kPropertyChildHeight).getArray()
                                     : getCalculated(kPropertyChildWidth).getArray();

    for (const auto& crossAxis : crossAxisArray) {
        auto d = crossAxis.asDimension(*mContext);

        if (d.isAbsolute()) {
            adjustedCrossAxisSize += d.getValue();
        }
    }

    auto adjustedCrossAxisDim = Dimension(DimensionType::Absolute, adjustedCrossAxisSize);
    mCalculated.set(crossAxisProp, adjustedCrossAxisDim);
    if (horizontal) {
        setHeight(adjustedCrossAxisDim);
    } else {
        setWidth(adjustedCrossAxisDim);
    }
}

/**
 * Grid sequence layout is quite special case. We want to figure out forced sizes of children before moving on with any
 * layout changes processing.
 */
void
GridSequenceComponent::processLayoutChanges(bool useDirtyFlag) {
    // Process parent sizing first to have info for child calculation.
    CoreComponent::processLayoutChanges(useDirtyFlag);

    //Calculate child sizes if needed.
    auto bounds = getCalculated(kPropertyBounds).getRect();
    if (mLastParentBounds != bounds) {
        mLastParentBounds = bounds;
        calculateAbsoluteChildSizes(bounds.getWidth(), bounds.getHeight());
        calculateItemsPerCourse();
    }

    // Process any lazy layouts.
    MultiChildScrollableComponent::processLayoutChanges(useDirtyFlag);
}

const ComponentPropDefSet&
GridSequenceComponent::propDefSet() const {
    static ComponentPropDefSet sSequenceComponentProperties(MultiChildScrollableComponent::propDefSet(), {
            {kPropertyChildHeight,     Object::EMPTY_ARRAY(),    asArray,             kPropIn|kPropRequired},
            {kPropertyChildWidth,      Object::EMPTY_ARRAY(),    asArray,             kPropIn|kPropRequired},
            {kPropertyScrollDirection, kScrollDirectionVertical, sScrollDirectionMap, kPropInOut|kPropVisualContext,
                                                                                      yn::setGridScrollDirection},
            {kPropertyWrap,            kFlexboxWrapWrap,         asInteger,           kPropOut,
                                                                                      yn::setWrap},
            {kPropertyItemsPerCourse,  0,                        asInteger,           kPropRuntimeState},
    });

    return sSequenceComponentProperties;
}

const ComponentPropDefSet*
GridSequenceComponent::layoutPropDefSet() const {
    static ComponentPropDefSet sGridChildProperties = ComponentPropDefSet().add( {
        { kPropertyNumbering, kNumberingNormal, sNumberingMap, kPropIn }
    });
    return &sGridChildProperties;
}

const EventPropertyMap&
GridSequenceComponent::eventPropertyMap() const
{
    static EventPropertyMap sGridEventProperties = eventPropertyMerge(
        MultiChildScrollableComponent::eventPropertyMap(),
            {
                {"itemsPerCourse", [](const CoreComponent *c) { return c->getCalculated(kPropertyItemsPerCourse); }},
            });

    return sGridEventProperties;
}

void GridSequenceComponent::layoutChildIfRequired(
        CoreComponentPtr& child,
        size_t childIdx,
        bool useDirtyFlag) {
    // We need to apply forced size before layout.
    applyChildSize(child, childIdx);
    MultiChildScrollableComponent::layoutChildIfRequired(child, childIdx, useDirtyFlag);
}

void GridSequenceComponent::calculateAbsoluteChildSizes(float gridWidth, float gridHeight) {
    if (isHorizontal()) {
        auto result = adjustChildDimensions(getCalculated(kPropertyChildWidth).getArray()[0].asDimension(*mContext),
            gridWidth,
            getCalculated(kPropertyChildHeight).getArray(),
            gridHeight);
        mAdjustedChildWidths.clear();
        mAdjustedChildWidths.emplace_back(result.first);
        mAdjustedChildHeights = result.second;
    } else {
        auto result = adjustChildDimensions(getCalculated(kPropertyChildHeight).getArray()[0].asDimension(*mContext),
            gridHeight,
            getCalculated(kPropertyChildWidth).getArray(),
            gridWidth);
        mAdjustedChildHeights.clear();
        mAdjustedChildHeights.emplace_back(result.first);
        mAdjustedChildWidths = result.second;
    }
}

std::pair<float, std::vector<float>>
GridSequenceComponent::adjustChildDimensions(
            const Dimension& transAxisChildDimension,
            float transAxisSize,
            const ObjectArray& crossAxisArray,
            float crossAxisSize) {

    assert(!crossAxisArray.empty());

    float adjustedTransAxiSize;
    std::vector<float> adjustedCrossAxisSizes;

    if (transAxisChildDimension.isAbsolute())
        adjustedTransAxiSize = transAxisChildDimension.getValue();
    else if (transAxisChildDimension.isRelative())
        adjustedTransAxiSize = (transAxisChildDimension.getValue() / 100.0) * transAxisSize;
    else // auto
        adjustedTransAxiSize = transAxisSize;

    float autoSizeBudget = crossAxisSize;
    int numAutos = 0;  // number of auto-sized components
    for (const auto& crossAxis : crossAxisArray) {
        auto d = crossAxis.asDimension(*mContext);
        auto adjustedSize = -1.0f;
        if (d.isAbsolute())
            adjustedSize = d.getValue();
        else if (mCrossAxisDimensionIsAuto)
            adjustedSize = 0;
        else if (d.isRelative()) {
            // We round percentages down to avoid viewhost rendering differences.
            // Also we clip items that don't fit (per spec).
            adjustedSize = std::min(std::floor(static_cast<float>(d.getValue()) / 100.0f * crossAxisSize),
                                    autoSizeBudget);
        } else if (d.isAuto()) {
            numAutos++;
        }
        adjustedCrossAxisSizes.emplace_back(adjustedSize);
        if (adjustedSize >= 0) {
            autoSizeBudget -= adjustedSize;
        }
    }

    // If only 1 relative or absolute child - we need to pad
    if (crossAxisArray.size() == 1) {
        auto d = crossAxisArray.at(0).asDimension(*mContext);
        if (d.isAbsolute() || d.isRelative()) {
            auto adjustedSize = adjustedCrossAxisSizes.at(0);
            if (adjustedSize > 0) {
                adjustedCrossAxisSizes.insert(adjustedCrossAxisSizes.end(), std::floor(autoSizeBudget/adjustedSize), adjustedSize);
                autoSizeBudget = 0;
            }
        }
    }

    // Fill in autos
    if (numAutos > 0) {
        // apply size budget to remaining components
        for (auto it = adjustedCrossAxisSizes.begin(); it != adjustedCrossAxisSizes.end(); ++it) {
            if (*it == -1) {
                float sizePerComponent = std::max(0.0f, std::round(autoSizeBudget / numAutos));
                *it = sizePerComponent;
                autoSizeBudget -= sizePerComponent;
                numAutos--;
            }
        }
    }

    return {adjustedTransAxiSize, std::move(adjustedCrossAxisSizes)};
}

void
GridSequenceComponent::calculateItemsPerCourse() {
    mItemsPerCourse = isHorizontal() ? mAdjustedChildHeights.size() : mAdjustedChildWidths.size();
    mCalculated.set(kPropertyItemsPerCourse, mItemsPerCourse);
}

void
GridSequenceComponent::applyChildSize(const CoreComponentPtr& coreChild, size_t index) const {
    auto horizontal = isHorizontal();

    size_t curRow = horizontal
            ? index % mItemsPerCourse
            : index / mItemsPerCourse;
    auto idx = std::min(curRow, mAdjustedChildHeights.size() - 1);
    coreChild->setHeight(mAdjustedChildHeights[idx]);

    size_t curColumn = horizontal
            ? index / mItemsPerCourse
            : index % mItemsPerCourse;
    idx = std::min(curColumn, mAdjustedChildWidths.size() - 1);
    coreChild->setWidth(mAdjustedChildWidths[idx]);
}

} // namespace apl
