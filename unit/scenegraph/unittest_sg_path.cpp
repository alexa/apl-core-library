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

#include "../testeventloop.h"
#include "test_sg.h"
#include "apl/scenegraph/builder.h"

using namespace apl;

TEST(SGPathTest, Basic)
{
    // Rectangles
    ASSERT_EQ(sg::path(Rect{0,20,100,100}),
              sg::path(Rect{0,20,100,100}));
    ASSERT_NE(sg::path(Rect{10,20,30,40}),
              sg::path(Rect{10,20,30,60}));

    // Rounded Rectangles
    ASSERT_EQ(sg::path(Rect{10,20,30,40}, 5),
              sg::path(Rect{10,20,30,40}, 5));
    ASSERT_NE(sg::path(Rect{10,20,30,40}, 5),
              sg::path(Rect{10,20,30,77}, 5));
    ASSERT_NE(sg::path(Rect{10,20,30,40}, 5),
              sg::path(Rect{10,20,30,40}, 2));

    // Rounded rectangles with complicated radii
    ASSERT_EQ(sg::path(Rect{10,20,30,40}, Radii{1,2,3,4}),
              sg::path(Rect{10,20,30,40}, Radii{1,2,3,4}));
    ASSERT_NE(sg::path(Rect{10,20,30,40}, Radii{1,2,3,4}),
              sg::path(Rect{10,20,30,77}, Radii{1,2,3,4}));
    ASSERT_NE(sg::path(Rect{10,20,30,40}, Radii{1,2,3,4}),
              sg::path(Rect{10,20,30,40}, Radii{1,2,3,7}));

    // Other variations of constructors with RoundedRect
    ASSERT_EQ(sg::path(Rect{10,20,30,40}, Radii{1,2,3,4}),
              sg::path(RoundedRect(Rect{10,20,30,40}, Radii{1,2,3,4})));
    ASSERT_EQ(sg::path(Rect{10,20,30,40}, 5),
              sg::path(RoundedRect(Rect{10,20,30,40}, Radii{5,5,5,5})));

    // String variations
    ASSERT_EQ(sg::path("M5,5 h20"), sg::path("M 5  5 L25,5"));
}