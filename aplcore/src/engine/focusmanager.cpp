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


#include "apl/engine/event.h"
#include "apl/engine/focusmanager.h"
#include "apl/component/corecomponent.h"

namespace apl {

static const bool DEBUG_FOCUS = false;

void
FocusManager::setFocus(const CoreComponentPtr& component, bool notifyViewhost)
{
    // Specifying a null component will clear existing focus, if applicable
    if (!component) {
        clearFocus(notifyViewhost);
        return;
    }

    auto focused = mFocused.lock();

    LOG_IF(DEBUG_FOCUS) << focused << " -> " << component;

    // If you target the already focused component, we don't need to do any work
    if (focused == component)
        return;

    // Ignore an attempt focus a component that is disabled, non-actionable, or has inheritParentState=true
    if (!component->isFocusable() || component->getState().get(kStateDisabled) || component->getInheritParentState())
        return;

    // Clear the old focus
    if (focused) {
        focused->setState(kStateFocused, false);
        focused->executeOnBlur();
    }

    // Set the new focus
    mFocused = component;
    component->setState(kStateFocused, true);
    component->executeOnFocus();
    if (notifyViewhost)
        component->getContext()->pushEvent(Event(kEventTypeFocus, component));
}

void
FocusManager::releaseFocus(const std::shared_ptr<apl::CoreComponent>& component, bool notifyViewhost)
{
    auto focused = mFocused.lock();

    LOG_IF(DEBUG_FOCUS) << focused << " -> " << component;

    if (focused == component)
        clearFocus(notifyViewhost);
}

/**
 * Remove any existing focus
 */
void
FocusManager::clearFocus(bool notifyViewhost)
{
    auto focused = mFocused.lock();
    mFocused.reset();

    LOG_IF(DEBUG_FOCUS) << focused;

    if (focused) {
        focused->setState(kStateFocused, false);
        focused->executeOnBlur();
        if (notifyViewhost) {
            focused->getContext()->pushEvent(Event(kEventTypeFocus, nullptr));  // Indicate null focus
        }
    }
}

}
