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

#include "apl/content/rootconfig.h"
#include "apl/engine/context.h"
#include "apl/utils/session.h"

namespace apl {

SessionMessage::SessionMessage(const SessionPtr& session, const char *filename, const char *function)
    : mSession(session),
      mFilename(filename),
      mFunction(function),
      mUncaught(std::uncaught_exception()) {}

SessionMessage::SessionMessage(const ContextPtr& contextPtr, const char *filename, const char *function)
    : mSession(contextPtr->session()),
      mFilename(filename),
      mFunction(function),
      mUncaught(std::uncaught_exception()) {}

SessionMessage::SessionMessage(const Context& context, const char *filename, const char *function)
    : mSession(context.session()),
      mFilename(filename),
      mFunction(function),
      mUncaught(std::uncaught_exception()) {}

SessionMessage::SessionMessage(const std::weak_ptr<Context>& contextPtr, const char *filename, const char *function)
    : mFilename(filename),
      mFunction(function),
      mUncaught(std::uncaught_exception())
{
    auto context = contextPtr.lock();
    if (context)
        mSession = context->session();
}

SessionMessage::SessionMessage(const RootConfigPtr& config, const char *filename, const char *function)
    : mSession(config->getSession()),
      mFilename(filename),
      mFunction(function),
      mUncaught(std::uncaught_exception()) {}

SessionMessage::~SessionMessage()
{
    if (mSession)
        mSession->write(mFilename.c_str(), mFunction.c_str(), mStringStream.str());
    else
        LoggerFactory::instance().getLogger(LogLevel::kWarn, mFilename.c_str(), mFunction.c_str())
                                 .log("%s", mStringStream.str().c_str());
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
        LoggerFactory::instance().getLogger(LogLevel::kWarn, filename, func).log("%s", value);
    }
};


SessionPtr makeDefaultSession()
{
    return std::make_shared<DefaultSession>();
}

} // namespace apl