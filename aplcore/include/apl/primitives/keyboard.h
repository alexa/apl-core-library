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

#ifndef _APL_KEYBOARD_H
#define _APL_KEYBOARD_H

#include <utility>
#include <string>

#include "rapidjson/document.h"
#include "object.h"

namespace apl {

enum KeyHandlerType {
    /**
     * This view host has received a key press and should execute the key handlers
     * specified by the 'handleKeyDown' property, if any, on the focused component.
     */
    kKeyDown,

    /**
     * This view host has received a key release and should execute the key handlers
     * specified by the 'handleKeyUp' property, if any, on the focused component.
     */
    kKeyUp
};

/**
 * Keyboard state for a key press.
 */
class Keyboard {
public:

    static const Keyboard& BACK_KEY() {
        static auto kb = Keyboard("Back", "GoBack");
        return kb;
    }

    static const Keyboard& ENTER_KEY() {
        static auto kb = Keyboard("Enter", "Enter");
        return kb;
    }

    static const Keyboard& NUMPAD_ENTER_KEY() {
        static auto kb = Keyboard("NumpadEnter", "NumpadEnter");
        return kb;
    }

    static const Keyboard& TAB_KEY() {
        static auto kb = Keyboard("Tab", "Tab");
        return kb;
    }

    static const Keyboard& SHIFT_TAB_KEY() {
        static auto kb = Keyboard("Tab", "Tab").shift(true);
        return kb;
    }

    static const Keyboard& ARROW_UP_KEY() {
        static auto kb = Keyboard("ArrowUp", "ArrowUp");
        return kb;
    }

    static const Keyboard& ARROW_DOWN_KEY() {
        static auto kb = Keyboard("ArrowDown", "ArrowDown");
        return kb;
    }

    static const Keyboard& ARROW_RIGHT_KEY() {
        static auto kb = Keyboard("ArrowRight", "ArrowRight");
        return kb;
    }

    static const Keyboard& ARROW_LEFT_KEY() {
        static auto kb = Keyboard("ArrowLeft", "ArrowLeft");
        return kb;
    }

    static const Keyboard& PAGE_UP_KEY() {
        static auto kb = Keyboard("PageUp", "PageUp");
        return kb;
    }

    static const Keyboard& PAGE_DOWN_KEY() {
        static auto kb = Keyboard("PageDown", "PageDown");
        return kb;
    }

    static const Keyboard& HOME_KEY() {
        static auto kb = Keyboard("Home", "Home");
        return kb;
    }

    static const Keyboard& END_KEY() {
        static auto kb = Keyboard("End", "End");
        return kb;
    }

    /**
     * Creates a representation of a non-repeating key, without modifier keys.
     * @param code
     */
     Keyboard(std::string code, std::string key) : mCode(std::move(code)), mKey(std::move(key)) {}

    /**
     * @return The string representation of the physical key on keyboard.
     */
    const std::string& getCode() const { return mCode; }

    /**
     * The string representation of key pressed on the keyboard, taking into account modifier keys.
     */
    const std::string& getKey() const { return mKey; }


    /**
     * Set the key repeat state
     * @param repeat Repeat state.
     * @return This Keyboard.
     */
    Keyboard& repeat(bool repeat) {
        mRepeat = repeat;
        return *this;
    }

    /**
     * Set the Alt key state.
     * @param altKey The pressed state of the Alt key
     * @return This Keyboard.
     */
    Keyboard& alt(bool altKey) {
        mAltKey = altKey;
        return *this;
    }

    /**
     * Set the Ctrl key state.
     * @param CtrlKey The pressed state of the Ctrl key
     * @return This Keyboard.
     */
    Keyboard& ctrl(bool ctrlKey) {
        mCtrlKey = ctrlKey;
        return *this;
    }

    /**
     * Set the meta key state.
     * @param metaKey The pressed state of the Meta key
     * @return This Keyboard.
     */
    Keyboard& meta(bool metaKey) {
        mMetaKey = metaKey;
        return *this;
    }

    /**
     * Set the shift key state.
     * @param shiftKey The pressed state of the Shift key
     * @return This Keyboard.
     */
    Keyboard& shift(bool shiftKey) {
        mShiftKey = shiftKey;
        return *this;
    }


    /**
     * @return True if this key is being held down so it auto-repeats.
     */
    bool isRepeat() const { return mRepeat; }

    /**
     * @return True if the “alt” key was pressed when the event occurred. (“option” on OS X)
     */
    bool isAltKey() const { return mAltKey; }

    /**
     * @return True if the “control” key was pressed when the event occurred.
     */
    bool isCtrlKey() const { return mCtrlKey; }

    /**
     * @return True if the “meta” key was pressed when the event occurred. On Windows keyboards this is the “Windows” key. On Macintosh keyboards this is the “command” key.
     */
    bool isMetaKey() const { return mMetaKey; }

    /**
     * True if the “shift” key was pressed when the event occurred.
     */
    bool isShiftKey() const { return mShiftKey; }

    /**
     * True if the the key is reserved for future use by APL.
     */
    bool isReservedKey() const;

    /**
     * @return True if the key is used internally by APL but will not be passed to a key event handler
     */
    bool isIntrinsicKey() const;

    /**
    * Serialize into JSON format
    * @param allocator
    * @return The serialized rectangle
    */
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

    /**
    * Serialize into ObjectMap format
    * @param allocator
    * @return The serialized rectangle
    */
    std::shared_ptr<ObjectMap> serialize() const;

    /**
     * @return Standard comparison function. <0 if current object should be before other, 0 if equal,
     * >0 if should be after other.
     */
    int compare(const Keyboard& other) const;

    /**
     * @return Compare without repeat. <0 if current object should be before other, 0 if equal,
     * >0 if should be after other.
     */
    int compareWithoutRepeat(const Keyboard& other) const;

    /// Comparison operators
    bool operator==(const Keyboard& other) const { return this->compare(other) == 0; }
    bool operator!=(const Keyboard& other) const { return this->compare(other) != 0; }
    bool operator<(const Keyboard& other) const { return this->compare(other) < 0; }
    bool operator>(const Keyboard& other) const { return this->compare(other) > 0; }

    /**
     * Compare ONLY key value.
     * @param other The key to compare to.
     * @return True if key is the same, false otherwise.
     */
    bool sameKey(const Keyboard& other) const {
        return mKey == other.mKey;
    }

    /**
     * Key equals comparison that ignores repeat.
     * @param rhs The key to compare to.
     * @return True, if all properties are equal without repeat comparison.
     */
    bool keyEquals(const Keyboard& rhs) const {
        return compareWithoutRepeat(rhs) == 0;
    }

    /**
     * Comparator definition without repeat to use in maps.
     */
    struct comparatorWithoutRepeat {
        bool operator() (const Keyboard& lhs, const Keyboard& rhs) const
        {
            return lhs.compareWithoutRepeat(rhs) < 0;
        }
    };

    std::string toDebugString() const;

private:
    std::string mCode;
    std::string mKey;
    bool mRepeat = false;
    bool mAltKey = false;
    bool mCtrlKey = false;
    bool mMetaKey = false;
    bool mShiftKey = false;
};

}

#endif // _APL_KEYBOARD_H