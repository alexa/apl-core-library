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

#ifndef _APL_GRAPHIC_ELEMENT_H
#define _APL_GRAPHIC_ELEMENT_H

#include <memory>
#include <set>

#include "apl/apl_config.h"
#include "apl/common.h"
#include "apl/utils/counter.h"
#include "apl/utils/noncopyable.h"
#include "apl/utils/userdata.h"
#include "apl/engine/uidobject.h"
#include "apl/primitives/object.h"
#include "apl/engine/properties.h"
#include "apl/engine/propertymap.h"
#include "apl/engine/recalculatetarget.h"

#include "apl/graphic/graphicproperties.h"
#ifdef SCENEGRAPH
#include "apl/scenegraph/common.h"
#endif // SCENEGRAPH

namespace apl {

class GraphicPropDefSet;

using GraphicChildren = std::vector<GraphicElementPtr>;
using GraphicDirtyProperties = std::set<GraphicPropertyKey>;
using GraphicDirtyChildren = std::set<GraphicElementPtr>;

using GraphicPropertyMap = PropertyMap<GraphicPropertyKey, sGraphicPropertyBimap>;

/**
 * A single element of a graphic.  This may be a group of other elements, a path element,
 * or the overall container. This class is instantiated internally by the Graphic class.
 */
class GraphicElement : public UIDObject,
                       public std::enable_shared_from_this<GraphicElement>,
                       public RecalculateTarget<GraphicPropertyKey>,
                       public UserData<GraphicElement>,
                       public Counter<GraphicElement> {
    friend class Graphic;
    friend class GraphicBuilder;

public:
    /**
     * Default constructor.  Use build() instead.
     */
    GraphicElement(const GraphicPtr& graphic, const ContextPtr& context);

    ~GraphicElement() override = default;

    /**
     * @deprecated Use getUniqueId(), here only for migration period.
     * @return The unique id for this element.
     */
    APL_DEPRECATED id_type getId() const;

    /**
     * @return The number of children.
     */
    size_t getChildCount() const { return mChildren.size(); }

    /**
     * Retrieve a child at an index.  Throws an exception if out of bounds.
     * @param index The zero-based index of the child.
     * @return The child.
     */
    const GraphicElementPtr& getChildAt(size_t index) const {
        return mChildren.at(index);
    }

    /**
     * Retrieve a property assigned to the element.  Will throw an exception of the property doesn't exist.
     * @param key The property key.
     * @return The property value.
     */
    const Object& getValue(GraphicPropertyKey key) const { return mValues.get(key); }

    /**
     * Clear all properties marked as dirty.
     */
    void clearDirtyProperties();

    /**
     * Check to see if a single graphic property has been marked as dirty.
     * @param key The property to check.
     * @return True if this property is dirty.
     */
    bool isDirty(GraphicPropertyKey key) const { return mDirtyProperties.find(key) != mDirtyProperties.end(); }

    /**
     * Check to see if any of these graphic properties have been marked as dirty.
     * @param keys The vector of properties to check.
     * @return True if at least one of them is dirty
     */
    bool isDirty(std::vector<GraphicPropertyKey> keys) const {
        for (const auto& m : keys)
            if (mDirtyProperties.find(m) != mDirtyProperties.end())
                return true;
        return false;
    }

    /**
     * @return The set of properties which are marked as dirty for this element.
     */
    const std::set<GraphicPropertyKey>& getDirtyProperties() const { return mDirtyProperties; }

    /**
     * @return The type of this element.
     */
    virtual GraphicElementType getType() const = 0;

    /**
     * @return The language as a BCP-47 string (e.g., en-US)
     */
    virtual std::string getLang() const;

    /**
     * @return The layoutDirection of the AVG (either LTR or RTL)
     */
    virtual GraphicLayoutDirection getLayoutDirection() const;

    /**
     * Update any assigned style state.
     */
    void updateStyle(const GraphicPtr& graphic);

    /**
     * @return True if this component supports children
     */
    virtual bool hasChildren() const { return false; }

    /**
     * Do any VectorGraphic or context dependent clean-up.
     */
    virtual void release();

#ifdef SCENEGRAPH
    /**
     * Ensure that a scene graph has been constructed for this element.
     */
    virtual sg::GraphicFragmentPtr buildSceneGraph(bool allowLayers,
                                                   sg::SceneGraphUpdates& sceneGraph) = 0;

    /**
     * Store a reference to the layer that needs to be redrawn if a content
     * node in the element's scene graph needs to be redrawn
     */
    void assignSceneGraphLayer(const sg::LayerPtr& containingLayer);

    /**
     * Update the scene graph based on dirty properties.
     */
    virtual void updateSceneGraph(sg::SceneGraphUpdates& sceneGraph) = 0;
#endif // SCENEGRAPH

    virtual std::string toDebugString() const = 0;
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

    static Object asAvgFill(const Context& context, const Object& object);

    void setValue(GraphicPropertyKey key, const Object& value, bool useDirtyFlag) override;

protected:
    virtual bool initialize(const GraphicPtr& graphic, const Object& json);
    virtual const GraphicPropDefSet& propDefSet() const = 0;

    StyleInstancePtr getStyle(const GraphicPtr& graphic) const;
    void updateStyleInternal(const StyleInstancePtr& stylePtr, const GraphicPropDefSet& gds);

    void markAsDirty();
    void updateTransform(const Context& context, GraphicPropertyKey inKey, GraphicPropertyKey outKey, bool useDirtyFlag);

#ifdef SCENEGRAPH
    sg::GraphicFragmentPtr ensureSceneGraphChildren(bool allowLayers, sg::SceneGraphUpdates& sceneGraph);
    void requestRedraw(sg::SceneGraphUpdates& sceneGraph);
    void requestSizeCheck(sg::SceneGraphUpdates& sceneGraph);
    bool includeInSceneGraph(GraphicPropertyKey key);
#endif // SCENEGRAPH

    static void fixFillTransform(GraphicElement& element);
    static void fixStrokeTransform(GraphicElement& element);

protected:
    GraphicPropertyMap     mValues;           // Calculated values
    GraphicChildren        mChildren;         // Child elements
    GraphicDirtyProperties mDirtyProperties;  // Set of dirty properties
    Properties             mProperties;
    std::weak_ptr<Graphic> mGraphic;          // The top-level graphic we belong to
    std::string            mStyle;            // Current style name.
    std::set<GraphicPropertyKey> mAssigned;
    mutable id_type mCachedTempId = 0;

#ifdef SCENEGRAPH
    sg::LayerPtr  mContainingLayer;  // The layer that this element will render in
    sg::NodePtr   mSceneGraphNode;  // Rendering node used for update operations
#endif
};

} // namespace apl

#endif //_APL_GRAPHIC_ELEMENT_H
