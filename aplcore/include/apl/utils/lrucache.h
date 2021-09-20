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

#ifndef _APL_LRU_CACHE_H
#define _APL_LRU_CACHE_H

#include <list>
#include <unordered_map>

namespace apl {

/**
 * Linked list + map implementation of LRU cache.
 */
template<class K, class V>
class LruCache {
public:
    LruCache(size_t sizeLimit) : mMaxSize(sizeLimit) {}

    void put(K id, V item) {
        mItems.push_front({id, item});
        if (mItems.size() > mMaxSize) {
            auto& removedItem = mItems.back();
            mAccess.erase(removedItem.first);
            mItems.pop_back();
        }
        mAccess.emplace(id, mItems.begin());
    }

    bool has(K id) const {
        return mAccess.count(id);
    }

    V& get(K id) {
        auto it = mAccess.find(id);
        assert(it != mAccess.end());
        mItems.splice(mItems.begin(), mItems, it->second);
        return mItems.front().second;
    }

private:
    using itemPack = std::pair<K, V>;
    std::list<itemPack> mItems;
    std::unordered_map<K, typename std::list<itemPack>::iterator> mAccess;
    size_t mMaxSize;
};

} // namespace apl

#endif //_APL_LRU_CACHE_H
