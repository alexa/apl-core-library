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

#include "../testeventloop.h"

#include "apl/engine/event.h"
#include "apl/primitives/object.h"

using namespace apl;

class CommandSetValueTest : public CommandTest {
public:
    ActionPtr executeSetValue(const std::string& component, const std::string& property, Object value) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "SetValue", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("property", rapidjson::Value(property.c_str(), alloc).Move(), alloc);
        cmd.AddMember("value", value.serialize(alloc), alloc);
        doc.SetArray().PushBack(cmd, alloc);
        return root->executeCommands(doc, false);
    }

    ActionPtr executeSetValue(const std::string& component, const std::string& property, rapidjson::Value& value) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = doc.GetAllocator();
        cmd.AddMember("type", "SetValue", alloc);
        cmd.AddMember("componentId", rapidjson::Value(component.c_str(), alloc).Move(), alloc);
        cmd.AddMember("property", rapidjson::Value(property.c_str(), alloc).Move(), alloc);
        cmd.AddMember("value", value.Move(), alloc);
        doc.SetArray().PushBack(cmd, alloc);
        return root->executeCommands(doc, false);
    }

    rapidjson::Document doc;
};


static const char *COMPONENT_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"TouchWrapper\","
    "      \"id\": \"tw\","
    "      \"width\": 100,"
    "      \"height\": 100,"
    "      \"items\": {"
    "        \"type\": \"Text\","
    "        \"id\": \"text\","
    "        \"text\": \"Simple text.\","
    "        \"inheritParentState\": true"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(CommandSetValueTest, Component)
{
    loadDocument(COMPONENT_TEST);

    ASSERT_EQ(kComponentTypeTouchWrapper, component->getType());

    auto text = component->getCoreChildAt(0);
    ASSERT_EQ(kComponentTypeText, text->getType());

    ASSERT_FALSE(ConsoleMessage());  // No console messages so far

    // Accessibility label is dynamic.
    executeSetValue("tw", "accessibilityLabel", "New one");
    ASSERT_TRUE(CheckDirty(component, kPropertyAccessibilityLabel));
    ASSERT_TRUE(CheckDirty(root, component));
    root->clearDirty();
    ASSERT_EQ("New one", component->getCalculated(kPropertyAccessibilityLabel).asString());
    ASSERT_FALSE(ConsoleMessage());

    // Opacity and all further properties in this test can be set
    executeSetValue("tw", "opacity", "0.7");
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    ASSERT_EQ(0.7, component->getCalculated(kPropertyOpacity).asNumber());

    auto& alloc = doc.GetAllocator();
    rapidjson::Value arr(rapidjson::kArrayType);
    rapidjson::Value t2(rapidjson::kObjectType);
    t2.AddMember("translateX", 10, alloc);
    arr.PushBack(t2, alloc);

    executeSetValue("tw", "transform", arr);
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();

    ASSERT_EQ(Object(Transform2D::translateX(10)), component->getCalculated(kPropertyTransform));

    executeSetValue("tw", "display", "none");
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    ASSERT_EQ(kDisplayNone, component->getCalculated(kPropertyDisplay).asInt());

    ASSERT_EQ(false, component->getState().get(kStateChecked));
    executeSetValue("tw", "checked", true);
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    ASSERT_EQ(true, component->getCalculated(kPropertyChecked).asBoolean());
    ASSERT_EQ(true, component->getState().get(kStateChecked));

    ASSERT_EQ(false, component->getState().get(kStateDisabled));
    executeSetValue("tw", "disabled", true);
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    ASSERT_EQ(true, component->getCalculated(kPropertyDisabled).asBoolean());
    ASSERT_EQ(true, component->getState().get(kStateDisabled));

    ASSERT_TRUE(CheckNoActions());
}

static const char *IMAGE_TEST =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"Image\","
        "      \"id\": \"image\","
        "      \"width\": 100,"
        "      \"height\": 100,"
        "      \"source\": \"http://foo.com/bar.png\""
        "    }"
        "  }"
        "}";

TEST_F(CommandSetValueTest, Image)
{
    loadDocument(IMAGE_TEST);

    ASSERT_EQ(kComponentTypeImage, component->getType());

    executeSetValue("image", "overlayColor", "red");
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    ASSERT_EQ(Color(session, "red"), component->getCalculated(kPropertyOverlayColor).getColor());

    ASSERT_EQ("http://foo.com/bar.png", component->getCalculated(kPropertySource).asString());
    executeSetValue("image", "source", "http://bar.com/foo.png");
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    ASSERT_EQ("http://bar.com/foo.png", component->getCalculated(kPropertySource).asString());

    ASSERT_TRUE(CheckNoActions());
}

static const char *FRAME_TEST =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"Text\","
        "      \"id\": \"text\","
        "      \"width\": 100,"
        "      \"height\": 100,"
        "      \"text\": \"Bar\""
        "    }"
        "  }"
        "}";

TEST_F(CommandSetValueTest, Frame)
{
    loadDocument(FRAME_TEST);

    ASSERT_EQ(kComponentTypeText, component->getType());

    executeSetValue("text", "color", "red");
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    ASSERT_EQ(Color(session, "red"), component->getCalculated(kPropertyColor).getColor());

    ASSERT_EQ("Bar", component->getCalculated(kPropertyText).asString());
    executeSetValue("text", "text", "Foo");
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    ASSERT_EQ("Foo", component->getCalculated(kPropertyText).asString());

    ASSERT_TRUE(CheckNoActions());
}

static const char *FORM_TEST =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"Frame\","
        "      \"id\": \"frame\","
        "      \"width\": 100,"
        "      \"height\": 100,"
        "      \"item\": {"
        "        \"type\": \"Text\","
        "        \"id\": \"text\","
        "        \"text\": \"Simple text.\","
        "        \"inheritParentState\": true"
        "      }"
        "    }"
        "  }"
        "}";

TEST_F(CommandSetValueTest, Text)
{
    loadDocument(FORM_TEST);

    ASSERT_EQ(kComponentTypeFrame, component->getType());
    auto text = component->getCoreChildAt(0);
    ASSERT_EQ(kComponentTypeText, text->getType());

    executeSetValue("frame", "backgroundColor", "red");
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    ASSERT_EQ(Color(session, "red"), component->getCalculated(kPropertyBackgroundColor).getColor());

    executeSetValue("frame", "borderColor", "red");
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    ASSERT_EQ(Color(session, "red"), component->getCalculated(kPropertyBorderColor).getColor());

    ASSERT_TRUE(CheckNoActions());
}

static const char *VIDEO_TEST =
        "{"
        "  \"type\": \"APL\","
        "  \"version\": \"1.1\","
        "  \"mainTemplate\": {"
        "    \"items\": {"
        "      \"type\": \"Video\","
        "      \"id\": \"video\","
        "      \"source\": \"https://video.com/video.mp4\""
        "    }"
        "  }"
        "}";

TEST_F(CommandSetValueTest, Video)
{
    loadDocument(VIDEO_TEST);

    ASSERT_EQ(kComponentTypeVideo, component->getType());

    auto source = component->getCalculated(kPropertySource);
    ASSERT_TRUE(source.isArray());
    ASSERT_EQ("https://video.com/video.mp4", source.at(0).getMediaSource().getUrl());
    executeSetValue("video", "source", "https://video.com/new_video.mp4");
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    source = component->getCalculated(kPropertySource);
    ASSERT_TRUE(source.isArray());
    ASSERT_EQ("https://video.com/new_video.mp4", source.at(0).getMediaSource().getUrl());

    ASSERT_TRUE(CheckNoActions());
}

static const char *BIND_CHANGE = "{"
                                 "  \"type\": \"APL\","
                                 "  \"version\": \"1.3\","
                                 "  \"mainTemplate\": {"
                                 "    \"item\": {"
                                 "      \"id\": \"main\","
                                 "      \"type\": \"Container\","
                                 "      \"bind\": ["
                                 "        {"
                                 "          \"name\": \"commonPrice\","
                                 "          \"value\": \"$3.50\""
                                 "        }"
                                 "      ],"
                                 "      \"items\": ["
                                 "        {"
                                 "          \"id\": \"text1\","
                                 "          \"type\": \"Text\","
                                 "          \"text\": \"Price1 ${commonPrice}\""
                                 "        },"
                                 "        {"
                                 "          \"id\": \"text2\","
                                 "          \"type\": \"Text\","
                                 "          \"text\": \"Price2 ${commonPrice}\""
                                 "        },"
                                 "        {"
                                 "          \"id\": \"text3\","
                                 "          \"type\": \"Text\","
                                 "          \"text\": \"Price3 ${commonPrice}\""
                                 "        }"
                                 "      ]"
                                 "    }"
                                 "  }"
                                 "}";

TEST_F(CommandSetValueTest, Bind)
{
    loadDocument(BIND_CHANGE);
    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeContainer, component->getType());

    auto text1 = component->findComponentById("text1");
    ASSERT_TRUE(text1);
    auto t1 = text1->getCalculated(kPropertyText).asString();
    ASSERT_EQ("Price1 $3.50", t1);

    auto text2 = component->findComponentById("text2");
    ASSERT_TRUE(text2);
    auto t2 = text2->getCalculated(kPropertyText).asString();
    ASSERT_EQ("Price2 $3.50", t2);

    auto text3 = component->findComponentById("text3");
    ASSERT_TRUE(text3);
    auto t3 = text3->getCalculated(kPropertyText).asString();
    ASSERT_EQ("Price3 $3.50", t3);

    // Let's introduce some tax here
    executeSetValue("main", "commonPrice", "$3.85");
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();

    t1 = text1->getCalculated(kPropertyText).asString();
    ASSERT_EQ("Price1 $3.85", t1);

    t2 = text2->getCalculated(kPropertyText).asString();
    ASSERT_EQ("Price2 $3.85", t2);

    t3 = text3->getCalculated(kPropertyText).asString();
    ASSERT_EQ("Price3 $3.85", t3);
}

static const char *DATA_BIND_OBJECT =  R"({"color": "#000000", "price": "$3.50"})";

static const char *BIND_OBJECT_CHANGE = "{"
                                 "  \"type\": \"APL\","
                                 "  \"version\": \"1.3\","
                                 "  \"mainTemplate\": {"
                                 "    \"parameters\": ["
                                 "      \"payload\""
                                 "    ],"
                                 "    \"item\": {"
                                 "      \"id\": \"main\","
                                 "      \"type\": \"Container\","
                                 "      \"bind\": ["
                                 "        {"
                                 "          \"name\": \"commonPrice\","
                                 "          \"value\": \"${payload.price}\""
                                 "        },"
                                 "        {"
                                 "          \"name\": \"commonColor\","
                                 "          \"value\": \"${payload.color}\""
                                 "        }"
                                 "      ],"
                                 "      \"items\": ["
                                 "        {"
                                 "          \"id\": \"text1\","
                                 "          \"type\": \"Text\","
                                 "          \"color\": \"${commonColor}\","
                                 "          \"text\": \"Price1 ${commonPrice}\""
                                 "        },"
                                 "        {"
                                 "          \"id\": \"text2\","
                                 "          \"type\": \"Text\","
                                 "          \"color\": \"${commonColor}\","
                                 "          \"text\": \"Price2 ${commonPrice}\""
                                 "        },"
                                 "        {"
                                 "          \"id\": \"text3\","
                                 "          \"type\": \"Text\","
                                 "          \"color\": \"${commonColor}\","
                                 "          \"text\": \"Price3 ${commonPrice}\""
                                 "        }"
                                 "      ]"
                                 "    }"
                                 "  }"
                                 "}";

TEST_F(CommandSetValueTest, BindObject)
{
    loadDocument(BIND_OBJECT_CHANGE, DATA_BIND_OBJECT);
    ASSERT_TRUE(component);
    ASSERT_EQ(kComponentTypeContainer, component->getType());

    auto text1 = component->findComponentById("text1");
    ASSERT_TRUE(text1);
    auto t1 = text1->getCalculated(kPropertyText).asString();
    auto c1 = text1->getCalculated(kPropertyColor).asString();
    ASSERT_EQ("Price1 $3.50", t1);
    ASSERT_EQ("#000000ff", c1);

    auto text2 = component->findComponentById("text2");
    ASSERT_TRUE(text2);
    auto t2 = text2->getCalculated(kPropertyText).asString();
    auto c2 = text1->getCalculated(kPropertyColor).asString();
    ASSERT_EQ("Price2 $3.50", t2);
    ASSERT_EQ("#000000ff", c2);

    auto text3 = component->findComponentById("text3");
    ASSERT_TRUE(text3);
    auto t3 = text3->getCalculated(kPropertyText).asString();
    auto c3 = text1->getCalculated(kPropertyColor).asString();
    ASSERT_EQ("Price3 $3.50", t3);
    ASSERT_EQ("#000000ff", c3);

    // Let's introduce some discount...+tax
    executeSetValue("main", "commonPrice", "$3.47");
    executeSetValue("main", "commonColor", "#FF0000");
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();

    t1 = text1->getCalculated(kPropertyText).asString();
    c1 = text1->getCalculated(kPropertyColor).asString();
    ASSERT_EQ("Price1 $3.47", t1);
    ASSERT_EQ("#ff0000ff", c1);

    t2 = text2->getCalculated(kPropertyText).asString();
    c2 = text1->getCalculated(kPropertyColor).asString();
    ASSERT_EQ("Price2 $3.47", t2);
    ASSERT_EQ("#ff0000ff", c2);

    t3 = text3->getCalculated(kPropertyText).asString();
    c3 = text1->getCalculated(kPropertyColor).asString();
    ASSERT_EQ("Price3 $3.47", t3);
    ASSERT_EQ("#ff0000ff", c3);
}


static const char *TEXT_LAYOUT_CHANGE = R"apl(
    {
      "type": "APL",
      "version": "1.5",
      "mainTemplate": {
        "items": {
          "type": "Container",
          "alignItems": "start",
          "items": {
            "type": "Text",
            "id": "MyText",
            "text": "Short phrase"
          }
        }
      }
    }
)apl";

TEST_F(CommandSetValueTest, TextLayout)
{
    loadDocument(TEXT_LAYOUT_CHANGE);
    ASSERT_TRUE(component);
    ASSERT_EQ(1, component->getChildCount());

    auto text = component->getChildAt(0);
    ASSERT_TRUE(IsEqual(Rect(0, 0, 120, 10), text->getCalculated(kPropertyBounds)));

    std::string s("Short phrase combined with a longer phrase");
    executeSetValue("MyText", "text", s);
    root->clearPending();  // This toggles the layout pass

    ASSERT_TRUE(CheckDirty(text, kPropertyBounds, kPropertyInnerBounds, kPropertyText));
    ASSERT_TRUE(CheckDirty(root, component, text));
    ASSERT_TRUE(IsEqual(Rect(0, 0, s.size() * 10, 10), text->getCalculated(kPropertyBounds)));
}