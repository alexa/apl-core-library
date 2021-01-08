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

#include "apl/engine/evaluate.h"
#include "apl/engine/arrayify.h"
#include "apl/utils/session.h"
#include "apl/primitives/object.h"
#include "apl/graphic/graphicpattern.h"
#include "apl/graphic/graphicelement.h"
#include "apl/graphic/graphicbuilder.h"

namespace apl {

// Start a little offset to catch errors
id_type GraphicPattern::sUniqueIdGenerator = 1000;

GraphicPattern::GraphicPattern(const std::string& description, double height, double width,
        std::vector<GraphicElementPtr>&& items) :
    mId(std::to_string(GraphicPattern::sUniqueIdGenerator++)),
    mDescription(description),
    mHeight(height),
    mWidth(width),
    mItems(std::move(items)) {}

Object
GraphicPattern::create(const Context& context, const Object& object)
{
    if (object.isGraphicPattern())
        return object;

    if (!object.isMap())
        return Object::NULL_OBJECT();

    std::string description = propertyAsString(context, object, "description");
    double height = propertyAsDouble(context, object, "height", -1);
    if (height < 0) {
        CONSOLE_CTX(context) << "GraphicPattern height is required.";
        return Object::NULL_OBJECT();
    }
    double width  = propertyAsDouble(context, object, "width", -1);
    if (width < 0) {
        CONSOLE_CTX(context) << "GraphicPattern width is required.";
        return Object::NULL_OBJECT();
    }

    std::vector<GraphicElementPtr> items;

    auto graphicElements = arrayifyProperty(context, object, "items", "item");
    for (auto& graphicElement : graphicElements) {
        auto item = GraphicBuilder::build(context, graphicElement);
        if (item)
            items.emplace_back(item);
    }

    return Object(std::make_shared<GraphicPattern>(description, height, width, std::move(items)));
}

std::string
GraphicPattern::toDebugString() const {
    std::string result = "GraphicPattern< id=" + getId() +
            " description=" + getDescription() +
            " width=" + std::to_string(getWidth()) +
            " height=" + std::to_string(getHeight()) +
            " items=[";
    for (const auto& item : getItems())
        result += " " + item->toDebugString();
    result += " ]>";
    return result;
}

rapidjson::Value
GraphicPattern::serialize(rapidjson::Document::AllocatorType& allocator) const {
    using rapidjson::Value;
    Value v(rapidjson::kObjectType);
    v.AddMember("id", rapidjson::Value(getId().c_str(), allocator).Move(), allocator);
    v.AddMember("description", rapidjson::Value(getDescription().c_str(), allocator).Move(), allocator);
    v.AddMember("width", getWidth(), allocator);
    v.AddMember("height", getHeight(), allocator);
    Value items(rapidjson::kArrayType);
    for(auto& item : getItems()) {
        items.PushBack(item->serialize(allocator), allocator);
    }
    v.AddMember("items", items.Move(), allocator);
    return v;
}

}  // namespace apl
