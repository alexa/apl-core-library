/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
    auto array = arrayify(context, object);
    if (context.getRequestedAPLVersion() == "1.0") {
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
            }
            else {  // Everything else uses the "asString" method
                m = m.asString();
            }
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

} // namespace apl