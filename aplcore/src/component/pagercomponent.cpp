/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "apl/command/setpagecommand.h"
#include "apl/component/componentpropdef.h"
#include "apl/component/pagercomponent.h"
#include "apl/component/yogaproperties.h"
#include "apl/content/rootconfig.h"
#include "apl/time/sequencer.h"
#include "apl/primitives/keyboard.h"

namespace apl {


CoreComponentPtr
PagerComponent::create(const ContextPtr& context,
                       Properties&& properties,
                       const std::string& path) {
    auto ptr = std::make_shared<PagerComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

PagerComponent::PagerComponent(const ContextPtr& context,
                               Properties&& properties,
                               const std::string& path)
        : ActionableComponent(context, std::move(properties), path) {
}

inline Object
defaultWidth(Component& component, const RootConfig& rootConfig) {
    return rootConfig.getDefaultComponentWidth(component.getType());
}

inline Object
defaultHeight(Component& component, const RootConfig& rootConfig) {
    return rootConfig.getDefaultComponentHeight(component.getType());
}

const ComponentPropDefSet&
PagerComponent::propDefSet() const {
    static ComponentPropDefSet sPagerComponentProperties(ActionableComponent::propDefSet(), {
            {kPropertyHeight,        Dimension(100),        asNonAutoDimension, kPropIn, yn::setHeight, defaultHeight},
            {kPropertyWidth,         Dimension(100),        asNonAutoDimension, kPropIn, yn::setWidth,  defaultWidth},
            {kPropertyInitialPage,   0,                     asInteger,          kPropIn},
            {kPropertyNavigation,    kNavigationWrap,       sNavigationMap,     kPropInOut},
            {kPropertyOnPageChanged, Object::EMPTY_ARRAY(), asCommand,          kPropIn}
    });

    return sPagerComponentProperties;
}

void
PagerComponent::initialize() {
    CoreComponent::initialize();

    // TODO: This needs to be clipped to the actual number of pages
    mCurrentPage = getCalculated(kPropertyInitialPage).asInt();
}

void
PagerComponent::update(UpdateType type, float value) {
    if (type == kUpdatePagerPosition || type == kUpdatePagerByEvent) {
        if (value != mCurrentPage) {
            mCurrentPage = value;
            ContextPtr eventContext = createEventContext("Page", mCurrentPage);
            mContext->sequencer().executeCommands(
                    getCalculated(kPropertyOnPageChanged),
                    eventContext,
                    shared_from_this(),
                    type == kUpdatePagerByEvent);  // If the user set the pager, run in fast mode.
        }

    } else
        CoreComponent::update(type, value);
}

const ComponentPropDefSet*
PagerComponent::layoutPropDefSet() const {
    static ComponentPropDefSet sPagerChildProperties = ComponentPropDefSet().add(
        {
            // Force absolute position because the pager children each occupy the entire space of their parent
            {kPropertyPosition, kPositionAbsolute, sPositionMap, kPropOut | kPropResetOnRemove, yn::setPositionType},

            // The width and height of the children of a pager are set to 100%
            {kPropertyWidth, Dimension(DimensionType::Relative, 100), asNonAutoDimension, kPropNone, yn::setWidth },
            {kPropertyHeight, Dimension(DimensionType::Relative, 100), asNonAutoDimension, kPropNone, yn::setHeight}
        });

    return &sPagerChildProperties;
}


std::shared_ptr<ObjectMap>
PagerComponent::getEventTargetProperties() const {
    auto target = CoreComponent::getEventTargetProperties();
    target->emplace("page", mCurrentPage);
    return target;
}

PageDirection
PagerComponent::pageDirection() const {
    // If we don't have children, there is no navigation
    auto len = mChildren.size();
    if (len <= 1)
        return kPageDirectionNone;

    auto navigation = static_cast<Navigation>(mCalculated.get(kPropertyNavigation).asInt());
    switch (navigation) {
        case kNavigationNormal:  // No wrapping, forward or back supported
            if (mCurrentPage == 0)
                return kPageDirectionForward;
            if (mCurrentPage == len - 1)
                return kPageDirectionBack;
            return kPageDirectionBoth;

        case kNavigationNone:
            return kPageDirectionNone;

        case kNavigationWrap:
            return kPageDirectionBoth;

        case kNavigationForwardOnly:
            if (mCurrentPage == len - 1)
                return kPageDirectionNone;

            return kPageDirectionForward;
    }
}

std::map<int, float>
PagerComponent::getChildrenVisibility(float realOpacity, const Rect& visibleRect) {
    std::map<int, float> result;

    if (!mChildren.empty()) {
        auto child = mChildren.at(mCurrentPage);
        auto childVisibility = child->calculateVisibility(realOpacity, visibleRect);
        if (childVisibility > 0.0) {
            result.emplace(mCurrentPage, childVisibility);
        }
    }

    return result;
}

bool
PagerComponent::getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator) {
    bool actionable = CoreComponent::getTags(outMap, allocator);
    if (mChildren.size() > 1) {
        bool allowForward = false;
        bool allowBackwards = false;

        switch (pageDirection()) {
            case kPageDirectionBoth:
                allowForward = true;
                allowBackwards = true;
                break;
            case kPageDirectionBack:
                allowBackwards = true;
                break;
            case kPageDirectionForward:
                allowForward = true;
                break;
            case kPageDirectionNone:
                break;
            default:
                break;
        }

        rapidjson::Value pager(rapidjson::kObjectType);
        pager.AddMember("index", mCurrentPage, allocator);
        pager.AddMember("pageCount", (int) mChildren.size(), allocator);
        pager.AddMember("allowForward", allowForward, allocator);
        pager.AddMember("allowBackwards", allowBackwards, allocator);
        outMap.AddMember("pager", pager, allocator);
        actionable = true;
    }

    return actionable;
}

ComponentPtr
PagerComponent::findChildAtPosition(const Point& position) const {
    // The current page may contain the target position
    if (mCurrentPage >= 0 && mCurrentPage < getChildCount()) {
        auto child = mChildren.at(mCurrentPage)->findComponentAtPosition(position);
        if (child != nullptr)
            return child;
    }

    return nullptr;
}


} // namespace apl
