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

#ifndef _APL_ACCESSIBILITY_H
#define _APL_ACCESSIBILITY_H

#include <string>
#include <vector>
#include <functional>

#include <rapidjson/document.h>

#include "apl/component/componentproperties.h"

namespace apl {
namespace sg {

/**
 * The public interface to accessibility features on a layer.
 *
 * This is a concrete class.  A layer with accessibility features will have an
 * instance of this class attached through a shared pointer.
 */
class Accessibility {
public:
    using ActionCallback = std::function<void(const std::string&)>;

    // This object is a simplification of the actual AccessibilityAction used in core.
    struct Action {
        std::string name;
        std::string label;
        bool enabled;

        bool operator==(const Action& rhs) const {
            return name == rhs.name && label == rhs.label && enabled == rhs.enabled;
        }

        bool operator!=(const Action& rhs) const {
            return name != rhs.name || label != rhs.label || enabled != rhs.enabled;
        }

        std::string toDebugString() const {
            return name + " label="+label + " enabled=" + (enabled ? "true" : "false");
        }
    };

    explicit Accessibility(ActionCallback callback)
        : mActionCallback(callback)
    {}

    // Execute this from the view host to trigger an action callback
    void executeCallback(const std::string& name) const;

    bool setLabel(const std::string& label);
    std::string getLabel() const { return mLabel; }

    bool setRole(Role role);
    Role getRole() const { return mRole; }

    void appendAction(const std::string& name, const std::string& label, bool enabled);

    const std::vector<Action>& actions() const { return mActions; }

    bool empty() const { return mLabel.empty() && mRole == kRoleNone && mActions.empty();}

    friend bool operator==(const Accessibility& lhs, const Accessibility& rhs){
        return lhs.mLabel == rhs.mLabel && lhs.mRole == rhs.mRole && lhs.mActions == rhs.mActions;
    }

    friend bool operator!=(const Accessibility& lhs, const Accessibility& rhs){
        return lhs.mLabel != rhs.mLabel || lhs.mRole != rhs.mRole || lhs.mActions != rhs.mActions;
    }

    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

protected:
    ActionCallback mActionCallback;
    std::string mLabel;
    Role mRole = kRoleNone;
    std::vector<Action> mActions;
};

} // namespace sg
} // namespace apl

#endif // _APL_ACCESSIBILITY_H
