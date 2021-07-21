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

#ifndef _APL_OBJECT_BAG_H
#define _APL_OBJECT_BAG_H

#include <map>

#include "apl/utils/bimap.h"
#include "object.h"

namespace apl {

using Mapper = Bimap<int, std::string>;

template<Mapper& mapper>
class ObjectBag {
private:
    std::map<int, Object> mValues;

public:
    ObjectBag() {}
    ObjectBag(std::map<int, Object>&& values) : mValues(std::move(values)) {}

    auto emplace(const std::string& key, Object value)
            -> decltype(mValues.emplace(mapper.at(key), std::move(value)))
    {
        return mValues.emplace(mapper.at(key), std::move(value));
    }

    auto emplace(int key, Object value)
            -> decltype(mValues.emplace(key, std::move(value)))
    {
        return mValues.emplace(key, std::move(value));
    }

    const Object& at(int index) const { return mValues.at(index); }
    const Object& at(const char *name) const { return mValues.at(mapper.at(name)); }

    typename std::map<int, Object>::const_iterator find(int index) const { return mValues.find(index); }
    typename std::map<int, Object>::const_iterator begin() const { return mValues.begin(); }
    typename std::map<int, Object>::const_iterator end() const { return mValues.end(); }

    unsigned int size() const { return mValues.size(); }

    bool operator==(const ObjectBag<mapper>& rhs) const { return mValues == rhs.mValues; }
};

} // namespace apl

#endif //_APL_OBJECT_BAG_H
