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

using namespace apl;

class LogTest : public ::testing::Test {
protected:
    class TestLogBridge : public LogBridge {
    public:
        void transport(LogLevel level, const std::string& log) override
        {
            mLevel = level;
            mLog = log;
            mCalls++;
        }

        void reset()
        {
            mLevel = LogLevel::kNone;
            mLog = "";
            mCalls = 0;
        }

        LogLevel mLevel;
        std::string mLog;
        int mCalls;
    };

    void reset() { mLogBridge->reset(); }
    LogLevel level() const { return mLogBridge->mLevel; }
    std::string log() const { return mLogBridge->mLog; }
    int calls() const { return mLogBridge->mCalls; }

    void SetUp() override
    {
        mLogBridge = std::make_shared<TestLogBridge>();
        LoggerFactory::instance().initialize(mLogBridge);
    }

    void TearDown() override
    {
        LoggerFactory::instance().reset();
        Test::TearDown();
    }

    std::shared_ptr<TestLogBridge> mLogBridge;
};

TEST_F(LogTest, Stream)
{
    LOG(LogLevel::kInfo) << "Log";
    ASSERT_EQ(LogLevel::kInfo, level());
    ASSERT_STREQ("unittest_log.cpp:TestBody : Log", log().c_str());
}

TEST_F(LogTest, Formatted)
{
    LOGF(LogLevel::kInfo, "Log %d", 42);
    ASSERT_EQ(LogLevel::kInfo, level());
    ASSERT_STREQ("unittest_log.cpp:TestBody : Log 42", log().c_str());
}

TEST_F(LogTest, Conditional)
{
    LOG_IF(true) << "LOG_TRUE";
    ASSERT_STREQ("unittest_log.cpp:TestBody : LOG_TRUE", log().c_str());
    ASSERT_EQ(1, calls());

    reset();
    LOG_IF(false) << "LOG_FALSE";
    ASSERT_STREQ("", log().c_str());
    ASSERT_EQ(0, calls());

    reset();
    LOGF_IF(true, "LOGF_%d", true);
    ASSERT_STREQ("unittest_log.cpp:TestBody : LOGF_1", log().c_str());
    ASSERT_EQ(1, calls());

    reset();
    LOGF_IF(false, "LOGF_%d", false);
    ASSERT_STREQ("", log().c_str());
    ASSERT_EQ(0, calls());
}
