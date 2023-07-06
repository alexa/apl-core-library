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

#include "apl/engine/sharedcontextdata.h"

#include "apl/embed/documentregistrar.h"
#include "apl/engine/dependantmanager.h"
#include "apl/engine/eventmanager.h"
#include "apl/engine/hovermanager.h"
#include "apl/engine/keyboardmanager.h"
#include "apl/engine/layoutmanager.h"
#include "apl/engine/tickscheduler.h"
#include "apl/engine/uidgenerator.h"
#include "apl/focus/focusmanager.h"
#include "apl/media/mediamanager.h"
#include "apl/touch/pointermanager.h"
#include "apl/utils/make_unique.h"

#ifdef SCENEGRAPH
#include "apl/scenegraph/textpropertiescache.h"
#endif // SCENEGRAPH

namespace apl {

static const bool DEBUG_YG_PRINT_TREE = false;

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

SharedContextData::SharedContextData(const CoreRootContextPtr& root, const Metrics& metrics,
                                     const RootConfig& config)
    : mRequestedVersion(config.getReportedAPLVersion()),
      mDocumentRegistrar(std::make_unique<DocumentRegistrar>()),
      mFocusManager(std::make_unique<FocusManager>(*root)),
      mHoverManager(std::make_unique<HoverManager>(*root)),
      mPointerManager(std::make_unique<PointerManager>(*root, *mHoverManager)),
      mKeyboardManager(std::make_unique<KeyboardManager>()),
      mLayoutManager(std::make_unique<LayoutManager>(
         *root,
         Size(static_cast<float>(metrics.getWidth()), static_cast<float>(metrics.getHeight())))),
      mTickScheduler(std::make_unique<TickScheduler>(config.getTimeManager())),
      mDirtyComponents(std::make_unique<DirtyComponents>()),
      mUniqueIdGenerator(std::make_unique<UIDGenerator>()),
      mEventManager(std::make_unique<EventManager>()),
      mDependantManager(std::make_unique<DependantManager>()),
      mDocumentManager(config.getDocumentManager()),
      mTimeManager(config.getTimeManager()),
      mMediaManager(config.getMediaManager()),
      mMediaPlayerFactory(config.getMediaPlayerFactory()),
      mYGConfigRef(YGConfigNew()),
      mTextMeasurement(config.getMeasure()),
      mCachedMeasures(config.getProperty(RootProperty::kTextMeasurementCacheLimit).getInteger()),
      mCachedBaselines(config.getProperty(RootProperty::kTextMeasurementCacheLimit).getInteger())
#ifdef SCENEGRAPH
      ,
      mTextPropertiesCache(new sg::TextPropertiesCache())
#endif // SCENEGRAPH
{
    YGConfigSetPrintTreeFlag(mYGConfigRef, DEBUG_YG_PRINT_TREE);
    YGConfigSetLogger(mYGConfigRef, ygLogger);
    YGConfigSetPointScaleFactor(mYGConfigRef, metrics.getDpi() / Metrics::CORE_DPI);
}

SharedContextData::SharedContextData(const RootConfig& config)
    : mRequestedVersion(config.getReportedAPLVersion()),
      mUniqueIdGenerator(std::make_unique<UIDGenerator>()),
      mDependantManager(std::make_unique<DependantManager>()),
      mYGConfigRef(YGConfigNew()),
      mTextMeasurement(config.getMeasure()),
      mCachedMeasures(0),
      mCachedBaselines(0)
#ifdef SCENEGRAPH
      ,
      mTextPropertiesCache(new sg::TextPropertiesCache())
#endif // SCENEGRAPH
{}

SharedContextData::~SharedContextData() {
    YGConfigFree(mYGConfigRef);
}

void
SharedContextData::halt()
{
    mLayoutManager->terminate();
    mTimeManager->clear();
    mEventManager->clear();
}
} // namespace apl
