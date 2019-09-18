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

#include <yoga/YGNode.h>

#include "apl/component/componentpropdef.h"
#include "apl/component/textcomponent.h"
#include "apl/component/textmeasurement.h"
#include "apl/content/rootconfig.h"

namespace apl {


CoreComponentPtr
TextComponent::create(const ContextPtr& context,
                      Properties&& properties,
                      const std::string& path)
{
    auto ptr = std::make_shared<TextComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

static inline YGSize
textMeasureFunc( YGNodeRef node,
                 float width,
                 YGMeasureMode widthMode,
                 float height,
                 YGMeasureMode heightMode )
{
    TextComponent *component = static_cast<TextComponent*>(node->getContext());
    assert(component);

    return component->getContext()->measure()->measure(component, width, widthMode, height, heightMode);
}

static inline float
textBaselineFunc( YGNodeRef node, float width, float height )
{
    TextComponent *component = static_cast<TextComponent*>(node->getContext());
    assert(component);

    return component->getContext()->measure()->baseline(component, width, height);
}

TextComponent::TextComponent(const ContextPtr& context,
                             Properties&& properties,
                             const std::string& path)
    : CoreComponent(context, std::move(properties), path)
{
    YGNodeSetMeasureFunc(mYGNodeRef, textMeasureFunc);
    YGNodeSetBaselineFunc(mYGNodeRef, textBaselineFunc);
    YGNodeSetNodeType(mYGNodeRef, YGNodeTypeText);
    YGNodeSetContext(mYGNodeRef, this);   // Stash a pointer to this component
}


inline void internalCheckKaraokeTargetColor(Component& component)
{
    auto& text = static_cast<TextComponent&>(component);
    text.checkKaraokeTargetColor();
}

inline Object defaultFontColor(Component& component, const RootConfig& rootConfig)
{
    return Object(rootConfig.getDefaultFontColor(component.getContext()->getTheme()));
}

inline Object defaultFontFamily(Component&, const RootConfig& rootConfig)
{
    return Object(rootConfig.getDefaultFontFamily());
}

const ComponentPropDefSet&
TextComponent::propDefSet() const
{
    static ComponentPropDefSet sTextComponentProperties(CoreComponent::propDefSet(), {
        {kPropertyColor,              Color(),                asColor,               kPropInOut | kPropStyled | kPropDynamic,
                                                                                                  internalCheckKaraokeTargetColor, defaultFontColor},
        {kPropertyColorKaraokeTarget, Color(),                asColor,               kPropOut,    defaultFontColor},
        {kPropertyColorNonKaraoke,    Color(),                asColor,               kPropOut,    defaultFontColor},
        {kPropertyFontFamily,         "",                     asString,              kPropInOut | kPropLayout | kPropStyled, defaultFontFamily},
        {kPropertyFontSize,           Dimension(40),          asAbsoluteDimension,   kPropInOut | kPropLayout | kPropStyled},
        {kPropertyFontStyle,          kFontStyleNormal,       sFontStyleMap,         kPropInOut | kPropLayout | kPropStyled},
        {kPropertyFontWeight,         400,                    sFontWeightMap,        kPropInOut | kPropLayout | kPropStyled},
        {kPropertyLetterSpacing,      Dimension(0),           asAbsoluteDimension,   kPropInOut | kPropLayout | kPropStyled},
        {kPropertyLineHeight,         1.25,                   asNonNegativeNumber,   kPropInOut | kPropLayout | kPropStyled},
        {kPropertyMaxLines,           0,                      asInteger,             kPropInOut | kPropLayout | kPropStyled},
        {kPropertyText,               StyledText::EMPTY(),    asStyledText,          kPropInOut | kPropLayout | kPropDynamic},
        {kPropertyTextAlign,          kTextAlignAuto,         sTextAlignMap,         kPropInOut | kPropLayout | kPropStyled},
        {kPropertyTextAlignVertical,  kTextAlignVerticalAuto, sTextAlignVerticalMap, kPropInOut | kPropLayout | kPropStyled}
    });

    return sTextComponentProperties;
}

Object
TextComponent::getValue() const
{
    return mCalculated.get(kPropertyText).asString();
}

/*
 * In Karaoke Mode, we pick colors as follows:
 *
 * Approach     Mode     Color              ColorKaraokeTarget
 * -----------------------------------------------------------
 * Classic      Line     !state.karaoke     state.karaoke
 * Classic      Block    state.karaoke      N/A
 * Modern       Line     state.karaoke      state.karaoke && state.karaokeTarget
 * Modern       Block    state.karaoke      N/A
 *
 * You see the problem - the state.karaoke needs to switch based on the command.  But we don't know what that is.
 * So the only way out of this problem is to provide ANOTHER color - one that is the non-karaoke-color.
 *
 * Solution: We define the following:
 *
 * kPropertyColor                Reflects the current state, including state.karaoke.
 * kPropertyColorKaraokeTarget   Same as kPropertyColor with additional state.karaokeTarget
 * kPropertyColorNonKaraoke      Same as kPropertyColor, with state.karaoke = false at all times.
 *
 * Then the rules for drawing are
 *
 * Approach     Mode     Text                       Highlighted color
 * -------------------------------------------------------------------
 * Classic      Line     kPropertyColorNonKaraoke   kPropertyColor
 * Classic      Block    kPropertyColor             N/A
 * Modern       Line     kPropertyColor             kPropertyColorKaraokeTarget
 * Modern       Block    kPropertyColor             N/A
 */

void
TextComponent::checkKaraokeTargetColor()
{
    Object karaokeTargetColor = getCalculated(kPropertyColorKaraokeTarget);
    Object nonKaraokeColor = getCalculated(kPropertyColorNonKaraoke);

    Object targetColor = getCalculated(kPropertyColor);
    Object nonColor = targetColor;

    // If we're in karaoke mode AND we haven't manually assigned a color, we need to recalculate the Karaoke target color
    // and the non-Karaoke color
    if (mState.get(kStateKaraoke) && mAssigned.count(kPropertyColor) == 0) {
        State state = mState;  // Copy the old state.

        // Check the karaoke target color
        state.set(kStateKaraokeTarget, true);
        auto stylePtr = mContext->getStyle(mStyle, state);
        if (stylePtr) {
            auto value = stylePtr->find("color");
            if (value != stylePtr->end())
                targetColor = value->second.asColor(mContext);
        }

        // Check the non-karaoke color
        state.set(kStateKaraokeTarget, false);
        state.set(kStateKaraoke, false);
        stylePtr = mContext->getStyle(mStyle, state);
        if (stylePtr) {
            auto value = stylePtr->find("color");
            if (value != stylePtr->end())
                nonColor = value->second.asColor(mContext);
        }
    }

    if (targetColor != karaokeTargetColor) {
        mCalculated.set(kPropertyColorKaraokeTarget, targetColor);
        setDirty(kPropertyColorKaraokeTarget);
    }

    if (nonColor != nonKaraokeColor) {
        mCalculated.set(kPropertyColorNonKaraoke, nonColor);
        setDirty(kPropertyColorNonKaraoke);
    }
}

/*
 * Initial assignment of properties. Don't set any dirty flags here.
 *
 * This method initializes the value of kPropertyColorKaraokeTarget
 */
void
TextComponent::assignProperties(const ComponentPropDefSet& propDefSet)
{
    CoreComponent::assignProperties(propDefSet);

    // Update the non-karaoke color.  We don't need to check the state because
    // components cannot be created with the karaoke state active.
    mCalculated.set(kPropertyColorKaraokeTarget, mCalculated.get(kPropertyColor));
    mCalculated.set(kPropertyColorNonKaraoke, mCalculated.get(kPropertyColor));
}

std::shared_ptr<ObjectMap>
TextComponent::getEventTargetProperties() const
{
    auto target = CoreComponent::getEventTargetProperties();
    target->emplace("text", mCalculated.get(kPropertyText));
    target->emplace("color", mCalculated.get(kPropertyColor));
    return target;
}

rapidjson::Value
TextComponent::serializeMeasure(rapidjson::Document::AllocatorType& allocator) const
{
    rapidjson::Value component(rapidjson::kObjectType);

    component.AddMember("id", rapidjson::Value(mUniqueId.c_str(), allocator).Move(), allocator);

    for (const auto& pds : propDefSet()) {
        if ((pds.second.flags & kPropLayout) != 0)
            component.AddMember(
                rapidjson::StringRef(pds.second.name.c_str()),   // We assume long-lived strings here
                mCalculated.get(pds.first).serialize(allocator),
                allocator);
    }

    return component;
}

std::string
TextComponent::getVisualContextType() {
    return getValue().empty() ? VISUAL_CONTEXT_TYPE_EMPTY : VISUAL_CONTEXT_TYPE_TEXT;
}



} // namespace apl