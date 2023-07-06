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

#include "apl/document/documentcontextdata.h"

#include "apl/engine/keyboardmanager.h"
#include "apl/engine/layoutmanager.h"
#include "apl/engine/sharedcontextdata.h"
#include "apl/engine/uidmanager.h"
#include "apl/extension/extensionmanager.h"
#include "apl/livedata/livedatamanager.h"
#include "apl/time/sequencer.h"
#include "apl/time/timemanager.h"
#include "apl/touch/pointermanager.h"
#include "apl/utils/make_unique.h"

#ifdef SCENEGRAPH
#include "apl/scenegraph/textpropertiescache.h"
#endif // SCENEGRAPH

namespace apl {

/**
 * Construct the root context from metrics.
 *
 * Internally we create a sequencer, a Yoga/Flexbox configuration,
 * and a copy of the currently installed TextMeasurement utility.
 *
 * @param metrics The display metrics.
 */
DocumentContextData::DocumentContextData(
                                 const DocumentContextPtr& document,
                                 const Metrics& metrics,
                                 const RootConfig& config,
                                 RuntimeState runtimeState,
                                 const SettingsPtr& settings,
                                 const SessionPtr& session,
                                 const std::vector<ExtensionRequest>& extensions,
                                 const SharedContextDataPtr& sharedContext)
    : ContextData(config, std::move(runtimeState), settings, "", kLayoutDirectionInherit),
      mSharedData(sharedContext),
      mDocument(document),
      mMetrics(metrics),
      mStyles(new Styles()),
      mSequencer(new Sequencer(config.getTimeManager(), mRuntimeState.getRequestedAPLVersion())),
      mDataManager(new LiveDataManager()),
      mExtensionManager(new ExtensionManager(extensions, config, session)),
      mUniqueIdManager(new UIDManager(sharedContext->uidGenerator(), session)),
      mSession(session)
{}

DocumentContextData::~DocumentContextData() {
    mSharedData.reset();
    mUniqueIdManager->terminate();
}

void
DocumentContextData::terminate()
{
    auto top = halt();
    if (top)
        top->release();
}

CoreComponentPtr
DocumentContextData::halt()
{
    if (mSequencer) {
        mSequencer->terminate();
    }

    // Clear any pending events and dirty components
    mSharedData->dirtyComponents().eraseScope(shared_from_this());
    mDirtyVisualContext.clear();

    auto result = mTop;
    mTop = nullptr;
    if (result) result->clearActiveState();

#ifdef ALEXAEXTENSIONS
    // Clear related extension events
    mExtensionEvents = std::queue<Event>();
#endif // ALEXAEXTENSIONS

    // Clear out events related to this context
    mSharedData->eventManager().eraseScope(shared_from_this());

    return result;
}

void
DocumentContextData::pushEvent(Event&& event)
{
    event.setDocument(mDocument);
    mSharedData->eventManager().emplace(shared_from_this(), std::move(event));
}

bool
DocumentContextData::embedded() const
{
    return top() && top()->getParent() != nullptr;
}

FocusManager& DocumentContextData::focusManager() const { return mSharedData->focusManager(); }
HoverManager& DocumentContextData::hoverManager() const { return mSharedData->hoverManager(); }
LayoutManager& DocumentContextData::layoutManager() const { return mSharedData->layoutManager(); }
MediaManager& DocumentContextData::mediaManager() const { return mSharedData->mediaManager(); }
MediaPlayerFactory& DocumentContextData::mediaPlayerFactory() const { return mSharedData->mediaPlayerFactory(); }
DependantManager& DocumentContextData::dependantManager() const { return mSharedData->dependantManager(); }
const YGConfigRef& DocumentContextData::ygconfig() const { return mSharedData->ygconfig(); }
const TextMeasurementPtr& DocumentContextData::measure() const { return mSharedData->measure(); }
void DocumentContextData::takeScreenLock() { mSharedData->takeScreenLock(); }
void DocumentContextData::releaseScreenLock() { mSharedData->releaseScreenLock(); }
LruCache<TextMeasureRequest, YGSize>& DocumentContextData::cachedMeasures() { return mSharedData->cachedMeasures(); }
LruCache<TextMeasureRequest, float>& DocumentContextData::cachedBaselines() { return mSharedData->cachedBaselines(); }
void DocumentContextData::setDirty(const ComponentPtr& component) { mSharedData->dirtyComponents().emplace(shared_from_this(), component); }
void DocumentContextData::clearDirty(const ComponentPtr& component) { mSharedData->dirtyComponents().eraseValue(component); }

#ifdef SCENEGRAPH
sg::TextPropertiesCache& DocumentContextData::textPropertiesCache() { return mSharedData->textPropertiesCache(); }
#endif // SCENEGRAPH

} // namespace apl
