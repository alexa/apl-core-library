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

#include "gtest/gtest.h"

#include "apl/utils/range.h"

using namespace apl;

class RangeTest : public ::testing::Test {};

TEST_F(RangeTest, Basic) {
    Range range;
    ASSERT_TRUE(range.empty());
    ASSERT_EQ(0, range.size());
    ASSERT_FALSE(range.contains(0));
    ASSERT_FALSE(range.contains(-1));
    ASSERT_FALSE(range.contains(5));

    range = Range(7, 15);
    ASSERT_FALSE(range.empty());
    ASSERT_EQ(9, range.size());
    ASSERT_EQ(7, range.lowerBound());
    ASSERT_EQ(15, range.upperBound());
    ASSERT_TRUE(range.contains(7));
    ASSERT_TRUE(range.contains(10));
    ASSERT_TRUE(range.contains(15));
    ASSERT_FALSE(range.contains(5));
    ASSERT_FALSE(range.contains(17));
    ASSERT_TRUE(range.above(20));
    ASSERT_TRUE(range.below(5));
}

TEST_F(RangeTest, Changes) {
    Range range;

    range.expandTo(7);
    ASSERT_FALSE(range.empty());
    ASSERT_EQ(7, range.lowerBound());
    ASSERT_EQ(7, range.upperBound());

    range.expandTo(15);
    ASSERT_EQ(7, range.lowerBound());
    ASSERT_EQ(15, range.upperBound());
    ASSERT_FALSE(range.contains(5));
    ASSERT_TRUE(range.contains(10));
    ASSERT_FALSE(range.contains(17));

    range.shift(3);
    ASSERT_EQ(10, range.lowerBound());
    ASSERT_EQ(18, range.upperBound());

    range.shift(-6);
    ASSERT_EQ(4, range.lowerBound());
    ASSERT_EQ(12, range.upperBound());

    range.expandTo(0);
    range.expandTo(15);
    ASSERT_EQ(0, range.lowerBound());
    ASSERT_EQ(15, range.upperBound());

    range.dropItemsFromTop(7);
    ASSERT_EQ(0, range.lowerBound());
    ASSERT_EQ(8, range.upperBound());

    range.dropItemsFromBottom(2);
    ASSERT_EQ(2, range.lowerBound());
    ASSERT_EQ(8, range.upperBound());

    range = Range();
    ASSERT_TRUE(range.empty());

    range.insert(0);
    ASSERT_EQ(0, range.lowerBound());
    ASSERT_EQ(0, range.upperBound());

    range.insert(1);
    range.insert(1);
    ASSERT_EQ(0, range.lowerBound());
    ASSERT_EQ(2, range.upperBound());
}

TEST_F(RangeTest, ReduceToEmpty) {
    Range range(7, 15);
    range.dropItemsFromTop(9);
    ASSERT_TRUE(range.empty());

    range.expandTo(0);
    range.expandTo(5);
    range.dropItemsFromBottom(6);
    ASSERT_TRUE(range.empty());
}

TEST_F(RangeTest, Equality) {
    Range range1(7, 15);
    Range range2(8, 16);

    ASSERT_FALSE(range1 == range2);

    range2.shift(-1);
    ASSERT_TRUE(range1 == range2);
}

TEST_F(RangeTest, ExtendTowards)
{
    Range r1;
    ASSERT_TRUE(r1.empty());

    // Extension on an empty range gives the number
    ASSERT_EQ(3, r1.extendTowards(3));

    // Extending towards a higher number indexes upwards
    ASSERT_EQ(4, r1.extendTowards(5));
    ASSERT_EQ(5, r1.extendTowards(5));
    ASSERT_EQ(5, r1.extendTowards(5));

    // Extending towards a lower number indexes downwards
    ASSERT_EQ(2, r1.extendTowards(0));
    ASSERT_EQ(1, r1.extendTowards(0));
    ASSERT_EQ(0, r1.extendTowards(0));
    ASSERT_EQ(0, r1.extendTowards(0));

    ASSERT_EQ(0, r1.lowerBound());
    ASSERT_EQ(5, r1.upperBound());
}