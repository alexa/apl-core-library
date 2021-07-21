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

#ifndef _TIMERS_H
#define _TIMERS_H

#include <functional>
#include <limits>

#include "apl/common.h"

namespace apl {

// avoids a collision with Windows threads.h
#undef INFINITE

class Timers
{
public:
    using Runnable = std::function<void()>;
    using Animator = std::function<void(apl_duration_t)>;

    /// Infinite duration
    constexpr static apl_duration_t INFINITE = std::numeric_limits<apl_duration_t>::max();

    virtual ~Timers() {}

    /**
     * Set a timeout function.
     * @param func The function to be called when the timeout expires.
     * @param delay The delay to pause before the timeout fires.
     * @return A unique ID for the timeout.
     */
    virtual timeout_id setTimeout(Runnable func, apl_duration_t delay = 0) = 0;

    /**
     * Set a method to be called as frequently as possible up until the time elapses.
     * @param animator The function to be called whenever possible until the animator expires.
     *                 The value passed to the animator is the amount of time that has passed since
     *                 the start of the animator.
     * @param duration The duration of the animation
     * @return A unique ID for the animator
     */
    virtual timeout_id setAnimator(Animator animator, apl_duration_t duration) = 0;

    /**
     * Cancel an executing timeout function.
     * @param id The id of the timeout.
     * @return True if it was cancelled, false if it could not be located.
     */
    virtual bool       clearTimeout(timeout_id id) = 0;

protected:
    Timers() {};

private:
    Timers(const Timers&) = delete;
    Timers& operator=(const Timers&) = delete;
};



} // namespace apl

#endif // _TIMERS_H