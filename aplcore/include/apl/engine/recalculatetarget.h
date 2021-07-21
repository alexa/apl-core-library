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

#ifndef _APL_RECALCULATE_TARGET_H
#define _APL_RECALCULATE_TARGET_H

#include <map>
#include <memory>

#include "apl/engine/dependant.h"

namespace apl {

/**
 * A mixin class that adds a multimap that points to the upstream dependants of this object.
 * Add this class to any object that has elements which are recalculated when an upstream value changes.
 * @tparam T The key type for the multimap for tracking elements.
 */
template<class T>
class RecalculateTarget {
public:
    /**
     * Virtual destructor makes sure that all upstream dependant objects are removed.
     */
    virtual ~RecalculateTarget() {
        removeUpstreamDependencies();
    }

    /**
     * Add an upstream dependant to this object.
     * @param key The key the target downstream property that will be recalculated.
     * @param dependant The dependant object.
     */
    void addUpstream(T key, const std::shared_ptr<Dependant>& dependant) {
        mUpstream.emplace(key, dependant);
    }

    /**
     * Search and remove all dependants that are associated with this downstream key.
     * @param key
     */
    void removeUpstream(T key) {
        for (auto it = mUpstream.begin() ; it != mUpstream.end() ; )
            if (it->first == key) {
                it->second->removeFromSource();
                it = mUpstream.erase(it);
            }
            else
                it++;
    }

    /**
     * Return how many upstream dependants are connected to this key.
     * @param key The key
     * @return The number of upstream dependants.
     */
    size_t countUpstream(T key) {
        return mUpstream.count(key);
    }

    /**
     * @return The total number of upstream dependants connected to this target.
     */
    size_t countUpstream() {
        return mUpstream.size();
    }

    /**
     * Explicitly clear upstream dependencies that drive this object.
     */
    void removeUpstreamDependencies() {
        for (auto& m : mUpstream) {
            m.second->removeFromSource();
        }
    }

private:
    std::multimap<T, std::shared_ptr<Dependant>> mUpstream;
};

} // namespace apl

#endif //_APL_RECALCULATE_TARGET_H
