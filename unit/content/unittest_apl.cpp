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

#include "apl/apl.h"

using namespace apl;

static const char *MAIN =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"import\": ["
    "    {"
    "      \"name\": \"basic\","
    "      \"version\": \"1.2\""
    "    }"
    "  ],"
    "  \"mainTemplate\": {"
    "    \"parameters\": ["
    "      \"payload\""
    "    ],"
    "    \"item\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"onPress\": ["
    "        {"
    "          \"type\": \"SendEvent\","
    "          \"arguments\": \"test\""
    "        }"
    "      ],"
    "      \"item\": {"
    "        \"type\": \"Frame\","
    "        \"inheritParentState\": true,"
    "        \"style\": \"frameStyle\","
    "        \"item\": {"
    "          \"type\": \"Text\","
    "          \"inheritParentState\": true,"
    "          \"text\": \"${payload}\","
    "          \"style\": \"textStyle\""
    "        }"
    "      }"
    "    }"
    "  }"
    "}";

static const char *BASIC =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"resources\": ["
    "    {"
    "      \"colors\": {"
    "        \"myRed\": \"rgb(255, 16, 32)\""
    "      }"
    "    }"
    "  ],"
    "  \"styles\": {"
    "    \"frameStyle\": {"
    "      \"values\": ["
    "        {"
    "          \"borderWidth\": 2,"
    "          \"borderColor\": \"transparent\""
    "        },"
    "        {"
    "          \"when\": \"${state.pressed}\","
    "          \"borderColor\": \"green\""
    "        }"
    "      ]"
    "    },"
    "    \"textStyle\": {"
    "      \"values\": ["
    "        {"
    "          \"color\": \"@myRed\""
    "        },"
    "        {"
    "          \"when\": \"${state.pressed}\","
    "          \"color\": \"blue\""
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

class MyTextMeasure : public TextMeasurement {
public:
    LayoutSize measure(Component *component, float width, MeasureMode widthMode, float height,
                   MeasureMode heightMode) override {
        return LayoutSize({ 120.0, 60.0 });
    }

    float baseline(Component *component, float width, float height) override {
        return 0;
    }
};

// Test that the apl.h file has all the necessary includes
TEST(APLTest, Basic)
{
    // Load the main document
    auto content = Content::create(MAIN, makeDefaultSession());
    ASSERT_TRUE(content);

    // The document has one import it is waiting for
    ASSERT_TRUE(content->isWaiting());
    auto requested = content->getRequestedPackages();
    ASSERT_EQ(1, requested.size());
    auto request = *requested.begin();
    ASSERT_STREQ("basic", request.reference().name().c_str());
    ASSERT_STREQ("1.2", request.reference().version().c_str());
    content->addPackage(request, BASIC);
    ASSERT_FALSE(content->isWaiting());

    // All imports have loaded.  The document has a parameter that needs assignment.
    ASSERT_FALSE(content->isReady());
    ASSERT_EQ(1, content->getParameterCount());
    content->addData(content->getParameterAt(0), "\"Your text inserted here\"");
    ASSERT_TRUE(content->isReady());

    // Inflate the document
    auto metrics = Metrics().size(800,800).dpi(320);
    auto measure = std::make_shared<MyTextMeasure>();
    RootConfig rootConfig = RootConfig().measure(measure);
    auto root = RootContext::create( metrics, content, rootConfig );
    ASSERT_TRUE(root);

    // Check the layout
    auto top = root->topComponent();  // The touchwrapper
    ASSERT_EQ(Rect(0, 0, 400, 400), top->getCalculated(kPropertyBounds).getRect());
    auto frame = top->getChildAt(0);
    ASSERT_EQ(Object(Color()), frame->getCalculated(kPropertyBorderColor));
    auto text = frame->getChildAt(0);
    ASSERT_EQ(Rect(2, 2, 120, 60), text->getCalculated(kPropertyBounds).getRect());  // Frame has a 2 dp border
    ASSERT_EQ(StyledText::create(root->context(), "Your text inserted here"), text->getCalculated(kPropertyText));
    ASSERT_EQ(Object(Color(root->getSession(), "#ff1020")), text->getCalculated(kPropertyColor));

    // Simulate a user touching on the screen
    root->handlePointerEvent(PointerEvent(kPointerDown, Point(1,1)));
    ASSERT_TRUE(root->isDirty());
    auto dirty = root->getDirty();
    ASSERT_EQ(2, dirty.size());
    ASSERT_EQ(1, dirty.count(frame));
    ASSERT_EQ(1, frame->getDirty().count(kPropertyBorderColor));
    ASSERT_EQ(Object(Color(Color::GREEN)), frame->getCalculated(kPropertyBorderColor));
    ASSERT_EQ(1, dirty.count(text));
    ASSERT_EQ(1, text->getDirty().count(kPropertyColor));
    ASSERT_EQ(Object(Color(root->getSession(), "blue")), text->getCalculated(kPropertyColor));
    root->clearDirty();

    // Simulate releasing in the touchwrapper
    root->handlePointerEvent(PointerEvent(kPointerUp, Point(1,1)));
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_FALSE(root->hasEvent());
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
    auto args = event.getValue(kEventPropertyArguments);
    ASSERT_EQ(1, args.size());
    ASSERT_EQ(Object("test"), args.at(0));
    ASSERT_TRUE(event.getActionRef().isEmpty());
}
