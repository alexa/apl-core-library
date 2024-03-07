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

#ifndef _APL_DOCUMENT_CONTEXT_H
#define _APL_DOCUMENT_CONTEXT_H

#include "apl/common.h"
#include "apl/primitives/object.h"
#include "apl/utils/counter.h"
#include "apl/utils/noncopyable.h"

namespace apl {

/**
 * Representation of the rendered document interface.
 */
class DocumentContext : public NonCopyable, public Counter<DocumentContext> {
public:
    virtual ~DocumentContext() = default;

    /**
     * Identifies when the visual context may have changed.  A call to serializeVisualContext resets this value to false.
     * @return true if the visual context has changed since the last call to serializeVisualContext, false otherwise.
     */
    virtual bool isVisualContextDirty() const = 0;

    /**
     * Clear the visual context dirty flag
     */
    virtual void clearVisualContextDirty() = 0;

    /**
     * Retrieve component's visual context as a JSON object. This method also clears the
     * visual context dirty flag
     * @param allocator Rapidjson allocator
     * @return The serialized visual context
     */
    virtual rapidjson::Value serializeVisualContext(rapidjson::Document::AllocatorType& allocator) = 0;

    /**
     * Identifies when the datasource context may have changed.  A call to serializeDatasourceContext resets this value to false.
     * @return true if the datasource context has changed since the last call to serializeDatasourceContext, false otherwise.
     */
    virtual bool isDataSourceContextDirty() const = 0;

    /**
     * Clear the datasource context dirty flag
     */
    virtual void clearDataSourceContextDirty() = 0;

    /**
     * Retrieve the document's content.
     */
    virtual const ContentPtr& content() const = 0;

    /**
     * Retrieve datasource context as a JSON array object. This method also clears the
     * datasource context dirty flag
     * @param allocator Rapidjson allocator
     * @return The serialized datasource context
     */
    virtual rapidjson::Value serializeDataSourceContext(rapidjson::Document::AllocatorType& allocator) = 0;

    /**
     * Serialize a complete version of the DOM
     * @param extended If true, serialize everything.  If false, just serialize external data
     * @param allocator Rapidjson allocator
     * @return The serialized DOM
     */
    virtual rapidjson::Value serializeDOM(bool extended, rapidjson::Document::AllocatorType& allocator) = 0;

    /**
     * Serialize the global values for developer tools
     * @param allocator Rapidjson allocator
     * @return The serialized global values
     */
    virtual rapidjson::Value serializeContext(rapidjson::Document::AllocatorType& allocator) = 0;

    /**
     * Execute an externally-driven command
     * @param commands The commands to execute
     * @param fastMode If true this handler will be invoked in fast mode
     */
    virtual ActionPtr executeCommands(const Object& commands, bool fastMode) = 0;
};

} // namespace apl

#endif //_APL_DOCUMENT_CONTEXT_H
