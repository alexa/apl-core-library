/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
 * Node structure
 */

#ifndef _APL_NODE_H
#define _APL_NODE_H

#include <memory>
#include <string>
#include <exception>
#include <initializer_list>
#include <vector>

#include "apl/primitives/object.h"
#include "apl/utils/log.h"

namespace apl {
namespace datagrammar {

const bool DEBUG_NODE = false;

/*
 * Node class is used for expression evaluation
 */

using OperatorFunc = Object (*)(const std::vector<Object>& args);

class Node : public Object::Data
{
public:
    Node(OperatorFunc op, std::vector<Object>&& args, const std::string& name)
        : mOp(op), mArgs(std::move(args)), mName(name) {}

    Object eval() const override
    {
        auto result = mOp(mArgs);
        LOG_IF(DEBUG_NODE) << *this << " ---> " << result;
        return result;
    }

    std::string getSuffix() const;

    void push(const Object& ptr)
    {
        mArgs.push_back(ptr);
    }

    void accept(Visitor<Object>& visitor) const override
    {
        visitor.push();
        for (auto it = mArgs.begin() ; !visitor.isAborted() && it != mArgs.end() ; it++)
            it->accept(visitor);
        visitor.pop();
    }

    const std::vector<Object>& args() const { return mArgs; }
    std::string name() const { return mName; }

    std::string toDebugString() const override;

    friend streamer& operator<<(streamer&, const Node&);

private:
    OperatorFunc mOp;
    std::vector<Object> mArgs;
    std::string mName;
};

} // namespace datagrammar
} // namespace apl

#endif // _APL_NODE_H