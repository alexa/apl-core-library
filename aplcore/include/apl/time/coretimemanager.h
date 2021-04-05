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

#ifndef _APL_CORE_TIME_MANAGER_H
#define _APL_CORE_TIME_MANAGER_H

#include <algorithm>
#include <vector>
#include <limits>
#include <atomic>

#include "apl/time/timemanager.h"

namespace apl {

/**
 * A heap-based implementation of a TimeManager.
 */
class CoreTimeManager : public TimeManager {
public:
    explicit CoreTimeManager(apl_time_t time) : mTime(time), mNextId(100), mAnimatorCount(0), mTerminated(false){}
    ~CoreTimeManager() override = default;

    /****** Methods from Timers *******/

    timeout_id setTimeout(Runnable func, apl_duration_t delay) override;
    timeout_id setAnimator(Animator animator, apl_duration_t delay) override;
    bool clearTimeout(timeout_id id) override;

    /****** Methods from TimeManager *******/

    int size() const override { return mTimerHeap.size(); }
    void updateTime(apl_time_t updatedTime) override;
    apl_time_t nextTimeout() override;
    apl_time_t currentTime() const override { return mTime; }
    void runPending() override;
    void clear() override;
    void terminate() override;
    bool isTerminated() override;

protected:
    void advanceToNext();

protected:
    // Hold regular timers until they fire
    struct TimeoutTuple {
        TimeoutTuple(Runnable runnable, Animator animator, apl_time_t startTime, apl_duration_t duration, timeout_id id)
            : runnable(runnable), animator(animator), startTime(startTime), endTime(startTime + duration), id(id) {}

        Runnable runnable;
        Animator animator;
        apl_time_t startTime;
        apl_time_t endTime;
        timeout_id id;

        bool operator<(const TimeoutTuple& rhs) const {
            return endTime > rhs.endTime;  // Reverse the order deliberately
        }
    };

    std::vector<TimeoutTuple> mTimerHeap;
    apl_time_t mTime;
    timeout_id mNextId;
    int mAnimatorCount;
    std::atomic<bool> mTerminated;
};


} // namespace apl


#endif // _APL_CORE_TIME_MANAGER_H
