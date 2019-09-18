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

#include "testeventloop.h"

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

    // First check that non-dynamic properties can't be set.
    executeSetValue("tw", "accessibilityLabel", "New one");
    ASSERT_FALSE(root->isDirty());
    ASSERT_EQ("", component->getCalculated(kPropertyAccessibilityLabel).asString());
    ASSERT_TRUE(ConsoleMessage());

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

    auto transform = component->getCalculated(kPropertyTransform).getTransform2D();
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