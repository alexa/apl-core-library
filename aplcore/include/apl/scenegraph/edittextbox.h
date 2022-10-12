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

#ifndef _APL_EDIT_TEXT_BOX_H
#define _APL_EDIT_TEXT_BOX_H

#include <string>

#include "apl/primitives/size.h"

namespace apl {
namespace sg {

/**
 * Subclass this in your view host and use it to return sizing information for
 * a box of text that holds a known number of characters.
 */
class EditTextBox {
public:
    virtual ~EditTextBox() = default;

    /**
     * @return The size of the edit text box
     */
    virtual Size getSize() const = 0;

    /**
     * @return The baseline of the text (measured from the top of the edit text box
     */
    virtual float getBaseline() const = 0;
};

} // namespace sg
} // namespace apl

#endif // _APL_EDIT_TEXT_BOX_H
