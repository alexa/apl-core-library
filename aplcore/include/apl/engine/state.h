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

#ifndef _APL_STATE_H
#define _APL_STATE_H

#include <memory>
#include <vector>

#include "apl/common.h"
#include "apl/utils/streamer.h"
#include "apl/utils/bimap.h"

namespace apl {

class Context;

/**
 * For now we assume only these keys are allowed.  In the future we should
 * allow custom states.
 */
enum StateProperty {
    kStatePressed = 0,
    kStateDisabled = 1,
    kStateFocused = 2,
    kStateChecked = 3,
    kStateKaraoke = 4,
    kStateKaraokeTarget = 5,
    kStateHover = 6,

    kStatePropertyCount
};

extern Bimap<StateProperty, std::string> sStateBimap;

class State {
public:
    /**
     * Convert from a string to a named state.
     * @param name The string
     * @return The state or -1 if the state does not exist.
     */
    static StateProperty stringToState(const std::string& name);

    /**
     * Construct a state object.  All properties are set to false.
     * @param disabled The setting for the disabled property.
     */
    State() : mStateMap(kStatePropertyCount, false) {}

    /**
     * Constructor that takes a variable number of arguments.
     * Used to initialize the state to a random set of
     * @tparam Args
     * @param args
     */
    template<class... Args>
    State(Args... args) : mStateMap(kStatePropertyCount, false) {
        static const std::size_t len = sizeof...(Args);
        StateProperty v[len] = {args...};
        for (auto key : v)
            mStateMap[key] = true;
    }

    /**
     * Convenience method for intitializing a state with properties
     * @param property The property to set to true
     * @return This state, for chaining
     */
    State& emplace(StateProperty property) {
        mStateMap[property] = true;
        return *this;
    }

    /**
     * Update a state property to a new value.
     * @param property The property to change.
     * @param value The value to set.
     * @return True if the property changed.
     */
    bool set(StateProperty property, bool value) {
        bool result = value != mStateMap[property];
        mStateMap[property] = value;
        return result;
    }

    /**
     * Get the setting of a state property.
     * @param property The property to retrieve.
     * @return The value of the state property (true or false).
     */
    bool get(StateProperty property) const { return mStateMap[property];}

    /**
     * Toggle the setting of a state property.
     * @param property The property to toggle between true and false.
     */
    void toggle(StateProperty property) { mStateMap[property] = !mStateMap[property]; }

    /**
     * Extend the context with state information (in the "state" property) and return
     * a new context. Used during style evaluation.
     * @param context The context to extend.
     * @return A new context with the additional "state" property.
     */
    ContextPtr extend(const ContextPtr& context) const;

    /**
     * Create a relative ordering between states.  Useful for storing in an ordered set or map.
     * @param other The state to compare against.
     * @return True or false depending on relative ordering.
     */
    bool operator< (const State& other) const;

    friend streamer& operator<<(streamer& os, const State& state);

private:
    std::vector<bool> mStateMap;
};

}  // namespace apl

#endif // _APL_STATE_H