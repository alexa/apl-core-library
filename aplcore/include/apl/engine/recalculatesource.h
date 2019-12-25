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
        mDownstream.emplace(key, dependant);
    }

    /**
     * Remove this downstream dependant object.
     * @param dependant The object to remove
     */
    void removeDownstream(const std::shared_ptr<Dependant>& dependant) {
        for (auto it = mDownstream.begin() ; it != mDownstream.end() ; it++) {
            if (it->second == dependant) {
                mDownstream.erase(it);
                return;
            }
        }

        LOG(LogLevel::ERROR) << "Unable to find downstream dependant";
    }

    /**
     * The "key" local element has changed.  Recalculate all downstream objects that depend on key.
     * @param key The key that has changed.
     * @param useDirtyFlag If true, mark downstream changes with the dirty flag
     */
    void recalculateDownstream(T key, bool useDirtyFlag) {
        auto dependants = mDownstream.equal_range(key);
        for (auto d = dependants.first ; d != dependants.second ; d++)
            d->second->recalculate(useDirtyFlag);
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
    std::multimap<T, std::shared_ptr<Dependant>> mDownstream;
};

} // namespace apl

#endif // _APL_RECALCULATE_SOURCE_H
