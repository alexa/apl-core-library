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

#include "apl/content/viewport.h"

#include "apl/content/content.h"
#include "apl/content/metrics.h"

namespace apl {

Object makeViewport( const Metrics& metrics, const std::string& theme ) {
    auto viewport = std::make_shared<ObjectMap>();
    viewport->emplace("width", metrics.getWidth());
    viewport->emplace("height", metrics.getHeight());
    viewport->emplace("autoWidth", metrics.getAutoWidth());
    viewport->emplace("autoHeight", metrics.getAutoHeight());
    viewport->emplace("dpi", metrics.getDpi());
    viewport->emplace("shape", metrics.getShape());
    viewport->emplace("pixelWidth", metrics.getPixelWidth());
    viewport->emplace("pixelHeight", metrics.getPixelHeight());
    viewport->emplace("mode", metrics.getMode());
    viewport->emplace("theme", theme);
    return viewport;
}

} // namespace apl
