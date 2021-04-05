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

#ifndef _APL_TIME_MANAGER_H
#define _APL_TIME_MANAGER_H

#include "apl/common.h"
#include "apl/time/timers.h"

namespace apl {

/**
 * Abstract class for managing timers.  Override this class if you need
 * to install custom timer and time management.
 */
class TimeManager : public Timers {
public:
    /**
     * @return The number of timers left to fire
     */
    virtual int size() const = 0;

    /**
     * Move forward in time
     * @param updatedTime
     */
    virtual void updateTime(apl_time_t updatedTime) = 0;

    /**
     * @return The time of the next timeout.  If animators are running, the
     *         next timeout is always 1 greater than the current time.
     */
    virtual apl_time_t nextTimeout() = 0;

    /**
     * @return The current APL time. This is the "elapsed" time since the APL document was opened.
     */
    virtual apl_time_t currentTime() const = 0;

    /**
     * Run any pending timers.
     */
    virtual void runPending() = 0;

    /**
     * Clear any pending actions and release any held references.
     */
    virtual void clear() = 0;

    /**
     * Clear any pending actions and release any held references. This instance can't be used after that.
     */
    virtual void terminate() = 0;

    /**
     * @return true if terminate() has been called.
     */
    virtual bool isTerminated() = 0;
};

} // namespace apl


#endif //_APL_TIME_MANAGER_H
