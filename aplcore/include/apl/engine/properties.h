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
 * Property bag
 *
 * These are the JSON properties that are passed when inflating a component. For example,
 *
 * {
 *   "type": "Image",
 *   "width": "100%",
 *   "height": "${viewport.height * 0.4}",
 *   "source": "http://xxx.yy"
 * }
 *
 * In this case, the properties are ["type", "width", "height", "source"].
 * The values are generally UNPROCESSED objects (no data binding, no casting yet).
 */

#ifndef _APL_PROPERTIES_H
#define _APL_PROPERTIES_H

#include "apl/primitives/object.h"
#include "parameterarray.h"

namespace apl {

class Properties {
public:
    Properties() {};
    explicit Properties(const Object& item) { emplace(item); }

    // These methods apply data-binding and extract the value or a default value
    std::string asLabel(const Context& context, const char *name);
    std::string asString(const Context& context, const char *name, const char *defvalue);
    bool asBoolean(const Context& context, const char *name, bool defvalue);
    double asNumber(const Context& context, const char *name, double defvalue);
    Dimension asAbsoluteDimension(const Context& context, const char *name, double defvalue);

    void emplace(const Object& item);
    void emplace(const std::string& name, const Object& value) { mProperties.emplace(name, value); }

    void addToContext(const ContextPtr &context, const Parameter &parameter, bool userWriteable);

    ObjectMap::const_iterator find(const char *name) const { return mProperties.find(name); }
    ObjectMap::const_iterator find(const std::string& name) const { return mProperties.find(name); }
    ObjectMap::const_iterator find(const std::vector<std::string>& names) const {
        for (const auto& name : names) {
            auto it = mProperties.find(name);
            if (it != mProperties.end())
                return it;
        }
        return mProperties.end();
    }

    ObjectMap::const_iterator begin() const { return mProperties.begin(); }
    ObjectMap::const_iterator end() const { return mProperties.end(); }

private:
    ObjectMap mProperties;
};

} // namespace apl

#endif // _APL_PROPERTIES_H
