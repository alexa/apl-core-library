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

#include "apl/component/textcomponent.h"

#include "apl/component/componentpropdef.h"
#include "apl/component/textmeasurement.h"
#include "apl/content/rootconfig.h"
#include "apl/primitives/styledtext.h"
#include "apl/time/sequencer.h"
#include "apl/utils/session.h"
#include "apl/utils/tracing.h"
#include "apl/yoga/yoganode.h"
#include "apl/yoga/yogaproperties.h"

#include "apl/scenegraph/textchunk.h"
#include "apl/scenegraph/textlayout.h"
#include "apl/scenegraph/textlayoutcache.h"
#include "apl/scenegraph/textmeasurement.h"
#include "apl/scenegraph/textproperties.h"
#include "apl/scenegraph/utilities.h"

#ifdef SCENEGRAPH
#include "apl/scenegraph/builder.h"
#include "apl/scenegraph/scenegraph.h"
#endif // SCENEGRAPH

namespace apl {

const static bool DEBUG_MEASUREMENT = false;

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
    : CoreComponent(context, std::move(properties), path),
      mLayoutPossiblyStale(false)
{
    mYogaNode.setMeasureFunc();
    mYogaNode.setBaselineFunc();
    mYogaNode.setNodeTypeText();
}


const ComponentPropDefSet&
TextComponent::propDefSet() const
{
    static auto defaultFontFamily = [](Component& component, const RootConfig& rootConfig) -> Object {
      return rootConfig.getProperty(RootProperty::kDefaultFontFamily);
    };

    static auto defaultFontColor = [](Component& component, const RootConfig& rootConfig) -> Object {
      return Object(rootConfig.getDefaultFontColor(component.getContext()->getTheme()));
    };

    static auto defaultLang = [](Component& component, const RootConfig& rootConfig) -> Object {
        return component.getContext()->getLang();
    };

    static auto karaokeTargetColorTrigger = [](Component& component) -> void {
        auto& text = (TextComponent&)component;
        text.checkKaraokeTargetColor();
    };

    static auto fixTextAlignTrigger = [](Component& component) -> void {
      auto& textComp = (TextComponent&)component;
      textComp.updateTextAlign(true);
      textComp.mTextProperties = nullptr;
      textComp.mLayout = nullptr;
    };

    static auto fixTextTrigger = [](Component& component) -> void {
      auto& textComp = (TextComponent&)component;
      textComp.mTextProperties = nullptr;
      textComp.mLayout = nullptr;
    };

    static auto fixTextChunkTrigger = [](Component& component) -> void {
        auto& textComp = (TextComponent&)component;
        textComp.mTextChunk = nullptr;
        textComp.mLayout = nullptr;
    };

    static ComponentPropDefSet sTextComponentProperties(CoreComponent::propDefSet(), {
        {kPropertyColor,               Color(),                 asColor,               kPropInOut | kPropStyled | kPropDynamic | kPropVisualHash, karaokeTargetColorTrigger,    defaultFontColor},
        {kPropertyRangeKaraokeTarget,  Range(),                 nullptr,               kPropOut | kPropVisualHash  },
        {kPropertyColorKaraokeTarget,  Color(),                 asColor,               kPropOut | kPropVisualHash,                                                              defaultFontColor},
        {kPropertyColorNonKaraoke,     Color(),                 asColor,               kPropOut | kPropVisualHash,                                                              defaultFontColor},
        {kPropertyFontFamily,          "",                      asString,              kPropInOut | kPropLayout | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash, fixTextTrigger, defaultFontFamily},
        {kPropertyFontSize,            Dimension(40),           asAbsoluteDimension,   kPropInOut | kPropLayout | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash, fixTextTrigger},
        {kPropertyFontStyle,           kFontStyleNormal,        sFontStyleMap,         kPropInOut | kPropLayout | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash, fixTextTrigger},
        {kPropertyFontWeight,          400,                     sFontWeightMap,        kPropInOut | kPropLayout | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash, fixTextTrigger},
        {kPropertyLang,                "",                      asString,              kPropInOut | kPropLayout | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash, fixTextTrigger, defaultLang},
        {kPropertyLetterSpacing,       Dimension(0),            asAbsoluteDimension,   kPropInOut | kPropLayout | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash, fixTextTrigger},
        {kPropertyLineHeight,          1.25,                    asNonNegativeNumber,   kPropInOut | kPropLayout | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash, fixTextTrigger},
        {kPropertyMaxLines,            0,                       asInteger,             kPropInOut | kPropLayout | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash, fixTextTrigger},
        {kPropertyOnTextLayout,        Object::EMPTY_ARRAY(),   asCommand,             kPropIn},
        {kPropertyText,                StyledText::EMPTY(),     asStyledText,          kPropInOut | kPropLayout | kPropDynamic | kPropVisualContext | kPropTextHash | kPropVisualHash, fixTextChunkTrigger},
        {kPropertyTextAlign,           kTextAlignAuto,          sTextAlignMap,         kPropOut | kPropTextHash | kPropVisualHash,                                              fixTextTrigger},
        {kPropertyTextAlignAssigned,   kTextAlignAuto,          sTextAlignMap,         kPropIn  | kPropLayout | kPropStyled | kPropDynamic,                                     fixTextAlignTrigger},
        {kPropertyTextAlignVertical,   kTextAlignVerticalAuto,  sTextAlignVerticalMap, kPropInOut | kPropLayout | kPropStyled | kPropDynamic | kPropTextHash | kPropVisualHash, fixTextTrigger},
    });

    return sTextComponentProperties;
}

const EventPropertyMap&
TextComponent::eventPropertyMap() const
{
    static EventPropertyMap sTextEventProperties = eventPropertyMerge(
            CoreComponent::eventPropertyMap(),
            {
                    {"text",  [](const CoreComponent *c) { return c->getCalculated(kPropertyText).get<StyledText>().getText(); }},
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
 *
 *
 * The Karaoke Range specifies where to draw the kPropertyColorKaraokeTarget
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

    if (textAlign == kTextAlignStart || textAlign == kTextAlignAuto) {
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

std::string
TextComponent::getVisualContextType() const
{
    return getValue().empty() ? VISUAL_CONTEXT_TYPE_EMPTY : VISUAL_CONTEXT_TYPE_TEXT;
}

Size
TextComponent::textMeasure(float width, MeasureMode widthMode, float height, MeasureMode heightMode)
{
    APL_TRACE_BEGIN("TextComponent::textMeasure");
    auto tm = std::static_pointer_cast<sg::TextMeasurement>(mContext->measure());

    ensureTextProperties();

    auto hash = mTextProperties->hash();
    hashCombine(hash, mTextChunk->hash());
    TextMeasureRequest tmr = {width, widthMode, height, heightMode, hash};

    LOG_IF(DEBUG_MEASUREMENT).session(mContext)
        << "Measuring: " << getUniqueId()
        << " hash: " << hash
        << " width: " << width
        << " widthMode: " << widthMode
        << " height: " << height
        << " heightMode: " << heightMode;

    auto& layoutCache = getContext()->textLayoutCache();
    auto layout = layoutCache.find(tmr);
    if (layout) {
        mLayout = layout;
    } else {
        APL_TRACE_BEGIN("CoreComponent:textMeasureInternal:runtimeMeasure");
        mLayout = tm->layout(this, mTextChunk, mTextProperties, width, widthMode, height, heightMode);
        APL_TRACE_END("CoreComponent:textMeasureInternal:runtimeMeasure");
        layoutCache.insert(tmr, mLayout);
    }

    if (!mLayout)
        return {0, 0};  // No text, no layout

    auto size = mLayout->getSize();

    onTextLayout();

    LOG_IF(DEBUG_MEASUREMENT).session(mContext) << "Size: " << size.getWidth() << "x" << size.getHeight();
    return size;
}

float
TextComponent::textBaseline(float width, float height)
{
    APL_TRACE_BEGIN("TextComponent::textBaseline");
    // Make the large assumption that Yoga needs baseline information with a text layout
    if (!mLayout) {
        auto tm = std::static_pointer_cast<sg::TextMeasurement>(mContext->measure());
        ensureTextProperties();

        APL_TRACE_BEGIN("CoreComponent:textBaselineInternal:runtimeMeasure");
        mLayout = tm->layout(this, mTextChunk, mTextProperties, width, MeasureMode::Undefined,
                             height, MeasureMode::Undefined);
        APL_TRACE_END("CoreComponent:textBaselineInternal:runtimeMeasure");
    }

    return mLayout ? mLayout->getBaseline() : 0;
}

void
TextComponent::ensureTextLayout()
{
    // Having a layout is no guarantee that the layout is correct.  Yoga caches previous layout calculations
    // for efficiency.  If the layout exists, you also need to check that it matches the desired size and scaling modes.
    if (mLayout) {
        if (!mLayoutPossiblyStale) return;
        mLayoutPossiblyStale = false;

        // Yoga rounds text box sizes.  They should be rounded UP (to ensure no clipping).
        // They round to the nearest pixel dimension.  We'll assume that if the layout size
        // is within 2 pixels of the measured size and _larger_ than the measured size, then
        // we are okay and don't need to re-layout.
        auto s = mLayout->getSize();
        auto dw = mContext->dpToPx(mYogaNode.getWidth() - s.getWidth());
        auto dh = mContext->dpToPx(mYogaNode.getHeight() - s.getHeight());

        if (dw >= 0.0 && dw <= 2.0 && dh >= 0.0 && dh <= 2.0)
            return;
    }

    auto measure = std::static_pointer_cast<sg::TextMeasurement>(mContext->measure());

    const auto& innerBounds = getCalculated(kPropertyInnerBounds).get<Rect>();

    ensureTextProperties();
    mLayout = measure->layout(this, mTextChunk, mTextProperties, innerBounds.getWidth(),
                              MeasureMode::AtMost, innerBounds.getHeight(), MeasureMode::AtMost);

    if (!mLayout) return;

    onTextLayout();
}

void
TextComponent::ensureTextProperties()
{
    if (!mTextChunk)
        mTextChunk = sg::TextChunk::create(getCalculated(kPropertyText).get<StyledText>());

    if (!mTextProperties)
        mTextProperties = sg::TextProperties::create(
            mContext->textPropertiesCache(),
            sg::splitFontString(mContext->getRootConfig(),
                                mContext->session(),
                                getCalculated(kPropertyFontFamily).getString()),
            getCalculated(kPropertyFontSize).asFloat(),
            getCalculated(kPropertyFontStyle).asEnum<FontStyle>(),
            getCalculated(kPropertyLang).getString(),
            getCalculated(kPropertyFontWeight).getInteger(),
            getCalculated(kPropertyLetterSpacing).asFloat(),
            getCalculated(kPropertyLineHeight).asFloat(),
            getCalculated(kPropertyMaxLines).getInteger(),
            getCalculated(kPropertyTextAlign).asEnum<TextAlign>(),
            getCalculated(kPropertyTextAlignVertical).asEnum<TextAlignVertical>());
}

void
TextComponent::onTextLayout() {
    assert(mLayout);
    auto commands = mCalculated.get(kPropertyOnTextLayout);
    if (!commands.empty()) {
        auto propMap = std::make_shared<ObjectMap>();
        auto size = mLayout->getSize();
        propMap->emplace("isTruncated", mLayout->isTruncated());
        propMap->emplace("laidOutText", mLayout->getLaidOutText());
        propMap->emplace("textWidth", size.getWidth());
        propMap->emplace("textHeight", size.getHeight());
        mContext->sequencer().executeCommands(commands, createEventContext("TextLayout", propMap, getValue()), shared_from_corecomponent(), true);
    }
}

#ifdef SCENEGRAPH
Point
TextComponent::calcSceneGraphOffset() const
{
    const auto& innerBounds = getCalculated(kPropertyInnerBounds).get<Rect>();
    auto textSize = mLayout ? mLayout->getSize() : Size{};

    float dx = 0;
    switch (static_cast<TextAlign>(getCalculated(kPropertyTextAlign).getInteger())) {
        case kTextAlignAuto:
        case kTextAlignLeft:  // TODO: Fix auto for rtol text
        case kTextAlignStart: // TODO: Fix for RTOL text
            dx = innerBounds.getLeft();
            break;
        case kTextAlignCenter:
            dx = innerBounds.getCenterX() - textSize.getWidth() / 2;
            break;
        case kTextAlignRight:
        case kTextAlignEnd: // TODO: Fix for RTOL text
            dx = innerBounds.getRight() - textSize.getWidth();
            break;
    }

    float dy = 0;
    switch (
        static_cast<TextAlignVertical>(getCalculated(kPropertyTextAlignVertical).getInteger())) {
        case kTextAlignVerticalAuto: // TODO: Fix auto for text
        case kTextAlignVerticalTop:
            dy = innerBounds.getTop();
            break;
        case kTextAlignVerticalCenter:
            dy = innerBounds.getCenterY() - textSize.getHeight() / 2;
            break;
        case kTextAlignVerticalBottom:
            dy = innerBounds.getBottom() - textSize.getHeight();
            break;
    }

    return {dx, dy};
}

/**
 * Construct the scene graph.
 *
 * In most cases we create a single TextNode wrapped by a TransformNode.
 * In karaoke we need to create between one and three nodes for highlighting individual lines.
 *
 * @param sceneGraph Screen graph update tracking.
 * @return The scene graph
 */
sg::LayerPtr
TextComponent::constructSceneGraphLayer(sg::SceneGraphUpdates& sceneGraph) {
    auto layer = CoreComponent::constructSceneGraphLayer(sceneGraph);
    assert(layer);
    
    layer->setCharacteristic(sg::Layer::kCharacteristicHasText);

    ensureTextLayout();

    auto transform = sg::transform(calcSceneGraphOffset(), nullptr);
    populateTextNodes(sg::TransformNode::cast(transform));
    layer->setContent(transform);

    return layer;
}

/*
 * This method is called if the TextComponent has a pre-existing scene graph.
 * Nominally "isDirty(PROPERTY)" tells us if a component property has changed and hence
 * may be need to be updated in the scene graph.  A shortcut is available since changing
 * some properties clears mLayout.
 *
 * These properties clear the layout
 *
 *   kPropertyFontFamily
 *   kPropertyFontSize
 *   kPropertyFontStyle
 *   kPropertyFontWeight
 *   kPropertyLang
 *   kPropertyLetterSpacing
 *   kPropertyLineHeight
 *   kPropertyMaxLines
 *   kPropertyTextAlign
 *   kPropertyTextAlignVertical
 *   kPropertyText
 *
 * These properties are needed to calculate paint and positioning of the text layout
 *
 *   kPropertyColor
 *   kPropertyRangeKaraokeTarget
 *   kPropertyColorKaraokeTarget
 *   kPropertyInnerBounds
 *   kPropertyBounds
 */
bool
TextComponent::updateSceneGraphInternal(sg::SceneGraphUpdates& sceneGraph)
{
    auto* transform = sg::TransformNode::cast(mSceneGraphLayer->content());
    auto* text = sg::TextNode::cast(transform->child());

    ensureTextLayout();

    const bool fixText = text->getTextLayout() != mLayout;
    const bool fixColor = isDirty(kPropertyColor);
    const bool fixKaraokeColor = isDirty(kPropertyColorKaraokeTarget);
    const bool fixKaraokeRange = isDirty(kPropertyRangeKaraokeTarget);
    const bool fixTransform = fixText || isDirty(kPropertyInnerBounds) || isDirty(kPropertyBounds);

    if (!fixColor && !fixKaraokeColor && !fixText && !fixTransform && !fixKaraokeRange)
        return false;

    if (fixTransform)
        transform->setTransform(Transform2D::translate(calcSceneGraphOffset()));

    // If Karaoke highlighting changes or if the text/color changes AND we're in
    // karaoke highlighting, then we just rebuild all of the nodes.
    if (fixKaraokeRange || (transform->childCount() > 1 && (fixColor || fixText || fixKaraokeColor))) {
        transform->removeAllChildren();
        populateTextNodes(transform);
    }
    else {
        if (fixText)
            text->setTextLayout(mLayout);
        if (fixColor) {
            auto* paint = sg::ColorPaint::cast(text->getOp()->paint);
            paint->setColor(getCalculated(kPropertyColor).getColor());
        }
    }

    return true;
}

void
TextComponent::populateTextNodes(sg::Node *transform)
{
    const auto range = getCalculated(kPropertyRangeKaraokeTarget).get<Range>();
    const auto lineCount = mLayout->getLineCount();
    const auto lineRange = lineCount > 0 ? Range(0, lineCount - 1) : Range();

    // Calculate Karaoke strips if needed
    if (!range.empty() && lineCount > 0) {
        sg::NodePtr child = nullptr;

        // Build up the child list in reverse order.  Start with lines beyond the karaoke range
        auto subrange = lineRange.subsetAbove(range.upperBound());
        if (!subrange.empty())
            child = sg::text(mLayout, sg::fill(sg::paint(getCalculated(kPropertyColor))), subrange);

        // Lines within the karaoke range
        subrange = lineRange.intersectWith(range);
        if (!subrange.empty())
            child =
                sg::text(mLayout, sg::fill(sg::paint(getCalculated(kPropertyColorKaraokeTarget))),
                         subrange)
                    ->setNext(child);

        // Identify lines BEFORE the karaoke range
        subrange = lineRange.subsetBelow(range.lowerBound());
        if (!subrange.empty())
            child = sg::text(mLayout, sg::fill(sg::paint(getCalculated(kPropertyColor))), subrange)
                        ->setNext(child);

        transform->setChild(child);
    }
    else {
        transform->setChild(sg::text(mLayout, sg::fill(sg::paint(getCalculated(kPropertyColor)))));
    }
}

#endif // SCENEGRAPH

bool
TextComponent::setKaraokeLine(Range byteRange)
{
    const auto& previousLineRange = mCalculated.get(kPropertyRangeKaraokeTarget).get<Range>();

    if (byteRange.empty()) {
        if (!previousLineRange.empty()) {
            mCalculated.set(kPropertyRangeKaraokeTarget, Range());
            setDirty(kPropertyRangeKaraokeTarget);
            return true;
        }
        return false;
    }

    // We have to ensure we have a text layout so we can find the start and end of lines
    ensureTextLayout();
    auto lineRange = mLayout->getLineRangeFromByteRange(byteRange);
    if (previousLineRange != lineRange) {
        mCalculated.set(kPropertyRangeKaraokeTarget, std::move(lineRange));
        setDirty(kPropertyRangeKaraokeTarget);
        return true;
    }

    return false;
}

Rect
TextComponent::getKaraokeBounds()
{
    const auto& range = mCalculated.get(kPropertyRangeKaraokeTarget).get<Range>();
    if (range.empty())
        return {};

    ensureTextLayout();
    return mLayout->getBoundingBoxForLines(range);
}

void
TextComponent::postProcessLayoutChanges(bool first)
{
    CoreComponent::postProcessLayoutChanges(first);
    if (isDirty(kPropertyInnerBounds)) {
        // Text layout MAY need to be changed.
        mLayoutPossiblyStale = true;
    }

    // If text layout was not populated during layout - report it now
    if (!mCalculated.get(kPropertyOnTextLayout).empty()) {
        ensureTextLayout();
    }
}

std::shared_ptr<TextComponent>
TextComponent::cast(const ComponentPtr& component)
{
    return component && component->getType() == ComponentType::kComponentTypeText
               ? std::static_pointer_cast<TextComponent>(component) : nullptr;
}

} // namespace apl
