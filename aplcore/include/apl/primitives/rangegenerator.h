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

#ifndef _APL_RANGE_GENERATOR_H
#define _APL_RANGE_GENERATOR_H

#include <memory>

#include "apl/primitives/generator.h"

namespace apl {

/**
 * Generates an Array-like without generating all of the entries in the array.
 *
 * Array.range(4)      => { 0, 1, 2, 3 }  [start=0 end=4 step=1]
 * Array.range(3,6.2)  => { 3, 4, 5, 6 }  [start=3 end=6.2 step=1]
 * Array.range(4,0,-1) => { 4, 3, 2, 1 }  [start=4 end=0 step=-1]
 */
class RangeGenerator : public Generator {
public:
    static std::shared_ptr<RangeGenerator> create(double min, double max, double step) {
        return std::make_shared<RangeGenerator>(min, max, step);
    }

    RangeGenerator(double min, double max, double step)
        : mMinimum(min),
          mStep(step)
    {
        if (step == 0 || (step < 0 && min < max) || (step > 0 && min > max))
            mSize = 0;
        else {
            mSize = std::ceil((max - min) / step);
        }
    }

    Object at(std::uint64_t index) const override {
        assert(index >= 0 && index < mSize);
        return mMinimum + mStep * index;
    }

    std::uint64_t size() const override {
        return mSize;
    }

    bool empty() const override {
        return mSize == 0;
    }

    std::string toDebugString() const override {
        return "RangeGenerator<"+std::to_string(mMinimum)+","+std::to_string(mStep)+","+std::to_string(mSize)+">";
    }

private:
    double mMinimum;
    double mStep;
    std::uint64_t mSize;
};

} // namespace apl

#endif // _APL_RANGE_GENERATOR_H
