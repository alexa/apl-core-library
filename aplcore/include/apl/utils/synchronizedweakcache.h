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

#ifndef _APL_SYNCHRONIZED_WEAK_CACHE_H
#define _APL_SYNCHRONIZED_WEAK_CACHE_H

#include <atomic>
#include <mutex>
#include "apl/utils/weakcache.h"

namespace apl {

/**
 * A thread-safe wrapper around a WeakCache
 */
template<class K, class V>
class SynchronizedWeakCache {
    using VPtr = std::shared_ptr<V>;
    using guard_t = std::lock_guard<std::recursive_mutex>;

public:
    SynchronizedWeakCache() {}

    SynchronizedWeakCache(std::initializer_list<std::pair<K, VPtr>> args)
        : mCache(std::move(args)) {}

    /**
     * Find an item in the cache and return it, if it exists.
     * @param key The key to look up in the cache
     * @return The value if it exists or nullptr
     */
    VPtr find(K key) {
        guard_t g{mMutex};
        return mCache.find(key);
    }

    /**
     * Insert a new item in the weak cache. The cache will automatically be cleaned prior to 
     * insertion if the cache has been marked as dirty.
     * @param key The key to add to the cache.
     * @param value The value to add
     */
    void insert(K key, const VPtr& value) {
        guard_t g{mMutex};
        if (mIsDirty.exchange(false)) {
            mCache.clean();
        }
        mCache.insert(key, value);
    }

    /**
     * Clean the cache - remove all expired items
     */
    void clean() {
        guard_t g{mMutex};
        mCache.clean();
    }

    /**
     * @return The size of the cache. This method will clean the cache of invalid entries.
     */
    size_t size() {
        guard_t g{mMutex};
        return mCache.size();
    }

    /**
     * @return True if the cache is empty. This method will clean the cache of invalid entries.
     */
    bool empty() {
        guard_t g{mMutex};
        return mCache.empty();
    }

    /**
     * Mark cache as dirty, it will be cleaned during the next call to insert()
     */
    void markDirty() {
        mIsDirty.store(true);
    }

    /**
     * @return True if the cache is currently marked as dirty
     */
    bool dirty() {
        return mIsDirty.load();
    }

private:
    WeakCache<K, V> mCache;
    std::recursive_mutex mMutex;
    std::atomic<bool> mIsDirty{false};
};

} // namespace apl

#endif // _APL_SYNCHRONIZED_WEAK_CACHE_H
