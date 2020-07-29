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

#ifndef _APL_TEXT_COMPONENT_H
#define _APL_TEXT_COMPONENT_H

#include "corecomponent.h"

namespace apl {


class TextComponent : public CoreComponent {
public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const std::string& path);
    TextComponent(const ContextPtr& context, Properties&& properties, const std::string& path);

    ComponentType getType() const override { return kComponentTypeText; };

    void checkKaraokeTargetColor();

    Object getValue() const override;

    rapidjson::Value serializeMeasure(rapidjson::Document::AllocatorType& allocator) const;

protected:
    const EventPropertyMap & eventPropertyMap() const override;
    const ComponentPropDefSet& propDefSet() const override;
    void assignProperties(const ComponentPropDefSet& propDefSet) override;
    std::string getVisualContextType() override;
};



} // namespace apl

#endif //_APL_TEXT_COMPONENT_H
