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

using namespace apl;

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
        r.setEnvironmentValue("testEnvironment", "23.2");
        c = Context::createTestContext(m,r);
    }

    void TearDown() override
    {
        c = nullptr;
        MemoryWrapper::TearDown();
    }

    ContextPtr c;
};

TEST_F(ContextTest, Basic)
{
    auto env = c->opt("environment");
    EXPECT_EQ("UnitTests", env.get("agentName").asString());
    EXPECT_EQ("1.0", env.get("agentVersion").asString());
    EXPECT_EQ("normal", env.get("animation").asString());
    EXPECT_FALSE(env.get("allowOpenURL").asBoolean());
    EXPECT_EQ("1.8", env.get("aplVersion").asString());
    EXPECT_FALSE(env.get("disallowVideo").asBoolean());
    EXPECT_EQ("23.2", env.get("testEnvironment").asString());
    EXPECT_EQ(1.0, env.get("fontScale").asNumber());
    EXPECT_EQ("normal", env.get("screenMode").asString());
    EXPECT_EQ("", env.get("lang").asString());
    EXPECT_EQ("LTR", env.get("layoutDirection").asString());
    EXPECT_EQ(false, env.get("screenReader").asBoolean());

    auto timing = env.get("timing");
    EXPECT_EQ(500, timing.get("doublePressTimeout").asNumber());
    EXPECT_EQ(1000, timing.get("longPressTimeout").asNumber());
    EXPECT_EQ(50, timing.get("minimumFlingVelocity").asNumber());
    EXPECT_EQ(64, timing.get("pressedDuration").asNumber());
    EXPECT_EQ(100, timing.get("tapOrScrollTimeout").asNumber());

    auto viewport = c->opt("viewport");
    EXPECT_EQ(2048, viewport.get("pixelWidth").asNumber());
    EXPECT_EQ(1024, viewport.get("width").asNumber());
    EXPECT_EQ(2048, viewport.get("pixelHeight").asNumber());
    EXPECT_EQ(1024, viewport.get("height").asNumber());
    EXPECT_EQ(320, viewport.get("dpi").asNumber());
    EXPECT_EQ("round", viewport.get("shape").asString());
    EXPECT_EQ("green", viewport.get("theme").asString());
    EXPECT_EQ(Object("tv"), viewport.get("mode"));

    EXPECT_TRUE(c->opt("Math").get("asin").isFunction());

    EXPECT_EQ(256, c->vhToDp(25));
    EXPECT_EQ(128, c->vwToDp(12.5));
    EXPECT_EQ(50, c->pxToDp(100));

    EXPECT_EQ(APLVersion(APLVersion::kAPLVersionIgnore), c->getRootConfig().getEnforcedAPLVersion());

    auto buildVersion = env.get("_coreRepositoryVersion").asString();
    ASSERT_FALSE(buildVersion.empty());
    ASSERT_NE("unknown", buildVersion.c_str());
}

TEST_F(ContextTest, AlternativeConfig)
{
    auto root = RootConfig().agent("MyTest", "0.2")
        .disallowVideo(true)
        .reportedAPLVersion("1.2")
        .allowOpenUrl(true)
        .animationQuality(RootConfig::kAnimationQualitySlow)
        .setEnvironmentValue("testEnvironment", 122)
        .fontScale(2.0)
        .screenMode(RootConfig::kScreenModeHighContrast)
        .screenReader(true)
        .doublePressTimeout(2000)
        .set(RootProperty::kLang, "en-US")
        .set(RootProperty::kLayoutDirection, "RTL")
        .longPressTimeout(2100)
        .minimumFlingVelocity(565)
        .pressedDuration(999)
        .tapOrScrollTimeout(777);

    c = Context::createTestContext(Metrics().size(400,400), root);

    auto env = c->opt("environment");
    EXPECT_EQ("MyTest", env.get("agentName").asString());
    EXPECT_EQ("0.2", env.get("agentVersion").asString());
    EXPECT_EQ("slow", env.get("animation").asString());
    EXPECT_TRUE(env.get("allowOpenURL").asBoolean());
    EXPECT_EQ("1.2", env.get("aplVersion").asString());
    EXPECT_TRUE(env.get("disallowVideo").asBoolean());
    EXPECT_EQ(122, env.get("testEnvironment").asNumber());
    EXPECT_EQ(2.0, env.get("fontScale").asNumber());
    EXPECT_EQ("high-contrast", env.get("screenMode").asString());
    EXPECT_EQ(true, env.get("screenReader").asBoolean());

    auto timing = env.get("timing");
    EXPECT_EQ(2000, timing.get("doublePressTimeout").asNumber());
    EXPECT_EQ(2100, timing.get("longPressTimeout").asNumber());
    EXPECT_EQ(565, timing.get("minimumFlingVelocity").asNumber());
    EXPECT_EQ(999, timing.get("pressedDuration").asNumber());
    EXPECT_EQ(777, timing.get("tapOrScrollTimeout").asNumber());

    auto buildVersion = env.get("_coreRepositoryVersion").asString();
    ASSERT_FALSE(buildVersion.empty());
}

TEST_F(ContextTest, Child)
{
    auto c2 = Context::createFromParent(c);
    auto c3 = Context::createFromParent(c2);

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
        c = Context::createTestContext(Metrics().shape(m.first), session);
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
        c = Context::createTestContext(Metrics().mode(m.first), session);
        ASSERT_EQ(Object(m.second), c->opt("viewport").get("mode")) << m.second;
    }
}

static const char * TIME_DOC =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.1\","
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Text\","
    "      \"text\": \"${localTime}\""
    "    }"
    "  }"
    "}";


TEST_F(ContextTest, Time)
{
    // Thu Sep 05 2019 15:39:17  (UTCTime)
    const unsigned long long utcTime = 1567697957924;
    const long long deltaTime = 3600 * 1000;

    auto rootConfig = RootConfig().utcTime(utcTime).localTimeAdjustment(deltaTime);
    ASSERT_EQ(utcTime, rootConfig.getUTCTime());
    ASSERT_EQ(deltaTime, rootConfig.getLocalTimeAdjustment());

    auto content = Content::create(TIME_DOC);
    auto root = RootContext::create(Metrics(), content, rootConfig);
    auto component = root->topComponent();

    ASSERT_EQ(utcTime + deltaTime, root->context().opt("localTime").asNumber());
    ASSERT_EQ(utcTime, root->context().opt("utcTime").asNumber());

    ASSERT_TRUE(IsEqual(std::to_string(utcTime + deltaTime), component->getCalculated(kPropertyText).asString()));

    // Change the local time zone
    const long long deltaNew = -10 * 3600 * 1000;
    root->setLocalTimeAdjustment(deltaNew);
    root->updateTime(100);
    ASSERT_TRUE(CheckDirty(component, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component));

    ASSERT_EQ(utcTime + 100, root->context().opt("utcTime").asNumber());
    ASSERT_EQ(utcTime + deltaNew + 100, root->context().opt("localTime").asNumber());
    ASSERT_TRUE(IsEqual(std::to_string(utcTime + deltaNew + 100), component->getCalculated(kPropertyText).asString()));

    // Demonstrate how to set the root config to reflect the current time in local time.
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
    rootConfig = RootConfig().utcTime(now.count());

    ASSERT_EQ(std::chrono::milliseconds{static_cast<long long>(rootConfig.getUTCTime())}, now);

    component = nullptr;
    root = nullptr;
}

static const char * DEFAULT_ENV_DOC = R"apl(
{
   "type": "APL",
   "version": "1.7",
   "mainTemplate": {
     "item": {
       "type": "Text",
       "text": "Document Lang: ${environment.lang} LayoutDirection: ${environment.layoutDirection}"
     }
   }
 }
)apl";

TEST_F(ContextTest, DefaultEnv)
{
    auto rootConfig = RootConfig();
    auto content = Content::create(DEFAULT_ENV_DOC);
    auto root = RootContext::create(Metrics(), content, rootConfig);
    auto component = root->topComponent();

    ASSERT_EQ("Document Lang:  LayoutDirection: LTR", component->getCalculated(kPropertyText).asString());
}


static const char * BASIC_ENV_DOC = R"apl(
{
   "type": "APL",
   "version": "1.7",
   "lang": "en-US",
   "layoutDirection": "RTL",
   "mainTemplate": {
     "item": {
       "type": "Text",
       "lang": "es-US",
       "height": 110,
       "text": "Document Lang: ${environment.lang} LayoutDirection: ${environment.layoutDirection}"
     }
   }
 }
)apl";

TEST_F(ContextTest, LangAndLayoutDirectionCheck)
{
    auto rootConfig = RootConfig();
    auto content = Content::create(BASIC_ENV_DOC);
    auto root = RootContext::create(Metrics(), content, rootConfig);
    auto component = root->topComponent();

    ASSERT_EQ("Document Lang: en-US LayoutDirection: RTL", component->getCalculated(kPropertyText).asString());
}

/*
 * Verify standard functions are included for type-evaluation contexts, but not for
 * the background evaluation context.
 */
TEST_F(ContextTest, NoStandardFunction)
{
    auto rootConfig = RootConfig();
    auto metrics = Metrics();
    auto session = makeDefaultSession();

    auto ctx1 = Context::createTypeEvaluationContext(rootConfig);
    auto ctx2 = Context::createBackgroundEvaluationContext(metrics, rootConfig, metrics.getTheme());

    ASSERT_FALSE(ctx1->opt("Array").empty());
    ASSERT_FALSE(ctx1->opt("Math").empty());
    ASSERT_FALSE(ctx1->opt("String").empty());
    ASSERT_FALSE(ctx1->opt("Time").empty());

    ASSERT_TRUE(ctx2->opt("Array").empty());
    ASSERT_TRUE(ctx2->opt("Math").empty());
    ASSERT_TRUE(ctx2->opt("String").empty());
    ASSERT_TRUE(ctx2->opt("Time").empty());
}

TEST_F(ContextTest, TrivialMethodChecks)
{
    auto rootConfig = RootConfig().set(RootProperty::kLang, "de-DE");
    auto content = Content::create(BASIC_ENV_DOC);
    auto root = RootContext::create(Metrics().theme("dark"), content, rootConfig);

    ASSERT_EQ(std::string("de-DE"), root->getRootConfig().getProperty(RootProperty::kLang).asString());
    ASSERT_EQ(std::string("dark"), root->getTheme());
    ASSERT_EQ(rootConfig.getMeasure(), root->measure());
    ASSERT_EQ(rootConfig.getTimeManager()->nextTimeout(), root->nextTime());
    ASSERT_EQ(0, root->getFocusableAreas().size());
    ASSERT_TRUE(root->nextFocus(kFocusDirectionForward, Rect(0, 0, 0, 0)));
    ASSERT_FALSE(root->setFocus(kFocusDirectionForward, Rect(0, 0, 0, 0), "TargetDoesNotExist"));
}


static const char * OVERRIDE_ENV_DOC = R"apl(
{
   "type": "APL",
   "version": "1.7",
   "lang": "en-US",
   "layoutDirection": "RTL",
   "environment": {
     "lang": "fi-FI",
     "layoutDirection": "LTR"
   },
   "mainTemplate": {
     "item": {
       "type": "Text",
       "text": "Document Lang: ${environment.lang} LayoutDirection: ${environment.layoutDirection}"
     }
   }
 }
)apl";


TEST_F(ContextTest, OverrideCheck)
{
    auto rootConfig = RootConfig();
    auto content = Content::create(OVERRIDE_ENV_DOC);
    auto root = RootContext::create(Metrics(), content, rootConfig);
    auto component = root->topComponent();

    ASSERT_EQ("Document Lang: fi-FI LayoutDirection: LTR", component->getCalculated(kPropertyText).asString());
}


// The built-in environment values are "" and "LTR" for lang and layoutDirection
static const char * CANCEL_OVERRIDE_ENV_DOC = R"apl(
{
   "type": "APL",
   "version": "1.7",
   "lang": "en-US",
   "layoutDirection": "RTL",
   "environment": {
     "lang": "${environment.lang}",
     "layoutDirection": "${environment.layoutDirection}"
   },
   "mainTemplate": {
     "item": {
       "type": "Text",
       "text": "Document Lang: ${environment.lang} LayoutDirection: ${environment.layoutDirection}"
     }
   }
 }
)apl";

TEST_F(ContextTest, CancelOverrideCheck)
{
    auto rootConfig = RootConfig();
    auto content = Content::create(CANCEL_OVERRIDE_ENV_DOC);
    auto root = RootContext::create(Metrics(), content, rootConfig);
    auto component = root->topComponent();

    ASSERT_EQ("Document Lang:  LayoutDirection: LTR", component->getCalculated(kPropertyText).asString());
}


static const char *ENVIRONMENT_PAYLOAD = R"apl(
 {
   "type": "APL",
   "version": "1.7",
   "environment": {
     "parameters": "payload",
     "lang": "${payload.lang}",
     "layoutDirection": "${payload.layoutDirection}"
   },
   "mainTemplate": {
     "parameters": "payload",
     "item": {
       "type": "Text",
       "text": "Document Lang: ${environment.lang} LayoutDirection: ${environment.layoutDirection}"
     }
   }
 }
)apl";

TEST_F(ContextTest, EnvironmentPayload)
{
    auto rootConfig = RootConfig();
    auto content = Content::create(ENVIRONMENT_PAYLOAD); //, );
    content->addData("payload", R"({"lang": "en-ES", "layoutDirection": "RTL"})" );
    auto root = RootContext::create(Metrics(), content, rootConfig);
    auto component = root->topComponent();

    ASSERT_EQ("Document Lang: en-ES LayoutDirection: RTL", component->getCalculated(kPropertyText).asString());
}