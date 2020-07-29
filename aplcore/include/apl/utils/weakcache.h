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

#ifndef _APL_WEAK_CACHE_H
#define _APL_WEAK_CACHE_H

#include <memory>
#include <map>

namespace apl {

/**
 * A weak cache is a map from a unique string to a weak_ptr.
 * As strong pointers are released the entries in the cache become invalid.
 * Periodically running the "clean()" method will remove those invalid entries.
 *
 * @tparam T
 */
template<class T>
class WeakCache {
public:
    WeakCache() {}

    WeakCache(std::initializer_list<std::pair<std::string, std::shared_ptr<T>>> args) {
        for (const auto& m : args)
            mCache.emplace(m);
    }

    /**
     * Find an item in the cache and return it, if it exists.
     * @param key
     * @return
     */
    std::shared_ptr<T> find(const std::string& key) {
        auto it = mCache.find(key);
        if (it != mCache.end()) {
            auto ptr = it->second.lock();
            if (ptr)
                return ptr;
            mCache.erase(it);
        }
        return nullptr;
    }

    /**
     * Insert a new item in the weak cache
     * @param key
     * @param value
     */
    void insert(std::string key, const std::shared_ptr<T>& value) {
        mCache.emplace(key, value);
    }

    /**
     * Clean the cache - remove all expired items
     */
    void clean() {
        auto it = mCache.begin();
        while (it != mCache.end()) {
            if (it->second.expired())
                it = mCache.erase(it);
            else
                it++;
        }
    }

    /**
     * @return The size of the cache.  This method will clean the cache of invalid entries.
     */
    size_t size() {
        clean();
        return mCache.size();
    }

    /**
     * @return True if the cache is empty.  This method will clean the cache of invalid entries.
     */
    bool empty() {
        clean();
        return mCache.empty();
    }

private:
    std::map<std::string, std::weak_ptr<T>> mCache;
};

} // namespace apl

#endif // _APL_WEAK_CACHE_H
