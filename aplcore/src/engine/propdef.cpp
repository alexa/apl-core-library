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

#include "apl/engine/propdef.h"
#include "apl/content/rootconfig.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace apl {

Object asOldArray(const Context &context, const Object &object) {
    auto result = evaluateRecursive(context, arrayify(context, object));
    if (context.getRequestedAPLVersion() != "1.0")
        return result;

    ObjectArray array = result.getArray();
    for (auto& m : array) {
        // Arrays and maps are serialized in JSON
        if (m.isArray() || m.isMap()) {
            rapidjson::Document doc;
            auto json = m.serialize(doc.GetAllocator());

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            json.Accept(writer);

            std::string s(buffer.GetString(), buffer.GetSize());
            m = s;
        } else {  // Everything else uses the "asString" method
            m = m.asString();
        }
    }

    return Object(std::move(array));
};

Object asOldBoolean(const Context& context, const Object& object)
{
    if (context.getRequestedAPLVersion() == "1.0" && object.isString() && object.getString() == "false")
        return Object::FALSE_OBJECT();

    return object.asBoolean();
}

Object asPaddingArray(const Context& context, const Object& object)
{
    std::vector<Object> data;
    for (auto& m : arrayify(context, object))
        data.emplace_back(asAbsoluteDimension(context, m));

    auto size = data.size();
    if (size == 1) {
        data.resize(4);
        data[1] = data[0];  // [X] -> [X,X,X,X]
        data[2] = data[0];
        data[3] = data[0];
    }
    else if (size == 2) {
        data.resize(4);
        data[2] = data[0];  // [X,Y] -> [X,Y,X,Y]
        data[3] = data[1];
    }
    else if (size == 3) {
        data.resize(4);
        data[3] = data[1];  // [X,Y,Z] -> [X,Y,Z,Y]
    }

    return Object(std::move(data));
}

} // namespace apl
