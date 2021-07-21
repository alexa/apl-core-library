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

#ifndef _APL_LIVE_ARRAY_CHANGE_H
#define _APL_LIVE_ARRAY_CHANGE_H

#include "apl/common.h"
#include "apl/primitives/object.h"

namespace apl {

/**
 * Represents a single change to a LiveArray.
 */
class LiveArrayChange {
public:
    using size_type = ObjectArray::size_type;

    enum Command {
        INSERT,
        REMOVE,
        UPDATE,
        REPLACE
    };

    static LiveArrayChange insert(size_type position, size_type count) { return {INSERT, position, count}; }
    static LiveArrayChange remove(size_type position, size_type count) { return {REMOVE, position, count}; }
    static LiveArrayChange update(size_type position, size_type count) { return {UPDATE, position, count}; }
    static LiveArrayChange replace() { return {REPLACE, 0, 0}; }

    Command command() const { return mCommand; }
    size_type position() const { return mPosition; }
    size_type count() const { return mCount; }

private:
    LiveArrayChange(Command command, size_type position, size_type count)
        : mCommand(command), mPosition(position), mCount(count) {}

private:
    Command mCommand;
    size_type mPosition;
    size_type mCount;
};

} // namespace apl

#endif // _APL_LIVE_ARRAY_CHANGE_H
