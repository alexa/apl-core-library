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

#ifndef _APL_YOGA_NODE_H
#define _APL_YOGA_NODE_H

#include <cmath>

#include "apl/component/componentproperties.h"
#include "apl/component/textmeasurement.h"
#include "apl/primitives/size.h"
#include "apl/utils/streamer.h"
#include "apl/yoga/yogaconfig.h"

namespace apl {

// Should be aligned with YGUndefined
#define YogaUndefined NAN

class CoreComponent;

/** "Dirtied" function definition. */
typedef void (*DirtiedFunc)(CoreComponent* component);

/**
 * Layout Edge. Should align with YGEdge.
 */
enum class Edge {
    Left = 0,
    Top = 1,
    Right = 2,
    Bottom = 3,
    Start = 4,
    End = 5,
    Horizontal = 6,
    Vertical = 7,
    All = 8
};

namespace yn {
/**
 * @param value Float value.
 * @return true if considered undefined by Yoga, false otherwise.
 * @note Should be aligned with YGFloatIsUndefined
 */
static inline bool isYogaUndefined(float value) { return std::isnan(value); }
};

/**
 * Encapsulated representation of YGNode. Generally a pass-through for Yoga calls, which does not
 * require any Yoga-specific types to be exposed outside of its implementation.
 */
class YogaNode : public NonCopyable {
public:
    explicit YogaNode(const YogaConfig& config);
    ~YogaNode();

    bool isValid() const { return mNode != nullptr; }

    void setComponent(CoreComponent *component);

    void setNodeTypeDefault();
    void setNodeTypeText();
    void insertChild(const YogaNode& child, uint32_t index);
    void removeChild(const YogaNode& child);
    void markDirty();
    void calculateLayout(float ownerWidth, float ownerHeight, LayoutDirection ownerDirection);
    bool isDirty() const;

    void setDirtiedFunc(DirtiedFunc dirtiedFunc);
    void setMeasureFunc();
    void setBaselineFunc();

    bool hasMeasureFunc() const;
    bool hasDirtiedFunc() const;

    bool hasOwner() const;
    YogaNode* getParent() const;
    YogaNode* getChild(uint32_t index) const;

    CoreComponent* getComponent() const;

    void setPropertyGrow(float value);
    void setPropertyShrink(float value);
    void setPositionType(Position value);
    void setWidth(float value);
    void setWidthPercent(float value);
    void setWidthAuto();
    void setMinWidth(float value);
    void setMinWidthPercent(float value);
    void setMaxWidth(float value);
    void setMaxWidthPercent(float value);
    void setHeight(float value);
    void setHeightPercent(float value);
    void setHeightAuto();
    void setMinHeight(float value);
    void setMinHeightPercent(float value);
    void setMaxHeight(float value);
    void setMaxHeightPercent(float value);
    void setPadding(Edge edge, float value);
    void setPaddingPercent(Edge edge, float value);
    void setBorder(Edge edge, float value);
    void setPosition(Edge edge, float value);
    void setPositionPercent(Edge edge, float value);
    void setFlexDirection(ContainerDirection value);
    void setJustifyContent(FlexboxJustifyContent value);
    void setWrap(FlexboxWrap value);
    void setAlignSelf(FlexboxAlign value);
    void setAlignItems(FlexboxAlign value);
    void setScrollDirection(ScrollDirection value);
    void setGridScrollDirection(ScrollDirection value);
    void setDisplay(Display value);
    void setLayoutDirection(LayoutDirection value);
    void setMargin(Edge edge, float margin);
    void setOverflowScroll();
    void setSpacing(float value, bool skip0);

    float getBorder(Edge edge) const;
    float getPadding(Edge edge) const;
    float getMargin(Edge edge) const;
    float getWidth() const;
    bool isAutoWidth() const;
    bool isAbsoluteWidth() const;
    float getMinWidth() const;
    float getMaxWidth() const;
    float getHeight() const;
    bool isAutoHeight() const;
    bool isAbsoluteHeight() const;
    float getMinHeight() const;
    float getMaxHeight() const;
    float getLeft() const;
    float getTop() const;
    LayoutDirection getLayoutDirection() const;
    ContainerDirection getFlexDirection() const;

    std::string toDebugString() const;
    bool operator==(YogaNode& that) const { return mNode == that.mNode; }

    // Required for testing
    void* get() const { return mNode; }

protected:
    friend streamer& operator<<(streamer&, const YogaNode&);

private:
    void* mNode = nullptr;
    CoreComponent* mComponent = nullptr;
    DirtiedFunc mDirtiedFunc = nullptr;
};

} // namespace apl


#endif // _APL_YOGA_PROPERTIES_H
