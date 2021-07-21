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

#ifndef _APL_SEQUENCE_COMPONENT_H
#define _APL_SEQUENCE_COMPONENT_H

#include "apl/component/multichildscrollablecomponent.h"

namespace apl {

class SequenceComponent : public MultiChildScrollableComponent {
public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const Path& path);
    SequenceComponent(const ContextPtr& context, Properties&& properties, const Path& path);

    ComponentType getType() const override { return kComponentTypeSequence; };

protected:
    const ComponentPropDefSet& propDefSet() const override;
    const ComponentPropDefSet* layoutPropDefSet() const override;
    bool childrenUseSpacingProperty() const override { return true; }
    size_t estimateChildrenToCover(float distance, size_t baseChild) override;

};

} // namespace apl

#endif //_APL_SEQUENCE_COMPONENT_H
