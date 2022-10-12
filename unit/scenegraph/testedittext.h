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

#ifndef _APL_TEST_EDIT_TEXT_H
#define _APL_TEST_EDIT_TEXT_H

#include <iostream>
#include <vector>

#include "apl/scenegraph/common.h"
#include "apl/scenegraph/edittext.h"
#include "apl/scenegraph/edittextfactory.h"

namespace apl {

class TestEditText : public sg::EditText {
public:
    TestEditText(sg::EditTextSubmitCallback submitCallback,
                 sg::EditTextChangedCallback changedCallback,
                 sg::EditTextFocusCallback focusCallback);

    // ************* Overrides of standard EditText methods *****************

    void release() override;
    void setFocus(bool hasFocus) override;

    // *********** Debugging functions ***********

    void changeText(const std::string& updated);
    void submit();
    void focus(bool focused);

private:
    bool mReleased = false;
};


class TestEditTextFactory : public sg::EditTextFactory {
public:
    sg::EditTextPtr createEditText(sg::EditTextSubmitCallback submitCallback,
                                   sg::EditTextChangedCallback changedCallback,
                                   sg::EditTextFocusCallback focusCallback) override {
        auto result =
            std::make_shared<TestEditText>(submitCallback, changedCallback, focusCallback);
        mItems.push_back(result);
        return result;
    }

    // For debugging, we just change ALL of the attached edit text items
    void changeText(const std::string& updated) {
        for (auto it = mItems.begin(); it != mItems.end();) {
            auto ptr = it->lock();
            if (ptr) {
                ptr->changeText(updated);
                it++;
            }
            else {
                it = mItems.erase(it);
            }
        }
    }

    void submit() {
        for (auto it = mItems.begin() ; it != mItems.end() ; ) {
            auto ptr = it->lock();
            if (ptr) {
                ptr->submit();
                it++;
            }
            else {
                it = mItems.erase(it);
            }
        }
    }

private:
    std::vector<std::weak_ptr<TestEditText>> mItems;
};

} // namespace apl

#endif // _APL_TEST_EDIT_TEXT_H
