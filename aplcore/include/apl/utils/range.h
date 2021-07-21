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

#ifndef _APL_RANGE_H
#define _APL_RANGE_H

namespace apl {

/**
 * Simple representation of closed integer range. Upper and lower bounds included into the range.
 */
class Range
{
public:
    Range() : mLowerBound(0), mUpperBound(-1) {}

    /**
     * Constructor.
     *
     * @param lowerBound lower bound to initialize range with.
     * @param upperBound upper bound to initialize range with.
     * @throws error when executed mLowerBound > mUpperBound.
     */
    Range(int lowerBound, int upperBound) : mLowerBound(lowerBound), mUpperBound(upperBound) {
        assert(mLowerBound <= mUpperBound);
    }

    bool operator==(const Range& rhs) const {
        return mLowerBound == rhs.mLowerBound && mUpperBound == rhs.mUpperBound;
    }

    /**
     * @return true if empty, false otherwise.
     */
    bool empty() const { return mUpperBound < mLowerBound; }

    /**
     * @return number of elements contained in the range.
     */
    size_t size() const {
        if (mUpperBound < mLowerBound) return 0;
        else return mUpperBound - mLowerBound + 1;
    }

    /**
     * @return Lower bound.
     * @throws error when executed on empty range.
     */
    int lowerBound() const {
        assert(mLowerBound <= mUpperBound);
        return mLowerBound;
    }

    /**
     * @return Upper bound.
     * @throws error when executed on empty range.
     */
    int upperBound() const {
        assert(mLowerBound <= mUpperBound);
        return mUpperBound;
    }

    /**
     * Check if element contained within [lowerBound, upperBound] range. Always false on empty range.
     * @param element number to check.
     * @return true in element is inside of a range, false otherwise.
     */
    bool contains(int element) const {
        return mUpperBound >= mLowerBound && mLowerBound <= element && element <= mUpperBound;
    }

    /**
     * @param element number to check.
     * @return true in element is above upper bound.
     * @throws error when executed on empty range.
     */
    bool above(int element) const {
        assert(mLowerBound <= mUpperBound);
        return element > mUpperBound;
    }

    /**
     * @param element number to check.
     * @return true in element is below lower bound.
     * @throws error when executed on empty range.
     */
    bool below(int element) const {
        assert(mLowerBound <= mUpperBound);
        return element < mLowerBound;
    }

    /**
     * Insert new item into a range. If range is empty both bounds assigned to provided value.
     * @param item new item.
     * @return offset of item from range start.
     * @throws error when item is outside of allowed insert boundaries.
     */
    int insert(int item) {
        if (mUpperBound < mLowerBound) {
            mLowerBound = item;
            mUpperBound = item;
        } else {
            assert(item <= mUpperBound + 1);
            assert(item >= mLowerBound);

            mUpperBound++;
        }

        return item - mLowerBound;
    }

    /**
     * Remove item from a range.
     * @param item item to remove.
     * @throws error when item is outside of range.
     */
    void remove(int item) {
        assert(contains(item));

        mUpperBound--;
    }

    /**
     * Expand range to provided bound. If range is empty both bounds assigned to provided value.
     * @param to position to expand range to.
     */
    void expandTo(int to) {
        if (mUpperBound < mLowerBound) {
            mLowerBound = to;
            mUpperBound = to;
        } else if (mLowerBound > to)
            mLowerBound = to;
        else if (mUpperBound < to)
            mUpperBound = to;
    }

    /**
     * @param count number of positions to trim from bottom of the range.
     * @throws error when executed on empty range.
     */
    void dropItemsFromBottom(size_t count) {
        assert(mLowerBound <= mUpperBound);
        if (count < size()) {
            mLowerBound += count;
        } else {
            *this = Range();
        }
    }

    /**
     * @param count number of positions to trim from top of the range.
     * @throws error when executed on empty range.
     */
    void dropItemsFromTop(size_t count) {
        assert(mLowerBound <= mUpperBound);
        if (count < size()) {
            mUpperBound -= count;
        } else {
            *this = Range();
        }
    }

    /**
     * Shift range values.
     * @param shift number of position to shift range by. Sign decides direction.
     * @throws error when executed on empty range.
     */
    void shift(int shift) {
        assert(mLowerBound <= mUpperBound);
        mLowerBound += shift;
        mUpperBound += shift;
    }

    /**
     * Extend the range by up to one unit towards the target number.  This is useful if you
     * need to perform an operation on each element in the extended range.  For example:
     *
     *     while (!range.contains(target)) {
     *       int index = range.extendTowards(target);
     *       int offset = index - range.lowerBound();
     *     }
     *
     * @param to position to expand the range to
     * @return the extension of the range
     */
    int extendTowards(int to) {
        if (mUpperBound < mLowerBound) {
            mLowerBound = to;
            mUpperBound = to;
            return to;
        }

        if (to < mLowerBound)
            return --mLowerBound;
        else if (to > mUpperBound)
            return ++mUpperBound;
        else
            return to;
    }

private:
    int mLowerBound;
    int mUpperBound;
};
} // namespace apl

#endif //_APL_RANGE_H