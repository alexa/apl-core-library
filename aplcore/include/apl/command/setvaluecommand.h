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

#ifndef _APL_SET_VALUE_COMMAND_H
#define _APL_SET_VALUE_COMMAND_H

#include "apl/command/corecommand.h"

namespace apl {

const static bool DEBUG_SET_VALUE = false;

class SetValueCommand : public CoreCommand {

public:
    static CommandPtr create(const ContextPtr& context,
                             Properties&& properties,
                             const CoreComponentPtr& base) {
        auto ptr = std::make_shared<SetValueCommand>(context, std::move(properties), base);
        return ptr->validate() ? ptr : nullptr;
    }

    SetValueCommand(const ContextPtr& context, Properties&& properties, const CoreComponentPtr& base)
            : CoreCommand(context, std::move(properties), base)
    {}

    const CommandPropDefSet& propDefSet() const override {
        static CommandPropDefSet sSetValueCommandProperties(CoreCommand::propDefSet(), {
                {kCommandPropertyComponentId, "",                    asString, kPropRequiredId },
                {kCommandPropertyProperty,    "",                    asString, kPropRequired },
                {kCommandPropertyValue,       Object::NULL_OBJECT(), asAny,    kPropRequired }
        });

        return sSetValueCommandProperties;
    };

    CommandType type() const override { return kCommandTypeSetValue; }

    ActionPtr execute(const TimersPtr& timers, bool fastMode) override {
        if (!calculateProperties())
            return nullptr;

        std::string property = mValues.at(kCommandPropertyProperty).asString();
        Object value = mValues.at(kCommandPropertyValue);
        LOG_IF(DEBUG_SET_VALUE) << "SetValue - property: "<< property << " value: "<< value;
        mTarget->setProperty(property, value);

        return nullptr;
    }
};

} // namespace apl

#endif // _APL_SET_VALUE_COMMAND_H
