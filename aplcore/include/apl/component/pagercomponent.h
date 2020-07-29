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

#ifndef _APL_PAGER_COMPONENT_H
#define _APL_PAGER_COMPONENT_H

#include "actionablecomponent.h"

namespace apl {

class PagerComponent : public ActionableComponent {
public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const std::string& path);
    PagerComponent(const ContextPtr& context, Properties&& properties, const std::string& path);

    ComponentType getType() const override { return kComponentTypePager; };

    void initialize() override;

    void update(UpdateType type, float value) override;

    const ComponentPropDefSet* layoutPropDefSet() const override;

    Object getValue() const override { return getCalculated(kPropertyCurrentPage); }

    ScrollType scrollType() const override { return kScrollTypeHorizontalPager; }
    PageDirection pageDirection() const override;
    int pagePosition() const override { return getCalculated(kPropertyCurrentPage).asInt(); }

    bool getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator) override;

    void processLayoutChanges(bool useDirtyFlag) override;

protected:
    const ComponentPropDefSet& propDefSet() const override;
    const EventPropertyMap & eventPropertyMap() const override;
    void accept(Visitor<CoreComponent>& visitor) const override;
    void raccept(Visitor<CoreComponent>& visitor) const override;
    bool insertChild(const ComponentPtr& child, size_t index, bool useDirtyFlag) override;
    void removeChild(const CoreComponentPtr& child, size_t index, bool useDirtyFlag) override;
    bool shouldAttachChildYogaNode(int index) const override;
    void finalizePopulate() override;

private:
    bool multiChild() const override { return true; }
    std::map<int, float> getChildrenVisibility(float realOpacity, const Rect &visibleRect) const override;
    bool attachCurrentAndReportLoaded();
};

} // namespace apl

#endif //_APL_PAGER_COMPONENT_H
