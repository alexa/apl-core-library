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

#ifndef _APL_VIEWPORT_H
#define _APL_VIEWPORT_H

#include "apl/primitives/object.h"

namespace apl {

class Metrics;
class Content;

/**
 * Construct a viewport data-binding object out of metrics and user-defined content
 * @param metrics The metrics to use
 * @param theme The current theme assigned by the document
 * @return The data-binding object to use as "viewport"
 */
extern Object makeViewport( const Metrics& metrics, const std::string& theme="dark" );

} // namespace apl

#endif // _APL_VIEWPORT_H
