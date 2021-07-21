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

#ifndef _APL_STYLE_DEFINITION_H
#define _APL_STYLE_DEFINITION_H

#include "apl/common.h"
#include "apl/engine/styleinstance.h"
#include "apl/engine/state.h"

#include "apl/utils/path.h"

namespace apl {

/**
 * The JSON data and definitions of a single style.  A single style is constructed from a number
 * of parents and a number of conditionally-selected JSON blocks.  We stash that information in
 * the StyleDefinition and do a lazy-evaluation construction of the properties for this style
 * based on the particular state settings.
 *
 * This class is internal to Styles.
 */
class StyleDefinition {
public:
    /**
     * Create a StyleDefinition.
     * @param value The JSON object that defines the style.
     * @param styleProvenance The JSON path to the style definition
     */
    StyleDefinition(const rapidjson::Value& value, const Path& styleProvenance);

    /**
     * This style extends another style.  Add that style to the end of the list of styles this
     * style extends.
     * @param extend The style that this style extends.
     */
    void extendWithStyle(const StyleDefinitionPtr& extend);

    /**
     * Given a component state and data-binding context, return a StyleInstance.
     * @param context The data-binding context.
     * @param state The component state.
     * @return The StyleInstance.
     */
    const StyleInstancePtr get(const ContextPtr& context, const State& state);

    /**
     * @return The provenance path of the style
     */
    const Path provenance() const { return mStyleProvenance; }

private:
    const Path mStyleProvenance;
    const Path mBlockBaseProvenance;
    std::vector<StyleDefinitionPtr > mExtends;   // Named styles we extend
    std::vector<const rapidjson::Value *> mBlocks;   // Ordered list of blocks to evaluate
    std::map<State, StyleInstancePtr> mCache;          // State cache of results
};

} // namespace apl

#endif //_APL_STYLE_DEFINITION_H
