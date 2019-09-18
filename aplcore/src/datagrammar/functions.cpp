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

#include <cassert>
#include <cmath>

#include "apl/datagrammar/node.h"
#include "apl/engine/context.h"
#include "apl/primitives/dimension.h"
#include "apl/utils/log.h"

namespace apl {
namespace datagrammar {

template<Object (*func)(const Object&)>
Object Unary(const Context& context, const std::vector<Object>& args) {
    return func(args.at(0).eval(context));
}

template<Object (*func)(const Object&, const Object&)>
Object Binary(const Context& context, const std::vector<Object>& args) {
    auto a = args.at(0);
    auto b = args.at(1);

    if (a.isNode())
        a = a.eval(context);

    if (b.isNode())
        b = b.eval(context);

    return func(a, b);
}

Object
CalculateUnaryPlus(const Object& arg) {
    if (arg.isNumber() || arg.isNonAutoDimension())
        return arg;

    return Object::NAN_OBJECT();
}

Object
UnaryPlus(std::vector<Object>&& args)
{
    assert(args.size() == 1);

    if (args.at(0).isNode())
        return std::make_shared<Node>(Unary<CalculateUnaryPlus>, std::move(args), "+");

    return CalculateUnaryPlus(args.at(0));
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
UnaryMinus(std::vector<Object>&& args) {
    assert(args.size() == 1);

    if (args.at(0).isNode())
        return std::make_shared<Node>(Unary<CalculateUnaryMinus>, std::move(args), "-");

    return CalculateUnaryMinus(args.at(0));
}


Object
CalculateUnaryNot(const Object& arg) {
    return !arg.truthy();
}

Object
UnaryNot(std::vector<Object>&& args) {
    assert(args.size() == 1);

    if (args.at(0).isNode())
        return std::make_shared<Node>(Unary<CalculateUnaryNot>, std::move(args), "!");

    return CalculateUnaryNot(args.at(0));
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
Multiply(std::vector<Object>&& args) {
    assert(args.size() == 2);

    if (args.at(0).isNode() || args.at(1).isNode())
        return std::make_shared<Node>(Binary<CalculateMultiply>, std::move(args), "*");

    return CalculateMultiply(args.at(0), args.at(1));
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
Divide(std::vector<Object>&& args) {
    assert(args.size() == 2);

    if (args.at(0).isNode() || args.at(1).isNode())
        return std::make_shared<Node>(Binary<CalculateDivide>, std::move(args), "/");

    return CalculateDivide(args.at(0), args.at(1));
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
Remainder(std::vector<Object>&& args) {
    assert(args.size() == 2);

    if (args.at(0).isNode() || args.at(1).isNode())
        return std::make_shared<Node>(Binary<CalculateRemainder>, std::move(args), "%");

    return CalculateRemainder(args.at(0), args.at(1));
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
Add(std::vector<Object>&& args) {
    assert(args.size() == 2);

    if (args.at(0).isNode() || args.at(1).isNode())
        return std::make_shared<Node>(Binary<CalculateAdd>, std::move(args), "+");

    return CalculateAdd(args.at(0), args.at(1));
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

Object
Subtract(std::vector<Object>&& args) {
    assert(args.size() == 2);

    if (args.at(0).isNode() || args.at(1).isNode())
        return std::make_shared<Node>(Binary<CalculateSubtract>, std::move(args), "-");

    return CalculateSubtract(args.at(0), args.at(1));
}

Object
CalculateLessThan(const Object& a, const Object& b) {
    if (a.isNumber()) {
        if (b.isNumber())
            return (a.getDouble() < b.getDouble() ? Object::TRUE() : Object::FALSE());
        if (b.isAbsoluteDimension())
            return (a.getDouble() < b.getAbsoluteDimension() ? Object::TRUE() : Object::FALSE());
        if (b.isRelativeDimension())
            return (a.getDouble() < b.getRelativeDimension() ? Object::TRUE() : Object::FALSE());
    }

    if (a.isAbsoluteDimension()) {
        if (b.isNumber())
            return (a.getAbsoluteDimension() < b.getDouble() ? Object::TRUE() : Object::FALSE());
        if (b.isAbsoluteDimension())
            return (a.getAbsoluteDimension() < b.getAbsoluteDimension() ?
                Object::TRUE() : Object::FALSE());
    }

    if (a.isRelativeDimension()) {
        if (b.isNumber())
            return (a.getRelativeDimension() < b.getDouble() ? Object::TRUE() : Object::FALSE());
        if (b.isRelativeDimension())
            return (a.getRelativeDimension() < b.getRelativeDimension() ?
                Object::TRUE() : Object::FALSE());
    }

    if (a.isString() && b.isString())
        return (a.asString() < b.asString() ? Object::TRUE() : Object::FALSE());

    return Object::FALSE();
}

Object
LessThan(std::vector<Object>&& args) {
    assert(args.size() == 2);

    if (args.at(0).isNode() || args.at(1).isNode())
        return std::make_shared<Node>(Binary<CalculateLessThan>, std::move(args), "<");

    return CalculateLessThan(args.at(0), args.at(1));
}

Object
CalculateGreaterThan(const Object& a, const Object& b) {
    if (a.isNumber()) {
        if (b.isNumber())
            return (a.getDouble() > b.getDouble() ? Object::TRUE() : Object::FALSE());
        if (b.isAbsoluteDimension())
            return (a.getDouble() > b.getAbsoluteDimension() ? Object::TRUE() : Object::FALSE());
        if (b.isRelativeDimension())
            return (a.getDouble() > b.getRelativeDimension() ? Object::TRUE() : Object::FALSE());
    }

    if (a.isAbsoluteDimension()) {
        if (b.isNumber())
            return (a.getAbsoluteDimension() > b.getDouble() ? Object::TRUE() : Object::FALSE());
        if (b.isAbsoluteDimension())
            return (a.getAbsoluteDimension() > b.getAbsoluteDimension() ?
                    Object::TRUE() : Object::FALSE());
    }

    if (a.isRelativeDimension()) {
        if (b.isNumber())
            return (a.getRelativeDimension() > b.getDouble() ? Object::TRUE() : Object::FALSE());
        if (b.isRelativeDimension())
            return (a.getRelativeDimension() > b.getRelativeDimension() ?
                    Object::TRUE() : Object::FALSE());
    }

    if (a.isString() && b.isString())
        return (a.asString() > b.asString() ? Object::TRUE() : Object::FALSE());

    return Object::FALSE();
}

Object
GreaterThan(std::vector<Object>&& args) {
    assert(args.size() == 2);

    if (args.at(0).isNode() || args.at(1).isNode())
        return std::make_shared<Node>(Binary<CalculateGreaterThan>, std::move(args), ">");

    return CalculateGreaterThan(args.at(0), args.at(1));
}

Object
CalculateLessEqual(const Object& a, const Object& b) {
    if (a.isNumber()) {
        if (b.isNumber())
            return (a.getDouble() <= b.getDouble() ? Object::TRUE() : Object::FALSE());
        if (b.isAbsoluteDimension())
            return (a.getDouble() <= b.getAbsoluteDimension() ? Object::TRUE() : Object::FALSE());
        if (b.isRelativeDimension())
            return (a.getDouble() <= b.getRelativeDimension() ? Object::TRUE() : Object::FALSE());
    }

    if (a.isAbsoluteDimension()) {
        if (b.isNumber())
            return (a.getAbsoluteDimension() <= b.getDouble() ? Object::TRUE() : Object::FALSE());
        if (b.isAbsoluteDimension())
            return (a.getAbsoluteDimension() <= b.getAbsoluteDimension() ?
                    Object::TRUE() : Object::FALSE());
    }

    if (a.isRelativeDimension()) {
        if (b.isNumber())
            return (a.getRelativeDimension() <= b.getDouble() ? Object::TRUE() : Object::FALSE());
        if (b.isRelativeDimension())
            return (a.getRelativeDimension() <= b.getRelativeDimension() ?
                    Object::TRUE() : Object::FALSE());
    }

    if (a.isString() && b.isString())
        return (a.asString() <= b.asString() ? Object::TRUE() : Object::FALSE());

    return Object::FALSE();
}

Object
LessEqual(std::vector<Object>&& args) {
    assert(args.size() == 2);

    if (args.at(0).isNode() || args.at(1).isNode())
        return std::make_shared<Node>(Binary<CalculateLessEqual>, std::move(args), "<=");

    return CalculateLessEqual(args.at(0), args.at(1));
}

Object
CalculateGreaterEqual(const Object& a, const Object& b) {
    if (a.isNumber()) {
        if (b.isNumber())
            return (a.getDouble() >= b.getDouble() ? Object::TRUE() : Object::FALSE());
        if (b.isAbsoluteDimension())
            return (a.getDouble() >= b.getAbsoluteDimension() ? Object::TRUE() : Object::FALSE());
        if (b.isRelativeDimension())
            return (a.getDouble() >= b.getRelativeDimension() ? Object::TRUE() : Object::FALSE());
    }

    if (a.isAbsoluteDimension()) {
        if (b.isNumber())
            return (a.getAbsoluteDimension() >= b.getDouble() ? Object::TRUE() : Object::FALSE());
        if (b.isAbsoluteDimension())
            return (a.getAbsoluteDimension() >= b.getAbsoluteDimension() ?
                    Object::TRUE() : Object::FALSE());
    }

    if (a.isRelativeDimension()) {
        if (b.isNumber())
            return (a.getRelativeDimension() >= b.getDouble() ? Object::TRUE() : Object::FALSE());
        if (b.isRelativeDimension())
            return (a.getRelativeDimension() >= b.getRelativeDimension() ?
                    Object::TRUE() : Object::FALSE());
    }

    if (a.isString() && b.isString())
        return (a.asString() >= b.asString() ? Object::TRUE() : Object::FALSE());

    return Object::FALSE();
}

Object
GreaterEqual(std::vector<Object>&& args) {
    assert(args.size() == 2);

    if (args.at(0).isNode() || args.at(1).isNode())
        return std::make_shared<Node>(Binary<CalculateGreaterEqual>, std::move(args), ">=");

    return CalculateGreaterEqual(args.at(0), args.at(1));
}

Object
CalculateEqual(const Object& a, const Object& b) {
    if (a.isNumber()) {
        if (b.isNumber())
            return (a.getDouble() == b.getDouble() ? Object::TRUE() : Object::FALSE());
        if (b.isAbsoluteDimension())
            return (a.getDouble() == b.getAbsoluteDimension() ? Object::TRUE() : Object::FALSE());
        if (b.isRelativeDimension())
            return (a.getDouble() == b.getRelativeDimension() ? Object::TRUE() : Object::FALSE());
    }

    if (a.isAbsoluteDimension()) {
        if (b.isNumber())
            return (a.getAbsoluteDimension() == b.getDouble() ? Object::TRUE() : Object::FALSE());
        if (b.isAbsoluteDimension())
            return (a.getAbsoluteDimension() == b.getAbsoluteDimension() ?
                    Object::TRUE() : Object::FALSE());
    }

    if (a.isRelativeDimension()) {
        if (b.isNumber())
            return (a.getRelativeDimension() == b.getDouble() ? Object::TRUE() : Object::FALSE());
        if (b.isRelativeDimension())
            return (a.getRelativeDimension() == b.getRelativeDimension() ?
                    Object::TRUE() : Object::FALSE());
    }

    if (a.isString() && b.isString())
        return (a.asString() == b.asString() ? Object::TRUE() : Object::FALSE());

    if (a.isBoolean() && b.isBoolean())
        return (a.asBoolean() == b.asBoolean() ? Object::TRUE() : Object::FALSE());

    if (a.isColor() && b.isColor())
        return (a.getColor() == b.getColor() ? Object::TRUE() : Object::FALSE());

    if ((a.isNull() && b.isNull()) ||
        (a.isAutoDimension() && b.isAutoDimension()))
        return Object::TRUE();

    return Object::FALSE();
}

Object
Equal(std::vector<Object>&& args) {
    assert(args.size() == 2);

    if (args.at(0).isNode() || args.at(1).isNode())
        return std::make_shared<Node>(Binary<CalculateEqual>, std::move(args), "==");

    return CalculateEqual(args.at(0), args.at(1));
}

Object
CalculateNotEqual(const Object& a, const Object& b) {
    if (a.isNumber()) {
        if (b.isNumber())
            return (a.getDouble() != b.getDouble() ? Object::TRUE() : Object::FALSE());
        if (b.isAbsoluteDimension())
            return (a.getDouble() != b.getAbsoluteDimension() ? Object::TRUE() : Object::FALSE());
        if (b.isRelativeDimension())
            return (a.getDouble() != b.getRelativeDimension() ? Object::TRUE() : Object::FALSE());
    }

    if (a.isAbsoluteDimension()) {
        if (b.isNumber())
            return (a.getAbsoluteDimension() != b.getDouble() ? Object::TRUE() : Object::FALSE());
        if (b.isAbsoluteDimension())
            return (a.getAbsoluteDimension() != b.getAbsoluteDimension() ?
                    Object::TRUE() : Object::FALSE());
    }

    if (a.isRelativeDimension()) {
        if (b.isNumber())
            return (a.getRelativeDimension() != b.getDouble() ? Object::TRUE() : Object::FALSE());
        if (b.isRelativeDimension())
            return (a.getRelativeDimension() != b.getRelativeDimension() ?
                    Object::TRUE() : Object::FALSE());
    }

    if (a.isString() && b.isString())
        return (a.asString() != b.asString() ? Object::TRUE() : Object::FALSE());

    if (a.isBoolean() && b.isBoolean())
        return (a.asBoolean() != b.asBoolean() ? Object::TRUE() : Object::FALSE());

    if (a.isColor() && b.isColor())
        return (a.getColor() != b.getColor() ? Object::TRUE() : Object::FALSE());

    if ((a.isNull() && b.isNull()) ||
        (a.isAutoDimension() && b.isAutoDimension()))
        return Object::FALSE();

    return Object::TRUE();
}

Object
NotEqual(std::vector<Object>&& args) {
    assert(args.size() == 2);

    if (args.at(0).isNode() || args.at(1).isNode())
        return std::make_shared<Node>(Binary<CalculateNotEqual>, std::move(args), "!=");

    return CalculateNotEqual(args.at(0), args.at(1));
}

Object
CalculateAnd(const Object& a, const Object& b) {
    return a.truthy() ? b : a;
}

Object
CalculateUnaryTruthy(const Object& a) {
    return a;
}

Object
And(std::vector<Object>&& args) {
    assert(args.size() == 2);

    auto a = args.at(0);

    // If "a" is a node, we always need to evaluate and calculate
    if (a.isNode())
        return std::make_shared<Node>(Binary<CalculateAnd>, std::move(args), "&&");

    // "a" is not a node and it is false.  Simply return "a' and ignore b
    if (!a.asBoolean())
        return a;

    // "a" is not a node and it is true.  Check if "b" is a node
    auto b = args.at(1);
    if (b.isNode())
        return std::make_shared<Node>(Unary<CalculateUnaryTruthy>, std::vector<Object>{b}, "&&");

    return b;
}

Object
CalculateOr(const Object& a, const Object& b) {
    return a.asBoolean() ? a : b;
}

Object
Or(std::vector<Object>&& args) {
    assert(args.size() == 2);

    auto a = args[0];

    // If "a" is a node, we always evaluate
    if (a.isNode())
        return std::make_shared<Node>(Binary<CalculateOr>, std::move(args), "||");

    // "a" is not a node.  If it is true, simply return it
    if (a.asBoolean())
        return a;

    // "a" is not a node and it is false.  The return value depends on b.
    auto b = args[1];
    if (b.isNode())
        return std::make_shared<Node>(Unary<CalculateUnaryTruthy>, std::vector<Object>{b}, "||");

    return b;
}

Object
CalculateNullc(const Object& a, const Object& b) {
    return a.isNull() ? b : a;
}

Object
Nullc(std::vector<Object>&& args) {
    assert(args.size() == 2);

    auto a = args[0];

    if (a.isNode())
        return std::make_shared<Node>(Binary<CalculateNullc>, std::move(args), "??");

    if (!a.isNull())
        return a;

    auto b = args[1];

    if (b.isNode())
        return std::make_shared<Node>(Unary<CalculateUnaryTruthy>, std::vector<Object>{b}, "??");

    return b;
}

Object
EvalUnaryTernary(const Context& context, const std::vector<Object>& args)
{
    auto a = args[0];
    if (a.isNode())
        a = a.eval(context);

    auto b = args.at(a.asBoolean() ? 1 : 2);
    if (b.isNode())
        b = b.eval(context);
    return b;
}

Object
Ternary(std::vector<Object>&& args) {
    assert(args.size() == 3);

    auto a = args[0];
    if (a.isNode())
        return std::make_shared<Node>(EvalUnaryTernary, std::move(args), "?:");

    return args.at(a.truthy() ? 1 : 2);
}


// Combine a group of objects into a single output.
// If there is only a single object, return it.
// If there is more than a single object but the rest of the objects are empty strings,
//   return that object.
// If there are two or more objects that don't have empty strings, string concatenate.
Object
EvalCombine(const Context& context, const std::vector<Object>& args) {
    std::string result;

    for (auto m : args) {
        if (m.isNode())
            m = m.eval(context);
        if (m.isString() && m.getString().empty())
            continue;
        result += m.asString();
    }

    return result;
}

Object
Combine(std::vector<Object>&& args) {
    assert(args.size() > 1);

    // Count empty strings and count nodes
    int count = args.size();
    int non_null_index = -1;
    int i = 0;
    int node_count = 0;

    for (auto it = args.begin(); it != args.end(); it++, i++) {
        if (it->isString() && it->getString().empty())
            count--;
        else {
            non_null_index = i;
            if (it->isNode())
                node_count++;
        }
    }

    // All arguments are empty (or there are no strings).
    if (count == 0)
        return Object("");

    // One argument is non-empty; that's our return value
    if (count == 1) {
        auto a = args.at(non_null_index);
        if (a.isNode())
            return std::make_shared<Node>(Unary<CalculateUnaryTruthy>, std::vector<Object>{a}, "combine");
        return a;
    }

    // There are at least two non-empty elements and one node.  Defer our calculation
    if (node_count > 0)
        return std::make_shared<Node>(EvalCombine, std::move(args), "combine");

    // There are at least two non-empty elements and no nodes.  Combine everything
    std::string result;
    for (auto& m : args) {
        if (m.isString() && m.empty())
            continue;
        result += m.asString();
    }

    return result;
}

// Keep this as a separate function so that Nodes can identify symbols by comparing function pointers
Object
SymbolAccess(const Context& context, const std::vector<Object>& args) {
    auto key = args.at(0).asString();
    return context.opt(key);
}

Object
Symbol(const Context& context, std::vector<Object>&& args, const std::string& name) {
    assert(args.size() == 1);

    Object object = args.front();
    if (!object.isString())
        return Object::NULL_OBJECT();

    auto symbolName = object.asString();
    // If the context contains the symbol and the symbol is not marked as mutable, we
    // return the symbol value.
    if (context.hasImmutable(symbolName))
        return context.opt(symbolName);

    return std::make_shared<Node>(SymbolAccess, std::move(args), name);
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

Object
FieldAccess(std::vector<Object>&& args)
{
    assert(args.size() == 2);

    if (args.at(0).isNode() || args.at(1).isNode())
        return std::make_shared<Node>(Binary<CalcFieldAccess>, std::move(args), ".");

    return CalcFieldAccess(args.at(0), args.at(1));
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
            size_t len = a.size();
            int index = static_cast<int>(std::rint(b.getDouble()));
            if (index < 0)
                index += len;
            if (index < 0 || index >= len)
                return Object::NULL_OBJECT();
            return a.at(index);
        }
    }

    return Object::NULL_OBJECT();
}

Object
ArrayAccess(std::vector<Object>&& args)
{
    assert(args.size() == 2);

    if (args.at(0).isNode() || args.at(1).isNode())
        return std::make_shared<Node>(Binary<CalcArrayAccess>, std::move(args), "[]");

    return CalcArrayAccess(args.at(0), args.at(1));
}

// f(A,B,C)
Object
EvalFunctionCall(const Context& context, const std::vector<Object>& args)
{
    auto a = args[0];
    if (a.isNode())
        a = a.eval(context);

    if (!a.isFunction())
        return Object::NULL_OBJECT();

    auto b = args[1];
    assert(b.isArray());

    auto len = b.size();
    std::vector<Object> argArray;

    for (int i = 0; i < len; i++)
        argArray.emplace_back(b.at(i).eval(context));

    return a.call(argArray);
}

Object
FunctionCall(std::vector<Object>&& args)
{
    assert(args.size() == 2);

    auto a = args.at(0);
    if (a.isNode())
        return std::make_shared<Node>(EvalFunctionCall, std::move(args), "function");

    if (!a.isFunction())
        return Object::NULL_OBJECT();

    auto b = args.at(1);
    assert(b.isArray());

    // Copy all of the arguments into a vector that can be called
    // If any of them are nodes, we toss it to the eval function
    std::vector<Object> argArray;
    auto len = b.size();
    for (int i = 0; i < len; i++) {
        if (b.at(i).isNode())
            return std::make_shared<Node>(EvalFunctionCall, std::move(args), "function");

        argArray.emplace_back(b.at(i));
    }

    return a.call(argArray);
}

}  // namespace datagrammar
}  // namespace apl
