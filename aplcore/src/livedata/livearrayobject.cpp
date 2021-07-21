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

#include "apl/livedata/livearrayobject.h"
#include "apl/livedata/livearray.h"
#include "apl/livedata/livearraychange.h"
#include "apl/engine/context.h"
#include "apl/livedata/livedatamanager.h"
#include "apl/livedata/layoutrebuilder.h"

namespace apl {

LiveArrayObject::LiveArrayObject(const LiveArrayPtr& liveArray, const ContextPtr& context, const std::string& key)
    : LiveDataObject(context, key),
      mLiveArray(liveArray)
{
    mToken = liveArray->addChangeCallback([this](const LiveArrayChange& change) {
        handleArrayMessage(change);
    });
}

LiveArrayObject::~LiveArrayObject() {
    mLiveArray->removeChangeCallback(mToken);
}

Object
LiveArrayObject::at(std::uint64_t index) const
{
    return mLiveArray->at(index);
}

std::uint64_t
LiveArrayObject::size() const
{
    return mLiveArray->size();
}

bool
LiveArrayObject::empty() const
{
    return mLiveArray->empty();
}

const std::vector<Object>&
LiveArrayObject::getArray() const
{
    return mLiveArray->getArray();
}

void
LiveArrayObject::accept(Visitor<Object>& visitor) const
{
    const auto& array = mLiveArray->getArray();
    visitor.push();
    for (auto it = array.begin(); !visitor.isAborted() && it != array.end(); it++)
        it->accept(visitor);
    visitor.pop();
}

void
LiveArrayObject::handleArrayMessage(const LiveArrayChange& change)
{
    // Ignore any commands once replaced has been set
    if (mReplaced)
        return;

    if (change.command() == LiveArrayChange::REPLACE) {
        mReplaced = true;
        mChanges.clear();
    }
    else {
        mChanges.push_back(change);
    }

    markDirty();
}

void
LiveArrayObject::flush() {
    LiveDataObject::flush();
    mChanges.clear();
}

/**
 * Return the index of the old item and a flag if that item has changed value.
 * The index is -1 if the item is completely new.
 * @param index The index in the new array.
 * @return
 */
std::pair<int, bool>
LiveArrayObject::newToOld(ObjectArray::size_type index)
{
    if (mReplaced)
        return {-1, false};

    bool changed = false;

    for (auto it = mChanges.rbegin() ; it != mChanges.rend() ; it++) {
        auto cmd = it->command();
        auto position = it->position();
        auto count = it->count();
        switch (cmd) {
            case LiveArrayChange::REMOVE:
                if (index >= position)
                    index += count;
                break;
            case LiveArrayChange::UPDATE:
                if (index >= position && index < position + count)
                    changed = true;
                break;
            case LiveArrayChange::INSERT:
                if (index >= position + count)
                    index -= count;
                else if (index >= position)
                    return {-1, false};
                break;
            default:
                break;
        }
    }

    return { index, changed };
}

} // namespace apl