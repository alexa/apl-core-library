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

#ifndef _APL_ROOT_CONTEXT_DATA_H
#define _APL_ROOT_CONTEXT_DATA_H

#include <map>
#include <string>
#include <queue>

#include "apl/content/content.h"
#include "apl/content/metrics.h"
#include "apl/content/rootconfig.h"
#include "apl/content/settings.h"
#include "apl/datasource/datasourceconnection.h"
#include "apl/engine/event.h"
#include "apl/engine/hovermanager.h"
#include "apl/engine/jsonresource.h"
#include "apl/engine/keyboardmanager.h"
#include "apl/engine/layoutmanager.h"
#include "apl/engine/runtimestate.h"
#include "apl/engine/styles.h"
#include "apl/extension/extensionmanager.h"
#include "apl/focus/focusmanager.h"
#include "apl/livedata/livedatamanager.h"
#include "apl/media/mediamanager.h"
#include "apl/primitives/textmeasurerequest.h"
#include "apl/primitives/size.h"
#include "apl/time/sequencer.h"
#include "apl/touch/pointermanager.h"
#include "apl/utils/counter.h"
#include "apl/utils/lrucache.h"

namespace apl {

class RootContextData : public Counter<RootContextData> {
    friend class RootContext;

public:
    /**
     * Stock constructor
     * @param metrics Display metrics
     * @param config Configuration settings
     * @param runtimeState Runtime state information (theme, required APL version, re-inflation state)
     * @param settings Document settings
     * @param session Session information for logging messages and warnings
     * @param extensions Mapping of requested extensions NAME -> URI
     */
    RootContextData(const Metrics& metrics,
                    const RootConfig& config,
                    RuntimeState runtimeState,
                    const SettingsPtr& settings,
                    const SessionPtr& session,
                    const std::vector<std::pair<std::string, std::string>>& extensions);

    ~RootContextData() {
        YGConfigFree(mYGConfigRef);
    }

    /**
     * Halt the RootContextData and release the component hierarchy..
     */
    void terminate();

    /**
     * This root context data is being replaced by a new one.  Terminate all processing
     * and return the top component.  To release memory, you must call release on the top
     * component after you are done with it.  Once halted the RootContextData cannot be
     * restarted.
     */
    CoreComponentPtr halt();

    std::shared_ptr<Styles> styles() const { return mStyles; }
    Sequencer& sequencer() const { return *mSequencer; }
    FocusManager& focusManager() const { return *mFocusManager; }
    HoverManager& hoverManager() const { return *mHoverManager; }
    PointerManager& pointerManager() const { return *mPointerManager; }
    KeyboardManager& keyboardManager() const { return *mKeyboardManager; }
    LiveDataManager& dataManager() const { return *mDataManager; }
    ExtensionManager& extensionManager() const { return *mExtensionManager; }
    LayoutManager& layoutManager() const { return *mLayoutManager; }
    MediaManager& mediaManager() const { return *mConfig.getMediaManager(); }
    MediaPlayerFactory& mediaPlayerFactory() const { return *mConfig.getMediaPlayerFactory(); }

    const YGConfigRef& ygconfig() const { return mYGConfigRef; }
    CoreComponentPtr top() const { return mTop; }
    const std::map<std::string, JsonResource>& layouts() const { return mLayouts; }
    const std::map<std::string, JsonResource>& commands() const { return mCommands; }
    const std::map<std::string, JsonResource>& graphics() const { return mGraphics; }
    const SessionPtr& session() const { return mSession; }

    RootContextData& lang(std::string lang) { mLang = lang; return *this; }
    RootContextData& layoutDirection(LayoutDirection layoutDirection) {
        mLayoutDirection = layoutDirection; return *this;
    }

    /**
     * @return The installed text measurement for this context.
     */
    const TextMeasurementPtr& measure() const { return mTextMeasurement; }

    const RootConfig& rootConfig() const { return mConfig; }

    /**
     * @return True if the screen lock is currently being held by a command.
     */
    bool screenLock() { return mScreenLockCount > 0; }

    /**
     * Acquire the screen lock
     */
    void takeScreenLock() { mScreenLockCount++; }

    /**
     * Release the screen lock
     */
    void releaseScreenLock() { mScreenLockCount--; }

    /**
     * @return internal text measurement cache.
     */
    LruCache<TextMeasureRequest, YGSize>& cachedMeasures() { return mCachedMeasures; }

    /**
     * @return internal text measurement baseline cache.
     */
    LruCache<TextMeasureRequest, float>& cachedBaselines() { return mCachedBaselines; }

    /**
     * @return List of pending onMount handlers for recently inflated components.
     */
    WeakPtrSet<CoreComponent>& pendingOnMounts() { return mPendingOnMounts; }

public:
    int getPixelWidth() const { return mMetrics.getPixelHeight(); }
    int getPixelHeight() const { return mMetrics.getPixelHeight(); }
    double getWidth() const { return mMetrics.getWidth(); }
    double getHeight() const { return mMetrics.getHeight(); }
    Size getSize() const { return { static_cast<float>(mMetrics.getWidth()), static_cast<float>(mMetrics.getHeight())}; }
    double getPxToDp() const { return 160.0 / mMetrics.getDpi(); }

    std::string getTheme() const { return mRuntimeState.getTheme(); }
    std::string getRequestedAPLVersion() const { return mRuntimeState.getRequestedAPLVersion(); }
    std::string getLang() const { return mLang; }
    LayoutDirection getLayoutDirection() const { return mLayoutDirection; }
    bool getReinflationFlag() const { return mRuntimeState.getReinflation(); }

    std::queue<Event> events;
#ifdef ALEXAEXTENSIONS
    std::queue<Event> extesnionEvents;
#endif
    std::set<ComponentPtr> dirty;
    std::set<ComponentPtr> dirtyVisualContext;
    std::set<DataSourceConnectionPtr> dirtyDatasourceContext;

private:
    RuntimeState mRuntimeState;
    std::map<std::string, JsonResource> mLayouts;
    std::map<std::string, JsonResource> mCommands;
    std::map<std::string, JsonResource> mGraphics;
    Metrics mMetrics;
    std::shared_ptr<Styles> mStyles;
    std::unique_ptr<Sequencer> mSequencer;
    std::unique_ptr<FocusManager> mFocusManager;
    std::unique_ptr<HoverManager> mHoverManager;
    std::unique_ptr<PointerManager> mPointerManager;
    std::unique_ptr<KeyboardManager> mKeyboardManager;
    std::unique_ptr<LiveDataManager> mDataManager;
    std::unique_ptr<ExtensionManager> mExtensionManager;
    std::unique_ptr<LayoutManager> mLayoutManager;
    YGConfigRef mYGConfigRef;
    TextMeasurementPtr mTextMeasurement;
    CoreComponentPtr mTop;         // The top component
    const RootConfig mConfig;
    int mScreenLockCount;
    SettingsPtr mSettings;
    SessionPtr mSession;
    std::string mLang;
    LayoutDirection mLayoutDirection;
    LruCache<TextMeasureRequest, YGSize> mCachedMeasures;
    LruCache<TextMeasureRequest, float> mCachedBaselines;
    WeakPtrSet<CoreComponent> mPendingOnMounts;
};


} // namespace apl

#endif //_APL_ROOT_CONTEXT_DATA_H
