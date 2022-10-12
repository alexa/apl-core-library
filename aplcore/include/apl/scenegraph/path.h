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

#ifndef _APL_SG_PATH_H
#define _APL_SG_PATH_H

#include <string>
#include <rapidjson/document.h>

#include "apl/scenegraph/common.h"
#include "apl/component/textmeasurement.h"
#include "apl/primitives/rect.h"
#include "apl/primitives/roundedrect.h"
#include "apl/utils/counter.h"
#include "apl/utils/noncopyable.h"
#include "apl/utils/userdata.h"

namespace apl {
namespace sg {


class Path : public Counter<Path>,
             public UserData<Path>,
             public NonCopyable,
             public std::enable_shared_from_this<Path> {
public:
    enum Type {
        kRect,
        kRoundedRect,
        kGeneral,
        kFrame,
    };

    ~Path() = default;

    /**
     * @return The type of this node
     */
    Type type() const { return mType; }

    /**
     * @return True if the path has been modified.  Calling this clears the flag
     */
    bool modified() {
        auto result = mModified;
        mModified = false;
        return result;
    }

    /**
     * @return True if this path has nothing to draw
     */
    virtual bool empty() const = 0;

    /**
     * @return A human-readable debugging string
     */
    virtual std::string toDebugString() const = 0;

    /**
     * Check if two paths are equal.  Two paths are equal only if they are the same
     * type and have the same internal values.
     * @param lhs The first path.
     * @param rhs The second path.
     * @return True if the paths are equal.
     */
    friend bool operator==(const PathPtr& lhs, const PathPtr& rhs);
    friend bool operator!=(const PathPtr& lhs, const PathPtr& rhs) { return !(lhs == rhs); }

    /**
     * Serialize this path
     * @param allocator RapidJSON memory allocator
     * @return The serialized value
     */
    virtual rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const = 0;

protected:
    explicit Path(Type type) : mType(type) {}

    const Type mType;
    bool mModified = false;
};


#define PATH_SUBCLASS(TYPE_NAME, TYPE_ENUM)                 \
  public:                                                   \
    TYPE_NAME() : Path(TYPE_ENUM) {}                        \
    static const TYPE_NAME *cast(const Path *path) {        \
        assert(path != nullptr && path->type() == (TYPE_ENUM)); \
        return reinterpret_cast<const TYPE_NAME*>(path);    \
    }                                                       \
    static TYPE_NAME *cast(Path *path) {                    \
        assert(path != nullptr && path->type() == (TYPE_ENUM)); \
        return reinterpret_cast<TYPE_NAME*>(path);          \
    }                                                       \
    static TYPE_NAME *cast(const PathPtr& path) {           \
        return cast(path.get());                            \
    }                                                       \
    static bool is_type(const Path *path) { return path && path->type() == (TYPE_ENUM); } \
    static bool is_type(const PathPtr& path) { return is_type(path.get()); }          \
    std::string toDebugString() const override;             \
    bool empty() const override;                            \
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const override

/**
 * A rectangular path.
 *
 * Assume that rectangle is drawn counter-clockwise.
 */
class RectPath : public Path {
    PATH_SUBCLASS(RectPath, kRect);

    bool setRect(Rect rect);
    const Rect& getRect() const { return mRect; }

private:
    Rect mRect;
};

/**
 * A rounded-rectangle (a rectangle with optional radii on each corner).
 * The rounded-rectangle is drawn counter-clockwise.
 */
class RoundedRectPath : public Path {
    PATH_SUBCLASS(RoundedRectPath, kRoundedRect);

    bool setRoundedRect(const RoundedRect& roundedRect);
    const RoundedRect& getRoundedRect() const { return mRoundedRect; }

private:
    RoundedRect mRoundedRect;
};

/**
 * A RoundRectPath with an inset.
 */
class FramePath : public Path {
    PATH_SUBCLASS(FramePath, kFrame);

    bool setRoundedRect(const RoundedRect& roundedRect);
    const RoundedRect& getRoundedRect() const { return mRoundedRect; }

    bool setInset(float inset);
    float getInset() const { return mInset; }

private:
    RoundedRect mRoundedRect;
    float mInset = 0.0f;
};

/**
 * A path specified by the AVG path data string description
 */
class GeneralPath : public Path {
    PATH_SUBCLASS(GeneralPath, kGeneral);

    bool setPaths(const std::string value, const std::vector<float> points);
    const std::string& getValue() const { return mValue; }
    const std::vector<float>& getPoints() const { return mPoints; }

private:
    std::string mValue;
    std::vector<float> mPoints;
};


} // namespace sg
} // namespace apl

#endif // _APL_SG_PATH_H
