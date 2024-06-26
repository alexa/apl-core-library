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

#include "apl/engine/visibilitymanager.h"

#include "apl/component/corecomponent.h"
#include "apl/engine/arrayify.h"
#include "apl/time/sequencer.h"

namespace apl {

void
VisibilityManager::registerForUpdates(const CoreComponentPtr& component)
{
    // Add component to registration queue. We can't really create a tree properly until component
    // requested for registration ultimately attached to the hierarchy root, and this only happens
    // when all children processed.
    mRegistrationQueue.emplace(component);
}

void
VisibilityManager::deregister(const CoreComponentPtr& component)
{
    if (component && mTrackedComponentVisibility.count(component)) {
        mTrackedComponentVisibility.erase(component);
    }
}

void
VisibilityManager::markDirty(const CoreComponentPtr& component)
{
    if (component && mTrackedComponentVisibility.count(component)) {
        mDirtyVisibility.emplace(component);
    }
}

void
VisibilityManager::processVisibilityChanges()
{
    // Register any new added components.
    for (const auto& weak : mRegistrationQueue) {
        auto component = weak.lock();
        if (!component) continue;

        mTrackedComponentVisibility.emplace(component, VisibilityState{-1, -1});
        auto parent = CoreComponent::cast(component->getParent());

        if (parent) parent->addDownstreamVisibilityTarget(component);

        markDirty(component);
    }

    mRegistrationQueue.clear();

    for (const auto& cc : mDirtyVisibility) {
        auto it = mTrackedComponentVisibility.find(cc);
        if (it == mTrackedComponentVisibility.end()) continue;

        auto component = it->first.lock();
        if (!component) continue;

        auto handlers = component->getCalculated(kPropertyHandleVisibilityChange);

        // Should not happen, components without handler defined should not even be registered
        assert(!handlers.isNull());

        auto& ctx = *component->getContext();
        auto commands = Object::NULL_OBJECT();
        for (auto& handler : handlers.getArray()) {
            if (propertyAsBoolean(ctx, handler, "when", true)) {
                commands = Object(arrayifyProperty(ctx, handler, "commands"));
            }
        }

        if (!commands.isArray()) continue;

        // displayed
        auto cumulativeOpacity = component->calculateRealOpacity();
        auto visibleArea = component->calculateVisibleRect().area();
        auto ownArea = component->getProperty(kPropertyBounds).get<Rect>().area();
        auto visibleRegionPercentage = (ownArea == 0 || visibleArea == 0) ? 0 : visibleArea / ownArea;

        // If same state - do not report
        if (it->second.visibleRegionPercentage == visibleRegionPercentage &&
            it->second.cumulativeOpacity == cumulativeOpacity) {
            continue;
        }

        it->second = VisibilityState{visibleRegionPercentage, cumulativeOpacity};

        auto visibilityOpt = std::make_shared<std::map<std::string, Object>>();
        visibilityOpt->emplace("visibleRegionPercentage", visibleRegionPercentage);
        visibilityOpt->emplace("cumulativeOpacity", cumulativeOpacity);

        auto eventContext = component->createEventContext("VisibilityChange", visibilityOpt);
        eventContext->sequencer().executeCommands(commands, eventContext, component, true);
    }

    mDirtyVisibility.clear();
}

} // namespace apl
