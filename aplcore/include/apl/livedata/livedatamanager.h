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

#ifndef _APL_LIVE_DATA_MANAGER_H
#define _APL_LIVE_DATA_MANAGER_H

#include <set>
#include <vector>

#include "apl/common.h"

namespace apl {

class LiveDataObject;

/**
 * A LiveDataManager is associated with a single RootContext and connects all live data
 * sources with the internal refresh logic.
 */
class LiveDataManager {
public:
    /**
     * Add a tracker to the manager
     * @param tracker The tracker to add
     */
    void add(const std::shared_ptr<LiveDataObject>& tracker) {
        mTrackers.emplace(tracker);
    }

    /**
     * Remove a tracker from the manager
     * @param tracker The tracker to remove
     */
    void remove(const std::shared_ptr<LiveDataObject>& tracker) {
        mTrackers.erase(mTrackers.find(tracker));
        mDirty.erase(mDirty.find(tracker));
    }

    /**
     * Mark a tracker as dirty
     * @param tracker The tracker to mark
     */
    void markDirty(const std::shared_ptr<LiveDataObject>& tracker) {
        mDirty.emplace(tracker);
    }

    /**
     * Flush all dirty changes associated with this data manager
     */
    void flushDirty();

    /**
     * @return The set of dirty trackers.
     */
    const SharedPtrSet<LiveDataObject>& dirty() const {
        return mDirty;
    }

    /**
     * @return The set of trackers
     */
    const SharedPtrSet<LiveDataObject>& trackers() const {
        return mTrackers;
    }

private:
    SharedPtrSet<LiveDataObject> mTrackers;
    SharedPtrSet<LiveDataObject> mDirty;
};

} // namespace apl

#endif // _APL_LIVE_DATA_MANAGER_H
