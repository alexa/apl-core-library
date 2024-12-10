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

#ifndef ACTIONDATA_H
#define ACTIONDATA_H

#include "apl/common.h"
#include "apl/component/component.h"

namespace apl {
/**
 * Class to hold the details of an Action such as the target component, command etc.
 */
class ActionData {
public:
    explicit ActionData() : mActionHint("None") {}

    /**
     * The target component associated with an action
     * @param target
     * @return this object for chaining
     */
    ActionData target(const ComponentPtr& target) {
        mTarget = target;
        return *this;
    }

    /**
     * The actionHint for the action
     * @param actionHint
     * @return
     */
    ActionData actionHint(const char* actionHint) {
      mActionHint = actionHint;
      return *this;
    }

    /**
     * If the action is associated with a command, then the provenance of the command
     * @param commandProvenance
     * @return
     */
    ActionData commandProvenance(const std::string& commandProvenance) {
        mCommandProvenance = commandProvenance;
        return *this;
    }

    /**
     * Serialize the action detail into a rapidjson object
     * @param allocator
     * @return rapidjson representation of the action detail
     */
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const {
        rapidjson::Value action(rapidjson::kObjectType);
        rapidjson::Value component(rapidjson::kObjectType);
        if (mTarget) {
            component.AddMember("provenance", rapidjson::Value(mTarget->provenance().c_str(), allocator).Move(), allocator);
            component.AddMember("targetComponentType", rapidjson::Value(mTarget->name().c_str(), allocator).Move(), allocator);
            component.AddMember("targetId", rapidjson::Value(mTarget->getId().c_str(), allocator).Move(), allocator);
            action.AddMember("component", component.Move(), allocator);
        }
        action.AddMember("actionHint", rapidjson::Value(mActionHint, allocator).Move(), allocator);
        if (!mCommandProvenance.empty())
            action.AddMember("commandProvenance", rapidjson::Value(mCommandProvenance.c_str(), allocator).Move(), allocator);
        return action;
    }

private:
    ComponentPtr mTarget;
    const char* mActionHint;
    std::string mCommandProvenance;
};

} // namespace apl

#endif //ACTIONDATA_H
