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

#ifndef _APL_ANIMATED_PROPERTY_H
#define _APL_ANIMATED_PROPERTY_H

#include "apl/common.h"
#include "apl/component/componentproperties.h"
#include "apl/primitives/transform.h"
#include "apl/utils/noncopyable.h"

namespace apl {

class CoreComponent;
class Context;

class AnimatedProperty : public NonCopyable {
public:
    virtual ~AnimatedProperty() = default;
    static std::unique_ptr<AnimatedProperty> create(const ContextPtr& context,
                                                    const CoreComponentPtr& component,
                                                    const Object& object);

    virtual void update(const CoreComponentPtr& component, float alpha) = 0;

    virtual PropertyKey key() const = 0;
};

class AnimatedDouble : public AnimatedProperty {
public:
    AnimatedDouble(PropertyKey key, double from, double to);
    AnimatedDouble(PropertyKey key, const CoreComponentPtr& component, double to);

    void update(const CoreComponentPtr& component, float alpha) override;

    PropertyKey key() const override { return mKey; }

private:
    PropertyKey mKey;  // The component property we're animating
    double mFrom;  // The from value
    double mTo;    // The to value
};

class AnimatedTransform : public AnimatedProperty {
public:
    AnimatedTransform(const std::shared_ptr<InterpolatedTransformation>& transformation);
    void update(const CoreComponentPtr& component, float alpha) override;

    PropertyKey key() const override { return kPropertyTransform; }
private:
    std::shared_ptr<InterpolatedTransformation> mTransformation;
};


} // namespace apl

#endif //_APL_ANIMATED_PROPERTY_H
