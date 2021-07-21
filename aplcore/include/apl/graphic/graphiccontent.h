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

#ifndef _APL_GRAPHIC_CONTENT_H
#define _APL_GRAPHIC_CONTENT_H

#include <memory>

#include "apl/common.h"
#include "apl/content/jsondata.h"

namespace apl {

/**
 * Hold the JSON definition of a vector graphic.  This class is commonly used
 * when a vector graphic component supplies a URL for the source of the vector graphic.
 * The downloaded graphic definition should be inflated using GraphicContent::create()
 * and then passed to the component.
 */
class GraphicContent {
public:
    /**
     * Construct a shared pointer for this JSON data
     * @deprecated This method will be removed soon.
     * @param data The raw data
     * @return Null if the graphic data is invalid
     */
    static GraphicContentPtr create(JsonData&& data);

    /**
     * Construct a shared pointer for this JSON data.
     * @param session The logging session
     * @param data The raw data.
     * @return Null if the graphic data is invalid.
     */
    static GraphicContentPtr create(const SessionPtr& session, JsonData&& data);

public:
    /**
     * Internal constructor.  Use create() instead.
     * @param data The raw JSON data.
     */
    GraphicContent(JsonData&& data);

    /**
     * @return The internal rapidjson representation.
     */
    const rapidjson::Value& get() const { return mJsonData.get(); }

private:
    JsonData mJsonData;
};

} // namespace apl

#endif //_APL_GRAPHIC_CONTENT_H
