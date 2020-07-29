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

#include "apl/graphic/graphiccontent.h"
#include "apl/graphic/graphicproperties.h"
#include "apl/utils/session.h"

namespace apl {

GraphicContentPtr
GraphicContent::create(JsonData&& data)
{
    return create(nullptr, std::move(data));
}

GraphicContentPtr
GraphicContent::create(const SessionPtr& session, JsonData&& data)
{
    const rapidjson::Value& json = data.get();

    // A bunch of sanity checks to make sure this is a well-formed graphic.
    // These checks don't guarantee that it could be drawn.

    if (!data) {
        CONSOLE_S(session) << "Graphic Json could not be parsed: " << data.error();
        return nullptr;
    }

    if (!json.IsObject()) {
        CONSOLE_S(session) << "Unknown object type for graphic";
        return nullptr;
    }

    auto type = json.FindMember("type");
    if (type == json.MemberEnd() || !type->value.IsString()) {
        CONSOLE_S(session) << "Missing 'type' property in graphic";
        return nullptr;
    }

    auto typeString = type->value.GetString();
    if (::strcmp(typeString, "AVG")) {
        CONSOLE_S(session) << "Invalid 'type' property in graphic - must be 'AVG'";
        return nullptr;
    }

    auto version = json.FindMember("version");
    if (version == json.MemberEnd() || !version->value.IsString()) {
        CONSOLE_S(session) << "Missing or invalid 'version' property in graphic";
        return nullptr;
    }

    std::string v = version->value.GetString();
    if (!sGraphicVersionBimap.has(v)) {
        CONSOLE_S(session) << "Invalid AVG version '" << v << "'";
        return nullptr;
    }

    auto height = json.FindMember("height");
    if (height == json.MemberEnd()) {
        CONSOLE_S(session) << "Missing 'height' property in graphic";
        return nullptr;
    }

    auto width = json.FindMember("width");
    if (width == json.MemberEnd()) {
        CONSOLE_S(session) << "Missing 'width' property in graphic";
        return nullptr;
    }

    return std::make_shared<GraphicContent>(std::move(data));
}

GraphicContent::GraphicContent(JsonData&& data)
    : mJsonData(std::move(data))
{}

}
