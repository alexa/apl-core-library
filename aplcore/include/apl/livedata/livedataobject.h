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

#ifndef _APL_LIVE_DATA_OBJECT_H
#define _APL_LIVE_DATA_OBJECT_H

#include <memory>

#include "apl/primitives/objecttype.h"
#include "apl/engine/context.h"
#include "apl/livedata/liveobject.h"

namespace apl {

class LiveArrayObject;
class LiveMapObject;

/**
 * Base class for a live array or object.  This is an object that can be modified outside
 * of the core engine and changes in the object will be reflected in the data-binding context
 * inside of the core engine.
 *
 * This is an abstract base class.  Do not create instances of this object.
 */
class LiveDataObject : public BaseArrayData,
                       public std::enable_shared_from_this<LiveDataObject> {
public:
    using FlushCallback = std::function<void(const std::string, LiveDataObject&)>;

    /**
     * Construct a suitable LiveDataObject subtype and register it with the LiveDataManager.
     *
     * This method adds the LiveDataObject as an Object in the data-binding context with the
     * name "key", registers the LiveDataObject for callbacks when the LiveObject changes, and
     * adds itself to the list of LiveDataObjects maintained by the LiveDataManager
     *
     * @param data The LiveObject to register
     * @param context The data-binding context
     * @param key The key to register the LiveObject as.
     * @return The constructed LiveDataObject.
     */
    static std::shared_ptr<LiveDataObject> create(const LiveObjectPtr& data,
                                                  const ContextPtr& context,
                                                  const std::string& key);

    /**
     * @return The object type contained
     */
    virtual LiveObject::ObjectType getType() const = 0;

    /**
     * @return This object as a live array object or nullptr if that is invalid.
     */
    virtual std::shared_ptr<LiveArrayObject> asArray() { return nullptr; }

    /**
     * @return This object as a live map object or nullptr if that is invalid
     */
    virtual std::shared_ptr<LiveMapObject> asMap() { return nullptr; }

    /**
     * @return datasource connection pointer or nullptr if livedata object is not type of datasource
     */
    virtual DataSourceConnectionPtr getDataSourceConnection() const { return nullptr; }

    /**
     * Called on all live data objects before any are flushed.
     *
     * This is done to give an opportunity to freeze any information that should not change during the course of the
     * overall live data flush. For example, we do not want to call new flush callback listeners that are added during
     * the course of flushing data, since they already have access to the latest data. Note that this assumes that live
     * data con only change outside of the data flushing stage. If we add a feature that allows the data flushing
     * pathway to modify the data in some way, we'll have to revisit this assumption.
     */
    void preFlush() {
        mMaxWatcherTokenBeforeFlush = mWatcherToken;
    }

    /**
     * Flush tracking changes
     */
    virtual void flush();

    /**
     * @return The data-binding context that the object is defined within.
     */
    ContextPtr getContext() const { return mContext.lock(); }

    /**
     * @return The key name the object is registered as.
     */
    std::string getKey() const { return mKey; }

    /**
     * Register a function to be called whenever the LiveArrayObject is flushed.
     * @param callback The callback to be executed.
     * @return An opaque token for the removeFlushCallback(int) routine.
     */
    int addFlushCallback(FlushCallback&& callback);

    /**
     * Remove a watcher
     * @param token The opaque token returned when the watcher was registered.
     */
    void removeFlushCallback(int token);

    // ObjectData overrides.
    bool operator==(const ObjectData& rhs) const override {
        // In progress of changes propagation. Considered not-equal for comparisons.
        if (mIsFlushing) return false;
        return BaseArrayData::operator==(rhs);
    }

protected:
    LiveDataObject(const ContextPtr& context, const std::string& key) : mContext(context), mKey(key) {}
    void markDirty();

protected:
    std::weak_ptr<Context> mContext;
    std::string mKey;
    std::map<int, FlushCallback> mFlushCallbacks;
    int mWatcherToken = 100;
    int mMaxWatcherTokenBeforeFlush = -1;
    bool mReplaced = false;
    int mToken = -1;
    bool mIsFlushing = false;
};

} // namespace apl

#endif // _APL_LIVE_DATA_OBJECT_H
