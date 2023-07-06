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

#ifndef APL_TICK_SCHEDULER_H
#define APL_TICK_SCHEDULER_H

#include <memory>

#include "apl/document/coredocumentcontext.h"
#include "apl/primitives/object.h"
#include "apl/time/timemanager.h"

namespace apl
{

class TickScheduler
{
public:
    explicit TickScheduler(const std::shared_ptr<TimeManager>& timeManager);

    void processTickHandlers(const CoreDocumentContextPtr& documentContext) const;

private:
    void scheduleTickHandler(const std::weak_ptr<CoreDocumentContext>& documentContext,
                             const Object& handler,
                             double delay) const;

    const std::shared_ptr<TimeManager> mTimeManager;
};

} // namespace apl

#endif // APL_TICK_SCHEDULER_H
