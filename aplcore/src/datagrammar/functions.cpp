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

#include <cassert>
#include <cmath>

#include "apl/datagrammar/boundsymbol.h"
#include "apl/engine/context.h"
#include "apl/primitives/dimension.h"
#include "apl/primitives/functions.h"
#include "apl/utils/log.h"
#include "apl/utils/session.h"

namespace apl {
namespace datagrammar {

Object
CalculateUnaryPlus(const Object& arg) {
    if (arg.isNumber() || arg.isNonAutoDimension())
        return arg;

    return Object::NAN_OBJECT();
}

Object
CalculateUnaryMinus(const Object& arg) {
    if (arg.isNumber())
        return Object(-arg.getDouble());

    if (arg.isAbsoluteDimension())
        return Object(Dimension(DimensionType::Absolute, -arg.getAbsoluteDimension()));

    if (arg.isRelativeDimension())
        return Object(Dimension(DimensionType::Relative, -arg.getRelativeDimension()));

    return Object::NAN_OBJECT();
}

Object
CalculateUnaryNot(const Object& arg) {
    return !arg.truthy();
}

Object
CalculateMultiply(const Object& a, const Object& b) {
    if (a.isNumber()) {
        if (b.isNumber())
            return a.getDouble() * b.getDouble();
        if (b.isAbsoluteDimension())
            return Dimension(a.getDouble() * b.getAbsoluteDimension());
        if (b.isRelativeDimension())
            return Dimension(DimensionType::Relative,
                             a.getDouble() * b.getRelativeDimension());
    } else if (b.isNumber()) {
        if (a.isAbsoluteDimension())
            return Dimension(a.getAbsoluteDimension() * b.getDouble());
        if (a.isRelativeDimension())
            return Dimension(DimensionType::Relative,
                             a.getRelativeDimension() * b.getDouble());
    }

    return Object::NAN_OBJECT();
}

Object
CalculateDivide(const Object& a, const Object& b) {
    if (b.isNumber()) {
        if (a.isNumber())
            return a.getDouble() / b.getDouble();  // Note: May return infinity
        if (a.isAbsoluteDimension())
            return Dimension(a.getAbsoluteDimension() / b.getDouble());
        if (a.isRelativeDimension())
            return Dimension(DimensionType::Relative,
                             a.getRelativeDimension() / b.getDouble());
    }
    // dim / dim is a scalar
    if(a.isAbsoluteDimension() && b.isAbsoluteDimension()) {
        return a.getAbsoluteDimension() / b.getAbsoluteDimension();
    }
    if(a.isRelativeDimension() && b.isRelativeDimension()) {
        return a.getRelativeDimension() / b.getRelativeDimension();
    }
    return Object::NAN_OBJECT();
}

Object
CalculateRemainder(const Object& a, const Object& b) {
    if (b.isNumber()) {
        if (a.isNumber())
            return std::fmod(a.getDouble(), b.getDouble());
        if (a.isAbsoluteDimension())
            return Dimension(std::fmod(a.getAbsoluteDimension(), b.getDouble()));
        if (a.isRelativeDimension())
            return Dimension(DimensionType::Relative,
                             std::fmod(a.getRelativeDimension(), b.getDouble()));
    }
    // dim % dim is a scalar
    if(a.isAbsoluteDimension() && b.isAbsoluteDimension()) {
        return std::fmod(a.getAbsoluteDimension(), b.getAbsoluteDimension());
    }
    if(a.isRelativeDimension() && b.isRelativeDimension()) {
        return std::fmod(a.getRelativeDimension(), b.getRelativeDimension());
    }
    return Object::NAN_OBJECT();
}

Object
CalculateAdd(const Object& a, const Object& b) {
    if (a.isNumber()) {
        if (b.isNumber())
            return Object(a.getDouble() + b.getDouble());
        if (b.isAbsoluteDimension())
            return Dimension(a.getDouble() + b.getAbsoluteDimension());
        if (b.isRelativeDimension())
            return Dimension(DimensionType::Relative,
                    a.getDouble() + b.getRelativeDimension());
    }

    if (a.isAbsoluteDimension()) {
        if (b.isNumber())
            return Dimension(a.getAbsoluteDimension() + b.getDouble());
        if (b.isAbsoluteDimension())
            return Dimension(a.getAbsoluteDimension() + b.getAbsoluteDimension());
    }

    if (a.isRelativeDimension()) {
        if (b.isNumber())
            return Dimension(DimensionType::Relative,
                    a.getRelativeDimension() + b.getDouble());
        if (b.isRelativeDimension())
            return Dimension(DimensionType::Relative,
                    a.getRelativeDimension() + b.getRelativeDimension());
    }

    return Object(a.asString() + b.asString());
}

Object
CalculateSubtract(const Object& a, const Object& b) {
    if (a.isNumber()) {
        if (b.isNumber())
            return Object(a.getDouble() - b.getDouble());
        if (b.isAbsoluteDimension())
            return Dimension(a.getDouble() - b.getAbsoluteDimension());
        if (b.isRelativeDimension())
            return Dimension(DimensionType::Relative,
                             a.getDouble() - b.getRelativeDimension());
    }

    if (a.isAbsoluteDimension()) {
        if (b.isNumber())
            return Dimension(a.getAbsoluteDimension() - b.getDouble());
        if (b.isAbsoluteDimension())
            return Dimension(a.getAbsoluteDimension() - b.getAbsoluteDimension());
    }

    if (a.isRelativeDimension()) {
        if (b.isNumber())
            return Dimension(DimensionType::Relative,
                             a.getRelativeDimension() - b.getDouble());
        if (b.isRelativeDimension())
            return Dimension(DimensionType::Relative,
                             a.getRelativeDimension() - b.getRelativeDimension());
    }

    return Object::NAN_OBJECT();
}

int
Compare(double a, double b)
{
    if (a == b) return 0;
    return a < b ? -1 : 1;
}

int
Compare(const std::string& a, const std::string& b)
{
    if (a == b) return 0;
    return a < b ? -1 : 1;
}

int
Compare(const Object& a, const Object& b)
{
    if (a.isNumber()) {
        if (b.isNumber())
            return Compare(a.getDouble(), b.getDouble());

        if (b.isAbsoluteDimension())
            return Compare(a.getDouble(), b.getAbsoluteDimension());

        if (b.isRelativeDimension())
            return Compare(a.getDouble(), b.getRelativeDimension());
    }

    if (a.isAbsoluteDimension()) {
        if (b.isNumber())
            return Compare(a.getAbsoluteDimension(), b.getDouble());
        if (b.isAbsoluteDimension())
            return Compare(a.getAbsoluteDimension(), b.getAbsoluteDimension());
    }

    if (a.isRelativeDimension()) {
        if (b.isNumber())
            return Compare(a.getRelativeDimension(), b.getDouble());
        if (b.isRelativeDimension())
            return Compare(a.getRelativeDimension(), b.getRelativeDimension());
    }

    if (a.isString() && b.isString())
        return Compare(a.asString(), b.asString());

    if (a.isBoolean() && b.isBoolean() && a.asBoolean() == b.asBoolean())
        return 0;

    if (a.isColor() && b.isColor() && a.getColor() == b.getColor())
        return 0;

    if (a.isNull() && b.isNull())
        return 0;

    if (a.isAutoDimension() && b.isAutoDimension())
        return 0;

    return -1;
}


// A.B
Object
CalcFieldAccess(const Object& a, const Object& b) {
    if (a.isMap() && b.isString())
        return a.get(b.asString());

    if (a.isArray() && b.isString() && b.getString() == "length")
        return a.size();

    return Object::NULL_OBJECT();
}

// A[B]
Object
CalcArrayAccess(const Object& a, const Object& b) {
    if (a.isMap() && b.isString())
        return a.get(b.asString());

    if (a.isArray()) {
        if (b.isString() && b.getString() == "length")
            return a.size();
        else if (b.isNumber()) {
            std::uint64_t len = a.size();
            auto index = static_cast<std::int64_t>(std::round(b.getDouble()));
            if (index < 0)
                index += len;
            if (index < 0 || index >= len)
                return Object::NULL_OBJECT();
            return a.at(index);
        }
    }

    return Object::NULL_OBJECT();
}

// A *+* B  (string merge operation)
Object
MergeOp(const Object& a, const Object& b) {
    if (a.isString() && a.empty())
        return b;
    if (b.isString() && b.empty())
        return a;
    return a.asString() + b.asString();
}

}  // namespace datagrammar
}  // namespace apl
