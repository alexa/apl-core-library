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

#ifndef _APL_SHARED_CONTEXT_DATA_H
#define _APL_SHARED_CONTEXT_DATA_H

#include <yoga/Yoga.h>

#include "apl/common.h"
#include "apl/content/content.h"
#include "apl/content/metrics.h"
#include "apl/content/settings.h"
#include "apl/engine/event.h"
#include "apl/engine/jsonresource.h"
#include "apl/engine/runtimestate.h"
#include "apl/engine/styles.h"
#include "apl/primitives/size.h"
#include "apl/primitives/textmeasurerequest.h"
#include "apl/utils/counter.h"
#include "apl/utils/lrucache.h"
#include "apl/utils/scopedset.h"

#ifdef SCENEGRAPH
#include "apl/scenegraph/common.h"
#endif // SCENEGRAPH

namespace apl {

class DocumentRegistrar;
class EventManager;
class FocusManager;
class HoverManager;
class KeyboardManager;
class LayoutManager;
class PointerManager;
class TickScheduler;
class UIDGenerator;
class DataSourceConnection;
using DataSourceConnectionPtr = std::shared_ptr<DataSourceConnection>;

class DirtyComponents : public ScopedSet<DocumentContextDataPtr, ComponentPtr> {
public:
    void clear() override {
        for (auto& component : getAll())
            component->clearDirty();

        ScopedSet<DocumentContextDataPtr, ComponentPtr>::clear();
    }

    std::set<ComponentPtr> extractScope(const DocumentContextDataPtr& documentData) override {
        auto erased = ScopedSet<DocumentContextDataPtr, ComponentPtr>::extractScope(documentData);
        for (auto& component : erased)
            component->clearDirty();

        return erased;
    }
};

/**
 * Small utilities to check the pointer before dereference.
 */
template<class T>
T& deref(const std::unique_ptr<T>& ptr) {
    if (ptr == nullptr)
        aplThrow("Can't dereference");
    return *ptr;
}

template<class T>
T& deref(const std::shared_ptr<T>& ptr) {
    if (ptr == nullptr)
        aplThrow("Can't dereference");
    return *ptr;
}

/**
 * Common data which is shared between rendered documents owned by one Core instance.
 */
class SharedContextData : public NonCopyable, public Counter<SharedContextData>,
    public std::enable_shared_from_this<SharedContextData> {
public:
    /**
     * Stock constructor
     * @param root RootContext pointer
     * @param metrics Display metrics
     * @param config Configuration settings
     */
    SharedContextData(const CoreRootContextPtr& root,
                      const Metrics& metrics,
                      const RootConfig& config);

    /**
     * Dummy constructor. Only used internally for test contexts.
     * @param config Configuration settings
     */
    SharedContextData(const RootConfig& config);

    ~SharedContextData();

    /**
     * Terminate common managers/processing
     */
    void halt();

    DocumentManager& documentManager() const { return deref(mDocumentManager); }
    DocumentRegistrar& documentRegistrar() const { return deref(mDocumentRegistrar); }
    FocusManager& focusManager() const { return deref(mFocusManager); }
    HoverManager& hoverManager() const { return deref(mHoverManager); }
    PointerManager& pointerManager() const { return deref(mPointerManager); }
    KeyboardManager& keyboardManager() const { return deref(mKeyboardManager); }
    LayoutManager& layoutManager() const { return deref(mLayoutManager); }
    MediaManager& mediaManager() const { return deref(mMediaManager); }
    MediaPlayerFactory& mediaPlayerFactory() const { return deref(mMediaPlayerFactory); }
    TickScheduler& tickScheduler() const { return deref(mTickScheduler); }
    DirtyComponents& dirtyComponents() const { return deref(mDirtyComponents); }
    TimeManager& timeManager() const { return deref(mTimeManager); }
    UIDGenerator& uidGenerator() const { return deref(mUniqueIdGenerator); }
    EventManager& eventManager() const { return deref(mEventManager); }
    DependantManager& dependantManager() const { return deref(mDependantManager); }

    const YGConfigRef& ygconfig() const { return mYGConfigRef; }

    /**
     * @return The installed text measurement for this context.
     */
    const TextMeasurementPtr& measure() const { return mTextMeasurement; }

    /**
     * @return True if the screen lock is currently being held by a command.
     */
    bool screenLock() const { return mScreenLockCount > 0; }

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

#ifdef SCENEGRAPH
    /**
     * @return A cache of TextProperties
     */
    sg::TextPropertiesCache& textPropertiesCache() { return *mTextPropertiesCache; }
#endif // SCENEGRAPH

private:
    std::string mRequestedVersion;
    std::unique_ptr<DocumentRegistrar> mDocumentRegistrar;
    std::unique_ptr<FocusManager> mFocusManager;
    std::unique_ptr<HoverManager> mHoverManager;
    std::unique_ptr<PointerManager> mPointerManager;
    std::unique_ptr<KeyboardManager> mKeyboardManager;
    std::unique_ptr<LayoutManager> mLayoutManager;
    std::unique_ptr<TickScheduler> mTickScheduler;
    std::unique_ptr<DirtyComponents> mDirtyComponents;
    std::unique_ptr<UIDGenerator> mUniqueIdGenerator;
    std::unique_ptr<EventManager> mEventManager;
    std::unique_ptr<DependantManager> mDependantManager;

    const DocumentManagerPtr mDocumentManager;
    std::shared_ptr<TimeManager> mTimeManager;
    std::shared_ptr<MediaManager> mMediaManager;
    std::shared_ptr<MediaPlayerFactory> mMediaPlayerFactory;

    YGConfigRef mYGConfigRef;
    TextMeasurementPtr mTextMeasurement;
    int mScreenLockCount = 0;
    LruCache<TextMeasureRequest, YGSize> mCachedMeasures;
    LruCache<TextMeasureRequest, float> mCachedBaselines;

#ifdef SCENEGRAPH
    std::unique_ptr<sg::TextPropertiesCache> mTextPropertiesCache;
#endif // SCENEGRAPH
};


} // namespace apl

#endif //_APL_SHARED_CONTEXT_DATA_H
