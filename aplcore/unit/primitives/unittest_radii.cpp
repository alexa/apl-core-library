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

#include "apl/primitives/radii.h"

using namespace apl;

class RadiiTest : public ::testing::Test {};

TEST_F(RadiiTest, Empty)
{
    Radii radii;
    ASSERT_TRUE(radii.empty());
    ASSERT_TRUE(radii.isRegular());
    ASSERT_FALSE(radii.truthy());

    ASSERT_EQ(0, radii.topLeft());
    ASSERT_EQ(0, radii.topRight());
    ASSERT_EQ(0, radii.bottomLeft());
    ASSERT_EQ(0, radii.bottomRight());
    ASSERT_EQ(0, radii.radius(apl::Radii::kTopLeft));
    ASSERT_EQ(0, radii.radius(apl::Radii::kTopRight));
    ASSERT_EQ(0, radii.radius(apl::Radii::kBottomLeft));
    ASSERT_EQ(0, radii.radius(apl::Radii::kBottomRight));
}

TEST_F(RadiiTest, Simple)
{
    auto radii = Radii(20);

    ASSERT_FALSE(radii.empty());
    ASSERT_TRUE(radii.isRegular());
    ASSERT_TRUE(radii.truthy());

    ASSERT_EQ(20, radii.topLeft());
    ASSERT_EQ(20, radii.topRight());
    ASSERT_EQ(20, radii.bottomLeft());
    ASSERT_EQ(20, radii.bottomRight());
    ASSERT_EQ(20, radii.radius(apl::Radii::kTopLeft));
    ASSERT_EQ(20, radii.radius(apl::Radii::kTopRight));
    ASSERT_EQ(20, radii.radius(apl::Radii::kBottomLeft));
    ASSERT_EQ(20, radii.radius(apl::Radii::kBottomRight));
}

TEST_F(RadiiTest, Complex)
{
    auto radii = Radii(1,2,3,4);

    ASSERT_FALSE(radii.empty());
    ASSERT_FALSE(radii.isRegular());
    ASSERT_TRUE(radii.truthy());

    ASSERT_EQ(1, radii.topLeft());
    ASSERT_EQ(2, radii.topRight());
    ASSERT_EQ(3, radii.bottomLeft());
    ASSERT_EQ(4, radii.bottomRight());
    ASSERT_EQ(1, radii.radius(apl::Radii::kTopLeft));
    ASSERT_EQ(2, radii.radius(apl::Radii::kTopRight));
    ASSERT_EQ(3, radii.radius(apl::Radii::kBottomLeft));
    ASSERT_EQ(4, radii.radius(apl::Radii::kBottomRight));
}

TEST_F(RadiiTest, Equality)
{
    ASSERT_EQ(Radii(), Radii(0) );
    ASSERT_EQ(Radii(10), Radii(10,10,10,10));
    ASSERT_NE(Radii(10), Radii(10,10,10,2));
}

TEST_F(RadiiTest, Sanitize)
{
    ASSERT_EQ(Radii(), Radii(-10));
    ASSERT_EQ(Radii(), Radii(-1, -2, -3, -4));
}

TEST_F(RadiiTest, Subtract)
{
    auto radii = Radii(10,15,20,25);
    ASSERT_EQ(radii.subtract(5), Radii(5,10,15,20));
    ASSERT_EQ(radii.subtract(20), Radii(0,0,0,5));
    ASSERT_EQ(radii.subtract(30), Radii());
}