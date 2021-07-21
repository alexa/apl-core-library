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

#ifndef _APL_FUNCTIONS_H
#define _APL_FUNCTIONS_H

#include <functional>

#include "apl/primitives/objectdata.h"

namespace apl {

using UserFunction = std::function<Object(const ObjectArray&)>;

extern void createStandardFunctions(Context& context);

/**
 * Hold information about a callable function
 */
class Function : public ObjectData {
public:
    static std::shared_ptr<Function> create(const std::string& name, UserFunction function, bool isPure = true) {
        return std::make_shared<Function>(name, function, isPure);
    }

    Function(const std::string& name, UserFunction function, bool isPure)
        : mName(name), mFunction(function), mIsPure(isPure)
    {}

    /**
     * @return The human-readable name of this function
     */
    std::string name() const { return mName; }

    /**
     * @return True if this function is "pure" and has no side effects or internal state
     *         storage.  A pure function will always return the same result for the same
     *         arguments.  Functions like a random number generator are not pure.
     */
    bool isPure() const { return mIsPure; }

    Object call(const ObjectArray& args) const override {
        return mFunction(args);
    }

    std::string toDebugString() const override {
        return "function<" + mName + ">";
    }

private:
    std::string mName;
    UserFunction mFunction;
    bool mIsPure;
};

}

#endif // _APL_FUNCTIONS_H
