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
#include "apl/scenegraph/textpropertiescache.h"

using namespace apl;

class SGNodeBoundsTest : public DocumentWrapper {
public:
    std::shared_ptr<MyTestMeasurement> measure = std::make_shared<MyTestMeasurement>();
};

class SGNodeBoundsTestMediaObject : public MediaObject {
public:
    static MediaObjectPtr createImage(Size size) {
        auto result = std::make_shared<SGNodeBoundsTestMediaObject>();
        result->mType = EventMediaType::kEventMediaTypeImage;
        result->mSize = size;
        return result;
    }

    std::string url() const override { return "TestImage"; }
    State state() const override { return mState; }
    EventMediaType type() const override { return mType; }
    Size size() const override { return mSize; }
    int errorCode() const override { return 0; }
    std::string errorDescription() const override { return ""; }
    const HeaderArray& headers() const override { return mHeaders; }

    CallbackID addCallback(MediaObjectCallback callback) override { return 0; }
    void removeCallback(CallbackID callbackId) override {}

    Size mSize;
    State mState = MediaObject::State::kReady;
    EventMediaType mType = EventMediaType::kEventMediaTypeImage;
    HeaderArray mHeaders;
};

class SGNodeBoundsTestMediaPlayer : public MediaPlayer {
public:
    static MediaPlayerPtr create() {
        return std::make_shared<SGNodeBoundsTestMediaPlayer>();
    }

    SGNodeBoundsTestMediaPlayer() : MediaPlayer([](MediaPlayerEventType, const MediaState&){}) {};
    void release() override {}
    void halt() override {}
    void setTrackList(std::vector<MediaTrack> tracks) override {}
    void play(ActionRef actionRef) override {}
    void pause() override {}
    void next() override {}
    void previous() override {}
    void rewind() override {}
    void seek(int offset) override {}
    void setTrackIndex(int trackIndex) override {}
    void setAudioTrack(AudioTrack audioTrack) override {}
};

TEST_F(SGNodeBoundsTest, DrawNode)
{
    auto paint = sg::paint(Color(Color::BLACK));

    auto node = sg::draw(sg::path("L10,20 80,-20"), sg::fill(paint));
    ASSERT_TRUE(IsEqual(Rect(0, -20, 80, 40), node->boundingBox(Transform2D())));
    ASSERT_TRUE(IsEqual(Rect(0, -10, 40, 20), node->boundingBox(Transform2D::scale(0.5))));

    // Empty path
    node = sg::draw(sg::path("M10,10"), sg::fill(paint));
    ASSERT_TRUE(IsEqual(Rect(), node->boundingBox(Transform2D())));
    ASSERT_TRUE(IsEqual(Rect(), node->boundingBox(Transform2D::scale(20))));

    // Path with a stroke width
    node = sg::draw(sg::path("L10,20 80,-20"),
                    sg::stroke(paint).strokeWidth(4).lineJoin(apl::kGraphicLineJoinRound).get());
    ASSERT_TRUE(IsEqual(Rect(-2, -22, 84, 44), node->boundingBox(Transform2D())));
    ASSERT_TRUE(IsEqual(Rect(-1, -11, 42, 22), node->boundingBox(Transform2D::scale(0.5))));

    // A series of drawing operations - use the one with the maximum width
    auto op = sg::fill(paint);
    auto op2 = sg::stroke(paint).strokeWidth(4).lineJoin(apl::kGraphicLineJoinRound).get();
    auto op3 = sg::stroke(paint).strokeWidth(6).lineJoin(apl::kGraphicLineJoinRound).get();
    op2->nextSibling = op3;
    op->nextSibling = op2;
    node = sg::draw(sg::path("L10,20 80,-20"), op);
    ASSERT_TRUE(IsEqual(Rect(-3, -23, 86, 46), node->boundingBox(Transform2D())));
    ASSERT_TRUE(IsEqual(Rect(-6, -46, 172, 92), node->boundingBox(Transform2D::scale(2))));
}

TEST_F(SGNodeBoundsTest, TextNode)
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
    auto node = sg::text(textLayout, op, Range(0,1));

    ASSERT_TRUE(IsEqual(Rect(0,0,96,24), node->boundingBox(Transform2D())));
    ASSERT_TRUE(IsEqual(Rect(10,10,96,24), node->boundingBox(Transform2D::translate(10,10))));
    ASSERT_TRUE(IsEqual(Rect(0,0,48,12), node->boundingBox(Transform2D::scale(0.5))));

    sg::TextNode::cast(node)->setRange(Range(0,0));
    ASSERT_TRUE(IsEqual(Rect(0,0,96,12), node->boundingBox(Transform2D())));
    ASSERT_TRUE(IsEqual(Rect(10,10,96,12), node->boundingBox(Transform2D::translate(10,10))));
    ASSERT_TRUE(IsEqual(Rect(0,0,48,6), node->boundingBox(Transform2D::scale(0.5))));

    op->nextSibling = sg::stroke(paint).strokeWidth(2).lineJoin(apl::kGraphicLineJoinRound).get();
    ASSERT_TRUE(IsEqual(Rect(-1,-1,98,14), node->boundingBox(Transform2D())));
    ASSERT_TRUE(IsEqual(Rect(9,9,98,14), node->boundingBox(Transform2D::translate(10,10))));
    ASSERT_TRUE(IsEqual(Rect(-0.5,-0.5,49,7), node->boundingBox(Transform2D::scale(0.5))));
}

TEST_F(SGNodeBoundsTest, ImageNode)
{
    auto node = sg::image(sg::filter(SGNodeBoundsTestMediaObject::createImage({200, 300})),
                          Rect{20, 20, 100, 100}, Rect{0, 0, 100, 100});
    ASSERT_TRUE(IsEqual(Rect(20, 20, 100, 100), node->boundingBox(Transform2D())));
    ASSERT_TRUE(
        IsEqual(Rect(0, 0, 100, 100), node->boundingBox(Transform2D::translate(-20, -20))));
    ASSERT_TRUE(IsEqual(Rect(40, 40, 200, 200), node->boundingBox(Transform2D::scale(2))));
}

TEST_F(SGNodeBoundsTest, VideoNode)
{
    auto node = sg::video(SGNodeBoundsTestMediaPlayer::create(), Rect(20, 20, 100, 100),
                          VideoScale::kVideoScaleBestFill);
    ASSERT_TRUE(IsEqual(Rect(20, 20, 100, 100), node->boundingBox(Transform2D())));
    ASSERT_TRUE(
        IsEqual(Rect(0, 0, 100, 100), node->boundingBox(Transform2D::translate(-20, -20))));
    ASSERT_TRUE(IsEqual(Rect(40, 40, 200, 200), node->boundingBox(Transform2D::scale(2))));
}

class SampleEditText : public sg::EditText {
public:
    SampleEditText() : EditText([]() {}, [](const std::string& text) {}, [](bool isFocused) {}) {}

    void release() override {}
    void setFocus(bool hasFocus) override {}
};

class SampleEditTextBox : public sg::EditTextBox {
    Size getSize() const override { return {100, 2}; }
    float getBaseline() const override { return 14.0f; }
};

TEST_F(SGNodeBoundsTest, EditNode)
{
    auto editText = std::make_shared<SampleEditText>();
    auto editTextBox = std::make_shared<SampleEditTextBox>();
    sg::TextPropertiesCache cache;
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

    ASSERT_TRUE(IsEqual(Rect(), node->boundingBox(Transform2D())));
}

TEST_F(SGNodeBoundsTest, CombiningNodes)
{
    auto paint = sg::paint(Color(Color::BLACK));
    auto node = sg::draw(sg::path("L10,20"), sg::fill(paint));

    // Transforms
    ASSERT_TRUE(IsEqual(Rect(0, 0, 10, 20), node->boundingBox(Transform2D())));
    auto transform = sg::transform(Transform2D::translate(5, 10), node);
    ASSERT_TRUE(IsEqual(Rect(5, 10, 10, 20), transform->boundingBox(Transform2D())));
    transform = sg::transform(Transform2D::rotate(90), node);
    ASSERT_TRUE(IsEqual(Rect(-20, 0, 20, 10), transform->boundingBox(Transform2D())));
    // Stack a transform
    transform = sg::transform(Transform2D::scale(2), transform);
    ASSERT_TRUE(IsEqual(Rect(-40, 0, 40, 20), transform->boundingBox(Transform2D())));

    // Clip nodes
    auto clip = sg::clip(sg::path(Rect{2, 3, 50, 5}), node);
    ASSERT_TRUE(IsEqual(Rect(2, 3, 8, 5), clip->boundingBox(Transform2D()))); // Intersection

    // Opacity node
    auto opacity = sg::opacity(0.0f, node); // Doesn't do anything
    ASSERT_TRUE(IsEqual(Rect(0, 0, 10, 20), opacity->boundingBox(Transform2D())));

    // Shadow node - missing
    ASSERT_TRUE(
        IsEqual(Rect(0, 0, 10, 20), sg::shadowNode(nullptr, node)->boundingBox(Transform2D())));

    // Sharp shadows
    auto sharp = sg::shadow(Color::BLACK, Point{5, 10}, 0.0f);
    ASSERT_TRUE(
        IsEqual(Rect(0, 0, 15, 30), sg::shadowNode(sharp, node)->boundingBox(Transform2D())));

    sharp = sg::shadow(Color::BLACK, Point{-5, -10}, 0.0f); // Go the other direction
    ASSERT_TRUE(
        IsEqual(Rect(-5, -10, 15, 30), sg::shadowNode(sharp, node)->boundingBox(Transform2D())));

    auto blurry = sg::shadow(Color::BLACK, Point{0, 0}, 4.0f); // Blurry, no shift
    ASSERT_TRUE(
        IsEqual(Rect(-4, -4, 18, 28), sg::shadowNode(blurry, node)->boundingBox(Transform2D())));

    auto offset_blurry = sg::shadow(Color::BLACK, Point{3, 5}, 4.0f);
    ASSERT_TRUE(IsEqual(Rect(-1, 0, 18, 29),
                          sg::shadowNode(offset_blurry, node)->boundingBox(Transform2D())));

    offset_blurry = sg::shadow(Color::BLACK, Point{6, -3}, 4.0f);
    ASSERT_TRUE(IsEqual(Rect(0, -7, 20, 28),
                          sg::shadowNode(offset_blurry, node)->boundingBox(Transform2D())));
}

TEST_F(SGNodeBoundsTest, NodeSiblings)
{
    auto paint = sg::paint(Color(Color::BLACK));

    auto n1 = sg::draw(sg::path("l10,20"), sg::fill(paint));
    auto n2 = sg::draw(sg::path("M6,2 l15,25"), sg::fill(paint));
    auto n3 = sg::draw(sg::path("l10,50"), sg::fill(paint));
    auto n4 = sg::draw(sg::path("M-14,18 l15,15"), sg::fill(paint));

    n1->setNext(n2);
    n2->setNext(n3);
    n3->setNext(n4);

    // Measuring the size of a node just gets that node itself and any children
    ASSERT_TRUE(IsEqual(Rect(0, 0, 10, 20), n1->boundingBox(Transform2D())));

    // Use the calculate method to include siblings
    ASSERT_TRUE(IsEqual(Rect(-14, 0, 35, 50), sg::Node::calculateBoundingBox(n1, Transform2D())));

    // Adding a parent node captures just the siblings in the chain.  In this case, n3 & n4
    auto t = sg::transform(Transform2D::scale(0.5), n3);
    ASSERT_TRUE(IsEqual(Rect(-7, 0, 12, 25), t->boundingBox(Transform2D())));
    ASSERT_TRUE(IsEqual(Rect(-14, 0, 24, 50), t->boundingBox(Transform2D::scale(2))));
}


