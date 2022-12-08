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

#include "apl/primitives/objecttype.h"

#include "apl/engine/context.h"
#include "apl/livedata/livedataobject.h"
#include "apl/primitives/color.h"
#include "apl/primitives/dimension.h"

namespace apl {

/// Base/grouping

Color
ObjectType::asColor(const Object::DataHolder&, const SessionPtr&) const
{
    return {};
}

Dimension
ObjectType::asDimension(const Object::DataHolder&, const Context&) const
{
    return {DimensionType::Absolute, 0};
}

Dimension
ObjectType::asAbsoluteDimension(const Object::DataHolder&, const Context&) const
{
    return {DimensionType::Absolute, 0};
}

Dimension
ObjectType::asNonAutoDimension(const Object::DataHolder&, const Context&) const
{
    return {DimensionType::Absolute, 0};
}

Dimension
ObjectType::asNonAutoRelativeDimension(const Object::DataHolder&, const Context&) const
{
    return {DimensionType::Relative, 0};
}

/// Primitive types

Color
Number::ObjectType::asColor(const Object::DataHolder& dataHolder, const SessionPtr&) const
{
    return {(uint32_t)dataHolder.value};
}

Dimension
Number::ObjectType::asDimension(const Object::DataHolder& dataHolder, const Context&) const
{
    return {DimensionType::Absolute, dataHolder.value};
}

Dimension
Number::ObjectType::asAbsoluteDimension(const Object::DataHolder& dataHolder, const Context&) const
{
    return {DimensionType::Absolute, dataHolder.value};
}

Dimension
Number::ObjectType::asNonAutoDimension(const Object::DataHolder& dataHolder, const Context&) const
{
    return {DimensionType::Absolute, dataHolder.value};
}

Dimension
Number::ObjectType::asNonAutoRelativeDimension(const Object::DataHolder& dataHolder, const Context&) const
{
    return {DimensionType::Relative, dataHolder.value * 100};
}

Color
String::ObjectType::asColor(const Object::DataHolder& dataHolder, const SessionPtr& session) const
{
    return {session, dataHolder.string};
}

Dimension
String::ObjectType::asDimension(const Object::DataHolder& dataHolder, const Context& context) const
{
    return {context, dataHolder.string};
}

Dimension
String::ObjectType::asAbsoluteDimension(const Object::DataHolder& dataHolder, const Context& context) const
{
    auto d = Dimension(context, dataHolder.string);
    return (d.getType() == DimensionType::Absolute ? d : Dimension(DimensionType::Absolute, 0));
}

Dimension
String::ObjectType::asNonAutoDimension(const Object::DataHolder& dataHolder, const Context& context) const
{
    auto d = Dimension(context, dataHolder.string);
    return (d.getType() == DimensionType::Auto ? Dimension(DimensionType::Absolute, 0) : d);
}

Dimension
String::ObjectType::asNonAutoRelativeDimension(const Object::DataHolder& dataHolder, const Context& context) const
{
    auto d = Dimension(context, dataHolder.string, true);
    return (d.getType() == DimensionType::Auto ? Dimension(DimensionType::Relative, 0) : d);
}

} // namespace apl
