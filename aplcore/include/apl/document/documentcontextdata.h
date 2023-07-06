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

#ifndef _APL_DOCUMENT_CONTEXT_DATA_H
#define _APL_DOCUMENT_CONTEXT_DATA_H

#include "apl/document/contextdata.h"

#include <queue>
#include <yoga/Yoga.h>

#include "apl/common.h"
#include "apl/content/extensionrequest.h"
#include "apl/engine/eventmanager.h"
#include "apl/engine/jsonresource.h"
#include "apl/primitives/textmeasurerequest.h"
#include "apl/utils/lrucache.h"

namespace apl {

class ExtensionManager;
class FocusManager;
class HoverManager;
class LayoutManager;
class LiveDataManager;
class Sequencer;
class Styles;
class UIDManager;
class DataSourceConnection;
using DataSourceConnectionPtr = std::shared_ptr<DataSourceConnection>;

/**
 * Data contained in the rendered document.
 */
class DocumentContextData : public ContextData, public std::enable_shared_from_this<DocumentContextData> {
    friend class CoreDocumentContext;

public:
    /**
     * Stock constructor
     * @param document Document this data belongs to.
     * @param metrics Display metrics
     * @param config Configuration settings
     * @param runtimeState Runtime state information (theme, required APL version, re-inflation state)
     * @param settings Document settings
     * @param session Session information for logging messages and warnings
     * @param extensions List of extension requests
     * @param sharedContext Shared data.
     */
    DocumentContextData(const DocumentContextPtr& document,
                        const Metrics& metrics,
                        const RootConfig& config,
                        RuntimeState runtimeState,
                        const SettingsPtr& settings,
                        const SessionPtr& session,
                        const std::vector<ExtensionRequest>& extensions,
                        const SharedContextDataPtr& sharedContext);

    ~DocumentContextData() override;

    bool fullContext() const override { return true; }

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
    LiveDataManager& dataManager() const { return *mDataManager; }
    ExtensionManager& extensionManager() const { return *mExtensionManager; }
    CoreComponentPtr top() const { return mTop; }
    const std::map<std::string, JsonResource>& layouts() const { return mLayouts; }
    const std::map<std::string, JsonResource>& commands() const { return mCommands; }
    const std::map<std::string, JsonResource>& graphics() const { return mGraphics; }

    Sequencer& sequencer() const { return *mSequencer; }
    FocusManager& focusManager() const;
    HoverManager& hoverManager() const;
    LayoutManager& layoutManager() const;
    MediaManager& mediaManager() const;
    MediaPlayerFactory& mediaPlayerFactory() const;
    UIDManager& uniqueIdManager() const { return *mUniqueIdManager; }
    DependantManager& dependantManager() const;

    const YGConfigRef& ygconfig() const;
    const SessionPtr& session() const override { return mSession; }

    /**
     * @return The installed text measurement for this context.
     */
    const TextMeasurementPtr& measure() const;

    const Metrics& metrics() const { return mMetrics; }

    /**
     * Acquire the screen lock
     */
    void takeScreenLock();

    /**
     * Release the screen lock
     */
    void releaseScreenLock();

    /**
     * @return internal text measurement cache.
     */
    LruCache<TextMeasureRequest, YGSize>& cachedMeasures();

    /**
     * @return internal text measurement baseline cache.
     */
    LruCache<TextMeasureRequest, float>& cachedBaselines();

    /**
     * @return List of pending onMount handlers for recently inflated components.
     */
    WeakPtrSet<CoreComponent>& pendingOnMounts() { return mPendingOnMounts; }

    /**
     * @return Parent DocumentContext.
     */
    DocumentContextPtr documentContext() const { return mDocument.lock(); }

#ifdef SCENEGRAPH
    /**
     * @return A cache of TextProperties
     */
    sg::TextPropertiesCache& textPropertiesCache();
#endif // SCENEGRAPH

    double getWidth() const { return mMetrics.getWidth(); }
    double getHeight() const { return mMetrics.getHeight(); }
    double getPxToDp() const { return Metrics::CORE_DPI / mMetrics.getDpi(); }

    ScreenShape getScreenShape() { return mMetrics.getScreenShape(); }
    int getDpi() const { return mMetrics.getDpi(); }
    ViewportMode getViewportMode() const { return mMetrics.getViewportMode(); }

    SharedContextDataPtr getShared() const { return mSharedData; }

    bool embedded() const override;
    void pushEvent(Event&& event);
    void setDirty(const ComponentPtr& component);
    void clearDirty(const ComponentPtr& component);

    std::set<ComponentPtr>& getDirtyVisualContext() { return mDirtyVisualContext; }
    std::set<DataSourceConnectionPtr>& getDirtyDatasourceContext() { return mDirtyDatasourceContext; }

#ifdef ALEXAEXTENSIONS
    std::queue<Event>& getExtensionEvents() { return mExtensionEvents; }
#endif

private:
    SharedContextDataPtr mSharedData;
    std::weak_ptr<DocumentContext> mDocument;
    Metrics mMetrics;
    std::map<std::string, JsonResource> mLayouts;
    std::map<std::string, JsonResource> mCommands;
    std::map<std::string, JsonResource> mGraphics;
    std::shared_ptr<Styles> mStyles;
    std::unique_ptr<Sequencer> mSequencer;
    std::unique_ptr<LiveDataManager> mDataManager;
    std::unique_ptr<ExtensionManager> mExtensionManager;
    std::unique_ptr<UIDManager> mUniqueIdManager;
    CoreComponentPtr mTop;
    SessionPtr mSession;
    WeakPtrSet<CoreComponent> mPendingOnMounts;
    std::set<ComponentPtr> mDirtyVisualContext;
    std::set<DataSourceConnectionPtr> mDirtyDatasourceContext;
#ifdef ALEXAEXTENSIONS
    std::queue<Event> mExtensionEvents;
#endif
};


} // namespace apl

#endif //_APL_DOCUMENT_CONTEXT_DATA_H
