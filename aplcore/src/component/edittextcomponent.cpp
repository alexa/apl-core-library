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

#include "apl/component/edittextcomponent.h"
#include "apl/component/componentpropdef.h"
#include "apl/component/yogaproperties.h"
#include "apl/content/rootconfig.h"
#include "apl/engine/event.h"
#include "apl/focus/focusmanager.h"
#include "apl/primitives/unicode.h"
#ifdef SCENEGRAPH
#include "apl/scenegraph/builder.h"
#include "apl/scenegraph/edittextconfig.h"
#include "apl/scenegraph/edittext.h"
#include "apl/scenegraph/edittextfactory.h"
#include "apl/scenegraph/scenegraphupdates.h"
#include "apl/scenegraph/edittextbox.h"
#include "apl/scenegraph/textchunk.h"
#include "apl/scenegraph/textlayout.h"
#include "apl/scenegraph/textmeasurement.h"
#include "apl/scenegraph/utilities.h"
#endif // SCENEGRAPH
#include "apl/time/sequencer.h"
#include "apl/touch/pointerevent.h"

namespace apl {

CoreComponentPtr
EditTextComponent::create(const ContextPtr& context,
                          Properties&& properties,
                          const Path& path)
{
    auto ptr = std::make_shared<EditTextComponent>(context, std::move(properties), path);
    ptr->initialize();
    return ptr;
}

EditTextComponent::EditTextComponent(const ContextPtr& context,
                                     Properties&& properties,
                                     const Path& path)
        : ActionableComponent(context, std::move(properties), path)
{
#ifdef SCENEGRAPH
    static auto sgTextMeasureFunc = [](YGNodeRef node, float width, YGMeasureMode widthMode,
                                       float height, YGMeasureMode heightMode) -> YGSize {
        // TODO: Hash this properly so we don't call it multiple times
        auto self = static_cast<EditTextComponent *>(node->getContext());
        return self->measureEditText(
            MeasureRequest(width, toMeasureMode(widthMode), height, toMeasureMode(heightMode)));
    };

    static auto sgTextBaselineFunc = [](YGNodeRef node, float width, float height) -> float {
        auto self = static_cast<EditTextComponent*>(node->getContext());
        return self->baselineText(width, height);
    };

    if (context->getRootConfig().getEditTextFactory()) {
        YGNodeSetMeasureFunc(mYGNodeRef, sgTextMeasureFunc);
        YGNodeSetBaselineFunc(mYGNodeRef, sgTextBaselineFunc);
        YGNodeSetNodeType(mYGNodeRef, YGNodeTypeDefault);
    } else {
#endif // SCENEGRAPH
        YGNodeSetMeasureFunc(mYGNodeRef, textMeasureFunc);
        YGNodeSetBaselineFunc(mYGNodeRef, textBaselineFunc);
        YGNodeSetNodeType(mYGNodeRef, YGNodeTypeText);
#ifdef SCENEGRAPH
    }
#endif // SCENEGRAPH
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
    ActionableComponent::assignProperties(propDefSet);
    calculateDrawnBorder(false);

    // Force the text to match the valid characters
    const auto& current = getCalculated(kPropertyText).getString();
    const auto& valid = getCalculated(kPropertyValidCharacters).getString();
    auto text = utf8StripInvalid(current, valid);
    utf8StringTrim(text, getCalculated(kPropertyMaxLength).getInteger());

    if (text != current)
        mCalculated.set(kPropertyText, text);

    // Calculate initial measurement hash.
    fixTextMeasurementHash();
}

void
EditTextComponent::preLayoutProcessing(bool useDirtyFlag)
{
    ActionableComponent::preLayoutProcessing(useDirtyFlag);

    // Update text measurement hash as some properties may have changed it
    // and we actually need it on layout time
    fixTextMeasurementHash();
}

static inline Object defaultFontColor(Component& component, const RootConfig& rootConfig)
{
    return Object(rootConfig.getDefaultFontColor(component.getContext()->getTheme()));
}

static inline Object defaultFontFamily(Component& component, const RootConfig& rootConfig)
{
    return Object(rootConfig.getDefaultFontFamily());
}

static inline Object inheritLang(Component& comp, const RootConfig& rconfig)
{
    return Object(comp.getContext()->getLang());
};

static inline Object defaultHighlightColor(Component& component, const RootConfig& rootConfig)
{
    return Object(rootConfig.getDefaultHighlightColor(component.getContext()->getTheme()));
}

const ComponentPropDefSet&
EditTextComponent::propDefSet() const
{
    // Private convenience routine to re-scan text, update the stored valued, and let the
    // edit layout know that the text has changed.
    static auto checkText = [](EditTextComponent& self) {
        const auto& unstripped = self.getCalculated(kPropertyText).getString();
        auto stripped = utf8StripInvalidAndTrim(unstripped,
                                                self.getCalculated(kPropertyValidCharacters).getString(),
                                                self.getCalculated(kPropertyMaxLength).getInteger());
        if (stripped != unstripped) {
            self.mCalculated.set(kPropertyText, stripped);
            self.setDirty(kPropertyText);
        }
    };

    static auto hintTextChanged = [](Component& component) -> void {
#ifdef SCENEGRAPH
        auto& self = static_cast<EditTextComponent&>(component);
        self.mHintText = nullptr;
        self.mHintLayout = nullptr;
#endif // SCENEGRAPH
    };

    static auto textChanged = [](Component& component) -> void {
        auto& self = static_cast<EditTextComponent&>(component);
        checkText(self);
    };

    static auto clearEditConfig = [](Component& component) -> void {
#ifdef SCENEGRAPH
        auto& self = static_cast<EditTextComponent&>(component);
        self.mEditTextConfig = nullptr;
#endif // SCENEGRAPH
    };

    static auto clearEditConfigAndCheckText = [](Component& component) -> void {
        auto& self = static_cast<EditTextComponent&>(component);
#ifdef SCENEGRAPH
        self.mEditTextConfig = nullptr;
#endif // SCENEGRAPH
        checkText(self);
    };

    static auto hintStyleOrWeightChanged = [](Component& component) -> void {
#ifdef SCENEGRAPH
        auto& self = static_cast<EditTextComponent&>(component);
        self.mHintLayout = nullptr;
        self.mHintTextProperties = nullptr;
#endif // SCENEGRAPH
    };

    static auto styleOrWeightChanged = [](Component& component) -> void {
#ifdef SCENEGRAPH
        auto& self = static_cast<EditTextComponent&>(component);
        self.mEditTextProperties = nullptr;
        self.mEditTextBox = nullptr;
        self.mEditTextConfig = nullptr;

        if (!self.mLastMeasureRequest.isExact())
            YGNodeMarkDirty(self.mYGNodeRef);
#endif // SCENEGRAPH
    };

    static auto familyOrSizeChanged = [](Component& component) -> void {
#ifdef SCENEGRAPH
        auto& self = static_cast<EditTextComponent&>(component);
        self.mHintLayout = nullptr;
        self.mHintTextProperties = nullptr;
        self.mEditTextProperties = nullptr;
        self.mEditTextBox = nullptr;
        self.mEditTextConfig = nullptr;

        if (!self.mLastMeasureRequest.isExact())
            YGNodeMarkDirty(self.mYGNodeRef);
#endif // SCENEGRAPH
    };

    static auto sizeChanged = [](Component& component) -> void {
#ifdef SCENEGRAPH
        auto& self = static_cast<EditTextComponent&>(component);
        self.mEditTextBox = nullptr;

        if (!self.mLastMeasureRequest.isExact())
            YGNodeMarkDirty(self.mYGNodeRef);
#endif // SCENEGRAPH
    };

    /*
     * What affects the layout on the screen?
     *
     * First, if the last measure mode was EXACTLY, then we don't need another global layout
     * If the last measure mode was UNDEFINED or AT_MOST then we probably _do_ need another global layout
     *
     * Second, the following properties can actually change the measurements:
     *
     *   FontFamily     FontWeight    BorderWidth
     *   FontSize       FontStyle     Size
     *
     * Note that BorderWidth directy calls yn::setBorder<YGEdgeAll>, so it will trigger a layout
     * pass if needed.
     *
     * We'll assume that the hint never affects the global layout (it just has to fit in the box)
     */
    static ComponentPropDefSet sEditTextComponentProperties(ActionableComponent::propDefSet(), {
            {kPropertyBorderColor,              Color(),                        asColor,                        kPropInOut | kPropStyled | kPropDynamic | kPropVisualHash},
            {kPropertyBorderWidth,              Dimension(0),                   asNonNegativeAbsoluteDimension, kPropInOut | kPropStyled | kPropDynamic,                                yn::setBorder<YGEdgeAll>, resolveDrawnBorder},
            {kPropertyColor,                    Color(),                        asColor,                        kPropInOut | kPropStyled | kPropDynamic | kPropVisualHash,                               clearEditConfig, defaultFontColor},
            {kPropertyFontFamily,               "",                             asString,                       kPropInOut | kPropLayout | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash, familyOrSizeChanged, defaultFontFamily},
            {kPropertyFontSize,                 Dimension(40),                  asAbsoluteDimension,            kPropInOut | kPropLayout | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash, familyOrSizeChanged},
            {kPropertyFontStyle,                kFontStyleNormal,               sFontStyleMap,                  kPropInOut | kPropLayout | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash, styleOrWeightChanged},
            {kPropertyFontWeight,               400,                            sFontWeightMap,                 kPropInOut | kPropLayout | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash, styleOrWeightChanged},
            {kPropertyHighlightColor,           Color(),                        asColor,                        kPropInOut | kPropStyled | kPropDynamic | kPropVisualHash, clearEditConfig,              defaultHighlightColor},
            {kPropertyHint,                     "",                             asString,                       kPropInOut | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash,               hintTextChanged},
            {kPropertyHintColor,                Color(),                        asColor,                        kPropInOut | kPropStyled | kPropDynamic | kPropVisualHash,                               defaultFontColor},
            {kPropertyHintStyle,                kFontStyleNormal,               sFontStyleMap,                  kPropInOut | kPropLayout | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash, hintStyleOrWeightChanged},
            {kPropertyHintWeight,               400,                            sFontWeightMap,                 kPropInOut | kPropLayout | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash, hintStyleOrWeightChanged},
            {kPropertyKeyboardType,             kKeyboardTypeNormal,            sKeyboardTypeMap,               kPropInOut | kPropStyled, clearEditConfig},
            {kPropertyLang,                     "",                             asString,                       kPropInOut | kPropLayout | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash, clearEditConfig, inheritLang},
            {kPropertyMaxLength,                0,                              asInteger,                      kPropInOut | kPropStyled | kPropTextHash | kPropVisualHash, clearEditConfigAndCheckText},
            {kPropertyOnSubmit,                 Object::EMPTY_ARRAY(),          asCommand,                      kPropIn},
            {kPropertyOnTextChange,             Object::EMPTY_ARRAY(),          asCommand,                      kPropIn},
            {kPropertySecureInput,              false,                          asBoolean,                      kPropInOut | kPropStyled | kPropDynamic, clearEditConfig},
            {kPropertyKeyboardBehaviorOnFocus,  kBehaviorOnFocusSystemDefault,  sKeyboardBehaviorOnFocusMap,    kPropIn | kPropStyled, clearEditConfig},
            {kPropertySelectOnFocus,            false,                          asBoolean,                      kPropInOut | kPropStyled, clearEditConfig },
            {kPropertySize,                     8,                              asPositiveInteger,              kPropInOut | kPropStyled | kPropLayout, sizeChanged},
            {kPropertySubmitKeyType,            kSubmitKeyTypeDone,             sSubmitKeyTypeMap,              kPropInOut | kPropStyled, clearEditConfig},
            {kPropertyText,                     "",                             asString,                       kPropInOut | kPropDynamic | kPropVisualContext | kPropTextHash | kPropVisualHash,
             textChanged},
            {kPropertyValidCharacters,          "",                             asString,                       kPropIn | kPropStyled, clearEditConfigAndCheckText},

            // The width of the drawn border.  If borderStrokeWith is set, the drawn border is the min of borderWidth
            // and borderStrokeWidth.  If borderStrokeWidth is unset, the drawn border defaults to borderWidth
            {kPropertyBorderStrokeWidth,        Object::NULL_OBJECT(),          asNonNegativeAbsoluteDimension, kPropIn | kPropStyled | kPropDynamic, resolveDrawnBorder},
            {kPropertyDrawnBorderWidth,         Object::NULL_OBJECT(),          asNonNegativeAbsoluteDimension, kPropOut | kPropVisualHash},
    });

    return sEditTextComponentProperties;
}

const EventPropertyMap&
EditTextComponent::eventPropertyMap() const
{
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
EditTextComponent::update(UpdateType type, float value)
{
    if (type == kUpdateSubmit) {
        ContextPtr eventContext = createEventContext("Submit");
        auto commands = getCalculated(kPropertyOnSubmit);
        mContext->sequencer().executeCommands(commands, eventContext, shared_from_corecomponent(), false);

    } else
    CoreComponent::update(type, value);
}

void
EditTextComponent::update(UpdateType type, const std::string& value)
{
    if (type == kUpdateTextChange) {
        auto requestedValue = Object(value);
        auto currentValue = mCalculated.get(kPropertyText);
        if (requestedValue != currentValue) {
            if (getRootConfig().experimentalFeatureEnabled(RootConfig::kExperimentalFeatureMarkEditTextDirtyOnUpdate)) {
                setProperty(kPropertyText, value);
            } else {
                mCalculated.set(kPropertyText, value);
            }
            ContextPtr eventContext = createEventContext("TextChange");
            auto commands = getCalculated(kPropertyOnTextChange);
            mContext->sequencer().executeCommands(commands, eventContext, shared_from_corecomponent(), false);
        }
    } else
        CoreComponent::update(type, value);
}

bool
EditTextComponent::isCharacterValid(const wchar_t wc) const
{
    return wcharValidCharacter(wc, getCalculated(kPropertyValidCharacters).getString());
}

PointerCaptureStatus
EditTextComponent::processPointerEvent(const PointerEvent& event, apl_time_t timestamp)
{
    auto pointerStatus = ActionableComponent::processPointerEvent(event, timestamp);
    if (pointerStatus != kPointerStatusNotCaptured)
        return pointerStatus;

    if (getRootConfig().experimentalFeatureEnabled(RootConfig::kExperimentalFeatureFocusEditTextOnTap) &&
            event.pointerEventType == kPointerUp) {
        auto self = shared_from_corecomponent();
        if (self != getContext()->focusManager().getFocus()) {
            getContext()->focusManager().setFocus(self, true);
            return kPointerStatusPendingCapture;
        }
    }
    if (getRootConfig().experimentalFeatureEnabled(RootConfig::kExperimentalFeatureRequestKeyboard) &&
            event.pointerEventType == kPointerUp) {
        getContext()->pushEvent(Event(kEventTypeOpenKeyboard, shared_from_this()));
        return kPointerStatusPendingCapture;
    }

    return kPointerStatusNotCaptured;
}

void
EditTextComponent::executeOnBlur()
{
    ActionableComponent::executeOnBlur();
#ifdef SCENEGRAPH
    if (mEditText)
        mEditText->setFocus(false);
#endif // SCENEGRAPH
}

void
EditTextComponent::executeOnFocus() {
    ActionableComponent::executeOnFocus();

    if (getCalculated(kPropertyKeyboardBehaviorOnFocus) == kBehaviorOnFocusOpenKeyboard) {
        getContext()->pushEvent(Event(kEventTypeOpenKeyboard, shared_from_this()));
    }

#ifdef SCENEGRAPH
    if (mEditText)
        mEditText->setFocus(true);
#endif // SCENEGRAPH
}

#ifdef SCENEGRAPH
YGSize
EditTextComponent::measureEditText(MeasureRequest&& request)
{
    if (request != mLastMeasureRequest) {
        mLastMeasureRequest = std::move(request);
        mEditTextBox = nullptr;
    }

    if (!mEditTextBox) {
        ensureEditTextProperties();

        assert(mContext->measure()->sceneGraphCompatible());
        auto measure = std::static_pointer_cast<sg::TextMeasurement>(mContext->measure());

        mEditTextBox = measure->box(
            getCalculated(kPropertySize).getInteger(),
            mEditTextProperties,
            mLastMeasureRequest.width(),
            mLastMeasureRequest.widthMode(),
            mLastMeasureRequest.height(),
            mLastMeasureRequest.heightMode());
    }

    if (!mEditTextBox)   // No box, no layout
        return {0, 0};

    auto size = mEditTextBox->getSize();
    return YGSize{ size.getWidth(), size.getHeight() };
}

float
EditTextComponent::baselineText(float width, float height)
{
    // This should only be called if the component has been laid out.
    return mEditTextBox ? mEditTextBox->getBaseline() : 0.0f;
}

/*
 * The EditText scene graph structure:
 *
 * Layer
 *   - Content
 *        DrawNode: Border
 *        TransformNode
 *            DrawText: Hint    (set color to transparent if it is hidden)
 *   - Children
 *        Layer
 *            EditTextNode:
 */
sg::LayerPtr
EditTextComponent::constructSceneGraphLayer(sg::SceneGraphUpdates& sceneGraph)
{
    assert(!mEditText);  // The edit text is only constructed once

    ensureEditTextBox();
    ensureEditConfig();
    ensureHintLayout();

    // Construct the edit text
    auto factory = mContext->getRootConfig().getEditTextFactory();
    assert(factory);

    // Note: We pass "this" to the callbacks because this class is careful to release()
    // the EditText before it is destroyed
    mEditText = factory->createEditText(
        [&]() {
            // Run the "onSubmit" callbacks (if there are any)
            auto commands = mCalculated.get(kPropertyOnSubmit);
            if (commands.isArray() && commands.size() > 0) {
                auto eventContext = createEventContext("Submit");
                mContext->sequencer().executeCommands(commands, eventContext,
                                                      shared_from_corecomponent(), false);
            }
        },
        [&](const std::string& text) {
            // Set the text. Only run the event handler if it actually changed
            auto old = mCalculated.get(kPropertyText);
            setProperty(kPropertyText, text);
            if (old != mCalculated.get(kPropertyText)) {
                auto commands = mCalculated.get(kPropertyOnTextChange);
                if (commands.isArray() && commands.size() > 0) {
                    auto eventContext = createEventContext("TextChange");
                    mContext->sequencer().executeCommands(commands, eventContext,
                                                          shared_from_corecomponent(), false);
                }
            }
        },
        [&](bool focused) {
            // Inform the focus manager of a potential focus change
            if (focused)
                mContext->focusManager().setFocus(shared_from_corecomponent(), false);
            else
                mContext->focusManager().releaseFocus(shared_from_corecomponent(), false);
        });

    // Build the scene graph
    auto layer = ActionableComponent::constructSceneGraphLayer(sceneGraph);
    assert(layer);

    // The first content node draws the outline
    auto size = getCalculated(kPropertyBounds).get<Rect>().getSize();
    auto outline = Rect{0, 0, size.getWidth(), size.getHeight()};
    auto strokeWidth = getCalculated(kPropertyDrawnBorderWidth).asFloat();
    auto content = sg::draw(sg::path(RoundedRect{outline, 0}, strokeWidth),
                        sg::fill(sg::paint(getCalculated(kPropertyBorderColor))));

    // The second content node draws the hint.  The hint is transparent if the edit control
    // has text to display.
    // TODO: Fix auto for RTOL text based on language
    Color hintColor = getCalculated(kPropertyText).empty()
                          ? getCalculated(kPropertyHintColor).getColor() : Color::TRANSPARENT;
    auto innerBounds = getCalculated(kPropertyInnerBounds).get<Rect>();
    auto hintSize = mHintLayout->getSize();

    content->setNext(sg::transform(
        Point{innerBounds.getLeft(), innerBounds.getCenterY() - hintSize.getHeight() / 2},
        sg::text(mHintLayout, sg::fill(sg::paint(hintColor)))));

    layer->setContent(content);

    // Construct an inner layer for the actual edit text
    auto innerLayer = sg::layer(mUniqueId + "-innerEditText", innerBounds, 1.0, Transform2D());
    innerLayer->setContent(sg::editText(mEditText.getPtr(), mEditTextBox,
                                        mEditTextConfig, getCalculated(kPropertyText).getString()));
    innerLayer->clearFlags();

    layer->appendChild(innerLayer);

    return layer;
}

bool
EditTextComponent::updateSceneGraphInternal(sg::SceneGraphUpdates& sceneGraph)
{
    auto result = false;

    // Fix the border
    const auto outlineChanged          = isDirty(kPropertyBounds);
    const auto borderWidthChanged      = isDirty(kPropertyBorderWidth);
    const auto drawnBorderWidthChanged = isDirty(kPropertyDrawnBorderWidth);
    const auto borderColorChanged      = isDirty(kPropertyBorderColor);

    if (outlineChanged || borderWidthChanged || drawnBorderWidthChanged || borderColorChanged) {
        auto* draw = sg::DrawNode::cast(mSceneGraphLayer->content());
        auto* path = sg::FramePath::cast(draw->getPath());

        if (outlineChanged || borderWidthChanged || drawnBorderWidthChanged) {
            auto size = getCalculated(kPropertyBounds).get<Rect>().getSize();
            auto outline = RoundedRect(Rect{0,0,size.getWidth(), size.getHeight()}, 0);
            result |= path->setRoundedRect(outline);
            result |= path->setInset(getCalculated(kPropertyDrawnBorderWidth).asFloat());
        }

        if (borderColorChanged) {
            auto* paint = sg::ColorPaint::cast(draw->getOp()->paint);
            result |= paint->setColor(getCalculated(kPropertyBorderColor).getColor());
        }
    }

    const bool fixInnerBounds = isDirty(kPropertyInnerBounds);
    const auto& innerBounds = getCalculated(kPropertyInnerBounds).get<Rect>();

    const auto fixText = isDirty(kPropertyText);

    // Fix the hint.  Note: If the text has changed, the hint color may need to turn on or off
    const auto fixHintLayout = ensureHintLayout();
    const bool fixHintColor = isDirty(kPropertyHintColor) || fixText;

    if (fixInnerBounds || fixHintLayout || fixHintColor) {
        auto* transform = sg::TransformNode::cast(mSceneGraphLayer->content()->next());
        auto* text = sg::TextNode::cast(transform->child());

        if (fixInnerBounds || fixHintLayout) {
            auto hintSize = mHintLayout->getSize();
            result |= transform->setTransform(Transform2D::translate(
                innerBounds.getLeft(), innerBounds.getCenterY() - hintSize.getHeight() / 2));
        }

        if (fixHintLayout)
            result |= text->setTextLayout(mHintLayout);

        auto hintColor = getCalculated(kPropertyText).empty()
                             ? getCalculated(kPropertyHintColor).getColor()
                             : Color::TRANSPARENT;

        // Redraw the content if the hint color changes (even if there is no hint)
        result |= sg::ColorPaint::cast(text->getOp()->paint)->setColor(hintColor);
    }

    // Fix the edit text.  This is in an inner layer, so the "result" variable is not changed
    const auto fixEditConfig = ensureEditConfig();
    const auto fixEditBox = ensureEditTextBox();

    if (fixInnerBounds || fixEditConfig || fixText || fixEditBox) {
        auto innerLayer = mSceneGraphLayer->children().at(0);
        auto *editNode = sg::EditTextNode::cast(innerLayer->content());

        if (fixInnerBounds)
            innerLayer->setBounds(innerBounds);

        if (fixEditConfig && editNode->setEditTextConfig(mEditTextConfig))
            innerLayer->setFlag(sg::Layer::kFlagRedrawContent);

        if (fixText && editNode->setText(getCalculated(kPropertyText).getString()))
            innerLayer->setFlag(sg::Layer::kFlagRedrawContent);

        if (fixEditBox && editNode->setEditTextBox(mEditTextBox))
            innerLayer->setFlag(sg::Layer::kFlagRedrawContent);

        // Transfer any layer changes to the change map
        sceneGraph.changed(innerLayer);
    }

    return result;
}


bool
EditTextComponent::ensureEditTextBox()
{
    if (mEditTextBox)
        return false;

    ensureEditTextProperties();

    assert(mContext->measure()->sceneGraphCompatible());
    auto measure = std::static_pointer_cast<sg::TextMeasurement>(mContext->measure());

    const auto& innerBounds = getCalculated(kPropertyInnerBounds).get<Rect>();

    mEditTextBox = measure->box(getCalculated(kPropertySize).getInteger(),
                                mEditTextProperties,
                                innerBounds.getWidth(),
                                MeasureMode::AtMost,
                                innerBounds.getHeight(),
                                MeasureMode::AtMost);

    return true;
}

bool
EditTextComponent::ensureEditConfig()
{
    if (mEditTextConfig)
        return false;

    ensureEditTextProperties();

    mEditTextConfig = sg::EditTextConfig::create(
        getCalculated(kPropertyColor).getColor(),
        getCalculated(kPropertyHighlightColor).getColor(),
        getCalculated(kPropertyKeyboardType).asEnum<KeyboardType>(),
        getCalculated(kPropertyLang).asString(),
        getCalculated(kPropertyMaxLength).asInt(),
        getCalculated(kPropertySecureInput).asBoolean(),
        getCalculated(kPropertySubmitKeyType).asEnum<SubmitKeyType>(),
        getCalculated(kPropertyValidCharacters).asString(),
        getCalculated(kPropertySelectOnFocus).asBoolean(),
        getCalculated(kPropertyKeyboardBehaviorOnFocus).asEnum<KeyboardBehaviorOnFocus>(),
        mEditTextProperties);

    return true;
}

bool
EditTextComponent::ensureEditTextProperties()
{
    if (mEditTextProperties)
        return false;

    mEditTextProperties = sg::TextProperties::create(
        mContext->textPropertiesCache(),
        sg::splitFontString(mContext->getRootConfig(),
                            getCalculated(kPropertyFontFamily).getString()),
        getCalculated(kPropertyFontSize).asFloat(),
        getCalculated(kPropertyFontStyle).asEnum<FontStyle>(),
        getCalculated(kPropertyFontWeight).getInteger());

    return true;
}

/**
 * Ensure that the hint layout has been constructed.  This should only be called after a layout
 * pass so that we have a valid "innerBounds" property.
 */
bool
EditTextComponent::ensureHintLayout()
{
    if (mHintLayout)
        return false;

    if (!mHintText)
        mHintText = sg::TextChunk::create(getCalculated(kPropertyHint).getString());

    const auto* context = mContext.get();
    assert(context);

    if (!mHintTextProperties)
        mHintTextProperties = sg::TextProperties::create(
            context->textPropertiesCache(),
            sg::splitFontString(context->getRootConfig(),
                                getCalculated(kPropertyFontFamily).getString()),
            getCalculated(kPropertyFontSize).asFloat(),
            getCalculated(kPropertyHintStyle).asEnum<FontStyle>(),
            getCalculated(kPropertyHintWeight).getInteger(),
            0,     // Letter spacing
            1.25f, // Line height
            1      // Max lines (force hint on a single line
        );

    auto borderWidth = getCalculated(kPropertyBorderWidth).asFloat();
    auto innerBounds = getCalculated(kPropertyInnerBounds).get<Rect>().inset(borderWidth);
    assert(mContext->measure()->sceneGraphCompatible());
    auto measure = std::static_pointer_cast<sg::TextMeasurement>(mContext->measure());

    mHintLayout =
        measure->layout(mHintText, mHintTextProperties, innerBounds.getWidth(), MeasureMode::AtMost,
                        innerBounds.getHeight(), MeasureMode::AtMost);
    return true;
}
#endif // SCENEGRAPH

}  // namespace apl
