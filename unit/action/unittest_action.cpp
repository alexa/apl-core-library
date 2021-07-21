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

#include <queue>

#include "../testeventloop.h"

using namespace apl;

static void connect(ActionPtr& p, bool& fired)
{
    p->then([&](const ActionPtr&){fired = true;});
}

static void connect(ActionPtr& p, bool& fired, int& argument)
{
    p->then([&](const ActionPtr&ptr){fired = true; argument = ptr->getIntegerArgument(); });
}

static void connect(ActionPtr& p, bool& fired, Rect& argument)
{
    p->then([&](const ActionPtr&ptr){fired = true; argument = ptr->getRectArgument(); });
}


class ActionTest : public ActionWrapper {
public:
    // Simulate starting an action that resolves immediately
    ActionPtr
    fake_action()
    {
        return Action::make(loop, [](ActionRef action) {
            action.resolve();
        });
    }

    // Simulate starting an action that resolves immediately with an argument
    ActionPtr
    fake_action_argument(int argument)
    {
        return Action::make(loop, [=](ActionRef action) {
            action.resolve(argument);
        });
    }

    // Simulate starting an action that resolves immediately with an argument
    ActionPtr
    fake_action_argument(Rect rect)
    {
        return Action::make(loop, [=](ActionRef action) {
            action.resolve(rect);
        });
    }

    // Simulate starting an action that resolves slowly
    ActionPtr
    fake_action(int duration)
    {
        return Action::make(loop, [duration](ActionRef action) {
            timeout_id id = action.timers()->setTimeout([=]() {
                action.resolve();
            }, duration);

            action.addTerminateCallback([id](const TimersPtr& timers) {
               timers->clearTimeout(id);
            });
        });
    }

    // Simulate delay-starting an action that resolves immediately
    ActionPtr
    fake_delayed_action(int delay)
    {
        return Action::makeDelayed(loop, delay, [=](ActionRef action) {
            action.resolve();
        });
    }

    // Simulate delay-starting an action that resolves slowly
    ActionPtr
    fake_delayed_action(int delay, int duration)
    {
        return Action::makeDelayed(loop, delay, [=](ActionRef action) {
            timeout_id id = action.timers()->setTimeout([=]() {
                action.resolve();
            }, duration);

            action.addTerminateCallback([id](const TimersPtr& timers) {
                timers->clearTimeout(id);
            });
        });
    }
};

/********************* Start of actual tests ******************/

TEST_F(ActionTest, MakeResolved)
{
    auto p = Action::make(loop);

    bool fired = false;
    connect(p, fired);
    ASSERT_FALSE(fired);

    // There should be a "resolved" pending on the event loop
    ASSERT_EQ(1, loop->size());
    loop->advance();

    ASSERT_TRUE(fired);
}

TEST_F(ActionTest, MakeActionAndResolve)
{
    auto p = fake_action();

    bool fired = false;
    connect(p, fired);
    ASSERT_FALSE(fired);

    // There should be a "resolved" pending on the event loop
    ASSERT_EQ(1, loop->size());
    loop->advance();

    ASSERT_TRUE(fired);
}

TEST_F(ActionTest, MakeWrappedActionAndResolve)
{
    auto p = fake_delayed_action(100);

    bool fired = false;
    bool result = false;
    auto wrapped = Action::wrapWithCallback(loop, p, [&] (bool res, const ActionPtr&) {
        fired = true;
        result = res;
    });

    ASSERT_FALSE(fired);

    // There should be a "resolved" pending on the event loop
    ASSERT_EQ(1, loop->size());
    loop->advanceToEnd();

    ASSERT_TRUE(fired);
    ASSERT_TRUE(result);
}

TEST_F(ActionTest, MakeWrappedActionAndTerminate)
{
    auto p = fake_delayed_action(100);

    bool fired = false;
    bool result = false;
    auto wrapped = Action::wrapWithCallback(loop, p, [&] (bool res, const ActionPtr&) {
      fired = true;
      result = res;
    });

    ASSERT_FALSE(fired);

    wrapped->terminate();

    ASSERT_TRUE(fired);
    ASSERT_FALSE(result);
}

TEST_F(ActionTest, MakeActionAndResolveArgument)
{
    auto p = fake_action_argument(23);

    bool fired = false;
    int arg = 0;
    connect(p, fired, arg);
    ASSERT_FALSE(fired);

    // There should be a "resolved" pending on the event loop
    ASSERT_EQ(1, loop->size());
    loop->advance();

    ASSERT_TRUE(fired);
    ASSERT_EQ(23, arg);
}

TEST_F(ActionTest, MakeActionAndResolveRect)
{
    Rect rect(10, 20, 30, 40);
    auto p = fake_action_argument(rect);

    bool fired = false;
    Rect resolvedRect;
    connect(p, fired, resolvedRect);
    ASSERT_FALSE(fired);

    // There should be a "resolved" pending on the event loop
    ASSERT_EQ(1, loop->size());
    loop->advance();

    ASSERT_TRUE(fired);
    ASSERT_EQ(rect, resolvedRect);
}


TEST_F(ActionTest, DelayedMakeActionAndResolve)
{
    auto p = fake_delayed_action(100);

    bool fired = false;
    connect(p, fired);
    ASSERT_FALSE(fired);

    // Need to advance pass the 100 ms timer
    ASSERT_EQ(1, loop->size());
    loop->advance();

    ASSERT_EQ(100, loop->currentTime());
    ASSERT_TRUE(fired);
    ASSERT_EQ(0, loop->size());
}

// Same as above, but terminate it before the timer goes off
TEST_F(ActionTest, DelayedActionTerminate)
{
    auto p = fake_delayed_action(100);

    bool fired = false;
    connect(p, fired);
    ASSERT_FALSE(fired);

    ASSERT_EQ(1, loop->size());

    // Terminate it before it has a chance to fire
    p->terminate();
    ASSERT_EQ(0, loop->size());
    ASSERT_FALSE(fired);
}

// Same as above, but this time we terminate after the delay, but before the resolve()
TEST_F(ActionTest, DelayedActionTerminate2)
{
    auto p = fake_delayed_action(100, 100);

    // First, we have to wait 100 units before the timer finishes
    bool fired = false;
    connect(p, fired);
    ASSERT_FALSE(fired);

    ASSERT_EQ(1, loop->size());
    loop->advance();

    // At this point we have gotten past the timer, but we haven't resolved (that's another 100)
    ASSERT_EQ(1, loop->size());
    ASSERT_FALSE(p->isTerminated());

    p->terminate();   // This should terminate and release the internal timer

    ASSERT_TRUE(p->isTerminated());
    ASSERT_FALSE(fired);
}

TEST_F(ActionTest, MakeAll)
{
    ActionList plist({
        Action::make(loop),
        fake_action(),
        fake_action(100)
    });

    auto p = Action::makeAll(loop, plist);

    loop->advanceToEnd();

    ASSERT_TRUE(p->isResolved());

    ASSERT_TRUE(plist[0]->isResolved());
    ASSERT_TRUE(plist[1]->isResolved());
}


TEST_F(ActionTest, MakeAllTerminate)
{
    ActionList plist({
        Action::make(loop),
        fake_action(50),
        fake_action(100),
        fake_delayed_action(75, 75)
    });

    auto p = Action::makeAll(loop, plist);

    loop->advanceToTime(75);
    ASSERT_FALSE(p->isResolved());  // Should be two left
    ASSERT_TRUE(plist[0]->isResolved());
    ASSERT_TRUE(plist[1]->isResolved());
    ASSERT_TRUE(plist[2]->isPending());
    ASSERT_TRUE(plist[3]->isPending());

    p->terminate();

    ASSERT_TRUE(p->isTerminated());
    ASSERT_TRUE(plist[0]->isResolved());
    ASSERT_TRUE(plist[1]->isResolved());
    ASSERT_TRUE(plist[2]->isTerminated());
    ASSERT_TRUE(plist[3]->isTerminated());
}

TEST_F(ActionTest, MakeAllEmpty)
{
    ActionList plist;
    auto p = Action::makeAll(loop, plist);
    loop->advanceToEnd();
    ASSERT_TRUE(p->isResolved());
}

TEST_F(ActionTest, MakeAny)
{
    ActionList plist({
        fake_action(),
        fake_action(100)
    });

    auto p = Action::makeAny(loop, plist);

    loop->advanceToEnd();

    ASSERT_TRUE(p->isResolved());
    ASSERT_TRUE(plist[0]->isResolved());
    ASSERT_TRUE(plist[1]->isTerminated());
}

TEST_F(ActionTest, MakeAnyTerminate)
{
    ActionList plist({
        fake_action(),
        fake_action(100)
    });

    auto p = Action::makeAny(loop, plist);

    p->terminate();

    ASSERT_TRUE(p->isTerminated());
    ASSERT_TRUE(plist[0]->isResolved());
    ASSERT_TRUE(plist[1]->isTerminated());
}

TEST_F(ActionTest, MakeAnyEmpty)
{
    ActionList plist;
    auto p = Action::makeAny(loop, plist);
    loop->advanceToEnd();
    ASSERT_TRUE(p->isResolved());
}

TEST_F(ActionTest, UserData)
{
    auto p = Action::make(loop);
    bool stashed = false;
    p->setUserData(&stashed);
    p->then([](const ActionPtr& ptr){
        *(static_cast<bool *>(ptr->getUserData())) = true;
    });

    ASSERT_FALSE(stashed);
    // There should be a "resolved" pending on the event loop
    ASSERT_EQ(1, loop->size());
    loop->advance();
    ASSERT_TRUE(stashed);
}

TEST_F(ActionTest, UserDataDelayed)
{
    auto p = Action::makeDelayed(loop, 1000);
    bool stashed = false;
    p->setUserData(&stashed);
    p->then([](const ActionPtr& ptr){
        *(static_cast<bool *>(ptr->getUserData())) = true;
    });

    ASSERT_FALSE(stashed);
    loop->advanceToEnd();
    ASSERT_TRUE(stashed);
}

#ifdef USER_DATA_RELEASE_CALLBACKS
TEST_F(ActionTest, UserDataRelease)
{
    int releaseCount = 0;
    Action::setUserDataReleaseCallback([&releaseCount](void *) {
        releaseCount += 1;
    });
    auto p = Action::make(loop);
    ASSERT_EQ(0, releaseCount);
    p = nullptr;
    ASSERT_EQ(1, releaseCount);
    Action::setUserDataReleaseCallback(nullptr);  // Unset to ensure it doesn't leak into other tests
}
#endif

TEST_F(ActionTest, Animation)
{
    int count = 0;
    apl_duration_t lastTimeout = 0;
    bool done = false;

    auto p = Action::makeAnimation(loop, 1000, [&](apl_duration_t delay) {
        count += 1;
        lastTimeout = delay;
    });

    p->then([&](const ActionPtr& ptr) {
        done = true;
    });

    ASSERT_EQ(1, loop->size());
    for (int i = 0, j = 0 ; i <= 1000 ; i+= 100, j++) {
        loop->advanceToTime(i);
        ASSERT_EQ(j, count);
        ASSERT_EQ(i, lastTimeout);
        ASSERT_EQ(done, i >= 1000) << " i=" << i;
    }

    ASSERT_EQ(0, loop->size());
}

TEST_F(ActionTest, AnimationNonZeroOffset)
{
    apl_duration_t lastTimeout = 0;
    loop->advanceToTime(12345);  // Move forward in time

    auto p = Action::makeAnimation(loop, 1000, [&](apl_duration_t delay) {
        lastTimeout = delay;
    });

    auto startTime = loop->currentTime();
    for (int i = 0 ; i <= 1000 ; i+= 250) {
        loop->advanceToTime(startTime + i);
        ASSERT_EQ(i, lastTimeout);
    }

    ASSERT_EQ(0, loop->size());
}

// Stagger the animation times so that they don't end exactly at the duration
TEST_F(ActionTest, AnimationWithTimeOffset)
{
    int count = 0;
    apl_duration_t lastTimeout = 0;
    bool done = false;

    auto p = Action::makeAnimation(loop, 1000, [&](apl_duration_t delay) {
        count += 1;
        lastTimeout = delay;
    });

    p->then([&](const ActionPtr& ptr) {
        done = true;
    });

    ASSERT_EQ(1, loop->size());
    for (int i = 50, j = 1 ; i <= 2000 ; i+= 100, j++) {
        loop->advanceToTime(i);

        if (i <= 1000) {
            ASSERT_EQ(i, lastTimeout);
            ASSERT_EQ(j, count);
            ASSERT_FALSE(done);
        }
        else {
            ASSERT_EQ(1000, lastTimeout);  // The last timeout should lock onto the duration
            ASSERT_EQ(11, count);  // The number of timeouts doesn't change [50,150,250,...,950,1000]
            ASSERT_TRUE(done);
        }
    }

    ASSERT_EQ(0, loop->size());
}

TEST_F(ActionTest, AnimationStop)
{
    int count = 0;
    apl_duration_t lastTimeout = 0;
    bool done = false;
    bool terminated = false;

    auto p = Action::makeAnimation(loop, 1000, [&](apl_duration_t delay) {
        ASSERT_FALSE(done);
        ASSERT_FALSE(terminated);
        count += 1;
        lastTimeout = delay;
    });

    p->then([&](const ActionPtr& ptr) {
        ASSERT_FALSE(terminated);
        done = true;
    });

    p->addTerminateCallback([&](const TimersPtr& timers) {
        ASSERT_FALSE(terminated);
        terminated = true;
    });

    ASSERT_EQ(1, loop->size());
    for (int i = 0, j = 0 ; i <= 500 ; i+= 100, j++) {
        loop->advanceToTime(i);
        ASSERT_EQ(j, count);
        ASSERT_EQ(i, lastTimeout);
        ASSERT_FALSE(done);
    }

    p->terminate();
    ASSERT_EQ(0, loop->size());
    ASSERT_TRUE(terminated);
    ASSERT_EQ(500, lastTimeout);
}


TEST_F(ActionTest, DoubleResolve)
{
    int resolve = 0;
    int terminate = 0;

    auto p = Action::make(loop, [](ActionRef action) {});

    p->addTerminateCallback([&terminate](const TimersPtr& timers) {
        terminate++;
    });

    p->then([&resolve](const ActionPtr& ptr){
        resolve++;
    });

    ASSERT_EQ(0, terminate);
    ASSERT_EQ(0, resolve);

    p->resolve();
    p->resolve();
    loop->advanceToEnd();

    ASSERT_EQ(0, terminate);
    ASSERT_EQ(1, resolve);
}

TEST_F(ActionTest, DoubleTerminate)
{
    int resolve = 0;
    int terminate = 0;

    auto p = Action::make(loop, [](ActionRef action) {});

    p->addTerminateCallback([&terminate](const TimersPtr& timers) {
        terminate++;
    });

    p->then([&resolve](const ActionPtr& ptr){
        resolve++;
    });

    ASSERT_EQ(0, terminate);
    ASSERT_EQ(0, resolve);

    p->terminate();
    p->terminate();
    loop->advanceToEnd();

    ASSERT_EQ(1, terminate);
    ASSERT_EQ(0, resolve);
}

TEST_F(ActionTest, ResolveAndTerminate)
{
    int resolve = 0;
    int terminate = 0;

    auto p = Action::make(loop, [](ActionRef action) {});

    p->addTerminateCallback([&terminate](const TimersPtr& timers) {
        terminate++;
    });

    p->then([&resolve](const ActionPtr& ptr){
        resolve++;
    });

    ASSERT_EQ(0, terminate);
    ASSERT_EQ(0, resolve);

    p->resolve();
    p->terminate();
    loop->advanceToEnd();

    ASSERT_EQ(0, terminate);
    ASSERT_EQ(1, resolve);
}

TEST_F(ActionTest, TerminateAndResolve)
{
    int resolve = 0;
    int terminate = 0;

    auto p = Action::make(loop, [](ActionRef action) {});

    p->addTerminateCallback([&terminate](const TimersPtr& timers) {
        terminate++;
    });

    p->then([&resolve](const ActionPtr& ptr){
        resolve++;
    });

    ASSERT_EQ(0, terminate);
    ASSERT_EQ(0, resolve);

    p->terminate();
    p->resolve();
    loop->advanceToEnd();

    ASSERT_EQ(1, terminate);
    ASSERT_EQ(0, resolve);
}
