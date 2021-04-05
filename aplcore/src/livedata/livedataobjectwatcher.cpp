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

#include "apl/livedata/livedataobjectwatcher.h"
#include "apl/livedata/livedataobject.h"

namespace apl {

void
LiveDataObjectWatcher::registerObjectWatcher(const std::shared_ptr<LiveDataObject>& object) {
    auto token = object->addFlushCallback([this](const std::string key, LiveDataObject& liveDataObject) {
        liveDataObjectFlushed(key, liveDataObject);
    });
    mWatches.emplace_back(std::pair<int, std::weak_ptr<LiveDataObject>>{token, object});
}

LiveDataObjectWatcher::~LiveDataObjectWatcher()
{
    for (auto& watch : mWatches) {
        auto object = watch.second.lock();
        if (object)
            object->removeFlushCallback(watch.first);
    }
    mWatches.clear();
}

} // namespace apl