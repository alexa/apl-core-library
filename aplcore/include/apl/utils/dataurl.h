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

#ifndef _APL_DATAURL_H
#define _APL_DATAURL_H

#include <string>

#include "apl/common.h"

namespace apl {


/**
 * Decomposed representation of RFC2397 to extract the elements. Only supports base64 extension for
 * images.
 */
class DataUrl {
public:
    /**
     * Parse a supposed Data URL.
     * @param session Current session
     * @param url Data URL.
     * @return Parsed object or nullptr if invalid.
     */
    static DataUrlPtr create(const SessionPtr& session, const std::string& url);

    /** Private constructor, do not use */
    DataUrl(const std::string& url, const std::string& data, const std::string& type, const std::string& subtype)
        : mUrl(url), mData(data), mType(type), mSubtype(subtype) {}

    /**
     * @return Original URL representation.
     */
    std::string getUrl() const { return mUrl; }

    /**
     * @return Base64 encoded data.
     */
    std::string getData() const { return mData; }

    /**
     * @return Media type.
     */
    std::string getType() const { return mType; }

    /**
     * @return Media subtype.
     */
    std::string getSubtype() const { return mSubtype; }

private:
    std::string mUrl;
    std::string mData;
    std::string mType;
    std::string mSubtype;
};

} // namespace apl

#endif // _APL_DATAURL_H
