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

#ifndef _APL_SCROLLABLE_COMPONENT_H
#define _APL_SCROLLABLE_COMPONENT_H

#include "actionablecomponent.h"

namespace apl {

/**
 * Abstract class representing actionable content that can scroll.
 */
class ScrollableComponent : public ActionableComponent {
public:
    /**
     * Get offset from current scroll position that will bring scrollable into snap position according to configured rules.
     * @return Offset from scroll position to snap position.
     */
    virtual Point getSnapOffset() const { return {}; }

    /**
     * @return true if snap should happen regardless of external factors like gesture velocity or interaction rules.
     */
    virtual bool shouldForceSnap() const { return false; }

    void update(UpdateType type, float value) override;
    bool canConsumeFocusDirectionEvent(FocusDirection direction, bool fromInside) override;
    CoreComponentPtr takeFocusFromChild(FocusDirection direction, const Rect& origin) override;

    std::shared_ptr<StickyChildrenTree> getStickyTree() override { return mStickyTree; }

protected:
    ScrollableComponent(const ContextPtr& context, Properties&& properties, const Path& path);

    /// Core component overrides
    void initialize() override;
    const EventPropertyMap& eventPropertyMap() const override;
    bool getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator) override;
    bool scrollable() const override { return true; }
    const ComponentPropDefSet& propDefSet() const override;

    /**
     * Override this to calculate maximum available scroll position.
     * @return maximum available scroll position.
     */
    virtual float maxScroll() const = 0;

    /**
     * Called when the scroll position changes so that subclasses can respond with the appropriate state changes.
     */
    virtual void onScrollPositionUpdated();

    /**
     * This method directly changes the scroll position, usually as a result of a "SetValue" call.  It has the
     * side effect of killing any Scroll-related commands that are being handled in the sequencer.
     * @param value The target scroll offset (in dp)
     */
    void setScrollPositionDirectly(float value);

private:
    bool setScrollPositionInternal(float value);
    bool canScroll(FocusDirection direction);

    // A tree of the descendants of this scroll with position: sticky.
    std::shared_ptr<StickyChildrenTree> mStickyTree;
};

} // namespace apl

#endif //_APL_SCROLLABLE_COMPONENT_H
