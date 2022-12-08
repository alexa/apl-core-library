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

#ifndef _APL_SG_PATHOP_H
#define _APL_SG_PATHOP_H

#include "apl/scenegraph/common.h"
#include "apl/graphic/graphicproperties.h"
#include "apl/scenegraph/paint.h"
#include "apl/utils/noncopyable.h"

namespace apl {
namespace sg {

enum FillType {
    kFillTypeEvenOdd,
    kFillTypeWinding
};

class PathOp;
using PathOpPtr = std::shared_ptr<PathOp>;

class PathOp : public NonCopyable {
public:
    enum Type {
        kStroke,
        kFill
    };

    explicit PathOp(Type type) : type(type) {}

    virtual ~PathOp() = default;
    virtual std::string toDebugString() const = 0;
    virtual bool visible() const { return paint && paint->visible(); }
    virtual float maxWidth() const { return 0; }

    const Type type;
    PaintPtr   paint;
    PathOpPtr  nextSibling;

    virtual rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;
};

#define PATH_OP_SUBCLASS(TYPE_NAME, TYPE_ENUM)                 \
  public:                                                   \
    TYPE_NAME() : PathOp(TYPE_ENUM) {}                        \
    static const TYPE_NAME *cast(const PathOp *pathOp) {                    \
        assert(pathOp != nullptr && pathOp->type == TYPE_ENUM); \
        return reinterpret_cast<const TYPE_NAME*>(pathOp);          \
    }                                                       \
    static TYPE_NAME *cast(PathOp *pathOp) {                    \
        assert(pathOp != nullptr && pathOp->type == TYPE_ENUM); \
        return reinterpret_cast<TYPE_NAME*>(pathOp);          \
    }                                                       \
    static TYPE_NAME *cast(const PathOpPtr& pathOp) {           \
        return cast(pathOp.get());                            \
    }                  \
    static std::shared_ptr<TYPE_NAME> castptr(const PathOpPtr& pathOp) {           \
        assert(pathOp != nullptr && pathOp->type == TYPE_ENUM); \
        return std::static_pointer_cast<TYPE_NAME>(pathOp);                        \
    }                  \
    static bool is_type(const PathOp *pathOp) { return pathOp && pathOp->type == TYPE_ENUM; } \
    static bool is_type(const PathOpPtr& pathOp) { return is_type(pathOp.get()); }          \
    std::string toDebugString() const override;                \
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const override


class StrokePathOp : public PathOp {
    PATH_OP_SUBCLASS(StrokePathOp, kStroke);

    bool visible() const override { return strokeWidth > 0 && PathOp::visible(); }
    float maxWidth() const override {
        // Miter joins stick out by a multiple of the line width
        if (lineJoin == kGraphicLineJoinMiter)
            return miterLimit * strokeWidth;
        return strokeWidth;
    }

    float strokeWidth = 1.0;
    float miterLimit = 4.0;
    float pathLength = 0.0;
    float dashOffset = 0.0;
    GraphicLineCap lineCap = kGraphicLineCapButt;
    GraphicLineJoin lineJoin = kGraphicLineJoinMiter;
    std::vector<float> dashes;  // Always should be an even number
};

class FillPathOp : public PathOp {
    PATH_OP_SUBCLASS(FillPathOp, kFill);

    FillType fillType = kFillTypeEvenOdd;
};

} // namespace sg
} // namespace apl

#endif // _APL_SG_PATHOP_H
