/**
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
//#include <iostream>
//#include <iomanip>
//
//#include "gtest/gtest.h"
//
//#include "apl/engine/evaluate.h"
//#include "apl/content/metrics.h"
//#include "apl/engine/context.h"
//#include "apl/content/rootconfig.h"

using namespace apl;

static Object o(const char *s){ return Object(s); }
static Object o(bool b){ return Object(b); }
static Object o(int i){ return Object(i); }
static Object o(double d){ return Object(d); }

class ContextTest : public MemoryWrapper {
protected:
    void SetUp() override
    {
        auto m = Metrics().size(2048,2048)
                      .dpi(320)
                      .theme("green")
                      .shape(apl::ROUND)
                      .mode(apl::kViewportModeTV);
        auto r = RootConfig().agent("UnitTests", "1.0");
        c = Context::create(m,r);
    }

    ContextPtr c;
};

TEST_F(ContextTest, Basic)
{
    EXPECT_EQ("UnitTests", c->opt("environment").get("agentName").asString());
    EXPECT_EQ("1.0", c->opt("environment").get("agentVersion").asString());
    EXPECT_EQ("normal", c->opt("environment").get("animation").asString());
    EXPECT_FALSE(c->opt("environment").get("allowOpenURL").asBoolean());
    EXPECT_EQ("1.2", c->opt("environment").get("aplVersion").asString());
    EXPECT_FALSE(c->opt("environment").get("disallowVideo").asBoolean());
    EXPECT_EQ(2048, c->opt("viewport").get("pixelWidth").asNumber());
    EXPECT_EQ(1024, c->opt("viewport").get("width").asNumber());
    EXPECT_EQ(2048, c->opt("viewport").get("pixelHeight").asNumber());
    EXPECT_EQ(1024, c->opt("viewport").get("height").asNumber());
    EXPECT_EQ(320, c->opt("viewport").get("dpi").asNumber());
    EXPECT_EQ("round", c->opt("viewport").get("shape").asString());
    EXPECT_EQ("green", c->opt("viewport").get("theme").asString());
    EXPECT_EQ(Object("tv"), c->opt("viewport").get("mode"));

    EXPECT_TRUE(c->opt("Math").get("asin").isFunction());

    EXPECT_EQ(256, c->vhToDp(25));
    EXPECT_EQ(128, c->vwToDp(12.5));
    EXPECT_EQ(50, c->pxToDp(100));

    EXPECT_EQ(APLVersion(APLVersion::kAPLVersionIgnore), c->getRootConfig().getEnforcedAPLVersion());
}

TEST_F(ContextTest, AlternativeConfig)
{
    auto root = RootConfig().agent("MyTest", "0.2")
        .disallowVideo(true)
        .reportedAPLVersion("1.2")
        .allowOpenUrl(true)
        .animationQuality(RootConfig::kAnimationQualitySlow);

    c = Context::create(Metrics().size(400,400), root);

    EXPECT_EQ("MyTest", c->opt("environment").get("agentName").asString());
    EXPECT_EQ("0.2", c->opt("environment").get("agentVersion").asString());
    EXPECT_EQ("slow", c->opt("environment").get("animation").asString());
    EXPECT_TRUE(c->opt("environment").get("allowOpenURL").asBoolean());
    EXPECT_EQ("1.2", c->opt("environment").get("aplVersion").asString());
    EXPECT_TRUE(c->opt("environment").get("disallowVideo").asBoolean());
}

TEST_F(ContextTest, Child)
{
    auto c2 = Context::create(c);
    auto c3 = Context::create(c2);

    c2->putConstant("name", "Fred");
    c2->putConstant("age", 23);

    c3->putConstant("name", "Jack");
    c3->putConstant("personality", "quixotic");

    EXPECT_EQ("Jack", c3->opt("name").asString());
    EXPECT_EQ(23, c3->opt("age").asNumber());
    EXPECT_EQ("quixotic", c3->opt("personality").asString());

    EXPECT_EQ("Fred", c2->opt("name").asString());
    EXPECT_EQ(23, c2->opt("age").asNumber());
    EXPECT_FALSE(c2->has("personality"));
}

TEST_F(ContextTest, Shape)
{
    for (auto m : std::map<ScreenShape , std::string>{
        { RECTANGLE, "rectangle"},
        { ROUND, "round"},
    }) {
        c = Context::create(Metrics().shape(m.first), session);
        ASSERT_EQ(Object(m.second), c->opt("viewport").get("shape")) << m.second;
    }
}

TEST_F(ContextTest, Mode)
{
    for (auto m : std::map<ViewportMode , std::string>{
        { kViewportModeAuto, "auto"},
        { kViewportModeHub, "hub"},
        { kViewportModeMobile, "mobile"},
        { kViewportModePC, "pc"},
        { kViewportModeTV, "tv"}
    }) {
        c = Context::create(Metrics().mode(m.first), session);
        ASSERT_EQ(Object(m.second), c->opt("viewport").get("mode")) << m.second;
    }
}

TEST_F(ContextTest, Time)
{
    // Thu Sep 05 2019 15:39:17  (LocalTime)
    auto root = RootConfig().localTime(1567697957924).localTimeAdjustment(3600 * 1000);
    ASSERT_EQ(1567697957924, root.getLocalTime());
    ASSERT_EQ(3600000, root.getLocalTimeAdjustment());

    // Demonstrate how to set the root config to reflect the current time in local time.
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
    root = RootConfig().localTime(now.count());

    ASSERT_EQ(std::chrono::milliseconds{root.getLocalTime()}, now);
}