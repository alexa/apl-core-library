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

#ifndef _APL_ACTIONABLE_COMPONENT_H
#define _APL_ACTIONABLE_COMPONENT_H

#include "corecomponent.h"

namespace apl {

/**
 * An actionable component is one that accepts keyboard focus.
 */
class ActionableComponent : public CoreComponent {
protected:
    ActionableComponent(const ContextPtr& context, Properties&& properties, const std::string& path) :
        CoreComponent(context, std::move(properties), path)
    {}

    bool isFocusable() const override { return true; }

    void executeOnBlur() override;

    void executeOnFocus() override;

    bool executeKeyHandlers(KeyHandlerType type, const ObjectMapPtr& keyboard)  override;

    const ComponentPropDefSet& propDefSet() const override;

};

} // namespace apl

#endif // _APL_ACTIONABLE_COMPONENT_H
