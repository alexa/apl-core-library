/*
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
 *
 * Data-binding rules
 */

#ifndef _APL_DATA_BINDING_STACK_H
#define _APL_DATA_BINDING_STACK_H

#include <cstdio>
#include <tao/pegtl.hpp>
#include "databindinggrammar.h"
#include "functions.h"
#include "node.h"
#include "apl/engine/context.h"
#include "apl/primitives/object.h"
#include "apl/utils/log.h"
#include "apl/utils/streamer.h"

namespace apl {
namespace datagrammar {

static const bool DEBUG_STATE = false;

// Operator precedence
enum {
    OP_FIELD_OR_FUNCTION = 1,
    OP_UNARY = 2,
    OP_TERM = 3,
    OP_EXPRESSION = 4,
    OP_COMPARISON = 5,
    OP_EQUALITY = 6,
    OP_LOGICAL_AND = 7,
    OP_LOGICAL_OR = 8,
    OP_NULLC = 9,   // Note: associates right-to-left
    OP_TERNARY = 10,
    OP_GROUP = 20,
    OP_DB = 21
};

using CreateFunction = Object (*)(std::vector<Object>&&);

struct Operator
{
    int order;
    CreateFunction func;
    std::string name;
};

static const std::map<std::string, Operator> sTermOperators = {
    {"*", {OP_TERM, Multiply,  "*"}},
    {"/", {OP_TERM, Divide,    "/"}},
    {"%", {OP_TERM, Remainder, "%"}}
};

static const std::map<std::string, Operator> sExpressionOperators = {
    {"+", {OP_EXPRESSION, Add,      "+"}},
    {"-", {OP_EXPRESSION, Subtract, "-"}}
};

static const std::map<std::string, Operator> sCompareOperators = {
    {"<",  {OP_COMPARISON, LessThan,     "<"}},
    {">",  {OP_COMPARISON, GreaterThan,  ">"}},
    {"<=", {OP_COMPARISON, LessEqual,    "<="}},
    {">=", {OP_COMPARISON, GreaterEqual, ">="}}
};

static const std::map<std::string, Operator> sEqualityOperators = {
    {"==", {OP_EQUALITY, Equal,    "=="}},
    {"!=", {OP_EQUALITY, NotEqual, "!="}}
};

static const std::map< char, Operator > sUnaryOperators = {
    {'!', {OP_UNARY, UnaryNot,   "!"}},
    {'+', {OP_UNARY, UnaryPlus,  "+"}},
    {'-', {OP_UNARY, UnaryMinus, "-"}}
};

static const Operator FIELD_ACCESS_OPERATOR = {OP_FIELD_OR_FUNCTION, FieldAccess, "."};
static const Operator ARRAY_ACCESS_OPERATOR = {OP_FIELD_OR_FUNCTION, ArrayAccess, "[" };
static const Operator TERNARY_OPERATOR = { OP_TERNARY, Ternary, "?:"};
static const Operator FUNCTION_OPERATOR = { OP_FIELD_OR_FUNCTION, FunctionCall, "function" };
static const Operator GROUP_OPERATOR = { OP_GROUP, nullptr, "("};
static const Operator NULLC_OPERATOR = { OP_NULLC, Nullc, "nullc"};
static const Operator AND_OPERATOR = {OP_LOGICAL_AND, And, "and" };
static const Operator OR_OPERATOR = {OP_LOGICAL_OR, Or, "or" };
static const Operator DB_OPERATOR = {OP_DB, nullptr, "${"};

// State within parsing a single string
enum CombineType {
    /** The top-level string holding the expression */
    kCombineTopString,

    /** An embedded string such as ${...'  '...} */
    kCombineEmbeddedString,

    /** A comma-separated list */
    kCombineVector,

    /** A single argument - for example, an atribute inside of brackets [] */
    kCombineSingle
};

class Stack {
public:
    Stack(int depth) : mDepth(depth) {}

    void push(const Object& object)
    {
        LOG_IF(DEBUG_STATE) << "Stack[" << mDepth << "].push_object " << object;
        mObjects.push_back(object);
    }

    void push(const Operator& op)
    {
        LOG_IF(DEBUG_STATE) << "Stack[" << mDepth << "].push( " << op.name << " )";
        mOps.push_back(op);
    }

    void pop(const Operator& op) {
        LOG_IF(DEBUG_STATE) << "Stack[" << mDepth << "].pop(" << op.name << " ) " << toString();
        assert(!mOps.empty());
        assert(mOps.back().order == op.order);
        mOps.pop_back();
    }

    double popNumber() {
        assert(!mObjects.empty());
        auto result = mObjects.back().getDouble();
        mObjects.pop_back();
        return result;
    }

    // Reduce left-to-right a series of binary operations
    void reduceLR(int order)
    {
        // Search from the back to the starting position with the first operator to combine
        auto backIter = mOps.rbegin();
        while ( backIter != mOps.rend() && backIter->order == order)
            backIter++;

        auto opIter = backIter.base();  // Points to the first valid operator
        auto objectIter = mObjects.end() - (mOps.end() - opIter + 1);  // Points to the starting object

        while (opIter != mOps.end()) {
            LOG_IF(DEBUG_STATE) << "Reducing " << opIter->name;
            auto node = opIter->func(std::vector<Object>(objectIter, objectIter+2));
            *objectIter = node;
            mObjects.erase( objectIter + 1, objectIter + 2);
            opIter = mOps.erase(opIter);
        }
    }

    // Reduce a unary operation.  Return true if we found a unary operation to reduce
    bool reduceUnary(int order) {
        auto back = mOps.rbegin();
        if (back == mOps.rend() || back->order != order)
            return false;

        auto node = back->func(std::vector<Object>(mObjects.end() - 1, mObjects.end()));
        mObjects.pop_back();
        mObjects.emplace_back(std::move(node));
        mOps.pop_back();
        return true;
    }

    // Reduce a single binary operation at the end
    void reduceBinary(int order) {
        auto back = mOps.rbegin();
        if (back == mOps.rend() || back->order != order)
            return;

        assert(mObjects.size() >= 2);
        auto node = back->func(std::vector<Object>(mObjects.end() - 2, mObjects.end()));
        mObjects.pop_back();
        mObjects.pop_back();
        mOps.pop_back();
        mObjects.emplace_back(std::move(node));
    }

    // Reduce a ternary operation at the end
    void reduceTernary(int order) {
        auto back = mOps.rbegin();
        if (back == mOps.rend() || back->order != order)
            return;

        back++;
        assert(back != mOps.rend() && back->order == order);
        auto node = back->func(std::vector<Object>(mObjects.end() - 3, mObjects.end()));
        mObjects.erase(mObjects.end() - 3, mObjects.end());
        mOps.erase(mOps.end() - 2, mOps.end());
        mObjects.emplace_back(std::move(node));
    }

    Object combine(CombineType combineType)
    {
        LOG_IF(DEBUG_STATE) << "[" << mDepth << "] Stack.combine";

        switch (combineType) {
            case kCombineEmbeddedString:
                // If there's nothing, we started with an empty string
                if (mObjects.empty())
                    return Object("");

                if (mObjects.size() == 1)
                    return mObjects.back();

                return Combine(std::vector<Object>(mObjects.begin(), mObjects.end()));

            case kCombineTopString:
                // If there's nothing, we started with an empty string
                if (mObjects.empty())
                    return Object("");

                if (mObjects.size() == 1)
                    return mObjects.back();

                return Combine(std::vector<Object>(mObjects.begin(), mObjects.end()));

            case kCombineVector:
                // TODO: Do we have move assignment?
                return Object(std::make_shared<std::vector<Object> >(mObjects));

            case kCombineSingle:
                assert(mObjects.size() == 1);
                return mObjects.back();
        }

        throw std::runtime_error("Illegal combination");
    }

    void dump()
    {
        printf(" stack %s", toString().c_str());
    }

    std::string toString() {
        streamer buf;

        buf << "[";
        auto it = mOps.begin();
        if (it != mOps.end()) {
            buf << it->name;
            it++;
        }

        while (it != mOps.end()) {
            buf << "," << it->name;
            it++;
        }

        buf << "][";
        auto it2 = mObjects.begin();
        if (it2 != mObjects.end()) {
            buf << *it2;
            it2++;
        }

        while (it2 != mObjects.end()) {
            buf << "," << *it2;
            it2++;
        }

        buf << "]";
        return buf.str();
    }

private:
    int mDepth;
    std::vector<Object> mObjects;
    std::vector<Operator> mOps;
};

class Stacks
{
public:
    // Start with an initial stack that is handling the outer string context
    Stacks(const Context& context) : mContext(context) {
        open();
    }

    // Call this when you start processing a new string region or list of arguments
    void open()
    {
        LOG_IF(DEBUG_STATE) << "Stacks.open";
        mStack.emplace_back(Stack(mStack.size() + 1));
    }

    // Call this when you stop processing a region (string, parenthesis, arglist)
    void close(CombineType combineType)
    {
        LOG_IF(DEBUG_STATE) << "Stacks.close";
        auto object = mStack.back().combine(combineType);
        mStack.pop_back();
        mStack.back().push(object);
    }

    // TODO: Change this to emplace_back
    void push(const Object& object) { mStack.back().push(object); }
    void push(const Operator& op) { mStack.back().push(op); }
    void pop(const Operator& op) { mStack.back().pop(op); }

    double popNumber() { return mStack.back().popNumber(); }

    /**
     * Reduce any number of operators with the same order, following a left-to-right
     * strategy.  For example, "1 - 3 + 4 - 5" will be resolved as (((1-3)+4)-5).
     * @param order The operator order (see the operator precedence enumeration)
     */
    void reduceLR(int order) { mStack.back().reduceLR(order); }

    /**
     * Reduce any number of unary operators with the given order.  If the top operator
     * on the stack does not match "order", this method does nothing.
     * @param order The order of the operator
     */
    void reduceUnary(int order) { while (mStack.back().reduceUnary(order)) ; }

    /**
     * Reduce a single binary operator with the given order.  If the top operator
     * on the stack does not match "order", this method does nothing.
     * @param order The order of the operator
     */
    void reduceBinary(int order) { mStack.back().reduceBinary(order); }

    /**
     * Reduce a single ternary operator with the given order.  If the top operator
     * on the stack does not match "order", this method does nothing.  If the
     * top TWO operators on the stack don't match "order", we throw an exception.
     * @param order The order of the operator.
     */
    void reduceTernary(int order) { mStack.back().reduceTernary(order); }

    Object finish()
    {
        LOG_IF(DEBUG_STATE) << "Stacks.finish";
        assert(mStack.size() == 1);
        return mStack.back().combine(kCombineTopString);
    }

    void dump()
    {
        LOG(LogLevel::kDebug) << "Stacks=" << mStack.size();
        for (auto& m : mStack)
            m.dump();
    }

    const Context& context() const { return mContext; }

private:
    std::vector<Stack> mStack;
    const Context& mContext;
};
} // namespace datagrammar
} // namespace apl

#endif // _APL_DATA_BINDING_STACK_H