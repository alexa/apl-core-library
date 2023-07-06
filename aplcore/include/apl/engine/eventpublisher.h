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

#ifndef APL_EVENT_PUBLISHER_H
#define APL_EVENT_PUBLISHER_H

#include "apl/engine/event.h"

namespace apl
{

/**
 * Write-only interface for publishing events from within a document.
 */
class EventPublisher
{
public:
    virtual ~EventPublisher() {};

    /**
     * Pushes event for publication.
     *
     * @param event The Event to publish
     */
    virtual void push(const Event& event) = 0;

    /**
     * Pushes event for publication.
     *
     * @param event The Event to publish
     */
    virtual void push(Event&& event) = 0;
};

} // namespace apl

#endif // APL_EVENT_PUBLISHER_H
