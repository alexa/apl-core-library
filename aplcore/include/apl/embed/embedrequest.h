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

#ifndef _APL_EMBED_REQUEST_H
#define _APL_EMBED_REQUEST_H

#include <string>

#include "apl/common.h"
#include "apl/primitives/urlrequest.h"

namespace apl {

/**
 * EmbedRequest tracks a request made to resolve a URL to APL Document Content.
 */
class EmbedRequest final {
public:
    static EmbedRequestPtr create(URLRequest url, const DocumentContextPtr& origin, const ComponentPtr& originComponent);

    EmbedRequest(URLRequest url, const DocumentContextPtr& origin, const ComponentPtr& parent);

    const URLRequest& getUrlRequest() const;

    DocumentContextPtr getOrigin() const;

private:
    friend class Content;

    const URLRequest mUrl;
    DocumentContextWeakPtr mOrigin;
    std::weak_ptr<Component> mOriginComponent;
};

} // namespace apl

#endif // _APL_EMBED_REQUEST_H
