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

#ifndef _APL_STYLES_H
#define _APL_STYLES_H

#include <string>
#include <vector>
#include <set>
#include <map>

#include "rapidjson/document.h"

#include "apl/primitives/object.h"

#include "apl/utils/counter.h"

#include "state.h"
#include "styledefinition.h"

namespace apl {

class Context;
class StyleProcessSet;

/**
 * Store all of the styles defined in an APL document and the loaded packages.
 */
class Styles : public Counter<Styles> {
public:
    Styles() : Styles(nullptr) {}
    Styles(const std::shared_ptr<Styles>& parent) : mParentStyle(parent) {}

    /**
     * Return a style by name.
     * @param context The top-level data-binding context.  This context will be used to evaluate the style if
     *                it has not already been evaluated.
     * @param name The name of the style.
     * @param state The state in which to evaluate the style.
     * @return The style or nullptr if it is not defined.
     */
    const StyleInstancePtr get(const ContextPtr& context, const std::string& name, const State& state);

    /**
     * Retrieve a style definition by name.
     * @param name The name of the style definition.
     * @return The style definition or nullptr if it does not exist
     */
    StyleDefinitionPtr getStyleDefinition(const std::string& name);

    /**
     * Add a collection of style definitions to the master table of styles.  Each property in the JSON object
     * passed in corresponds to a named style and will be mapped to a StyleDefinition.  The provenance path
     * passed in is the provenance path of the JSON object and is passed on to the style definitions so that
     * each created style can be associated with where it was defined in the JSON content.
     * @param session The logging session
     * @param json The JSON object containing one or more styles.
     * @param provenance The JSON path to the JSON object.
     */
    void addStyleDefinitions(const SessionPtr& session, const rapidjson::Value *json, const Path& provenance);

    /**
     * @return The number of styles
     */
    size_t size() const { return mStyleDefinitions.size(); }

    /**
     * @return The entire style definition map.
     */
    const std::map<std::string, StyleDefinitionPtr>& styleDefinitions() const {
        return mStyleDefinitions;
    }

    friend StyleProcessSet;

private:
    /**
     * Store a style definition
     * @param name The name of the style definition
     * @param styleDefinitionPtr The style definition to store.
     */
    void setStyleDefinition(const std::string& name, const StyleDefinitionPtr& styleDefinitionPtr) {
        mStyleDefinitions[name] = styleDefinitionPtr;
    }

private:
    std::map<std::string, StyleDefinitionPtr> mStyleDefinitions;
    std::shared_ptr<Styles>                   mParentStyle;
};

}  // namespace apl

#endif // _APL_STYLES_H
