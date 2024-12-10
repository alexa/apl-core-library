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

#include "apl/yoga/yogaconfig.h"

#include <yoga/Yoga.h>

#include "apl/component/corecomponent.h"

namespace apl {

#define TO_YOGA_CONF(CONF) ((YGConfigRef)(CONF))

static LogLevel
ygLevelToDebugLevel(YGLogLevel level) {
    switch (level) {
        case YGLogLevelError: return LogLevel::kError;
        case YGLogLevelWarn: return LogLevel::kWarn;
        case YGLogLevelInfo: return LogLevel::kInfo;
        case YGLogLevelDebug: return LogLevel::kDebug;
        case YGLogLevelVerbose: return LogLevel::kTrace;
        case YGLogLevelFatal: return LogLevel::kCritical;
    }
    return LogLevel::kDebug;
}

static int
ygLogger(const YGConfigRef config,
         const YGNodeRef node,
         YGLogLevel level,
         const char* format,
         va_list args) {
    va_list args_copy;
    va_copy(args_copy, args);
    std::vector<char> buf(1 + std::vsnprintf(nullptr, 0, format, args_copy));
    va_end(args_copy);

    std::vsnprintf(buf.data(), buf.size(), format, args);
    va_end(args);

    LOG(ygLevelToDebugLevel(level)) << buf.data();
    return 1; // Does this matter?
}

YogaConfig::YogaConfig(const Metrics& metrics, bool debug) {
    mConfig = YGConfigNew();
    YGConfigSetPrintTreeFlag(TO_YOGA_CONF(mConfig), debug);
    YGConfigSetLogger(TO_YOGA_CONF(mConfig), ygLogger);
    YGConfigSetPointScaleFactor(TO_YOGA_CONF(mConfig), metrics.getDpi() / Metrics::CORE_DPI);
}

YogaConfig::YogaConfig() {
    mConfig = YGConfigNew();
}

YogaConfig::~YogaConfig() {
    YGConfigFree(TO_YOGA_CONF(mConfig));
}

} // namespace apl
