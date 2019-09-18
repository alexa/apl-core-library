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

#ifndef _APL_OPEN_URL_COMMAND_H
#define _APL_OPEN_URL_COMMAND_H

#include "apl/command/corecommand.h"
#include "apl/action/openurlaction.h"
#include "apl/content/rootconfig.h"

namespace apl {

class OpenURLCommand : public CoreCommand {
public:
    static CommandPtr create(const ContextPtr& context,
                             Properties&& properties,
                             const CoreComponentPtr& base) {
        auto ptr = std::make_shared<OpenURLCommand>(context, std::move(properties), base);
        return ptr->validate() ? ptr : nullptr;
    }

    OpenURLCommand(const ContextPtr& context, Properties&& properties, const CoreComponentPtr& base)
            : CoreCommand(context, std::move(properties), base)
    {}

    const CommandPropDefSet& propDefSet() const override {
        static CommandPropDefSet sOpenURLCommandProperties(CoreCommand::propDefSet(), {
                {kCommandPropertyOnFail, Object::EMPTY_ARRAY(), asArray },
                {kCommandPropertySource, "",                    asString,  kPropRequired},
        });

        return sOpenURLCommandProperties;
    };

    CommandType type() const override { return kCommandTypeOpenURL; }

    ActionPtr execute(const TimersPtr& timers, bool fastMode) override
    {
        if (fastMode) {
            CONSOLE_CTP(mContext) << "Ignoring OpenURL in fast mode";
            return nullptr;
        }

        if (!calculateProperties())
            return nullptr;

        auto self = std::static_pointer_cast<CoreCommand>(shared_from_this());
        auto allowed = mContext->getRootConfig().getAllowOpenUrl();
        if(!allowed) {
            // 405 - HTTP code for web "Method Not Allowed".
            return OpenURLAction::makeFailed(timers, self, 405);
        }

        auto action = Action::make(timers, [this](ActionRef ref) {
            EventBag bag;
            bag.emplace(kEventPropertySource, mValues.at(kCommandPropertySource));
            mContext->pushEvent(Event(kEventTypeOpenURL, std::move(bag), mTarget, ref));
        });

        return OpenURLAction::make(timers, self, std::move(action));
    }
};

} // namespace apl

#endif // _APL_OPENURLCOMMAND_H
