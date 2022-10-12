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

#include "testedittext.h"

namespace apl {

TestEditText::TestEditText(sg::EditTextSubmitCallback submitCallback,
                           sg::EditTextChangedCallback changedCallback,
                           sg::EditTextFocusCallback focusCallback)
    : sg::EditText(std::move(submitCallback), std::move(changedCallback), std::move(focusCallback))
{
}

void
TestEditText::release()
{
    mReleased = true;
    mSubmitCallback = nullptr;
    mChangedCallback = nullptr;
    mFocusCallback = nullptr;
}

void
TestEditText::setFocus(bool hasFocus)
{
    if (mReleased)
        return;
}

// Debugging function - simulate the user typing
void
TestEditText::changeText(const std::string& updated)
{
    if (mChangedCallback)
        mChangedCallback(updated);
}

void
TestEditText::submit()
{
    if (mSubmitCallback)
        mSubmitCallback();
}

void
TestEditText::focus(bool focused)
{
    if (mFocusCallback)
        mFocusCallback(focused);
}

} // namespace apl