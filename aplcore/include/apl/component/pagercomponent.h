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

#ifndef _APL_PAGER_COMPONENT_H
#define _APL_PAGER_COMPONENT_H

#include "actionablecomponent.h"
#include "apl/engine/event.h"

namespace apl {

class InterpolatedTransformation;
class ActionRef;
class PageMoveHandler;

class PagerComponent : public ActionableComponent {
public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const Path& path);
    PagerComponent(const ContextPtr& context, Properties&& properties, const Path& path);

    ComponentType getType() const override { return kComponentTypePager; };

    /// Component overrides.
    void initialize() override;
    void update(UpdateType type, float value) override;
    const ComponentPropDefSet* layoutPropDefSet() const override;
    Object getValue() const override { return getCalculated(kPropertyCurrentPage); }
    ScrollType scrollType() const override;
    PageDirection pageDirection() const override;
    int pagePosition() const override { return getCalculated(kPropertyCurrentPage).asInt(); }
    bool getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator) override;
    void processLayoutChanges(bool useDirtyFlag, bool first) override;
    bool allowForward() const override;
    bool allowBackwards() const override;
    void release() override;

    /// Actionable overrides
    bool isHorizontal() const override { return scrollType() == kScrollTypeHorizontalPager; }
    bool isVertical() const override { return scrollType() == kScrollTypeVerticalPager; }
    bool canConsumeFocusDirectionEvent(FocusDirection direction, bool fromInside) override;
    CoreComponentPtr takeFocusFromChild(FocusDirection direction, const Rect& origin) override;
    bool shouldBeFullyInflated(int index) const override { return false; }

    /**
     * Command page switch helper function.
     * @param context Execution context.
     * @param target Target component (needs to be pager).
     * @param index page to switch to. Should be in valid [0; mChildren.size()) range.
     * @param direction Paging direction.
     * @param ref Action reference to resolve after switch completed.
     * @param skipDefaultAnimation if set to true no page switch animation will be performed in
     * case if no custom processing was assigned with handlePageMove.
     */
    static void setPageUtil(const ContextPtr& context, const ComponentPtr& target, int index,
        PageDirection direction, const ActionRef& ref, bool skipDefaultAnimation = false);

    /**
     * Start page move process. Set initial states. Should be followed by executePageMove and endPageMove.
     * @param direction page move direction.
     * @param currentPage page to switch from.
     * @param targetPage page to switch to.
     */
    void startPageMove(PageDirection direction, int currentPage, int targetPage);

    /**
     * Execute step of page move.
     * @param amount move amount from 0-1.
     */
    void executePageMove(float amount);

    /**
     * End page move. Set new page states, reset transformations and intectionability.
     * @param fulfilled true if switch should happen, false if pager should stay on current page.
     * @param ref action to resolve when required handlers resolved.
     * @param fast true if executed from command, false for user interaction.
     */
    void endPageMove(bool fulfilled, const ActionRef& ref = ActionRef(nullptr), bool fast = true);

protected:
    const ComponentPropDefSet& propDefSet() const override;
    const EventPropertyMap & eventPropertyMap() const override;
    void accept(Visitor<CoreComponent>& visitor) const override;
    void raccept(Visitor<CoreComponent>& visitor) const override;
    bool insertChild(const CoreComponentPtr& child, size_t index, bool useDirtyFlag) override;
    void removeChild(const CoreComponentPtr& child, size_t index, bool useDirtyFlag) override;
    bool shouldAttachChildYogaNode(int index) const override;
    void finalizePopulate() override;
    void ensureDisplayedChildren() override;

private:
    bool multiChild() const override { return true; }
    void attachYogaNodeIfRequired(const CoreComponentPtr& coreChild, int index) override {};

    std::map<int, float> getChildrenVisibility(float realOpacity, const Rect &visibleRect) const override;
    void attachPageAndReportLoaded(int page);
    ActionPtr executePageChangeEvent(bool fast);
    void setPage(int page);
    void setPageImmediate(int pageIndex);
    void handleSetPage(int index, PageDirection direction, const ActionRef& ref, bool skipDefaultAnimation);
    PageDirection focusDirectionToPage(FocusDirection direction);

    void reportLoadedInternal(size_t index);

    ActionPtr mCurrentAnimation;
    ActionPtr mDelayLayoutAction;
    std::unique_ptr<PageMoveHandler> mPageMoveHandler;
};

} // namespace apl

#endif //_APL_PAGER_COMPONENT_H
