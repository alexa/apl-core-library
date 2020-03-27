/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "apl/livedata/livemapobject.h"
#include "apl/livedata/livemap.h"
#include "apl/livedata/livemapchange.h"
#include "apl/livedata/livedatamanager.h"
#include "apl/engine/context.h"

namespace apl {

LiveMapObject::LiveMapObject(const LiveMapPtr& liveMap,
                             const ContextPtr& context,
                             const std::string& key)
    : LiveDataObject(context, key),
      mLiveMap(liveMap)
{
    mToken = liveMap->addChangeCallback([this](const LiveMapChange& change) {
        handleMapMessage(change);
    });
}

LiveMapObject::~LiveMapObject()
{
    mLiveMap->removeChangeCallback(mToken);
}

const Object
LiveMapObject::get(const std::string& key) const
{
    return mLiveMap->get(key);
}

bool
LiveMapObject::has(const std::string& key) const
{
    return mLiveMap->has(key);
}

const ObjectMap&
LiveMapObject::getMap() const
{
    return mLiveMap->getMap();
}

void
LiveMapObject::accept(Visitor<Object>& visitor) const
{
    const auto& map = mLiveMap->getMap();
    visitor.push();
    for (auto it = map.begin() ; !visitor.isAborted() && it != map.end() ; it++) {
        Object(it->first).accept(visitor);
        if (!visitor.isAborted()) {
            visitor.push();
            it->second.accept(visitor);
            visitor.pop();
        }
    }
    visitor.pop();
}

const std::set<std::string>&
LiveMapObject::getChanged()
{
    // If we've been replaced, EVERYTHING has been changed or updated in some way
    if (mReplaced) {
        const auto& map = mLiveMap->getMap();
        for (const auto& m : map)
            mChanges.emplace(m.first);
        mReplaced = false;
    }
    return mChanges;
}

void
LiveMapObject::handleMapMessage(const LiveMapChange& change)
{
    // Ignore any commands once replaced has been set
    if (mReplaced)
        return;

    switch (change.command()) {
        case LiveMapChange::REMOVE:
            mChanges.erase(change.key());
            break;
        case LiveMapChange::SET:
            mChanges.emplace(change.key());
            break;
        case LiveMapChange::REPLACE:
            mReplaced = true;
            mChanges.clear();
            break;
    }

    markDirty();
}

} // namespace apl