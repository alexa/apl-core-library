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

#include "apl/apl.h"
#include "apl/touch/utils/velocitytracker.h"

using namespace apl;

class VelocityTrackingTest : public ::testing::Test {};

TEST_F(VelocityTrackingTest, Simple) {
    auto config = RootConfig();
    auto vt = std::make_shared<VelocityTracker>(config);
    vt->addPointerEvent(PointerEvent(kPointerDown, Point(0, 0)), 0);
    vt->addPointerEvent(PointerEvent(kPointerUp, Point(10, 100)), 10);

    auto velocities = vt->getEstimatedVelocity();
    ASSERT_EQ(1, velocities.getX());
    ASSERT_EQ(10, velocities.getY());


    vt = std::make_shared<VelocityTracker>(config);
    vt->addPointerEvent(PointerEvent(kPointerDown, Point(0, 0)), 0);
    vt->addPointerEvent(PointerEvent(kPointerMove, Point(10, 100)), 10);
    vt->addPointerEvent(PointerEvent(kPointerUp, Point(20, 200)), 20);

    velocities = vt->getEstimatedVelocity();
    ASSERT_EQ(1, velocities.getX());
    ASSERT_EQ(10, velocities.getY());
}

TEST_F(VelocityTrackingTest, Accelerating) {
    auto config = RootConfig();
    auto vt = std::make_shared<VelocityTracker>(config);
    vt->addPointerEvent(PointerEvent(kPointerDown, Point(0, 0)), 0);
    vt->addPointerEvent(PointerEvent(kPointerMove, Point(10, 100)), 10);
    vt->addPointerEvent(PointerEvent(kPointerUp, Point(30, 300)), 20);

    auto velocities = vt->getEstimatedVelocity();
    // 0.4 * 1 + 0.6 * 2
    ASSERT_EQ(1.6f, velocities.getX());
    ASSERT_EQ(16.0f, velocities.getY());
}

TEST_F(VelocityTrackingTest, Inherit) {
    auto config = RootConfig();
    auto vt = std::make_shared<VelocityTracker>(config);
    vt->addPointerEvent(PointerEvent(kPointerDown, Point(0, 0)), 0);
    vt->addPointerEvent(PointerEvent(kPointerUp, Point(10, 100)), 10);

    auto velocities = vt->getEstimatedVelocity();
    ASSERT_EQ(1, velocities.getX());
    ASSERT_EQ(10, velocities.getY());

    vt->addPointerEvent(PointerEvent(kPointerDown, Point(0, 0)), 0);
    vt->addPointerEvent(PointerEvent(kPointerUp, Point(20, 200)), 10);

    velocities = vt->getEstimatedVelocity();
    // 0.4 * 1 + 0.6 * 2
    ASSERT_EQ(1.6f, velocities.getX());
    ASSERT_EQ(16.0f, velocities.getY());
}

TEST_F(VelocityTrackingTest, DirectionChange) {
    auto config = RootConfig();
    auto vt = std::make_shared<VelocityTracker>(config);
    vt->addPointerEvent(PointerEvent(kPointerDown, Point(0, 0)), 0);
    vt->addPointerEvent(PointerEvent(kPointerMove, Point(10, 100)), 10);
    vt->addPointerEvent(PointerEvent(kPointerUp, Point(5, 50)), 20);

    auto velocities = vt->getEstimatedVelocity();
    // Change in direction overrides to v2
    ASSERT_EQ(-0.5f, velocities.getX());
    ASSERT_EQ(-5.0f, velocities.getY());
}

TEST_F(VelocityTrackingTest, InteractionTimeout) {
    auto config = RootConfig();
    config.pointerInactivityTimeout(50);

    auto vt = std::make_shared<VelocityTracker>(config);
    vt->addPointerEvent(PointerEvent(kPointerDown, Point(0, 0)), 0);
    vt->addPointerEvent(PointerEvent(kPointerMove, Point(10, 100)), 10);
    vt->addPointerEvent(PointerEvent(kPointerUp, Point(5, 50)), 80);

    auto velocities = vt->getEstimatedVelocity();
    // Change in direction overrides to v2
    ASSERT_EQ(0, velocities.getX());
    ASSERT_EQ(0, velocities.getY());
}