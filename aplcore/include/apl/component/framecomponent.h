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

#ifndef _APL_FRAME_COMPONENT_H
#define _APL_FRAME_COMPONENT_H

#include "corecomponent.h"

namespace apl {

class FrameComponent : public CoreComponent {
public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const Path& path);
    FrameComponent(const ContextPtr& context, Properties&& properties, const Path& path);

    ComponentType getType() const override { return kComponentTypeFrame; };

    void setHeight(const Dimension& height) override;
    void setWidth(const Dimension& width) override;

    void fixBorder(bool useDirtyFlag=true);

protected:
    const ComponentPropDefSet& propDefSet() const override;
    void assignProperties(const ComponentPropDefSet& propDefSet) override;

private:
    bool singleChild() const override { return true; }
};

} // namespace apl

#endif //_APL_FRAME_COMPONENT_H
