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

#ifndef APL_TEST_DOCUMENT_MANAGER_H
#define APL_TEST_DOCUMENT_MANAGER_H

#include <algorithm>
#include <memory>
#include <map>

#include "apl/common.h"
#include "apl/embed/documentmanager.h"

namespace apl
{

class TestDocumentManager : public DocumentManager
{
public:
    struct TestEmbedRequest {
        std::string url;
        std::weak_ptr<EmbedRequest> request;
        EmbedRequestSuccessCallback success;
        EmbedRequestFailureCallback error;
    };

    void request(const std::weak_ptr<EmbedRequest>& request,
                 EmbedRequestSuccessCallback success,
                 EmbedRequestFailureCallback error) override;

    /**
     * Invoke success callback with content for the request associated to url. By default, request
     * state is preserved for repeated invocations with the same url; request state can be erased by
     * setting cleanup to true.
     *
     * @param url The url identifying the request to succeed
     * @param content The resolved content; may not be ready if host is to supply pending parameters
     * @param cleanup if true then erase the request and callbacks for url post-processing
     * @return DocumentContext
     */
    DocumentContextPtr succeed(const std::string &url,
                               const ContentPtr &content,
                               bool sameOrigin = false,
                               DocumentConfigPtr documentConfig = nullptr,
                               bool cleanup = false);

    /**
     * Invoke success callback with content for the request in the FIFO order.
     *
     * @return DocumentContext
     */
    DocumentContextPtr succeed(const ContentPtr &content);

    /**
     * Invoke fail callback with failure for the request associated to url. By default, request
     * state is preserved for repeated invocations with the same url; request state can be erased by
     * setting cleanup to true.
     *
     * @param url The url identifying the request to succeed
     * @param failure describes the failure to resolve content for url
     * @param cleanup iff true then erase the request and callbacks for url post-processing
     */
    void fail(const std::string &url, const std::string &failure, bool cleanup = false);

    /**
     * Get first request with matching url
     */
    const std::weak_ptr<EmbedRequest>& get(const std::string &url) const;

    const std::vector<TestEmbedRequest>& getUnresolvedRequests() const { return requests; }

    int getResolvedRequestCount() const { return resolvedRequests.size(); }

private:
    std::vector<TestEmbedRequest> requests;
    std::vector<std::weak_ptr<EmbedRequest>> resolvedRequests;
};

} // namespace apl

#endif // APL_TEST_DOCUMENT_MANAGER_H
