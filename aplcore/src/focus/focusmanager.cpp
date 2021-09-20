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

#include "apl/focus/focusmanager.h"
#include "apl/action/scrolltoaction.h"
#include "apl/component/corecomponent.h"
#include "apl/engine/event.h"
#include "apl/engine/rootcontextdata.h"
#include "apl/primitives/rect.h"
#include "apl/time/timemanager.h"

namespace apl {

static const bool DEBUG_FOCUS = false;
static const std::string FOCUS_RELEASE_SEQUENCER = "__FOCUS_RELEASE_SEQUENCER";

FocusManager::FocusManager(const RootContextData& core) :
    mCore(core), mFinder(std::unique_ptr<FocusFinder>(new FocusFinder()))
{}

void
FocusManager::reportFocusedComponent()
{
    auto focused = mFocused.lock();
    if (focused) {
        EventBag bag;
        Rect bounds;
        focused->getBoundsInParent(mCore.top(), bounds);
        bag.emplace(kEventPropertyValue, Object(std::move(bounds)));
        focused->getContext()->pushEvent(Event(kEventTypeFocus, std::move(bag), focused));
    }
}

/**
 * Scroll component into view of it's parent, then do the same for parent, and so on recursively.
 */
static inline ActionPtr
scrollIntoView(const std::shared_ptr<TimeManager>& timers, const CoreComponentPtr& component)
{
    auto duration = component->getContext()->getRootConfig().getProperty(RootProperty::kScrollOnFocusDuration).getDouble();
    ActionList actions;
    auto current = component;
    while (true) {
        auto action = ScrollToAction::makeUsingSnap(timers, current, duration);
        if (!action) break;
        if (action->isPending()) actions.emplace_back(action);
        current = action->getScrollableContainer();
        action = nullptr;
    }
    if (actions.empty()) return nullptr;
    return Action::makeAll(timers, actions);
}

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

    if (notifyViewhost) {
        auto timers = mCore.rootConfig().getTimeManager();
        mCore.sequencer().terminateSequencer(FOCUS_RELEASE_SEQUENCER);
        // Get target into viewport (a.g. scroll it in)
        auto action = scrollIntoView(timers, component);
        if (action && action->isPending()) {
            auto wrapped = Action::wrapWithCallback(timers, action, [this](bool, const ActionPtr&) {
                reportFocusedComponent();
            });
            mCore.sequencer().attachToSequencer(wrapped, FOCUS_SEQUENCER);
        } else {
            reportFocusedComponent();
        }
    }
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
FocusManager::clearFocus(bool notifyViewhost, FocusDirection direction, bool force)
{
    auto focused = mFocused.lock();
    LOG_IF(DEBUG_FOCUS) << focused;

    if (focused) {
        if (!notifyViewhost) {
            clearFocusedComponent();
            return;
        }

        // If forced we just release straight away without asking for viewhost confirmation
        if (force) {
            clearFocusedComponent();
            focused->getContext()->pushEvent(Event(kEventTypeFocus, nullptr));  // Indicate null focus
        } else {
            auto timers = std::dynamic_pointer_cast<Timers>(mCore.rootConfig().getTimeManager());
            Rect bounds;
            focused->getBoundsInParent(mCore.top(), bounds);
            auto boundsObject = Object(std::move(bounds));
            auto action = Action::make(timers, [focused, boundsObject, direction](ActionRef ref) {
                EventBag bag;
                bag.emplace(kEventPropertyDirection, direction);
                bag.emplace(kEventPropertyValue, boundsObject);
                focused->getContext()->pushEvent(Event(kEventTypeFocus, std::move(bag), nullptr, ref));
            });

            auto wrapped = Action::wrapWithCallback(timers, action, [this](bool, const ActionPtr& ptr) {
                if (ptr->getIntegerArgument() > 0)
                    clearFocusedComponent();
            });
            mCore.sequencer().terminateSequencer(FOCUS_SEQUENCER);
            mCore.sequencer().attachToSequencer(wrapped, FOCUS_RELEASE_SEQUENCER);
        }
    }
}

void
FocusManager::clearFocusedComponent()
{
    auto focused = mFocused.lock();
    mFocused.reset();
    if (focused) {
        focused->setState(kStateFocused, false);
        focused->executeOnBlur();
    }
}

inline Rect generateOrigin(FocusDirection direction, const Rect& viewport)
{
    Rect offsetRect = viewport;
    switch (direction) {
        case kFocusDirectionLeft:
            offsetRect.offset(Point(viewport.getWidth(), 0));
            break;
        case kFocusDirectionRight:
            offsetRect.offset(Point(-viewport.getWidth(), 0));
            break;
        case kFocusDirectionUp:
            offsetRect.offset(Point(0, viewport.getHeight()));
            break;
        case kFocusDirectionDown:
            offsetRect.offset(Point(0, -viewport.getHeight()));
            break;
        default:
            return {};
    }
    return offsetRect;
}

bool
FocusManager::focus(FocusDirection direction)
{
    if (mFinder) {
        auto focused = mFocused.lock();
        if (focused) {
            auto next = mFinder->findNext(focused, direction);
            if (next) {
                setFocus(next, true);
            }
            else {
                // Release out of core in an appropriate direction.
                clearFocus(true, direction);
            }
            return true;
        }
        else {
            auto origin = generateOrigin(direction, mCore.top()->getCalculated(kPropertyBounds).getRect());
            return focus(direction, origin);
        }
    }
    return false;
}

bool
FocusManager::focus(FocusDirection direction, const Rect& origin)
{
    if (mFinder) {
        auto next = find(direction, origin);
        if (next) {
            setFocus(next, true);
        }
        else {
            // Release out of core in an appropriate direction.
            clearFocus(true, direction);
        }
        return true;
    }
    return false;
}

bool
FocusManager::focus(FocusDirection direction, const Rect& origin, const CoreComponentPtr& root)
{
    if (mFinder) {
        auto next = mFinder->findNext(mFocused.lock(), origin, direction, root);
        if (next) {
            setFocus(next, true);
        }
        else if (root->isFocusable()) {
            setFocus(root, true);
        }
        return true;
    }
    return false;
}

CoreComponentPtr
FocusManager::find(FocusDirection direction)
{
    auto focused = mFocused.lock();
    if (focused) {
        return mFinder->findNext(focused, direction);
    } else {
        return nullptr;
    }
}

CoreComponentPtr
FocusManager::find(FocusDirection direction, const Rect& origin)
{
    return mFinder->findNext(mFocused.lock(), origin, direction, std::dynamic_pointer_cast<CoreComponent>(mCore.top()));
}

CoreComponentPtr
FocusManager::find(FocusDirection direction, const CoreComponentPtr& origin, const Rect& originRect, const CoreComponentPtr& root)
{
    return mFinder->findNext(origin, originRect, direction, root);
}

std::map<std::string, Rect>
FocusManager::getFocusableAreas()
{
    std::map<std::string, Rect> result;
    auto root = std::dynamic_pointer_cast<CoreComponent>(mCore.top());
    auto focusables = mFinder->getFocusables(root, false);
    if(root->isFocusable()) {
        focusables.push_back(root);
    }

    for (const auto& focusable : focusables) {
        Rect outRect;
        focusable->getBoundsInParent(root, outRect);
        result.emplace(focusable->getUniqueId(), outRect);
    }

    return result;
}

}
