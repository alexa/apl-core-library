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
    CONSOLE(session) << "Test1";
    ASSERT_STREQ("Test1", console().c_str());
}

TEST_F(ConsoleTest, Formatted)
{
    CONSOLE(session).log("%s: %d", "Test1", 26);
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

    CONSOLE(session) << "TestVerifyLog";
    ASSERT_EQ(1, bridge->mCount);
    ASSERT_EQ(LogLevel::kWarn, bridge->mLevel);
    ASSERT_STREQ((session->getLogId() + ":unittest_session.cpp:TestBody : TestVerifyLog").c_str(), bridge->mLog.c_str());
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
    CONSOLE(session) << "cce   %s";
    ASSERT_EQ(1, bridge->mCount);
    ASSERT_EQ(LogLevel::kWarn, bridge->mLevel);
    ASSERT_STREQ((session->getLogId() + ":unittest_session.cpp:TestBody : cce   %s").c_str(), bridge->mLog.c_str());
}

TEST(DefaultConsole, SameSessionId)
{
    auto session = makeDefaultSession();
    session->setLogIdPrefix("ABCDEF");
    auto idWithPrefix1 = session->getLogId();
    session->setLogIdPrefix("ABCDEF");
    auto idWithPrefix2 = session->getLogId();

    ASSERT_EQ(idWithPrefix1, idWithPrefix2);

    ASSERT_TRUE(idWithPrefix1.rfind("ABCDEF-", 0) == 0);
}

TEST(DefaultConsole, ShortSessionId)
{
    auto session = makeDefaultSession();
    session->setLogIdPrefix("ABC");
    ASSERT_TRUE(session->getLogId().rfind("ABC___-", 0) == 0);
}

TEST(DefaultConsole, LongSessionId)
{
    auto session = makeDefaultSession();
    session->setLogIdPrefix("ABCDEFGH");
    ASSERT_TRUE(session->getLogId().rfind("ABCDEF-", 0) == 0);
}

TEST(DefaultConsole, InvalidCharsSessionId)
{
    auto session = makeDefaultSession();
    session->setLogIdPrefix("A- +1k");
    ASSERT_TRUE(session->getLogId().rfind("A_____-", 0) == 0);
}

TEST(DefaultConsole, InvalidSessionId)
{
    auto session = makeDefaultSession();
    auto currentId = session->getLogId();
    session->setLogIdPrefix("1- +1k");
    ASSERT_EQ(currentId, session->getLogId());
}