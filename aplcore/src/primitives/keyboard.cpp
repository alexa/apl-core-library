/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "apl/primitives/keyboard.h"

namespace apl {

static std::set<std::string> sReservedKeys {
    Keyboard::BACK_KEY().getKey(),
    Keyboard::ENTER_KEY().getKey(),
    Keyboard::ARROW_UP_KEY().getKey(),
    Keyboard::ARROW_DOWN_KEY().getKey(),
    Keyboard::ARROW_LEFT_KEY().getKey(),
    Keyboard::ARROW_RIGHT_KEY().getKey(),
    Keyboard::PAGE_UP_KEY().getKey(),
    Keyboard::PAGE_DOWN_KEY().getKey(),
    Keyboard::HOME_KEY().getKey(),
    Keyboard::END_KEY().getKey(),
    Keyboard::TAB_KEY().getKey(),
    Keyboard::SHIFT_TAB_KEY().getKey(),
};

bool
Keyboard::isReservedKey() const {
    return sReservedKeys.find(mKey) != sReservedKeys.end();
};

rapidjson::Value
Keyboard::serialize(rapidjson::Document::AllocatorType& allocator) const {
    rapidjson::Value v(rapidjson::kArrayType);
    v.AddMember("code", rapidjson::Value(mCode.c_str(), allocator).Move(), allocator);
    v.AddMember("key", rapidjson::Value(mKey.c_str(), allocator).Move(), allocator);
    v.AddMember("repeat", mRepeat, allocator);
    v.AddMember("altKey", mAltKey, allocator);
    v.AddMember("ctrlKey", mCtrlKey, allocator);
    v.AddMember("metaKey", mMetaKey, allocator);
    v.AddMember("shiftKey", mShiftKey, allocator);
    return v;
}

std::shared_ptr<ObjectMap>
Keyboard::serialize() const {
    auto map = std::make_shared<ObjectMap>();
    map->emplace("code", mCode);
    map->emplace("key", mKey);
    map->emplace("repeat", mRepeat);
    map->emplace("altKey", mAltKey);
    map->emplace("ctrlKey", mCtrlKey);
    map->emplace("metaKey", mMetaKey);
    map->emplace("shiftKey", mShiftKey);
    return map;
}

} // namespace apl