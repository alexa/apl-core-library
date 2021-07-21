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

#include "gtest/gtest.h"

#include "apl/utils/log.h"
#include "apl/utils/session.h"

using namespace apl;

class TestSession : public Session {
public:
    void write(const char *filename, const char* func, const char *value) override {
        mLog += value;
        mCalls += 1;
    }

    void reset() {
        mLog = "";
        mCalls = 0;
    }

    std::string mLog;
    int mCalls = 0;
};

class ConsoleTest : public ::testing::Test {
public:
    ConsoleTest()
        : session(std::make_shared<TestSession>())
    {
    }

    const std::string& console() const {
        return session->mLog;
    }

    int calls() const {
        return session->mCalls;
    }

    void SetUp() override
    {
        Test::SetUp();
        session->reset();
    }

    void TearDown() override {
        session.reset();
        Test::TearDown();
    }

    std::shared_ptr<TestSession> session;
};

TEST_F(ConsoleTest, Stream)
{
    CONSOLE_S(session) << "Test1";
    ASSERT_STREQ("Test1", console().c_str());
}

TEST_F(ConsoleTest, Formatted)
{
    CONSOLE_S(session).log("%s: %d", "Test1", 26);
    ASSERT_STREQ("Test1: 26", console().c_str());
}


class TestLoggingBridge : public LogBridge {
public:
    void transport(LogLevel level, const std::string& log) override {
        mLevel = level;
        mLog = log;
        mCount++;
    }

    LogLevel mLevel = LogLevel::kNone;
    std::string mLog = "";
    int mCount = 0;
};

TEST(DefaultConsole, VerifyLog)
{
    auto bridge = std::make_shared<TestLoggingBridge>();
    LoggerFactory::instance().initialize(bridge);

    auto session = makeDefaultSession();

    CONSOLE_S(session) << "TestVerifyLog";
    ASSERT_EQ(1, bridge->mCount);
    ASSERT_EQ(LogLevel::kWarn, bridge->mLevel);
    ASSERT_STREQ("unittest_session.cpp:TestBody : TestVerifyLog", bridge->mLog.c_str());
}

/**
 * Test to verify that user strings that may get log are not expanded and may access to
 * non valid positions.
 */
TEST(DefaultConsole, UserDataInjection)
{
    auto bridge = std::make_shared<TestLoggingBridge>();
    LoggerFactory::instance().initialize(bridge);

    auto session = makeDefaultSession();
    // if this entry expands, it will crash
    CONSOLE_S(session) << "cce   %s";
    ASSERT_EQ(1, bridge->mCount);
    ASSERT_EQ(LogLevel::kWarn, bridge->mLevel);
    ASSERT_STREQ("unittest_session.cpp:TestBody : cce   %s", bridge->mLog.c_str());
}