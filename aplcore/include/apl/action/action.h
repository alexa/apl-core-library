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

#ifndef _APL_ACTION_H
#define _APL_ACTION_H

#include <cassert>
#include <functional>
#include <memory>
#include <vector>

#include "apl/common.h"
#include "apl/time/timers.h"
#include "apl/utils/counter.h"
#include "apl/utils/noncopyable.h"
#include "apl/utils/streamer.h"
#include "apl/utils/userdata.h"
#include "apl/primitives/rect.h"

namespace apl {

// Forward declarations
class ActionRef;

using ActionList = std::vector<ActionPtr>;

using CallbackFunc = std::function< void(bool,const ActionPtr&) >;
using StartFunc = std::function< void(ActionRef) >;
using ThenFunc  = std::function< void(const ActionPtr&) >;
using TerminateFunc = std::function< void(const TimersPtr&) >;

union ActionResolveArg {
    int arg;
    Rect rect;
};

/**
 * Common base class of action contracts.
 */
class Action : public std::enable_shared_from_this<Action>,
               public Counter<Action>,
               public NonCopyable,
               public UserData<Action> {

public:
    /**
     * Make a generic action.  The StartFunc runs immediately.  If you don't pass a starting function,
     * the action is resolved immediately.
     * @param timers
     * @param func
     * @return The action
     */
    static ActionPtr make(const TimersPtr& timers, StartFunc func = nullptr);

    /**
     * Make an action that fires after a delay.  If you don't pass a starting function,
     * the action resolves after the delay.
     * @param timers
     * @param delay
     * @param func
     * @return The action
     */
    static ActionPtr makeDelayed(const TimersPtr& timers, apl_duration_t delay, StartFunc func = nullptr);

    /**
     * Make an action that resolves after all of the child actions resolve.
     * @param timers
     * @param actionList A list of actions.
     * @return The tail of this action
     */
    static ActionPtr makeAll(const TimersPtr& timers, const ActionList &actionList);

    /**
     * Make an action that resolves after any of the child actions resolve.
     * The other child actions are terminated.
     * @param timers
     * @param actionList A list of actions
     * @return The tail of this action.
     */
    static ActionPtr makeAny(const TimersPtr& timers, const ActionList &actionList);

    /**
     * Make an action that runs an animation.  The animator function is called as time is advanced
     * up to and including when the duration is reached.  It is _not_ called for a time of zero.
     * @param timers
     * @param duration The duration of the animation.
     * @param animator The function to call up to and including when the duration time is reached.
     * @return The animation action
     */
    static ActionPtr makeAnimation(const TimersPtr& timers,
                                   apl_duration_t duration,
                                   Timers::Animator animator);

    /**
     * Wrap provided action with another one that will call a callback when provided resolved. If
     * provided action is terminated wrap action will also be terminated.
     * @param timers
     * @param action action to wrap.
     * @param callback Callback to call.
     * @return Wrap action.
     */
    static ActionPtr wrapWithCallback(const TimersPtr& timers,
                                      const ActionPtr& action,
                                      CallbackFunc&& callback);

public:
    explicit Action(const TimersPtr& timers, TerminateFunc terminate = nullptr);
    ~Action() override;

    Action& operator=(const Action&) = delete;
    Action(const Action&) = delete;

    /**
     * Set a callback function to execute when this action finishes.  This can be called
     * after the action resolves and the function will still be executed.  The callback
     * is placed on the timer loop with 0 delay.
     * @param func The callback function.
     */
    void then(ThenFunc func);

    /**
     * Terminate the action prematurely.  This will immediately call any registered
     * termination functions.
     */
    void terminate();

    /**
     * Resolve the action. The "then" callback will be executed if the action was not
     * already resolved or terminated.
     */
    void resolve();

    /**
     * Resolve the action passing an argument for later use.
     * @param argument The argument
     */
    void resolve(int argument);

    /**
     * Resolve with a rect. Used to pass back a bounds for the first line of a text component
     * during line karaoke.
     * @param argument
     */
    void resolve(const Rect& argument);

    /**
     * Add a terminate callback.  If the action has already been terminated, this method
     * will not be called.
     * @param terminateFunc The termination callback.
     */
    void addTerminateCallback(TerminateFunc terminateFunc);

     // Convenience methods to introspect current state

    /**
     * @return True if this action is still pending and has not resolved or terminated.
     */
    bool isPending() const { return mState == ActionState::PENDING; }

    /**
     * @return True if this action was terminated.
     */
    bool isTerminated() const { return mState == ActionState::TERMINATED; }

    /**
     * @return True if this action has resolved.
     */
    bool isResolved() const { return mState == ActionState::RESOLVED; }

    /**
     * @return The common timers object for scheduling timeouts.
     */
    const TimersPtr& timers() const { return mTimers; }

    /**
     * @return The resolve-supplied argument.  If it wasn't set, this returns 0.
     */
    int getIntegerArgument() const { return mArgument.arg; }
    Rect getRectArgument() const { return mArgument.rect; }

    friend streamer& operator<<(streamer&, Action&);

protected:
    virtual void onFinish() {}

private:
    void doResolve();

private:
    enum class ActionState { PENDING, RESOLVED, TERMINATED };
    ActionState mState;
    ThenFunc mThen;
    std::vector<TerminateFunc> mTerminate;
    timeout_id mTimeoutId;
    const TimersPtr mTimers;
    ActionResolveArg mArgument;
#ifdef DEBUG_MEMORY_USE
    int mActionNumber;   // Individually number actions to simplify debugging
#endif
};

/**
 * The ActionRef is passed into the user function; the user is expected to
 * call resolve() eventually.
 */
class ActionRef  {
public:
    ActionRef(const ActionPtr& ptr) : mPtr(ptr) {}
    ActionRef(const ActionRef& ref) = default;

    /**
     * Resolve the action
     */
    void resolve() const { mPtr->resolve(); }

    /**
     * Resolve the action with a union
     * @param argument
     */
    void resolve(const Rect& argument) const { mPtr->resolve(argument); }

    /**
     * Resolve the action with an argument
     * @param argument The argument
     */
    void resolve(int argument) const { mPtr->resolve(argument); }

    /**
     * Attach a terminate callback to the action.
     * @param terminateFunc The termination callback
     */
    void addTerminateCallback(TerminateFunc terminateFunc) const { mPtr->addTerminateCallback(std::move(terminateFunc)); }

    /**
     * @return True if this action is still pending and has not resolved or terminated.
     */
    bool isPending() const { return mPtr->isPending(); }

    /**
     * @return True if this action was terminated.
     */
    bool isTerminated() const { return mPtr->isTerminated(); }

    /**
     * @return True if this action has resolved.
     */
    bool isResolved() const { return mPtr->isResolved(); }

    /**
     * @return The common timers object for scheduling timeouts.
     */
    const TimersPtr& timers() const { return mPtr->timers(); }

    /**
     * @return True if there is no action associated with this action ref.
     */
    bool isEmpty() const { return mPtr == nullptr; }

    /**
     * Attach a chunk of user data to this action
     * @param userData The user data to attach
     */
    void setUserData(void *userData) { mPtr->setUserData(userData); }

    /**
     * @return The user data attached to the action
     */
    void * getUserData() const { return mPtr->getUserData(); }

protected:
    ActionPtr mPtr;
};

} // namespace apl

#endif // _APL_ACTION_H
