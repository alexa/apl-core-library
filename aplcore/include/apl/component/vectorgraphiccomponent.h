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

#ifndef _APL_VECTOR_GRAPHIC_COMPONENT_H
#define _APL_VECTOR_GRAPHIC_COMPONENT_H

#include "apl/component/touchablecomponent.h"
#include "apl/component/mediacomponenttrait.h"

namespace apl {

class VectorGraphicComponent : public TouchableComponent,
                               public MediaComponentTrait {
public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const Path& path);
    VectorGraphicComponent(const ContextPtr& context, Properties&& properties, const Path& path);

    ComponentType getType() const override { return kComponentTypeVectorGraphic; };
    void initialize() override;
    void updateStyle() override;
    bool updateGraphic(const GraphicContentPtr& json) override;
    void clearDirty() override;
    std::shared_ptr<ObjectMap> createTouchEventProperties(const Point &point) const override;
    void postProcessLayoutChanges(bool first) override;

    bool isFocusable() const override;
    bool isActionable() const override;
    bool isTouchable() const override;

protected:
    const EventPropertyMap& eventPropertyMap() const override;
    const ComponentPropDefSet& propDefSet() const override;
    void processLayoutChanges(bool useDirtyFlag, bool first) override;
    void releaseSelf() override;

    std::string getVisualContextType() const override;
    bool setPropertyInternal(const std::string& key, const Object& value) override;
    std::pair<Object, bool> getPropertyInternal(const std::string& key) const override;

    // Media component trait overrides
    std::vector<URLRequest> getSources() override;
    EventMediaType mediaType() const override { return kEventMediaTypeVectorGraphic; }
    void onFail(const MediaObjectPtr&) override;
    void onLoad() override;

    // Component trait overrides
    CoreComponentPtr getComponent() override { return shared_from_corecomponent(); }

    // Release any existing vector graphic
    void releaseGraphic();
    // The graphic source has changed
    void sourcePropertyChanged();

#ifdef SCENEGRAPH
    // Common scene graph handling
    sg::LayerPtr constructSceneGraphLayer(sg::SceneGraphUpdates& sceneGraph) override;
    bool updateSceneGraphInternal(sg::SceneGraphUpdates& sceneGraph) override;
    bool fixMediaLayer(sg::SceneGraphUpdates& sceneGraph, const sg::LayerPtr& layer);
#endif // SCENEGRAPH

private:
    /**
     * When providing parameters to the graphic, use this component's explicit parameters property,
     * if specified. Otherwise fall back to exposing properties as implicit parameters.
     */
    Properties getGraphicParameters() { 
        const auto& explicitParameters = getCalculated(kPropertyParameters);
        return explicitParameters.empty() ? mProperties : Properties(explicitParameters);
    }

private:
    bool mOnLoadOnFailReported = false;
    bool mGraphicReplaced = false;
};


} // namespace apl

#endif //_APL_VECTOR_GRAPHIC_COMPONENT_H
