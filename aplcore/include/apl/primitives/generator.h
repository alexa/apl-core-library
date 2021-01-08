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

#ifndef _APL_GENERATOR_H
#define _APL_GENERATOR_H

#include "apl/primitives/objectdata.h"

namespace apl {

/**
 * A generator is something that looks and acts like an Object::kArrayType, but generates the
 * array entries dynamically.
 *
 * The common generator base class will generate a locally cached copy of the generator output
 * when the Object::getArray() method is called.  In most common APL interactions this is never
 * called, so the generator is memory efficient.
 */
class Generator : public ObjectData {
public:
    /**
     * The getArray() method returns a const reference to an ObjectArray.  Generators
     * do not have built-in ObjectArrays, so they must populate a local object array with a
     * suitable copy of the data.
     * @return The cached local object array
     */
    const ObjectArray& getArray() const override {
        const auto len = size();
        if (len > 0 && mCached.empty()) {
            auto& cached = const_cast<ObjectArray&>(mCached);
            for (size_t i = 0 ; i < len ; i++)
                cached.push_back(at(i));
        }

        return mCached;
    }

private:
    ObjectArray mCached;
};

} // namespace apl

#endif // _APL_GENERATOR_H
