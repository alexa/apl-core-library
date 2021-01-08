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

#ifndef _APL_VELOCITY_TRACKER_H
#define _APL_VELOCITY_TRACKER_H

#include "apl/common.h"
#include "apl/primitives/point.h"
#include "apl/utils/ringbuffer.h"

namespace apl {

struct PointerEvent;


/**
 * Simple extendable velocity estimation strategy interface that acts on base of pointer movement
 * history.
 */
class VelocityEstimationStrategy {
public:
    VelocityEstimationStrategy();
    virtual ~VelocityEstimationStrategy() = default;

    /**
     * Calculate and return estimated interaction velocity. Will take in account previously
     * calculated velocity if was not reset previously.
     * @return Pair of X and Y velocities for the interaction.
     */
    virtual Point getEstimatedVelocity() = 0;

    /**
     * Add Pointer movement to the history. Limited to predefined value.
     * @param timestamp event timestamp.
     * @param position event position.
     */
    void addMovement(apl_time_t timestamp, Point position);

    /**
     * Reset internal state and any Movement history.
     */
    void reset();

protected:
    struct Movement {
        Movement() = default;
        Movement(apl_time_t timestamp, Point position) : timestamp(timestamp), position(position) {}

        apl_time_t timestamp;
        Point position;
    };

    RingBuffer<Movement> mHistory;
    float mXVelocity;
    float mYVelocity;
};

/**
 * Simple velocity tracking interface.
 */
class VelocityTracker {
public:
    /**
     * @param rootConfig RootConfig for configuration.
     */
    VelocityTracker(const RootConfig& rootConfig);

    /**
     * Process pointer event.
     * @param pointerEvent event.
     * @param timestamp event timestamp.
     */
    void addPointerEvent(const PointerEvent& pointerEvent, apl_time_t timestamp);

    /**
     * Calculate and return estimated interaction velocity according to selected strategy.
     * @return Point of X and Y velocities for the interaction.
     */
    Point getEstimatedVelocity();

    /**
     * Reset tracker internal state.
     */
    void reset();

private:
    std::shared_ptr<VelocityEstimationStrategy> mEstimationStrategy;
    apl_time_t mLastEventTimestamp;
    apl_duration_t mPointerInactivityTimeout;
};

} // namespace apl

#endif //_APL_VELOCITY_TRACKER_H
