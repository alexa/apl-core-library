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

#include "apl/utils/log.h"
#include "apl/utils/session.h"

namespace apl {

SessionMessage::~SessionMessage()
{
    if (mSession)
        mSession->write(mFilename.c_str(), mFunction.c_str(), mStringStream.str());
    else
        LoggerFactory::instance().getLogger(LogLevel::WARN, mFilename.c_str(), mFunction.c_str())
                                 .log(mStringStream.str().c_str());
}

SessionMessage& SessionMessage::log(const char *format, ...)
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
    return *this;
}

/**
 * The default session writes all console messages to the standard log with log level WARN.
 */
class DefaultSession : public Session {
public:
    void write(const char *filename, const char *func, const char *value) override {
        LoggerFactory::instance().getLogger(LogLevel::WARN, filename, func).log(value);
    }
};


SessionPtr makeDefaultSession()
{
    return std::make_shared<DefaultSession>();
}

} // namespace apl