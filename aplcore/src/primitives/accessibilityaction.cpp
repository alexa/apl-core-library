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

#include "apl/primitives/accessibilityaction.h"
#include "apl/component/corecomponent.h"
#include "apl/engine/typeddependant.h"
#include "apl/engine/dependantmanager.h"
#include "apl/engine/evaluate.h"
#include "apl/engine/propdef.h"
#include "apl/primitives/boundsymbolset.h"
#include "apl/time/sequencer.h"
#include "apl/utils/session.h"

namespace apl {

/**
 * The accessibility action can have a dynamic "enabled" property.  This dependant is used
 * to track when the equation driving "enabled" changes.  If we need to make other accessibility
 * action properties dynamic in the future (such as "label"), this class will need to be modified
 * so that we can differentiate which property is being driven.
 */

using AccessibilityActionDependent = TypedDependant<AccessibilityAction, AccessibilityActionKey>;

std::shared_ptr<AccessibilityAction>
AccessibilityAction::create(const CoreComponentPtr& component, const Object& object)
{
    auto context = component->getContext();

    if (object.is<AccessibilityAction>())
        return object.get<AccessibilityAction>();

    if (!object.isMap()) {
        CONSOLE(context) << "Invalid accessibility action";
        return nullptr;
    }

    auto name = propertyAsString(*context, object, "name");
    if (name.empty()) {
        CONSOLE(context) << "Accessibility action missing name";
        return nullptr;
    }

    auto label = propertyAsString(*context, object, "label");
    if (label.empty()) {
        CONSOLE(context) << "Accessibility action name='" << name << "' missing label";
        return nullptr;
    }

    auto aa = std::make_shared<AccessibilityAction>(component,
                                                    std::move(name),
                                                    std::move(label));
    aa->initialize(context, object);
    return aa;
}

void
AccessibilityAction::initialize(const ContextPtr& context, const Object& object)
{
    // The enabled property is allowed to be dynamic
    if (object.has("enabled")) {
        auto assigned = object.get("enabled");
        if (assigned.isString()) {  // It may be a data-binding
            auto result = parseAndEvaluate(*context, assigned);
            if (!result.symbols.empty())
                AccessibilityActionDependent::create(shared_from_this(),
                                                     kAccessibilityActionEnabled,
                                                     std::move(result.expression),
                                                     context,
                                                     sBindingFunctions.at(kBindingTypeBoolean),
                                                     std::move(result.symbols));
            mEnabled = result.value.asBoolean();
        }
        else {
            mEnabled = assigned.asBoolean();
        }
    }

    mCommands = arrayifyPropertyAsObject(*context, object, "commands", "command");
}

std::string
AccessibilityAction::toDebugString() const {
    return "AccessibilityAction<" + mName + " '" + mLabel + "' enabled="+(mEnabled?"true":"false")+">";
}

rapidjson::Value
AccessibilityAction::serialize(rapidjson::Document::AllocatorType& allocator) const
{
    using rapidjson::Value;
    rapidjson::Value v(rapidjson::kObjectType);
    v.AddMember("name", Value(mName.c_str(), allocator).Move(), allocator);
    v.AddMember("label", Value(mLabel.c_str(), allocator).Move(), allocator);
    v.AddMember("enabled", mEnabled, allocator);
    // Commands are not serialized
    return v;
}

void
AccessibilityAction::setValue(AccessibilityActionKey key, const Object& value, bool useDirtyFlag)
{
    assert(key == kAccessibilityActionEnabled);
    auto enabled = value.getBoolean();
    if (enabled != mEnabled) {
        mEnabled = enabled;
        if (useDirtyFlag) {
            auto lock = mComponent.lock();
            if (lock)
                lock->setDirty(kPropertyAccessibilityActions);
        }
    }
}

} // namespace apl
