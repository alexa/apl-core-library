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

#include "apl/livedata/livemap.h"
#include "apl/livedata/livemapchange.h"

namespace apl {

void
LiveMap::clear()
{
    mMap.clear();
    broadcast(LiveMapChange::replace());
}

void
LiveMap::replace(ObjectMap&& map)
{
    mMap = std::forward<ObjectMap>(map);
    broadcast(LiveMapChange::replace());
}

bool
LiveMap::remove(const std::string& key)
{
    auto it = mMap.find(key);
    if (it == mMap.end())
        return false;
    mMap.erase(it);
    broadcast(LiveMapChange::remove(key));
    return true;
}

void
LiveMap::broadcast(const LiveMapChange& command)
{
    for (const auto& m: mChangeCallbacks)
        m.second(command);
}

void
LiveMap::broadcastSet(const std::string& key)
{
    broadcast(LiveMapChange::set(key));
}

} // namespace apl