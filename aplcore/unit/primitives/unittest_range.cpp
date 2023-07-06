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

#include "apl/primitives/range.h"

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

TEST_F(RangeTest, Iterator)
{
    Range r1{2,4};

    auto it = r1.begin();
    ASSERT_EQ(2, *it);
    ASSERT_EQ(2, *it++);
    ASSERT_EQ(3, *it++);
    ASSERT_EQ(4, *it++);
    ASSERT_EQ(r1.end(), it);

    Range r2;
    ASSERT_EQ(r2.begin(), r2.end());
}

TEST_F(RangeTest, Serialize)
{
    rapidjson::Document doc;
    Range r1{1,10};

    auto result = r1.serialize(doc.GetAllocator());
    ASSERT_TRUE(result.IsObject());
    ASSERT_EQ(1, result["lowerBound"].GetInt());
    ASSERT_EQ(10, result["upperBound"].GetInt());
}

TEST_F(RangeTest, Intersect)
{
    ASSERT_EQ( Range(5,6), Range(2, 10).intersectWith(Range(5, 6)));
    ASSERT_EQ( Range(5,10), Range(2, 10).intersectWith(Range(5, 15)));
    ASSERT_EQ( Range(2,5), Range(2, 10).intersectWith(Range(0, 5)));
    ASSERT_EQ( Range(2,10), Range(2, 10).intersectWith(Range(0, 15)));

    ASSERT_EQ( Range(), Range().intersectWith(Range()));
    ASSERT_EQ( Range(), Range(2, 10).intersectWith(Range()));
    ASSERT_EQ( Range(), Range().intersectWith(Range(-10, 10)));
    ASSERT_EQ( Range(), Range(2, 10).intersectWith(Range(11, 20)));
    ASSERT_EQ( Range(), Range(2, 10).intersectWith(Range(-10, 1)));
}

TEST_F(RangeTest, SubsetBelow)
{
    ASSERT_EQ( Range(2,4), Range(2,10).subsetBelow(5));
    ASSERT_EQ( Range(2,9), Range(2,10).subsetBelow(10));
    ASSERT_EQ( Range(2,10), Range(2,10).subsetBelow(20));
    ASSERT_EQ( Range(2,2), Range(2,10).subsetBelow(3));
    ASSERT_EQ( Range(), Range(2,10).subsetBelow(2));

    ASSERT_EQ( Range(), Range().subsetBelow(2));
}

TEST_F(RangeTest, SubsetAbove)
{
    ASSERT_EQ( Range(6,10), Range(2,10).subsetAbove(5));
    ASSERT_EQ( Range(), Range(2,10).subsetAbove(10));
    ASSERT_EQ( Range(2,10), Range(2,10).subsetAbove(1));
    ASSERT_EQ( Range(10,10), Range(2,10).subsetAbove(9));
    ASSERT_EQ( Range(3,10), Range(2,10).subsetAbove(2));

    ASSERT_EQ( Range(), Range().subsetAbove(2));
}