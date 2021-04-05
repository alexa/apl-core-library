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

#ifndef _APL_MEDIA_MANAGER_H
#define _APL_MEDIA_MANAGER_H

#include <set>
#include <map>
#include <vector>
#include <string>

#include "apl/common.h"

namespace apl {

class RootContextData;
class ConfigurationChange;
using CoreComponentWPtr = std::weak_ptr<CoreComponent>;

/**
 * Media resources manager. Inflated components (based on viewport window) may request some media resources to be
 * loaded. Manager will dedupe this requests and send them to runtime as an event. Runtime supposed to answer with call
 * to RootContext::mediaLoaded with every source that was successfully loaded.
 */
class MediaManager {
public:
    explicit MediaManager(const RootContextData& core);

    /**
     * Register set of media sources behind the component.
     * @param component component.
     * @param sources set of sources required by this component.
     * @result number of sources still required to be loaded.
     */
    size_t registerComponentMedia(const CoreComponentPtr& component, const std::set<std::string>& sources);

    /**
     * @param source source to check.
     * @return true if source loaded, false otherwise.
     */
    bool isLoaded(const std::string& source) const { return mLoadedSources.count(source) > 0; }

    /**
     * Go though current list of registered components and generate request to load all required sources.
     */
    void processMediaRequests();

    /**
     * Notify manager about media source loaded. If this fulfills any of component requirements it will be notified
     * accordingly.
     * @param source source that was loaded.
     */
    void mediaLoaded(const std::string& source);

    /**
     * Notify manager about media source fail to load.
     * @param source source that failed to load.
     */
    void mediaLoadFailed(const std::string& source);

private:
    const RootContextData& mCore;
    bool mComponentSetDirty = false;

    std::multimap<CoreComponentWPtr, std::string, std::owner_less<CoreComponentWPtr>> mComponentToSource;
    std::map<std::string, int> mPendingPerComponent;
    std::set<std::string> mRequestedSources;
    std::set<std::string> mLoadedSources;

    void noLongerPending(const std::string& source, bool failed);
};

} // namespace apl

#endif // _APL_MEDIA_MANAGER_H
