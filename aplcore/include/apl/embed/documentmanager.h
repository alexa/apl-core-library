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

#ifndef _APL_DOCUMENT_MANAGER_H
#define _APL_DOCUMENT_MANAGER_H

#include <functional>
#include <memory>
#include <string>

#include "apl/common.h"
#include "apl/content/documentconfig.h"
#include "apl/embed/embedrequest.h"

namespace apl {

/**
 * Successful response structure.
 * @param request The request identifying the resolved Content
 * @param content The requested Content
 * @param connectedVisualContext True for cases when the embedded document's visual context should
 * @param documentConfig Configuration for the embedded document
 * be stitched into the parent document's visual context.
 */
struct EmbeddedRequestSuccessResponse {
    EmbedRequestPtr request;
    ContentPtr content;
    bool connectedVisualContext;
    DocumentConfigPtr documentConfig;
};

using EmbedRequestSuccessCallback = std::function<DocumentContextPtr(EmbeddedRequestSuccessResponse&& response)>;

/**
 * Failed response structure.
 * @param request The request that could not be resolved
 * @param failure A failure message
 */
struct EmbeddedRequestFailureResponse {
    EmbedRequestPtr request;
    std::string failure;
};

using EmbedRequestFailureCallback = std::function<void(EmbeddedRequestFailureResponse&& response)>;

/**
 * DocumentManager facilitates embedding APL Documents within other APL Documents by enabling the
 * "hosting" RootContext to request APL Document Content via URL.
 */
class DocumentManager {
public:
    virtual ~DocumentManager() = default;

    /**
     * Request to resolve the given URL to APL Document Content. Once resolved, exactly one of
     * success or failure must be invoked, informing the requester of the outcome. If the same Content
     * is requested by multiple callers, resolving that request must result in invoking one of success
     * or failure for each requester, i.e., invoke each of the success callbacks for each requester,
     * or invoke each of the failure callbacks for each requester.
     *
     * "success" may be invoked prior to the requested Content being "ready" iff the Content has
     * one or more pending parameters.
     *
     * If request is expired then the request is considered cancelled; neither success nor error
     * will be invoked once a request is cancelled.
     *
     * @param request The request identifying the Content to resolve.
     * @param success The callback to invoke when the requested Content is available.
     * @param failure The callback to invoke if the EmbedRequest will never be available.
     */
    virtual void request(
        const std::weak_ptr<EmbedRequest>& request,
        EmbedRequestSuccessCallback success,
        EmbedRequestFailureCallback error) = 0;
};

} // namespace apl

#endif // _APL_DOCUMENT_MANAGER_H
