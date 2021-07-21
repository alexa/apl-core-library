/*
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

#include "apl/utils/log.h"

namespace apl {

Logger::Logger(std::shared_ptr<LogBridge> bridge, LogLevel level, const std::string& file, const std::string& function):
    mUncaught{std::uncaught_exception()},
    mBridge{bridge},
    mLevel{level}
{
    mStringStream << file << ":" << function << " : ";
}

Logger::~Logger()
{
    if(mLevel > LogLevel::kNone) {
        mBridge->transport(mLevel, (mUncaught != std::uncaught_exception()) ? "***" : "" + mStringStream.str());
    }
}

void
Logger::log(const char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);

    va_list args2;
    va_copy(args2, argptr);

    std::vector<char> buf(1 + std::vsnprintf(nullptr, 0, format, argptr));
    va_end(argptr);

    std::vsnprintf(buf.data(), buf.size(), format, args2);
    va_end(args2);

    mStringStream << std::string(buf.begin(), buf.end());
}

void
Logger::log(const char *format, va_list args)
{
    va_list args2;
    va_copy(args2, args);

    std::vector<char> buf(1 + std::vsnprintf(nullptr, 0, format, args));
    std::vsnprintf(buf.data(), buf.size(), format, args2);
    va_end(args2);

    mStringStream << std::string(buf.begin(), buf.end());
}

LoggerFactory&
LoggerFactory::instance() {
    static LoggerFactory instance;
    return instance;
}

void
LoggerFactory::initialize(std::shared_ptr<LogBridge> bridge) {
    mLogBridge = bridge;
    mInitialized = true;
}

void
LoggerFactory::reset() {
    mLogBridge = std::make_shared<DefaultLogBridge>();
    mInitialized = false;
    mWarned = false;
}

Logger
LoggerFactory::getLogger(LogLevel level, const std::string& file, const std::string& function) {
    if(!mInitialized && !mWarned) {
        Logger(mLogBridge, LogLevel::kWarn, __FILE__, __func__) << "Logs not initialized. Using default bridge.";
        mWarned = true;
    }
    return Logger(mLogBridge, level, file, function);
}

LoggerFactory::LoggerFactory() :
    mInitialized(false),
    mWarned(false)
{
    mLogBridge = std::make_shared<DefaultLogBridge>();
}

} // namespace apl