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
 */

#ifndef _APL_SG_PATH_BOUNDS_H
#define _APL_SG_PATH_BOUNDS_H

#include "apl/primitives/rect.h"
#include "apl/primitives/transform2d.h"

namespace apl {
namespace sg {

/**
 * Given a series of commands (M=Move, L=LineTo, Q=QuadraticTo, C=CubicTo, Z=Close path
 * and a set of points used by those commands, calculate the bounding box of the path.
 * If the path has no line segments, the bounding box will be an empty Rectangle.
 *
 * @param commands The path drawing commands
 * @param points A vector of points used by the drawing commands.  Each command uses 0, 2, 4, or 6
 *               points.
 * @return A rectangle that just encloses the path.
 */
Rect calculatePathBounds(const std::string& commands, const std::vector<float>& points);

/**
 * Given a series of commands (M=Move, L=LineTo, Q=QuadraticTo, C=CubicTo, Z=Close path
 * and a set of points used by those commands, calculate the bounding box of the path.
 * If the path has no line segments, the bounding box will be an empty Rectangle.
 * The transform is applied to each pair of points before the bounds are calculated.
 *
 * @param transform A transform to be pre-applied against the points.
 * @param commands The path drawing commands
 * @param points A vector of points used by the drawing commands.  Each command uses 0, 2, 4, or 6
 *               points.
 * @return A rectangle that just encloses the path.
 */
Rect calculatePathBounds(const Transform2D& transform,
                         const std::string& commands,
                         const std::vector<float>&points);

} // namespace sg
} // namespace apl

#endif //_APL_SG_PATH_BOUNDS_H
