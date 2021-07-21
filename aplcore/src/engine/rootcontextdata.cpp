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

#include "apl/component/corecomponent.h"
#include "apl/component/textmeasurement.h"
#include "apl/content/metrics.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/rootcontextdata.h"
#include "apl/engine/keyboardmanager.h"
#include "apl/engine/styles.h"
#include "apl/focus/focusmanager.h"
#include "apl/livedata/livedatamanager.h"
#include "apl/time/timemanager.h"
#include "apl/utils/log.h"

namespace apl {

static const bool DEBUG_YG_PRINT_TREE = false;

static LogLevel
ygLevelToDebugLevel(YGLogLevel level)
{
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
                                 RuntimeState runtimeState,
                                 const SettingsPtr& settings,
                                 const SessionPtr& session,
                                 const std::vector<std::pair<std::string, std::string>>& extensions)
    : mRuntimeState(std::move(runtimeState)),
      mMetrics(metrics),
      mStyles(new Styles()),
      mSequencer(new Sequencer(config.getTimeManager(), mRuntimeState.getRequestedAPLVersion())),
      mFocusManager(new FocusManager(*this)),
      mHoverManager(new HoverManager(*this)),
      mPointerManager(new PointerManager(*this)),
      mKeyboardManager(new KeyboardManager()),
      mDataManager(new LiveDataManager()),
      mExtensionManager(new ExtensionManager(extensions, config)),
      mLayoutManager(new LayoutManager(*this)),
      mYGConfigRef(YGConfigNew()),
      mTextMeasurement(config.getMeasure()),
      mConfig(config),
      mScreenLockCount(0),
      mSettings(settings),
      mSession(session),
      mLayoutDirection(kLayoutDirectionInherit)
{
    YGConfigSetPrintTreeFlag(mYGConfigRef, DEBUG_YG_PRINT_TREE);
    YGConfigSetLogger(mYGConfigRef, ygLogger);
    YGConfigSetPointScaleFactor(mYGConfigRef, metrics.getDpi() / 160.0);
}

void
RootContextData::terminate()
{
    auto top = halt();
    if (top)
        top->release();
}

CoreComponentPtr
RootContextData::halt()
{
    mLayoutManager->terminate();
    mConfig.getTimeManager()->clear();

    if (mSequencer) {
        mSequencer->terminate();
        mSequencer = nullptr;
    }

    // Clear any pending events and dirty components
    events = std::queue<Event>();
    dirty.clear();
    dirtyVisualContext.clear();

    auto result = mTop;
    mTop = nullptr;
    return result;
}

} // namespace apl
