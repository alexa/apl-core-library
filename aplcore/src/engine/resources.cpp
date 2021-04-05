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

#include "rapidjson/document.h"

#include "apl/engine/context.h"
#include "apl/engine/evaluate.h"
#include "apl/engine/resources.h"
#include "apl/utils/log.h"
#include "apl/engine/propdef.h"

namespace apl {


const bool DEBUG_RESOURCES = false;

ResourceOperators sDefaultResourceOperators = {
    { "number", asNumber },
    { "numbers", asNumber },
    { "dimension", asDimension },
    { "dimensions", asDimension },
    { "string", asString },
    { "strings", asString },
    { "boolean", asBoolean },
    { "booleans", asBoolean },
    { "color", asColor },
    { "colors", asColor },
    { "gradient", asGradient },
    { "gradients", asGradient },
    { "easing", asEasing },
    { "easings", asEasing },
};


static void
addResourceBlock(
        Context& context,
        const rapidjson::Value& block,
        const Path& path,
        const ResourceOperators& resourceOperators)
{
    if (DEBUG_RESOURCES) {
        LOG(LogLevel::kDebug) << path;
        auto description = block.FindMember("description");
        if (description != block.MemberEnd() && description->value.IsString())
            LOG(LogLevel::kDebug) << "Evaluating resource block: " << description->value.GetString();
    }

    auto when = block.FindMember("when");
    if (when != block.MemberEnd() && !evaluate(context, when->value).asBoolean()) {
        LOG_IF(DEBUG_RESOURCES) << "...skipping";
        return;
    }

    // Iterate over all members
    for (auto propIter = block.MemberBegin() ; propIter != block.MemberEnd() ; propIter++) {
        if (!propIter->value.IsObject())
            continue;

        auto name = propIter->name.GetString();
        auto opIter = resourceOperators.find(name);
        if (opIter == resourceOperators.end())
            continue;

        auto conversionFunc = opIter->second;
        auto memberPath = path.addObject(name);

        const rapidjson::Value& properties = propIter->value;
        for (auto itemIter = properties.MemberBegin() ; itemIter != properties.MemberEnd() ; itemIter++) {
            auto resourceName = itemIter->name.GetString();
            auto result = conversionFunc(context, evaluate(context, itemIter->value));
            context.putResource(std::string("@")+resourceName, result, memberPath.addObject(resourceName));
            LOG_IF(DEBUG_RESOURCES) << " @" << resourceName << ": " << result
                << " [" << memberPath.addObject(resourceName).toString() << "]";
        }
    }
}

void
addOrderedResources(
        Context& context,
        const rapidjson::Value& value,
        const Path& path,
        const ResourceOperators& resourceOperators)
{
    if (value.IsArray()) {
        LOG_IF(DEBUG_RESOURCES) << "addOrderedResources: " << value.GetArray().Size();

        auto arraySize = value.Size();
        for (rapidjson::SizeType i = 0; i < arraySize; i++) {
            const auto &item = value[i];
            if (!item.IsObject()) {
                LOG_IF(DEBUG_RESOURCES) << "addOrderedResources - item is not an object: " << path << i;
                continue;
            }

            addResourceBlock(context, item, path.addIndex(i), resourceOperators);
        }
    } else {
        addResourceBlock(context, value, path, resourceOperators);
    }
}

void
addNamedResourcesBlock(
        Context &context,
        const rapidjson::Value &json,
        const Path &path,
        const std::string &resourceBlockName,
        const ResourceOperators& resourceOperators)
{
    auto resIter = json.FindMember(resourceBlockName.c_str());
    if (resIter != json.MemberEnd()) {
        if (resIter->value.IsArray())
            addOrderedResources(context, resIter->value, path.addArray(resourceBlockName), resourceOperators);
        else if (resIter->value.IsObject())
            addOrderedResources(context, resIter->value, path.addObject(resourceBlockName), resourceOperators);
    }
}

} // namespace apl