/**
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "apl/content/rootconfig.h"
#include "apl/content/settings.h"
#include "apl/content/content.h"
#include "apl/engine/event.h"
#include "apl/extension/extensionmanager.h"
#include "apl/engine/focusmanager.h"
#include "apl/engine/hovermanager.h"
#include "apl/engine/jsonresource.h"
#include "apl/engine/keyboardmanager.h"
#include "apl/engine/styles.h"
#include "apl/livedata/livedatamanager.h"
#include "apl/time/sequencer.h"
#include "apl/touch/pointermanager.h"

namespace apl {

class RootContextData {
    friend class RootContext;

public:
    /**
     * Stock constructor
     * @param metrics Display metrics
     * @param config Configuration settings
     * @param theme Display theme
     * @param requestedAPLVersion Version of APL requested by the document
     * @param session Logging session
     * @param extensions Mapping of requested extension NAME -> URI
     */
    RootContextData(const Metrics& metrics,
                    const RootConfig& config,
                    const std::string& theme,
                    const std::string& requestedAPLVersion,
                    const SettingsPtr& settings,
                    const SessionPtr& session,
                    const std::vector<std::pair<std::string, std::string>>& extensions);

    ~RootContextData() {
        YGConfigFree(mYGConfigRef);
    }

    /**
     * Discontinue use of this data.  Inform all children that they are no longer alive.
     */
    void terminate();

    std::shared_ptr<Styles> styles() const { return mStyles; }
    Sequencer& sequencer() const { return *mSequencer; }
    FocusManager& focusManager() const { return *mFocusManager; }
    HoverManager& hoverManager() const { return *mHoverManager; }
    PointerManager& pointerManager() const { return *mPointerManager; }
    KeyboardManager& keyboardManager() const { return *mKeyboardManager; }
    LiveDataManager& dataManager() const { return *mDataManager; }
    ExtensionManager& extensionManager() const { return *mExtensionManager; }

    const YGConfigRef& ygconfig() const { return mYGConfigRef; }
    ComponentPtr top() const { return mTop; }
    const std::map<std::string, JsonResource>& layouts() const { return mLayouts; }
    const std::map<std::string, JsonResource>& commands() const { return mCommands; }
    const std::map<std::string, JsonResource>& graphics() const { return mGraphics; }
    const SessionPtr& session() const { return mSession; }

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

public:
    const int pixelWidth;
    const int pixelHeight;
    const double width;
    const double height;
    const double pxToDp;
    const std::string theme;
    const std::string requestedAPLVersion;

    std::queue<Event> events;
    std::set<ComponentPtr> dirty;

private:
    std::map<std::string, JsonResource> mLayouts;
    std::map<std::string, JsonResource> mCommands;
    std::map<std::string, JsonResource> mGraphics;
    std::shared_ptr<Styles> mStyles;
    std::unique_ptr<Sequencer> mSequencer;
    std::unique_ptr<FocusManager> mFocusManager;
    std::unique_ptr<HoverManager> mHoverManager;
    std::unique_ptr<PointerManager> mPointerManager;
    std::unique_ptr<KeyboardManager> mKeyboardManager;
    std::unique_ptr<LiveDataManager> mDataManager;
    std::unique_ptr<ExtensionManager> mExtensionManager;
    YGConfigRef mYGConfigRef;
    TextMeasurementPtr mTextMeasurement;
    CoreComponentPtr mTop;         // The top component
    const RootConfig mConfig;
    int mScreenLockCount;
    SettingsPtr mSettings;
    SessionPtr mSession;
};


} // namespace apl

#endif //_APL_ROOT_CONTEXT_DATA_H
