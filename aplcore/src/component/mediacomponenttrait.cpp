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
#include "apl/engine/mediamanager.h"

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
    if (component->getCalculated(kPropertyLaidOut).truthy()) {
        ensureMediaRequested();
    }
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
    if (!sources.empty()) {
        auto stillPending = context->mediaManager().registerComponentMedia(component, sources);
        if (stillPending > 0) {
            component->setCalculated(kPropertyMediaState, kMediaStatePending);
        } else {
            component->setCalculated(kPropertyMediaState, kMediaStateReady);
        }
        component->setDirty(kPropertyMediaState);
    }
}

void
MediaComponentTrait::pendingMediaLoaded(const std::string& source, int stillPending)
{
    auto component = getComponent();
    auto state = static_cast<ComponentMediaState>(component->getCalculated(kPropertyMediaState).getInteger());
    switch (state) {
        case kMediaStatePending:
        {
            if (stillPending == 0) {
                component->setCalculated(kPropertyMediaState, kMediaStateReady);
                component->setDirty(kPropertyMediaState);
            }
            break;
        }
        case kMediaStateReady: // FALL_THROUGH
        case kMediaStateNotRequested:
        case kMediaStateError:
        default:
            // We are in state that can't produce any changes.
            return;
    }
}

void
MediaComponentTrait::pendingMediaFailed(const std::string& source)
{
    auto component = getComponent();
    auto state = static_cast<ComponentMediaState>(component->getCalculated(kPropertyMediaState).getInteger());
    if (state != kMediaStateError) {
        component->setCalculated(kPropertyMediaState, kMediaStateError);
        component->setDirty(kPropertyMediaState);
    }
}

} // namespace apl
