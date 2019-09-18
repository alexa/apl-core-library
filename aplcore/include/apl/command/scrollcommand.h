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

#ifndef _APL_SCROLL_COMMAND_H
#define _APL_SCROLL_COMMAND_H

#include "apl/command/corecommand.h"
#include "apl/utils/session.h"

namespace apl {

const static bool DEBUG_SCROLL_COMMAND = false;

class ScrollCommand : public CoreCommand {
public:
    static CommandPtr create(const ContextPtr& context,
                             Properties&& properties,
                             const CoreComponentPtr& base) {
        auto ptr = std::make_shared<ScrollCommand>(context, std::move(properties), base);
        return ptr->validate() ? ptr : nullptr;
    }

    ScrollCommand(const ContextPtr& context, Properties&& properties, const CoreComponentPtr& base)
            : CoreCommand(context, std::move(properties), base)
    {}

    const CommandPropDefSet& propDefSet() const override {
        static CommandPropDefSet sScrollCommandProperties(CoreCommand::propDefSet(), {
                {kCommandPropertyComponentId, "", asString,                   kPropRequiredId},
                {kCommandPropertyDistance,    0,  asNonAutoRelativeDimension},
        });

        return sScrollCommandProperties;
    };

    CommandType type() const override { return kCommandTypeScroll; }

    ActionPtr execute(const TimersPtr& timers, bool fastMode) override {
        if (fastMode) {
            CONSOLE_CTP(mContext) << "Ignoring Scroll in fast mode";
            return nullptr;
        }

        if (!calculateProperties())
            return nullptr;

        if (!mTarget || mTarget->scrollType() == kScrollTypeNone) {
            CONSOLE_CTP(mContext) << "Attempting to scroll non-scrollable component";
            return nullptr;
        }

        auto self = std::static_pointer_cast<CoreCommand>(shared_from_this());

        return Action::make(timers, [self](ActionRef ref) {
            EventBag bag;
            for (const auto& it : self->propDefSet()) {
                if ((it.second.flags & kPropOut) != 0)
                    bag.emplace(it.second.key, self->getValue(it.second.key));
            }

            // Now set the position
            auto target = self->target();
            auto vertical = (target->scrollType() == kScrollTypeVertical);
            auto innerBounds = target->getCalculated(kPropertyInnerBounds).getRect();

            auto targetSize = (vertical ? innerBounds.getHeight() : innerBounds.getWidth());
            auto prop = self->getValue(kCommandPropertyDistance);
            float distance = 0;

            if (prop.isRelativeDimension())
                distance = prop.getRelativeDimension() * targetSize / 100;
            else if (prop.isAbsoluteDimension())
                distance = prop.getAbsoluteDimension();

            // Calculate the new position by trimming the old position plus the distance
            auto position = target->trimScroll(target->scrollPosition() + Point(distance, distance));
            bag.emplace(kEventPropertyPosition, Dimension(vertical ? position.getY() : position.getX()));

            LOG_IF(DEBUG_SCROLL_COMMAND) << "Pushing scroll event position=" << position;
            self->context()->pushEvent(Event(kEventTypeScrollTo, std::move(bag), self->target(), ref));
        });
    }
};

} // namespace apl

#endif // _APL_SCROLL_COMMAND_H
