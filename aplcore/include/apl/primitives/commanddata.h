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

#ifndef _APL_COMMAND_DATA
#define _APL_COMMAND_DATA

#include "apl/primitives/object.h"

namespace apl {

/**
 * Simple wrapper class for data which commands are inflated from. Main purpose is to maintain
 * origin memory alive in cases when down level commands created from references to the top one
 * (which is almost always the case).
 * This class relies on Object being trivially copyable and actual references been maintained by
 * ObjectData (as shared pointers).
 */
class CommandData {
public:
    CommandData(const Object& data) : mOrigin(Object::NULL_OBJECT()), mData(data) {}

    CommandData(Object&& data, const CommandData& originData) :
          mOrigin(originData.origin()), mData(std::move(data)) {}

    CommandData(const Object& data, const CommandData& originData) :
          mOrigin(originData.origin()), mData(data) {}

    const Object& get() const { return mData; }

    size_t size() const { return mData.size(); }

    CommandData at(size_t index) const {
        assert(mData.isArray());
        return {mData.at(index), *this};
    }

private:
    const Object& origin() const { return mOrigin.isNull() ? mData : mOrigin; }

    // Origin set in cases when command data derived from other command data.
    Object mOrigin;
    Object mData;
};
}

#endif // _APL_MEDIA_STATE_H