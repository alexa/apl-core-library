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

#ifndef _APL_SEQUENCE_COMPONENT_H
#define _APL_SEQUENCE_COMPONENT_H

#include "scrollablecomponent.h"
#include "apl/utils/range.h"

namespace apl {

class SequenceComponent : public ScrollableComponent {
public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const std::string& path);
    SequenceComponent(const ContextPtr& context, Properties&& properties, const std::string& path);
    const ComponentPropDefSet* layoutPropDefSet() const override;

    ComponentType getType() const override { return kComponentTypeSequence; };
    bool allowForward() const override;
    Object getValue() const override;

    ScrollType scrollType() const override;
    Point scrollPosition() const override;
    Point trimScroll(const Point& point) const override;

    void processLayoutChanges(bool useDirtyFlag) override;

protected:
    const ComponentPropDefSet& propDefSet() const override;

    bool getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator) override;

    bool shouldAttachChildYogaNode(int index) const override;

    void update(UpdateType type, float value) override;

    ComponentPtr findChildAtPosition(const Point& position) const override;

    bool insertChild(const ComponentPtr& child, size_t index, bool useDirtyFlag) override;

    void removeChild(const CoreComponentPtr& child, size_t index, bool useDirtyFlag) override;

    float maxScroll() const override;

private:
    bool multiChild() const override { return true; }
    std::map<int, float> getChildrenVisibility(float realOpacity, const Rect &visibleRect) override;
    void updateSeen();
    ComponentPtr findDirectChildAtPosition(const Point& position) const;
    void layoutChildIfRequired(const Rect& parentBounds, const CoreComponentPtr& child, size_t childIdx, bool useDirtyFlag);

    Range mIndexesSeen;
};

} // namespace apl

#endif //_APL_SEQUENCE_COMPONENT_H
