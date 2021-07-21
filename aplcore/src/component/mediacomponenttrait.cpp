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
#include "apl/engine/event.h"
#include "apl/media/mediamanager.h"
#include "apl/media/mediaobject.h"

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
MediaComponentTrait::resetMediaFetchState()
{
    auto component = getComponent();
    component->setCalculated(kPropertyMediaState, kMediaStateNotRequested);
    mMediaObjects.clear();

    if (component->getCalculated(kPropertyLaidOut).truthy())
        ensureMediaRequested();
}

void
MediaComponentTrait::postProcessLayoutChanges() {
    // Component was laid out, so likely needs media loaded
    ensureMediaRequested();
}


void
MediaComponentTrait::ensureMediaRequested()
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
        mMediaObjects.push_back(context->mediaManager().request(m, mediaType()));
        if (mMediaObjects.back()->state() == MediaObject::kPending) {
            auto weak = std::weak_ptr<CoreComponent>(component);
            mMediaObjects.back()->addCallback([weak](const MediaObjectPtr& mediaObjectPtr) {
                auto self = weak.lock();
                if (self)
                    std::dynamic_pointer_cast<MediaComponentTrait>(self)->pendingMediaReturned(mediaObjectPtr);
            });
        }
    }

    // Some of the media objects may have been ready or in error state
    updateMediaState();
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

    // Check for error conditions first
    if (std::any_of(mMediaObjects.begin(),
                    mMediaObjects.end(),
                    [](const MediaObjectPtr& ptr) { return ptr->state() == MediaObject::kError; })) {
        component->setCalculated(kPropertyMediaState, kMediaStateError);
        component->setDirty(kPropertyMediaState);
        return;
    }

    // Check if all media objects are ready
    if (std::all_of(mMediaObjects.begin(),
                    mMediaObjects.end(),
                    [](const MediaObjectPtr& ptr) {
                        return ptr->state() == MediaObject::kReady;
                    })) {
        component->setCalculated(kPropertyMediaState, kMediaStateReady);
        component->setDirty(kPropertyMediaState);
    }
}


} // namespace apl
