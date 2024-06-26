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

#ifndef _APL_TEXT_COMPONENT_H
#define _APL_TEXT_COMPONENT_H

#include "apl/component/corecomponent.h"
#include "apl/primitives/range.h"
#include "apl/scenegraph/textproperties.h"

namespace apl {

class TextComponent : public CoreComponent {
public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const Path& path);
    TextComponent(const ContextPtr& context, Properties&& properties, const Path& path);

    ComponentType getType() const override { return kComponentTypeText; };

    void checkKaraokeTargetColor();

    Object getValue() const override;
    void preLayoutProcessing(bool useDirtyFlag) override;

#ifdef SCENEGRAPH
    sg::TextLayoutPtr getTextLayout() const { return mLayout; }

    bool setKaraokeLine(Range byteRange);
    void clearKaraokeLine() { setKaraokeLine(Range{}); }
    Rect getKaraokeBounds();
#endif // SCENEGRAPH

private:
    void updateTextAlign(bool useDirtyFlag);
    void ensureTextProperties();
    void ensureTextLayout();
    void onTextLayout();
    void postProcessLayoutChanges(bool first) override;

protected:
    void handleLayoutDirectionChange(bool useDirtyFlag) override { updateTextAlign(useDirtyFlag); };

    const EventPropertyMap & eventPropertyMap() const override;
    const ComponentPropDefSet& propDefSet() const override;
    void assignProperties(const ComponentPropDefSet& propDefSet) override;

    std::string getVisualContextType() const override;

    YGSize textMeasure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) override;
    float textBaseline(float width, float height) override;

private:
    sg::TextPropertiesPtr mTextProperties;
    sg::TextChunkPtr mTextChunk;
    sg::TextLayoutPtr mLayout;
    bool mLayoutPossiblyStale;

#ifdef SCENEGRAPH
protected:
    // Common scene graph handling
    sg::LayerPtr constructSceneGraphLayer(sg::SceneGraphUpdates& sceneGraph) override;
    bool updateSceneGraphInternal(sg::SceneGraphUpdates& sceneGraph) override;

private:
    Point calcSceneGraphOffset() const;
    void populateTextNodes(sg::Node *transform);
#endif // SCENEGRAPH
};

} // namespace apl

#endif //_APL_TEXT_COMPONENT_H
