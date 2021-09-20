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

#include "gtest/gtest.h"

/**
 * The purpose of this unit test is to verify that apl/apl.h includes
 * all of the files that a consumer will need in order to use the core
 * of APL.
 *
 * Do NOT add any more include files here!!!!
 */

#include "rapidjson/document.h"

#include "apl/apl.h"
#include "apl/engine/evaluate.h"
#include "apl/engine/rootcontext.h"
#include "apl/engine/styles.h"

#include "apl/primitives/dimension.h"
#include "apl/engine/hovermanager.h"
#include "apl/component/scrollviewcomponent.h"
#include "apl/component/sequencecomponent.h"

#include "../testeventloop.h"

using namespace apl;

static bool DEBUG_HOVER_TEST = false;

static const std::string TEXT_TEXT = "My text";
static const std::string ON_CURSOR_ENTER_TEXT = "Enter";
static const std::string ON_CURSOR_EXIT_TEXT = "Exit";

static const std::string FRAME_BORDERCOLOR = "transparent";
static const std::string FRAME_BORDERCOLOR_HOVER = "yellow";
static const std::string TEXT_COLOR = "#ff1020";
static const std::string TEXT_COLOR_HOVER = "red";

static const std::string DOCUMENT_BEGIN =
        std::string(
                "{"
                "  \"type\": \"APL\","
                "  \"version\": \"1.0\","
                "  \"styles\": {"
                "    \"frameStyle\": {"
                "      \"values\": ["
                "        {"
                "          \"borderWidth\": 2,"
                "          \"borderColor\": \"")
        + FRAME_BORDERCOLOR
        + std::string(
                "\""
                "        },"
                "        {"
                "          \"when\": \"${state.hover}\","
                "          \"borderColor\": \"")
        + FRAME_BORDERCOLOR_HOVER
        + std::string(
                "\""
                "        }"
                "      ]"
                "    },"
                "    \"textStyle\": {"
                "      \"values\": ["
                "        {"
                "          \"color\": \"")
        + TEXT_COLOR
        + std::string(
                "\""
                "        },"
                "        {"
                "          \"when\": \"${state.hover}\","
                "          \"color\": \"")
        + TEXT_COLOR_HOVER
        + std::string(
                "\""
                "        }"
                "      ]"
                "    }"
                "  },"
                "  \"mainTemplate\": {"
                "    \"item\": {"
                "      \"type\": \"TouchWrapper\","
                "      \"width\": \"100%\","
                "      \"height\": \"100%\","
                "      \"onPress\": ["
                "        {"
                "          \"type\": \"SendEvent\","
                "          \"arguments\": \"Press\""
                "        }"
                "      ],");

static const std::string DOCUMENT_END =
        "    }"
        "  }"
        "}";

static const std::string ON_CURSOR =
        std::string(
                "         ,\"onCursorEnter\": ["
                "            {"
                "              \"type\": \"SetValue\","
                "              \"componentId\": \"textComp\","
                "              \"property\": \"text\","
                "              \"value\": \"")
        + ON_CURSOR_ENTER_TEXT
        + std::string(
                "\""
                "            }"
                "          ],"
                "          \"onCursorExit\": ["
                "            {"
                "              \"type\": \"SetValue\","
                "              \"componentId\": \"textComp\","
                "              \"property\": \"text\","
                "              \"value\": \"")
        + ON_CURSOR_EXIT_TEXT
        + std::string(
                "\""
                "            }"
                "          ]");

static const std::string DOCUMENT =
        DOCUMENT_BEGIN
        + std::string(
                "      \"item\": {"
                "        \"type\": \"Frame\","
                "        \"id\": \"frameComp\"%s,"
                "        \"style\": \"frameStyle\","
                "        \"item\": {"
                "          \"type\": \"Text\","
                "          \"id\": \"textComp\","
                "          \"text\": \"")
        + TEXT_TEXT
        + std::string(
                "\"%s,"
                "          \"style\": \"textStyle\"")
        + std::string(
                "        }"
                "      }")
        + DOCUMENT_END;

class HoverTest : public DocumentWrapper {
public:
    inline CoreComponentPtr getTouchWrapper(const RootContextPtr& root) const {
        return std::static_pointer_cast<CoreComponent>(root->topComponent());
    }
    inline CoreComponentPtr getFrame(const ComponentPtr& touchWrapper) const {
        return std::static_pointer_cast<CoreComponent>(touchWrapper->getChildAt(0));
    }
    inline CoreComponentPtr getText(const ComponentPtr& cmp) const {
        return std::static_pointer_cast<CoreComponent>(cmp->getChildAt(0));
    }

    void init(const char *json) {
        loadDocument(json);
        top = getTouchWrapper(root);
        frame = getFrame(top);
        text = getText(frame);
    }

    void init(const char* frameProperties, const char* textProperties) {
        static char json[4096];
        sprintf(json, DOCUMENT.c_str(), frameProperties, textProperties);
        init(json);
        LOG_IF(DEBUG_HOVER_TEST) << json;
    }

    void validateHoverStates(bool topHoverState, bool frameHoverState, bool textHoverState) const {
        ASSERT_EQ(topHoverState, top->getState().get(kStateHover));
        ASSERT_EQ(frameHoverState, frame->getState().get(kStateHover));
        ASSERT_EQ(textHoverState, text->getState().get(kStateHover));
    }

    void validateFrame() const {
        std::string borderColor = (frame->getState().get(kStateHover)) ? FRAME_BORDERCOLOR_HOVER : FRAME_BORDERCOLOR;
        ASSERT_EQ(1, frame->getDirty().count(kPropertyBorderColor));
        ASSERT_TRUE(IsEqual(Color(session, borderColor), frame->getCalculated(kPropertyBorderColor)));
    }

    void validateFrameDisabledState(bool disabledState) const {
        ASSERT_EQ(1, frame->getDirty().count(kPropertyDisabled));
        ASSERT_EQ(disabledState, frame->getState().get(kStateDisabled));
    }

    void validateText() const {
        std::string color = (text->getState().get(kStateHover)) ? TEXT_COLOR_HOVER : TEXT_COLOR;
        ASSERT_EQ(1, text->getDirty().count(kPropertyColor));
        ASSERT_TRUE(IsEqual(Color(session, color), text->getCalculated(kPropertyColor)));
    }

    void validateTextDisabledState(bool disabledState) const {
        ASSERT_EQ(1, text->getDirty().count(kPropertyDisabled));
        ASSERT_EQ(disabledState, text->getState().get(kStateDisabled));
    }

    void validateTextString(const std::string str=TEXT_TEXT) {
        if (str != TEXT_TEXT) {
            ASSERT_EQ(1, text->getDirty().count(kPropertyText));
        }
        ASSERT_EQ(StyledText::create(*context, str), text->getCalculated(kPropertyText));
    }

    void resetTextString() {
        ASSERT_TRUE(text->setProperty(kPropertyText, TEXT_TEXT));
        validateTextString();
        root->clearDirty();
    }

    void executeScroll(const std::string& component, float distance) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "Scroll", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("distance", distance, alloc);
        doc.SetArray().PushBack(cmd, alloc);
        root->executeCommands(doc, false);
    }

    void completeScroll(const ComponentPtr& component, float distance) {
        ASSERT_FALSE(root->hasEvent());
        executeScroll(component->getId(), distance);
        advanceTime(1000);
    }

    void executeScrollToComponent(const std::string& component, CommandScrollAlign align) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "ScrollToComponent", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("align", rapidjson::StringRef(sCommandAlignMap.at(align).c_str()), alloc);
        doc.SetArray().PushBack(cmd, alloc);
        root->executeCommands(doc, false);
    }

    void printBounds(const std::string& name, const ComponentPtr& component) {
        std::cerr << "[          ] " << name << ": " << component.get() << std::endl;
        std::cerr << "[          ]\tbounds " << component->getCalculated(kPropertyBounds).getRect().toString() << std::endl;
        std::cerr << "[          ]\tinner bounds " << component->getCalculated(kPropertyInnerBounds).getRect().toString() << std::endl;
        std::cerr << "[          ]\tglobal bounds " << component->getGlobalBounds().toString() << std::endl;
    }

    void TearDown() override {
        top.reset();
        frame.reset();
        text.reset();
        DocumentWrapper::TearDown();
    }
    rapidjson::Document doc;

    CoreComponentPtr top;
    CoreComponentPtr frame;
    CoreComponentPtr text;
};

static const Point invalidPos = Point(-1,-1);
static const Point framePos = Point(1,1);
static const Point textPos = Point(4,4);

// frame display=invisible, text display=normal
TEST_F(HoverTest, DisplayFrameInvisible) {
    std::string frameProperties = ",\"display\": \"invisible\"";
    std::string textProperties = ON_CURSOR;
    init(frameProperties.c_str(), textProperties.c_str());

    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(framePos), top);
    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(textPos), top);
}

// frame display=none, text display=normal
TEST_F(HoverTest, DisplayFrameNone) {
    std::string frameProperties = ",\"display\": \"none\"";
    std::string textProperties = ON_CURSOR;
    init(frameProperties.c_str(), textProperties.c_str());

    // frame display=none, text display=normal
    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(framePos), top);
    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(textPos), top);
}

// frame display=normal, text display=invisible
TEST_F(HoverTest, DisplayTextInvisible) {
    std::string frameProperties = "";
    std::string textProperties = ",\"display\": \"invisible\"" + ON_CURSOR;
    init(frameProperties.c_str(), textProperties.c_str());

    // frame display=normal, text display=invisible
    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(framePos), frame);
    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(textPos), frame);
}

// frame display=normal, text display=none
TEST_F(HoverTest, DisplayTextNone) {
    std::string frameProperties = "";
    std::string textProperties = ",\"display\": \"none\"" + ON_CURSOR;
    init(frameProperties.c_str(), textProperties.c_str());

    // frame display=normal, text display=none
    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(framePos), frame);
    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(Point(3,3)), frame);
}

//
TEST_F(HoverTest, Opacity) {
    std::string frameProperties = "";
    std::string textProperties = ON_CURSOR;
    init(frameProperties.c_str(), textProperties.c_str());

    // frame opacity=0.0, text=opacity=1.0
    frame->setProperty(kPropertyOpacity, 0.0);
    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(framePos), top);
    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(textPos), top);
    frame->setProperty(kPropertyOpacity, 1.0);

    // frame opacity=1.0, text=opacity=0.0
    text->setProperty(kPropertyOpacity, 0.0);
    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(framePos), frame);
    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(textPos), frame);

    // frame opacity=1.0, text=opacity=0.5
    text->setProperty(kPropertyOpacity, 0.5);
    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(framePos), frame);
    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(textPos), text);
    text->setProperty(kPropertyOpacity, 1.0);

    // frame opacity=0.5, text=opacity=0.5
    frame->setProperty(kPropertyOpacity, 0.5);
    text->setProperty(kPropertyOpacity, 0.5);
    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(framePos), frame);
    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(textPos), text);
    frame->setProperty(kPropertyOpacity, 1.0);
    text->setProperty(kPropertyOpacity, 1.0);
}

// Test hover state
TEST_F(HoverTest, Basic)
{
    std::string frameProperties = "";
    std::string textProperties = ON_CURSOR;
    init(frameProperties.c_str(), textProperties.c_str());

    // Simulate cursor entering in the frame
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, framePos));
    root->clearPending();

    // validate hover states
    ASSERT_FALSE(CheckState(top, kStateHover));
    ASSERT_TRUE(CheckState(frame, kStateHover));
    ASSERT_FALSE(CheckState(text, kStateHover));
    // validate frame changes
    ASSERT_TRUE(CheckDirty(frame, kPropertyBorderColor, kPropertyVisualHash));
    ASSERT_TRUE(IsEqual(Color(session, FRAME_BORDERCOLOR_HOVER), frame->getCalculated(kPropertyBorderColor)));
    // validate text string
    ASSERT_FALSE(CheckDirty(text, kPropertyText));
    ASSERT_TRUE(IsEqual(TEXT_TEXT, text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual(Color(session, TEXT_COLOR), text->getCalculated(kPropertyColor)));
    // Only the frame was dirty
    ASSERT_TRUE(CheckDirty(root, frame));

    // Simulate cursor entering in the text
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, textPos));
    root->clearPending();

    // validate hover states
    ASSERT_FALSE(CheckState(top, kStateHover));
    ASSERT_FALSE(CheckState(frame, kStateHover));
    ASSERT_TRUE(CheckState(text, kStateHover));
    // validate frame changes
    ASSERT_TRUE(CheckDirty(frame, kPropertyBorderColor, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(IsEqual(Color(session, FRAME_BORDERCOLOR), frame->getCalculated(kPropertyBorderColor)));
    // validate text string
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyColorNonKaraoke,
        kPropertyInnerBounds, kPropertyBounds, kPropertyVisualHash));
    ASSERT_TRUE(IsEqual(ON_CURSOR_ENTER_TEXT, text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual(Color(session, TEXT_COLOR_HOVER), text->getCalculated(kPropertyColor)));
    // Frame and text were dirty
    ASSERT_TRUE(CheckDirty(root, text, frame));

    // Simulate cursor exiting in the text
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, framePos));
    root->clearPending();

    // validate hover states
    ASSERT_FALSE(CheckState(top, kStateHover));
    ASSERT_TRUE(CheckState(frame, kStateHover));
    ASSERT_FALSE(CheckState(text, kStateHover));
    // validate frame changes
    // validate frame changes
    ASSERT_TRUE(CheckDirty(frame, kPropertyBorderColor, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(IsEqual(Color(session, FRAME_BORDERCOLOR_HOVER), frame->getCalculated(kPropertyBorderColor)));
    // validate text string
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyColorNonKaraoke,
        kPropertyInnerBounds, kPropertyBounds, kPropertyVisualHash));
    ASSERT_TRUE(IsEqual(ON_CURSOR_EXIT_TEXT, text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual(Color(session, TEXT_COLOR), text->getCalculated(kPropertyColor)));
    // Frame and text were dirty
    ASSERT_TRUE(CheckDirty(root, frame, text));

    // Reset text string
    resetTextString();

    // Simulate cursor exiting all components
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, invalidPos));
    root->clearPending();

    // validate hover states
    ASSERT_FALSE(CheckState(top, kStateHover));
    ASSERT_FALSE(CheckState(frame, kStateHover));
    ASSERT_FALSE(CheckState(text, kStateHover));
    // validate frame changes
    ASSERT_TRUE(CheckDirty(frame, kPropertyBorderColor, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(IsEqual(Color(session, FRAME_BORDERCOLOR), frame->getCalculated(kPropertyBorderColor)));
    // validate text string
    ASSERT_TRUE(CheckDirty(text, kPropertyBounds, kPropertyInnerBounds, kPropertyVisualHash));
    ASSERT_TRUE(IsEqual(TEXT_TEXT, text->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual(Color(session, TEXT_COLOR), text->getCalculated(kPropertyColor)));
    // The frame and the text were dirty
    ASSERT_TRUE(CheckDirty(root, frame, text));
}

// Test hover state with frame inherits parent state
TEST_F(HoverTest, FrameInherit)
{
    std::string frameProperties = ",\"inheritParentState\": \"true\"";
    std::string textProperties = ON_CURSOR;
    init(frameProperties.c_str(), textProperties.c_str());

    // Simulate cursor entering in the touch wrapper
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, framePos));
    root->clearPending();

    validateHoverStates(true, true, false);
    validateFrame();
    validateTextString();
    ASSERT_TRUE(CheckDirty(frame, kPropertyBorderColor, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, frame));

    // Simulate cursor entering in the text
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, textPos));
    root->clearPending();

    validateHoverStates(false, false, true);
    validateFrame();
    validateText();
    validateTextString(ON_CURSOR_ENTER_TEXT);
    ASSERT_TRUE(CheckDirty(frame, kPropertyBorderColor, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyBounds, kPropertyColorKaraokeTarget, kPropertyColorNonKaraoke,
        kPropertyInnerBounds, kPropertyColor, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, frame, text));

    // Simulate cursor exiting in the text
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, framePos));
    root->clearPending();

    validateHoverStates(true, true, false);
    validateFrame();
    validateText();
    validateTextString(ON_CURSOR_EXIT_TEXT);
    ASSERT_TRUE(CheckDirty(frame, kPropertyBorderColor, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyColor, kPropertyColorNonKaraoke, kPropertyColorKaraokeTarget,
        kPropertyInnerBounds, kPropertyBounds, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, frame, text));

    // Reset text
    resetTextString();

    // Simulate cursor exiting in the touch wrapper
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, invalidPos));
    root->clearPending();

    validateHoverStates(false, false, false);
    validateFrame();
    validateTextString();
    ASSERT_TRUE(CheckDirty(frame, kPropertyBorderColor, kPropertyNotifyChildrenChanged, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(text, kPropertyBounds, kPropertyInnerBounds, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, frame, text));
}


// Test hover state with frame disabled
TEST_F(HoverTest, FrameDisabled)
{
    std::string frameProperties = "";
    std::string textProperties = ON_CURSOR;
    init(frameProperties.c_str(), textProperties.c_str());

    // Simulate cursor entering in the touch wrapper
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, framePos));
    root->clearPending();

    ASSERT_TRUE(CheckState(top));
    ASSERT_TRUE(CheckState(frame, kStateHover));
    ASSERT_TRUE(CheckState(text));
    validateTextString();
    ASSERT_TRUE(CheckDirty(frame, kPropertyBorderColor, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, frame));

    // Disable the touch wrapper
    frame->setProperty(kPropertyDisabled, true);
    root->clearPending();

    ASSERT_TRUE(CheckState(top));
    ASSERT_TRUE(CheckState(frame, kStateDisabled));
    ASSERT_TRUE(CheckState(text));
    validateFrame();
    validateFrameDisabledState(true);
    validateTextString();
    ASSERT_TRUE(CheckDirty(frame, kPropertyBorderColor, kPropertyDisabled, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, frame));

    // Simulate cursor entering in the text
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, textPos));
    root->clearPending();
    ASSERT_TRUE(CheckState(top));
    ASSERT_TRUE(CheckState(frame, kStateDisabled));
    ASSERT_TRUE(CheckState(text, kStateHover));
    validateText();
    validateTextString(ON_CURSOR_ENTER_TEXT);
    ASSERT_TRUE(CheckDirty(text, kPropertyText, kPropertyBounds, kPropertyInnerBounds, kPropertyColorKaraokeTarget,
        kPropertyColorNonKaraoke, kPropertyColor, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, frame, text));

    // Reset text
    resetTextString();
    // Enable the touch wrapper
    frame->setProperty(kPropertyDisabled, false);
    root->clearPending();

    validateHoverStates(false, false, true);
    validateFrameDisabledState(false);
    validateTextString();
    ASSERT_TRUE(CheckDirty(frame, kPropertyDisabled, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(text, kPropertyBounds, kPropertyInnerBounds, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, frame, text));
}

// Test hover state with frame disabled and text inherits parent state
TEST_F(HoverTest, FrameDisabledTextInherit)
{
    std::string frameProperties = "";
    std::string textProperties = ",\"inheritParentState\": \"true\"" + ON_CURSOR;
    init(frameProperties.c_str(), textProperties.c_str());

    // Simulate cursor entering in the touch wrapper
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, framePos));
    // validate hover states
    ASSERT_FALSE(CheckState(top, kStateHover));
    ASSERT_TRUE(CheckState(frame, kStateHover));
    ASSERT_TRUE(CheckState(text, kStateHover));
    // validate text string
    validateTextString();
    root->clearDirty();

    // Disable the frame
    frame->setProperty(kPropertyDisabled, true);
    auto dirty = root->getDirty();
    ASSERT_EQ(2, dirty.size());
    ASSERT_EQ(1, dirty.count(frame));
    ASSERT_EQ(1, dirty.count(text));
    // validate hover states
    ASSERT_FALSE(CheckState(top, kStateHover));
    ASSERT_FALSE(CheckState(frame, kStateHover));
    ASSERT_FALSE(CheckState(text, kStateHover));
    // validate frame changes
    validateFrame();
    validateFrameDisabledState(true);
    // validate text changes
    validateText();
    validateTextString();
    root->clearDirty();

    // enable the frame
    frame->setProperty(kPropertyDisabled, false);
    dirty = root->getDirty();
    ASSERT_EQ(2, dirty.size());
    ASSERT_EQ(1, dirty.count(frame));
    ASSERT_EQ(1, dirty.count(text));
    // validate hover states
    ASSERT_FALSE(CheckState(top, kStateHover));
    ASSERT_TRUE(CheckState(frame, kStateHover));
    ASSERT_TRUE(CheckState(text, kStateHover));
    // validate frame changes
    validateFrame();
    validateFrameDisabledState(false);
    // validate text changes
    validateText();
    validateTextDisabledState(false);
    validateTextString();
    root->clearDirty();
}

static const char *SCROLL_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"id\": \"myContainer\","
    "      \"paddingTop\": 75,"
    "      \"paddingBottom\": 75,"
    "      \"width\": 200,"
    "      \"height\": 300,"
    "      \"items\": ["
    "        {"
    "          \"type\": \"ScrollView\","
    "          \"id\": \"myScrollView\","
    "          \"paddingTop\": 50,"
    "          \"paddingBottom\": 50,"
    "          \"width\": \"200\","
    "          \"height\": \"200\","
    "          \"items\": {"
    "            \"type\": \"Frame\","
    "            \"id\": \"myFrame\","
    "            \"paddingTop\": 25,"
    "            \"paddingBottom\": 25,"
    "            \"width\": 200,"
    "            \"height\": 1000"
    "          }"
    "        },"
    "        {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"myTouch\","
    "          \"onPress\": {"
    "            \"type\": \"Scroll\","
    "            \"componentId\": \"myScrollView\","
    "            \"distance\": 0.5"
    "          }"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(HoverTest, ScrollView) {
    loadDocument(SCROLL_TEST);

    auto top = root->topComponent();
    auto container = context->findComponentById("myContainer");
    auto scroll = context->findComponentById("myScrollView");
    auto frame = context->findComponentById("myFrame");

    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(Point(1,1)), container);
    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(Point(1,76)), scroll);
    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(Point(1,126)), frame);

    scroll->update(kUpdateScrollPosition, 200);
    ASSERT_EQ(scroll->scrollPosition(), Point(0, 200));

    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(Point(1,1)), container);
    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(Point(1,76)), frame);
    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(Point(1,156)), frame);
}

static const char *SCROLL_CONTAINER_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"id\": \"myContainer\","
    "      \"width\": 200,"
    "      \"height\": 300,"
    "      \"item\": ["
    "        {"
    "          \"type\": \"ScrollView\","
    "          \"id\": \"myScrollView\","
    "          \"width\": \"200\","
    "          \"height\": \"200\","
    "          \"item\": {"
    "            \"type\": \"Container\","
    "            \"direction\": \"column\","
    "            \"id\": \"myScrollViewContainer\","
    "            \"data\": ["
    "              1,"
    "              2,"
    "              3,"
    "              4,"
    "              5"
    "            ],"
    "            \"item\": {"
    "              \"type\": \"Frame\","
    "              \"id\": \"id${data}\","
    "              \"width\": 100,"
    "              \"height\": 100"
    "            }"
    "          }"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(HoverTest, ScrollViewContainer) {
    loadDocument(SCROLL_CONTAINER_TEST);
    auto top = root->topComponent();
    auto scroll = context->findComponentById("myScrollView");
    auto container = context->findComponentById("myScrollViewContainer");

    std::vector<ComponentPtr> frames;
    for (int i = 1; i <= 5; i++) {
        std::string id = "id" + std::to_string(i);
        auto frame = context->findComponentById(id);
        frames.push_back(frame);
    }

    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(Point(1,1)), frames[0]);
    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(Point(1,101)), frames[1]);

    scroll->update(kUpdateScrollPosition, 200);
    ASSERT_EQ(scroll->scrollPosition(), Point(0, 200));

    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(Point(1,1)), frames[2]);
    ASSERT_EQ(top->getContext()->hoverManager().findHoverByPosition(Point(1,101)), frames[3]);
}

static const char *PAGER_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Pager\","
    "      \"id\": \"myPager\","
    "      \"width\": 100,"
    "      \"height\": 100,"
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"id\": \"id${data}\","
    "        \"text\": \"TEXT${data}\","
    "        \"speech\": \"URL${data}\""
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4,"
    "        5"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(HoverTest, Pager) {
    loadDocument(PAGER_TEST);

    auto pager = context->findComponentById("myPager");

    std::vector<ComponentPtr> frames;
    for (int i = 1; i <= 5; i++) {
        std::string id = std::string("id") + std::to_string(i);
        frames.push_back(context->findComponentById(id));
    }
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,1)), frames[0]);
    executeScrollToComponent("id2", kCommandScrollAlignFirst);
    advanceTime(600);
    ASSERT_EQ(1, component->pagePosition());

    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,1)), frames[1]);
}

static const char *PAGER_TEST_FRAME =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Pager\","
    "      \"id\": \"myPager\","
    "      \"width\": 300,"
    "      \"height\": 100,"
    "      \"item\": {"
    "        \"type\": \"Frame\","
    "        \"id\": \"frame${data}\","
    "        \"width\": 100,"
    "        \"height\": 100,"
    "        \"items\": {"
    "          \"type\": \"Text\","
    "          \"id\": \"text${data}\","
    "          \"text\": \"TEXT${data}\","
    "          \"speech\": \"URL${data}\""
    "        }"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4,"
    "        5"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(HoverTest, PagerFrame) {
    loadDocument(PAGER_TEST_FRAME);
    advanceTime(10);
    root->clearDirty();

    auto pager = context->findComponentById("myPager");

    std::vector<ComponentPtr> frames;
    std::vector<ComponentPtr> texts;
    for (int i = 1; i <= 5; i++) {
        std::string frameId = std::string("frame") + std::to_string(i);
        std::string textId = std::string("text") + std::to_string(i);
        frames.push_back(context->findComponentById(frameId));
        texts.push_back(context->findComponentById(textId));
    }
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,1)), texts[0]);
    executeScrollToComponent("frame2", kCommandScrollAlignFirst);
    advanceTime(600);
    ASSERT_EQ(1, component->pagePosition());

    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,1)), texts[1]);
}

static const char *SEQUENCE_HORIZONTAL =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Sequence\","
    "      \"scrollDirection\": \"horizontal\","
    "      \"id\": \"mySequence\","
    "      \"width\": 200,"
    "      \"height\": 300,"
    "      \"paddingLeft\": 50,"
    "      \"paddingRight\": 50,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"id\": \"id${data}\","
    "        \"width\": 100,"
    "        \"height\": 100"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4,"
    "        5,"
    "        6,"
    "        7,"
    "        8,"
    "        9,"
    "        10"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(HoverTest, SequenceHorizontal) {
    loadDocument(SEQUENCE_HORIZONTAL);

    auto sequence = std::dynamic_pointer_cast<SequenceComponent>(context->findComponentById("mySequence"));

    completeScroll(component, 1);
    ASSERT_EQ(sequence->scrollPosition(), Point(100.0, 0.0));

    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,1)), context->findComponentById("id1"));
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(51,1)), context->findComponentById("id2"));

    completeScroll(component, 4);
    ASSERT_EQ(sequence->scrollPosition(), Point(500.0, 0.0));

    std::vector<ComponentPtr> frames;
    for (int i = 1; i <= 10; i++) {
        std::string id = std::string("id") + std::to_string(i);
        frames.push_back(context->findComponentById(id));
    }

    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,1)), frames[4]);
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(51,1)), frames[5]);
}

static const char *SEQUENCE_VERTICAL_PADDING =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Sequence\","
    "      \"scrollDirection\": \"vertical\","
    "      \"id\": \"mySequence\","
    "      \"width\": 200,"
    "      \"height\": 300,"
    "      \"paddingTop\": 50,"
    "      \"paddingBottom\": 50,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"id\": \"id${data}\","
    "        \"spacing\": 10,"
    "        \"width\": 100,"
    "        \"height\": 100"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4,"
    "        5,"
    "        6,"
    "        7,"
    "        8,"
    "        9,"
    "        10"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(HoverTest, SequenceVerticalPadding) {
    loadDocument(SEQUENCE_VERTICAL_PADDING);

    auto sequence = std::dynamic_pointer_cast<SequenceComponent>(context->findComponentById("mySequence"));

    completeScroll(component, 1);
    ASSERT_EQ(sequence->scrollPosition(), Point(0.0, 200.0));

    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,1)), context->findComponentById("id2"));
    // id2 110-210, space 210-220, id3 220-320 -- paddingTop +50
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,51)), context->findComponentById("id2"));
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,61)), sequence);
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,71)), context->findComponentById("id3"));
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,251)), context->findComponentById("id4"));

    completeScroll(component, 3);
    ASSERT_EQ(sequence->scrollPosition(), Point(0.0, 800.0));

    std::vector<ComponentPtr> frames;
    for (int i = 1; i <= 10; i++) {
        std::string id = std::string("id") + std::to_string(i);
        frames.push_back(context->findComponentById(id));
    }

    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,1)), frames[6]);
    // id8 770-870, space 870-880, id9 880-980 -- paddingTop +50
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,51)), frames[7]);
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,121)), sequence);
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,131)), frames[8]);
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,251)), frames[9]);
}

static const char *SEQUENCE_VERTICAL =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Sequence\","
    "      \"scrollDirection\": \"vertical\","
    "      \"id\": \"mySequence\","
    "      \"width\": 200,"
    "      \"height\": 300,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"id\": \"id${data}\","
    "        \"width\": 100,"
    "        \"height\": 100,"
    "        \"item\": {"
    "          \"type\": \"Text\","
    "          \"id\": \"text${data}\","
    "          \"text\": \"Number ${data}\""
    "        }"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4,"
    "        5,"
    "        6,"
    "        7,"
    "        8,"
    "        9,"
    "        10"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(HoverTest, SequenceVertical) {
    loadDocument(SEQUENCE_VERTICAL);

    auto sequence = std::dynamic_pointer_cast<SequenceComponent>(context->findComponentById("mySequence"));

    completeScroll(component, 1);
    ASSERT_EQ(sequence->scrollPosition(), Point(0.0, 300.0));

    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,1)), context->findComponentById("text4"));
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,11)), context->findComponentById("id4"));
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,101)), context->findComponentById("text5"));
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,111)), context->findComponentById("id5"));

    completeScroll(component, 1);
    ASSERT_EQ(sequence->scrollPosition(), Point(0.0, 600.0));

    std::vector<ComponentPtr> frames;
    std::vector<ComponentPtr> texts;
    for (int i = 1; i <= 10; i++) {
        std::string frameId = std::string("id") + std::to_string(i);
        std::string textId = std::string("text") + std::to_string(i);
        frames.push_back(context->findComponentById(frameId));
        texts.push_back(context->findComponentById(textId));
    }

    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,1)), texts[6]);
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,11)), frames[6]);
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,101)), texts[7]);
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,111)), frames[7]);
}

static const char *SEQUENCE_VERTICAL_PADDING_TEXT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Sequence\","
    "      \"scrollDirection\": \"vertical\","
    "      \"id\": \"mySequence\","
    "      \"width\": 200,"
    "      \"height\": 300,"
    "      \"paddingTop\": 50,"
    "      \"paddingBottom\": 50,"
    "      \"items\": {"
    "        \"type\": \"Frame\","
    "        \"id\": \"id${data}\","
    "        \"spacing\": 10,"
    "        \"width\": 100,"
    "        \"height\": 100,"
    "        \"item\": {"
    "          \"type\": \"Text\","
    "          \"id\": \"text${data}\","
    "          \"text\": \"Number ${data}\""
    "        }"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2,"
    "        3,"
    "        4,"
    "        5,"
    "        6,"
    "        7,"
    "        8,"
    "        9,"
    "        10"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(HoverTest, SequenceVerticalPaddingText) {
    loadDocument(SEQUENCE_VERTICAL_PADDING_TEXT);

    auto sequence = std::dynamic_pointer_cast<SequenceComponent>(context->findComponentById("mySequence"));

    completeScroll(component, 1);
    ASSERT_EQ(sequence->scrollPosition(), Point(0.0, 200.0));

    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,1)), context->findComponentById("id2"));
    // id2 110-210, space 210-220, id3 220-320 -- paddingTop +50
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,51)), context->findComponentById("id2")); // y=201
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,61)), sequence); // y=211
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,71)), context->findComponentById("text3")); // y=221
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,81)), context->findComponentById("id3")); // y=231
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,251)), context->findComponentById("id4")); // y=401

    completeScroll(component, 3);
    ASSERT_EQ(sequence->scrollPosition(), Point(0.0, 800.0));

    std::vector<ComponentPtr> frames;
    std::vector<ComponentPtr> texts;
    for (int i = 1; i <= 10; i++) {
        std::string frameId = std::string("id") + std::to_string(i);
        std::string textId = std::string("text") + std::to_string(i);
        frames.push_back(context->findComponentById(frameId));
        texts.push_back(context->findComponentById(textId));
    }

    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,1)), context->findComponentById("id7"));
    // id8 770-870, space 870-880, id9 880-980 -- paddingTop +50
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,51)), frames[7]); // y=801
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,121)), sequence); // y=871
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,131)), texts[8]); // y=881
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,141)), frames[8]); // y=891
    ASSERT_EQ(context->hoverManager().findHoverByPosition(Point(1,251)), context->findComponentById("id10")); // y=1001
}

static const char *LOCAL_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Container\","
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"text\": \"Text ${data}\","
    "        \"color\": \"red\","
    "        \"width\": 100,"
    "        \"height\": 100,"
    "        \"onCursorEnter\": ["
    "          {"
    "            \"type\": \"SetValue\","
    "            \"property\": \"color\","
    "            \"value\": \"blue\""
    "          },"
    "          {"
    "            \"type\": \"SetValue\","
    "            \"property\": \"text\","
    "            \"value\": \"Blue Text ${data}\""
    "          }"
    "        ],"
    "        \"onCursorExit\": ["
    "          {"
    "            \"type\": \"SetValue\","
    "            \"property\": \"color\","
    "            \"value\": \"green\""
    "          },"
    "          {"
    "            \"type\": \"SetValue\","
    "            \"property\": \"text\","
    "            \"value\": \"Green Text ${data}\""
    "          }"
    "        ]"
    "      },"
    "      \"data\": ["
    "        1,"
    "        2"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(HoverTest, LOCAL_TEST)
{
    loadDocument(LOCAL_TEST);
    ASSERT_TRUE(component);
    ASSERT_EQ(2, component->getChildCount());

    auto text1 = component->getChildAt(0);
    auto text2 = component->getChildAt(1);

    ASSERT_TRUE(IsEqual(Color(Color::RED), text1->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual(Color(Color::RED), text2->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual("Text 1", text1->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual("Text 2", text2->getCalculated(kPropertyText).asString()));

    // Hover over the first component
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, {50,50}));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(text1, kPropertyText, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyColorNonKaraoke,
                           kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, text1));

    ASSERT_TRUE(IsEqual(Color(Color::BLUE), text1->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual("Blue Text 1", text1->getCalculated(kPropertyText).asString()));

    // Hover over the second component
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, {50,150}));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(text1, kPropertyText, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyColorNonKaraoke,
                           kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(text2, kPropertyText, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyColorNonKaraoke,
                           kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, text1, text2));

    ASSERT_TRUE(IsEqual(Color(Color::GREEN), text1->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual("Green Text 1", text1->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), text2->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual("Blue Text 2", text2->getCalculated(kPropertyText).asString()));

    // Clear away from all components
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, {300, 300}));
    root->clearPending();

    ASSERT_TRUE(CheckDirty(text2, kPropertyText, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyColorNonKaraoke,
                           kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, text2));

    ASSERT_TRUE(IsEqual(Color(Color::GREEN), text2->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual("Green Text 2", text2->getCalculated(kPropertyText).asString()));
}


/*
 * Verify OnCursor handlers are executed when the disabled state of the hover component changes.
 * Disable hover component => OnCursorExit
 * Enable hover component => OnCursorEnter
 */
TEST_F(HoverTest, OnCursor_DisableState_Change)
{
    loadDocument(LOCAL_TEST);
    ASSERT_TRUE(component);
    ASSERT_EQ(2, component->getChildCount());

    auto text1 = std::static_pointer_cast<CoreComponent>(component->getChildAt(0));
    auto& fm = root->context().hoverManager();

    // Hover over the component
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, {50,50}));
    root->clearPending();

    // verify state when hover = true
    ASSERT_EQ(fm.getHover(), text1);
    ASSERT_TRUE(CheckState(text1, kStateHover));
    ASSERT_TRUE(CheckDirty(text1, kPropertyText, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyColorNonKaraoke,
                           kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, text1));
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), text1->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual("Blue Text 1", text1->getCalculated(kPropertyText).asString()));

    // disable the component
    text1->setState(kStateDisabled, true);
    root->clearPending();

    // verify onCursorExit handler was executed
    ASSERT_EQ(fm.getHover(), text1);
    ASSERT_TRUE(CheckState(text1, kStateDisabled));
    ASSERT_TRUE(CheckDirty(text1, kPropertyText, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyColorNonKaraoke,
                           kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, text1));
    ASSERT_TRUE(IsEqual(Color(Color::GREEN), text1->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual("Green Text 1", text1->getCalculated(kPropertyText).asString()));

    // enable the component
    text1->setState(kStateDisabled, false);
    root->clearPending();

    // verify onCursorEnter handler was executed
    ASSERT_EQ(fm.getHover(), text1);
    ASSERT_TRUE(CheckState(text1, kStateHover));
    ASSERT_TRUE(CheckDirty(text1, kPropertyText, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyColorNonKaraoke,
                           kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, text1));
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), text1->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual("Blue Text 1", text1->getCalculated(kPropertyText).asString()));

}


/*
 * Verify cursor movement in and out of a disabled component
 * Disable hover component => OnCursorExit
 * Enable hover component => OnCursorEnter
 */
TEST_F(HoverTest, CursorMove_DisabledComponent)
{
    loadDocument(LOCAL_TEST);
    ASSERT_TRUE(component);
    ASSERT_EQ(2, component->getChildCount());

    auto text1 = std::static_pointer_cast<CoreComponent>(component->getChildAt(0));
    auto& fm = root->context().hoverManager();

    // disable the component
    text1->setState(kStateDisabled, true);
    root->clearPending();

    // Hover over the component
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, {50,50}));
    root->clearPending();

    // verify state matches the initial state and no changes due to hover
    ASSERT_EQ(fm.getHover(), text1);
    ASSERT_TRUE(CheckState(text1, kStateDisabled));
    ASSERT_FALSE(CheckDirty(root, text1));
    ASSERT_TRUE(IsEqual(Color(Color::RED), text1->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual("Text 1", text1->getCalculated(kPropertyText).asString()));

    // Hover outside the component
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, {50,150}));
    root->clearPending();

    // verify state matches the initial state and no changes due to hover
    ASSERT_NE(fm.getHover(), text1);
    ASSERT_TRUE(CheckState(text1, kStateDisabled));
    ASSERT_FALSE(CheckDirty(root, text1));
    ASSERT_TRUE(IsEqual(Color(Color::RED), text1->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual("Text 1", text1->getCalculated(kPropertyText).asString()));

}



static const char *STYLE_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.2\","
    "  \"styles\": {"
    "    \"frameStyle\": {"
    "      \"values\": ["
    "        {"
    "          \"backgroundColor\": \"blue\""
    "        },"
    "        {"
    "          \"when\": \"${state.hover}\","
    "          \"backgroundColor\": \"red\""
    "        }"
    "      ]"
    "    },"
    "    \"textStyle\": {"
    "      \"values\": ["
    "        {"
    "          \"color\": \"white\""
    "        },"
    "        {"
    "          \"when\": \"${state.hover}\","
    "          \"color\": \"black\""
    "        }"
    "      ]"
    ""
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Frame\","
    "      \"id\": \"testFrame\","
    "      \"style\": \"frameStyle\","
    "      \"paddingTop\": 50,"
    "      \"paddingLeft\": 50,"
    "      \"width\": 100,"
    "      \"height\": 100,"
    "      \"item\": {"
    "        \"type\": \"Text\","
    "        \"id\": \"textComp\","
    "        \"text\": \"Text\","
    "        \"style\": \"textStyle\","
    "        \"inheritParentState\": \"true\""
    "      }"
    "    }"
    "  }"
    "}";

/*
 * Test style changes based on inherited state.  Verify unnecessary changes don't happen.
 * Situation: child text inherits state from parent frame, styles change properties based on hover state:
 * - move cursor to parent => parent and child in hover state, properties are dirty
 * - move cursor to child => parent and child in hover state, no dirty properties
 * - move cursor out => parent and child not in hover state, properties are dirty
 * - move cursor to child -> parent and child in hover state, properties are dirty
 * - move cursor to parent -> parent and child in hover state, no dirty properties
 */
TEST_F(HoverTest, StyleUpdates_InheritedState)
{
    loadDocument(STYLE_TEST);
    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getChildCount());
    auto& fm = root->context().hoverManager();

    auto text1 = component->getChildAt(0);
    ASSERT_TRUE(text1);

    // validate initial state
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), component->getCalculated(kPropertyBackgroundColor)));
    ASSERT_TRUE(IsEqual(Color(Color::WHITE), text1->getCalculated(kPropertyColor)));

    // Hover over the parent frame
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, {20,20}));
    root->clearPending();

    // verify the frame and child(inheritParentState=true) are dirty, and show hover state
    ASSERT_EQ(component, fm.getHover());
    ASSERT_TRUE(CheckDirty(root, component, text1));
    ASSERT_TRUE(IsEqual(Color(Color::RED), component->getCalculated(kPropertyBackgroundColor)));
    ASSERT_TRUE(IsEqual(Color(Color::BLACK), text1->getCalculated(kPropertyColor)));

    // Hover over the child text
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, {50,50}));
    root->clearPending();

    // verify the frame and child(inheritParentState=true) show hover state, but are NOT dirty
    ASSERT_EQ(text1, fm.getHover());
    ASSERT_FALSE(CheckDirty(root, component, text1));
    ASSERT_TRUE(IsEqual(Color(Color::RED), component->getCalculated(kPropertyBackgroundColor)));
    ASSERT_TRUE(IsEqual(Color(Color::BLACK), text1->getCalculated(kPropertyColor)));

    // exit all components
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, {500,500}));
    root->clearPending();

    // verify the frame and child(inheritParentState=true) are dirty, and no longer show hover state
    ASSERT_EQ(nullptr, fm.getHover());
    ASSERT_TRUE(CheckDirty(root, component, text1));
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), component->getCalculated(kPropertyBackgroundColor)));
    ASSERT_TRUE(IsEqual(Color(Color::WHITE), text1->getCalculated(kPropertyColor)));

    // Hover over the child text
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, {50,50}));
    root->clearPending();

    // verify the frame and child(inheritParentState=true) are dirty, and show hover state
    ASSERT_EQ(text1, fm.getHover());
    ASSERT_TRUE(CheckDirty(root, component, text1));
    ASSERT_TRUE(IsEqual(Color(Color::RED), component->getCalculated(kPropertyBackgroundColor)));
    ASSERT_TRUE(IsEqual(Color(Color::BLACK), text1->getCalculated(kPropertyColor)));

    // Hover over the parent frame
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, {20,20}));
    root->clearPending();

    // verify the frame and child(inheritParentState=true) show hover state, but are NOT dirty
    ASSERT_EQ(component, fm.getHover());
    ASSERT_FALSE(CheckDirty(root, component, text1));
    ASSERT_TRUE(IsEqual(Color(Color::RED), component->getCalculated(kPropertyBackgroundColor)));
    ASSERT_TRUE(IsEqual(Color(Color::BLACK), text1->getCalculated(kPropertyColor)));

}

TEST_F(HoverTest, PointerCancelWithNoActivePointer)
{
    loadDocument(LOCAL_TEST);
    ASSERT_TRUE(component);

    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove,   {1000,216}));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove,   {1030,190}));
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerCancel, {1030,190}));
}

