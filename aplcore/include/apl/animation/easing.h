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


#ifndef _APL_EASING_H
#define _APL_EASING_H

#include <assert.h>
#include <string>

#include "apl/common.h"

namespace apl {

class EasingCurve;
class streamer;

class Easing {
public:
    Easing(EasingCurve* curve) : mCurve(curve), mLastTime(0), mLastValue(0) { assert(curve); }
    Easing(const Easing& other) : mCurve(other.mCurve), mLastTime(0), mLastValue(0) {}
    Easing& operator=(const Easing& other) {
        mCurve = other.mCurve;
        mLastValue = other.mLastValue;
        mLastTime = other.mLastTime;
        return *this;
    }

    /**
     * Evaluate the easing curve at a given time between 0 and 1.
     * @param time
     * @return The value
     */
    float operator()(float time);

    /**
     * Generate an easing curve from a string.  If the string is invalid, we return a linear easing curve
     * @param easing The character string.
     * @return A reference to the easing curve.
     */
    static Easing parse(const SessionPtr& session, const std::string& easing);

    /**
     * Check to see if an easing curve has been defined.
     * @param easing The character string of the easing curve.
     * @return True if the easing curve exists.
     */
    static bool has(const char *easing);

    /**
     * @return The default linear easing curve
     */
    static Easing linear();

    bool operator==(const Easing& rhs);
    bool operator!=(const Easing& rhs);

    friend streamer& operator<<(streamer& , const Easing&);

private:
    // Note: We use a raw pointer because the Easing::parse method caches the curves.  We only throw out
    //       curves when the cache gets too large.
    const EasingCurve *mCurve;
    float mLastTime, mLastValue;
};

} // namespace apl


#endif //APL_EASING_H
