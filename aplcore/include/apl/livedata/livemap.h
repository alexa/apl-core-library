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

#ifndef _APL_LIVE_MAP_H
#define _APL_LIVE_MAP_H

#include "apl/livedata/liveobject.h"
#include "apl/utils/counter.h"

namespace apl {

class LiveMapChange;

/**
 * A Live object is a single APL Object that changes over time.  LiveMaps are
 * created and modified by ViewHosts.  For example:
 *
 *     // Before creating the root context:
 *     LiveMap myObject({"Sample String Object"});
 *     rootConfig.addLive("MyLiveMap", myObject);
 *
 *     // After the root context has been created:
 *     myObject.update("Changed string object");
 *
 * Inside of the APL document the LiveMap may be used normally in data-binding
 * contexts.  For example:
 *
 *     {
 *       "type": "Text",
 *       "text": "The live object is currently '${MyLiveMap}'"
 *     }
 *
 *  The same LiveMap may be used by multiple RootContext objects. This allows
 *  an application to create a single LiveMap to track a source of changing
 *  data and display it in multiple view hosts simultaneously.
 *
 *  Changing the key-value pairs in a live map will update all data-bound
 *  values currently in the component hierarchy that depend on those values.
 *
 *  Please note that as in LiveArrays, the component hierarchy will NOT
 *  be re-inflated.
 */
class LiveMap : public LiveObject, public Counter<LiveMap> {
public:
    using ChangeCallback = std::function<void(const LiveMapChange&)>;

    /**
     * Create an empty LiveMap.
     * @return The LiveMap
     */
    static LiveMapPtr create() {
        return std::make_shared<LiveMap>(ObjectMap{});
    }

    /**
     * Create a LiveMap with an initial Object
     * @param object The initial Object
     * @return The LiveMap
     */
    static LiveMapPtr create(ObjectMap&& map) {
        return std::make_shared<LiveMap>(std::move(map));
    }

    /**
     * Default constructor. Do not call this; use the create() method.
     * @param object
     */
    explicit LiveMap(ObjectMap&& map) : mMap(std::move(map)) {}

    // Override from LiveObject
    Object::ObjectType getType() const override { return Object::ObjectType::kMapType; }

    /**
     * Check to see if there are no elements in the map
     * @return True if the map is empty
     */
    bool empty() const noexcept {
        return mMap.empty();
    }

    /**
     * Clear all elements from the map
     */
    void clear();

    /**
     * Retrieve a value from the map.  If the key does not exist, a NULL object
     * will be returned.
     * @param key The key of the object to return.
     * @return The internal object.
     */
    Object get(const std::string& key) const {
        auto it = mMap.find(key);
        if (it == mMap.end())
            return Object::NULL_OBJECT();

        return it->second;
    }

    /**
     * Check to see if a key exists in the map.
     * @param key The key to search for.
     * @return True if the key exists.
     */
    bool has(const std::string& key) const {
        return mMap.find(key) != mMap.end();
    }

    /**
     * Set a key-value pair in the map.
     * @param key The key to insert
     * @param value The value to store
     */
    template<class T>
    void set(const std::string& key, T&& value) {
        mMap[key] = std::forward<T>(value);
        broadcastSet(key);
    }

    /**
     * Set a collection of values from a different map
     * @map The object map to copy values from
     */
    void update(const ObjectMap& map) {
        for (const auto& m : map)
            set(m.first, m.second);
    }

    /**
     * Replace the LiveMap with a new map.
     * @param map The new map to set.
     */
    void replace(ObjectMap&& map);

    /**
     * Remove a key from the map
     * @param key The key to remove
     * @return True if the key was found and removed.
     */
    bool remove(const std::string& key);

    /**
     * @return The internal map.  Generally you should not use this.
     */
    const ObjectMap& getMap() const { return mMap; }

    /**
     * Add a change callback to this LiveMap.
     * @param callback The callback to invoke whenever the object changes.
     * @return An opaque token to be used to remove the change callback.
     */
    int addChangeCallback(ChangeCallback&& callback) {
        int token = mChangeCallbackToken++;
        mChangeCallbacks.emplace(token, std::move(callback));
        return token;
    }

    /**
     * Remove a change callback from this LiveMap.
     * @param token The opaque token from the addChangeCallback(ChangeCallback) method.
     */
    void removeChangeCallback(int token) {
        mChangeCallbacks.erase(mChangeCallbacks.find(token));
    }

private:
    void broadcast(const LiveMapChange& command);
    void broadcastSet(const std::string& key);

private:
    ObjectMap mMap;
    int mChangeCallbackToken = 100;
    std::map<int, ChangeCallback> mChangeCallbacks;
};

} // namespace apl

#endif // _APL_LIVE_MAP_H
