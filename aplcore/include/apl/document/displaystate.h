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

#ifndef _APL_DISPLAY_STATE_H
#define _APL_DISPLAY_STATE_H

#include <string>

#include "apl/utils/bimap.h"

namespace apl {

/**
 * Describes the visibility of the document on the display, so that documents
 * can be frugal with system resources (e.g. limit animations when not in the
 * foreground)
 */
enum DisplayState {
    // Not visible
    kDisplayStateHidden,
    // Neither hidden nor foreground
    kDisplayStateBackground,
    // Visible and at the front
    kDisplayStateForeground
};

/**
 * Sets a default display state in case the view host does not support this
 * feature. Foreground is the most reasonable default because prior to the
 * introduction of display state, that is what was implicitly assumed.
 */
static const DisplayState DEFAULT_DISPLAY_STATE = kDisplayStateForeground;

extern Bimap<int, std::string> sDisplayStateMap;

}  // namespace apl

#endif //_APL_DISPLAY_STATE_H
