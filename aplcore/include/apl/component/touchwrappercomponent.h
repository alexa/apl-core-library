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

#ifndef _APL_TOUCH_WRAPPER_COMPONENT_H
#define _APL_TOUCH_WRAPPER_COMPONENT_H

#include <utility>

#include "actionablecomponent.h"

namespace apl {

class TouchWrapperComponent : public ActionableComponent {
public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const std::string& path);
    TouchWrapperComponent(const ContextPtr& context, Properties&& properties, const std::string& path);

    ComponentType getType() const override { return kComponentTypeTouchWrapper; };
    void update(UpdateType type, float value) override;

    Object getValue() const override { return mState.get(kStateChecked); }

protected:

    const ComponentPropDefSet& propDefSet() const override;

    bool getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator) override;

private:
    bool singleChild() const override { return true; }


};

} // namespace apl

#endif //_APL_TOUCH_WRAPPER_COMPONENT_H
