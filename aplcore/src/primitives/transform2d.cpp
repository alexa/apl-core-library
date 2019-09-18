/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <map>

#include "apl/primitives/transform2d.h"

namespace apl {

Bimap<TransformType, std::string> sTransformTypeMap = {
    { kTransformTypeRotate, "rotate" },
    { kTransformTypeScaleX, "scaleX" },
    { kTransformTypeScaleY, "scaleY" },
    { kTransformTypeScale, "scale" },
    { kTransformTypeSkewX, "skewX" },
    { kTransformTypeSkewY, "skewY" },
    { kTransformTypeTranslateX, "translateX" },
    { kTransformTypeTranslateY, "translateY" },
};

streamer&
operator<<(streamer& os, const Transform2D& transform)
{
    for (int i = 0 ; i < 6 ; i++) {
        os << transform.mData[i];
        if (i != 5) os << ", ";
    }
    return os;
}

}  // namespace apl
