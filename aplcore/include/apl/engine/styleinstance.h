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

#ifndef _APL_STYLED_H
#define _APL_STYLED_H

#include <map>

#include "apl/primitives/object.h"

namespace apl {

class Path;

/**
 * A StyleInstance contains a map of property names to values.  A single StyleInstance is created by
 * a StyleDefinition for each set of state values used in a Component.  For example, when the Component
 * switches from "disabled" state to "enabled" state a new StyleInstance will be created.
 *
 * Each named property in the style also has a provenance which is the JSON path to the content that
 * defined that particular property in the style.  The overall style also has a provenance which is the
 * JSON path to the content where the style was defined.
 */
class StyleInstance {
public:
    /**
     * Common constructor.
     * @param styleProvenance The provenance path of the style definition itself.
     */
    StyleInstance(const Path& styleProvenance);

    /**
     * Find the defined value of a named style property
     * @param key The name of the style property
     * @return A constant iterator to the named style property or end()
     */
    std::map<std::string, Object>::const_iterator find(const std::string& key) const { return mValue.find(key); }
    
    std::map<std::string, Object>::const_iterator find(const std::vector<std::string>& keys) const {
        for (const auto& key : keys) {
            auto it = mValue.find(key);
            if (it != mValue.end())
                return it;
        }
        return mValue.end();
    }

    /**
     * @return An iterator to the begining of the style.
     */
    std::map<std::string, Object>::const_iterator begin() const { return mValue.begin(); }

    /**
     * @return An iterator to the end of the style
     */
    std::map<std::string, Object>::const_iterator end() const { return mValue.end(); }

    /**
     * Lookup up a style property by name.
     * @param key The name of the style property
     * @return The value of the style property or the null object if it is not found
     */
    Object at(const std::string& key) const;

    /**
     * Lookup the provenance path of a style property by name.
     * @param key The name of the style property
     * @return The path to the JSON content where this style property was defined.
     */
    std::string provenance(const std::string& key) const;

    /**
     * @return The path to the JSON content where this style was defined.
     */
    std::string styleProvenance() const;

    /**
     * @return The number of defined style properties in this style.
     */
    size_t size() const { return mValue.size(); }

    friend class StyleDefinition;

protected:
    void put(const std::string& key, const Object& value, const std::string& provenance);

private:
    std::map<std::string, Object> mValue;
    std::map<std::string, std::string> mProvenance;
    const std::string mStyleProvenance;
};


} // namespace apl

#endif //_APL_STYLED_H
