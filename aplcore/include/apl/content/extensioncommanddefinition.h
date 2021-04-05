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

#ifndef _APL_EXTENSION_COMMAND_DEFINITION_H
#define _APL_EXTENSION_COMMAND_DEFINITION_H

#include "apl/content/extensionproperty.h"

namespace apl {

/**
 * Define a custom document-level command.  The name of the command should be
 * unique and not overlap with any macros or existing commands. A sample registration
 * is:
 *
 *     rootConfig.registerExtensionCommand(
 *         ExtensionCommandDefinition("MyURI", "ChangeVolume").allowFastMode(true)
 *                                                            .property("volume", 3, false)
 *                                                            .property("channel", "all", false)
 *     );
 *
 * This command may now be used from within APL:
 *
 *     "onPress": {
 *       "type": "MyURI:ChangeVolume",
 *       "volume": 7
 *     }
 *
 * When this command fires, it will be returned as an Event to the RootContext.  The custom
 * command will have the following values: kEventPropertyExtension, kEventPropertyName, kEventPropertySource,
 * and kEventPropertyCustom.
 *
 * For example, the above "ChangeVolume" custom command will satisfy:
 *
 *     event.getType() == kEventTypeCustom
 *     event.getValue(kEventPropertyName) == Object("ChangeVolume")
 *     event.getValue(kEventPropertyExtensionURI) == Object("MyURI")
 *     event.getValue(kEventPropertySource).get("type") == Object("TouchWrapper")
 *     event.getValue(kEventPropertyCustom).get("volume") == Object(7)
 *     event.getValue(kEventPropertyCustom).get("channel") == Object("all")
 *
 * kEventPropertyExtensionURI is the URI of the extension
 * kEventPropertyName is the name of the extension assigned by the APL document
 * kEventPropertySource is a map of the source object that generated the event.
 * See the SendEvent command for a description of the source fields.
 * kEventPropertyCustom value is a map of the user-specified properties listed at registration time.
 */
class ExtensionCommandDefinition {
public:
    /**
     * Standard constructor
     * @param uri The URI of the extension
     * @param name The name of the command.
     */
    ExtensionCommandDefinition(std::string uri, std::string name) : mURI(std::move(uri)), mName(std::move(name)) {}

    /**
     * Configure if this command can run in fast mode.  When the command runs in fast mode the
     * "requireResolution" property is ignored (fast mode commands do not support action resolution).
     * @param allowFastMode If true, this command can run in fast mode.
     * @return This object for chaining
     */
    ExtensionCommandDefinition& allowFastMode(bool allowFastMode) {
        mAllowFastMode = allowFastMode;
        return *this;
    }

    /**
     * Configure if this command (in normal mode) will return an action pointer
     * that must be resolved by the view host before the next command in the sequence is executed.
     * @param requireResolution If true, this command will provide an action pointer (in normal mode).
     * @return This object for chaining.
     */
    ExtensionCommandDefinition& requireResolution(bool requireResolution) {
        mRequireResolution = requireResolution;
        return *this;
    }

    /**
     * Add a named property. The property names "when" and "type" are reserved.
     * @param property The property to add
     * @param defvalue The default value to use for this property when it is not provided.
     * @param required If true and the property is not provided, the command will not execute.
     * @return This object for chaining.
     */
    ExtensionCommandDefinition& property(const std::string& name, const Object& defvalue, bool required) {
        property(name, kBindingTypeAny, defvalue, required);
        return *this;
    }

    /**
     * Add a named property. The property names "when" and "type" are reserved.
     * @param name Property name.
     * @param btype Binding type.
     * @param defvalue The default value to use for this property when it is not provided.
     * @param required If true and the property is not provided, the command will not execute.
     * @return This object for chaining.
     */
    ExtensionCommandDefinition& property(const std::string& name, BindingType btype, const Object& defvalue, bool required) {
        property(name, ExtensionProperty{btype, defvalue, required});
        return *this;
    }

    /**
     * Add a named property. The property names "when" and "type" are reserved.
     * @param name Property name.
     * @param prop Extension property definition.
     * @return This object for chaining.
     */
    ExtensionCommandDefinition& property(const std::string& name, ExtensionProperty&& prop) {
        if (name == "when" || name == "type")
            LOG(LogLevel::kWarn) << "Unable to register property '" << name << "' in custom command " << mName;
        else
            mPropertyMap.emplace(name, std::move(prop));
        return *this;
    }

    /**
     * Add a named array-ified property. The property will be converted into an array of values. The names "when"
     * and "type" are reserved.
     * @param property The property to add
     * @param defvalue The default value to use for this property when it is not provided.
     * @param required If true and the property is not provided, the command will not execute.
     * @return This object for chaining.
     */
    ExtensionCommandDefinition& arrayProperty(std::string property, bool required) {
        if (property == "when" || property == "type")
            LOG(LogLevel::kWarn) << "Unable to register array-ified property '" << property << "' in custom command " << mName;
        else
            mPropertyMap.emplace(property, ExtensionProperty{kBindingTypeArray, Object::EMPTY_ARRAY(), required});
        return *this;
    }

    /**
     * @return The URI of the extension
     */
    std::string getURI() const { return mURI; }

    /**
     * @return The name of the command
     */
    std::string getName() const { return mName; }

    /**
     * @return True if this command can execute in fast mode
     */
    bool getAllowFastMode() const { return mAllowFastMode; }

    /**
     * @return True if this command will return an action pointer that must be
     *         resolved.  Please note that a command running in fast mode will
     *         never wait to be resolved.
     */
    bool getRequireResolution() const { return mRequireResolution; }

    /**
     * @return The map of all defined properties in this custom command.
     */
    const std::map<std::string, ExtensionProperty>& getPropertyMap() const { return mPropertyMap; }

    /**
     * @return string for debugging.
     */
    std::string toDebugString() const {
        return "ExtensionCommandDefinition< uri:" + mURI + ",name:" + mName + ">";
    };

private:
    std::string mURI;
    std::string mName;
    std::map<std::string, ExtensionProperty> mPropertyMap;
    bool mAllowFastMode = false;
    bool mRequireResolution = false;
};
} // namespace apl

#endif // _APL_EXTENSION_COMMAND_DEFINITION_H
