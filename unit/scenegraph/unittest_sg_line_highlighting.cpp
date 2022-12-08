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

#include "../audio/audiotest.h"
#include "test_sg.h"

using namespace apl;

class AudioHighlightTest : public AudioTest {
public:
    AudioHighlightTest() : AudioTest() {
        config->measure(std::make_shared<MyTestMeasurement>());
    }
};

static const char *BASIC = R"apl(
{
  "type": "APL",
  "version": "1.8",
  "styles": {
    "TextStyle": {
      "values": [
        { "color": "blue" },
        { "color": "red", "when": "${state.karaoke}" },
        { "color": "green", "when": "${state.karaokeTarget}" }
      ]
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "TEXT",
      "width": 100,
      "height": 100,
      "fontSize": 20,
      "style": "TextStyle",
      "speech": "http://foo.com",
      "text": "Fuzzy duck"
    }
  }
}
)apl";

TEST_F(AudioHighlightTest, Basic)
{
    factory->addFakeContent({
        {"http://foo.com",
         1000,
         200,
         -1,
         {{kSpeechMarkSentence, 0, 10, 0, "Fuzzy duck"},
          {kSpeechMarkWord, 0, 5, 0, "Fuzzy"},
          {kSpeechMarkWord, 6, 10, 500, "duck"}}} // 1000 ms long, 200 ms buffer delay
    });

    loadDocument(BASIC);
    ASSERT_TRUE(component);

    // Check the initial color
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), component->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), component->getCalculated(kPropertyColorKaraokeTarget)));
    ASSERT_TRUE(IsEqual(Range(), component->getCalculated(kPropertyRangeKaraokeTarget)));

    // ======= Execute SpeakItem ========
    executeCommand("SpeakItem", {{"componentId", "TEXT"}, {"highlightMode", "line"}}, false);

    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    // Switched to karaoke state, but no highlighted line
    ASSERT_TRUE(IsEqual(Color(Color::RED), component->getCalculated(kPropertyColor)));
    ASSERT_TRUE(
        IsEqual(Color(Color::GREEN), component->getCalculated(kPropertyColorKaraokeTarget)));
    ASSERT_TRUE(IsEqual(Range(), component->getCalculated(kPropertyRangeKaraokeTarget)));

    ASSERT_TRUE(
        CheckDirty(component, kPropertyColor, kPropertyColorKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component));

    // ======= Advance to the start of audio playback ========
    advanceTime(200);
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kPlay));
    ASSERT_FALSE(factory->hasEvent());

    // We've got the first speech mark, so we have Karaoke-Target state
    ASSERT_TRUE(IsEqual(Color(Color::RED), component->getCalculated(kPropertyColor)));
    ASSERT_TRUE(
        IsEqual(Color(Color::GREEN), component->getCalculated(kPropertyColorKaraokeTarget)));
    ASSERT_TRUE(IsEqual(Range(0, 0), component->getCalculated(kPropertyRangeKaraokeTarget)));

    ASSERT_TRUE(CheckDirty(component, kPropertyRangeKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component));

    // ======= Advance to the next speech mark ========
    advanceTime(500);
    ASSERT_FALSE(factory->hasEvent());

    // Second line is highlighted
    ASSERT_TRUE(IsEqual(Color(Color::RED), component->getCalculated(kPropertyColor)));
    ASSERT_TRUE(
        IsEqual(Color(Color::GREEN), component->getCalculated(kPropertyColorKaraokeTarget)));
    ASSERT_TRUE(IsEqual(Range(1, 1), component->getCalculated(kPropertyRangeKaraokeTarget)));

    ASSERT_TRUE(CheckDirty(component, kPropertyRangeKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component));

    // ======= Advance to the end of audio playback ========
    advanceTime(500);
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    // Everything is unhighlighted again
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), component->getCalculated(kPropertyColor)));
    ASSERT_TRUE(IsEqual(Color(Color::BLUE), component->getCalculated(kPropertyColorKaraokeTarget)));
    ASSERT_TRUE(IsEqual(Range(), component->getCalculated(kPropertyRangeKaraokeTarget)));

    ASSERT_TRUE(CheckDirty(component, kPropertyColor, kPropertyColorKaraokeTarget,
                           kPropertyRangeKaraokeTarget, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component));
}

// The same test as above, but instead of checking dirty properties we check the scene graph.
// We can't do both because the scene takes care of fixing dirty properties.
TEST_F(AudioHighlightTest, BasicSceneGraph)
{
    factory->addFakeContent({
        {"http://foo.com",
         1000,
         200,
         -1,
         {{kSpeechMarkSentence, 0, 10, 0, "Fuzzy duck"},
          {kSpeechMarkWord, 0, 5, 0, "Fuzzy"},
          {kSpeechMarkWord, 6, 10, 500, "duck"}}} // 1000 ms long, 200 ms buffer delay
    });

    loadDocument(BASIC);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(sg);

    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 100, 100}, "...Text")
                .content(IsTransformNode().child(
                    IsTextNode().text("Fuzzy duck").pathOp(IsFillOp(IsColorPaint(Color::BLUE)))))));

    // ======= Execute SpeakItem ========
    executeCommand("SpeakItem", {{"componentId", "TEXT"}, {"highlightMode", "line"}}, false);

    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    // Switched to karaoke state, but no highlighted line
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 100, 100}, "...Text")
                .dirty(sg::Layer::kFlagRedrawContent)
                .content(IsTransformNode().child(
                    IsTextNode().text("Fuzzy duck").pathOp(IsFillOp(IsColorPaint(Color::RED)))))));

    // ======= Advance to the start of audio playback ========
    advanceTime(200);
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kPlay));
    ASSERT_FALSE(factory->hasEvent());

    // We've got the first speech mark, so we have Karaoke-Target state
    sg = root->getSceneGraph();
    ASSERT_TRUE(
        CheckSceneGraph(sg, IsLayer(Rect{0, 0, 100, 100}, "...Text")
                                .dirty(sg::Layer::kFlagRedrawContent)
                                .content(IsTransformNode().child(
                                    IsTextNode()
                                        .text("Fuzzy duck")
                                        .range({0, 0})
                                        .pathOp(IsFillOp(IsColorPaint(Color::GREEN)))
                                        .next(IsTextNode()
                                                  .text("Fuzzy duck")
                                                  .range({1, 1})
                                                  .pathOp(IsFillOp(IsColorPaint(Color::RED))))))));

    // ======= Advance to the next speech mark ========
    advanceTime(500);
    ASSERT_FALSE(factory->hasEvent());

    // Second line is highlighted
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 100, 100}, "...Text")
                .dirty(sg::Layer::kFlagRedrawContent)
                .content(IsTransformNode().child(
                    IsTextNode()
                        .text("Fuzzy duck")
                        .range({0, 0})
                        .pathOp(IsFillOp(IsColorPaint(Color::RED)))
                        .next(IsTextNode()
                                  .text("Fuzzy duck")
                                  .range({1, 1})
                                  .pathOp(IsFillOp(IsColorPaint(Color::GREEN))))))));

    // ======= Advance to the end of audio playback ========
    advanceTime(500);
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    // Everything is unhighlighted again
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 100, 100}, "...Text")
                .dirty(sg::Layer::kFlagRedrawContent)
                .content(IsTransformNode().child(
                    IsTextNode().text("Fuzzy duck").pathOp(IsFillOp(IsColorPaint(Color::BLUE)))))));
}



static const char *SCROLLING = R"apl(
{
  "type": "APL",
  "version": "1.8",
  "onConfigChange": {
    "type": "Reinflate",
    "preservedSequencers": ["MAGIC"]
  },
  "styles": {
    "TextStyle": {
      "values": [
        {
          "color": "blue"
        },
        {
          "color": "red",
          "when": "${state.karaoke}"
        },
        {
          "color": "green",
          "when": "${state.karaokeTarget}"
        }
      ]
    }
  },
  "mainTemplate": {
    "item": {
      "type": "ScrollView",
      "width": 100,
      "height": 60,
      "items": {
        "type": "Text",
        "id": "TEXT",
        "width": 100,
        "fontSize": 20,
        "style": "TextStyle",
        "speech": "http://foo.com",
        "text": "Line1Line2Line3Line4Line5"
      }
    }
  }
}
)apl";

/**
 * This test checks for scrolling each line into view
 *
 * Note that each line can hold exactly 5 characters, so we should get a text box of the form:
 *
 * Line1
 * Line2
 * Line3
 * Line4
 * Line5
 */
TEST_F(AudioHighlightTest, Scrolling) {
    config->set(RootProperty::kScrollCommandDuration, 50);

    factory->addFakeContent({{"http://foo.com",
                              1000, // Overall length 1000 ms
                              200,  // 200 ms delay at start
                              -1,
                              {{kSpeechMarkWord, 10, 15, 100, "Line1"},
                               {kSpeechMarkWord, 30, 35, 300, "Line2"},
                               {kSpeechMarkWord, 50, 55, 500, "Line3"},
                               {kSpeechMarkWord, 70, 75, 700, "Line4"},
                               {kSpeechMarkWord, 90, 95, 900, "Line5"}}}});

    loadDocument(SCROLLING);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(sg);

    // At the start we have five lines of text. The first three are visible
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 100, 60}, "...ScrollView")
                .vertical()
                .child(IsLayer(Rect{0, 0, 100, 100}, "...Text")
                           .content(IsTransformNode().child(
                               IsTextNode()
                                   .text("Line1Line2Line3Line4Line5")
                                   .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))))));

    // Execute SpeakItem with line highlighting.  Align the line to "first"
    executeCommand("SpeakItem",
                   {{"componentId", "TEXT"},
                    {"sequencer", "MAGIC"},
                    {"highlightMode", "line"},
                    {"align", "first"}},
                   false);

    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    root->clearPending(); // There's a zero-duration scroll command that needs to be cleared

    // Switched to karaoke state, but no highlighted line
    sg = root->getSceneGraph();
    ASSERT_TRUE(
        CheckSceneGraph(sg, IsLayer(Rect{0, 0, 100, 60}, "...ScrollView")
                                .vertical()
                                .child(IsLayer(Rect{0, 0, 100, 100}, "...Text")
                                           .dirty(sg::Layer::kFlagRedrawContent)
                                           .content(IsTransformNode().child(
                                               IsTextNode()
                                                   .text("Line1Line2Line3Line4Line5")
                                                   .pathOp(IsFillOp(IsColorPaint(Color::RED))))))));

    ASSERT_FALSE(factory->hasEvent());

    // ========== Advance time past the initial delay ===========
    advanceTime(200);

    // The player posts Ready and Play
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kPlay));
    ASSERT_FALSE(factory->hasEvent());

    // The scene graph has not changed - no Karaoke yet
    sg = root->getSceneGraph();
    ASSERT_TRUE(
        CheckSceneGraph(sg, IsLayer(Rect{0, 0, 100, 60}, "...ScrollView")
                                .vertical()
                                .child(IsLayer(Rect{0, 0, 100, 100}, "...Text")
                                           .content(IsTransformNode().child(
                                               IsTextNode()
                                                   .text("Line1Line2Line3Line4Line5")
                                                   .pathOp(IsFillOp(IsColorPaint(Color::RED))))))));

    // ========== The first karaoke word hits ===========
    advanceTime(100);

    // The first line turns GREEN
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 100, 60}, "...ScrollView")
                .vertical()
                .child(IsLayer(Rect{0, 0, 100, 100}, "...Text")
                           .dirty(sg::Layer::kFlagRedrawContent)
                           .content(IsTransformNode().child(
                               IsTextNode()
                                   .text("Line1Line2Line3Line4Line5")
                                   .range({0, 0})
                                   .pathOp(IsFillOp(IsColorPaint(Color::GREEN)))
                                   .next(IsTextNode()
                                             .text("Line1Line2Line3Line4Line5")
                                             .range({1, 4})
                                             .pathOp(IsFillOp(IsColorPaint(Color::RED))))

                                   )))));

    // ========== The second karaoke word hits.  Starts scrolling Line2 ===========
    advanceTime(200);

    // The second line turns GREEN
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 100, 60}, "...ScrollView")
            .vertical()
            .child(
                IsLayer(Rect{0, 0, 100, 100}, "...Text")
                    .dirty(sg::Layer::kFlagRedrawContent)
                    .content(IsTransformNode().child(
                        IsTextNode()
                            .text("Line1Line2Line3Line4Line5")
                            .range({0, 0})
                            .pathOp(IsFillOp(IsColorPaint(Color::RED)))
                            .next(IsTextNode()
                                      .text("Line1Line2Line3Line4Line5")
                                      .range({1, 1})
                                      .pathOp(IsFillOp(IsColorPaint(Color::GREEN)))
                                      .next(IsTextNode()
                                                .text("Line1Line2Line3Line4Line5")
                                                .range({2, 4})
                                                .pathOp(IsFillOp(IsColorPaint(Color::RED))))))))));

    // ========== Advance past the initial scrolling but before the next word ===========
    advanceTime(100);

    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 100, 60}, "...ScrollView")
            .vertical()
            .dirty(sg::Layer::kFlagChildOffsetChanged)
            .childOffset(Point(0, 20))
            .child(
                IsLayer(Rect{0, 0, 100, 100}, "...Text")
                    .content(IsTransformNode().child(
                        IsTextNode()
                            .text("Line1Line2Line3Line4Line5")
                            .range({0, 0})
                            .pathOp(IsFillOp(IsColorPaint(Color::RED)))
                            .next(IsTextNode()
                                      .text("Line1Line2Line3Line4Line5")
                                      .range({1, 1})
                                      .pathOp(IsFillOp(IsColorPaint(Color::GREEN)))
                                      .next(IsTextNode()
                                                .text("Line1Line2Line3Line4Line5")
                                                .range({2, 4})
                                                .pathOp(IsFillOp(IsColorPaint(Color::RED))))))))));

    // ========== Run off until all playback is done ===========
    for (int i = 0; i < 2000; i += 100)
        advanceTime(100);

    // The player has finished
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    // The scroll view is fully scrolled
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 100, 60}, "...ScrollView")
                .vertical()
                .dirty(sg::Layer::kFlagChildOffsetChanged)
                .childOffset(Point(0, 40))
                .child(IsLayer(Rect{0, 0, 100, 100}, "...Text")
                           .dirty(sg::Layer::kFlagRedrawContent)
                           .content(IsTransformNode().child(
                               IsTextNode()
                                   .text("Line1Line2Line3Line4Line5")
                                   .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))))));
}

TEST_F(AudioHighlightTest, ScrollingWithPreserve)
{
    config->set(RootProperty::kScrollCommandDuration, 50);

    factory->addFakeContent({{"http://foo.com",
                              1000, // Overall length 1000 ms
                              200,  // 200 ms delay at start
                              -1,
                              {{kSpeechMarkWord, 10, 15, 100, "Line1"},
                               {kSpeechMarkWord, 30, 35, 300, "Line2"},
                               {kSpeechMarkWord, 50, 55, 500, "Line3"},
                               {kSpeechMarkWord, 70, 75, 700, "Line4"},
                               {kSpeechMarkWord, 90, 95, 900, "Line5"}}}});

    loadDocument(SCROLLING);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(sg);

    // At the start we have five lines of text. The first three are visible
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 100, 60}, "...ScrollView")
                .vertical()
                .child(IsLayer(Rect{0, 0, 100, 100}, "...Text")
                           .content(IsTransformNode().child(
                               IsTextNode()
                                   .text("Line1Line2Line3Line4Line5")
                                   .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))))));

    // Execute SpeakItem with line highlighting.  Align the line to "first"
    executeCommand("SpeakItem",
                   {{"componentId", "TEXT"},
                    {"sequencer", "MAGIC"},
                    {"highlightMode", "line"},
                    {"align", "first"}},
                   false);

    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    root->clearPending(); // There's a zero-duration scroll command that needs to be cleared

    // Switched to karaoke state, but no highlighted line
    sg = root->getSceneGraph();
    ASSERT_TRUE(
        CheckSceneGraph(sg, IsLayer(Rect{0, 0, 100, 60}, "...ScrollView")
                                .vertical()
                                .child(IsLayer(Rect{0, 0, 100, 100}, "...Text")
                                           .dirty(sg::Layer::kFlagRedrawContent)
                                           .content(IsTransformNode().child(
                                               IsTextNode()
                                                   .text("Line1Line2Line3Line4Line5")
                                                   .pathOp(IsFillOp(IsColorPaint(Color::RED))))))));

    ASSERT_FALSE(factory->hasEvent());

    // ========== Advance time past the initial delay ===========
    advanceTime(200);

    // The player posts Ready and Play
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kPlay));
    ASSERT_FALSE(factory->hasEvent());

    // The scene graph has not changed - no Karaoke yet
    sg = root->getSceneGraph();
    ASSERT_TRUE(
        CheckSceneGraph(sg, IsLayer(Rect{0, 0, 100, 60}, "...ScrollView")
                                .vertical()
                                .child(IsLayer(Rect{0, 0, 100, 100}, "...Text")
                                           .content(IsTransformNode().child(
                                               IsTextNode()
                                                   .text("Line1Line2Line3Line4Line5")
                                                   .pathOp(IsFillOp(IsColorPaint(Color::RED))))))));

    // ========== The first karaoke word hits ===========
    advanceTime(100);

    // The first line turns GREEN
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 100, 60}, "...ScrollView")
                .vertical()
                .child(IsLayer(Rect{0, 0, 100, 100}, "...Text")
                           .dirty(sg::Layer::kFlagRedrawContent)
                           .content(IsTransformNode().child(
                               IsTextNode()
                                   .text("Line1Line2Line3Line4Line5")
                                   .range({0, 0})
                                   .pathOp(IsFillOp(IsColorPaint(Color::GREEN)))
                                   .next(IsTextNode()
                                             .text("Line1Line2Line3Line4Line5")
                                             .range({1, 4})
                                             .pathOp(IsFillOp(IsColorPaint(Color::RED))))

                                   )))));

    // ========== The second karaoke word hits.  Starts scrolling Line2 ===========
    advanceTime(200);

    // The second line turns GREEN
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 100, 60}, "...ScrollView")
            .vertical()
            .child(
                IsLayer(Rect{0, 0, 100, 100}, "...Text")
                    .dirty(sg::Layer::kFlagRedrawContent)
                    .content(IsTransformNode().child(
                        IsTextNode()
                            .text("Line1Line2Line3Line4Line5")
                            .range({0, 0})
                            .pathOp(IsFillOp(IsColorPaint(Color::RED)))
                            .next(IsTextNode()
                                      .text("Line1Line2Line3Line4Line5")
                                      .range({1, 1})
                                      .pathOp(IsFillOp(IsColorPaint(Color::GREEN)))
                                      .next(IsTextNode()
                                                .text("Line1Line2Line3Line4Line5")
                                                .range({2, 4})
                                                .pathOp(IsFillOp(IsColorPaint(Color::RED))))))))));

    // ========== Advance past the initial scrolling but before the next word ===========
    advanceTime(100);

    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 100, 60}, "...ScrollView")
            .vertical()
            .dirty(sg::Layer::kFlagChildOffsetChanged)
            .childOffset(Point(0, 20))
            .child(
                IsLayer(Rect{0, 0, 100, 100}, "...Text")
                    .content(IsTransformNode().child(
                        IsTextNode()
                            .text("Line1Line2Line3Line4Line5")
                            .range({0, 0})
                            .pathOp(IsFillOp(IsColorPaint(Color::RED)))
                            .next(IsTextNode()
                                      .text("Line1Line2Line3Line4Line5")
                                      .range({1, 1})
                                      .pathOp(IsFillOp(IsColorPaint(Color::GREEN)))
                                      .next(IsTextNode()
                                                .text("Line1Line2Line3Line4Line5")
                                                .range({2, 4})
                                                .pathOp(IsFillOp(IsColorPaint(Color::RED))))))))));

    auto playerTimer = factory->getPlayers().at(0).lock()->getTimeoutId();
    loop->freeze(playerTimer);

    configChange(ConfigurationChange(1000, 1000));
    processReinflate();

    loop->rehydrate(playerTimer);

    // ========== Run off until all playback is done ===========
    for (int i = 0; i < 2000; i += 100)
        advanceTime(100);

    // The player has finished
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("http://foo.com", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());

    // The scroll view is fully scrolled
    sg = root->getSceneGraph();
    ASSERT_TRUE(CheckSceneGraph(
        sg, IsLayer(Rect{0, 0, 100, 60}, "...ScrollView")
                .vertical()
                .childOffset(Point(0, 40))
                .child(IsLayer(Rect{0, 0, 100, 100}, "...Text")
                           .content(IsTransformNode().child(
                               IsTextNode()
                                   .text("Line1Line2Line3Line4Line5")
                                   .pathOp(IsFillOp(IsColorPaint(Color::BLUE))))))));
}

static const char *SPEECH_MARK_HANDLER = R"apl({
  "type": "APL",
  "version": "2022.2",
  "theme": "dark",
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "width": 400,
        "height": 400,
        "id": "root",
        "speech": "URL1",
        "onSpeechMark": {
          "type": "SendEvent",
          "sequencer": "SPEAK",
          "arguments": [
            "TEST",
            "${event.source.source}",
            "${event.source.handler}",
            "${event.source.id}",
            "${event.source.value}",
            "${event.markType}",
            "${event.markTime}",
            "${event.markValue}"
          ]
        }
      }
    ]
  }
}
)apl";

// Just check it stil works with SG.
TEST_F(AudioHighlightTest, SpeechMarkHandler)
{
    // Limited subset of marks to avoid too much verification
    factory->addFakeContent({
        {"URL1", 2500, 100, -1,
         {
             {SpeechMarkType::kSpeechMarkWord, 0, 5, 500, "uno"},
             {SpeechMarkType::kSpeechMarkSSML, 42, 46, 1000, "dos"},
             {SpeechMarkType::kSpeechMarkWord, 42, 46, 1250, "tres"},
             {SpeechMarkType::kSpeechMarkSentence, 64, 70, 1500, "I am a sentence"},
             {SpeechMarkType::kSpeechMarkViseme, 90, 97, 2000, "V"}
         }
        }
    });

    loadDocument(SPEECH_MARK_HANDLER);

    executeSpeakItem("root", kCommandScrollAlignFirst, apl::kCommandHighlightModeLine, 1000);
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kPreroll));
    ASSERT_FALSE(factory->hasEvent());

    advanceTime(100);
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kReady));
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kPlay));

    advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "TEST", "Container", "SpeechMark", "root", Object::NULL_OBJECT(), "word", 500, "uno"));

    advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "TEST", "Container", "SpeechMark", "root", Object::NULL_OBJECT(), "ssml", 1000, "dos"));

    advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "TEST", "Container", "SpeechMark", "root", Object::NULL_OBJECT(), "word", 1250, "tres"));
    ASSERT_TRUE(CheckSendEvent(root, "TEST", "Container", "SpeechMark", "root", Object::NULL_OBJECT(), "sentence", 1500, "I am a sentence"));

    advanceTime(500);
    ASSERT_TRUE(CheckSendEvent(root, "TEST", "Container", "SpeechMark", "root", Object::NULL_OBJECT(), "viseme", 2000, "V"));

    advanceTime(500);
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kDone));
    ASSERT_TRUE(CheckPlayer("URL1", TestAudioPlayer::kRelease));
    ASSERT_FALSE(factory->hasEvent());
}