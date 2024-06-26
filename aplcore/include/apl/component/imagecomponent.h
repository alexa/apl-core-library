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

#ifndef _APL_IMAGE_COMPONENT_H
#define _APL_IMAGE_COMPONENT_H

#include "apl/component/mediacomponenttrait.h"

namespace apl {

class ImageComponent : public CoreComponent,
                       public MediaComponentTrait {
public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const Path& path);

    ImageComponent(const ContextPtr& context, Properties&& properties, const Path& path);

    ComponentType getType() const override { return kComponentTypeImage; }

    void postProcessLayoutChanges(bool first) override;

protected:
    const EventPropertyMap& eventPropertyMap() const override;
    const ComponentPropDefSet& propDefSet() const override;

    std::string getVisualContextType() const override;

    void releaseSelf() override;

    /// Media component trait overrides
    std::vector<URLRequest> getSources() override;
    EventMediaType mediaType() const override { return kEventMediaTypeImage; }
    void onFail(const MediaObjectPtr&) override;
    void onLoad() override;

    // Component trait overrides
    CoreComponentPtr getComponent() override { return shared_from_corecomponent(); }

#ifdef SCENEGRAPH
    // Common scene graph handling
    sg::LayerPtr constructSceneGraphLayer(sg::SceneGraphUpdates& sceneGraph) override;
    bool updateSceneGraphInternal(sg::SceneGraphUpdates& sceneGraph) override;
#endif // SCENEGRAPH

private:
    bool mOnLoadOnFailReported = false;
#ifdef SCENEGRAPH
    sg::FilterPtr getFilteredImage();

    struct ImageRects {
        Rect source;  // Portion of image to draw, in pixels
        Rect target;  // Target rectangle to draw in the DP coordinate system
    };

    ImageRects getImageRects(const sg::FilterPtr& filter);
#endif // SCENEGRAPH
};

} // namespace apl

#endif //_APL_IMAGE_COMPONENT_H
