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

#ifndef APL_STICKYFUNCTIONS_H
#define APL_STICKYFUNCTIONS_H

#include "apl/common.h"
#include "apl/component/component.h"
#include "apl/primitives/point.h"

namespace apl {

/**
 * Helper functions for position: sticky
 */
namespace stickyfunctions {

/**
 * Handle when a components position type is set from 'sticky' to something else
 */
void restoreComponentInsets(const CoreComponentPtr &component);

/**
 * Calculate and apply the left/top offset for a component with position: sticky
 */
void updateStickyOffset(const CoreComponentPtr &component);

/**
 * @return The offset which should be applied to this component if it has position: sticky
 */
Point calculateStickyOffset(const CoreComponentPtr &component);

/**
 * Traverse up the ancestors and find the nearest horizontal and vertical scrollables if they exist
 */
std::pair<CoreComponentPtr, CoreComponentPtr>
getAncestorHorizontalAndVerticalScrollable(const CoreComponentPtr &component);

}  // namespace stickyfunctions

}  // namespace apl

#endif //APL_STICKYFUNCTIONS_H
