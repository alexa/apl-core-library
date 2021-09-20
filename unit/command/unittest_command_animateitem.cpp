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

// #include <algorithm>

#include "../testeventloop.h"
#include "apl/time/sequencer.h"

using namespace apl;


class AnimateItemTest : public CommandTest {};

static const char *ANIMATE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Frame\","
    "          \"id\": \"box\","
    "          \"width\": 100,"
    "          \"height\": 100"
    "        },"
    "        {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"go\","
    "          \"onPress\": {"
    "            \"type\": \"AnimateItem\","
    "            \"componentId\": \"box\","
    "            \"duration\": 1000,"
    "            \"value\": {"
    "              \"property\": \"opacity\","
    "              \"from\": 0.5,"
    "              \"to\": 0"
    "            }"
    "          }"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(AnimateItemTest, Basic)
{
    loadDocument(ANIMATE);
    advanceTime(10);
    auto frame = root->context().findComponentById("box");
    auto goButton = root->context().findComponentById("go");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(goButton);

    ASSERT_EQ(Object(1), frame->getCalculated(kPropertyOpacity));
    ASSERT_TRUE(CheckDirty(root));

    performClick(1, 100);
    root->clearPending();

    ASSERT_EQ(Object(0.5), frame->getCalculated(kPropertyOpacity));

    for (int i = 1; i <= 10; i++) {
        advanceTime(100);
        ASSERT_NEAR(0.5 * (1 - i * 0.1), frame->getCalculated(kPropertyOpacity).asNumber(), 0.00001);
        ASSERT_TRUE(CheckDirty(frame, kPropertyOpacity, kPropertyVisualHash));
    }
    ASSERT_TRUE(CheckDirty(root, component, frame));

    ASSERT_EQ(0, loop->size());
    ASSERT_TRUE(CheckDirty(root));
}

TEST_F(AnimateItemTest, AnimateNone)
{
    config->animationQuality(RootConfig::AnimationQuality::kAnimationQualityNone);
    loadDocument(ANIMATE);
    auto frame = root->context().findComponentById("box");
    auto goButton = root->context().findComponentById("go");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(goButton);

    ASSERT_EQ(Object(1.0), frame->getCalculated(kPropertyOpacity));
    ASSERT_TRUE(CheckDirty(root));

    performClick(1, 100);
    root->clearPending();

    // Should go straight to end state.
    ASSERT_EQ(Object(0.0), frame->getCalculated(kPropertyOpacity));
    ASSERT_FALSE(CheckDirty(root));
}

static const char *ANIMATE_WITH_DELAY =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Frame\","
    "          \"id\": \"box\","
    "          \"width\": 100,"
    "          \"height\": 100"
    "        },"
    "        {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"go\","
    "          \"onPress\": {"
    "            \"type\": \"AnimateItem\","
    "            \"delay\": 1000,"
    "            \"componentId\": \"box\","
    "            \"duration\": 1000,"
    "            \"value\": {"
    "              \"property\": \"opacity\","
    "              \"from\": 0.5,"
    "              \"to\": 0"
    "            }"
    "          }"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(AnimateItemTest, BasicDelay)
{
    loadDocument(ANIMATE_WITH_DELAY);
    auto frame = root->context().findComponentById("box");
    auto goButton = root->context().findComponentById("go");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(goButton);

    ASSERT_EQ(Object(1), frame->getCalculated(kPropertyOpacity));

    performClick(1, 100);
    root->clearPending();

    ASSERT_EQ(Object(1.0), frame->getCalculated(kPropertyOpacity));
    ASSERT_TRUE(CheckDirty(frame));

    // Advance past the delay
    loop->advanceToTime(1000);
    ASSERT_EQ(Object(0.5), frame->getCalculated(kPropertyOpacity));
    ASSERT_TRUE(CheckDirty(frame, kPropertyOpacity, kPropertyVisualHash));

    auto startTime = loop->currentTime();
    for (int i = 1; i <= 10; i++) {
        loop->advanceToTime(startTime + i * 100);
        ASSERT_NEAR(0.5 * (1 - i * 0.1), frame->getCalculated(kPropertyOpacity).asNumber(), 0.00001);
        ASSERT_TRUE(CheckDirty(frame, kPropertyOpacity, kPropertyVisualHash));
    }

    ASSERT_EQ(0, loop->size());
}

static const char *ANIMATE_IMPLICIT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Frame\","
    "          \"id\": \"box\","
    "          \"width\": 100,"
    "          \"height\": 100"
    "        },"
    "        {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"go\","
    "          \"onPress\": {"
    "            \"type\": \"AnimateItem\","
    "            \"componentId\": \"box\","
    "            \"duration\": 1000,"
    "            \"value\": {"
    "              \"property\": \"opacity\","
    "              \"to\": 0"
    "            }"
    "          }"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

// Leave out the "From" property
TEST_F(AnimateItemTest, ImplicitOpacity)
{
    loadDocument(ANIMATE_IMPLICIT);
    auto frame = root->context().findComponentById("box");
    auto goButton = root->context().findComponentById("go");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(goButton);

    ASSERT_EQ(Object(1), frame->getCalculated(kPropertyOpacity));

    performClick(1, 100);
    root->clearPending();

    ASSERT_EQ(Object(1.0), frame->getCalculated(kPropertyOpacity));
    ASSERT_TRUE(CheckDirty(frame));  // The opacity didn't change even though the animation started

    auto startTime = loop->currentTime();
    for (int i = 1; i <= 10; i++) {
        loop->advanceToTime(startTime + i * 100);
        ASSERT_NEAR(1 - i * 0.1, frame->getCalculated(kPropertyOpacity).asNumber(), 0.00001);
        ASSERT_TRUE(CheckDirty(frame, kPropertyOpacity, kPropertyVisualHash));
    }

    ASSERT_EQ(0, loop->size());
    ASSERT_TRUE(CheckDirty(frame));
}

static const char *ANIMATE_REPEAT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Frame\","
    "          \"id\": \"box\","
    "          \"width\": 100,"
    "          \"height\": 100"
    "        },"
    "        {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"go\","
    "          \"onPress\": {"
    "            \"type\": \"AnimateItem\","
    "            \"componentId\": \"box\","
    "            \"duration\": 1000,"
    "            \"repeatCount\": 2,"
    "            \"value\": {"
    "              \"property\": \"opacity\","
    "              \"to\": 0"
    "            }"
    "          }"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

// Repeat twice
TEST_F(AnimateItemTest, Repeat)
{
    loadDocument(ANIMATE_REPEAT);
    auto frame = root->context().findComponentById("box");
    auto goButton = root->context().findComponentById("go");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(goButton);

    ASSERT_EQ(Object(1), frame->getCalculated(kPropertyOpacity));

    performClick(1, 100);
    root->clearPending();

    ASSERT_EQ(Object(1.0), frame->getCalculated(kPropertyOpacity));
    ASSERT_TRUE(CheckDirty(frame));   // No opacity change yet

    for (int j = 0; j < 3; j++) {
        auto startTime = loop->currentTime();
        for (int i = 1; i <= 10; i++) {
            loop->advanceToTime(startTime + i * 100);
            float expectedOpacity = 1 - i * 0.1;
            if (i == 10 && j < 2)  // In these cases we've wrapped around and started again
                expectedOpacity = 1.0;
            ASSERT_NEAR(expectedOpacity, frame->getCalculated(kPropertyOpacity).asNumber(), 0.00001)
                                << " i=" << i << " j=" << j;
            ASSERT_TRUE(CheckDirty(frame, kPropertyOpacity, kPropertyVisualHash));
        }
    }

    ASSERT_EQ(0, loop->size());
    ASSERT_TRUE(CheckDirty(frame));
}

static const char *ANIMATE_REPEAT_REVERSE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Frame\","
    "          \"id\": \"box\","
    "          \"width\": 100,"
    "          \"height\": 100"
    "        },"
    "        {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"go\","
    "          \"onPress\": {"
    "            \"type\": \"AnimateItem\","
    "            \"componentId\": \"box\","
    "            \"duration\": 1000,"
    "            \"repeatCount\": 2,"
    "            \"repeatMode\": \"reverse\","
    "            \"value\": {"
    "              \"property\": \"opacity\","
    "              \"to\": 0"
    "            }"
    "          }"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

// Repeat twice with a reverse.
TEST_F(AnimateItemTest, RepeatReverse)
{
    loadDocument(ANIMATE_REPEAT_REVERSE);
    auto frame = root->context().findComponentById("box");
    auto goButton = root->context().findComponentById("go");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(goButton);

    ASSERT_EQ(Object(1), frame->getCalculated(kPropertyOpacity));

    performClick(1, 100);
    root->clearPending();

    ASSERT_EQ(Object(1.0), frame->getCalculated(kPropertyOpacity));
    ASSERT_TRUE(CheckDirty(frame));   // No opacity change yet

    float expectedOpacity = 1.0;
    for (int j = 0; j < 3; j++) {
        auto startTime = loop->currentTime();
        for (int i = 1; i <= 10; i++) {
            loop->advanceToTime(startTime + i * 100);
            if (j % 2 == 0)
                expectedOpacity -= 0.1;
            else
                expectedOpacity += 0.1;

            ASSERT_NEAR(expectedOpacity, frame->getCalculated(kPropertyOpacity).asNumber(), 0.00001)
                                << " i=" << i << " j=" << j << " time=" << loop->currentTime();
            ASSERT_TRUE(CheckDirty(frame, kPropertyOpacity, kPropertyVisualHash));
        }
    }

    ASSERT_EQ(0, loop->size());
    ASSERT_TRUE(CheckDirty(frame));
}

static const char * ANIMATE_REPEAT_REVERSE_EASING =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Frame\","
    "          \"id\": \"box\","
    "          \"width\": 100,"
    "          \"height\": 100"
    "        },"
    "        {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"go\","
    "          \"onPress\": {"
    "            \"type\": \"AnimateItem\","
    "            \"componentId\": \"box\","
    "            \"duration\": 1000,"
    "            \"repeatCount\": 3,"
    "            \"repeatMode\": \"reverse\","
    "            \"easing\": \"path(0.5,1)\","
    "            \"value\": {"
    "              \"property\": \"opacity\","
    "              \"to\": 0"
    "            }"
    "          }"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

// Repeat twice with a reverse and an odd easing curve.
TEST_F(AnimateItemTest, RepeatReverseEasing)
{
    loadDocument(ANIMATE_REPEAT_REVERSE_EASING);
    auto frame = root->context().findComponentById("box");
    auto goButton = root->context().findComponentById("go");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(goButton);

    ASSERT_EQ(Object(1), frame->getCalculated(kPropertyOpacity));

    performClick(1, 100);
    root->clearPending();

    ASSERT_EQ(Object(1.0), frame->getCalculated(kPropertyOpacity));
    ASSERT_TRUE(CheckDirty(frame));   // No opacity change yet

    float expectedOpacity = 1.0;
    float lastOpacity = 1.0;
    for (int j = 0; j < 4; j++) {
        auto startTime = loop->currentTime();
        for (int i = 1; i <= 10; i++) {
            loop->advanceToTime(startTime + i * 100);
            if (j % 2 == 0 && i <= 5)
                expectedOpacity -= 0.2;
            else if (j % 2 == 1 && i >= 6)
                expectedOpacity += 0.2;

            ASSERT_NEAR(expectedOpacity, frame->getCalculated(kPropertyOpacity).asNumber(), 0.00001)
                                << " i=" << i << " j=" << j << " time=" << loop->currentTime();

            if (expectedOpacity != lastOpacity)
                ASSERT_TRUE(CheckDirty(frame, kPropertyOpacity, kPropertyVisualHash));
            else
                ASSERT_TRUE(CheckDirty(frame));
            lastOpacity = expectedOpacity;
        }
    }

    ASSERT_EQ(0, loop->size());
    ASSERT_TRUE(CheckDirty(frame));
}

static const char * ANIMATE_REPEAT_NO_DURATION =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Frame\","
    "          \"id\": \"box\","
    "          \"width\": 100,"
    "          \"height\": 100"
    "        },"
    "        {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"go\","
    "          \"onPress\": {"
    "            \"type\": \"AnimateItem\","
    "            \"componentId\": \"box\","
    "            \"duration\": 0,"
    "            \"repeatCount\": 2,"
    "            \"repeatMode\": \"reverse\","   // Reversed with repeatCount=2 -> final position is end
    "            \"value\": {"
    "              \"property\": \"opacity\","
    "              \"from\": 0.25,"
    "              \"to\": 0.75"
    "            }"
    "          }"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(AnimateItemTest, NoDuration)
{
    loadDocument(ANIMATE_REPEAT_NO_DURATION);
    auto frame = root->context().findComponentById("box");
    auto goButton = root->context().findComponentById("go");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(goButton);

    ASSERT_EQ(Object(1), frame->getCalculated(kPropertyOpacity));
    ASSERT_TRUE(CheckDirty(frame));  // Nothing dirty so far

    performClick(1, 100);
    root->clearPending();

    ASSERT_EQ(Object(0.75), frame->getCalculated(kPropertyOpacity));
    ASSERT_EQ(0, loop->size());
    ASSERT_TRUE(CheckDirty(frame, kPropertyOpacity, kPropertyVisualHash));  // Should have been set exactly once
}


static const char * ANIMATE_REPEAT_NO_DURATION_REVERSED =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Frame\","
    "          \"id\": \"box\","
    "          \"width\": 100,"
    "          \"height\": 100"
    "        },"
    "        {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"go\","
    "          \"onPress\": {"
    "            \"type\": \"AnimateItem\","
    "            \"componentId\": \"box\","
    "            \"duration\": 0,"
    "            \"repeatCount\": 3,"
    "            \"repeatMode\": \"reverse\","   // Reversed with repeatCount=3 -> final position is start
    "            \"value\": {"
    "              \"property\": \"opacity\","
    "              \"from\": 0.25,"
    "              \"to\": 0.75"
    "            }"
    "          }"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(AnimateItemTest, NoDurationReversed)
{
    loadDocument(ANIMATE_REPEAT_NO_DURATION_REVERSED);
    auto frame = root->context().findComponentById("box");
    auto goButton = root->context().findComponentById("go");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(goButton);

    ASSERT_EQ(Object(1), frame->getCalculated(kPropertyOpacity));

    performClick(1, 100);
    root->clearPending();

    ASSERT_EQ(Object(0.25), frame->getCalculated(kPropertyOpacity));
    ASSERT_EQ(0, loop->size());
    ASSERT_TRUE(CheckDirty(frame, kPropertyOpacity, kPropertyVisualHash));
}


static const char * ANIMATE_OPACITY_AND_TRANSFORM =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Frame\","
    "          \"id\": \"box\","
    "          \"width\": 100,"
    "          \"height\": 100"
    "        },"
    "        {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"go\","
    "          \"onPress\": {"
    "            \"type\": \"AnimateItem\","
    "            \"componentId\": \"box\","
    "            \"duration\": 1000,"
    "            \"repeatCount\": 3,"
    "            \"value\": ["
    "              {"
    "                \"property\": \"opacity\","
    "                \"from\": 0,"
    "                \"to\": 1"
    "              },"
    "              {"
    "                \"property\": \"transform\","
    "                \"from\": {"
    "                  \"translateX\": \"100vw\""
    "                },"
    "                \"to\": {"
    "                  \"translateX\": 0"
    "                }"
    "              }"
    "            ]"
    "          }"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(AnimateItemTest, OpacityAndTransform)
{
    loadDocument(ANIMATE_OPACITY_AND_TRANSFORM);
    auto frame = root->context().findComponentById("box");
    auto goButton = root->context().findComponentById("go");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(goButton);

    ASSERT_EQ(Object(1), frame->getCalculated(kPropertyOpacity));
    ASSERT_EQ(Object::IDENTITY_2D(), frame->getCalculated(kPropertyTransform));

    performClick(1, 100);
    root->clearPending();

    ASSERT_EQ(Object(0), frame->getCalculated(kPropertyOpacity));
    ASSERT_EQ(Transform2D::translateX(metrics.getWidth()), frame->getCalculated(kPropertyTransform).getTransform2D());
    ASSERT_TRUE(CheckDirty(frame, kPropertyOpacity, kPropertyTransform, kPropertyVisualHash));

    for (int repeat = 0 ; repeat <= 3 ; repeat++) {
        auto startTime = loop->currentTime();
        for (int i = 100 ; i <= 1000 ; i += 100) {
            loop->advanceToTime(startTime + i);
            float expectedOpacity = i * .001;
            float expectedX = metrics.getWidth() * (1000 - i) * .001;
            if (i == 1000 && repeat < 3) {
                expectedOpacity = 0;
                expectedX = metrics.getWidth();
            }

            ASSERT_EQ(Object(expectedOpacity), frame->getCalculated(kPropertyOpacity));
            ASSERT_TRUE(IsEqual(Transform2D::translateX(expectedX),
                        frame->getCalculated(kPropertyTransform).getTransform2D()));
            ASSERT_TRUE(CheckDirty(frame, kPropertyOpacity, kPropertyTransform, kPropertyVisualHash));
        }
    }

    ASSERT_EQ(0, loop->size());
    ASSERT_TRUE(CheckDirty(frame));
}

// Terminate in the middle of the test
TEST_F(AnimateItemTest, OpacityAndTransformTerminate)
{
    loadDocument(ANIMATE_OPACITY_AND_TRANSFORM);
    auto frame = root->context().findComponentById("box");
    auto goButton = root->context().findComponentById("go");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(goButton);

    ASSERT_EQ(Object(1), frame->getCalculated(kPropertyOpacity));
    ASSERT_EQ(Object::IDENTITY_2D(), frame->getCalculated(kPropertyTransform));

    performClick(1, 100);
    root->clearPending();

    ASSERT_EQ(Object(0), frame->getCalculated(kPropertyOpacity));
    ASSERT_EQ(Transform2D::translateX(metrics.getWidth()), frame->getCalculated(kPropertyTransform).getTransform2D());
    ASSERT_TRUE(CheckDirty(frame, kPropertyOpacity, kPropertyTransform, kPropertyVisualHash));

    auto startTime = loop->currentTime();
    for (int i = 100; i <= 700; i += 100) {
        loop->advanceToTime(startTime + i);
        float expectedOpacity = i * .001;
        float expectedX = metrics.getWidth() * (1000 - i) * .001;
        ASSERT_EQ(Object(expectedOpacity), frame->getCalculated(kPropertyOpacity));
        ASSERT_TRUE(IsEqual(Transform2D::translateX(expectedX),
                            frame->getCalculated(kPropertyTransform).getTransform2D()));
        ASSERT_TRUE(CheckDirty(frame, kPropertyOpacity, kPropertyTransform, kPropertyVisualHash));
    }

    // Cancel execution. This should clear the animation AND set everything to the end value.
    root->cancelExecution();
    ASSERT_EQ(0, loop->size());

    ASSERT_EQ(Object(1), frame->getCalculated(kPropertyOpacity));
    ASSERT_EQ(Object::IDENTITY_2D(), frame->getCalculated(kPropertyTransform));
    ASSERT_TRUE(CheckDirty(frame, kPropertyTransform,  kPropertyOpacity, kPropertyVisualHash));
}


static const char * OPACITY_AND_RICH_TRANSFORM =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Frame\","
    "          \"id\": \"box\","
    "          \"width\": 100,"
    "          \"height\": 100"
    "        },"
    "        {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"go\","
    "          \"onPress\": {"
    "            \"type\": \"AnimateItem\","
    "            \"componentId\": \"box\","
    "            \"duration\": 1000,"
    "            \"value\": ["
    "              {"
    "                \"property\": \"opacity\","
    "                \"from\": 0,"
    "                \"to\": 1"
    "              },"
    "              {"
    "                \"property\": \"transform\","
    "                \"from\": ["
    "                  {"
    "                    \"translateX\": \"100vw\""
    "                  },"
    "                  {"
    "                    \"scale\": 0.1"
    "                  },"
    "                  {"
    "                    \"rotate\": 90"
    "                  }"
    "                ],"
    "                \"to\": ["
    "                  {"
    "                    \"translateX\": 0"
    "                  },"
    "                  {"
    "                    \"scale\": 1"
    "                  },"
    "                  {"
    "                    \"rotate\": 0"
    "                  }"
    "                ]"
    "              }"
    "            ]"
    "          }"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(AnimateItemTest, OpacityAndRichTransform)
{
    loadDocument(OPACITY_AND_RICH_TRANSFORM);
    auto frame = root->context().findComponentById("box");
    auto goButton = root->context().findComponentById("go");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(goButton);

    ASSERT_EQ(Object(1), frame->getCalculated(kPropertyOpacity));
    ASSERT_EQ(Object::IDENTITY_2D(), frame->getCalculated(kPropertyTransform));

    performClick(1, 100);
    root->clearPending();

    auto startTime = loop->currentTime();
    for (int i = 0; i <= 1000; i += 100) {
        loop->advanceToTime(startTime + i);
        float expectedOpacity = i * .001;
        float expectedX = metrics.getWidth() * (1000 - i) * .001;
        float expectedScale = 0.1 + i * 0.001 * 0.9;
        float expectedAngle = 90 * (1000 - i) * 0.001;
        Transform2D expectedTransform =
            Transform2D::translate(50, 50) *
            Transform2D::translateX(expectedX) *
            Transform2D::scale(expectedScale) *
            Transform2D::rotate(expectedAngle) *
            Transform2D::translate(-50, -50);

        ASSERT_EQ(Object(expectedOpacity), frame->getCalculated(kPropertyOpacity));
        ASSERT_TRUE(IsEqual(expectedTransform, frame->getCalculated(kPropertyTransform).getTransform2D()))
            << " time=" << i << " tx=" << expectedX << " scale=" << expectedScale << " rotate=" << expectedAngle;
        ASSERT_TRUE(CheckDirty(frame, kPropertyOpacity, kPropertyTransform, kPropertyVisualHash));
    }

    ASSERT_EQ(0, loop->size());
    ASSERT_TRUE(CheckDirty(frame));
}

static const char *RESOURCE_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"resources\": ["
    "    {"
    "      \"dimensions\": {"
    "        \"SLIDE_DIST\": 200"
    "      },"
    "      \"numbers\": {"
    "        \"OPACITY_START\": 0.2"
    "      }"
    "    }"
    "  ],"
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Frame\","
    "          \"id\": \"box\","
    "          \"width\": 100,"
    "          \"height\": 100"
    "        },"
    "        {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"go\","
    "          \"onPress\": {"
    "            \"type\": \"AnimateItem\","
    "            \"componentId\": \"box\","
    "            \"duration\": 1000,"
    "            \"value\": ["
    "              {"
    "                \"property\": \"opacity\","
    "                \"from\": \"${@OPACITY_START + 0.3}\","
    "                \"to\": 1"
    "              },"
    "              {"
    "                \"property\": \"transform\","
    "                \"from\": {"
    "                  \"translateX\": \"@SLIDE_DIST\""
    "                },"
    "                \"to\": {"
    "                  \"translateX\": 0"
    "                }"
    "              }"
    "            ]"
    "          }"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(AnimateItemTest, ResourceTest)
{
    loadDocument(RESOURCE_TEST);
    auto frame = root->context().findComponentById("box");
    auto goButton = root->context().findComponentById("go");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(goButton);

    ASSERT_EQ(Object(1), frame->getCalculated(kPropertyOpacity));
    ASSERT_EQ(Object::IDENTITY_2D(), frame->getCalculated(kPropertyTransform));

    performClick(1, 100);
    root->clearPending();

    auto startTime = loop->currentTime();
    for (int i = 0; i <= 1000; i += 100) {
        loop->advanceToTime(startTime + i);
        float expectedOpacity = 0.5 + 0.5 * i * .001;
        float expectedX = 200 * (1000 - i) * .001;
        Transform2D expectedTransform = Transform2D::translateX(expectedX);

        ASSERT_NEAR(expectedOpacity, frame->getCalculated(kPropertyOpacity).asNumber(), 0.0001);
        ASSERT_TRUE(IsEqual(expectedTransform, frame->getCalculated(kPropertyTransform).getTransform2D()));
        ASSERT_TRUE(CheckDirty(frame, kPropertyTransform, kPropertyOpacity, kPropertyVisualHash));
    }

    ASSERT_EQ(0, loop->size());
    ASSERT_TRUE(CheckDirty(frame));
}

static const char *MISSING_TRANSFORM_FROM =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Frame\","
    "          \"id\": \"box\","
    "          \"width\": 100,"
    "          \"height\": 100"
    "        },"
    "        {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"go\","
    "          \"onPress\": {"
    "            \"type\": \"AnimateItem\","
    "            \"componentId\": \"box\","
    "            \"duration\": 1000,"
    "            \"value\": ["
    "              {"
    "                \"property\": \"transform\","
    "                \"to\": {"
    "                  \"translateX\": 0"
    "                }"
    "              },"
    "              {"
    "                \"property\": \"opacity\","
    "                \"from\": 0.5"
    "              }"
    "            ]"
    "          }"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(AnimateItemTest, MissingTransformFrom)
{
    loadDocument(MISSING_TRANSFORM_FROM);
    auto frame = root->context().findComponentById("box");
    auto goButton = root->context().findComponentById("go");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(goButton);

    ASSERT_EQ(Object::IDENTITY_2D(), frame->getCalculated(kPropertyTransform));

    performClick(1, 100);
    root->clearPending();

    // Because the transform is missing a "from" and the opacity is missing a "to",
    // we don't have any properties to animate
    ASSERT_EQ(0, loop->size());
    ASSERT_TRUE(CheckDirty(frame));
    ASSERT_TRUE(ConsoleMessage());
}

static const char *MISSING_TRANSFORM_ROTATE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"Container\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Frame\","
    "          \"id\": \"box\","
    "          \"width\": 100,"
    "          \"height\": 100"
    "        },"
    "        {"
    "          \"type\": \"TouchWrapper\","
    "          \"id\": \"go\","
    "          \"onPress\": {"
    "            \"type\": \"AnimateItem\","
    "            \"componentId\": \"box\","
    "            \"duration\": 1000,"
    "            \"value\": ["
    "              {"
    "                \"property\": \"transform\","
    "                \"from\": ["
    "                  {"
    "                    \"translateX\": 100"
    "                  },"
    "                  {"
    "                    \"rotate\": 90"
    "                  }"
    "                ],"
    "                \"to\": ["
    "                  {"
    "                    \"translateX\": 0"
    "                  }"
    "                ]"
    "              }"
    "            ]"
    "          }"
    "        }"
    "      ]"
    "    }"
    "  }"
    "}";

TEST_F(AnimateItemTest, MissingTransformRotate)
{
    loadDocument(MISSING_TRANSFORM_ROTATE);
    auto frame = root->context().findComponentById("box");
    auto goButton = root->context().findComponentById("go");
    ASSERT_TRUE(frame);
    ASSERT_TRUE(goButton);

    ASSERT_EQ(Object(1), frame->getCalculated(kPropertyOpacity));
    ASSERT_EQ(Object::IDENTITY_2D(), frame->getCalculated(kPropertyTransform));

    performClick(1, 100);
    root->clearPending();
    ASSERT_TRUE(ConsoleMessage());  // We should get a warning about a missing rotate "to" value

    auto startTime = loop->currentTime();
    for (int i = 0; i <= 1000; i += 100) {
        loop->advanceToTime(startTime + i);
        float expectedX = 100 * (1000 - i) * .001;
        Transform2D expectedTransform = Transform2D::translateX(expectedX);

        // The Rotation transformation only showed up in the "from" list, so it is ignored
        ASSERT_TRUE(IsEqual(expectedTransform, frame->getCalculated(kPropertyTransform).getTransform2D()));
        ASSERT_TRUE(CheckDirty(frame, kPropertyTransform));
    }

    ASSERT_EQ(0, loop->size());
    ASSERT_TRUE(CheckDirty(frame));
}

static const char *SCROLL_TEST_WITH_ANIMATE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"item\": {"
    "      \"type\": \"ScrollView\","
    "      \"height\": \"100%\","
    "      \"width\": \"100%\","
    "      \"items\": ["
    "        {"
    "          \"type\": \"Frame\","
    "          \"id\": \"box\","
    "          \"width\": 100,"
    "          \"height\": 1000"
    "        }"
    "      ],"
    "      \"onScroll\": {"
    "        \"type\": \"AnimateItem\","
    "        \"componentId\": \"box\","
    "        \"duration\": 1000,"
    "        \"value\": ["
    "          {"
    "            \"property\": \"opacity\","
    "            \"from\": 0,"
    "            \"to\": \"${event.source.value * 5}\""
    "          }"
    "        ]"
    "      }"
    "    }"
    "  }"
    "}";

TEST_F(AnimateItemTest, ScrollTest)
{
    loadDocument(SCROLL_TEST_WITH_ANIMATE);
    auto frame = root->context().findComponentById("box");
    ASSERT_TRUE(frame);

    ASSERT_NEAR(1, frame->getCalculated(kPropertyOpacity).asNumber(), .0001);
    float lastOpacity = 1;

    // Execute the onScroll command.  This runs in fast mode, so we should jump to the final opacity
    for (int i = 10 ; i <= 200 ; i++) {
        component->update(kUpdateScrollPosition, i);
        ASSERT_EQ(1, loop->size());
        float expectedOpacity = i / metrics.getHeight() * 5;
        if (expectedOpacity > 1.0)
            expectedOpacity = 1.0;
        ASSERT_NEAR(expectedOpacity, frame->getCalculated(kPropertyOpacity).asNumber(), .0001);

        if (lastOpacity != expectedOpacity)
            ASSERT_TRUE(CheckDirty(frame, kPropertyOpacity, kPropertyVisualHash));
        else
            ASSERT_TRUE(CheckDirty(frame));
        lastOpacity = expectedOpacity;

        loop->advanceToEnd();
    }

    ASSERT_EQ(0, loop->size());
    ASSERT_TRUE(CheckDirty(frame));
}
