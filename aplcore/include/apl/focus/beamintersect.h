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

#ifndef _APL_BEAM_INTERSECT_H
#define _APL_BEAM_INTERSECT_H

#include "apl/common.h"
#include "apl/focus/focusdirection.h"
#include "apl/primitives/rect.h"

namespace apl {

/**
 * Beam (ray) based focus finder. Decision on what to consider as next component to be focused based
 * on intersection parameters of a beam generated from current (origin) focused area and provided
 * direction. Algo tries to replicate Android behavior in relation to that.
 */
class BeamIntersect {
public:
    /**
     * Build BeamIntersect score.
     * @param origin currently focused area.
     * @param candidate candidate area.
     * @param direction focus movement direction.
     * @return score representation object.
     */
    static BeamIntersect build(const Rect& origin, const Rect& candidate, FocusDirection direction);

    BeamIntersect() : mCandidate(Rect()), mIntersect(0), mDistance(0) {}
    virtual ~BeamIntersect() = default;

    /**
     * @return candidate area.
     */
    Rect getCandidate() const { return mCandidate; }

    /**
     * @return true if empty/invalid, false otherwise.
     */
    bool empty() const { return mCandidate.empty(); }

    /**
     * @return Standard comparison function. <0 if current object should be before other, 0 if equal,
     * >0 if should be after other.
     */
    int compare(const BeamIntersect& other) const;

    /// Comparison operators
    bool operator==(const BeamIntersect& other) const { return this->compare(other) == 0; }
    bool operator!=(const BeamIntersect& other) const { return this->compare(other) != 0; }
    bool operator<(const BeamIntersect& other) const { return this->compare(other) < 0; }
    bool operator>(const BeamIntersect& other) const { return this->compare(other) > 0; }

    friend streamer& operator<<(streamer& os, const BeamIntersect& intersect);

private:
    BeamIntersect(Rect candidate, float intersect, float distance) :
            mCandidate(candidate), mIntersect(intersect), mDistance(distance)
    {}

    Rect mCandidate;
    float mIntersect;
    float mDistance;
};

} // namespace apl

#endif //_APL_BEAM_INTERSECT_H
