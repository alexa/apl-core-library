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

#ifndef _APL_EDIT_TEXT_H
#define _APL_EDIT_TEXT_H

#include <functional>
#include <string>

#include "apl/primitives/range.h"

namespace apl {
namespace sg {

/**
 * The submit callback should be executed by the view host in a thread-safe manner.
 */
using EditTextSubmitCallback = std::function<void()>;

/**
 * The text-changed callback should be executed by the view host in a thread-safe manner.
 * This should be executed each time the text is changed in the edit text.
 */
using EditTextChangedCallback = std::function<void(const std::string& text)>;

/**
 * The focus callback should be executed by the view host in a thread-safe manner.
 * This should be executed when the focus changes
 */
using EditTextFocusCallback = std::function<void(bool isFocused)>;

/**
 * The public interface to a single-line text editor
 *
 * This is an abstract class.  The view host should implement this class in a thread-safe manner.
 * These methods are intended to be used by the core engine and should not be called by the view
 * host.
 *
 * The callbacks must be executed on the core engine thread correctly protected against a different
 * thread entering the core engine at the same time.
 *
 * The EditText is may only be created by calling EditTextFactory::createEditText()
 *
 * As the user edits the text or if the text has to be adjusted to fit the maximum length or
 * valid characters list, the the mChangedCallback will be executed.  The callback should always
 * be run as a fresh entry point into Core.  That is, the EditText should put a block
 * on the dispatch queue to execute the callback instead of running immediately within the stack
 * of either the setText() method or original creation of the edit text.  The Edit Text Component
 * caches the modified text and uses it for all context updates,
 */
class EditText {
public:
    explicit EditText(EditTextSubmitCallback submitCallback,
                      EditTextChangedCallback changedCallback,
                      EditTextFocusCallback focusCallback)
        : mSubmitCallback(std::move(submitCallback)),
          mChangedCallback(std::move(changedCallback)),
          mFocusCallback(std::move(focusCallback))
    {}
    
    virtual ~EditText() = default;

    /**
     * Release this edit text and associated resources.  After this method is called the view host
     * should not respond to any further method calls and should not execute any callbacks.
     */
    virtual void release() = 0;

    /**
     * Set the focus state of the edit text.  An edit text that has focus should show the appropriate blinking cursor.
     * @param hasFocus True if the edit text has focus.
     */
    virtual void setFocus(bool hasFocus) = 0;

protected:
    EditTextSubmitCallback  mSubmitCallback;
    EditTextChangedCallback mChangedCallback;
    EditTextFocusCallback   mFocusCallback;
};

} // namespace sg
} // namespace apl

#endif // _APL_EDIT_TEXT_H
