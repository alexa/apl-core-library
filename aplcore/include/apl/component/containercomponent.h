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

#ifndef _APL_CONTAINER_COMPONENT_H
#define _APL_CONTAINER_COMPONENT_H

#include "corecomponent.h"

namespace apl {

class ContainerComponent : public CoreComponent {
public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const Path& path);
    ContainerComponent(const ContextPtr& context, Properties&& properties, const Path& path);

    ComponentType getType() const override { return kComponentTypeContainer; };

    const ComponentPropDefSet* layoutPropDefSet() const override;

    void processLayoutChanges(bool useDirtyFlag, bool first) override;

protected:
    const ComponentPropDefSet& propDefSet() const override;

private:
    bool multiChild() const override { return true; }
    std::map<int,int> calculateChildrenVisualLayer(const std::map<int, float>& visibleIndexes,
                                                   const Rect& visibleRect, int visualLayer) override;
};

} // namespace apl

#endif //_APL_CONTAINER_COMPONENT_H
