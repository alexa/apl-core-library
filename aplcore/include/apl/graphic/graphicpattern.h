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

#ifndef _APL_GRAPHIC_PATTERN_H
#define _APL_GRAPHIC_PATTERN_H

#include "apl/common.h"
#include "apl/primitives/objectdata.h"
#include "apl/utils/counter.h"

namespace apl {

/**
 * AVG patterns are non-parameterized re-usable vector graphic elements that can be applied to path stroke and
 * fill properties.
 */
class GraphicPattern : public ObjectData,
                       public Counter<GraphicPattern> {
public:
    static Object create(const Context& context, const Object& object);

    /**
     * Constructor, use create instead.
     */
    GraphicPattern(const std::string& description, double height, double width, std::vector<GraphicElementPtr>&& items);

    /**
     * @return Unique pattern ID.
     */
    std::string getId() const { return mId; }

    /**
     * @return Optional pattern description.
     */
    std::string getDescription() const { return mDescription; }

    /**
     * @return Pattern height.
     */
    double getHeight() const { return mHeight; }

    /**
     * @return Pattern width.
     */
    double getWidth() const { return mWidth; }

    /**
     * @return AVG elements contained in the pattern.
     */
    const std::vector<GraphicElementPtr> getItems() const { return mItems; }

    /* Standard ObjectData methods */
    std::string toDebugString() const override;
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const override;
    bool empty() const override { return false; }
    bool truthy() const override { return true; }
    std::uint64_t size() const override { return mItems.size(); }
private:
    std::string mId;
    std::string mDescription;
    double mHeight;
    double mWidth;
    std::vector<GraphicElementPtr> mItems;

    static id_type sUniqueIdGenerator;
};

} // namespace apl

#endif //_APL_GRAPHIC_PATTERN_H
