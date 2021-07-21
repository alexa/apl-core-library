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

#include "apl/command/setvaluecommand.h"
#include "apl/time/sequencer.h"

namespace apl {

const static bool DEBUG_SET_VALUE = false;

const CommandPropDefSet&
SetValueCommand::propDefSet() const {
    static CommandPropDefSet sSetValueCommandProperties(CoreCommand::propDefSet(), {
            {kCommandPropertyComponentId, "",                    asString, kPropRequiredId },
            {kCommandPropertyProperty,    "",                    asString, kPropRequired },
            {kCommandPropertyValue,       Object::NULL_OBJECT(), asAny,    kPropRequired | kPropEvaluated }
    });

    return sSetValueCommandProperties;
}

ActionPtr
SetValueCommand::execute(const TimersPtr& timers, bool fastMode) {
    if (!calculateProperties())
        return nullptr;

    std::string property = mValues.at(kCommandPropertyProperty).asString();
    Object value = mValues.at(kCommandPropertyValue);
    LOG_IF(DEBUG_SET_VALUE) << "SetValue - property: "<< property << " value: "<< value;

    if (sComponentPropertyBimap.has(property)) {
        auto propKey = static_cast<PropertyKey>(sComponentPropertyBimap.at(property));
        mContext->sequencer().releaseResource({kExecutionResourceProperty, mTarget, propKey});
    }

    mTarget->setProperty(property, value);

    return nullptr;
}

} // namespace apl
