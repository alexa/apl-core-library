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

#include <algorithm>

#include "apl/action/action.h"
#include "apl/time/timers.h"
#include "apl/utils/log.h"

namespace apl {

const static bool DEBUG_ACTION = false;

#ifdef DEBUG_MEMORY_USE
static int sActionNumber = 0;
#endif

/**
 * Common base class of action contracts.
 */
Action::Action(const TimersPtr& timers, TerminateFunc terminate)
    : mState(ActionState::PENDING),
      mThen(nullptr),
      mTimeoutId(0),
      mTimers(timers),
      mArgument({0})
#ifdef DEBUG_MEMORY_USE
      , mActionNumber(++sActionNumber)
#endif
{
    if (terminate)
        mTerminate.push_back(terminate);

    LOG_IF(DEBUG_ACTION) << *this;
}

Action::~Action()
{
    LOG_IF(DEBUG_ACTION) << *this << " timeout_id " << mTimeoutId;
    if (mTimeoutId) {
        mTimers->clearTimeout(mTimeoutId);
        mTimeoutId = 0;
    }
}

/**
 * Set a callback to execute when the action completes
 */
void
Action::then(ThenFunc func)
{
    LOG_IF(DEBUG_ACTION) << *this;
    mThen = std::move(func);
    doResolve();
}

void
Action::terminate()
{
    if (mState == ActionState::PENDING) {
        mState = ActionState::TERMINATED;
        onFinish();
        if (mTimeoutId) {
            mTimers->clearTimeout(mTimeoutId);
            mTimeoutId = 0;
        }

        for (const auto& f : mTerminate)
            f(mTimers);
    }
}

/*
 * Resolve the contract and fire all "then" clauses
 */
void
Action::resolve()
{
    if (mState == ActionState::PENDING) {
        mState = ActionState::RESOLVED;
        doResolve();
    }
}

/*
 * Resolve the contract and fire all "then" clauses
 */
void
Action::resolve(int argument)
{
    if (mState == ActionState::PENDING) {
        mArgument.arg = argument;
        mState = ActionState::RESOLVED;
        doResolve();
    }
}

void
Action::resolve(const Rect& argument) {
    if (mState == ActionState::PENDING) {
        mArgument.rect = argument;
        mState = ActionState::RESOLVED;
        doResolve();
    }
}

/*
 * Set a callback to be execute on termination
 */
void
Action::addTerminateCallback(TerminateFunc f)
{
    mTerminate.push_back(f);
}

void
Action::doResolve()
{
    if (mState == ActionState::RESOLVED && mThen) {
        LOG_IF(DEBUG_ACTION) << "Resolving " << *this;

        auto weak = std::weak_ptr<Action>(shared_from_this());
        mTimeoutId = mTimers->setTimeout([weak]() {
            auto self = weak.lock();
            if (self) {
                self->mTimeoutId = 0;
                if (self->mState == ActionState::RESOLVED && self->mThen) {
                    self->onFinish();
                    self->mThen(self);
                }
            }
        });
    }
}

/**
 * Make a generic action.  The StartFunc runs immediately.  If you don't pass a starting function,
 * the action is resolved immediately.
 */
ActionPtr
Action::make(const TimersPtr& timers, StartFunc func)
{
    auto ptr = std::make_shared<Action>(timers);
    if (func)
        func(ptr);
    else
        ptr->resolve();
    return ptr;
}

/**
 * Make an action that fires after a delay.  If you don't pass a starting function,
 * the action resolves after the delay.
 */

ActionPtr
Action::makeDelayed(const TimersPtr& timers, apl_duration_t delay, StartFunc func)
{
    if (delay == 0)
        return make(timers, func);

    auto ptr = std::make_shared<Action>(timers);
    auto weak = std::weak_ptr<Action>(ptr);

    ptr->mTimeoutId = timers->setTimeout([func, weak]() {
        auto self = weak.lock();
        if (self) {
            self->mTimeoutId = 0;
            if (self->isPending()) {
                if (func)
                    func(self->shared_from_this());
                else
                    self->resolve();
            }
        }
    }, delay);

    return ptr;
}

ActionPtr
Action::makeAnimation(const TimersPtr& timers, apl_duration_t delay, Timers::Animator animator)
{
    if (delay <= 0) {
        animator(0);  // Run the animation at least once
        return make(timers);  // Return a resolved action
    }

    auto ptr = std::make_shared<Action>(timers);
    auto weak = std::weak_ptr<Action>(ptr);

    ptr->mTimeoutId = timers->setAnimator([delay, animator, weak](apl_duration_t currentDelay) {
        auto self = weak.lock();
        if (self) {
            if (self->isTerminated())
                return;

            if (currentDelay < delay) {
                animator(currentDelay);
            } else {
                animator(delay);
                self->mTimeoutId = 0;
                self->resolve();
            }
        }
    }, delay);

    return ptr;
}


class Collection : public Action {
protected:
    ActionList mActions;

public:
    Collection(const TimersPtr& timers, const ActionList& actionList)
            : Action(timers)
    {
        for (const auto& action : actionList)
            if (!action->isTerminated())
                mActions.push_back(action);
    }

    void
    removeAction(const ActionPtr& ptr)
    {
        auto it = std::find(mActions.begin(), mActions.end(), ptr);
        assert(it != mActions.end());
        mActions.erase(it);
    }

    void
    terminateRemaining()
    {
        for (auto& p : mActions)
            p->terminate();
        mActions.clear();
    }

    unsigned int
    size()
    {
        return mActions.size();
    }

    ActionList&
    actions()
    {
        return mActions;
    }
};

ActionPtr
Action::makeAll(const TimersPtr& timers, const ActionList &actionList)
{
    auto ptr = std::make_shared<Collection>(timers, actionList);
    std::weak_ptr<Collection> weak_ptr(ptr);
    ptr->addTerminateCallback([weak_ptr](const TimersPtr&) {
        if (auto self = weak_ptr.lock())
           self->terminateRemaining();
    });
    if (ptr->size() == 0)
        return make(timers);

    for (auto& action : ptr->actions()) {
        action->then([weak_ptr](const ActionPtr& p) {
            auto ptr = weak_ptr.lock();
            if (ptr && !ptr->isTerminated()) {
                ptr->removeAction(p);
                if (ptr->size() == 0)
                    ptr->resolve();
            }
        });
    }

    return std::static_pointer_cast<Action>(ptr);
}

ActionPtr
Action::makeAny(const TimersPtr& timers, const ActionList &actionList)
{
    auto ptr = std::make_shared<Collection>(timers, actionList);
    std::weak_ptr<Collection> weak_ptr(ptr);
    ptr->addTerminateCallback([weak_ptr](const TimersPtr&) {
        if (auto self = weak_ptr.lock())
            self->terminateRemaining();
    });

    if (ptr->size() == 0)
        return make(timers);

    for (auto& action : ptr->actions()) {
        action->then([weak_ptr](const ActionPtr& a) {
            auto ptr = weak_ptr.lock();
            if (ptr && !ptr->isTerminated()) {
                ptr->removeAction(a);
                ptr->terminateRemaining();
                ptr->resolve();
            }
        });
    }

    return std::static_pointer_cast<Action>(ptr);
}

class WrappedAction : public Action {
public:
    ActionPtr action;
    CallbackFunc callback;

    WrappedAction(const TimersPtr& timers, const ActionPtr& act, CallbackFunc&& cb)
        : Action(timers), action(act), callback(std::move(cb))
    {}
};

ActionPtr
Action::wrapWithCallback(const TimersPtr& timers, const ActionPtr& action, CallbackFunc&& callback)
{
    if (!action->isPending()) {
        callback(action->isResolved(), action);
        return action;
    }

    auto ptr = std::make_shared<WrappedAction>(timers, action, std::move(callback));
    std::weak_ptr<WrappedAction> weak_ptr(ptr);
    ptr->addTerminateCallback([weak_ptr](const TimersPtr&) {
        if (auto self = weak_ptr.lock()) {
            self->callback(false, self->action);
            self->action->terminate();
        }
    });

    action->then([weak_ptr](const ActionPtr& actionPtr) {
       auto self = weak_ptr.lock();
       if (self) {
           self->callback(true, actionPtr);
           self->resolve();
       }
    });

    return ptr;
}

streamer&
operator<<(streamer& os, Action& action)
{
    os << "Action<"
#ifdef DEBUG_MEMORY_USE
        << action.mActionNumber << ">";
#else
        << (void *) &action << ">";
#endif
    return os;
}
} // namespace apl
