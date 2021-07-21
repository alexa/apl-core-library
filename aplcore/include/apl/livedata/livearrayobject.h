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

#ifndef _APL_LIVE_ARRAY_OBJECT_H
#define _APL_LIVE_ARRAY_OBJECT_H

#include "apl/common.h"
#include "apl/utils/counter.h"
#include "apl/livedata/livedataobject.h"
#include "apl/livedata/livearraychange.h"

namespace apl {

/**
 * A LiveArrayObject is the representation of a LiveArray inside of a data-binding context.
 * It is the internal data structure held by an Object.  As changes occur in the LiveArray, the LiveArrayObject
 * accumulates a list of the changes.  The LiveDataManager is responsible for holding pointers to the
 * registered LiveArrayObjects.  The changes accumulated in a LiveArrayObject are periodically flushed by the
 * LiveDataManager, which propagates the LiveArray changes to all data-binding dependencies of the LiveArrayObject
 * object.
 *
 * To observe when a LiveArrayObject is flushed, register a "flush" callback.
 *
 * TODO: All internal changes are stored in a single array.  It may be more efficient to collapse
 *       these into three vectors: "removed", "updated", and "inserted".
 */
class LiveArrayObject : public LiveDataObject, Counter<LiveArrayObject> {
public:
    using size_type = ObjectArray::size_type;

    /**
     * Constructor.  Do not call this directly; use the create() method instead.
     */
    LiveArrayObject(const LiveArrayPtr& liveArray, const ContextPtr& context, const std::string& key);

    /**
     * Destructor
     */
    ~LiveArrayObject() override;

    /**
     * Overrides from LiveDataObject
     */
    Object::ObjectType getType() const override { return Object::kArrayType; }

    std::shared_ptr<LiveArrayObject> asArray() override {
        return std::static_pointer_cast<LiveArrayObject>(shared_from_this());
    }

    /// Overrides from ObjectData
    Object at(std::uint64_t index) const override;
    std::uint64_t size() const override;
    bool empty() const override;
    const std::vector<Object>& getArray() const override;
    void accept(Visitor<Object>& visitor) const override;
    std::string toDebugString() const override { return "LiveArrayObject<size=" + std::to_string(size()) + ">"; }

    virtual void ensure(size_t index) {}

    /**
     * This is called from the LiveDataManager to flush all stored array changes and update the context
     */
    void flush() override;

    /**
     * @return list of processed changes for this array.
     */
    const std::vector<LiveArrayChange>& getChanges() { return mChanges; }

    /**
     * Given a current index into the array, map it back into the old index that before the stored changes occurred.
     * @param index The current index in the array.
     * @return A pair containing the old index and a boolean flag marking if the item in the data array was updated.
     *         If the current index reflects a new item in the array, the old index is set to -1.
     */
    std::pair<int, bool> newToOld(ObjectArray::size_type index);

    /**
     * @return true if internal implementation may request items added or removed based on current binding state, false
     * otherwise.
     */
    virtual bool isPaginating() const { return false; }

private:
    void handleArrayMessage(const LiveArrayChange& change);

private:
    LiveArrayPtr mLiveArray;
    std::vector<LiveArrayChange> mChanges;
};

} // namespace apl

#endif // _APL_LIVE_ARRAY_OBJECT_H
