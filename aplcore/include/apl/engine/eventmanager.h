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

#ifndef APL_EVENT_MANAGER_H
#define APL_EVENT_MANAGER_H

#include <memory>

#include "apl/engine/event.h"
#include "apl/engine/eventpublisher.h"

namespace apl
{

class EventManager;

using EventManagerPtr = std::shared_ptr<EventManager>;

/**
 * Read-Write interface for publishing and consuming events.
 */
class EventManager : public EventPublisher
{
public:
    virtual ~EventManager() {};

    /**
     * Discard all pending, published events.
     */
    virtual void clear() = 0;

    /**
     * Determine if any published events are pending.
     *
     * @return true iff there are no pending events
     */
    virtual bool empty() const = 0;

    /**
     * Return the next pending published event. Check empty first.
     * @return A reference to the next event
     */
    virtual Event& front() = 0;

    /**
     * Return the next pending published event. Check empty first.
     * @return A reference to the next event if it exists; ??? otherwise.
     */
    virtual const Event& front() const = 0;

    /**
     * Removes the next pending event. Check empty first.
     */
    virtual void pop() = 0;
};

} // namespace apl

#endif // APL_EVENT_MANAGER_H
