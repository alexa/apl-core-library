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

#include "apl/command/commandproperties.h"
#include "apl/action/speakitemaction.h"
#include "apl/action/scrolltoaction.h"
#include "apl/command/corecommand.h"
#include "apl/time/sequencer.h"

namespace apl {

SpeakItemAction::SpeakItemAction(const TimersPtr& timers, const std::shared_ptr<CoreCommand>& command,
        const CoreComponentPtr& target)
        : ResourceHoldingAction(timers, command->context()),
          mCommand(command),
          mTarget(target)
{
    addTerminateCallback([this](const TimersPtr&) {
        if (mCurrentAction) {
            mCurrentAction->terminate();
            mCurrentAction = nullptr;
        }
    });
}

std::shared_ptr<SpeakItemAction>
SpeakItemAction::make(const TimersPtr& timers,
                      const std::shared_ptr<CoreCommand>& command,
                      const CoreComponentPtr& target)
{
    auto t = target ? target : command->target();
    if (!t)
        return nullptr;

    auto ptr = std::make_shared<SpeakItemAction>(timers, command, t);
    ptr->start();
    return ptr;
}

void
SpeakItemAction::start()
{
    auto context = mCommand->context();
    context->sequencer().claimResource(kExecutionResourceForegroundAudio, shared_from_this());

    // Start by sending a pre-roll event
    mSource = mTarget->getCalculated(kPropertySpeech).asString();
    if (!mSource.empty()) {
        EventBag bag;
        bag.emplace(kEventPropertySource, mSource);
        context->pushEvent(Event(kEventTypePreroll, std::move(bag), mTarget));
    }

    // If we need line bounds send an event to the viewhost. This viewhost resolves this action with
    // the bounds of the first line of text.
    auto scrollable = mTarget->getParent();
    while (scrollable && scrollable->scrollType() == kScrollTypeNone)
        scrollable = scrollable->getParent();
    if(scrollable && mTarget->getType() == kComponentTypeText
        && mCommand->getValue(kCommandPropertyHighlightMode) == kCommandHighlightModeLine) {
        mCurrentAction = Action::make(timers(), [this](ActionRef ref) {
            mCommand->context()->pushEvent(Event(kEventTypeRequestFirstLineBounds, mTarget, ref));
        });
        std::weak_ptr<SpeakItemAction> weak_ptr(std::static_pointer_cast<SpeakItemAction>(shared_from_this()));
        mCurrentAction->then([weak_ptr, this](const ActionPtr& actionPtr){
            auto self = weak_ptr.lock();
            if (!self)
                return;
            Rect bounds = actionPtr->getRectArgument();
            auto scrollAction = ScrollToAction::make(timers(), mCommand, bounds, mTarget);
            scroll(scrollAction);
        });
    }
    else {
        auto scrollAction = ScrollToAction::make(timers(), mCommand, mTarget);
        scroll(scrollAction);
    }
}


void
SpeakItemAction::scroll(const std::shared_ptr<ScrollToAction>& action) {
    // Next scroll the component into view
    mCurrentAction = action;
    if (mCurrentAction) {
        std::weak_ptr<SpeakItemAction> weak_ptr(std::static_pointer_cast<SpeakItemAction>(shared_from_this()));
        mCurrentAction->then([weak_ptr](const ActionPtr& actionPtr) {
            auto self = weak_ptr.lock();
            if (!self)
                return;

            self->advance();
        });

        // If scroll was killed by conflicting operation - kill whole SpeakItem.
        mCurrentAction->addTerminateCallback([weak_ptr](const TimersPtr&) {
            auto self = weak_ptr.lock();
            if (!self)
                return;

            self->terminate();
        });
    }
    else {
        advance();
    }
}

void
SpeakItemAction::advance()
{
    mCurrentAction = nullptr;

    ActionPtr dwellAction = nullptr;
    ActionPtr speakAction = nullptr;

    // Construct the speak action
    if (!mSource.empty()) {
        speakAction = Action::make(timers(), [this](ActionRef ref) {
            EventBag bag;
            bag.emplace(kEventPropertySource, mSource);
            bag.emplace(kEventPropertyHighlightMode, mCommand->getValue(kCommandPropertyHighlightMode).getInteger());
            bag.emplace(kEventPropertyAlign, mCommand->getValue(kCommandPropertyAlign).getInteger());
            mCommand->context()->pushEvent(Event(kEventTypeSpeak, std::move(bag), mTarget, ref));
        });
    }

    // Construct the dwell action
    auto minDwell = mCommand->getValue(kCommandPropertyMinimumDwellTime).asInt();
    if (minDwell > 0)
        dwellAction = Action::makeDelayed(timers(), minDwell);

    // Consolidate to a single action
    mCurrentAction = speakAction;
    if (dwellAction)
        mCurrentAction = mCurrentAction ? Action::makeAll(timers(), {mCurrentAction, dwellAction}) : dwellAction;

    // We didn't have a speak or a dwell; resolve and return immediately (see APLSpecification::SpeakItem Command)
    if (!mCurrentAction) {
        resolve();
        return;
    }

    // We have an action.  Set the karaoke state on the target
    mTarget->setState(kStateKaraoke, true);

    std::weak_ptr<SpeakItemAction> weak_ptr(std::static_pointer_cast<SpeakItemAction>(shared_from_this()));

    // Wait for the action to finish and then clear karaoke
    mCurrentAction->then([weak_ptr](const ActionPtr& actionPtr) {
        auto self = weak_ptr.lock();
        if (!self)
            return;

        self->mTarget->setState(kStateKaraoke, false);
        self->resolve();
    });

    // If we are terminated early, we still need to clear the karaoke state
    mCurrentAction->addTerminateCallback([weak_ptr](const TimersPtr&) {
        auto self = weak_ptr.lock();
        if (!self)
            return;

        self->mTarget->setState(kStateKaraoke, false);
    });
}


} // namespace apl