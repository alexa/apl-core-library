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

    /// Scrollable overrides
    bool isHorizontal() const override { return getCalculated(kPropertyScrollDirection) == kScrollDirectionHorizontal; }
    bool isVertical() const override { return getCalculated(kPropertyScrollDirection) == kScrollDirectionVertical; }
    Point getSnapOffset() const override;
    bool shouldForceSnap() const override;

    /**
     * @return first visible child index
     */
    int getFirstChildInView() const {
        const_cast<MultiChildScrollableComponent*>(this)->ensureChildrenVisibilityUpdated();
        return mFirstChildInView;
    }

    /**
     * @return first fully visible child index
     */
    int getFirstChildFullyInView() const {
        const_cast<MultiChildScrollableComponent*>(this)->ensureChildrenVisibilityUpdated();
        return mFirstChildFullyInView;
    }

    /**
     * @return last fully visible child index
     */
    int getLastChildFullyInView() const {
        const_cast<MultiChildScrollableComponent*>(this)->ensureChildrenVisibilityUpdated();
        return mLastChildFullyInView;
    }

    /**
     * @return last visible child index
     */
    int getLastChildInView() const {
        const_cast<MultiChildScrollableComponent*>(this)->ensureChildrenVisibilityUpdated();
        return mLastChildInView;
    }

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
    virtual void layoutChildIfRequired(CoreComponentPtr& child, size_t childIdx, bool useDirtyFlag);
    float maxScroll() const override;
    bool shouldAttachChildYogaNode(int index) const override;

    const EventPropertyMap & eventPropertyMap() const override;
    void handlePropertyChange(const ComponentPropDef& def, const Object& value) override;

    void onScrollPositionUpdated() override;

    virtual size_t getItemsPerCourse() const { return 1; }

private:
    Range mIndexesSeen;
    bool mChildrenVisibilityStale = false;

    // These cache variables are being used for event property calculation (lazy calculation)
    // and being calculated on layout or property changes.
    int mFirstChildInView = -1;
    int mFirstChildFullyInView = -1;
    int mLastChildFullyInView = -1;
    int mLastChildInView = -1;

    /**
     * Ensure that current state of visibility parameters properly calculated.
     * It provides mechanism of lazy calculation to visibility related parameters.
     */
    void ensureChildrenVisibilityUpdated();

    float getSnapOffsetForChild(const ComponentPtr& child, Snap snap, float parentStart, float parentEnd) const;
    float childFractionOnScreenWithProposedScrollOffset(const ComponentPtr& child,
                                                        float scrollOffset) const;
};
} // namespace apl

#endif //_APL_MULTICHILD_SCROLLABLE_COMPONENT_H
