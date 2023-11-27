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

#ifndef _APL_DOCUMENT_CONFIG_H
#define _APL_DOCUMENT_CONFIG_H

#include "apl/common.h"

#include "apl/primitives/object.h"

#ifdef ALEXAEXTENSIONS
#include <alexaext/alexaext.h>
#endif

namespace apl {

/**
 * Configuration at the document level, applicable to embedded as well as primary documents.
 */
class DocumentConfig {
public:
    /**
     * @return DocumentConfig instance.
     */
    static DocumentConfigPtr create() { return std::make_shared<DocumentConfig>(); }

    /**
     * Default constructor. Use create() instead.
     */
    DocumentConfig() {}

#ifdef ALEXAEXTENSIONS
    /**
     * Assign a Alexa Extension mediator.
     *
     * @param extensionMediator Message mediator manages messages between Extension and APL engine.
     *        and the APL engine.
     * @return This object for chaining
     */
    DocumentConfig& extensionMediator(const ExtensionMediatorPtr &extensionMediator) {
        mExtensionMediator = extensionMediator;
        return *this;
    }

    /**
     * @return The extension mediator.
     */
    const ExtensionMediatorPtr& getExtensionMediator() const {
        return mExtensionMediator;
    }
#endif

    /**
     * Add DataSource provider implementation.
     * @param dataSourceProvider provider implementation.
     * @return This object for chaining.
     */
    DocumentConfig& dataSourceProvider(const DataSourceProviderPtr& dataSourceProvider) {
        mDataSources.emplace(dataSourceProvider);
        return *this;
    }

    /**
     * @return Set of DataSource providers.
     */
    const std::set<DataSourceProviderPtr>& getDataSourceProviders() const { return mDataSources; }

    /**
     * Set Environment value for the document.
     * @param name Environment key.
     * @param value Environment value.
     * @return This object for chaining.
     */
    DocumentConfig& setEnvironmentValue(const std::string& name, const Object& value) {
        mEnvironmentValues[name] = value;
        return *this;
    }

    /**
     * @return Environment values.
     */
    const ObjectMap& getEnvironmentValues() const {
        return mEnvironmentValues;
    }

private:
#ifdef ALEXAEXTENSIONS
    ExtensionMediatorPtr mExtensionMediator;
#endif
    std::set<DataSourceProviderPtr> mDataSources;
    ObjectMap mEnvironmentValues;
};

}

#endif //_APL_DOCUMENT_CONFIG_H
