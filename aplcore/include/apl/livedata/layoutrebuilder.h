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

#ifndef _APL_LAYOUT_REBUILDER_H
#define _APL_LAYOUT_REBUILDER_H

#include "apl/common.h"
#include "apl/utils/counter.h"
#include "apl/utils/noncopyable.h"
#include "apl/utils/path.h"
#include "apl/livedata/livedataobjectwatcher.h"

namespace apl {

class LiveArrayObject;
using LiveArrayObjectPtr = std::shared_ptr<LiveArrayObject>;

/**
 * A LayoutRebuilder updates and adjust the children of a Component if the "data" property
 * of the Component references a LiveArray.  The LayoutRebuilder is responsible for inserting
 * new Components as the array is extended, removing Components that no longer apply, and updating
 * Components that have a changed data set.
 */
class LayoutRebuilder : public std::enable_shared_from_this<LayoutRebuilder>,
                        public NonCopyable,
                        public Counter<LayoutRebuilder> {
public:
    /**
     * Construct a LayoutRebuilder and register it with the parent layout and LiveArray
     * @param context The data-binding context that children will be created with.
     * @param layout The parent layout Component
     * @param old old top component for case of reinflation
     * @param array The DynamicArray that is controlled by the LiveArray
     * @param items The list of items defined in the Component
     * @param childPath The provenance path for the children
     * @param numbered True if the items in the layout should be numbered
     * @return The LayoutRebuilder.
     */
    static std::shared_ptr<LayoutRebuilder> create(const ContextPtr& context,
                                                   const CoreComponentPtr& layout,
                                                   const CoreComponentPtr& old,
                                                   const LiveArrayObjectPtr& array,
                                                   const ObjectArray& items,
                                                   const Path& childPath,
                                                   bool numbered);

    /**
     * Internal constructor.  Do not call this directly; use the static create() method instead.
     */
    LayoutRebuilder(const ContextPtr& context,
                    const CoreComponentPtr& layout,
                    const CoreComponentPtr& old,
                    const LiveArrayObjectPtr& array,
                    const ObjectArray& items,
                    const Path& childPath,
                    bool numbered);

    /**
     * Standard destructor
     */
    ~LayoutRebuilder();

    /**
     * Populate the layout with the initial set of children
     * @param useDirtyFlag true to notify runtime about changes with dirty properties
     */
    void build(bool useDirtyFlag);

    /**
     * Repopulate the layout with children based on changes in the LiveArray
     */
    void rebuild();

    /**
     * Inflate child of component associated with rebuilder if not fully inflated already.
     * @param child child to inflate.
     */
    void inflateIfRequired(const CoreComponentPtr& child);

    /**
     * Notify rebuilder that particular data index is on screen.
     * @param idx
     */
    void notifyItemOnScreen(int idx);

    /**
     * Notify rebuilder that the start edge of existing range was reached,
     */
    void notifyStartEdgeReached();

    /**
     * Notify rebuilder that the end edge of existing range was reached,
     */
    void notifyEndEdgeReached();

    /**
     * Must be called after construction to inform the LayoutRebuilder that the layout has a firstItem or lastItem
     * @param hasFirstItem True if the layout has a "firstItem"
     * @param hasLastItem True if the layout has a "lastItem"
     */
    void setFirstLast(bool hasFirstItem, bool hasLastItem) {
        mHasFirstItem = hasFirstItem;
        mHasLastItem = hasLastItem;
    }

    /**
     * @return true if has first item, false otherwise.
     */
    bool hasFirstItem() const {
        return mHasFirstItem;
    }

    /**
     * @return true if has last item, false otherwise.
     */
    bool hasLastItem() const {
        return mHasLastItem;
    }

    /**
     * @return array that backs this Rebuilder. May be nullptr if array is lost by any reason.
     */
    std::shared_ptr<LiveArrayObject> getBackingArray() const { return mArray.lock(); }

private:
    ContextPtr buildBaseChildContext(const LiveArrayObjectPtr& array, size_t dataIndex, size_t insertIndex);

private:
    ContextPtr mContext;
    std::weak_ptr<CoreComponent> mLayout;
    std::weak_ptr<CoreComponent> mOld;
    std::weak_ptr<LiveArrayObject> mArray;
    const ObjectArray mItems;
    Path mChildPath;
    bool mNumbered;
    int mCallbackToken = -1;

    bool mHasFirstItem = false;
    bool mHasLastItem = false;

    int mRebuilderToken;  // Unique token generated for each LayoutRebuilder.  Helps find where a rebuilder is active.

    static int sRebuilderToken;
};

} // namespace apl

#endif // _APL_LAYOUT_REBUILDER_H
