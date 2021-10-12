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

#ifndef _APL_SESSION_H
#define _APL_SESSION_H

#include "apl/common.h"
#include "apl/utils/log.h"

namespace apl {

/**
 * The Session object provides a virtual console to report errors and problems that occur when
 * parsing the APL document and packages.  These are the errors that should be reported back to
 * the APL content author.
 *
 * Each view host should provide a custom Session object per logical displayed document.
 * If no session object is provided, console errors will be written to the standard log.
 *
 * Remember!  If the error is something the APL author should correct, report it on the CONSOLE
 * macros below and it should be routed to the APL author.  If the error is something inside
 * of the APL Core Library, report it using LOG.
 */
class Session {
public:
    virtual ~Session() = default;

    /**
     * Write a string in the session log, including the filename and function where
     * the log was generated.
     *
     * Override this method in your Session subclass.
     *
     * @param filename The name of the file where the console message was generated
     * @param func The name of the function where the console message was generated
     * @param value The string to write
     */
    virtual void write(const char *filename, const char *func, const char *value) = 0;

    /**
     * Write a string in the session log, including the filename and function where
     * the log was generated.  This is a convenience method for std::string
     *
     * @param filename
     * @param func
     * @param value
     */
    void write(const char *filename, const char *func, const std::string& value) {
        write(filename, func, value.c_str());
    }
};

/**
 * Construct a default session which passes console messages to the log
 * @return
 */
extern SessionPtr makeDefaultSession();

/**
 * Temporary object used to accumulate logging information before writing it to the session.
 */
class SessionMessage {
public:
    SessionMessage(const SessionPtr& session, const char *filename, const char *function);

    SessionMessage(const ContextPtr& contextPtr, const char *filename, const char *function);

    SessionMessage(const Context& context, const char *filename, const char *function);

    SessionMessage(const std::weak_ptr<Context>& contextPtr, const char *filename, const char *function);

    SessionMessage(const RootConfigPtr& config, const char *filename, const char *function);

    ~SessionMessage();

    template<class T> friend SessionMessage& operator<<(SessionMessage&& sm, T&& value)
    {
        sm.mStringStream << std::forward<T>(value);
        return sm;
    }

    template<class T> friend SessionMessage& operator<<(SessionMessage& sm, T&& value)
    {
        sm.mStringStream << std::forward<T>(value);
        return sm;
    }

    template<class T> friend SessionMessage& operator<<(SessionMessage& sm, const std::vector<T>& values)
    {
        auto len = values.size();
        for (auto i = 0 ; i < len ; i++) {
            sm.mStringStream << values.at(i);
            if (i < len - 1)
                sm.mStringStream << "/";
        }
        return sm;
    }

    /**
     * @param format Log message format. Same format as for printf.
     * @param ... Variable arguments.
     */
    SessionMessage& log(const char *format, ...);

private:
    SessionPtr mSession;
    std::string mFilename;
    std::string mFunction;

    const bool mUncaught;
    streamer mStringStream;
};

/// Report content errors using a session object
#define CONSOLE_S(SESSION)   SessionMessage(SESSION,__FILENAME__,__func__)

/// Report content errors using a context object pointer (which contains a session)
#define CONSOLE_CTP(CONTEXT_PTR) SessionMessage(CONTEXT_PTR,__FILENAME__,__func__)

/// Report content errors using a context object (which contains a session)
#define CONSOLE_CTX(CONTEXT) SessionMessage(CONTEXT,__FILENAME__,__func__)

/// Report content errors using a config object pointer (which contains a session)
#define CONSOLE_CFGP(CONFIG_PTR) SessionMessage(CONFIG_PTR,__FILENAME__,__func__)

} // namespace apl

#endif // _APL_SESSION_H
