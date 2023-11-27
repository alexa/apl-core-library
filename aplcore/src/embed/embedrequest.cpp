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

#include <utility>

#include "apl/embed/embedrequest.h"

#include "apl/component/hostcomponent.h"
#include "apl/document/documentcontext.h"

namespace apl {

EmbedRequestPtr
EmbedRequest::create(URLRequest url, const DocumentContextPtr& origin, const ComponentPtr& originComponent) {
    return std::make_shared<EmbedRequest>(std::move(url), origin, originComponent);
}

EmbedRequest::EmbedRequest(URLRequest url, const DocumentContextPtr& origin, const ComponentPtr& originComponent)
    : mUrl(std::move(url)), mOrigin(origin), mOriginComponent(originComponent)
{}

const URLRequest&
EmbedRequest::getUrlRequest() const
{
    return mUrl;
}

DocumentContextPtr
EmbedRequest::getOrigin() const
{
    return mOrigin.lock();
}

} // namespace apl
