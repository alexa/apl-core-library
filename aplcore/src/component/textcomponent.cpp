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
#include "apl/component/textcomponent.h"
#include "apl/content/rootconfig.h"

namespace apl {


CoreComponentPtr
TextComponent::create(const ContextPtr& context,
                      Properties&& properties,
                      const Path& path)
{
    auto ptr = std::make_shared<TextComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

TextComponent::TextComponent(const ContextPtr& context,
                             Properties&& properties,
                             const Path& path)
    : CoreComponent(context, std::move(properties), path)
{
    YGNodeSetMeasureFunc(mYGNodeRef, textMeasureFunc);
    YGNodeSetBaselineFunc(mYGNodeRef, textBaselineFunc);
    YGNodeSetNodeType(mYGNodeRef, YGNodeTypeText);
}


static inline void internalCheckKaraokeTargetColor(Component& component)
{
    auto& text = static_cast<TextComponent&>(component);
    text.checkKaraokeTargetColor();
}

static inline Object defaultFontColor(Component& component, const RootConfig& rootConfig)
{
    return Object(rootConfig.getDefaultFontColor(component.getContext()->getTheme()));
}

static inline Object defaultFontFamily(Component&, const RootConfig& rootConfig)
{
    return Object(rootConfig.getDefaultFontFamily());
}

static inline Object inheritLang(Component& comp, const RootConfig& rconfig)
{
    return Object(comp.getContext()->getLang());
};

const ComponentPropDefSet&
TextComponent::propDefSet() const
{
    auto fixTextAlign = [] (Component& comp) {
      auto& coreComp = dynamic_cast<TextComponent&>(comp);
      coreComp.updateTextAlign(true);
    };

    static ComponentPropDefSet sTextComponentProperties(CoreComponent::propDefSet(), {
        {kPropertyColor,               Color(),                asColor,               kPropInOut | kPropStyled | kPropDynamic | kPropVisualHash, internalCheckKaraokeTargetColor, defaultFontColor},
        {kPropertyColorKaraokeTarget,  Color(),                asColor,               kPropOut | kPropVisualHash,                                                                 defaultFontColor},
        {kPropertyColorNonKaraoke,     Color(),                asColor,               kPropOut | kPropVisualHash,                                                                 defaultFontColor},
        {kPropertyFontFamily,          "",                     asString,              kPropInOut | kPropLayout | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash,    defaultFontFamily},
        {kPropertyFontSize,            Dimension(40),          asAbsoluteDimension,   kPropInOut | kPropLayout | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash},
        {kPropertyFontStyle,           kFontStyleNormal,       sFontStyleMap,         kPropInOut | kPropLayout | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash},
        {kPropertyFontWeight,          400,                    sFontWeightMap,        kPropInOut | kPropLayout | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash},
        {kPropertyLang,                "",                     asString,              kPropInOut | kPropLayout | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash,    inheritLang},
        {kPropertyLetterSpacing,       Dimension(0),           asAbsoluteDimension,   kPropInOut | kPropLayout | kPropStyled | kPropTextHash | kPropVisualHash},
        {kPropertyLineHeight,          1.25,                   asNonNegativeNumber,   kPropInOut | kPropLayout | kPropStyled | kPropTextHash | kPropVisualHash},
        {kPropertyMaxLines,            0,                      asInteger,             kPropInOut | kPropLayout | kPropStyled | kPropTextHash | kPropVisualHash},
        {kPropertyText,                StyledText::EMPTY(),    asStyledText,          kPropInOut | kPropLayout | kPropDynamic | kPropVisualContext | kPropTextHash | kPropVisualHash},
        {kPropertyTextAlign,           kTextAlignAuto,         sTextAlignMap,         kPropOut | kPropTextHash | kPropVisualHash},
        {kPropertyTextAlignAssigned,   kTextAlignAuto,         sTextAlignMap,         kPropIn  | kPropLayout | kPropStyled | kPropDynamic,                                       fixTextAlign},
        {kPropertyTextAlignVertical,   kTextAlignVerticalAuto, sTextAlignVerticalMap, kPropInOut | kPropLayout | kPropStyled | kPropTextHash | kPropVisualHash},
    });

    return sTextComponentProperties;
}

const EventPropertyMap&
TextComponent::eventPropertyMap() const
{
    static EventPropertyMap sTextEventProperties = eventPropertyMerge(
            CoreComponent::eventPropertyMap(),
            {
                    {"text",  [](const CoreComponent *c) { return c->getCalculated(kPropertyText).getStyledText().getText(); }},
                    {"color", [](const CoreComponent *c) { return c->getCalculated(kPropertyColor); }},
            });

    return sTextEventProperties;
}

Object
TextComponent::getValue() const
{
    return mCalculated.get(kPropertyText).asString();
}

void
TextComponent::preLayoutProcessing(bool useDirtyFlag)
{
    CoreComponent::preLayoutProcessing(useDirtyFlag);

    // Update text measurement hash as some properties may have changed it
    // and we actually need it on layout time
    fixTextMeasurementHash();
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
                targetColor = value->second.asColor(*mContext);
        }

        // Check the non-karaoke color
        state.set(kStateKaraokeTarget, false);
        state.set(kStateKaraoke, false);
        stylePtr = mContext->getStyle(mStyle, state);
        if (stylePtr) {
            auto value = stylePtr->find("color");
            if (value != stylePtr->end())
                nonColor = value->second.asColor(*mContext);
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

void
TextComponent::updateTextAlign(bool useDirtyFlag)
{
    auto layoutDirection = static_cast<LayoutDirection>(mCalculated.get(kPropertyLayoutDirection).asInt());
    auto textAlign = static_cast<TextAlign>(mCalculated.get(kPropertyTextAlignAssigned).asInt());

    if (textAlign == kTextAlignStart) {
        textAlign = layoutDirection == kLayoutDirectionRTL ? kTextAlignRight : kTextAlignLeft;
    } else if (textAlign == kTextAlignEnd) {
        textAlign = layoutDirection == kLayoutDirectionRTL ? kTextAlignLeft  : kTextAlignRight;
    }
    if (textAlign != mCalculated.get(kPropertyTextAlign).asInt()) {
        mCalculated.set(kPropertyTextAlign, textAlign);
        if (useDirtyFlag)
            setDirty(kPropertyTextAlign);
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
    updateTextAlign(false);

    // Calculate initial measurement hash.
    fixTextMeasurementHash();
}

rapidjson::Value
TextComponent::serializeMeasure(rapidjson::Document::AllocatorType& allocator) const
{
    rapidjson::Value component(rapidjson::kObjectType);

    component.AddMember("id", rapidjson::Value(mUniqueId.c_str(), allocator).Move(), allocator);

    for (const auto& pds : propDefSet()) {
        if ((pds.second.flags & kPropLayout) != 0)
            component.AddMember(
                rapidjson::StringRef(pds.second.names[0].c_str()),   // We assume long-lived strings here
                mCalculated.get(pds.first).serialize(allocator),
                allocator);
    }

    return component;
}

std::string
TextComponent::getVisualContextType() const
{
    return getValue().empty() ? VISUAL_CONTEXT_TYPE_EMPTY : VISUAL_CONTEXT_TYPE_TEXT;
}



} // namespace apl
