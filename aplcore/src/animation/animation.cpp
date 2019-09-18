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

#include "apl/engine/evaluate.h"
#include "apl/primitives/object.h"
#include "apl/animation/animation.h"
#include "apl/component/corecomponent.h"
#include "apl/component/componentproperties.h"
#include "apl/utils/session.h"

namespace apl {

Object
Animation::create(const Context& context, const std::vector<Object>& array)
{
    for (const auto& m : array) {
        if (!m.isMap()) {
            CONSOLE_CTX(context) << "Animation has illegal entry";
            continue;
        }

        if (!m.has("property")) {
            CONSOLE_CTX(context) << "Animation missing 'property' value";
            continue;
        }

        if (!m.has("to")) {
            CONSOLE_CTX(context) << "Animation missing 'to' value";
            continue;
        }

        std::string property = propertyAsString(context, m, "property");


        bool hasFrom = m.has("from");
        float from = propertyAsDouble(context, m, "from", -1);
        float to = propertyAsDouble(context, m, "to", -1);
    }
    return Object::NULL_OBJECT();
}

void
Animation::initialize(CoreComponent& component)
{

}

void
Animation::apply(CoreComponent& component, float alpha)
{
    for (auto& m : mAnimations) {
        switch (m.type) {
            case AnimationValue::OPACITY:
                component.setProperty(kPropertyOpacity, alpha);   // TODO: Fix from/to
                break;
            case AnimationValue::TRANSFORM:
                break;
        }
    }
}

} // namespace apl
