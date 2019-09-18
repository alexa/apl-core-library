/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "apl/primitives/rect.h"

using namespace apl;

class RectTest : public ::testing::Test {};

TEST_F(RectTest, Basic) {
    Rect rect(0, 0, 100, 100);

    ASSERT_EQ(Point(0, 0), rect.getTopLeft());
    ASSERT_EQ(Point(100, 100), rect.getBottomRight());

    Point offset(50, 50);
    rect.offset(offset);
    ASSERT_EQ(offset, rect.getTopLeft());
    ASSERT_EQ(Point(150, 150), rect.getBottomRight());
}

TEST_F(RectTest, IntersectInside) {
    Rect outside(0, 0, 100, 100);
    Rect inside(10, 10, 30, 30);

    Rect intersect1 = outside.intersect(inside);
    Rect intersect2 = inside.intersect(outside);
    ASSERT_EQ(inside, intersect1);
    ASSERT_EQ(inside, intersect2);
}

TEST_F(RectTest, NotIntersectOutsideHorizontal) {
    Rect rect1(0, 0, 100, 100);
    Rect rect2(110, 0, 30, 30);

    Rect intersect1 = rect1.intersect(rect2);
    Rect intersect2 = rect2.intersect(rect1);
    ASSERT_EQ(Rect(), intersect1);
    ASSERT_EQ(Rect(), intersect2);
}

TEST_F(RectTest, NotIntersectOutsideVertical) {
    Rect rect1(0, 0, 100, 100);
    Rect rect2(0, 110, 30, 30);

    Rect intersect1 = rect1.intersect(rect2);
    Rect intersect2 = rect2.intersect(rect1);
    ASSERT_EQ(Rect(), intersect1);
    ASSERT_EQ(Rect(), intersect2);
}

TEST_F(RectTest, IntersectHorizontal) {
    Rect rect1(0, 0, 100, 100);
    Rect rect2(50, 0, 100, 100);

    Rect intersect1 = rect1.intersect(rect2);
    Rect intersect2 = rect2.intersect(rect1);
    ASSERT_EQ(Rect(50, 0, 50, 100), intersect1);
    ASSERT_EQ(Rect(50, 0, 50, 100), intersect2);
}

TEST_F(RectTest, IntersectVertical) {
    Rect rect1(0, 0, 100, 100);
    Rect rect2(0, 50, 100, 100);

    Rect intersect1 = rect1.intersect(rect2);
    Rect intersect2 = rect2.intersect(rect1);
    ASSERT_EQ(Rect(0, 50, 100, 50), intersect1);
    ASSERT_EQ(Rect(0, 50, 100, 50), intersect2);
}

TEST_F(RectTest, IntersectCorner) {
    Rect rect1(0, 0, 100, 100);
    Rect rect2(50, 50, 100, 100);

    Rect intersect1 = rect1.intersect(rect2);
    Rect intersect2 = rect2.intersect(rect1);
    ASSERT_EQ(Rect(50, 50, 50, 50), intersect1);
    ASSERT_EQ(Rect(50, 50, 50, 50), intersect2);
}

TEST_F(RectTest, Area) {
    Rect rect1(0, 0, 100, 100);
    Rect rect2(0, 0, 50, 50);

    ASSERT_EQ(10000.0f, rect1.area());
    ASSERT_EQ(2500.0f, rect2.area());
}

TEST_F(RectTest, Print) {
    Rect rect1(7, 8, 100, 200);
    Rect rect2(-7, -8, 200, 100);

    streamer s;
    s << rect1 << " " << rect2;

    ASSERT_EQ("100x200+7+8", rect1.toString());
    ASSERT_EQ("200x100-7-8", rect2.toString());

}