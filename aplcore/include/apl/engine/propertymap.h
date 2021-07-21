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

#ifndef _APL_PROPERTY_MAP_H
#define _APL_PROPERTY_MAP_H

#include "apl/utils/bimap.h"
#include "apl/primitives/object.h"

namespace apl {

/**
 * Store calculated values that can be accessed by either string or integer index.
 * @tparam T The enumerated type stored.
 * @tparam bimap The bi-directional map.
 */
template<class T, Bimap<int, std::string>& bimap>
class PropertyMap {
public:
    PropertyMap() {}

    /**
     * @return The number of elements in the property map
     */
    std::size_t size() const { return mValues.size(); }

    /**
     * Return object by key lookup.
     * @param key The key.
     * @return The value or Object::NULL_OBJECT if it does not exist
     */
    const Object& get(T key) const {
        auto it = mValues.find(key);
        if (it != mValues.end())
            return it->second;

        return Object::NULL_OBJECT();
    }

    /**
     * Return object by key lookup.
     * @param key The key.
     * @return The value or Object::NULL_OBJECT if it does not exist
     */
    Object get(T key) {
        auto it = mValues.find(key);
        if (it != mValues.end())
            return it->second;

        return Object::NULL_OBJECT();
    }

    /**
     * Return object by string lookup.
     * @param key The key
     * @return The value or Object::NULL_OBJECT if it does not exist
     */
    const Object& get(const std::string& key) const {
        int index = bimap.get(key, -1);
        if (index == -1)
            return Object::NULL_OBJECT();

        return get(static_cast<T>(index));
    }

    /**
    * Return the string representation of the int key.
    * @param key The key
    * @return The value or NULL if it does not exist
    */
    const std::string getKey(int key) const {
        auto s = bimap.get(key, "");
        return s;
    }

    /**
     * Return object by string lookup.
     * @param key The key
     * @return The value or Object::NULL_OBJECT if it does not exist
     */
    const Object& get(const char *key) const {
        return get(std::string(key));
    }

    /**
     * Return object by string lookup.
     * @param key The key
     * @return The value or Object::NULL_OBJECT if it does not exist
     */
    Object get(const char *key) {
        return get(std::string(key));
    }

    /**
     * Store a value in the property map
     * @param key The key
     * @param value The value
     */
    void set(T key, const Object& value) {
        mValues[key] = value;
    }

    const Object& operator[](T key) const {
        return get(key);
    }

    const Object& operator[](const std::string& key) const {
        return get(key);
    }

    typename std::map<T,Object>::const_iterator find(const T& key) const { return mValues.find(key); }
    typename std::map<T,Object>::const_iterator begin() const { return mValues.begin(); }
    typename std::map<T,Object>::const_iterator end() const { return mValues.end(); }

private:
    std::map<T, Object> mValues;
};

} // namespace apl

#endif //_APL_PROPERTY_MAP_H
