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

#include "apl/time/sequencer.h"
#include "apl/engine/event.h"
#include "apl/command/arraycommand.h"

#include "../testeventloop.h"

using namespace apl;

class TestCommand : public Command {
public:
    static CommandPtr create(const ContextPtr& context,
                             Properties&& props,
                             const CoreComponentPtr& base,
                             const std::string& parentSequencer = "") {
        auto value = props.asNumber(*context, "argument", -1);
        return std::make_shared<TestCommand>(value);
    }

    TestCommand(int value) : mValue(value) {}

    unsigned long delay() const override { return 1000; }
    std::string name() const override { return "Test"; }
    ActionPtr execute(const TimersPtr& timers, bool fastMode) override {
        sSum += mValue;
        return Action::make(timers);
    }
    std::string sequencer() const override { return ""; }

    int mValue;
    static int sSum;
};

int TestCommand::sSum;

class ArrayCommandTest : public ActionWrapper {
public:
    ArrayCommandTest()
        : ActionWrapper()
    {
        CommandFactory::instance().set("Test", TestCommand::create);
        TestCommand::sSum = 0;

        context = Context::createTestContext(Metrics(), RootConfig().timeManager(loop));
    }

    void TearDown() override {
        context->sequencer().terminate();
        context = nullptr;
        ActionWrapper::TearDown();
    }
public:
    ContextPtr context;
};


TEST_F(ArrayCommandTest, SingleCommand)
{
    ASSERT_EQ(0, loop->size());
    ASSERT_EQ(0, TestCommand::sSum);

    Properties p;
    p.emplace("argument", 10);
    auto action = context->sequencer().execute(TestCommand::create(context, std::move(p), nullptr), false);
    ASSERT_TRUE(action);

    loop->advanceToEnd();

    ASSERT_EQ(0, loop->size());
    ASSERT_EQ(10, TestCommand::sSum);
    ASSERT_TRUE(action->isResolved());
}


TEST_F(ArrayCommandTest, SingleCommandCleanUp)
{
    ASSERT_EQ(0, loop->size());
    ASSERT_EQ(0, TestCommand::sSum);

    Properties p;
    p.emplace("argument", 10);
    auto action = context->sequencer().execute(TestCommand::create(context, std::move(p), nullptr), true);
    ASSERT_FALSE(action);

    loop->runPending();

    ASSERT_EQ(0, loop->size());
    ASSERT_EQ(10, TestCommand::sSum);
}


static const char *COMMAND_LIST =
    "["
    "  {"
    "    \"type\": \"Test\","
    "    \"argument\": 1"
    "  }"
    "]";

TEST_F(ArrayCommandTest, MultipleCommand)
{
    auto json = JsonData(COMMAND_LIST);
    auto command = ArrayCommand::create(context, json.get(), nullptr, Properties());

    auto action = context->sequencer().execute(command, false);
    ASSERT_TRUE(action);

    loop->advanceToEnd();

    ASSERT_EQ(0, loop->size());
    ASSERT_EQ(1, TestCommand::sSum);
    ASSERT_TRUE(action->isResolved());
}

TEST_F(ArrayCommandTest, MultipleCommandCleanUp)
{
    auto json = JsonData(COMMAND_LIST);
    auto command = ArrayCommand::create(context, json.get(), nullptr, Properties());

    auto action = context->sequencer().execute(command, true);
    ASSERT_FALSE(action);

    loop->runPending();

    ASSERT_EQ(0, loop->size());
    ASSERT_EQ(1, TestCommand::sSum);
}


static const char *BASIC =
    "["
    "  {"
    "    \"type\": \"Test\","
    "    \"argument\": 1"
    "  },"
    "  {"
    "    \"type\": \"Test\","
    "    \"argument\": 2"
    "  },"
    "  {"
    "    \"type\": \"Test\","
    "    \"argument\": 4"
    "  }"
    "]";

TEST_F(ArrayCommandTest, Basic)
{
    auto json = JsonData(BASIC);
    auto command = ArrayCommand::create(context, json.get(), nullptr, Properties());
    auto action = command->execute(loop, false);

    ASSERT_EQ(1, loop->size());
    ASSERT_TRUE(action->isPending());

    // Run to the end of time.  All commands should have executed.
    loop->updateTime(3000);
    ASSERT_FALSE(action->isPending());
    ASSERT_EQ(0, loop->size());
    ASSERT_EQ(7, TestCommand::sSum);
}

TEST_F(ArrayCommandTest, AbortEarly)
{
    auto json = JsonData(BASIC);
    auto command = ArrayCommand::create(context, json.get(), nullptr, Properties());
    auto action = command->execute(loop, false);

    ASSERT_EQ(1, loop->size());
    ASSERT_TRUE(action->isPending());

    // Run to the first execution
    loop->updateTime(1000);
    ASSERT_EQ(1, TestCommand::sSum);

    // Now abort and all the commands should stop
    action->terminate();
    ASSERT_FALSE(action->isPending());
    ASSERT_EQ(0, loop->size());
    ASSERT_EQ(1, TestCommand::sSum);
}

TEST_F(ArrayCommandTest, AbortEarlyWithTerminateFinish)
{
    auto json = JsonData(BASIC);
    auto command = ArrayCommand::create(context, json.get(), nullptr, Properties(), "", true);
    auto action = command->execute(loop, false);

    ASSERT_EQ(1, loop->size());
    ASSERT_TRUE(action->isPending());

    // Run to the first execution
    loop->updateTime(1000);
    ASSERT_EQ(1, TestCommand::sSum);

    // Now abort and all the commands should stop
    action->terminate();
    loop->runPending();

    ASSERT_FALSE(action->isPending());
    ASSERT_EQ(0, loop->size());
    ASSERT_EQ(5, TestCommand::sSum);
}

// Run the BASIC commands using the sequencer
TEST_F(ArrayCommandTest, SequencerNormalMode)
{
    auto json = JsonData(BASIC);
    auto command = ArrayCommand::create(context, json.get(), nullptr, Properties());
    auto action = context->sequencer().execute(command, false);

    ASSERT_TRUE(action);
    ASSERT_TRUE(action->isPending());
    ASSERT_EQ(1, loop->size());
    ASSERT_EQ(0, TestCommand::sSum);   // Nothing has run yet

    // Run to the end of time.  All commands should have executed.
    loop->advanceToEnd();
    ASSERT_FALSE(action->isPending());
    ASSERT_EQ(0, loop->size());
    ASSERT_EQ(7, TestCommand::sSum);
}

TEST_F(ArrayCommandTest, SequencerFastMode)
{
    auto json = JsonData(BASIC);
    auto command = ArrayCommand::create(context, json.get(), nullptr, Properties());
    auto action = context->sequencer().execute(command, true);

    ASSERT_FALSE(action);
    ASSERT_EQ(1, loop->size());
    ASSERT_EQ(1, TestCommand::sSum);   // The first one has run already

    // Clear anything that was due to run.  These are all fast mode, so time won't advance
    loop->runPending();
    ASSERT_EQ(0, loop->size());
    ASSERT_EQ(7, TestCommand::sSum);
}
