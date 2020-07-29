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

#include <set>
#include <algorithm>

#include "apl/engine/styles.h"

#include "apl/engine/evaluate.h"
#include "apl/engine/context.h"
#include "apl/engine/state.h"
#include "apl/primitives/object.h"
#include "apl/engine/arrayify.h"
#include "apl/utils/log.h"
#include "apl/utils/session.h"

namespace apl {

static const bool DEBUG_STYLES = false;

/**
 * Encapsulation class so we can hold the state of adding a set of style definitions into the Styles object
 */
class StyleProcessSet {
public:
    StyleProcessSet(const SessionPtr& session, Styles& styles, const rapidjson::Value *json, const Path& path)
        : mSession(session), mStyles(styles), mJson(json), mPath(path)
    {}

    void process() {
        // Copy all of the named styles in this JSON into the to-be-processed list
        for (auto& m : mJson->GetObject())
            mToBeProcessed.insert(m.name.GetString());

        // Process each style in the list.  Adding a style may pull other styles from the list due to dependencies
        while (!mToBeProcessed.empty()) {
            std::string styleName = *mToBeProcessed.begin();
            addOne(styleName);
        }

        // Ensure no leftover styles
        assert(mInProcess.empty());
    }

    StyleDefinitionPtr addOne(const std::string& name) {
        LOG_IF(DEBUG_STYLES) << "Styles::addOne " << name;

        if (mInProcess.find(name) != mInProcess.end()) {
            CONSOLE_S(mSession) << "Loop in style specification with " << name;
            return nullptr;
        }

        auto it = mToBeProcessed.find(name);
        if (it == mToBeProcessed.end())
            return mStyles.getStyleDefinition(name);

        mToBeProcessed.erase(it);
        mInProcess.insert(name);

        auto sd = buildStyle(name);
        mStyles.setStyleDefinition(name, sd);

        // Remove from the in-process list
        mInProcess.erase(mInProcess.find(name));
        return sd;
    }

    StyleDefinitionPtr buildStyle(const std::string& name) {
        LOG_IF(DEBUG_STYLES) << name;

        const rapidjson::Value& value = mJson->GetObject()[name.c_str()];
        StyleDefinitionPtr styledef = std::make_shared<StyleDefinition>(value, mPath.addObject(name));

        LOG_IF(DEBUG_STYLES) << "  extend, extends";
        for (auto&& m : arrayifyProperty(value, "extend", "extends")) {
            if (!m.IsString())
                continue;

            styledef->extendWithStyle(addOne(m.GetString()));
        }

        return styledef;
    }

private:
    SessionPtr mSession;
    Styles& mStyles;
    const rapidjson::Value* mJson;
    const Path mPath;
    std::set<std::string> mToBeProcessed;
    std::set<std::string> mInProcess;
};

/****************************************************************/

const StyleInstancePtr
Styles::get(const ContextPtr& context, const std::string& name, const State& state)
{
    LOG_IF(DEBUG_STYLES) << "Styles::get " << name << " " << state;

    auto definition = getStyleDefinition(name);
    if (definition)
        return definition->get(context, state);

    LOG_IF(DEBUG_STYLES) << "Didn't find anything";
    return nullptr;
}

void
Styles::addStyleDefinitions(const SessionPtr& session, const rapidjson::Value *json, const Path& provenance)
{
    StyleProcessSet sps(session, *this, json, provenance);
    sps.process();
}

StyleDefinitionPtr
Styles::getStyleDefinition(const std::string& name) {
    auto it = mStyleDefinitions.find(name);
    if (it != mStyleDefinitions.end())
        return it->second;

    if (mParentStyle) {
        return mParentStyle->getStyleDefinition(name);
    }
    return nullptr;
}


} // namespace apl
