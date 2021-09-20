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

#include <algorithm>
#include <iterator>

#include "apl/time/coretimemanager.h"
#include "apl/utils/log.h"

namespace apl {

const static bool DEBUG_CORE_TIME = false;

timeout_id
CoreTimeManager::setTimeout(Runnable func, apl_duration_t delay)
{
    LOG_IF(DEBUG_CORE_TIME) << "id=" << mNextId << " delay=" << delay;

    timeout_id id = mNextId++;
    mTimerHeap.emplace_back(TimeoutTuple(func, nullptr, mTime, delay, id));
    std::push_heap(mTimerHeap.begin(), mTimerHeap.end());
    return id;
}

timeout_id
CoreTimeManager::setAnimator(Animator animator, apl_duration_t delay)
{
    LOG_IF(DEBUG_CORE_TIME) << "id=" << mNextId << " delay=" << delay;

    timeout_id id = mNextId++;
    mTimerHeap.emplace_back(TimeoutTuple(nullptr, animator, mTime, delay, id));
    std::push_heap(mTimerHeap.begin(), mTimerHeap.end());
    mAnimatorCount++;
    return id;
}

bool
CoreTimeManager::clearTimeout(timeout_id id)
{
    LOG_IF(DEBUG_CORE_TIME) << "id=" << id;

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

void
CoreTimeManager::updateTime(apl_time_t updatedTime)
{
    LOG_IF(DEBUG_CORE_TIME) << "updateTime=" << updatedTime;

    if (mTerminated) {
        LOG(LogLevel::kError) << "Attempt to update time after termination.";
        return;
    }

    // Block going backwards in time, but clear any pending timeouts.
    if (updatedTime <= mTime) {
        runPending();
        return;
    }

    while (!mTimerHeap.empty() && mTimerHeap.begin()->endTime <= updatedTime)
        advanceToNext();

    mTime = updatedTime;

    // Collect active (i.e. not completed) animators
    std::vector<TimeoutTuple> animators;
    animators.reserve(mAnimatorCount);
    std::copy_if(mTimerHeap.begin(), mTimerHeap.end(), std::back_inserter(animators),
                 [](TimeoutTuple m) { return m.animator; });

    // Run animators outside timer heap iteration since running animators can mutate the heap
    for (auto &m : animators) {
        m.animator(mTime - m.startTime);
    }
}

apl_time_t
CoreTimeManager::nextTimeout()
{
    if (mAnimatorCount > 0)
        return mTime + 1;

    if (mTimerHeap.size())
        return mTimerHeap.at(0).endTime;

    return std::numeric_limits<apl_time_t>::max();
}

void
CoreTimeManager::runPending()
{
    while (mTimerHeap.size() && mTimerHeap.begin()->endTime <= mTime)
        advanceToNext();
}

void
CoreTimeManager::clear()
{
    mTimerHeap.clear();
}

void
CoreTimeManager::terminate()
{
    mTerminated = true;
    clear();
}

bool
CoreTimeManager::isTerminated()
{
    return mTerminated;
}

void CoreTimeManager::advanceToNext()
{
    std::pop_heap(mTimerHeap.begin(), mTimerHeap.end());  // Move to end
    TimeoutTuple tt = mTimerHeap.back();  // Grab the last element
    mTimerHeap.pop_back();  // Remove the last element
    mTime = tt.endTime;   // Advance the clock
    if (tt.runnable) {
        LOG_IF(DEBUG_CORE_TIME) << "Executing the runnable";
        tt.runnable();  // Execute the runnable
    }
    else if (tt.animator) {
        LOG_IF(DEBUG_CORE_TIME) << "Executing the animator";
        tt.animator(tt.endTime - tt.startTime);
        mAnimatorCount--;
    }
    else {
        LOG(LogLevel::kError) << "No animator or runnable defined";
    }
}

} // namespace apl
