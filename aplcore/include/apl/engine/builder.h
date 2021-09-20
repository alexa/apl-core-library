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

#ifndef _APL_BUILDER_H
#define _APL_BUILDER_H

#include "apl/component/corecomponent.h"

namespace apl {

class Path;

using MakeComponentFunc = std::function<CoreComponentPtr(const ContextPtr&, Properties&&, const Path&)>;

/**
 * Static methods for inflating component view hierarchies.  These methods are used when constructing a
 * RootContext or when calling Component::inflate().  Do not call them directly.
 */
class Builder {
    friend LayoutRebuilder;

public:
    /**
     * Construct a builder object.
     * @param old If defined, this references the old component hierarchy that is being re-inflated.
     */
    Builder(CoreComponentPtr old = nullptr) : mOld(std::move(old)) {}

    /**
     * Inflate the mainTemplate out of an APL document
     * @param context Top-level data-binding context.
     * @param mainProperties Raw properties that should be passed to the inflation routine.  These come from any
     *                       initial data-binding applied to the document.
     * @param mainDocument The master APL document
     * @return The inflated component hierarchy, or nullptr if the document is malformed.
     */
    CoreComponentPtr inflate(const ContextPtr& context,
                             Properties& mainProperties,
                             const rapidjson::Value& mainDocument);

    /**
     * Inflate a component or component hierarchy from an Object.
     * @param context The top-level data-binding context to use in inflation.
     * @param component The Object containing a component or the component hierarchy. If array - first that fulfils
     *        "while" requirements is selected.
     * @return The inflated component hierarchy, or nullptr if component is malformed.
     */
    CoreComponentPtr inflate(const ContextPtr& context,
                             const Object& component);

private:
    void populateSingleChildLayout(const ContextPtr& context,
                                   const Object& item,
                                   const CoreComponentPtr& layout,
                                   const Path& path,
                                   bool fullBuild,
                                   bool useDirtyFlag);

    void populateLayoutComponent(const ContextPtr& context,
                                 const Object& item,
                                 const CoreComponentPtr& layout,
                                 const Path& path,
                                 bool fullBuild,
                                 bool useDirtyFlag);

    CoreComponentPtr expandLayout(const ContextPtr& context,
                                  Properties& properties,
                                  const rapidjson::Value& layout,
                                  const CoreComponentPtr& parent,
                                  const Path& path,
                                  bool fullBuild,
                                  bool useDirtyFlag);

    void copyPreservedBindings(const CoreComponentPtr& newComponent, const CoreComponentPtr& originalComponent);

    void copyPreservedProperties(const CoreComponentPtr& newComponent, const CoreComponentPtr& originalComponent);

    CoreComponentPtr expandSingleComponentFromArray(const ContextPtr& context,
                                                    const std::vector<Object>& items,
                                                    Properties&& properties,
                                                    const CoreComponentPtr& parent,
                                                    const Path& path,
                                                    bool fullBuild,
                                                    bool useDirtyFlag);

    CoreComponentPtr expandSingleComponent(const ContextPtr& context,
                                           const Object& item,
                                           Properties&& properties,
                                           const CoreComponentPtr& parent,
                                           const Path& path,
                                           bool fullBuild,
                                           bool useDirtyFlag);

    static void attachBindings(const ContextPtr& context, const Object& item);
private:

    MakeComponentFunc findComponentBuilderFunc(const ContextPtr& context, const std::string &type);
    CoreComponentPtr mOld;
};

} // namespace apl

#endif // _APL_BUILDER_H
