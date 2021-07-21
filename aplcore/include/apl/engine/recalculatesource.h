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

#ifndef _APL_RECALCULATE_SOURCE_H
#define _APL_RECALCULATE_SOURCE_H

#include <map>
#include <memory>

#include "apl/engine/dependant.h"
#include "apl/utils/log.h"

namespace apl {

/**
 * A mixin class for objects where changing an element of this object will trigger recalculation of properties
 * on downstream objects.
 * @tparam T The key type used to distinguish the various elements of this object.
 */
template<class T>
class RecalculateSource {
public:
    /**
     * Add a dependant object that is downstream of this object.
     * @param key The key of the local element.  When this element is changed, the downstream dependant should recalculate.
     * @param dependant The dependant object connecting to the downstream dependant object.
     */
    void addDownstream(T key, const std::shared_ptr<Dependant>& dependant) {
        // For now, we strip off the "/" section of the keys
        auto name = key.substr(0, key.find("/", 0));

        // Don't add this pair if it already exists
        // While we're searching, we check for weak pointers
        auto dependants = mDownstream.equal_range(key);
        auto it = dependants.first;
        while (it != dependants.second) {
            auto ptr = it->second.lock();
            if (ptr) {
                if (ptr == dependant) {
                    LOG(LogLevel::kWarn) << "Attempted to add duplicate pair " << key;
                    return;    // This pair already exists
                }
                it++;
            } else {
                LOG(LogLevel::kWarn) << "Unexpected released weak pointer";
                it = mDownstream.erase(it);  // Throw it away
            }
        }

        mDownstream.emplace(name, dependant);
    }

    /**
     * Remove this downstream dependant object.  Note that we also clear out
     * any released weak_ptrs at the same time.
     * @param dependant The object to remove
     */
    void removeDownstream(const std::shared_ptr<Dependant>& dependant) {
        auto it = mDownstream.begin();
        while (it != mDownstream.end()) {
            // TODO: Possible optimization by using owner_before comparison
            if (it->second.expired() || it->second.lock() == dependant)
                it = mDownstream.erase(it);
            else
                it++;
        }
    }

    /**
     * The "key" local element has changed.  Recalculate all downstream objects that depend on key.
     * @param key The key that has changed.
     * @param useDirtyFlag If true, mark downstream changes with the dirty flag
     */
    void recalculateDownstream(T key, bool useDirtyFlag) {
        auto dependants = mDownstream.equal_range(key);
        auto it = dependants.first;
        while (it != dependants.second) {
            auto ptr = it->second.lock();
            if (ptr) {
                ptr->recalculate(useDirtyFlag);
                it++;
            }
            else {
                LOG(LogLevel::kWarn) << "Unexpected released weak pointer";
                it = mDownstream.erase(it);
            }
        }
    }

    /**
     * Return how many downstream dependants are connected to this key.
     * @param key The key
     * @return The number of downstream dependants.
     */
    size_t countDownstream(T key) {
        return mDownstream.count(key);
    }

    /**
     * @return The total number of downstream dependants connected to this source
     */
    size_t countDownstream() {
        return mDownstream.size();
    }

private:
    std::multimap<T, std::weak_ptr<Dependant>> mDownstream;
};

} // namespace apl

#endif // _APL_RECALCULATE_SOURCE_H
