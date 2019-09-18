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

#ifndef _APL_SCROLL_TO_INDEX_COMMAND_H
#define _APL_SCROLL_TO_INDEX_COMMAND_H

#include "apl/command/corecommand.h"
#include "apl/action/scrolltoaction.h"

namespace apl {

class ScrollToIndexCommand : public CoreCommand {
public:
    static CommandPtr create(const ContextPtr& context,
                             Properties&& properties,
                             const CoreComponentPtr& base) {
        auto ptr = std::make_shared<ScrollToIndexCommand>(context, std::move(properties), base);
        return ptr->validate() ? ptr : nullptr;
    }

    ScrollToIndexCommand(const ContextPtr& context, Properties&& properties, const CoreComponentPtr& base)
            : CoreCommand(context, std::move(properties), base)
    {}

    const CommandPropDefSet& propDefSet() const override {
        static CommandPropDefSet sScrollToIndexCommandProperties(CoreCommand::propDefSet(), {
                {kCommandPropertyAlign,       kCommandScrollAlignVisible, sCommandAlignMap },
                {kCommandPropertyComponentId, "",                         asString,         kPropRequiredId},
                {kCommandPropertyIndex,       0,                          asInteger,        kPropRequired},
        });

        return sScrollToIndexCommandProperties;
    };

    CommandType type() const override { return kCommandTypeScrollToIndex; }

    ActionPtr execute(const TimersPtr& timers, bool fastMode) override {
        if (fastMode) {
            CONSOLE_CTP(mContext) << "Ignoring ScrollToIndex in fast mode";
            return nullptr;
        }

        if (!calculateProperties())
            return nullptr;

        // Switch the mTarget component to point to the thing being scrolled
        auto childIndex = mValues.at(kCommandPropertyIndex).getInteger();
        auto childCount = mTarget->getChildCount();
        childIndex = childIndex < 0 ? childIndex + childCount : childIndex;
        if (childIndex >= childCount || childIndex < 0) {
            CONSOLE_CTP(mContext) << "ScrollToIndex invalid child index=" << childIndex;
            return nullptr;
        }

        mTarget = mTarget->getCoreChildAt(childIndex);

        return ScrollToAction::make(timers, std::static_pointer_cast<CoreCommand>(shared_from_this()));
    }
};

} // namespace apl

#endif // _APL_SCROLL_TO_INDEX_COMMAND_H
