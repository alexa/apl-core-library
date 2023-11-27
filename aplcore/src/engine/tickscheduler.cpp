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

#include "apl/engine/tickscheduler.h"

#include "apl/content/content.h"
#include "apl/document/coredocumentcontext.h"
#include "apl/engine/arrayify.h"
#include "apl/engine/evaluate.h"
#include "apl/engine/propdef.h"
#include "apl/time/sequencer.h"

namespace apl
{

TickScheduler::TickScheduler(const std::shared_ptr<TimeManager>& timeManager)
    : mTimeManager(timeManager)
{}

void
TickScheduler::processTickHandlers(const CoreDocumentContextPtr& documentContext) const
{
    auto& json = documentContext->content()->getDocument()->json();
    auto it = json.FindMember("handleTick");
    if (it == json.MemberEnd())
        return;

    auto tickHandlers = asArray(documentContext->context(), evaluate(documentContext->context(), it->value));

    if (tickHandlers.empty() || !tickHandlers.isArray())
        return;

    for (const auto& handler : tickHandlers.getArray()) {
        auto delay = std::max(propertyAsDouble(documentContext->context(), handler, "minimumDelay", 1000),
                              documentContext->rootConfig().getProperty(RootProperty::kTickHandlerUpdateLimit).getDouble());
        scheduleTickHandler(std::weak_ptr<CoreDocumentContext>(documentContext), handler, delay);
    }
}

void
TickScheduler::scheduleTickHandler(const std::weak_ptr<CoreDocumentContext>& documentContext,
                                   const Object& handler,
                                   double delay) const
{
    // Lambda capture takes care of handler here as it's a copy.
    mTimeManager->setTimeout([this, documentContext, handler, delay]() {
        auto dc = documentContext.lock();
        if (!dc)
            return;

        auto ctx = dc->createDocumentContext("Tick");
        if (propertyAsBoolean(*ctx, handler, "when", true)) {
            auto commands = Object(arrayifyProperty(*ctx, handler, "commands"));
            if (!commands.empty())
                dc->context().sequencer().executeCommands(commands, ctx, nullptr, true);
        }

        scheduleTickHandler(documentContext, handler, delay);

    }, delay);
}

} // namespace apl
