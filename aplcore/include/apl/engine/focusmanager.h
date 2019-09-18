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

#ifndef _APL_FOCUS_MANAGER_H
#define _APL_FOCUS_MANAGER_H

#include <memory>

namespace apl {

class CoreComponent;

class FocusManager {
public:
    /**
     * Assign focus to this component.
     * @param component The component to focus.  If null, will clear the focus.
     * @param notifyViewhost Flag to identify if viewhost notification required for this focus change.
     */
    void setFocus(const CoreComponentPtr& component, bool notifyViewhost);

    /**
     * Release the focus if it is set on this component.
     * @param component The component to release
     * @param notifyViewhost Flag to identify if viewhost notification required for this focus change.
     */
    void releaseFocus(const CoreComponentPtr& component, bool notifyViewhost);

    /**
     * Remove any existing focus.
     * @param notifyViewhost Flag to identify if viewhost notification required for this focus change.
     */
    void clearFocus(bool notifyViewhost);

    /**
     * @return The component that currently has focus or nullptr
     */
    CoreComponentPtr getFocus() { return mFocused.lock(); }

private:
    std::weak_ptr<CoreComponent> mFocused;
};

} // namespace apl

#endif //_APL_FOCUS_MANAGER_H
