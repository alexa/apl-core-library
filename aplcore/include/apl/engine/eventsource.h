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

#ifndef _APL_EVENT_SOURCE_H
#define _APL_EVENT_SOURCE_H

#include "apl/component/component.h"

namespace apl {

class EventSource {
public:
    EventSource(const std::string& type, const std::string& handler, const std::string& id, const Object& value) :
        mType(type), mHandler(handler), mId(id), mValue(value) {};

    const std::string& getType() const { return mType; }

    const std::string& getHandler() const { return mHandler; }

    const std::string& getId() const { return mId; }

    const Object& getValue() const { return mValue; }

    Object toObject() const {
        auto ret = std::make_shared<std::map<std::string, Object>>();

        ret->emplace("type", Object(getType()));
        ret->emplace("handler", Object(getHandler()));
        ret->emplace("id", Object(getId()));
        ret->emplace("value", getValue());

        return Object(ret);
    }

private:
    const std::string&  mType;
    std::string mHandler;
    std::string mId;
    Object mValue;
};

}  // namespace apl

#endif //_APL_EVENT_SOURCE_H
