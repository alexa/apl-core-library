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

#include "apl/engine/event.h"
#include "apl/time/sequencer.h"
#include "apl/content/rootconfig.h"
#include "apl/content/settings.h"
#include "apl/engine/styles.h"
#include "focusmanager.h"
#include "hovermanager.h"
#include "keyboardmanager.h"
#include "jsonresource.h"

namespace apl {

class RootConfig;
class Sequencer;
class FocusManager;
class HoverManager;
class KeyboardManager;

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
     */
    RootContextData(const Metrics& metrics,
                    const RootConfig& config,
                    const std::string& theme,
                    const std::string& requestedAPLVersion,
                    const SessionPtr& session);

    /**
     * Discontinue use of this data.  Inform all children that they are no longer alive.
     */
    void terminate()
    {
        assert(mSequencer);
        mSequencer->terminate();
        mTop = nullptr;
    }

    Styles& styles() const { return *mStyles; }
    Sequencer& sequencer() const { return *mSequencer; }
    FocusManager& focusManager() const { return *mFocusManager; }
    HoverManager& hoverManager() const { return *mHoverManager; }
    KeyboardManager& keyboardManager() const { return *mKeyboardManager; }
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
    const std::unique_ptr<Styles> mStyles;
    std::unique_ptr<Sequencer> mSequencer;
    std::unique_ptr<FocusManager> mFocusManager;
    std::unique_ptr<HoverManager> mHoverManager;
    std::unique_ptr<KeyboardManager> mKeyboardManager;
    YGConfigRef mYGConfigRef;
    TextMeasurementPtr mTextMeasurement;
    CoreComponentPtr mTop;         // The top component
    const RootConfig mConfig;
    int mScreenLockCount;
    Settings mSettings;
    SessionPtr mSession;
};


} // namespace apl

#endif //_APL_ROOT_CONTEXT_DATA_H
