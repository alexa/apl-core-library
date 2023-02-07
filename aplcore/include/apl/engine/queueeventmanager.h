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

#ifndef APL_QUEUE_EVENT_MANAGER_H
#define APL_QUEUE_EVENT_MANAGER_H

#include <memory>
#include <queue>

#include "apl/engine/eventmanager.h"

namespace apl
{

/**
 * An EventManager that delegates all operations to an encapsulated std::queue.
 */
class QueueEventManager : public EventManager
{
public:
    ~QueueEventManager() override = default;

    void clear() override { events = std::queue<Event>(); }
    bool empty() const override { return events.empty(); }
    const Event& front() const override { return events.front(); }
    Event& front() override { return events.front(); }
    void pop() override { events.pop(); }
    void push(const Event& event) override { events.push(event); }
    void push(Event&& event) override { events.push(event); }

private:
    std::queue<Event> events;
};

} // namespace apl

#endif // APL_QUEUE_EVENT_MANAGER_H
