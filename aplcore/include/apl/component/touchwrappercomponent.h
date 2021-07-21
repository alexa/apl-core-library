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

#ifndef _APL_TOUCH_WRAPPER_COMPONENT_H
#define _APL_TOUCH_WRAPPER_COMPONENT_H

#include <utility>

#include "apl/component/touchablecomponent.h"

namespace apl {

class TouchWrapperComponent : public TouchableComponent {
public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const Path& path);
    TouchWrapperComponent(const ContextPtr& context, Properties&& properties, const Path& path);

    ComponentType getType() const override { return kComponentTypeTouchWrapper; };

    Object getValue() const override { return mState.get(kStateChecked); }

    /**
     * Inject component that will replace current child of touch wrapper. While likely short lived - it will exist for
     * customer interaction time + any animations left.
     * Component will inherit TouchWrapper's internal dimensions.
     * @param child Component to inject.
     * @param above true if should be above existing child, false otherwise.
     */
    void injectReplaceComponent(const CoreComponentPtr& child, bool above);

    /**
     * Reset child's position to default (in case of TW it's always relative and always 1 child).
     * Makes sense only in conjunction with injectReplaceComponent (as yoga does not play nicely with single absolute
     * positioned child here).
     */
    void resetChildPositionType();

protected:
    const ComponentPropDefSet& propDefSet() const override;

private:
    bool singleChild() const override { return true; }


};

} // namespace apl

#endif //_APL_TOUCH_WRAPPER_COMPONENT_H
