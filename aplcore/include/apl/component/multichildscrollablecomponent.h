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
#include "apl/utils/range.h"

namespace apl {

/**
 * Alignment properties used preserving a MultiChildScrollableComponent scroll position
 * during reinflation.  The properties "firstId" and "firstIndex" use kScrollableAlignFirst;
 * the properties "centerId" and "centerIndex" use kScrollableAlignCenter.
 */
enum ScrollableAlign {
    kScrollableAlignFirst = 0,
    kScrollableAlignCenter = 1
};

/**
 * Abstract class for logic common to components that are both scrollable
 * and have multiple children.
 */
class MultiChildScrollableComponent : public ScrollableComponent {
public:
    MultiChildScrollableComponent(const ContextPtr& context, Properties&& properties, const Path& path) :
            ScrollableComponent(context, std::move(properties), path) {};
    Object getValue() const override;
    bool multiChild() const override { return true; }
    void processLayoutChanges(bool useDirtyFlag, bool first) override;
    void accept(Visitor<CoreComponent>& visitor) const override;
    void raccept(Visitor<CoreComponent>& visitor) const override;
    Point scrollPosition() const override;
    ScrollType scrollType() const override {
        return getCalculated(kPropertyScrollDirection) == kScrollDirectionVertical ?
            kScrollTypeVertical : kScrollTypeHorizontal;
    }
    Point trimScroll(const Point& point) override;

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

    /**
     * Scroll immediately to position this child appropriately.
     * @param child A direct child of the component
     * @param align alignment of the child
     * @param offset The offset
     */
    void setScrollPositionDirectlyByChild(const CoreComponentPtr& child, ScrollableAlign align, float offset );

    /// CoreComponent override.
    void ensureChildLayout(const CoreComponentPtr& child, bool useDirtyFlag) override;

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
    bool insertChild(const CoreComponentPtr& child, size_t index, bool useDirtyFlag) override;
    void removeChild(const CoreComponentPtr& child, size_t index, bool useDirtyFlag) override;
    bool getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator) override;
    virtual void layoutChildIfRequired(const CoreComponentPtr& child, size_t childIdx, bool useDirtyFlag, bool first);
    void relayoutInPlace(bool useDirtyFlag, bool first);
    float maxScroll() const override;
    bool shouldAttachChildYogaNode(int index) const override { return false; }
    bool shouldBeFullyInflated(int index) const override;

    const EventPropertyMap & eventPropertyMap() const override;
    void handlePropertyChange(const ComponentPropDef& def, const Object& value) override;

    void onScrollPositionUpdated() override;

    virtual size_t getItemsPerCourse() const { return 1; }

    virtual void ensureChildAttached(const CoreComponentPtr& child, int targetIdx);

    void attachYogaNode(const CoreComponentPtr& child) override;

    /**
     * Estimate number of children required to cover provided distance based on parameters of child provided.
     */
    virtual size_t estimateChildrenToCover(float distance, size_t baseChild) = 0;

private:
    /**
     * Ensure that current state of visibility parameters properly calculated.
     * It provides mechanism of lazy calculation to visibility related parameters.
     */
    void ensureChildrenVisibilityUpdated();

    /**
     * Calculate which child is the first visible child in the view and how far that child
     * has been shifted from the "ideal" position of having the top of the child exactly aligned
     * on the top of the innerBounds of the scrollable component.  The child shift value is calculated
     * as a percentage of the height/width of the child.
     * @return The first child or nullptr if no child is visible and the child shift percentage.
     */
    std::pair<CoreComponentPtr, float> getFirstChildInViewInternal() const;

    /**
     * Calculate the child closest to the center of the view or nullptr if no child is visible.
     * There is no guarantee that the child actually occupies the center of the view.
     * This method also returns how far that child has been shifted from the "ideal" position of having
     * the center of the child exactly aligned with the center of the innerBounds of the scrollable
     * component.  The child shift value is calculated as a percentage of the height/width of the child.
     * @return The center child or nullptr if no child is visible and the child shift percentage
     */
    std::pair<CoreComponentPtr, float> getCenterChildInViewInternal() const;

    float getSnapOffsetForChild(const ComponentPtr& child, Snap snap, float parentStart, float parentEnd) const;
    float childFractionOnScreenWithProposedScrollOffset(const ComponentPtr& child,
                                                        float scrollOffset) const;

    /**
     * Find child which center is closest to provided position. Search reduced to the current scrolling direction.
     * @param position position to measure from.
     * @return child pointer.
     */
    ComponentPtr findChildCloseToPosition(const Point& position) const;

    void attachYogaNodeIfRequired(const CoreComponentPtr& coreChild, int index) override;
    bool attachChild(const CoreComponentPtr& child, size_t index);
    void runLayoutHeuristics(size_t anchorIdx, float childCache, float pageSize, bool useDirtyFlag, bool first);
    void fixScrollPosition(const Rect& oldAnchorRect, const Rect& anchorRect);

private:
    Range mIndexesSeen;
    Range mEnsuredChildren;
    bool mChildrenVisibilityStale = false;

    // These cache variables are being used for event property calculation (lazy calculation)
    // and being calculated on layout or property changes.
    int mFirstChildInView = -1;
    int mFirstChildFullyInView = -1;
    int mLastChildFullyInView = -1;
    int mLastChildInView = -1;

    ActionPtr mDelayLayoutAction;
};
} // namespace apl

#endif //_APL_MULTICHILD_SCROLLABLE_COMPONENT_H
