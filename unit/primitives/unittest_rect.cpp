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

#include "apl/primitives/rect.h"
#include <cmath>

using namespace apl;

class RectTest : public ::testing::Test {};

TEST_F(RectTest, Basic) {
    Rect rect(0, 0, 100, 100);

    ASSERT_EQ(Point(0, 0), rect.getTopLeft());
    ASSERT_EQ(Point(100, 100), rect.getBottomRight());
    ASSERT_EQ(Point(50,50), rect.getCenter());

    Point offset(25, 50);
    rect.offset(offset);
    ASSERT_EQ(offset, rect.getTopLeft());
    ASSERT_EQ(Point(125, 150), rect.getBottomRight());
    ASSERT_EQ(Point(75,100), rect.getCenter());
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

TEST_F(RectTest, Equality) {
    Rect rect1(7, 8, 100, 200);
    Rect rect2(-7, -8, 200, 100);
    Rect rect3(-7, -8, 200, 100);

    ASSERT_TRUE(rect1 != rect2);
    ASSERT_TRUE(rect2 == rect3);
    ASSERT_FALSE(rect1 == rect2);
    ASSERT_FALSE(rect2 != rect3);
}

TEST_F(RectTest, EqualityNaN) {
    Rect rect1(7, 8, 100, 200);
    Rect rect2(NAN, NAN, NAN, NAN);
    Rect rect3(NAN, NAN, NAN, NAN);

    ASSERT_TRUE(rect1 != rect2);
    ASSERT_TRUE(rect2 == rect3);
    ASSERT_FALSE(rect1 == rect2);
    ASSERT_FALSE(rect2 != rect3);
}

TEST_F(RectTest, Empty) {
    Rect rect(0, 0, 0, 0);
    ASSERT_TRUE(rect.isEmpty());
    rect = Rect(0, 0, NAN, 100);
    ASSERT_TRUE(rect.isEmpty());
    rect = Rect(0, 0, 100, NAN);
    ASSERT_TRUE(rect.isEmpty());
}

TEST_F(RectTest, Contains) {
    // Empty can't contain point.
    ASSERT_FALSE(Rect(0, 0, 0, 0).contains(Point(0, 0)));

    ASSERT_TRUE(Rect(0, 0, 100, 100).contains(Point(0, 0)));
    ASSERT_TRUE(Rect(10, 10, 100, 100).contains(Point(50, 50)));
    ASSERT_FALSE(Rect(10, 10, 100, 100).contains(Point(5, 5)));
}

TEST_F(RectTest, DistanceTo) {
    ASSERT_EQ(0.0f, Rect(0,0,0,0).distanceTo(Point(0,0)));
    ASSERT_EQ(0.0f, Rect(10,10,20,20).distanceTo(Point(10,30)));
    ASSERT_EQ(0.0f, Rect(10,10,20,20).distanceTo(Point(30,10)));

    // Purely horizontal distance
    ASSERT_EQ(10.0f, Rect(10,10,20,20).distanceTo(Point(40,10)));
    ASSERT_EQ(10.0f, Rect(10,10,20,20).distanceTo(Point(0,20)));

    // Purely vertical distance
    ASSERT_EQ(10.0f, Rect(10,10,20,20).distanceTo(Point(10,40)));
    ASSERT_EQ(10.0f, Rect(10,10,20,20).distanceTo(Point(20,0)));

    // Diagonal
    ASSERT_FLOAT_EQ(50.f, Rect(10,10,20,20).distanceTo(Point(60, -30)));
}

TEST_F(RectTest, Serialize) {
    Rect rect(10, 20, 30, 40);
    ASSERT_FALSE(rect.isEmpty());

    rapidjson::Document doc;

    // Simple case serialized properly
    auto serialized = rect.serialize(doc.GetAllocator());
    ASSERT_TRUE(serialized.IsArray());
    ASSERT_EQ(10, serialized[0]);
    ASSERT_EQ(20, serialized[1]);
    ASSERT_EQ(30, serialized[2]);
    ASSERT_EQ(40, serialized[3]);

    // NaN replaced with 0 as NaN is not in JSON concepts.
    rect = Rect(NAN, NAN, NAN, NAN);
    serialized = rect.serialize(doc.GetAllocator());
    ASSERT_TRUE(serialized.IsArray());
    ASSERT_EQ(0, serialized[0]);
    ASSERT_EQ(0, serialized[1]);
    ASSERT_EQ(0, serialized[2]);
    ASSERT_EQ(0, serialized[3]);
}