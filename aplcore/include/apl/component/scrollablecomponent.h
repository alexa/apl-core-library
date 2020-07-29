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

#ifndef _APL_SCROLLABLE_COMPONENT_H
#define _APL_SCROLLABLE_COMPONENT_H

#include "actionablecomponent.h"

namespace apl {

/**
 * Abstract class representing actionable content that can scroll.
 */
class ScrollableComponent : public ActionableComponent {
protected:
    ScrollableComponent(const ContextPtr& context, Properties&& properties, const std::string& path) :
        ActionableComponent(context, std::move(properties), path) {};

    void update(UpdateType type, float value) override;

    const EventPropertyMap& eventPropertyMap() const override;

    virtual bool getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator) override;

    bool scrollable() const override { return true; }

    /**
     * Override this to calculate whether the scrollable can continue to scroll forward
     * @return true if scrollable can continue to scroll forward, false otherwise.
     */
    virtual bool allowForward() const = 0;

    /**
     * Override this to calculate whether the scrollable can scroll backwards from its
     * current position.
     * @return true if scrollable can continue to scroll backwards, false otherwise.
     */
    virtual bool allowBackwards() const = 0;

    bool isFocusable() const override { return true; }

    const ComponentPropDefSet& propDefSet() const override;

    /**
     * Override this to calculate maximum available scroll position.
     * @return maximum available scroll position.
     */
    virtual float maxScroll() const = 0;
};

} // namespace apl

#endif //_APL_SCROLLABLE_COMPONENT_H
