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

#include <string>

#include "apl/common.h"
#include "apl/engine/event.h"

namespace apl {

/**
 * Media resources manager. Inflated components (based on viewport window) may request some media resources to be
 * loaded. Manager will dedupe this requests and send them to runtime as an event. Runtime supposed to answer with call
 * to RootContext::mediaLoaded with every source that was successfully loaded.
 *
 * Please note that a media manager may be shared across multiple view hosts.  Media managers shared across
 * multiple view hosts should implement thread safety.
 */
class MediaManager {
public:
    virtual ~MediaManager() = default;

    /**
     * Request a media object
     * @param url The source required
     * @param type The type of media requested.  This should be removed in the future.
     * @return the media object
     */
    virtual MediaObjectPtr request(const std::string& url, EventMediaType type) = 0;

    /**
     * Go though current list of registered components and generate requests to load all required sources.
     * This method is called from the main event loop.  Override this method if your media manager needs
     * to be called frequently.
     * @param context The local data-binding context
     */
    virtual void processMediaRequests(const ContextPtr& context) {}

    /**
     * Notify the manager about a media object which either loaded or failed to load.  This method
     * is not required.  It is called by RootContext::mediaLoaded() and RootContext::mediaLoadFailed().
     * @param source The URL of the media object
     * @param isReady True if success, false if failure
     * @param errorCode The integer value with the error code in case of failure.
     * @param errorReason The reason in case of failure.
     */
    virtual void mediaLoadComplete(
        const std::string& source,
        bool isReady,
        int errorCode,
        const std::string& errorReason) {}
};

} // namespace apl

#endif // _APL_MEDIA_MANAGER_H
