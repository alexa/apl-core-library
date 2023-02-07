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

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "apl/animation/easing.h"
#include "apl/content/rootconfig.h"
#include "apl/graphic/graphicfilter.h"
#include "apl/graphic/graphicpattern.h"
#include "apl/primitives/filter.h"
#include "apl/primitives/gradient.h"
#include "apl/primitives/mediasource.h"
#include "apl/primitives/object.h"
#include "apl/primitives/styledtext.h"
#include "apl/primitives/transform.h"
#include "apl/primitives/urlrequest.h"

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

    return std::move(array);
};

Object asOldBoolean(const Context& context, const Object& object)
{
    if (context.getRequestedAPLVersion() == "1.0" && object.isString() && object.getString() == "false")
        return Object::FALSE_OBJECT();

    return object.asBoolean();
}

Object asStyledText(const Context& context, const Object& object)
{
    return StyledText::create(context, object);
}

Object asColor(const Context& context, const Object& object)
{
    return object.asColor(context);
}

Object asFilterArray(const Context& context, const Object& object)
{
    std::vector<Object> data;
    for (auto& m : arrayify(context, object)) {
        auto f = Filter::create(context, m);
        if (f.is<Filter>())
            data.push_back(std::move(f));
    }
    return std::move(data);
}

Object asGraphicFilterArray(const Context& context, const Object& object)
{
    std::vector<Object> data;
    for (auto& m : arrayify(context, object)) {
        auto f = GraphicFilter::create(context, m);
        if (f.is<GraphicFilter>())
            data.push_back(std::move(f));
    }
    return std::move(data);
}

Object asGradient(const Context& context, const Object& object)
{
    return Gradient::create(context, object);
}

Object asFill(const Context& context, const Object& object)
{
    auto gradient = asGradient(context, object);
    return gradient.is<Gradient>() ? gradient : asColor(context, object);
}

Object asVectorGraphicSource(const Context& context, const Object& object)
{
    return object.isString() ? object : URLRequest::create(context, object);
}

Object asImageSourceArray(const Context& context, const Object& object)
{
    std::vector<Object> data;
    for (auto& m : arrayify(context, object)) {
        auto request = m.isString() ? m : URLRequest::create(context, m);
        if (!request.isNull())
            data.emplace_back(std::move(request));
    }

    if (data.empty())
        return "";

    // In case of URLs the spec enforces an array of elements.
    // We don't want to break runtimes that are not using urls and depend
    // on the old logic that evaluates the strings via arrayify
    if (data.size() == 1 && data.front().isString())
        return data.front();

    return std::move(data);
}

Object asMediaSourceArray(const Context& context, const Object& object) {
    std::vector<Object> data;
    for (auto& m : arrayify(context, object)) {
        auto ms = MediaSource::create(context, m);
        if (ms.is<MediaSource>())
            data.push_back(std::move(ms));
    }
    return std::move(data);
}

Object asFilteredText(const Context& context, const Object& object)
{
    return StyledText::create(context, object).getText();
}

Object asTransformOrArray(const Context& context, const Object& object)
{
    if (object.is<Transformation>())
        return object;

    return evaluateRecursive(context, arrayify(context, object));
}

Object asEasing(const Context& context, const Object& object)
{
    if (object.is<Easing>())
        return object;

    return Easing::parse(context.session(), object.asString());
}

Object asGraphicPattern(const Context& context, const Object& object)
{
    return GraphicPattern::create(context, object);
}

Object asAvgGradient(const Context& context, const Object& object)
{
    return Gradient::createAVG(context, object);
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

    return {std::move(data)};
}

} // namespace apl
