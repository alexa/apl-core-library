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

#ifndef _APL_VECTOR_GRAPHIC_COMPONENT_H
#define _APL_VECTOR_GRAPHIC_COMPONENT_H

#include "apl/component/touchablecomponent.h"

namespace apl {

class VectorGraphicComponent : public TouchableComponent {
public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const std::string& path);
    VectorGraphicComponent(const ContextPtr& context, Properties&& properties, const std::string& path);
    void release() override;

    ComponentType getType() const override { return kComponentTypeVectorGraphic; };
    void initialize() override;
    void updateStyle() override;
    bool updateGraphic(const GraphicContentPtr& json) override;
    void clearDirty() override;
    std::shared_ptr<ObjectMap> createTouchEventProperties(const Point &point) const override;

    bool isFocusable() const override;
    bool isActionable() const override;
    bool isTouchable() const override;

protected:
    const EventPropertyMap& eventPropertyMap() const override;
    const ComponentPropDefSet& propDefSet() const override;
    void processLayoutChanges(bool useDirtyFlag) override;
    std::string getVisualContextType() override;
    bool setPropertyInternal(const std::string& key, const Object& value) override;

};


} // namespace apl

#endif //_APL_VECTOR_GRAPHIC_COMPONENT_H
