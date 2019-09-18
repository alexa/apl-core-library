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

#ifndef _APL_SCROLL_VIEW_COMPONENT_H
#define _APL_SCROLL_VIEW_COMPONENT_H

#include "scrollablecomponent.h"

namespace apl {

class ScrollViewComponent : public ScrollableComponent {
public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const std::string& path);
    ScrollViewComponent(const ContextPtr& context, Properties&& properties, const std::string& path);

    ComponentType getType() const override { return kComponentTypeScrollView; };
    Object getValue() const override;

    ScrollType scrollType() const override { return kScrollTypeVertical; }
    Point scrollPosition() const override { return Point(0, mCurrentPosition); }
    Point trimScroll(const Point& point) const override;
    bool allowForward() const override;

protected:
    const ComponentPropDefSet& propDefSet() const override;

private:
    bool singleChild() const override { return true; }

    float calculateMaxY() const;
};

} // namespace apl

#endif //_APL_SCROLL_VIEW_COMPONENT_H
