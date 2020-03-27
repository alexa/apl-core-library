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

#include <apl/utils/log.h>

#include "apl/component/textmeasurement.h"
#include "apl/content/metrics.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/focusmanager.h"
#include "apl/engine/keyboardmanager.h"
#include "apl/engine/rootcontextdata.h"
#include "apl/engine/styles.h"
#include "apl/livedata/livedatamanager.h"

namespace apl {

static const bool DEBUG_YG_PRINT_TREE = false;

static LogLevel
ygLevelToDebugLevel(YGLogLevel level)
{
    switch (level) {
        case YGLogLevelError: return LogLevel::ERROR;
        case YGLogLevelWarn: return LogLevel::WARN;
        case YGLogLevelInfo: return LogLevel::INFO;
        case YGLogLevelDebug: return LogLevel::DEBUG;
        case YGLogLevelVerbose: return LogLevel::TRACE;
        case YGLogLevelFatal: return LogLevel::CRITICAL;
    }
}

static int
ygLogger(const YGConfigRef config,
         const YGNodeRef node,
         YGLogLevel level,
         const char *format,
         va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);
    std::vector<char> buf(1 + std::vsnprintf(nullptr, 0, format, args_copy));
    va_end(args_copy);

    std::vsnprintf(buf.data(), buf.size(), format, args);
    va_end(args);

    LOG(ygLevelToDebugLevel(level)) << buf.data();
    return 1;   // Does this matter?
}

/**
 * Construct the root context from metrics.
 *
 * Internally we create a sequencer, a Yoga/Flexbox configuration,
 * and a copy of the currently installed TextMeasurement utility.
 *
 * @param metrics The display metrics.
 */
RootContextData::RootContextData(const Metrics& metrics,
                                 const RootConfig& config,
                                 const std::string& theme,
                                 const std::string& requestedAPLVersion,
                                 const SessionPtr& session,
                                 const std::vector<std::pair<std::string, std::string>>& extensions)
    : pixelWidth(metrics.getPixelWidth()),
      pixelHeight(metrics.getPixelHeight()),
      width(metrics.getWidth()),
      height(metrics.getHeight()),
      pxToDp(160.0 / metrics.getDpi()),
      theme(theme),
      requestedAPLVersion(requestedAPLVersion),
      mStyles(new Styles()),
      mSequencer(new Sequencer(config.getTimeManager())),
      mFocusManager(new FocusManager()),
      mHoverManager(new HoverManager(*this)),
      mKeyboardManager(new KeyboardManager()),
      mDataManager(new LiveDataManager()),
      mExtensionManager(new ExtensionManager(extensions, config)),
      mYGConfigRef(YGConfigNew()),
      mTextMeasurement(config.getMeasure()),
      mConfig(config),
      mScreenLockCount(0),
      mSettings(config),
      mSession(session)
{
    YGConfigSetPrintTreeFlag(mYGConfigRef, DEBUG_YG_PRINT_TREE);
    YGConfigSetLogger(mYGConfigRef, ygLogger);
    YGConfigSetPointScaleFactor(mYGConfigRef, metrics.getDpi() / 160.0);
}


} // namespace apl
