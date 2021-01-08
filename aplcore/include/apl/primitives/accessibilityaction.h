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

#ifndef _APL_ACCESSIBILITY_ACTION_H
#define _APL_ACCESSIBILITY_ACTION_H

#include "apl/engine/recalculatetarget.h"
#include "apl/primitives/objectdata.h"
#include "apl/utils/userdata.h"

namespace apl {

enum AccessibilityActionKey {
    kAccessibilityActionEnabled
};

/**
 * An AccessibilityAction is a named method of invoking commands on a component.  The name may be one of the
 * predefined standard accessibility actions (e.g., "activate", "doubletap") or may be defined by the APL document
 * author.  The label is a human-readable, localized text string used to inform the user of what accessibility
 * actions may be invoked.  Accessibility actions may be individually enabled or disabled based on data-bound
 * expressions.
 *
 * Each accessibility action has an attached set of commands to be executed when the action is invoked.  If the
 * command list is omitted, the standard accessibility actions invoke the standard commands associated with
 * that action.  For example, the standard "activate" accessibility action will invoke the "onPress" commands.
 *
 * An accessibility action is programatically invoked by calling:
 *
 *     component->update(kUpdateAccessibilityAction, NAME);
 *
 * Although accessibility actions are designed for accessibility systems, there is no prohibition on using
 * them for other uses.  For example, a test harness that needs to execute a series of different commands in the
 * context of some component could define those commands in the component's action block and then invoke them
 * individually.
 */
class AccessibilityAction : public ObjectData,
                            public std::enable_shared_from_this<AccessibilityAction>,
                            public RecalculateTarget<AccessibilityActionKey>,
                            public UserData<AccessibilityAction> {
public:
    friend class AccessibilityActionDependant;

    /**
     * Create an accessibility action from an object description with "name", "label", and other properties.
     * @param component The component the accessibility action will be attached to.
     * @param object A map object containing the properties that define the accessibility action.
     * @return The accessibility action or nullptr if the action can't be created.
     */
    static std::shared_ptr<AccessibilityAction> create(const CoreComponentPtr& component, const Object& object);

    /**
     * @return The name of the accessibility action
     */
    std::string getName() const { return mName; }

    /**
     * @return The label
     */
    std::string getLabel() const { return mLabel; }

    /**
     * @return True if this accessibility action is enabled
     */
    bool enabled() const { return mEnabled; }

    /**
     * @return An object containing the unparsed commands to be executed
     */
    const Object& getCommands() const { return mCommands; }

    // Standard ObjectData methods
    bool operator==(const ObjectData& other) const override {
        return *this == dynamic_cast<const AccessibilityAction&>(other);
    }

    bool operator==(const AccessibilityAction& other) const {
        return mName == other.mName &&
            mLabel == other.mLabel &&
            mEnabled == other.mEnabled &&
            mCommands == other.mCommands &&
            !mComponent.owner_before(other.mComponent) &&
            !other.mComponent.owner_before(mComponent);
    }

    bool empty() const override { return false; }

    bool truthy() const override { return true; }

    std::string toDebugString() const override;

    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const override;

    /**
     * Internal constructor.  Do not use directly; instead, use the "create" static method.
     */
    AccessibilityAction(const CoreComponentPtr& component, std::string name, std::string label)
        : mComponent(component), mName(std::move(name)), mLabel(std::move(label)), mEnabled(true) {}

protected:
    void initialize(const ContextPtr& context, const Object& object);
    void setEnabled(bool enabled, bool useDirtyFlag);

private:
    std::weak_ptr<CoreComponent> mComponent;
    std::string mName;
    std::string mLabel;
    bool mEnabled;
    Object mCommands;
};

} // namespace apl

#endif // _APL_ACCESSIBILITY_ACTION_H
