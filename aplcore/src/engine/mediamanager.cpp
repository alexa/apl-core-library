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

#include "apl/engine/mediamanager.h"
#include "apl/component/corecomponent.h"
#include "apl/component/mediacomponenttrait.h"
#include "apl/engine/rootcontextdata.h"

namespace apl {

MediaManager::MediaManager(const apl::RootContextData& core) : mCore(core) {}

size_t
MediaManager::registerComponentMedia(const CoreComponentPtr& component, const std::set<std::string>& sources)
{
    auto cid = component->getUniqueId();
    size_t requiredToBeLoaded = 0;
    mComponentToSource.erase(component);
    mPendingPerComponent.erase(cid);
    for (auto& source : sources) {
        mComponentToSource.emplace(component, source);
        if (mLoadedSources.count(source) <= 0) {
            requiredToBeLoaded++;
        }
    }

    if (requiredToBeLoaded > 0) {
        mPendingPerComponent[cid] = requiredToBeLoaded;
        mComponentSetDirty = true;
    }

    return requiredToBeLoaded;
}

void
MediaManager::processMediaRequests()
{
    if (!mComponentSetDirty)
        return;

    mComponentSetDirty = false;

    std::set<std::string> dedupedSources;
    for (auto& c2s : mComponentToSource) {
        dedupedSources.emplace(c2s.second);
    }

    std::set<std::string> diff;
    std::set_difference(
            dedupedSources.begin(), dedupedSources.end(),
            mRequestedSources.begin(), mRequestedSources.end(),
            std::inserter(diff, diff.begin()));

    if (diff.empty())
        return;

    for (const auto &s : diff)
        mRequestedSources.emplace(s);

    auto sourcesToRequest = std::make_shared<std::vector<Object>>();
    for (const auto& source : diff)
        sourcesToRequest->emplace_back(source);

    EventBag bag;
    bag.emplace(kEventPropertySource, sourcesToRequest);
    mCore.top()->getContext()->pushEvent(Event(kEventTypeMediaRequest, std::move(bag)));
}

void
MediaManager::mediaLoaded(const std::string& source)
{
    noLongerPending(source, false);
}

void
MediaManager::mediaLoadFailed(const std::string& source)
{
   noLongerPending(source, true);
}

void
MediaManager::noLongerPending(const std::string& source, bool failed)
{
    if (mLoadedSources.count(source)) {
        return;
    }

    // We consider it "loaded", retries is out of scope for now
    mLoadedSources.emplace(source);

    for (auto& c2s : mComponentToSource) {
        if (c2s.second == source) {
            auto component = c2s.first.lock();
            if (!component) {
                mComponentToSource.erase(c2s.first);
                mComponentSetDirty = true;
                return;
            }

            auto cid = component->getUniqueId();
            auto pending = mPendingPerComponent[cid];
            if (pending > 0) {
                pending--;
                mPendingPerComponent[cid] = pending;
            } else {
                continue;
            }

            auto mediaTrait = std::dynamic_pointer_cast<MediaComponentTrait>(component);
            if (failed) {
                mediaTrait->pendingMediaFailed(source);
            } else {
                mediaTrait->pendingMediaLoaded(source, pending);
            }
        }
    }
}

} // namespace apl