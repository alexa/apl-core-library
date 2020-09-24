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
#ifndef _APL_MULTICHILD_SCROLLABLE_COMPONENT_H
#define _APL_MULTICHILD_SCROLLABLE_COMPONENT_H

#include "apl/component/scrollablecomponent.h"

namespace apl {

/**
 * Abstract class for logic common to components that are both scrollable
 * and have multiple children.
 */
class MultiChildScrollableComponent : public ScrollableComponent {
public:
    MultiChildScrollableComponent(const ContextPtr& context, Properties&& properties, const std::string& path) :
            ScrollableComponent(context, std::move(properties), path) {};
    Object getValue() const override;
    bool multiChild() const override { return true; }
    void processLayoutChanges(bool useDirtyFlag) override;
    void accept(Visitor<CoreComponent>& visitor) const override;
    void raccept(Visitor<CoreComponent>& visitor) const override;
    Point scrollPosition() const override;
    ScrollType scrollType() const override {
        return getCalculated(kPropertyScrollDirection) == kScrollDirectionVertical ?
            kScrollTypeVertical : kScrollTypeHorizontal;
    }
    Point trimScroll(const Point& point) const override;
    void update(UpdateType type, float value) override;

protected:
    /**
     * Finds the immediate child, if any, at the given position.
     *
     * @param position Point to test for a child.
     * @return Component Pointer to an immediate child, or null.
     */
    ComponentPtr findDirectChildAtPosition(const Point& position) const;

    /**
     * Some components may need to apply adjustment logic to child spacing.
     * Override and return true to apply spacing fixes on layout changes in case if it's supported.
     * @return True to apply spacing fixes, otherwise false.
     */
    virtual bool childrenUseSpacingProperty() const = 0;

    bool allowBackwards() const override;
    bool allowForward() const override;
    const ComponentPropDefSet& propDefSet() const override;
    std::map<int, float> getChildrenVisibility(float realOpacity, const Rect &visibleRect) const override;
    bool insertChild(const ComponentPtr& child, size_t index, bool useDirtyFlag) override;
    void removeChild(const CoreComponentPtr& child, size_t index, bool useDirtyFlag) override;
    bool getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator) override;
    virtual void layoutChildIfRequired(const Rect& parentBounds, CoreComponentPtr& child, size_t childIdx, bool useDirtyFlag);
    float maxScroll() const override;
    bool shouldAttachChildYogaNode(int index) const override;

    bool isHorizontal() const { return getCalculated(kPropertyScrollDirection) == kScrollDirectionHorizontal; }
    bool isVertical() const { return getCalculated(kPropertyScrollDirection) == kScrollDirectionVertical; }

    const EventPropertyMap & eventPropertyMap() const override;

private:
    void updateChildrenVisibility();

    Range mIndexesSeen;
    int mFirstChildInView = -1;
    int mFirstChildFullyInView = -1;
    int mLastChildFullyInView = -1;
    int mLastChildInView = -1;
};
} // namespace apl

#endif //_APL_MULTICHILD_SCROLLABLE_COMPONENT_H
