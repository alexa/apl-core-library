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

#include "testdocumentmanager.h"

using namespace apl;

void
TestDocumentManager::request(const std::weak_ptr<EmbedRequest>& request,
                             EmbedRequestSuccessCallback success,
                             EmbedRequestFailureCallback error)
{
    requests.emplace_back(TestEmbedRequest{request.lock()->getUrlRequest().getUrl(), request, std::move(success), std::move(error)});
}

DocumentContextPtr
TestDocumentManager::succeed(const std::string &url,
                             const ContentPtr &content,
                             bool connectedVisualContext,
                             DocumentConfigPtr documentConfig,
                             bool cleanup)
{
    DocumentContextPtr documentContext;

    auto it = std::find_if(requests.begin(), requests.end(), [url](const TestEmbedRequest& request){
        return request.url == url;
    });


    if (it != requests.end())
    {
        auto ptr = it->request.lock();
        if (ptr != nullptr) {
            documentContext = it->success({ptr, content, connectedVisualContext, documentConfig});
        }

        if (cleanup) {
            resolvedRequests.push_back(it->request);
            requests.erase(it);
        }
    }

    return documentContext;
}

DocumentContextPtr
TestDocumentManager::succeed(const ContentPtr &content)
{
    DocumentContextPtr documentContext;
    auto it = requests.begin();

    if (it != requests.end()) {
        documentContext = it->success({it->request.lock(), content, false});
    }

    if (documentContext) {
        resolvedRequests.push_back(it->request);
        requests.erase(it);
    }

    return documentContext;
}

const std::weak_ptr<EmbedRequest>&
TestDocumentManager::get(const std::string &url) const
{
    static std::weak_ptr<EmbedRequest> sMissingNo = std::weak_ptr<EmbedRequest>();

    auto it = std::find_if(requests.begin(), requests.end(), [url](const TestEmbedRequest& request){
        return request.url == url;
    });
    return it != requests.end()
               ? it->request
               : sMissingNo;
}

void
TestDocumentManager::fail(const std::string &url, const std::string &failure, bool cleanup)
{
    auto it = std::find_if(requests.begin(), requests.end(), [url](const TestEmbedRequest& request){
        return request.url == url;
    });

    if (it != requests.end())
    {
        auto ptr = it->request.lock();
        if (ptr != nullptr) {
            it->error({ptr, failure});
        }
        if (cleanup) {
            resolvedRequests.push_back(it->request);
            requests.erase(it);
        }
    }
}
