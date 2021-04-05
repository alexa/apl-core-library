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

#ifndef _APL_EXTENSION_FILTER_DEFINITION_H
#define _APL_EXTENSION_FILTER_DEFINITION_H

#include "apl/primitives/object.h"
#include "apl/engine/binding.h"

namespace apl {

/**
 * Declare a custom filter for use in Image components.  The name of the filter should
 * be unique and not overlap with any existing filters.  A sample registration is:
 *
 *   rootConfig.registerExtensionFilter(
 *       ExtensionFilterDefinition("MyURI", "CannyEdgeDetector", ExtensionFilterDefinition::ONE)
 *                                   .property("min", 0.1, kBindingTypeNumber)
 *                                   .property("max", 0.9, kBindingTypeNumber);
 *
 * This filter may now be used in an Image component filter list:
 *
 * {
 *   "type": "Image",
 *   "filters": [
 *     {
 *       "type": "MyURI:CannyEdgeDetector",
 *       "min": 0.2,
 *       "max": 0.8,
 *       "source": 2
 *     }
 *   ]
 *
 * The filter will satisfy
 *
 *   filter.getType()                                     == kFilterTypeExtension
 *   filter.getValue(kFilterPropertyExtensionURI)         == "MyURI"
 *   filter.getValue(kFilterPropertyName)                 == "CannyEdgeDetector"
 *   filter.getValue(kFilterPropertySource)               == 2
 *   filter.getValue(kFilterPropertyExtension).get("min") == 0.2
 *   filter.getValue(kFilterPropertyExtension).get("max") == 0.8
 *
 * A custom filter will have the following properties:
 *
 *   kFilterPropertyExtension       Map of string -> Object (includes properties like source, destination)
 *   kFilterPropertyExtensionURI    URI of the extension
 *   kFilterPropertyName            Name of the extension command
 *   kFilterPropertySource          If ImageCount == ONE || TWO
 *   kFilterPropertyDestination     If ImageCount == TWO
 */
class ExtensionFilterDefinition {
public:
    /**
     * Defines the number of images that will be referenced by this filter.  The first
     * reference name is "source" and will be stored at kFilterPropertySource.  The second
     * reference name is "destination" and will be stored at kFilterPropertyDestination.
     */
    enum ImageCount { ZERO, ONE, TWO };

    /**
     * Defines the binding type and default value for an extension property.
     */
    struct Property {
        BindingType bindingType;
        Object defaultValue;
    };

    /**
     * Standard constructor
     * @param uri The URI of the extension
     * @param name The name of the filter
     * @param imageCount The number of images referenced by this filter
     */
    ExtensionFilterDefinition(std::string uri, std::string name, ImageCount imageCount)
        : mURI(std::move(uri)),
          mName(std::move(name)),
          mImageCount(imageCount) {}

    /**
     * Add a named property. The property names "when", "type", "source", and "destination" are reserved.
     * @param property The property name
     * @param defvalue The default value to use for this property when it is not provided.
     * @param bindingType Binding type.
     * @return This object for chaining.
     */
    ExtensionFilterDefinition& property(const std::string& name,
                                        const Object& defvalue,
                                        BindingType bindingType=kBindingTypeAny) {
        return property(name, Property{bindingType, defvalue});
    }

    /**
     * Add a named property. The property names "when", "type", "source", and "destination" are reserved.
     * @param name The property name.
     * @param prop Extension property definition.
     * @return This object for chaining.
     */
    ExtensionFilterDefinition& property(const std::string& name, Property&& prop) {
        if (name == "when" || name == "type" || name == "source" || name == "destination")
            LOG(LogLevel::kWarn) << "Unable to register property '" << name << "' in custom filter extension " << mName;
        else
            mPropertyMap.emplace(name, std::move(prop));
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
     * @return The number of images referenced by this filter (0, 1, or 2);
     */
    ImageCount getImageCount() const { return mImageCount; }

    /**
     * @return The map of all defined properties in this custom filter.
     */
    const std::map<std::string, Property>& getPropertyMap() const { return mPropertyMap; }

    /**
     * @return string for debugging.
     */
    std::string toDebugString() const {
        return "ExtensionFilterDefinition< uri:" + mURI + ",name:" + mName + ">";
    }

private:
    std::string mURI;
    std::string mName;
    ImageCount mImageCount;
    std::map<std::string, Property> mPropertyMap;
};

} // namespace apl

#endif // _APL_EXTENSION_FILTER_DEFINITION_H
