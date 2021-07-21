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

#ifndef APL_TRACING_H
#define APL_TRACING_H

#include <string>

/**
 * This file defines macros to enable tracing viewhost activity:
 *
 * APL_TRACE_BEGIN(NAME) / APL_TRACE_END(NAME) : used to mark the beginning and end of an activity. These need
 *                                               to always appear in pairs with matching names. Only C-style
 *                                               strings can be passed in as arguments (typically a string
 *                                               literal, e.g. APL_TRACE_BEGIN("myInterestingTask").
 * APL_TRACE_BLOCK(NAME) : used to register a tracepoint for an entire block (e.g. a C++ function). The tracepoint
 *                         will begin from the macro location, and automatically end when the block is exited.
 *                         This macro accepts a @c std::string parameter.
 *
 * When Trace support is disabled, these macros are noops.
 */

#ifdef ENABLE_TRACING

#define APL_TRACE_BEGIN(NAME) Tracing::beginSection(NAME)
#define APL_TRACE_END(NAME) Tracing::endSection(NAME)
#define APL_TRACE_BLOCK(NAME) apl::TraceBlock _apl_trace_block(NAME)

#else

#define APL_TRACE_BEGIN(NAME)
#define APL_TRACE_END(NAME)
#define APL_TRACE_BLOCK(NAME)

#endif

namespace apl {

/**
 * Support class to handle tracing libraries.
 */
class Tracing {
public:
    static void beginSection(const char *sectionName);
    static void endSection(const char *sectionName);

private:
    static void initialize();

private:
    static bool mSupported;
    static bool mInitialized;
};

/**
 * Convenience class to add begin/end tracepoints to any block. It relies on the lifetime of a local object
 * to start and stop the instrumentation (i.e., RAII-style).
 */
class TraceBlock {
public:
    /**
     * Constructor. Begins the specified tracepoint on allocation.
     * @param name The name of the tracepoint controlled by this instance.
     */
    explicit TraceBlock(const std::string& name)
            : mName(name) {
        APL_TRACE_BEGIN(mName.c_str());
    }

    /**
     * Destructor. Ends the stored tracepoint on deallocation.
     */
    ~TraceBlock() {
        APL_TRACE_END(mName.c_str());
    }

private:
    std::string mName;
};

} // namespace apl

#endif //APL_TRACING_H