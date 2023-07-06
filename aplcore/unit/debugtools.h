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

#ifndef _APL_DEBUG_TOOLS_H
#define _APL_DEBUG_TOOLS_H

#include "apl/apl.h"

namespace apl {

/**
 * Convenience method for dumping the visual hierarchy.  Each component in the hierarchy will
 * always report type, unique id, and any user-assigned ID.  Pass additional string arguments
 * to specify properties you'd like to display.  In addition to the standard component properties,
 * you may also pass "pseudo" properties such as "__inherit".  Refer to the pseudo-property
 * list for valid pseudo properties.
 *
 * For example, to check the actual drawing bounds of components clipped to their parents, try:
 *
 *    dumpHierarchy( component, {"__actual", "_bounds", "__inherit"} );
 *
 * @tparam Args
 * @param component
 * @param args
 */
void dumpHierarchy(const ComponentPtr& component, std::initializer_list<std::string> args);

/**
 * Convenience method for visualizing the visual hierarchy
 * @param context
 */
void dumpContext(const ContextPtr& context, int indent = 0);

/**
 * Convenience method for displaying the Yoga hierarchy
 * @param component
 */
void dumpYoga(const ComponentPtr& component);

/**
 * Convenience method for dumping the current state of layouts
 * @param component
 */
void dumpLayout(const ComponentPtr& component);


} // namespace apl

#endif // _APL_DEBUG_TOOLS_H
