/*
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
*
*/

#include "../testeventloop.h"
#include "test_sg.h"

#include "apl/scenegraph/builder.h"
#include "apl/scenegraph/node.h"
#include "apl/scenegraph/textpropertiescache.h"

using namespace apl;

class SGNodeTest : public ::testing::Test {
public:
    std::shared_ptr<MyTestMeasurement> measure = std::make_shared<MyTestMeasurement>();
};


TEST_F(SGNodeTest, DrawNode)
{
    auto path = sg::path(Rect(0, 10, 20, 30));
    auto op = sg::fill(sg::paint(Color(Color::BLUE)));
    auto node = sg::draw(path, op);
    ASSERT_EQ(node->toDebugString(), "DrawNode");
    ASSERT_TRUE(node->visible());

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(node->serialize(doc.GetAllocator()), StringToMapObject(R"apl(
        {
            "type": "draw",
            "path": {
                "type": "rectPath",
                "rect": [0.0,10.0,20.0,30.0]
            },
            "op": [
                {
                    "paint": {
                        "opacity": 1.0,
                        "type": "colorPaint",
                        "color": "#0000ffff"
                    },
                    "type": "fill",
                    "fillType": "even-odd"
                }
            ]
        }
    )apl")));

    ASSERT_TRUE(sg::DrawNode::is_type(node));
    auto draw = sg::DrawNode::cast(node);
    ASSERT_TRUE(draw);

    ASSERT_FALSE(draw->setPath(path));
    ASSERT_TRUE(draw->setPath(sg::path(Rect(0, 10, 20, 31))));

    ASSERT_TRUE(op->paint->setOpacity(0.0f));
    ASSERT_FALSE(draw->visible());

    ASSERT_FALSE(draw->setOp(op));
    ASSERT_TRUE(draw->setOp(sg::fill(sg::paint(Color(Color::RED)))));
}


TEST_F(SGNodeTest, TextNode)
{
    sg::TextPropertiesCache cache;
    auto chunk = sg::TextChunk::create(StyledText::createRaw("hello, world"));
    auto properties =
        sg::TextProperties::create(cache, {"Arial"}, 12, FontStyle::kFontStyleNormal, 500);
    auto textLayout = measure->layout(chunk, properties,
                                      100, // Width
                                      MeasureMode::AtMost,
                                      100, // Height
                                      MeasureMode::Exactly);
    auto paint = sg::paint(Color(Color::RED));
    auto op = sg::fill(paint);
    auto node = sg::text(textLayout, op, Range(0,5));

    ASSERT_EQ("TextNode size=96.000000x100.000000 range=Range<0,5> text=hello, world", node->toDebugString());
    ASSERT_TRUE(node->visible());

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(node->serialize(doc.GetAllocator()), StringToMapObject(R"apl(
        {
            "type": "text",
            "op": [
                {
                    "paint": {
                        "opacity": 1.0,
                        "type": "colorPaint",
                        "color": "#ff0000ff"
                    },
                    "type": "fill",
                    "fillType": "even-odd"}
            ],
            "range":{
                "lowerBound":0,
                "upperBound":5
            },
            "layout": null
        }
    )apl")));

    ASSERT_TRUE(sg::TextNode::is_type(node));
    auto text = sg::TextNode::cast(node);
    ASSERT_TRUE(text);

    ASSERT_FALSE(text->setTextLayout(textLayout));
    ASSERT_FALSE(text->setRange(Range(0,5)));
    ASSERT_TRUE(text->setRange(Range(0,4)));

    ASSERT_TRUE(op->paint->setOpacity(0.0f));
    ASSERT_FALSE(node->visible());

    ASSERT_FALSE(text->setOp(op));
    ASSERT_TRUE(text->setOp(sg::fill(sg::paint(Color(Color::TEAL)))));
}


TEST_F(SGNodeTest, TransformNode)
{
    auto node = sg::transform();
    ASSERT_EQ("TransformNode transform=Transform2D<1.000000, 0.000000, 0.000000, 1.000000, 0.000000, 0.000000>", node->toDebugString());
    ASSERT_FALSE(node->visible());

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(node->serialize(doc.GetAllocator()), StringToMapObject(R"apl(
        {
            "type":"transform",
            "transform": [1,0,0,1,0,0]
        }
    )apl")));

    ASSERT_TRUE(sg::TransformNode::is_type(node));
    auto transform = sg::TransformNode::cast(node);
    ASSERT_TRUE(transform);

    ASSERT_FALSE(transform->setTransform(Transform2D()));
    ASSERT_TRUE(transform->setTransform(Transform2D::scale(2)));
}


TEST_F(SGNodeTest, ClipNode)
{
    auto path = sg::path(Rect(0,0,20,20));
    auto child = sg::transform();
    auto node = sg::clip(path, child);

    ASSERT_EQ("ClipNode", node->toDebugString());
    ASSERT_FALSE(node->visible());

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(node->serialize(doc.GetAllocator()), StringToMapObject(R"apl(
        {
            "type":"clip",
            "path": {
                "type": "rectPath",
                "rect": [0, 0, 20, 20]
            },
            "children":[
                {
                    "type":"transform",
                    "transform":[1,0,0,1,0,0]
                }
            ]
        }
    )apl")));

    ASSERT_TRUE(sg::ClipNode::is_type(node));
    auto clip = sg::ClipNode::cast(node);
    ASSERT_TRUE(clip);

    ASSERT_FALSE(clip->setPath(path));
    ASSERT_TRUE(clip->setPath(sg::path(Rect(0,0,20,21))));
}


TEST_F(SGNodeTest, OpacityNode)
{
    auto child = sg::transform();
    auto node = sg::opacity(0.5f, child);

    ASSERT_EQ("OpacityNode opacity=0.500000", node->toDebugString());
    ASSERT_FALSE(node->visible());

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(node->serialize(doc.GetAllocator()), StringToMapObject(R"apl(
        {
            "type":"opacity",
            "opacity": 0.5,
            "children":[
                {
                    "type":"transform",
                    "transform":[1,0,0,1,0,0]
                }
            ]
        }
    )apl")));

    ASSERT_TRUE(sg::OpacityNode::is_type(node));
    auto opacity = sg::OpacityNode::cast(node);
    ASSERT_TRUE(opacity);

    ASSERT_FALSE(opacity->setOpacity(0.5f));
    ASSERT_TRUE(opacity->setOpacity(1.0f));
}


TEST_F(SGNodeTest, ImageNode)
{
    auto paint = sg::paint(Color(Color::RED));
    auto filter = sg::solid(paint);
    auto node = sg::image(filter, Rect(0, 0, 100, 100), Rect(0, 0, 1, 1));

    ASSERT_EQ("ImageNode target=Rect<100x100+0+0> source=Rect<1x1+0+0>", node->toDebugString());
    ASSERT_TRUE(node->visible());

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(node->serialize(doc.GetAllocator()), StringToMapObject(R"apl(
        {
            "type":"image",
            "target": [0, 0, 100, 100],
            "source": [0, 0, 1, 1],
            "image":{
                "type":"solidFilter",
                "paint":{
                    "opacity":1,
                    "type":"colorPaint",
                    "color":"#ff0000ff"
                }
            }
        }
    )apl")));

    ASSERT_TRUE(sg::ImageNode::is_type(node));
    auto image = sg::ImageNode::cast(node);
    ASSERT_TRUE(image);

    ASSERT_FALSE(image->setTarget(Rect(0, 0, 100, 100)));
    ASSERT_TRUE(image->setTarget(Rect(0, 0, 100, 101)));

    ASSERT_FALSE(image->setSource(Rect(0, 0, 1, 1)));
    ASSERT_TRUE(image->setSource(Rect(0, 0, 2, 1)));

    ASSERT_FALSE(image->setImage(filter));
    ASSERT_TRUE(image->setImage(sg::solid(sg::paint(Color(Color::TRANSPARENT)))));
    ASSERT_FALSE(node->visible());
}


class TrivialMediaPlayer : public MediaPlayer {
public:
    TrivialMediaPlayer(MediaPlayerCallback callback, const std::string& name)
        : MediaPlayer(callback),
          mName(name) {}

    void release() override {}
    void halt() override {}
    void setTrackList(std::vector<MediaTrack> tracks) override {}
    void play(ActionRef actionRef) override {}
    void pause() override {}
    void next() override {}
    void previous() override {}
    void rewind() override {}
    void seek( int offset ) override {}
    void setTrackIndex( int trackIndex ) override {}
    void setAudioTrack( AudioTrack audioTrack ) override {}

    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const override {
        auto result = rapidjson::Value(rapidjson::kObjectType);
        result.AddMember("name", rapidjson::Value(mName.c_str(), allocator).Move(), allocator);
        return result;
    }

private:
    std::string mName;
};


TEST_F(SGNodeTest, VideoNode)
{
    auto player = std::make_shared<TrivialMediaPlayer>([](MediaPlayerEventType eventType,
                                                                   const MediaState& state){}, "Foobar");
    auto node = sg::video(player, Rect(0, 0, 100, 100), VideoScale::kVideoScaleBestFill);

    ASSERT_EQ("VideoNode target=Rect<100x100+0+0> PLAYER", node->toDebugString());
    ASSERT_TRUE(node->visible());

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(node->serialize(doc.GetAllocator()), StringToMapObject(R"apl(
        {
            "type": "video",
            "target": [0, 0, 100, 100],
            "scale": "best-fill",
            "player":{
                "name": "Foobar"
            }
        }
    )apl")));

    ASSERT_TRUE(sg::VideoNode::is_type(node));
    auto video = sg::VideoNode::cast(node);
    ASSERT_TRUE(video);

    ASSERT_FALSE(video->setTarget(Rect(0, 0, 100, 100)));
    ASSERT_TRUE(video->setTarget(Rect(0, 0, 100, 101)));

    ASSERT_FALSE(video->setMediaPlayer(player));

    player = std::make_shared<TrivialMediaPlayer>([](MediaPlayerEventType eventType,
                                                     const MediaState& state){}, "New player");
    ASSERT_TRUE(video->setMediaPlayer(player));
    ASSERT_TRUE(node->visible());

    ASSERT_FALSE(video->setScale(VideoScale::kVideoScaleBestFill));
    ASSERT_TRUE(video->setScale(VideoScale::kVideoScaleBestFit));
}


TEST_F(SGNodeTest, ShadowNode)
{
    auto child = sg::transform();
    auto shadow = sg::shadow(Color(Color::FUCHSIA), Point(5,5), 3.0f);
    auto node = sg::shadowNode(shadow, child);

    ASSERT_EQ("ShadowNode", node->toDebugString());
    ASSERT_FALSE(node->visible());

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(node->serialize(doc.GetAllocator()), StringToMapObject(R"apl(
        {
            "type":"shadow",
            "shadow":{
                "color":"#ff00ffff",
                "offset":[5,5],
                "radius":3.0
            },
            "children":[
                {
                    "type":"transform",
                    "transform":[1,0,0,1,0,0]
                }
            ]
        }
    )apl")));

    ASSERT_TRUE(sg::ShadowNode::is_type(node));
    auto shadowNode = sg::ShadowNode::cast(node);
    ASSERT_TRUE(shadowNode);

    ASSERT_FALSE(shadowNode->setShadow(shadow));
    ASSERT_TRUE(shadowNode->setShadow(sg::shadow(Color(Color::YELLOW), Point(5,5), 3.0f)));
}


class TrivialEditText : public sg::EditText {
public:
    TrivialEditText() : EditText([]() {}, [](const std::string& text) {}, [](bool isFocused) {}) {}

    void release() override {}
    void setFocus(bool hasFocus) override {}
};

class TrivialEditTextBox : public sg::EditTextBox {
    Size getSize() const override { return Size(100, 20); }
    float getBaseline() const override { return 14.0f; }
};

TEST_F(SGNodeTest, EditTextNode)
{
    sg::TextPropertiesCache cache;

    auto editText = std::make_shared<TrivialEditText>();
    auto editTextBox = std::make_shared<TrivialEditTextBox>();
    auto properties =
        sg::TextProperties::create(cache, {"Arial"}, 12, FontStyle::kFontStyleNormal, 500);

    auto editTextConfig = sg::EditTextConfig::create(
            Color(Color::RED),
            Color(Color::BLUE),
            KeyboardType::kKeyboardTypeEmailAddress,
            "klingon",
            23,
            false,  // Secure input
            SubmitKeyType::kSubmitKeyTypeGo,
            "a-zA-Z@.",
            false, // Select on focus
            KeyboardBehaviorOnFocus::kBehaviorOnFocusSystemDefault,
            properties
        );

    auto node = sg::editText(editText, editTextBox, editTextConfig, "Hello, world!");

    ASSERT_EQ("EditTextNode text=Hello, world! color=#ff0000ff", node->toDebugString());
    ASSERT_TRUE(node->visible());

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(node->serialize(doc.GetAllocator()), StringToMapObject(R"apl(
        {
            "type": "edit",
            "box": {
                "size": [100,20],
                "baseline": 14
            },
            "config": {
                "textColor": "#ff0000ff",
                "highlightColor": "#0000ffff",
                "keyboardType": "emailAddress",
                "keyboardBehaviorOnFocus": "systemDefault",
                "language": "klingon",
                "maxLength": 23,
                "secureInput": false,
                "selectOnFocus": false,
                "submitKeyType": "go",
                "validCharacters": "a-zA-Z@.",
                "textProperties": {
                    "fontFamily": ["Arial"],
                    "fontSize": 12,
                    "fontStyle": "normal",
                    "fontWeight": 500,
                    "letterSpacing": 0,
                    "lineHeight": 1.25,
                    "maxLines": 0,
                    "textAlign": "auto",
                    "textAlignVertical": "auto"
                }
            },
            "text": "Hello, world!"
        }
    )apl")));

    ASSERT_TRUE(sg::EditTextNode::is_type(node));
    auto edit = sg::EditTextNode::cast(node);
    ASSERT_TRUE(edit);

    ASSERT_FALSE(edit->setEditText(editText));
    ASSERT_TRUE(edit->setEditText(std::make_shared<TrivialEditText>()));

    ASSERT_FALSE(edit->setEditTextBox(editTextBox));
    ASSERT_TRUE(edit->setEditTextBox(std::make_shared<TrivialEditTextBox>()));

    ASSERT_FALSE(edit->setEditTextConfig(editTextConfig));
    ASSERT_TRUE(edit->setEditTextConfig( sg::EditTextConfig::create(
        Color(Color::RED),
        Color(Color::BLUE),
        KeyboardType::kKeyboardTypeEmailAddress,
        "klingon",
        23,
        false,  // Secure input
        SubmitKeyType::kSubmitKeyTypeGo,
        "a-zA-Z@.",
        false, // Select on focus
        KeyboardBehaviorOnFocus::kBehaviorOnFocusSystemDefault,
        properties
        )));

    ASSERT_FALSE(edit->setText("Hello, world!"));
    ASSERT_TRUE(edit->setText("Goodbye..."));
}

