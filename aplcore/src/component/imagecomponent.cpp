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

#include "apl/component/componentpropdef.h"
#include "apl/component/imagecomponent.h"
#include "apl/component/yogaproperties.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/event.h"
#include "apl/media/mediamanager.h"

namespace apl {

CoreComponentPtr
ImageComponent::create(const ContextPtr& context,
                       Properties&& properties,
                       const Path& path)
{
    auto ptr = std::make_shared<ImageComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

ImageComponent::ImageComponent(const ContextPtr& context,
                               Properties&& properties,
                               const Path& path)
    : CoreComponent(context, std::move(properties), path)
{
}

const ComponentPropDefSet&
ImageComponent::propDefSet() const
{
    static auto resetMediaState = [](Component& component) {
        auto& comp = dynamic_cast<ImageComponent&>(component);
        comp.resetMediaFetchState();
    };

    static ComponentPropDefSet sImageComponentProperties = ComponentPropDefSet(
        CoreComponent::propDefSet(), MediaComponentTrait::propDefList()).add({
        {kPropertyAlign,           kImageAlignCenter,        sAlignMap,           kPropInOut | kPropStyled | kPropDynamic}, // Doesn't match 1.0 spec
        {kPropertyBorderRadius,    Object::ZERO_ABS_DIMEN(), asAbsoluteDimension, kPropInOut | kPropStyled | kPropDynamic},
        {kPropertyFilters,         Object::EMPTY_ARRAY(),    asFilterArray,       kPropInOut },
        {kPropertyOverlayColor,    Color(),                  asColor,             kPropInOut | kPropStyled | kPropDynamic},
        {kPropertyOverlayGradient, Object::NULL_OBJECT(),    asGradient,          kPropInOut | kPropStyled | kPropDynamic},
        {kPropertyScale,           kImageScaleBestFit,       sScaleMap,           kPropInOut | kPropStyled | kPropDynamic},
        {kPropertySource,          "",                       asStringOrArray,     kPropInOut | kPropDynamic, resetMediaState}
    });

    return sImageComponentProperties;
}

std::vector<std::string>
ImageComponent::getSources()
{
    std::vector<std::string> sources;

    auto& sourceProp = getCalculated(kPropertySource);
    // Check if there anything to fetch
    if (sourceProp.empty()) {
        return sources;
    }

    if (sourceProp.isString()) { // Single source
        sources.emplace_back(sourceProp.getString());
    } else if (sourceProp.isArray()) {
        auto& filters = getCalculated(kPropertyFilters);
        if (filters.empty()) { // If no filters use last
            sources.emplace_back(sourceProp.at(sourceProp.size() - 1).getString());
        } else { // Else request everything
            for (auto& source : sourceProp.getArray()) {
                sources.emplace_back(source.getString());
            }
        }
    }

    return sources;
}

const EventPropertyMap&
ImageComponent::eventPropertyMap() const
{
    static EventPropertyMap sImageEventProperties = eventPropertyMerge(
        CoreComponent::eventPropertyMap(),
        {
            {"source", [](const CoreComponent *c) { return c->getCalculated(kPropertySource); }},
            {"url", [](const CoreComponent *c) { return c->getCalculated(kPropertySource); }},
        });
    return sImageEventProperties;
}

std::string
ImageComponent::getVisualContextType() const
{
    return getCalculated(kPropertySource).empty() ? VISUAL_CONTEXT_TYPE_EMPTY : VISUAL_CONTEXT_TYPE_GRAPHIC;
}

void
ImageComponent::postProcessLayoutChanges()
{
    CoreComponent::postProcessLayoutChanges();
    MediaComponentTrait::postProcessLayoutChanges();
}

} // namespace apl
