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

#ifndef _APL_LIVE_MAP_OBJECT_H
#define _APL_LIVE_MAP_OBJECT_H

#include "apl/common.h"
#include "apl/utils/counter.h"
#include "apl/livedata/livedataobject.h"
#include "apl/livedata/livemapchange.h"

namespace apl {

/**
 * A LiveMapObject is the representation of a LiveMap inside of a data-binding context.
 * It is the internal data structure held by an Object. As changes occur in the LiveMap,
 * the LiveMapObject accumulates a list of the changes.  The LiveDataManager is responsible
 * for holding pointers to the registered LiveMapObjects. The changes accumulated in a LiveMapObject
 * are periodically flushed by the LiveDataManager, which propagates the LiveMap changes to all
 * data-binding dependencies of the LiveMapObject object.
 *
 * To observe when a LiveMapObject is flushed, register a "flush" callback.
 */
class LiveMapObject : public LiveDataObject, Counter<LiveMapObject> {
public:
    /**
     * Constructor. Do not call this directly; use the LiveDataObject::create() method instead
     */
    LiveMapObject(const LiveMapPtr& liveMap, const ContextPtr& context, const std::string& key);

    /**
     * Destructor
     */
    ~LiveMapObject() override;

    /**
     * Overrides from LiveDataObject
     */
    Object::ObjectType getType() const override { return Object::kMapType; }

    std::shared_ptr<LiveMapObject> asMap() override {
        return std::static_pointer_cast<LiveMapObject>(shared_from_this());
    }

    /**
     * Overrides from Object::Data
     */
    Object get(const std::string& key) const override;
    bool has(const std::string& key) const override;
    const ObjectMap& getMap() const override;
    void accept(Visitor<Object>& visitor) const override;

    std::string toDebugString() const override { return "LiveMapObject<size=" + std::to_string(size()) + ">"; }

    /**
     * This is called from the LiveDataManager to flush all stored map changes and update the context
     */
    void flush() override;

    /**
     * @return list of changes processed for this map.
     */
    const std::vector<LiveMapChange>& getChanges();

    /**
     * @return List of changed keys.
     */
    const std::set<std::string>& getChanged();

private:
    void handleMapMessage(const LiveMapChange& change);

private:
    LiveMapPtr mLiveMap;
    std::vector<LiveMapChange> mChanges;
    std::set<std::string> mChanged;
};

} // namespace apl

#endif // _APL_LIVE_MAP_OBJECT_H
