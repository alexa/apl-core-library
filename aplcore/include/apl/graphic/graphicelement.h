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

#include "apl/common.h"
#include "apl/utils/counter.h"
#include "apl/utils/noncopyable.h"
#include "apl/utils/userdata.h"
#include "apl/primitives/object.h"
#include "apl/engine/properties.h"
#include "apl/engine/propertymap.h"
#include "apl/engine/recalculatetarget.h"

#include "apl/graphic/graphicproperties.h"

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
class GraphicElement : public std::enable_shared_from_this<GraphicElement>,
                       public RecalculateTarget<GraphicPropertyKey>,
                       public UserData<GraphicElement>,
                       public NonCopyable,
                       public Counter<GraphicElement> {
    friend class Graphic;
    friend class GraphicDependant;
    friend class GraphicBuilder;

    static id_type sUniqueGraphicIdGenerator;

public:
    /**
     * Default constructor.  Use build() instead.
     */
    GraphicElement(const GraphicPtr& graphic, const ContextPtr& context);

    ~GraphicElement() override = default;

    /**
     * @return The unique id for this element.
     */
    id_type getId() const { return mId; }

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
     * @return The set of properties which are marked as dirty for this element.
     */
    const std::set<GraphicPropertyKey>& getDirtyProperties() const { return mDirtyProperties; }

    /**
     * @return The type of this element.
     */
    virtual GraphicElementType getType() const = 0;

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
    void release();

    virtual std::string toDebugString() const = 0;
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

    static Object asAvgFill(const Context& context, const Object& object);

protected:
    virtual bool initialize(const GraphicPtr& graphic, const Object& json);
    virtual const GraphicPropDefSet& propDefSet() const = 0;
    bool setValue(GraphicPropertyKey key, const Object& value, bool useDirtyFlag);

    StyleInstancePtr getStyle(const GraphicPtr& graphic) const;
    void updateStyleInternal(const StyleInstancePtr& stylePtr, const GraphicPropDefSet& gds);

    void markAsDirty();

    void updateTransform(const Context& context, GraphicPropertyKey inKey, GraphicPropertyKey outKey, bool useDirtyFlag);

    static void fixFillTransform(GraphicElement& element);
    static void fixStrokeTransform(GraphicElement& element);

protected:
    id_type                mId;               // Unique ID assigned to this vector graphic element
    GraphicPropertyMap     mValues;           // Calculated values
    GraphicChildren        mChildren;         // Child elements
    GraphicDirtyProperties mDirtyProperties;  // Set of dirty properties
    Properties             mProperties;
    std::weak_ptr<Graphic> mGraphic;          // The top-level graphic we belong to
    std::string            mStyle;            // Current style name.
    std::set<GraphicPropertyKey> mAssigned;
    ContextPtr             mContext;
};

} // namespace apl

#endif //_APL_GRAPHIC_ELEMENT_H
