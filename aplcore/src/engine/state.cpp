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

#include "apl/engine/state.h"
#include "apl/engine/context.h"
#include "apl/utils/bimap.h"

namespace apl {

Bimap<StateProperty, std::string> sStateBimap = {
    {kStatePressed, "pressed"},
    {kStateDisabled, "disabled"},
    {kStateFocused, "focused"},
    {kStateChecked, "checked"},
    {kStateKaraoke, "karaoke"},
    {kStateKaraokeTarget, "karaokeTarget"},
    {kStateHover, "hover"}
};

StateProperty
State::stringToState(const std::string& name)
{
    return sStateBimap.get(name, static_cast<StateProperty>(-1));
}

ContextPtr
State::extend(const ContextPtr& context) const
{
    auto c = Context::createFromParent(context);
    auto map = std::make_shared<ObjectMap>();
    for (auto& m : sStateBimap)
        map->emplace(m.second, mStateMap[m.first]);
    c->putConstant("state", map);
    return c;
}

bool
State::operator< (const State& other) const
{
    for (auto& m : sStateBimap) {
        bool left = mStateMap.at(m.first);
        bool right = other.mStateMap.at(m.first);
        if (left != right)
            return right;
    }

    return false;
}

streamer&
operator<<(streamer& os, const State& state)
{
    os << "state< ";
    for (auto& m : sStateBimap) {
        if (state.mStateMap.at(m.first))
            os << m.second << " ";
    }
    os << ">";
    return os;
}

} // namespace apl