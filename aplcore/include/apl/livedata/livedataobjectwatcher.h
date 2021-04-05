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

#ifndef _APL_LIVE_DATA_OBJECT_OBSERVER_H
#define _APL_LIVE_DATA_OBJECT_OBSERVER_H

#include "apl/common.h"
#include "apl/primitives/object.h"

namespace apl {

/**
 * Simple LiveData watcher that gets notified when registered objects flushed.
 */
class LiveDataObjectWatcher {
public:
    virtual ~LiveDataObjectWatcher();

    void registerObjectWatcher(const std::shared_ptr<LiveDataObject>& object);

protected:
    virtual void liveDataObjectFlushed(const std::string& key, LiveDataObject& liveDataObject) = 0;

private:
    std::vector<std::pair<int, std::weak_ptr<LiveDataObject>>> mWatches;
};

} // namespace apl

#endif // _APL_LIVE_DATA_OBJECT_OBSERVER_H
