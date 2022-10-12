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

#ifndef _APL_EDIT_TEXT_FACTORY_H
#define _APL_EDIT_TEXT_FACTORY_H

#include "apl/scenegraph/edittext.h"

namespace apl {
namespace sg {

class EditTextFactory {
public:
    virtual ~EditTextFactory() = default;

    virtual sg::EditTextPtr createEditText(EditTextSubmitCallback submitCallback,
                                           EditTextChangedCallback changedCallback,
                                           EditTextFocusCallback focusCallback) = 0;
};

} // namespace sg
} // namespace apl

#endif // _APL_EDIT_TEXT_FACTORY_H
