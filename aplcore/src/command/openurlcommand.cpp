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

#include "apl/command/openurlcommand.h"
#include "apl/action/openurlaction.h"
#include "apl/content/rootconfig.h"
#include "apl/utils/session.h"

namespace apl {

const CommandPropDefSet&
OpenURLCommand::propDefSet() const {
    static CommandPropDefSet sOpenURLCommandProperties(CoreCommand::propDefSet(), {
            {kCommandPropertyOnFail, Object::EMPTY_ARRAY(), asArray },
            {kCommandPropertySource, "",                    asString,  kPropRequired},
    });

    return sOpenURLCommandProperties;
}

ActionPtr
OpenURLCommand::execute(const TimersPtr& timers, bool fastMode) {
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

} // namespace apl