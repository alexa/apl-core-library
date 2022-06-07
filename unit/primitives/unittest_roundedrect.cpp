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
#include "rapidjson/document.h"

#include "apl/primitives/roundedrect.h"
#include <cmath>

using namespace apl;

class RoundedRectTest : public ::testing::Test {};

TEST_F(RoundedRectTest, Empty) {
    ASSERT_TRUE(RoundedRect().empty());
    ASSERT_TRUE(RoundedRect().isRect());
    ASSERT_TRUE(RoundedRect().isRegular());

    ASSERT_EQ(Rect(), RoundedRect().rect());
    ASSERT_EQ(Radii(), RoundedRect().radii());
    ASSERT_EQ(Size(), RoundedRect().getSize());
}

TEST_F(RoundedRectTest, Basic) {
    auto rrect = RoundedRect(Rect(10,20,100,200), 25);

    ASSERT_FALSE(rrect.empty());
    ASSERT_FALSE(rrect.isRect());
    ASSERT_TRUE(rrect.isRegular());

    ASSERT_EQ(Rect(10,20,100,200), rrect.rect());
    ASSERT_EQ(Radii(25), rrect.radii());
    ASSERT_EQ(Point(10,20), rrect.getTopLeft());
    ASSERT_EQ(Size(100,200), rrect.getSize());
}

TEST_F(RoundedRectTest, Complex) {
    auto rrect = RoundedRect(Rect(10,20,100,200), Radii(5,10,15,20));

    ASSERT_FALSE(rrect.empty());
    ASSERT_FALSE(rrect.isRect());
    ASSERT_FALSE(rrect.isRegular());

    ASSERT_EQ(Rect(10,20,100,200), rrect.rect());
    ASSERT_EQ(Radii(5,10,15,20), rrect.radii());
    ASSERT_EQ(Point(10,20), rrect.getTopLeft());
    ASSERT_EQ(Size(100,200), rrect.getSize());
}

TEST_F(RoundedRectTest, Trimmed) {
    // A zero width -> all the radii are trimmed to zero
    auto rr = RoundedRect(Rect(10,20,0,10), Radii(20));
    ASSERT_EQ(Radii(0,0,0,0), rr.radii());

    // A square with too much of a radius is trimmed to a circle
    rr = RoundedRect(Rect(10,20,10,10), Radii(100));
    ASSERT_EQ(Radii(5), rr.radii());

    // A rectangle with too much of a radius is trimmed to a pill shape
    rr = RoundedRect(Rect(0,0,100,20), Radii(100));
    ASSERT_EQ(Radii(10), rr.radii());

    rr = RoundedRect(Rect(0,0,20,100), Radii(100));
    ASSERT_EQ(Radii(10), rr.radii());

    // A rectangle can have uneven radii if they all fit.  They are clipped to a side length
    rr = RoundedRect(Rect(0,0,20,100), Radii(20,0,50,0));
    ASSERT_EQ(Radii(20,0,20,0), rr.radii());

    // If two radii conflict, they are scaled proportionally
    auto radii = RoundedRect(Rect(0,0,100,100), Radii(60,80,0,0)).radii();
    ASSERT_FLOAT_EQ(radii.topLeft(), 100 * 6.0 / 14.0);
    ASSERT_FLOAT_EQ(radii.topRight(), 100 * 8.0 / 14.0);
}

TEST_F(RoundedRectTest, Equality) {
    ASSERT_EQ(RoundedRect(), RoundedRect());
    ASSERT_EQ(RoundedRect(), RoundedRect(Rect(), Radii(5)));   // Radius gets trimmed

    auto rect1 = Rect(0,10,20,30);
    auto rect2 = Rect(-10,15,30,20);
    auto radii1 = Radii(0,2,3,4);
    auto radii2 = Radii(2,0,5,3);

    ASSERT_EQ(RoundedRect(rect1, radii1), RoundedRect(rect1, radii1));
    ASSERT_NE(RoundedRect(rect1, radii1), RoundedRect(rect2, radii1));
    ASSERT_NE(RoundedRect(rect1, radii1), RoundedRect(rect1, radii2));
    ASSERT_NE(RoundedRect(rect1, radii1), RoundedRect(rect2, radii2));
}

TEST_F(RoundedRectTest, Offset) {
    auto rr = RoundedRect(Rect(10,0,100,100), Radii(5,5,5,5));
    auto rr2 = rr;
    rr2.offset(Point(10,100));
    ASSERT_EQ(RoundedRect(Rect(20,100,100,100), Radii(5)), rr2);
}

TEST_F(RoundedRectTest, Inset) {
    auto rr = RoundedRect( Rect(-10, -20, 100, 200), Radii(10,20,30,40));
    ASSERT_EQ(RoundedRect(Rect(0,-10,80,180), Radii(0,10,20,30)), rr.inset(10));
    ASSERT_EQ(RoundedRect(Rect(-20,-30,120,220), Radii(20,30,40,50)), rr.inset(-10));

    ASSERT_EQ(rr, rr.inset(-100).inset(100));  // Expand outwards and then back in => same
    ASSERT_EQ(RoundedRect(rr.rect(), Radii(45)), rr.inset(45).inset(-45));  // Expand in and out => different
}