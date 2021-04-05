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

#ifndef _APL_SLICE_GENERATOR_H
#define _APL_SLICE_GENERATOR_H

#include <memory>

#include "apl/primitives/generator.h"

namespace apl {

/**
 * Generate a subsection (slice) of an array without generating all of the entries in the array
 *
 * let array = [ 101, 102, 103, 104, 105, 106 ]
 * Math.slice(array, 3)      => [ 104, 105, 106 ]    start=3, end=array.length
 * Math.slice(array, 1, 4)   => [ 102, 103, 104 ]    start=1, end=4
 * Math.slice(array, -4, -1) => [ 103, 104, 105 ]    start=-4 (=array.length - 4), end=-1
 */
class SliceGenerator : public Generator {
public:
    static std::shared_ptr<SliceGenerator> create(Object array, std::int64_t start, std::int64_t end) {
        return std::make_shared<SliceGenerator>(std::move(array), start, end);
    }

    SliceGenerator(Object array, std::int64_t start, std::int64_t end)
        : mArray(std::move(array)),
          mStart(start),
          mEnd(end)
    {
        assert(mArray.isArray());
        const std::int64_t len = mArray.size();

        // Negative values are offsets from the end
        if (mStart < 0)
            mStart += len;
        if (mEnd < 0)
            mEnd += len;

        // Trim mStart and mEnd so that 0 <= mStart <= mEnd <= len
        if (mStart < 0)
            mStart = 0;
        if (mEnd > len)
            mEnd = len;
        if (mEnd < mStart)
            mEnd = mStart;
    }

    Object at(std::uint64_t index) const override {
        assert(index >= 0 && index < size());
        return mArray.at(index + mStart);
    }

    std::uint64_t size() const override {
        return mEnd - mStart;
    }

    bool empty() const override {
        return mStart == mEnd;
    }

    std::string toDebugString() const override {
        return "SliceGenerator<"+std::to_string(mStart)+","+std::to_string(mEnd)+">";
    }

private:
    Object mArray;
    std::int64_t mStart;
    std::int64_t mEnd;
};

} // namespace apl

#endif // _APL_SLICE_GENERATOR_H
