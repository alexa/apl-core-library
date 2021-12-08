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

#ifndef APL_URLREQUEST_H
#define APL_URLREQUEST_H

#include "apl/primitives/object.h"

#include <string>
#include <vector>

namespace apl {

using HeaderItem = std::string;
using HeaderArray = std::vector<HeaderItem>;

/**
 * URLRequest class stores the common elements required for any media source.
 */
class URLRequest {
public:
    /**
     * Build a URLRequest from an Object. The source object may be a URLRequest (in
     * which case it is copied), array, single string or map.
     *
     * @example
     *
     * Using create will have pre-filtered headers set up via RootConfig.
     *
     * "item": {
     *   "type": "Image",
     *   "source": {
     *     "url": "url with an image",
     *     "description": "milky way",
     *     "headers": [ "X-amzn-test: XXX",  "X-amzn-test2: YYY", "User-Agent": "XXXX"]
     *   }
     * }
     *
     * Setting up deny for User-Agent will end up with the following list of headers
     *
     * "item": {
     *   "type": "Image",
     *   "source": {
     *     "url": "url with an image",
     *     "description": "milky way",
     *     "headers": [ "X-amzn-test: XXX",  "X-amzn-test2: YYY"]
     *   }
     * }
     *
     *
     * @see RootConfig#filterHeaders
     *
     * @param context The data-binding context.
     * @param object The source of the media source.
     * @return An object containing a source or null.
     */
    static Object create(const Context& context, const Object& object);

    /**
     * Builds a URLRequest.
     *
     * This method should not be used directly, use URLRequest#create instead.
     *
     * Using the constructor will not pre-filter headers.
     *
     * @param url with the location to obtain the source
     * @param headers vector of key/value pairs with http/s headers to use for the header.
     */
    URLRequest(const std::string url, const HeaderArray headers);

    /**
     * @return Get request url.
     */
    std::string getUrl() const { return mUrl; }

    /**
     * @return headers to append to the request to obtain access to the source
     */
    const HeaderArray& getHeaders() const { return mHeaders; }

    bool operator==(const URLRequest& other) const;

    std::string toDebugString() const;
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

    bool empty() const { return false; }
    bool truthy() const { return true; }

private:
    std::string mUrl;
    HeaderArray mHeaders;
};

}
#endif // APL_URLREQUEST_H
