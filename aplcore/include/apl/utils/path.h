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

#ifndef _APL_PATH_H
#define _APL_PATH_H

#include <string>

#include "streamer.h"

#include "apl/primitives/object.h"
#include "apl/utils/log.h"

namespace apl {

/**
 * The Path class is used for tracking resource, style, and component provenance.  In other words, where in the
 * original JSON document and packages a particular resource/style/component was inflated from.
 *
 * A Path looks like a variation of a slash-separated ("/") string containing a JSONPath-like representation
 * of where the component comes from.  We track this in a separate class because it allows a lightweight optimization
 * to ignore path data when it is not needed.
 */
class Path {
public:
    static const char *MAIN;

    /**
     * Default path construction.
     * @param base The package name or "_main".  If the base is empty, all path calculations will be disabled.
     */
    explicit Path(const std::string& base="") : mPath(base) {}

    /**
     * Add an object segment to the path
     * @param segment The name of the object.
     * @return A new Path object
     */
    Path addObject(const std::string& segment) const {
        if (mPath.empty())
            return *this;

        if (mPath.back() == '/')
            LOG(LogLevel::kError) << "Adding string segment to array path " << mPath << " - " << segment;

        return Path(mPath + '/' + segment);
    }

    Path addObject(const char *segment) const {
        return addObject(std::string(segment));
    }

    /**
     * Add an array segment to the path
     * @param segment The name of the array
     * @return
     */
    Path addArray(const std::string& segment) const {
        if (mPath.empty())
            return *this;

        if (mPath.back() == '/')
            LOG(LogLevel::kError) << "Adding array segment to array path " << mPath << " - " << segment;

        return Path(mPath + '/' + segment + '/');
    }

    Path addArray(const char *segment) const {
        return addArray(std::string(segment));
    }

    /**
     * Add a numeric segment to the path
     * @param index The index to add.
     * @return A new path object
     */
    Path addIndex(size_t index) const {
        if (mPath.empty())
            return *this;

        if (mPath.back() == '/')
            return Path(mPath + std::to_string(index));

        if (index != 0)
            LOG(LogLevel::kError) << "Expected zero index for '" << mPath << "'";

        return *this;
    }

    /**
     * Add a property by name to this path.  This method is the termination of the
     * templated method.
     * @param item The object containing the property.
     * @return The path
     */
    Path addProperty(const Object& item) const {
        return *this;
    }

    /**
     * Add a property by name to this path.
     * @tparam T The type of the property name.  Probably const char * or std::string.
     * @tparam Types The types of the additional property names.
     * @param item The item to search for a named property.
     * @param name The name of the property to add.
     * @param other Additional property names to search.
     * @return The path
     */
    template<typename T, typename... Types>
    Path addProperty(const Object& item, T name, Types... other) const {
        if (mPath.empty())
            return *this;

        if (item.isMap() && item.has(name)) {
            // If it is an array we append a '/' to signal downstream that we expect an index
            if (item.get(name).isArray())
                return addArray(name);

            return addObject(name);
        }

        return addProperty(item, other...);
    }
    /**
     * @return This path represented as a string
     */
    std::string toString() const { return mPath; }

    /**
     * @return True if this path is empty
     */
    bool empty() const { return mPath.empty(); }

    friend streamer& operator<<(streamer& os, const Path& p) { os << p.toString(); return os; }

private:
    std::string mPath;
};

} // namespace apl

#endif // _APL_PATH_H
