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

#include "apl/content/content.h"
#include "apl/embed/documentregistrar.h"
#include "apl/engine/dependantmanager.h"
#include "apl/engine/eventmanager.h"
#include "apl/engine/hovermanager.h"
#include "apl/engine/keyboardmanager.h"
#include "apl/engine/layoutmanager.h"
#include "apl/engine/tickscheduler.h"
#include "apl/engine/uidgenerator.h"
#include "apl/engine/visibilitymanager.h"
#include "apl/focus/focusmanager.h"
#include "apl/media/mediamanager.h"
#include "apl/scenegraph/textlayoutcache.h"
#include "apl/scenegraph/textpropertiescache.h"
#include "apl/touch/pointermanager.h"
#include "apl/utils/make_unique.h"

namespace apl {

static const bool DEBUG_YG_PRINT_TREE = false;

SharedContextData::SharedContextData(const CoreRootContextPtr& root,
                                     const Metrics& metrics,
                                     const RootConfig& config)
    : mRequestedVersion(config.getProperty(RootProperty::kReportedVersion).getString()),
      mDocumentRegistrar(std::make_unique<DocumentRegistrar>()),
      mFocusManager(std::make_unique<FocusManager>(*root)),
      mHoverManager(std::make_unique<HoverManager>(*root)),
      mPointerManager(std::make_unique<PointerManager>(*root, *mHoverManager)),
      mKeyboardManager(std::make_unique<KeyboardManager>()),
      mLayoutManager(std::make_unique<LayoutManager>(*root, metrics.getViewportSize())),
      mTickScheduler(std::make_unique<TickScheduler>(config.getTimeManager())),
      mDirtyComponents(std::make_unique<DirtyComponents>()),
      mUniqueIdGenerator(std::make_unique<UIDGenerator>()),
      mEventManager(std::make_unique<EventManager>()),
      mDependantManager(std::make_unique<DependantManager>()),
      mVisibilityManager(std::make_unique<VisibilityManager>()),
      mDocumentManager(config.getDocumentManager()),
      mTimeManager(config.getTimeManager()),
      mMediaManager(config.getMediaManager()),
      mMediaPlayerFactory(config.getMediaPlayerFactory()),
      mYogaConfig(metrics, DEBUG_YG_PRINT_TREE),
      mTextMeasurement(config.getMeasure()),
      mTextLayoutCache(new sg::TextLayoutCache()),
      mTextPropertiesCache(new sg::TextPropertiesCache())
{
}

SharedContextData::SharedContextData(const RootConfig& config)
    : mRequestedVersion(config.getProperty(RootProperty::kReportedVersion).getString()),
      mUniqueIdGenerator(std::make_unique<UIDGenerator>()),
      mDependantManager(std::make_unique<DependantManager>()),
      mYogaConfig(),
      mTextMeasurement(config.getMeasure()),
      mTextLayoutCache(new sg::TextLayoutCache()),
      mTextPropertiesCache(new sg::TextPropertiesCache())
{}

SharedContextData::~SharedContextData() {}

void
SharedContextData::halt()
{
    mFocusManager->terminate();
    mLayoutManager->terminate();
    mTimeManager->clear();
    mEventManager->clear();
}
} // namespace apl
