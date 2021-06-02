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

#ifndef _APL_FOCUS_MANAGER_H
#define _APL_FOCUS_MANAGER_H

#include <memory>
#include <map>
#include "apl/common.h"
#include "apl/focus/focusdirection.h"
#include "apl/focus/focusfinder.h"

namespace apl {

class CoreComponent;
class RootContextData;
class Rect;

static const std::string FOCUS_SEQUENCER = "__FOCUS_SEQUENCER";

/**
 * Class that allows to control APL focus and perform Spatial Navigation.
 * Runtime should use APIs exposed in @see RootContext to pass and acquire focus from APL controlled area.
 * In order to pass focus to APL core runtime should:
 * 1. Retrieve focusable areas using RootContext::getFocusableAreas()
 * 2. Pass received areas through runtime specific focus navigation processing algorithm and find
 *    appropriate one.
 * 3. Pass ID corresponding to chosen area along with focus movement direction and origin focused
 *    area brought to APL viewhost coordinate space to RootContext::setFocus().
 * 4. APL Core will respond with EventType::kEventTypeFocus containing reference to focused component
 *    and actually focused area.
 *
 * In order to directly request core to switch focus in any particular direction (in response to
 * accessibility system finishing read out of current item or any other platform-specific reason)
 * RootContext::nextFocus() should be used with an appropriate direction.
 *
 * In order to take focus from APL (in case if some other component was directly actioned (touched),
 * for example) runtime should call RootConfig::clearFocus(). It's always succeeds and no confirmation
 * event sent.
 *
 * In case if focus changed internally due to commands or Spatial Navigation
 * EventType::kEventTypeFocus will be sent with corresponding parameters.
 *
 * In case if focus needs to be released internally due to commands or Spatial Navigation
 * EventType::kEventTypeFocus will be sent without component reference which runtime should resolve
 * with true in case if focus should leave APL area of control and false if it should stay where it is.
 */
class FocusManager {
public:
    explicit FocusManager(const RootContextData& core);

    /**
     * Focus next available component based on provided parameters. Requires some component to be focused.
     * @param direction focus movement direction.
     * @return true if processed successfully, false otherwise.
     */
    bool focus(FocusDirection direction);

    /**
     * Focus next available component based on provided parameters.
     * @param direction focus movement direction.
     * @param origin Rect where focus originates from.
     * @return true if something was focused, false otherwise.
     */
    bool focus(FocusDirection direction, const Rect& origin);

    /**
     * Focus next available component based on provided parameters.
     * @param direction focus movement direction.
     * @param origin Rect where focus originates from.
     * @param root parent component to use as search hierarchy root.
     * @return true if something was focused, false otherwise.
     */
    bool focus(FocusDirection direction, const Rect& origin, const CoreComponentPtr& root);

    /**
     * Find next component to be focused based on provided parameters.
     * @param direction direction of search.
     * @return Found component, nullptr otherwise.
     */
    CoreComponentPtr find(FocusDirection direction);

    /**
     * Find next component to be focused based on provided parameters.
     * @param direction direction of search.
     * @param origin currently focused rectangle.
     * @return Found component, nullptr otherwise.
     */
    CoreComponentPtr find(FocusDirection direction, const Rect& origin);

    /**
     * Find next component to be focused based on provided parameters.
     * @param direction direction of search.
     * @param origin Component focus originates from if any, nullptr otherwise.
     * @param originRect currently focused rectangle.
     * @param root root of search hierarchy.
     * @return Found component, nullptr otherwise.
     */
    CoreComponentPtr find(FocusDirection direction, const CoreComponentPtr& origin, const Rect& originRect, const CoreComponentPtr& root);

    /**
     * Get focusable areas available from APL Core.
     * All dimensions is in APL viewport coordinate space.
     * @return map from ID to focusable area.
     */
    std::map<std::string, Rect> getFocusableAreas();

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
     * @param direction focus exit direction.
     * @param force true to release focus without viewhost confirmation.
     */
    void clearFocus(bool notifyViewhost, FocusDirection direction = kFocusDirectionNone, bool force = false);

    /**
     * @return The component that currently has focus or nullptr
     */
    CoreComponentPtr getFocus() { return mFocused.lock(); }

private:
    const RootContextData& mCore;
    std::unique_ptr<FocusFinder> mFinder;
    std::weak_ptr<CoreComponent> mFocused;

    void reportFocusedComponent();
    void clearFocusedComponent();
};

} // namespace apl

#endif //_APL_FOCUS_MANAGER_H
