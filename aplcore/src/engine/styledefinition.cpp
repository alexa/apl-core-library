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

#include <cstring>

#include "apl/engine/evaluate.h"
#include "apl/engine/styledefinition.h"
#include "apl/engine/arrayify.h"

#include "apl/utils/log.h"

namespace apl {

static const bool DEBUG_STYLES = false;
static const char *WHEN = "when";
static const char *VALUE = "value";
static const char *VALUES = "values";
static const char *DESCRIPTION = "description";

StyleDefinition::StyleDefinition(const rapidjson::Value& value, const Path& styleProvenance)
    : mStyleProvenance(styleProvenance),
      mBlockBaseProvenance(styleProvenance.addProperty(value, VALUE, VALUES))
{
    for (auto& m : arrayifyProperty(value, VALUE, VALUES))
        mBlocks.push_back(&m);
}

void
StyleDefinition::extendWithStyle(const StyleDefinitionPtr& extend)
{
    if (extend)
        mExtends.push_back(extend);
}

const StyleInstancePtr
StyleDefinition::get(const ContextPtr& context, const State& state)
{
    LOG_IF(DEBUG_STYLES) << "StyleDefinition::get " << state;
    auto it = mCache.find(state);
    if (it != mCache.end())
        return it->second;

    LOG_IF(DEBUG_STYLES) << "Constructing style";
    StyleInstancePtr ptr = std::make_shared<StyleInstance>(mStyleProvenance);

    // Build extensions in order
    for (const auto& sd : mExtends) {
        const StyleInstancePtr estyle = sd->get(context, state);
        for (auto& kv : *estyle)
            ptr->put(kv.first, kv.second, estyle->provenance(kv.first));
    }

    // Evaluate each block in order
    auto extendedContext = state.extend(context);
    size_t index = 0;
    for (const rapidjson::Value *block : mBlocks) {
        const Path path = mBlockBaseProvenance.addIndex(index++);
        if (!block->IsObject())
            continue;

        // Check the "when" clause
        auto when = block->FindMember(WHEN);
        if (when != block->MemberEnd() && !evaluate(*extendedContext, when->value).asBoolean())
            continue;

        // Iterate over all members (skipping "when" and "description")
        for (auto& m : block->GetObject()) {
            const char *name = m.name.GetString();
            if (std::strcmp(name, WHEN) != 0 && std::strcmp(name, DESCRIPTION) != 0)
                ptr->put(name, evaluate(*extendedContext, m.value), path.addObject(name).toString());
        }
    }

    mCache[state] = ptr;
    return ptr;
}

} // namespace apl