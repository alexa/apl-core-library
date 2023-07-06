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

#ifndef _APL_PAGE_MOVE_HANDLER_H
#define _APL_PAGE_MOVE_HANDLER_H

#include "apl/component/component.h"

namespace apl {

class InterpolatedTransformation;
using ITPtr = std::shared_ptr<InterpolatedTransformation>;
class PagerComponent;
using PagerPtr = std::shared_ptr<PagerComponent>;

/**
 * The drawOrder property controls whether the current page or the next page will be displayed on top. Refer to
 * specification for ordering clarification.
 */
enum PageMoveDrawOrder {
    kPageMoveDrawOrderNextAbove,
    kPageMoveDrawOrderNextBelow,
    kPageMoveDrawOrderHigherAbove,
    kPageMoveDrawOrderHigherBelow,
};

/**
 * Container for pageMoveHandler definition. Here mainly for modularity purposes.
 */
class PageMoveHandler {
public:
    /**
     * Create PageMoveHandler.
     * @param component parent pager component.
     * @param array aray containing handlers definition. If null or empty - default handler will be
     * created.
     * @param swipeDirection swipe motion direction.
     * @param pageDirection paging direction.
     * @param currentPage current page index.
     * @param targetPage target page index.
     * @return created handler or nullptr if children don't exist.
     */
    static std::unique_ptr<PageMoveHandler> create(
            const CoreComponentPtr& component,
            const Object& array,
            SwipeDirection swipeDirection,
            PageDirection pageDirection,
            int currentPage,
            int targetPage);

    /**
     *  Constructors. Don't use directly, @see create.
     */
    PageMoveHandler(
            Object&& commands,
            PageMoveDrawOrder drawOrder,
            SwipeDirection swipeDirection,
            PageDirection pageDirection,
            const CoreComponentPtr& currentPage,
            const CoreComponentPtr& targetPage);
    PageMoveHandler(
            SwipeDirection swipeDirection,
            PageDirection pageDirection,
            const CoreComponentPtr& currentPage,
            const CoreComponentPtr& targetPage,
            ITPtr&& mCurrentPageTransform,
            ITPtr&& mTargetPageTransform);

    /**
     * @return true if default handler, false if custom.
     */
    bool isDefault() const { return mCommands.isNull(); }

    /**
     * Execute PageMoveHandler.
     * @param component parent pager component.
     * @param amount move amount.
     */
    void execute(const CoreComponentPtr& component, float amount);

    /**
     * Reset state of affected pages.
     */
    void reset();

    PageMoveDrawOrder getDrawOrder() const { return mDrawOrder; }
    std::weak_ptr<CoreComponent> getCurrentPage() const { return mCurrentPage; }
    SwipeDirection getSwipeDirection() const { return mSwipeDirection; }

    /**
     * Get the index of the target page within the supplied parent or -1 on failure. 
     *
     * @param component parent pager component
     *
     * @return the index in the parent component of -1 if the target page
     *         component is gone or not a direct child of the supplied parent
     */
    int getTargetPageIndex(const CoreComponentPtr& component) const;

    /**
     * Get the current page component, verified to (still) be a child of the specified pager.
     *
     * @param component parent pager component
     *
     * @return the current page or nullptr if the is no longer a current page that is a child of the
     * specified pager.
     */
    CoreComponentPtr getCheckedCurrentPage(const CoreComponentPtr& component) const;

    /**
     * Get the target page component, verified to (still) be a child of the specified pager.
     *
     * @param component parent pager component
     *
     * @return the target page or nullptr if the is no longer a target page that is a child of the
     * specified pager.
     */
    CoreComponentPtr getCheckedTargetPage(const CoreComponentPtr& component) const;

private:
    static ContextPtr createPageMoveContext(
            float amount,
            SwipeDirection direction,
            PageDirection pageDirection,
            const CoreComponentPtr& self,
            const CoreComponentPtr& currentChild,
            const CoreComponentPtr& nextChild);
    static ITPtr getPageTransformation(
            const PagerPtr& component,
            bool comeIn,
            bool fromLeft);
    void executeDefaultPagingAnimation(
            float amount,
            const CoreComponentPtr& currentChild,
            const CoreComponentPtr& nextChild);

    Object mCommands;
    PageMoveDrawOrder mDrawOrder;
    SwipeDirection mSwipeDirection;
    PageDirection  mPageDirection;

    std::weak_ptr<CoreComponent> mCurrentPage;
    std::weak_ptr<CoreComponent> mTargetPage;

    // Transforms for default animation handling
    ITPtr mCurrentPageTransform;
    ITPtr mTargetPageTransform;
};

}  // namespace apl

#endif //_APL_PAGE_MOVE_HANDLER_H
