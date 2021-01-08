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
#include <string>

#include "apl/component/componentpropdef.h"
#include "apl/component/edittextcomponent.h"
#include "apl/component/yogaproperties.h"
#include "apl/content/rootconfig.h"
#include "apl/primitives/characterrange.h"
#include "apl/time/sequencer.h"

namespace apl {

CoreComponentPtr
EditTextComponent::create(const ContextPtr& context,
                          Properties&& properties,
                          const std::string& path) {
    auto ptr = std::make_shared<EditTextComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

EditTextComponent::EditTextComponent(const ContextPtr& context,
                                     Properties&& properties,
                                     const std::string& path)
        : ActionableComponent(context, std::move(properties), path) {
    YGNodeSetMeasureFunc(mYGNodeRef, textMeasureFunc<EditTextComponent>);
    YGNodeSetBaselineFunc(mYGNodeRef, textBaselineFunc<EditTextComponent>);
    YGNodeSetNodeType(mYGNodeRef, YGNodeTypeText);
    YGNodeSetContext(mYGNodeRef, this);   // Stash a pointer to this component
}

/*
 * Initial assignment of properties.  Don't set any dirty flags here; this
 * all should be running in the constructor.
 *
 * This method initializes the values of the border corners.
*/
void
EditTextComponent::assignProperties(const ComponentPropDefSet& propDefSet)
{
    CoreComponent::assignProperties(propDefSet);
    calculateDrawnBorder(false);
    parseValidCharactersProperty();
}

inline Object defaultFontColor(Component& component, const RootConfig& rootConfig)
{
    return Object(rootConfig.getDefaultFontColor(component.getContext()->getTheme()));
}

inline Object defaultFontFamily(Component& component, const RootConfig& rootConfig)
{
    return Object(rootConfig.getDefaultFontFamily());
}

inline Object defaultHighlightColor(Component& component, const RootConfig& rootConfig)
{
    return Object(rootConfig.getDefaultHighlightColor(component.getContext()->getTheme()));
}

const ComponentPropDefSet&
EditTextComponent::propDefSet() const {
    static ComponentPropDefSet sEditTextComponentProperties(ActionableComponent::propDefSet(), {
            {kPropertyBorderColor,      Color(),            asColor,             kPropInOut | kPropStyled | kPropDynamic},
            {kPropertyBorderWidth,      Dimension(0),       asNonNegativeAbsoluteDimension, kPropInOut | kPropStyled, yn::setBorder<YGEdgeAll>},
            {kPropertyColor,            Color(),            asColor,             kPropInOut | kPropStyled | kPropDynamic, defaultFontColor},
            {kPropertyFontFamily,       "",                 asString,            kPropInOut | kPropLayout | kPropStyled, defaultFontFamily},
            {kPropertyFontSize,         Dimension(40),      asAbsoluteDimension, kPropInOut | kPropLayout | kPropStyled},
            {kPropertyFontStyle,        kFontStyleNormal,   sFontStyleMap,       kPropInOut | kPropLayout | kPropStyled},
            {kPropertyFontWeight,       400,                sFontWeightMap,      kPropInOut | kPropLayout | kPropStyled},
            {kPropertyHighlightColor,   Color(),            asColor,             kPropInOut | kPropStyled, defaultHighlightColor},
            {kPropertyHint,             "",                 asString,            kPropInOut | kPropStyled},
            {kPropertyHintColor,        Color(),            asColor,             kPropInOut | kPropStyled, defaultFontColor},
            {kPropertyHintStyle,        kFontStyleNormal,   sFontStyleMap,       kPropInOut | kPropLayout | kPropStyled},
            {kPropertyHintWeight,       400,                sFontWeightMap,      kPropInOut | kPropLayout | kPropStyled},
            {kPropertyKeyboardType,     kKeyboardTypeNormal, sKeyboardTypeMap,   kPropInOut | kPropStyled},
            {kPropertyMaxLength,        0,                  asInteger,           kPropInOut | kPropStyled},
            {kPropertyOnSubmit,         Object::EMPTY_ARRAY(), asCommand,        kPropIn},
            {kPropertyOnTextChange,     Object::EMPTY_ARRAY(), asCommand,        kPropIn},
            {kPropertySecureInput,      false,              asBoolean,           kPropInOut | kPropStyled | kPropDynamic},
            {kPropertySelectOnFocus,    false,              asBoolean,           kPropInOut | kPropStyled},
            {kPropertySize,             8,                  asPositiveInteger,   kPropInOut | kPropStyled | kPropLayout},
            {kPropertySubmitKeyType,    kSubmitKeyTypeDone, sSubmitKeyTypeMap,   kPropInOut | kPropStyled},
            {kPropertyText,             "",                 asString,            kPropInOut | kPropDynamic | kPropVisualContext},
            {kPropertyValidCharacters,  "",                 asString,            kPropIn | kPropStyled},

            // The width of the drawn border.  If borderStrokeWith is set, the drawn border is the min of borderWidth
            // and borderStrokeWidth.  If borderStrokeWidth is unset, the drawn border defaults to borderWidth
            {kPropertyBorderStrokeWidth, Object::NULL_OBJECT(), asNonNegativeAbsoluteDimension, kPropIn | kPropStyled | kPropDynamic, resolveDrawnBorder},
            {kPropertyDrawnBorderWidth,  Object::NULL_OBJECT(), asNonNegativeAbsoluteDimension, kPropOut},

    });

    return sEditTextComponentProperties;
}

const EventPropertyMap&
EditTextComponent::eventPropertyMap() const {
    static EventPropertyMap sEditTextEventProperties = eventPropertyMerge(
        CoreComponent::eventPropertyMap(),
        {
            {"text",  [](const CoreComponent *c) { return c->getCalculated(kPropertyText); }},
            {"color", [](const CoreComponent *c) { return c->getCalculated(kPropertyColor); }},
        });

    return sEditTextEventProperties;
}

Object
EditTextComponent::getValue() const
{
    return mCalculated.get(kPropertyText);
}

void
EditTextComponent::update(UpdateType type, float value) {
    if (type == kUpdateSubmit) {
        ContextPtr eventContext = createEventContext("Submit");
        auto commands = getCalculated(kPropertyOnSubmit);
        mContext->sequencer().executeCommands(commands, eventContext, shared_from_corecomponent(), false);

    } else
    CoreComponent::update(type, value);
}

void
EditTextComponent::update(UpdateType type, const std::string& value) {
    if (type == kUpdateTextChange) {
        auto requestedValue = Object(value);
        auto currentValue = mCalculated.get(kPropertyText);
        if (requestedValue != currentValue) {
            mCalculated.set(kPropertyText, value);
            ContextPtr eventContext = createEventContext("TextChange");
            auto commands = getCalculated(kPropertyOnTextChange);
            mContext->sequencer().executeCommands(commands, eventContext, shared_from_corecomponent(), false);
        }
    } else
        CoreComponent::update(type, value);
}

bool
EditTextComponent::isCharacterValid(const wchar_t wc) const {
    if (mCharacterRangesPtr == nullptr) return true;

    std::vector<CharacterRangeData> validRanges = mCharacterRangesPtr->getRanges();
    if (validRanges.size() == 0) return true;

    for (std::vector<CharacterRangeData>::iterator it = validRanges.begin(); it != validRanges.end(); ++it) {
        if (it->isCharacterValid(wc)) return true;
    }
    return false;
}

void EditTextComponent::parseValidCharactersProperty() {
    mCharacterRangesPtr = std::make_shared<CharacterRanges>(CharacterRanges(getContext()->session(),
            mCalculated.get(kPropertyValidCharacters).asString()));
    }

}  // namespace apl