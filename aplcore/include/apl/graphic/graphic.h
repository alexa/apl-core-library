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

#ifndef _APL_GRAPHIC_H
#define _APL_GRAPHIC_H

#include "apl/graphic/graphiccontent.h"
#include "apl/graphic/graphicelement.h"
#include "apl/engine/jsonresource.h"
#include "apl/engine/styles.h"
#include "apl/utils/noncopyable.h"
#include "apl/utils/userdata.h"

namespace apl {

class VectorGraphicComponent;

/**
 * Represent a vector graphic element.  This outer element holds a tree of GraphicElement
 * internally.
 */
class Graphic : public std::enable_shared_from_this<Graphic>,
                public NonCopyable,
                public Counter<Graphic>,
                public UserData<Graphic> {
    friend class GraphicElement;
    friend class GraphicDependant;
    friend class VectorGraphicComponent;

public:
    /**
     * Construct a Graphic.
     * @param context The data-binding context.  Typically this only contains resources.
     * @param jsonResource The json resource defining this graphic.
     * @param properties Assigned properties to this graphic.
     * @param component The component this Graphic is assigned to.
     * @return A shared pointer to the graphic or nullptr if it was invalid.
     */
    static GraphicPtr create(const ContextPtr& context,
                             const JsonResource& jsonResource,
                             Properties&& properties,
                             const std::shared_ptr<CoreComponent>& component);

    /**
     * Construct a graphic from raw JSON data
     * @param context The data-binding context.  Typically this only contains resources.
     * @param json The json content defining this graphic.
     * @param properties Assigned properties to this graphic.
     * @param styledPtr Assigned style.
     * @return A shared pointer to the graphic or nullptr if it was invalid.
     */
    static GraphicPtr create(const ContextPtr& context,
                             const rapidjson::Value& json,
                             Properties&& properties,
                             const std::shared_ptr<CoreComponent>& component,
                             const Path& path,
                             const StyleInstancePtr& styledPtr = nullptr);
    /**
     * Internal constructor.  Use Graphic:create instead
     */
    Graphic(const ContextPtr& context, const rapidjson::Value& json, GraphicVersions version);

    /**
     * Override standard destructor to clear out
     */
    ~Graphic() override;

    /**
     * @return True if this graphic was successfully inflated and has content.
     */
    bool isValid() const { return mRootElement != nullptr; }

    /**
     * @return The top-level graphic element. This should be a container.
     */
    const GraphicElementPtr& getRoot() const { return mRootElement; }

    /**
     * @return The intrinsic (standard) height of the graphic as defined within the graphic.
     *         This is measured in DP.
     */
    double getIntrinsicHeight() const;

    /**
     * @return The intrinsic (standard) width of the graphic as defined within the graphic.
     *         This is measured in DP.
     */
    double getIntrinsicWidth() const;

    /**
     * @return The width of the viewport drawing area.
     */
    double getViewportWidth() const;

    /**
     * @return The height of the viewport drawing area
     */
    double getViewportHeight() const;

    /**
     * @return The version of the current graphic as an enumerated value
     */
    GraphicVersions getVersion() const { return mVersion; }

    /**
     * Inform the graphic of the actual assigned width and height (in DP).  This may cause internal
     * changes in the layout of the graphic.
     * @param width The assigned width in DP
     * @param height The assigned height in DP
     * @param useDirtyFlag If true any changed properties due to the layout will have the dirty flag set.
     * @return True if the graphic changed
     */
    bool layout(double width, double height, bool useDirtyFlag);

    /**
     * Update the style assigned to the graphic. This will cause dirty flags to be set.
     * @param styledPtr The new style.
     * @return True if something changed.
     */
    bool updateStyle(const StyleInstancePtr& styledPtr);

    /**
     * Clear the list of dirty children. This resets the dirty elements in each child.
     */
    void clearDirty();

    /**
     * @return The set of graphic elements that have dirty property flags.
     */
    const GraphicDirtyChildren& getDirty() { return mDirty; }

    /**
     * Set the value of one of the parameters
     * @param key The name of the parameter
     * @param value The value to set it to.
     * @return True if this was a valid parameter.
     */
    bool setProperty(const std::string& key, const Object& value);

    /**
     * @return The internal data-binding context used by this graphic
     */
    const ContextPtr& getContext() const { return mInternalContext; }

    /**
     * @return Styles access interface.
     */
    std::shared_ptr<Styles> styles() const { return mStyles; }

    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

private:
    /**
     * Assign this graphic to a VectorGraphicComponent
     * @param component The component
     */
    void setComponent(const std::shared_ptr<CoreComponent>& component) {
        mComponent = component;
    }

    /**
     * Release any held resources.
     */
    void release();

    void initialize(const ContextPtr &sourceContext,
                    const rapidjson::Value &json,
                    Properties &&properties,
                    const std::shared_ptr<CoreComponent> &component,
                    const Path& path,
                    const StyleInstancePtr &styledPtr);
    void addDirtyChild(const GraphicElementPtr& child);

private:
    ContextPtr                   mInternalContext;
    ParameterArray               mParameterArray;
    GraphicVersions              mVersion;

    GraphicElementPtr            mRootElement;
    GraphicDirtyChildren         mDirty;
    std::set<std::string>        mAssigned;        // Track which parameters have been assigned.  The remainder are styled.

    std::weak_ptr<CoreComponent> mComponent;
    std::shared_ptr<Styles>      mStyles;
};

} // namespace apl

#endif //_APL_GRAPHIC_H
