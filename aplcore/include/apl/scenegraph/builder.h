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

#ifndef _APL_SG_BUILDER_H
#define _APL_SG_BUILDER_H

#include "apl/common.h"
#include "apl/scenegraph/accessibility.h"
#include "apl/scenegraph/filter.h"
#include "apl/scenegraph/layer.h"
#include "apl/scenegraph/node.h"
#include "apl/scenegraph/path.h"
#include "apl/scenegraph/pathop.h"

namespace apl {
namespace sg {


class stroke {
public:
    stroke(PaintPtr ptr);
    stroke(std::shared_ptr<StrokePathOp> op);

    stroke& strokeWidth(float value);
    stroke& miterLimit(float value);
    stroke& pathLength(float value);
    stroke& dashOffset(float value);
    stroke& lineCap(GraphicLineCap value);
    stroke& lineJoin(GraphicLineJoin value);
    stroke& dashes(const Object& value);

    PathOpPtr get() { return mStroke; }

private:
    std::shared_ptr<StrokePathOp> mStroke;
};


LayerPtr layer(const std::string& name, Rect bounds, float opacity, Transform2D transform);

NodePtr draw(PathPtr path, PathOpPtr op);
NodePtr text(TextLayoutPtr textLayout, PathOpPtr op);
NodePtr text(TextLayoutPtr textLayout, PathOpPtr op, Range range);
NodePtr editText(EditTextPtr editText, EditTextBoxPtr editTextBox,
                 EditTextConfigPtr editTextConfig, const std::string& text);
NodePtr transform(Transform2D transform, const NodePtr& child);
NodePtr transform(Point offset, const NodePtr& child);
NodePtr transform(const Object& object, const NodePtr& child);
NodePtr transform();
NodePtr clip(PathPtr path, const NodePtr& child);
NodePtr opacity(float opacity, const NodePtr& child);
NodePtr opacity(const Object& object, const NodePtr& child);
NodePtr image(FilterPtr image, Rect target, Rect source);
NodePtr video(MediaPlayerPtr player, Rect target, VideoScale scale);
NodePtr shadowNode(ShadowPtr shadow, const NodePtr& child);

PathPtr path(Rect rect);
PathPtr path(Rect rect, float radius);
PathPtr path(Rect rect, Radii radii);
PathPtr path(RoundedRect roundedRect);
PathPtr path(RoundedRect roundedRect, float inset);
PathPtr path(const std::string& path);

PaintPtr paint(Color color, float opacity=1.0f);
PaintPtr paint(const Gradient& gradient, float opacity=1.0f, Transform2D transform=Transform2D());
PaintPtr paint(const GraphicPatternPtr& pattern, float opacity=1.0f, Transform2D transform=Transform2D());
PaintPtr paint(const Object& object, float opacity=1.0f, Transform2D transform=Transform2D());

PathOpPtr fill(PaintPtr paint, FillType fillType = kFillTypeEvenOdd);

AccessibilityPtr accessibility(CoreComponent& component);

ShadowPtr shadow(Color color, Point offset, float radius);

FilterPtr filter(MediaObjectPtr image);
FilterPtr blend(FilterPtr back, FilterPtr front, BlendMode blendMode);  // Align top-left
FilterPtr blur(FilterPtr filter, float radius);
FilterPtr grayscale(FilterPtr filter, float amount);
FilterPtr noise(FilterPtr filter, NoiseFilterKind kind, bool useColor, float sigma );
FilterPtr saturate(FilterPtr filter, float amount);
FilterPtr solid(PaintPtr paint);

} // namespace sg
} // namespace apl

#endif // _APL_SG_BUILDER_H
