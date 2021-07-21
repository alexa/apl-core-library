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

#ifndef _APL_SCROLL_VIEW_COMPONENT_H
#define _APL_SCROLL_VIEW_COMPONENT_H

#include "scrollablecomponent.h"

namespace apl {

class ScrollViewComponent : public ScrollableComponent {
public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const Path& path);
    ScrollViewComponent(const ContextPtr& context, Properties&& properties, const Path& path);

    ComponentType getType() const override { return kComponentTypeScrollView; };
    Object getValue() const override;

    ScrollType scrollType() const override { return kScrollTypeVertical; }
    Point scrollPosition() const override;
    Point trimScroll(const Point& point) override;

    bool isHorizontal() const override { return false; }
    bool isVertical() const override { return true; }

protected:
    const ComponentPropDefSet& propDefSet() const override;

    float maxScroll() const override;

    bool allowForward() const override;
    bool allowBackwards() const override;
    void onScrollPositionUpdated() override;

private:
    bool singleChild() const override { return true; }
};

} // namespace apl

#endif //_APL_SCROLL_VIEW_COMPONENT_H
