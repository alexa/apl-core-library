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

#include "apl/touch/utils/velocitytracker.h"
#include "apl/touch/pointerevent.h"
#include "apl/content/rootconfig.h"

namespace apl {

static const size_t VELOCITY_HISTORY_LIMIT = 20;
static const float ALPHA_FILTER_WEIGHT = 0.4;
static const float BETA_FILTER_WEIGHT = 1 - ALPHA_FILTER_WEIGHT;

/**
 * Simple a-b filter that prefer most recent velocity sample. In simplified terms:
 * 1) Assume we have following data, consisting of 3 points:
 *      p0 : t0, x0, y0
 *      p1 : t1, x1, y1
 *      p2 : t2, x2, y2
 * 2) This gives us two sequential pairs that gives us two velocity samples (X only for simplicity)
 * calculated as:
 *      v1 = (x1 - x0)/(t1 - t0)
 *      v2 = (x2 - x1)/(t2 - t1)
 * 3) Estimated velocity sum calculated as:
 *      Sv = A * v1 + B * v2
 * where A and B is filter coefficients.
 * 4) Any following velocity sample (vn) will be incorporated in similar way:
 *      Sv = A * Sv + B * vn
 *
 * Some points that is specific to current APL implementation:
 * 1) A = 0.4, B = 0.6
 * 2) Speed reset to 0 on direction change.
 */
class FilterVelocityEstimationStrategy : public VelocityEstimationStrategy {
public:
    Point getEstimatedVelocity() override {
        if (mHistory.size() >= 2) {
            std::pair<float, float> preVelocity{0.0f, 0.0f};
            auto prevMove = mHistory.dequeue();

            while (!mHistory.empty()) {
                auto move = mHistory.dequeue();
                auto currVelocity = getVelocityBetweenMovements(prevMove, move);
                prevMove = move;

                mXVelocity = accumulate(preVelocity.first, mXVelocity, currVelocity.first);
                mYVelocity = accumulate(preVelocity.second, mYVelocity, currVelocity.second);

                preVelocity = currVelocity;
            }
        }

        // We keep velocities signed to detect direction changes, but report dabsolute
        return {mXVelocity, mYVelocity};
    }

private:
    std::pair<float, float> getVelocityBetweenMovements(const Movement& start, const Movement& end) {
        auto timeDiff = end.timestamp - start.timestamp;
        auto xdiff = end.position.getX() - start.position.getX();
        auto ydiff = end.position.getY() - start.position.getY();
        return {xdiff/timeDiff, ydiff/timeDiff};
    }

    float accumulate(float preVelocity, float accumulatedVelocity, float currentVelocity) {
        if (accumulatedVelocity == 0)
            return currentVelocity;

        // If there is a direction change
        if (accumulatedVelocity * currentVelocity < 0.0f) {
            // If move is continuing in opposite direction, reset and calculate for current direction
            if(preVelocity * currentVelocity > 0.0f)
                return ALPHA_FILTER_WEIGHT * preVelocity + BETA_FILTER_WEIGHT * currentVelocity;
            else
                // Don't apply filter immediately on direction change but wait for next movement
                // velocity. Just reduce the velocity here, applying filter here can significantly
                // reduce the velocity if direction change is because of faulty/ misplaced pointer
                // event in opposite direction of the gesture.
                return accumulatedVelocity + currentVelocity;
        }

        return ALPHA_FILTER_WEIGHT * accumulatedVelocity + BETA_FILTER_WEIGHT * currentVelocity;
    }
};

//////////////////////////////////////////////////////////////////////////////////////////

VelocityTracker::VelocityTracker(const RootConfig& rootConfig)
    : mLastEventTimestamp(0),
      mPointerInactivityTimeout(rootConfig.getProperty(RootProperty::kPointerInactivityTimeout).getDouble()) {
    // TODO: Basic case here is something that is trivial filter preferring later velocity.
    //  Need to make strategies configurable.
    mEstimationStrategy = std::make_shared<FilterVelocityEstimationStrategy>();
}

void
VelocityTracker::addPointerEvent(const PointerEvent& pointerEvent, apl_time_t timestamp) {
    switch (pointerEvent.pointerEventType) {
        case kPointerDown:
            // Just reset last event time and fall through.
            mLastEventTimestamp = timestamp;
        case kPointerMove:
        case kPointerUp:
            // Reset speed tracking if pointer considered inactive.
            if (timestamp >= mLastEventTimestamp + mPointerInactivityTimeout) {
                mEstimationStrategy->reset();
            }
            mLastEventTimestamp = timestamp;

            mEstimationStrategy->addMovement(timestamp, pointerEvent.pointerEventPosition);
            break;
        default:
            mEstimationStrategy->reset();
            break;
    }
}

Point
VelocityTracker::getEstimatedVelocity() {
    return mEstimationStrategy->getEstimatedVelocity();
}

void
VelocityTracker::reset() {
    mEstimationStrategy->reset();
    mLastEventTimestamp = 0;
}

VelocityEstimationStrategy::VelocityEstimationStrategy() : mHistory(VELOCITY_HISTORY_LIMIT), mXVelocity(0), mYVelocity(0) {}

void
VelocityEstimationStrategy::addMovement(apl::apl_time_t timestamp, Point position) {
    if (!mHistory.empty()) {
        // Skip if time not changed or changed backwards
        auto& last = mHistory.back();
        if (timestamp <= last.timestamp) {
            return;
        }
    }

    mHistory.enqueue({timestamp, position});
}

void
VelocityEstimationStrategy::reset() {
    mHistory.clear();
    mXVelocity = 0;
    mYVelocity = 0;
}

} // namespace apl