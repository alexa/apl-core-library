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

#ifndef _APL_CORE_TIME_MANAGER_H
#define _APL_CORE_TIME_MANAGER_H

#include <algorithm>
#include <vector>
#include <limits>

#include "apl/time/timemanager.h"

namespace apl {

/**
 * A heap-based implementation of a TimeManager.
 */
class CoreTimeManager : public TimeManager {
public:
    CoreTimeManager(apl_time_t time) : mTime(time), mNextId(100), mAnimatorCount(0) {}
    ~CoreTimeManager() override = default;

    /****** Methods from Timers *******/

    timeout_id setTimeout(Runnable func, apl_duration_t delay) override {
        timeout_id id = mNextId++;
        mTimerHeap.emplace_back(TimeoutTuple(func, nullptr, mTime, delay, id));
        std::push_heap(mTimerHeap.begin(), mTimerHeap.end());
        return id;
    }

    timeout_id setAnimator(Animator animator, apl_duration_t delay) override {
        timeout_id id = mNextId++;
        mTimerHeap.emplace_back(TimeoutTuple(nullptr, animator, mTime, delay, id));
        std::push_heap(mTimerHeap.begin(), mTimerHeap.end());
        mAnimatorCount++;
        return id;
    }

    bool clearTimeout(timeout_id id) override {
        for (auto it = mTimerHeap.begin(); it != mTimerHeap.end(); it++) {
            if (it->id == id) {
                if (it->animator)
                    mAnimatorCount--;
                it = mTimerHeap.erase(it);
                std::make_heap(mTimerHeap.begin(), mTimerHeap.end());
                return true;
            }
        }

        return false;
    }

    /****** Methods from TimeManager *******/

    int size() const override { return mTimerHeap.size(); }

    void updateTime(apl_time_t updatedTime) override {
        // Block going backwards in time, but clear any pending timeouts.
        if (updatedTime <= mTime) {
            runPending();
            return;
        }

        auto deltaTime = updatedTime - mTime;

        while (!mTimerHeap.empty() && mTimerHeap.begin()->endTime <= updatedTime)
            advanceToNext();

        mTime = updatedTime;

        // Run any animations outstanding. TODO: Could we make this more efficient by skipping non-animators?
        for (auto& m : mTimerHeap) {
            if (m.animator)
                m.animator(mTime - m.startTime);
        }
    }

    virtual apl_time_t nextTimeout() override {
        if (mAnimatorCount > 0)
            return mTime + 1;

        if (mTimerHeap.size())
            return mTimerHeap.at(0).endTime;

        return std::numeric_limits<apl_time_t>::max();
    }

    virtual apl_time_t currentTime() const override { return mTime; }

    virtual void runPending() override {
        while (mTimerHeap.size() && mTimerHeap.begin()->endTime <= mTime)
            advanceToNext();
    }

protected:
    void advanceToNext() {
        std::pop_heap(mTimerHeap.begin(), mTimerHeap.end());  // Move to end
        TimeoutTuple tt = mTimerHeap.back();  // Grab the last element
        mTimerHeap.pop_back();  // Remove the last element
        mTime = tt.endTime;   // Advance the clock
        if (tt.runnable)
            tt.runnable();  // Execute the runnable
        else {
            tt.animator(tt.endTime - tt.startTime);
            mAnimatorCount--;
        }
    }

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
};


} // namespace apl


#endif // _APL_CORE_TIME_MANAGER_H
