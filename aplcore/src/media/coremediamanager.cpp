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

#include <forward_list>

#include "apl/engine/context.h"
#include "apl/media/coremediamanager.h"
#include "apl/media/mediaobject.h"

namespace apl {

/**
 * An implementation of the MediaObject class used by CoreMediaManager
 */
class CoreMediaObject : public MediaObject {
public:
    friend class CoreMediaManager;

    CoreMediaObject(std::string url, EventMediaType type, std::weak_ptr<CoreMediaManager> manager)
        : mUrl(std::move(url)),
          mMediaType(type),
          mMediaErrorCode(-1),
          mMediaManager(std::move(manager))
    {}

    ~CoreMediaObject() override {
        auto manager = mMediaManager.lock();
        if (manager)
            manager->removeMediaObject(mUrl);
    };

    std::string url() const override { return mUrl; }
    State state() const override { return mState; }
    EventMediaType type() const override { return mMediaType; }
    Size size() const override { return {}; }   // Note: Size is not supported
    int errorCode() const override { return mMediaErrorCode; }
    std::string errorDescription() const override { return mMediaErrorText; }

    CallbackID addCallback(MediaObjectCallback callback) override {
        if (mState != kPending)
            return 0;

        mCallbackToken++;
        mCallbacks.emplace(mCallbackToken, std::move(callback));
        return mCallbackToken;
    }

    void removeCallback(MediaObject::CallbackID token) override {
        if (token != 0) {
            auto it = mCallbacks.find(token);
            if (it != mCallbacks.end()) {
                mCallbacks.erase(it);
            }
        }
    }

private:
    State mState = kPending;
    std::string mUrl;
    EventMediaType mMediaType;
    int mMediaErrorCode;
    std::string mMediaErrorText;
    std::map<MediaObject::CallbackID , MediaObjectCallback> mCallbacks;
    std::weak_ptr<CoreMediaManager> mMediaManager;
    MediaObject::CallbackID mCallbackToken = 0;
};

// ********************** CoreMediaManager implementation ********************

MediaObjectPtr
CoreMediaManager::request(const std::string& url, EventMediaType type)
{
    // Check if the URL is in our loaded map
    auto it = mObjectMap.find(url);
    if (it != mObjectMap.end()) {
        auto ptr = it->second.lock();
        if (ptr)
            return ptr;
        mObjectMap.erase(it);
    }

    // Unrecognized URL; create a new media object and add it to the pending pool
    auto ptr = std::make_shared<CoreMediaObject>(url, type, shared_from_this());
    mObjectMap.emplace(url, ptr);
    mPending.emplace(ptr);

    return ptr;
}

void
CoreMediaManager::processMediaRequests(const ContextPtr& context)
{
    if (mPending.empty())
        return;

    // Note: We run these in enumerated order to simplify unit tests
    // that expect events to be generated in this order
    static std::vector<EventMediaType> sRequestTypes = {
        kEventMediaTypeImage,
        kEventMediaTypeVectorGraphic,
    };

    // Generate one media request per type
    // This could be done more efficiently, but it should be replaced with a single request in the future.
    for (const auto& type : sRequestTypes) {
        auto sources = std::make_shared<ObjectArray>();
        for (const auto& m : mPending) {
            auto ptr = m.lock();
            if (ptr && ptr->type() == type)
                sources->emplace_back(ptr->url());
        }

        if (!sources->empty()) {
            EventBag bag;
            bag.emplace(kEventPropertySource, sources);
            bag.emplace(kEventPropertyMediaType, type);
            context->pushEvent(Event(kEventTypeMediaRequest, std::move(bag)));
        }
    }

    mPending.clear();
}

void
CoreMediaManager::mediaLoadComplete(
    const std::string& source,
    bool isReady,
    int errorCode,
    const std::string& errorReason)
{
    auto it = mObjectMap.find(source);
    if (it == mObjectMap.end())
        return;

    // Check to see if a client still needs this media.
    auto ptr = it->second.lock();
    if (!ptr) {
        mObjectMap.erase(it);
        return;
    }

    auto mediaObject = std::dynamic_pointer_cast<CoreMediaObject>(ptr);
    if (!mediaObject) {
        LOG(LogLevel::kError) << "Unrecognized media object type";
        return;
    }

    // Update the media object state and execute all callbacks
    mediaObject->mState = isReady ? MediaObject::kReady : MediaObject::kError;
    mediaObject->mMediaErrorCode = isReady ? 0 : errorCode;
    mediaObject->mMediaErrorText = isReady ? std::string() : errorReason;
    for (const auto& m : mediaObject->mCallbacks)
        m.second(ptr);
    mediaObject->mCallbacks.clear();
}

void
CoreMediaManager::removeMediaObject(const std::string& url)
{
    auto it = mObjectMap.find(url);
    if (it != mObjectMap.end())
        mObjectMap.erase(it);

    // Don't bother to scan the pending set; it will automatically be cleared in the next processMediaRequests call
}

} // namespace apl