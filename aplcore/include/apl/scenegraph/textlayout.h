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

#ifndef _APL_TEXT_LAYOUT_H
#define _APL_TEXT_LAYOUT_H

#include <string>
#include <memory>
#include <rapidjson/document.h>

#include "apl/primitives/range.h"
#include "apl/primitives/rect.h"
#include "apl/primitives/size.h"

namespace apl {
namespace sg {

/**
 * Subclass this in your view host and use it to return sizing information
 * for a chunk of Text.
 */
class TextLayout {
public:
    virtual ~TextLayout() = default;

    virtual bool empty() const = 0;
    virtual Size getSize() const = 0;
    virtual float getBaseline() const = 0;
    virtual int getLineCount() const = 0;
    virtual std::string getLaidOutText() const { return ""; }
    virtual std::string toDebugString() const = 0;
    virtual bool isTruncated() const { return false; }

    virtual unsigned int getByteLength() const = 0;
    virtual Range getLineRangeFromByteRange(Range byteRange) const = 0;

    /**
     * Calculate the bounding box that surrounds a range of lines in this text layout.
     * If the range is empty then the bounding box surrounding all lines is returned.
     * @param lineRange The range of lines to use.
     * @return The bounding box of those lines or the bounding box of all lines if the range is empty.
     */
    virtual Rect getBoundingBoxForLines(Range lineRange) const = 0;

    virtual rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const {
        return rapidjson::Value();
    };
};

} // namespace sg
} // namespace apl

#endif // _APL_TEXT_LAYOUT_H
