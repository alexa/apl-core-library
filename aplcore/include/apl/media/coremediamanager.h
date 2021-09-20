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

#ifndef _APL_CORE_MEDIA_MANAGER_H
#define _APL_CORE_MEDIA_MANAGER_H

#include <map>
#include <memory>
#include <set>

#include "apl/media/mediamanager.h"

namespace apl {

/**
 * The core media manager pushes events onto the event queue when media objects are requested.
 *
 * This media manager is not thread safe and should not be used by multiple view hosts (create one per view host).
 * This is the default media manager that will be instantiated in RootConfig if not overwritten.
 */
class CoreMediaManager : public MediaManager, public std::enable_shared_from_this<CoreMediaManager> {
public:
    ~CoreMediaManager() override = default;

    MediaObjectPtr request(const std::string& url, EventMediaType type) override;

    void processMediaRequests(const ContextPtr& context) override;

    void mediaLoadComplete(const std::string& source,
                           bool isReady,
                           int errorCode = 0,
                           const std::string& errorReason = std::string()) override;

    /**
     * Remove a URL from the map of media objects.  Note that this does not release any MediaObjects using that
     * URL.  It only removes it from the known map maintained by the core media manager.
     * @param url The URL to remove.
     */
    void removeMediaObject(const std::string& url);

protected:
    std::map<std::string, std::weak_ptr<MediaObject>> mObjectMap;
    std::set<std::weak_ptr<MediaObject>, std::owner_less<std::weak_ptr<MediaObject>>> mPending;
};

} // namespace apl

#endif // _APL_CORE_MEDIA_MANAGER_H
