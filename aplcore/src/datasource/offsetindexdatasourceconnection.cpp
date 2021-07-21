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

#include <vector>

#include "apl/datasource/offsetindexdatasourceconnection.h"
#include "apl/primitives/object.h"
#include "apl/livedata/livearray.h"

using namespace apl;

OffsetIndexDataSourceConnection::OffsetIndexDataSourceConnection(
        std::weak_ptr<LiveArray> liveArray,
        size_t offset,
        size_t maxItems,
        size_t cacheChunkSize) :
        mMaxItems(maxItems),
        mOffset(offset),
        mLiveArray(std::move(liveArray)),
        mCacheChunkSize(cacheChunkSize) {}

bool
OffsetIndexDataSourceConnection::update(size_t index, const std::vector<Object>& data, bool replace) {
    LiveArrayPtr liveArray = mLiveArray.lock();
    if (!liveArray) return false;
    size_t incomingSize = data.size();

    auto lowerBound = mOffset;
    auto upperBound = mOffset + liveArray->size();

    // We don't allow gaps and repeat fetching is out of scope of basic implementation
    if (index > upperBound || index + incomingSize < lowerBound || data.empty()) {
        return false;
    }

    auto insertLeftCount = lowerBound > index ? lowerBound - index : 0;
    auto insertRightCount = index + incomingSize > upperBound ? index + incomingSize - upperBound : 0;
    auto updateCount = incomingSize - insertLeftCount - insertRightCount;

    if (updateCount > 0 && replace)
        liveArray->update(index > lowerBound ? index - lowerBound : 0,
                          data.begin() + insertLeftCount, data.begin() + insertLeftCount + updateCount);

    int sizeClip = 0;
    size_t remainingRight = mMaxItems - mOffset - liveArray->size();
    // Clip to mMaxItems
    if (insertRightCount > remainingRight) {
        sizeClip = insertRightCount - remainingRight;
    }
    if (insertRightCount)
        liveArray->insert(liveArray->size(), data.end() - insertRightCount, data.end() - sizeClip);

    if (insertLeftCount) {
        liveArray->insert(0, data.begin(), data.begin() + insertLeftCount);
    }

    mOffset = std::min(index, mOffset);
    return true;
}

bool
OffsetIndexDataSourceConnection::insert(size_t index, const Object& item) {
    LiveArrayPtr liveArray = mLiveArray.lock();
    if (!liveArray) return false;

    auto lowerBound = mOffset;
    auto upperBound = mOffset + liveArray->size();

    size_t idx = index;
    if (index < lowerBound) {
        idx++;
    }

    if (idx > upperBound + 1 || idx < lowerBound) {
        return false;
    }

    if (liveArray->insert(idx - mOffset, item)) {
        if (index < lowerBound) {
            mOffset--;
        } else {
            mMaxItems++;
        }
        return true;
    }

    return false;
}

bool
OffsetIndexDataSourceConnection::remove(size_t index) {
    LiveArrayPtr liveArray = mLiveArray.lock();
    if (!liveArray) return false;

    auto lowerBound = mOffset;
    auto upperBound = mOffset + liveArray->size();

    if (index > upperBound || index < lowerBound) {
        return false;
    }

    if (liveArray->remove(index - mOffset)) {
        mMaxItems--;
        return true;
    }

    return false;
}

bool
OffsetIndexDataSourceConnection::insert(size_t index, const std::vector<Object>& items) {
    LiveArrayPtr liveArray = mLiveArray.lock();
    if (!liveArray) return false;

    auto lowerBound = mOffset;
    auto upperBound = mOffset + liveArray->size();

    size_t idx = index;
    if (index < lowerBound) {
        idx++;
    }

    if (idx > upperBound + 1 || idx < lowerBound) {
        return false;
    }

    if (liveArray->insert(idx - mOffset, items.begin(), items.end())) {
        if (index < lowerBound) {
            mOffset -= items.size();
        } else {
            mMaxItems += items.size();
        }
        return true;
    }

    return false;
}

bool
OffsetIndexDataSourceConnection::remove(size_t index, size_t count) {
    LiveArrayPtr liveArray = mLiveArray.lock();
    if (!liveArray) return false;

    auto lowerBound = mOffset;
    auto upperBound = mOffset + liveArray->size();

    if (index > upperBound || index < lowerBound) {
        return false;
    }

    if (index + count > upperBound) count = upperBound - index;

    if (liveArray->remove(index - mOffset, count)) {
        mMaxItems -= count;
        return true;
    }

    return false;
}

void
OffsetIndexDataSourceConnection::ensure(size_t index) {
    LiveArrayPtr liveArray = mLiveArray.lock();
    if (!liveArray) return;

    size_t baseSize = liveArray->size();

    if (baseSize < mMaxItems && (int)index + mCacheChunkSize > baseSize) {
        size_t dsOffset = mOffset + baseSize;
        size_t toCache = std::min(mMaxItems - dsOffset, (size_t)mCacheChunkSize);

        if (toCache != 0) {
            fetch(dsOffset, toCache);
        }
    }

    if (baseSize && mOffset > 0 && (int)index - mCacheChunkSize < 0) {
        size_t dsOffset = mOffset > mCacheChunkSize ? mOffset - mCacheChunkSize : 0;
        size_t toCache = mOffset - dsOffset;

        if (toCache != 0) {
            fetch(dsOffset, toCache);
        }
    }
}

std::shared_ptr<LiveArray>
OffsetIndexDataSourceConnection::getLiveArray() {
    return mLiveArray.lock();
}

bool
OffsetIndexDataSourceConnection::overlaps(size_t index, size_t count) {
    LiveArrayPtr liveArray = mLiveArray.lock();
    if (!liveArray) return false;

    auto lowerBound = mOffset;
    auto upperBound = mOffset + liveArray->size();

    if (index > upperBound || index + count < lowerBound) {
        return false;
    }

    auto insertLeftCount = lowerBound > index ? lowerBound - index : 0;
    auto insertRightCount = index + count > upperBound ? index + count - upperBound : 0;
    return count - insertLeftCount - insertRightCount > 0;
}
