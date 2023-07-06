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

#include "apl/utils/session.h"

#include "apl/content/content.h"
#include "apl/content/rootconfig.h"
#include "apl/document/coredocumentcontext.h"
#include "apl/engine/context.h"
#include "apl/utils/random.h"

namespace apl {

static const size_t LOG_PREFIX_SIZE = 6;
static const size_t LOG_ID_SIZE = 10;

Session::Session() : mLogId(Random::generateSimpleToken(LOG_ID_SIZE)) {}

void
Session::setLogIdPrefix(const std::string& prefix) {
    mLogId = mLogId.substr(mLogId.size() - LOG_ID_SIZE);
    auto resultPrefix = prefix;
    if (!resultPrefix.empty()) {
        resultPrefix.erase(std::remove_if(resultPrefix.begin(), resultPrefix.end(),
                                             [](unsigned char c) {
                                                 return !sutil::isupper(c);
                                             }), resultPrefix.end());
        if (!resultPrefix.empty()) {
            resultPrefix.resize(LOG_PREFIX_SIZE, '_');
            mLogId = resultPrefix + "-" + mLogId;
        }
    }
}

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

SessionMessage::SessionMessage(const ContentPtr& content, const char *filename, const char *function)
    : mSession(content->getSession()),
      mFilename(filename),
      mFunction(function),
      mUncaught(std::uncaught_exception()) {}

SessionMessage::SessionMessage(const CoreDocumentContextPtr& document, const char *filename, const char *function)
    : mSession(document->getSession()),
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
        LoggerFactory::instance().getLogger(LogLevel::kWarn, filename, func).session(*this).log("%s", value);
    }
};


SessionPtr makeDefaultSession()
{
    return std::make_shared<DefaultSession>();
}

} // namespace apl