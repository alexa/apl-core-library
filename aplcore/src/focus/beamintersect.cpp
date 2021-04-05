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

#include "apl/focus/beamintersect.h"

/// Virtually "infinite"
static const float BEAM_LENGTH = 1000000;

/**
 * Weight down cross-axis component of distance score.
 * NOTE: Likely requires adjustment.
 */
static const float CROSS_AXIS_WEIGHT = 0.8f;

namespace apl {

/**
 * Generate virtually infinite beam originating from origin in requested direction.
 */
static Rect
getBeam(const Rect& origin, FocusDirection direction)
{
    switch (direction) {
        case kFocusDirectionLeft:
            return {origin.getX() - BEAM_LENGTH, origin.getY(), BEAM_LENGTH + origin.getWidth(), origin.getHeight()};
        case kFocusDirectionRight:
            return {origin.getX(), origin.getY(), BEAM_LENGTH, origin.getHeight()};
        case kFocusDirectionUp:
            return {origin.getX(), origin.getY() - BEAM_LENGTH, origin.getWidth(), BEAM_LENGTH + origin.getHeight()};
        case kFocusDirectionDown:
            return {origin.getX(), origin.getY(), origin.getWidth(), BEAM_LENGTH};
        default:
            return {};
    }
}

/**
 * Calculate intersection of candidate rect with beam projected from origin. Represented as
 * percentage of beam cross-section covered by candidate rect.
 */
static float
beamIntersect(const Rect& origin, const Rect& candidate, FocusDirection direction)
{
    auto intersectRect = getBeam(origin, direction).intersect(candidate);
    switch (direction) {
        case kFocusDirectionLeft:
        case kFocusDirectionRight:
            return intersectRect.getHeight()/candidate.getHeight();
        case kFocusDirectionUp:
        case kFocusDirectionDown:
            return intersectRect.getWidth()/candidate.getWidth();
        default:
            return 0.0f;
    }
}

/**
 * Calculate the distance score between origin and candidate.
 */
static float
distanceScore(const Rect& origin, const Rect& candidate, FocusDirection direction, bool calculateCrossAxis)
{
    // Cross-axis difference is treated as 0 if candidate intersects with origin.
    float crossAxisDiff = 0;
    if (calculateCrossAxis) {
        switch (direction) {
            case kFocusDirectionLeft:
            case kFocusDirectionRight:
                if (origin.getBottom() < candidate.getTop())
                    crossAxisDiff = candidate.getTop() - origin.getBottom();
                else if (candidate.getBottom() < origin.getTop())
                    crossAxisDiff = origin.getTop() - candidate.getBottom();
                break;
            case kFocusDirectionUp:
            case kFocusDirectionDown:
                if (origin.getRight() < candidate.getLeft())
                    crossAxisDiff = candidate.getLeft() - origin.getRight();
                else if (candidate.getRight() < origin.getLeft())
                    crossAxisDiff = origin.getLeft() - candidate.getRight();
                break;
            default:
                // Virtually unreachable.
                crossAxisDiff = BEAM_LENGTH;
                break;
        }
    }

    // Axis difference is a distance between edges opposite to direction of movement. This allows to
    // measure "distance" to candidate that intersects origin.
    // Function assumes that candidate is always "to the direction" from origin, so no negative results
    // possible below.
    float axisDiff;
    switch (direction) {
        case kFocusDirectionLeft:
            axisDiff = origin.getRight() - candidate.getRight();
            break;
        case kFocusDirectionRight:
            axisDiff = candidate.getLeft() - origin.getLeft();
            break;
        case kFocusDirectionUp:
            axisDiff = origin.getBottom() - candidate.getBottom();
            break;
        case kFocusDirectionDown:
            axisDiff = candidate.getTop() - origin.getTop();
            break;
        default:
            // Virtually unreachable.
            axisDiff = BEAM_LENGTH;
            break;
    }

    return crossAxisDiff * crossAxisDiff * CROSS_AXIS_WEIGHT + axisDiff * axisDiff;
}

BeamIntersect
BeamIntersect::build(const Rect& origin, const Rect& candidate, FocusDirection direction)
{
    auto intersectScore = beamIntersect(origin, candidate, direction);
    auto distance = distanceScore(origin, candidate, direction, intersectScore <= 0.0f);
    return BeamIntersect(
        candidate,
        intersectScore,
        distance);
}

int
BeamIntersect::compare(const BeamIntersect& other) const {
    // Something that intersects the directional beam from origin is always better vs non intersecting
    auto currentIntersect = mIntersect;
    auto candidateIntersect = other.mIntersect;

    if ((currentIntersect > 0 && candidateIntersect > 0) ||
        (currentIntersect <= 0 && candidateIntersect <= 0)) {
        // If both intersect - go with simple distance, if no intersection - go real.
        auto currentDistance = mDistance;
        auto candidateDistance = other.mDistance;
        if (candidateDistance < currentDistance) {
            return -1;
        }
        if (candidateDistance > currentDistance) {
            return 1;
        }
    }

    return candidateIntersect >= currentIntersect ? -1 : 1;
}

streamer&
operator<<(streamer& os, const BeamIntersect& intersect)
{
    os << "BeamIntersect(mCandidate: " << intersect.mCandidate.toDebugString() << ", "
       << "mIntersect: " << intersect.mIntersect << ", "
       << "mDistance: " << intersect.mDistance << ")";
    return os;
}

}