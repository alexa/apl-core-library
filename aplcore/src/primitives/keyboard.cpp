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

#include "apl/primitives/keyboard.h"

namespace apl {

static std::set<std::string> sReservedKeys {
    Keyboard::BACK_KEY().getKey(),
    Keyboard::PAGE_UP_KEY().getKey(),
    Keyboard::PAGE_DOWN_KEY().getKey(),
    Keyboard::HOME_KEY().getKey(),
    Keyboard::END_KEY().getKey(),
};

static std::set<std::string> sIntrinsicKeys {
    Keyboard::ENTER_KEY().getKey(),
    Keyboard::NUMPAD_ENTER_KEY().getKey()
};

bool
Keyboard::isReservedKey() const {
    return sReservedKeys.find(mKey) != sReservedKeys.end();
}

bool
Keyboard::isIntrinsicKey() const {
    return sIntrinsicKeys.find(mKey) != sIntrinsicKeys.end();
}

int
Keyboard::compare(const Keyboard& other) const {
    // Virtual ordering. Comparing in the order: key, code, alt, ctrl, meta, shift, repeat
    auto result = compareWithoutRepeat(other);
    if (result != 0) {
        return result;
    }

    if (mRepeat != other.mRepeat) {
        return other.mRepeat ? 1 : -1;
    }

    return 0;
}

int
Keyboard::compareWithoutRepeat(const Keyboard& other) const {

    if (mKey != other.mKey) {
        return mKey.compare(other.mKey);
    }

    if (mCode != other.mCode) {
        return mCode.compare(other.mCode);
    }

    if (mAltKey != other.mAltKey) {
        return other.mAltKey ? 1 : -1;
    }

    if (mCtrlKey != other.mCtrlKey) {
        return other.mCtrlKey ? 1 : -1;
    }

    if (mMetaKey != other.mMetaKey) {
        return other.mMetaKey ? 1 : -1;
    }

    if (mShiftKey != other.mShiftKey) {
        return other.mShiftKey ? 1 : -1;
    }

    return 0;
}

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

std::string
Keyboard::toDebugString() const {
    return "Keyboard<code=" + mCode +
        ", key=" + mKey +
        ", repeat=" + std::to_string(mRepeat) +
        ", altKey=" + std::to_string(mAltKey) +
        ", ctrlKey=" + std::to_string(mCtrlKey) +
        ", metaKey=" + std::to_string(mMetaKey) +
        ", shiftKey=" + std::to_string(mShiftKey) + ">";
}

} // namespace apl
