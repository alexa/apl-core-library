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

#include "apl/animation/easing.h"
#include "apl/graphic/graphicfilter.h"
#include "apl/primitives/filter.h"
#include "apl/primitives/gradient.h"
#include "apl/primitives/mediasource.h"
#include "apl/primitives/radii.h"
#include "apl/primitives/rect.h"
#include "apl/primitives/styledtext.h"
#include "apl/primitives/transform2d.h"

#include "apl/primitives/objectdata.h"

namespace apl {

/****************************************************************************/

template<> const Object::ObjectType DirectObjectData<Filter>::sType = Object::kFilterType;
template<> const Object::ObjectType DirectObjectData<GraphicFilter>::sType = Object::kGraphicFilterType;
template<> const Object::ObjectType DirectObjectData<Gradient>::sType = Object::kGradientType;
template<> const Object::ObjectType DirectObjectData<MediaSource>::sType = Object::kMediaSourceType;
template<> const Object::ObjectType DirectObjectData<Rect>::sType = Object::kRectType;
template<> const Object::ObjectType DirectObjectData<Radii>::sType = Object::kRadiiType;
template<> const Object::ObjectType DirectObjectData<Transform2D>::sType = Object::kTransform2DType;
template<> const Object::ObjectType DirectObjectData<StyledText>::sType = Object::kStyledTextType;

} // namespace apl
