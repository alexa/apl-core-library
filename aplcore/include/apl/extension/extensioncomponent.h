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

#ifndef _APL_EXTENSION_COMPONENT_H
#define _APL_EXTENSION_COMPONENT_H

#include "apl/component/touchablecomponent.h"
#include "apl/content/extensioncomponentdefinition.h"
#include "apl/component/componentpropdef.h"

namespace apl {

/**
 * ExtensionComponents are special components for which the rendering surface is drawn to by an
 * extension and composited with the APL layout. This class handles the custom properties for each
 * extension component as well as the resource state.
 */
class ExtensionComponent : public CoreComponent {
public:
    static CoreComponentPtr create(const ExtensionComponentDefinition& definition,
                                   const ContextPtr& context,
                                   Properties&& properties,
                                   const Path& path);

    ExtensionComponent(const ExtensionComponentDefinition& definition,
                       const ContextPtr& context,
                       Properties&& properties,
                       const Path& path);

    ComponentType getType() const override { return kComponentTypeExtension; };

    /**
     * Retrieves the name of the extension component as defined by the extension
     * @return name
     */
    std::string name() const override;

    void release() override;

    void initialize() override;

    const ComponentPropDefSet& propDefSet() const override { return mPropDefSet; };

    /*
     * Retrieves the URI associated with extension component.
     */
    std::string getUri() const { return mDefinition.getURI(); }

    /**
     * Finds an appropriate custom handler to invoke.  Returns NULL if no such handler exists
     * @param handler The extension event handler to invoke
     * @return The Object attached to this handler or NULL.
     */
    Object findHandler(const ExtensionEventHandler& handler);

    /**
     * Called when extension has experienced a component failure.  Sets the state of the component
     * to @c kResourceError and notifies the document error handler if present.
     * @param errorCode Error code
     * @param message Message associated with error code.
     */
    void extensionComponentFail(int errorCode, const std::string& message);

    /**
     * @return The unique identifier of the resource associated with ExtensionComponent
     */
    std::string getResourceID() const { return mResourceID; }

    /**
     * Updates state of resource associated with extension component. The extension
     * is notified of the change.
     * @param state resource state enum.
     */
    void updateResourceState(const ExtensionComponentResourceState& state) override;

protected:
    /**
     * Function which executes the handler associated with FatalError
     * @param errorCode Error code
     * @param message Message associated with error code.
     */
    void onFatalError(int errorCode, const std::string& message);

    /**
     * Override behavior to send update to extension.
     */
    void handlePropertyChange(const ComponentPropDef& def, const Object& value) override;


    const EventPropertyMap& eventPropertyMap() const override;

    std::string getVisualContextType() const override;

private:
    /**
     * Notify the extension the component has changed state or property.
     */
    void notifyExtension();

    const ExtensionComponentDefinition& mDefinition;
    ComponentPropDefSet mPropDefSet;
    std::string mResourceID;

};

} // namespace apl

#endif //_APL_EXTENSION_COMPONENT_H
