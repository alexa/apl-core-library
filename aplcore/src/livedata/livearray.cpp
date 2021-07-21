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

#include "apl/livedata/livearray.h"
#include "apl/livedata/livearraychange.h"

namespace apl {

void
LiveArray::clear()
{
    mArray.clear();
    broadcast(LiveArrayChange::replace());
}

bool
LiveArray::insert( size_type position, const Object& value )
{
    if (position > mArray.size())
        return false;

    mArray.insert(mArray.begin() + position, value);
    broadcast(LiveArrayChange::insert(position, 1));
    return true;
}

bool
LiveArray::insert( size_type position, Object && value )
{
    if (position > mArray.size())
        return false;

    mArray.insert(mArray.begin() + position, std::move(value));
    broadcast(LiveArrayChange::insert(position, 1));
    return true;
}

bool
LiveArray::remove( size_type position, size_type count )
{
    if (count > mArray.size() || position > mArray.size() - count)
        return false;

    mArray.erase(mArray.begin() + position, mArray.begin() + position + count);

    // Tell all trackers an item has been removed
    // If all items have been removed, just mark the array as replaced
    bool isReplaced = (mArray.size() == 0);
    broadcast(isReplaced ? LiveArrayChange::replace() : LiveArrayChange::remove(position, count));
    return true;
}

bool
LiveArray::update( size_type position, const Object& value )
{
    if (position >= mArray.size())
        return false;

    mArray[position] = value;
    broadcast(LiveArrayChange::update(position, 1));
    return true;
}

bool
LiveArray::update( size_type position, Object&& value )
{
    if (position >= mArray.size())
        return false;

    mArray[position] = std::move(value);
    broadcast(LiveArrayChange::update(position, 1));
    return true;
}

void
LiveArray::push_back( const Object& value )
{
    auto position = mArray.size();
    mArray.insert(mArray.begin() + position, value);
    broadcast(LiveArrayChange::insert(position, 1));
}

void
LiveArray::push_back( Object&& value )
{
    auto position = mArray.size();
    mArray.insert(mArray.begin() + position, std::move(value));
    broadcast(LiveArrayChange::insert(position, 1));
}

void
LiveArray::broadcast(const LiveArrayChange& command)
{
    for (const auto& m: mChangeCallbacks)
        m.second(command);
}

void
LiveArray::broadcastInsert(size_type position, size_type count)
{
    broadcast(LiveArrayChange::insert(position, count));
}

void
LiveArray::broadcastUpdate(size_type position, size_type count)
{
    broadcast(LiveArrayChange::update(position, count));
}

} // namespace apl