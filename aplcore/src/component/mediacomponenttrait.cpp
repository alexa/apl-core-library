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

#include "apl/component/mediacomponenttrait.h"

#include "apl/component/componentpropdef.h"
#include "apl/content/rootconfig.h"
#include "apl/media/mediamanager.h"

namespace apl {

const std::vector<ComponentPropDef>&
MediaComponentTrait::propDefList()
{
    static std::vector<ComponentPropDef> properties = {
        {kPropertyMediaState, kMediaStateNotRequested, asInteger, kPropRuntimeState}
    };

    return properties;
}

void
MediaComponentTrait::resetMediaFetchState(SourceValidator&& validator)
{
    auto component = getComponent();
    component->setCalculated(kPropertyMediaState, kMediaStateNotRequested);
    mMediaObjectHolders.clear();

    if (component->getCalculated(kPropertyLaidOut).truthy())
        ensureMediaRequested(std::move(validator));
}

void
MediaComponentTrait::postProcessLayoutChanges(SourceValidator&& validator) {
    // Component was laid out, so likely needs media loaded
    ensureMediaRequested(std::move(validator));
}

class FailedDataUrlMediaObject : public MediaObject {
public:
    FailedDataUrlMediaObject(const std::string& url) : mUrl(url) {}

    virtual std::string url() const override { return mUrl; }
    virtual State state() const override { return kError; }
    virtual EventMediaType type() const override { return kEventMediaTypeImage; }
    virtual Size size() const override { return Size(0, 0); }
    virtual int errorCode() const override { return 415; }
    virtual std::string errorDescription() const override { return "Failed to parse data URL"; }
    virtual const HeaderArray& headers() const override { return mHA; }
    virtual CallbackID addCallback(MediaObjectCallback callback) override { return -1; }
    virtual void removeCallback(CallbackID callbackToken) override {}

private:
    std::string mUrl;
    HeaderArray mHA;
};

void
MediaComponentTrait::ensureMediaRequested(SourceValidator&& validator)
{
    auto component = getComponent();
    auto state = static_cast<ComponentMediaState>(component->getCalculated(kPropertyMediaState).getInteger());
    // Check if we already fetched the source
    auto context = component->getContext();
    if (state != kMediaStateNotRequested ||
        !context->getRootConfig().experimentalFeatureEnabled(RootConfig::kExperimentalFeatureManageMediaRequests) ||
        component->getCalculated(kPropertySource).empty())
        return;

    auto sources = getSources();
    if (sources.empty())
        return;

    component->setCalculated(kPropertyMediaState, kMediaStatePending);
    component->setDirty(kPropertyMediaState);

    for (const auto& m : sources) {
        if (!validator(getComponent()->getContext()->session(), m)) {
            // Don't even request it. Add as failed MediaObject so onFail fired.
            auto mo = std::make_shared<FailedDataUrlMediaObject>(m.getUrl());
            mMediaObjectHolders.emplace_back(mo, -1);
            continue;
        }

        auto mediaObject = context->mediaManager().request(m.getUrl(), mediaType(), m.getHeaders());
        MediaObject::CallbackID callbackToken = 0;
        if (mediaObject->state() == MediaObject::kPending) {
            callbackToken = mediaObject->addCallback([this](const MediaObjectPtr& mediaObjectPtr) {
                pendingMediaReturned(mediaObjectPtr);
            });
        }
        mMediaObjectHolders.emplace_back(mediaObject, callbackToken);
    }

    // Some media objects may have been ready or in error state
    updateMediaState();
}

void
MediaComponentTrait::release()
{
    mMediaObjectHolders.clear();
}

void
MediaComponentTrait::pendingMediaReturned(const MediaObjectPtr& source)
{
    updateMediaState();
}

void
MediaComponentTrait::updateMediaState()
{
    auto component = getComponent();
    auto state = static_cast<ComponentMediaState>(component->getCalculated(kPropertyMediaState).getInteger());
    if (state != kMediaStatePending)
        return;
    // Check for error conditions first, any_of stops the first time the condition is met
    MediaObjectPtr onFailObject;
    if (std::any_of(mMediaObjectHolders.begin(),
                    mMediaObjectHolders.end(),
                    [&onFailObject](const MediaObjectHolder& holder) {
                        bool isError = holder.getMediaObject()->state() == MediaObject::kError;
                        if (isError)
                            onFailObject = holder.getMediaObject();
                        return isError;
                    })) {
        component->setCalculated(kPropertyMediaState, kMediaStateError);
        component->setDirty(kPropertyMediaState);
        onFail(onFailObject);
        return;
    } else if (std::all_of(mMediaObjectHolders.begin(), // Check if all media objects are ready
                           mMediaObjectHolders.end(),
                    [](const MediaObjectHolder& holder) {
                        return holder.getMediaObject()->state() == MediaObject::kReady;
                    })) {
        component->setCalculated(kPropertyMediaState, kMediaStateReady);
        component->setDirty(kPropertyMediaState);
        onLoad();
    }
}

} // namespace apl
