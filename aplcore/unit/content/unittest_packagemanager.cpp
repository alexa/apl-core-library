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

#include "testpackagemanager.h"
#include "packagegenerator.h"

using namespace apl;

class PackageManagerTest : public DocumentWrapper {};

const char* B_IMPORT = R"apl({
    "type": "APL",
    "version": "1.0",
    "resources": [
      {
        "strings": {
          "B": "B"
        }
      }
    ]
})apl";

TEST_F(PackageManagerTest, RepeatedImportDifferentSources)
{
    const char* REPEATED_IMPORT_DIFFERENT_SOURCES = R"apl({
        "type": "APL",
        "version": "1.0",
        "import": [
          {
            "name": "B",
            "version": "1.0",
            "source": "custom.json"
          },
          {
            "name": "B",
            "version": "1.0",
            "source": "other.json"
          }
        ],
        "mainTemplate": {
          "item": {
            "type": "Text"
          }
        }
    })apl";

    auto testPackageManager = std::make_shared<TestPackageManager>();
    auto config = RootConfig().packageManager(testPackageManager);
    auto doc = Content::create(REPEATED_IMPORT_DIFFERENT_SOURCES, makeDefaultSession(), Metrics(), config);
    ASSERT_TRUE(doc);
    bool successCalled = false;
    doc->load([&]() { successCalled = true;},
              []{});

    ASSERT_FALSE(successCalled);
    ASSERT_EQ(1, testPackageManager->getUnresolvedRequests().size());
    auto request = testPackageManager->get("B:1.0");
    ASSERT_TRUE(request.isValid());
    ASSERT_EQ("custom.json", request.source());

    testPackageManager->succeed(request, SharedJsonData(B_IMPORT));
    ASSERT_TRUE(successCalled);
    auto expected = std::vector<std::string>{ "B:1.0" };
    ASSERT_EQ(expected, doc->getLoadedPackageNames());
}

TEST_F(PackageManagerTest, DeepLoop)
{
    std::map<std::string, std::string> packageMap = {
        { "A", makeTestPackage({"B", "C"}, {}) },
        { "B", makeTestPackage({"C", "D"}, {}) },
        { "C", makeTestPackage({"D"}, {}) },
        { "D", makeTestPackage({"A"}, {}) }
    };

    auto json = makeTestPackage({"A"}, {{"test", "value"}});
    auto testPackageManager = std::make_shared<TestPackageManager>();
    auto config = RootConfig().packageManager(testPackageManager);
    auto content = Content::create(json, session, Metrics(), config);
    ASSERT_TRUE(content);

    bool failureCalled = false;
    content->load([]() {},
                  [&]{ failureCalled = true; });

    ASSERT_FALSE(failureCalled);
    ASSERT_EQ(1, testPackageManager->getUnresolvedRequests().size());

    auto requestA = testPackageManager->get("A:1.0");
    testPackageManager->succeed(requestA, SharedJsonData(packageMap.find("A")->second));
    ASSERT_EQ(1, testPackageManager->getResolvedRequestCount());
    ASSERT_EQ(2, testPackageManager->getUnresolvedRequests().size());

    auto requestB = testPackageManager->get("B:1.0");
    auto requestC = testPackageManager->get("C:1.0");
    testPackageManager->succeed(requestB, SharedJsonData(packageMap.find("B")->second));
    ASSERT_EQ(2, testPackageManager->getResolvedRequestCount());
    ASSERT_EQ(2, testPackageManager->getUnresolvedRequests().size());

    auto requestD = testPackageManager->get("D:1.0");
    testPackageManager->succeed(requestC, SharedJsonData(packageMap.find("C")->second));
    ASSERT_EQ(3, testPackageManager->getResolvedRequestCount());
    ASSERT_EQ(1, testPackageManager->getUnresolvedRequests().size());

    testPackageManager->succeed(requestD, SharedJsonData(packageMap.find("D")->second));
    ASSERT_EQ(4, testPackageManager->getResolvedRequestCount());
    ASSERT_EQ(0, testPackageManager->getUnresolvedRequests().size());
    ASSERT_TRUE(failureCalled);
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(PackageManagerTest, Loop)
{
    auto testPackageManager = std::make_shared<TestPackageManager>();
    auto json = makeTestPackage({"A", "B"}, {{"test", "value"}});
    auto pkg_a = makeTestPackage({"B"}, {{"testA", "A"}});
    auto pkg_b = makeTestPackage({"A"}, {{"testB", "B"}});
    testPackageManager->putPackage("A:1.0", pkg_a);
    testPackageManager->putPackage("B:1.0", pkg_b);

    auto config = RootConfig().packageManager(testPackageManager);
    auto content = Content::create(json, session, Metrics(), config);
    ASSERT_TRUE(content);

    bool failureCalled = false;
    content->load([](){},
              [&]{ failureCalled = true; });

    ASSERT_TRUE(failureCalled);
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(PackageManagerTest, NonReversal)
{
    auto testPackageManager = std::make_shared<TestPackageManager>();
    auto json = makeTestPackage({"A", "B"}, {{"test", "value"}});
    auto pkg_a = makeTestPackage({}, {{"testA", "A"}, {"testB", "A"}});
    auto pkg_b = makeTestPackage({"A"}, {{"testB", "B"}});
    testPackageManager->putPackage("A:1.0", pkg_a);
    testPackageManager->putPackage("B:1.0", pkg_b);

    auto config = RootConfig().packageManager(testPackageManager);
    auto content = Content::create(json, makeDefaultSession(), Metrics(), config);
    ASSERT_TRUE(content);

    bool successCalled = false;
    content->load([&]() { successCalled = true; },
                  []{});
    ASSERT_TRUE(successCalled);
    ASSERT_TRUE(content->getPackage("A:1.0"));
    ASSERT_TRUE(content->getPackage("B:1.0"));

    auto doc = RootContext::create(Metrics(), content);
    ASSERT_TRUE(doc);
    auto context = doc->contextPtr();
    ASSERT_TRUE(context);

    ASSERT_EQ(3, doc->info().resources().size());
    ASSERT_EQ(Object("value"), context->opt("@test"));
    ASSERT_EQ(Object("A"), context->opt("@testA"));
    ASSERT_EQ(Object("B"), context->opt("@testB"));  // B depends on A, so B overrides A
}

TEST_F(PackageManagerTest, Reversal)
{
    auto testPackageManager = std::make_shared<TestPackageManager>();
    auto json = makeTestPackage({"A", "B"}, {{"test", "value"}});
    auto pkg_a = makeTestPackage({"B"}, {{"testA", "A"}, {"testB", "A"}});
    auto pkg_b = makeTestPackage({}, {{"testB", "B"}});
    testPackageManager->putPackage("A:1.0", pkg_a);
    testPackageManager->putPackage("B:1.0", pkg_b);

    auto config = RootConfig().packageManager(testPackageManager);
    auto content = Content::create(json, makeDefaultSession(), Metrics(), config);
    ASSERT_TRUE(content);

    bool successCalled = false;
    content->load([&]() { successCalled = true; },
                  []{});
    ASSERT_TRUE(successCalled);
    ASSERT_TRUE(content->getPackage("A:1.0"));
    ASSERT_TRUE(content->getPackage("B:1.0"));

    auto doc = RootContext::create(Metrics(), content);
    ASSERT_TRUE(doc);
    auto context = doc->contextPtr();
    ASSERT_TRUE(context);

    ASSERT_EQ(3, doc->info().resources().size());
    ASSERT_EQ(Object("value"), context->opt("@test"));
    ASSERT_EQ(Object("A"), context->opt("@testA"));
    ASSERT_EQ(Object("A"), context->opt("@testB"));  // A depends on B, so A overrides B
}

TEST_F(PackageManagerTest, Diamond)
{
    auto testPackageManager = std::make_shared<TestPackageManager>();
    auto json = makeTestPackage({"A", "B"}, {{"test", "value"}});
    auto pkg_a = makeTestPackage({"C"}, {{"testA", "A"}});
    auto pkg_b = makeTestPackage({"C"}, {{"testB", "B"}});
    auto pkg_c = makeTestPackage({}, {{"testC", "C"}, {"testA", "C"}, {"testB", "C"}});
    testPackageManager->putPackage("A:1.0", pkg_a);
    testPackageManager->putPackage("B:1.0", pkg_b);
    testPackageManager->putPackage("C:1.0", pkg_c);

    auto config = RootConfig().packageManager(testPackageManager);
    auto content = Content::create(json, makeDefaultSession(), Metrics(), config);
    ASSERT_TRUE(content);

    bool successCalled = false;
    content->load([&]() { successCalled = true; },
                  []{});
    ASSERT_TRUE(successCalled);
    ASSERT_TRUE(content->getPackage("A:1.0"));
    ASSERT_TRUE(content->getPackage("B:1.0"));
    ASSERT_TRUE(content->getPackage("C:1.0"));
    
    auto doc = RootContext::create(Metrics(), content);
    ASSERT_TRUE(doc);
    auto context = doc->contextPtr();
    ASSERT_TRUE(context);

    ASSERT_EQ(4, doc->info().resources().size());
    ASSERT_EQ(Object("value"), context->opt("@test"));
    ASSERT_EQ(Object("A"), context->opt("@testA"));
    ASSERT_EQ(Object("B"), context->opt("@testB"));
    ASSERT_EQ(Object("C"), context->opt("@testC"));
}

TEST_F(PackageManagerTest, ChangeConfigAfterContentInitialization)
{
    const char *THEME_BASED_NESTED_INCLUDE = R"apl({
      "type": "APL",
      "version": "2023.3",
      "onConfigChange": {
        "type": "Reinflate"
      },
      "import": [
        {
          "name": "StyledFrame",
          "version": "1.0"
        }
      ],
      "mainTemplate": {
        "item": {
          "type": "StyledFrame"
        }
      }
    })apl";

    const char *STYLED_FRAME_OVERRIDE_DEPENDS = R"apl({
      "type": "APL",
      "version": "2023.3",
      "import": [
        {
          "name": "conditional",
          "version": "1.2",
          "when": "${environment.hasMagic == 'magic'}",
          "loadAfter": "dbasic"
        },
        {
          "name": "dbasic",
          "description": "force it to to be requested later",
          "version": "1.2"
        }
      ],
      "layouts": {
        "StyledFrame": {
          "item": {
            "type": "Frame",
            "width": "100%",
            "height": "100%",
            "backgroundColor": "@MyRed"
          }
        }
      }
    })apl";

    const char *BASIC = R"apl({
      "type": "APL",
      "version": "2023.3",
      "resources": [
        {
          "colors": {
            "MyRed": "#ff0101ff"
          }
        }
      ]
    })apl";

    const char *CONDITIONAL = R"apl({
      "type": "APL",
      "version": "2023.3",
      "resources": [
        {
          "colors": {
            "MyRed": "#ff0000ff"
          }
        }
      ]
    })apl";


    auto testPackageManager = std::make_shared<TestPackageManager>();
    testPackageManager->putPackage("StyledFrame:1.0", STYLED_FRAME_OVERRIDE_DEPENDS);
    testPackageManager->putPackage("dbasic:1.2", BASIC);
    testPackageManager->putPackage("conditional:1.2", CONDITIONAL);

    auto config = RootConfig::create();
    config->packageManager(testPackageManager);

    auto content = Content::create(THEME_BASED_NESTED_INCLUDE, session, Metrics(), *config);
    ASSERT_TRUE(content);

    int successCalled = 0;
    content->load([&]() { successCalled++; },
                  []{});

    ASSERT_EQ(1, successCalled);
    ASSERT_TRUE(content->isReady());

    // Config (or metrics/or both) changed when RootContext creation possible. Should still account
    // for it.
    config->setEnvironmentValue("hasMagic", "magic");
    content->refresh(Metrics(), *config);

    content->load([&]() { successCalled++; },
                  []{});

    ASSERT_EQ(2, successCalled);
    ASSERT_TRUE(content->isReady());

    auto expectedOrder = std::vector<std::string> {
        "dbasic:1.2",
        "conditional:1.2",
        "StyledFrame:1.0",
    };

    ASSERT_EQ(expectedOrder, content->getLoadedPackageNames());
}

TEST_F(PackageManagerTest, PackageFailure)
{
    auto testPackageManager = std::make_shared<TestPackageManager>();
    auto json = makeTestPackage({"A"}, {{"test", "value"}});
    auto pkg_a = makeTestPackage({"C"}, {{"testA", "A"}});
    testPackageManager->putPackage("A:1.0", pkg_a);

    auto config = RootConfig().packageManager(testPackageManager);
    auto content = Content::create(json, session, Metrics(), config);
    ASSERT_TRUE(content);

    bool failureCalled = false;
    content->load([]{},
                  [&]() { failureCalled = true; });

    testPackageManager->fail(testPackageManager->get("C:1.0"));
    ASSERT_TRUE(failureCalled);
    ASSERT_TRUE(ConsoleMessage());
}

TEST_F(PackageManagerTest, LoadedContentStillSucceeds)
{
    auto testPackageManager = std::make_shared<TestPackageManager>();
    auto json = makeTestPackage({"A"}, {{"test", "value"}});
    auto pkg_a = makeTestPackage({}, {});
    testPackageManager->putPackage("A:1.0", pkg_a);

    auto config = RootConfig().packageManager(testPackageManager);
    auto content = Content::create(json, makeDefaultSession(), Metrics(), config);
    ASSERT_TRUE(content);

    int successCount = 0;
    content->load([&]() { successCount++; },
                  []{});

    ASSERT_EQ(1, successCount);
    auto expected = std::vector<std::string>{ "A:1.0" };
    ASSERT_EQ(expected, content->getLoadedPackageNames());

    content->load([&]() { successCount++; },
                  []{});
    ASSERT_EQ(2, successCount);
    ASSERT_EQ(expected, content->getLoadedPackageNames());
}

TEST_F(PackageManagerTest, LoadedContentWhilePendingInvokesSecondLambda)
{
    auto testPackageManager = std::make_shared<TestPackageManager>();
    auto json = makeTestPackage({"A"}, {{"test", "value"}});
    auto pkg_a = makeTestPackage({}, {});

    auto config = RootConfig().packageManager(testPackageManager);
    auto content = Content::create(json, makeDefaultSession(), Metrics(), config);
    ASSERT_TRUE(content);

    bool successA = false;
    bool successB = false;
    auto lamdbaA = [&]() { successA = true; };
    auto lamdbaB= [&]() { successB = true; };
    // Two loads are triggered, so first one is canceled and second one will trigger.
    content->load(lamdbaA,
                  []{});

    content->load(lamdbaB,
                  []{});

    testPackageManager->succeed(testPackageManager->get("A:1.0"), SharedJsonData(pkg_a));
    ASSERT_FALSE(successA);
    ASSERT_TRUE(successB);
}

TEST_F(PackageManagerTest, BadPackage)
{
    auto badPackages = std::vector<std::string>{
        "<bad package>",
        R"apl({
         "version": "1.1"
        })apl",
        R"apl({
         "type": "APL"
        })apl",
        R"apl({
         "type": "APL",
         "version": "1.1",
         "import": "foo"
        })apl",
        R"apl({
         "type": "APL",
         "version": "1.1",
         "import": ["foo"]
        })apl",
    };

    for (const auto& badPackage: badPackages) {
        auto testPackageManager = std::make_shared<TestPackageManager>();
        auto json = makeTestPackage({"A"}, {{"test", "value"}});
        testPackageManager->putPackage("A:1.0", badPackage);

        auto config = RootConfig().packageManager(testPackageManager);
        auto content = Content::create(json, session, Metrics(), config);
        ASSERT_TRUE(content);

        bool failureCalled = false;
        content->load([] {}, [&]() { failureCalled = true; });

        ASSERT_TRUE(failureCalled);
        ASSERT_TRUE(content->isError());
        ASSERT_TRUE(ConsoleMessage());
    }
}

TEST_F(PackageManagerTest, ContentAddsWrongPackage)
{
    auto json = makeTestPackage({"A"}, {{"test", "value"}});

    auto content = Content::create(json, session, Metrics(), RootConfig());
    ASSERT_TRUE(content);

    content->addPackage(ImportRequest("B", "1.0", "", {}, nullptr, nullptr), makeTestPackage({},{}));

    ASSERT_TRUE(LogMessage());
}

TEST_F(PackageManagerTest, CanceledContent)
{
    auto testPackageManager = std::make_shared<TestPackageManager>();
    auto json = makeTestPackage({"A"}, {{"test", "value"}});
    auto pkg_a = makeTestPackage({"B"}, {});
    auto pkg_b = makeTestPackage({}, {});

    auto config = RootConfig().packageManager(testPackageManager);
    auto content = Content::create(json, makeDefaultSession(), Metrics(), config);
    ASSERT_TRUE(content);

    bool successA = false;
    bool successB = false;
    auto lamdbaA = [&]() { successA = true; };
    auto lamdbaB= [&]() { successB = true; };

    content->load(lamdbaA,
                  []{});

    testPackageManager->succeed(testPackageManager->get("A:1.0"), SharedJsonData(pkg_a));

    content->load(lamdbaB,
                  []{});

    // Only second lambda runs
    testPackageManager->succeed(testPackageManager->get("A:1.0"), SharedJsonData(pkg_a));
    testPackageManager->succeed(testPackageManager->get("B:1.0"), SharedJsonData(pkg_b));
    ASSERT_FALSE(successA);
    ASSERT_TRUE(successB);
}

static const char *IMPORT_PACKAGE_DOC = R"(
{
  "type": "APL",
  "version": "2024.1",
  "onMount": [],
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "mainContainer",
      "items": [
        {
          "type": "TouchWrapper",
          "width": "100%",
          "onPress": [
            {
              "type": "ImportPackage",
              "name": "levelone",
              "version": "1.0",
              "source": "levelonesource"
            }
          ],
          "items": [
            {
              "type": "Text",
              "text": "ImportPackage test template"
            }
          ]
        },
        {
          "type": "TouchWrapper",
          "width": "100%",
          "onPress": [
            {
              "delay": 1000,
              "type": "InsertItem",
              "componentId": "mainContainer",
              "item": {
                "type": "Text",
                "text": "@leveloneString"
              }
            },
            {
              "type": "InsertItem",
              "componentId": "mainContainer",
              "item": {
                "type": "Text",
                "text": "@leveltwoString"
              }
            },
            {
              "type": "InsertItem",
              "componentId": "mainContainer",
              "item": {
                "type": "Text",
                "text": "@leveltwoStringAgain"
              }
            },
            {
              "type": "InsertItem",
              "componentId": "mainContainer",
              "item": {
                "type": "Text",
                "text": "@levelthreeString"
              }
            }
          ],
          "items": [
            {
              "type": "Text",
              "text": "insertItem runner"
            }
          ]
        }
      ]
    }
  }
}
)";

static const char *LEVEL_ONE_PACKAGE_JSON = R"(
{
  "type": "APL",
  "version": "2024.1",
  "import": [
    {
      "name": "leveltwo",
      "version": "1.0"
    }
  ],
  "resources": [
    {
      "string": {
        "leveloneString": "parent package loaded"
      }
    }
  ]
}
)";

static const char *LEVEL_TWO_PACKAGE_JSON = R"(
{
  "type": "APL",
  "version": "2024.1",
  "resources": [
    {
      "string": {
        "leveltwoString": "child package loaded"
      }
    }
  ]
}
)";

TEST_F(PackageManagerTest, SingleOneLevelNestedPackageImport) {
    auto testPackageManager = std::make_shared<TestPackageManager>();
    testPackageManager->putPackage("levelone:1.0", LEVEL_ONE_PACKAGE_JSON);
    testPackageManager->putPackage("leveltwo:1.0", LEVEL_TWO_PACKAGE_JSON);

    config->packageManager(testPackageManager);
    createContent(IMPORT_PACKAGE_DOC, "{}", true);
    content->load([]{}, []{});
    inflate();
    ASSERT_TRUE(root);
    rootDocument = root->topDocument();

    // Send the ImportPackage command
    performClick(0, 0);
    loop->advanceToEnd();

    // Trigger InsertItem - using content from dynamically loaded package
    performClick(0, 10);
    loop->advanceToEnd();

    auto top = root->topComponent();
    ASSERT_EQ("parent package loaded", top->getChildAt(2)->getCalculated(apl::kPropertyText).asString());
    ASSERT_EQ("child package loaded", top->getChildAt(3)->getCalculated(apl::kPropertyText).asString());
}

static const char *LEVEL_ONE_DUAL_IMPORT_PACKAGE_JSON = R"(
{
  "type": "APL",
  "version": "2024.1",
  "import": [
    {
      "name": "leveltwo",
      "version": "1.0"
    },
    {
      "name": "leveltwoagain",
      "version": "1.0"
    }
  ],
  "resources": [
    {
      "string": {
        "leveloneString": "parent package loaded"
      }
    }
  ]
}
)";

static const char *LEVEL_TWO_AGAIN_PACKAGE_JSON = R"(
{
  "type": "APL",
  "version": "2024.1",
  "resources": [
    {
      "string": {
        "leveltwoStringAgain": "child package loaded, again!"
      }
    }
  ]
}
)";


TEST_F(PackageManagerTest, DualOneLevelNestedPackageImport) {
    auto testPackageManager = std::make_shared<TestPackageManager>();
    testPackageManager->putPackage("levelone:1.0", LEVEL_ONE_DUAL_IMPORT_PACKAGE_JSON);
    testPackageManager->putPackage("leveltwo:1.0", LEVEL_TWO_PACKAGE_JSON);
    testPackageManager->putPackage("leveltwoagain:1.0", LEVEL_TWO_AGAIN_PACKAGE_JSON);

    config->packageManager(testPackageManager);

    createContent(IMPORT_PACKAGE_DOC, "{}", true);
    content->load([]{}, []{});
    inflate();
    ASSERT_TRUE(root);
    rootDocument = root->topDocument();

    // Send the ImportPackage command
    performClick(0, 0);
    loop->advanceToEnd();

    // Trigger InsertItem - using content from dynamically loaded package
    performClick(0, 10);
    loop->advanceToEnd();

    auto top = root->topComponent();
    ASSERT_EQ("parent package loaded", top->getChildAt(2)->getCalculated(apl::kPropertyText).asString());
    ASSERT_EQ("child package loaded", top->getChildAt(3)->getCalculated(apl::kPropertyText).asString());
    ASSERT_EQ("child package loaded, again!", top->getChildAt(4)->getCalculated(apl::kPropertyText).asString());
}


static const char *LEVEL_TWO_TARGETING_THREE_PACKAGE_JSON = R"(
{
  "type": "APL",
  "version": "2024.1",
  "import": [
    {
      "name": "levelthree",
      "version": "1.0"
    }
  ],
  "resources": [
    {
      "string": {
        "leveltwoString": "child package loaded"
      }
    }
  ]
}
)";

static const char *LEVEL_THREE_PACKAGE_JSON = R"(
{
  "type": "APL",
  "version": "2024.1",
  "resources": [
    {
      "string": {
        "levelthreeString": "strings all the way down"
      }
    }
  ]
}
)";


TEST_F(PackageManagerTest, TwoLevelNestedPackageImport) {
    auto testPackageManager = std::make_shared<TestPackageManager>();
    testPackageManager->putPackage("levelone:1.0", LEVEL_ONE_PACKAGE_JSON);
    testPackageManager->putPackage("leveltwo:1.0", LEVEL_TWO_TARGETING_THREE_PACKAGE_JSON);
    testPackageManager->putPackage("levelthree:1.0", LEVEL_THREE_PACKAGE_JSON);
    config->packageManager(testPackageManager);

    createContent(IMPORT_PACKAGE_DOC, "{}", true);
    content->load([]{}, []{});
    inflate();
    ASSERT_TRUE(root);
    rootDocument = root->topDocument();
    ASSERT_TRUE(component);

    // Send the ImportPackage command
    performClick(0, 0);
    loop->advanceToEnd();

    // Trigger InsertItem - using content from dynamically loaded package
    performClick(0, 10);
    loop->advanceToEnd();

    auto top = root->topComponent();
    ASSERT_EQ("parent package loaded", top->getChildAt(2)->getCalculated(apl::kPropertyText).asString());
    ASSERT_EQ("child package loaded", top->getChildAt(3)->getCalculated(apl::kPropertyText).asString());
    ASSERT_EQ("strings all the way down", top->getChildAt(5)->getCalculated(apl::kPropertyText).asString());
}


static const char *DYNAMIC_IMPORT_DOC = R"(
{
  "type": "APL",
  "version": "2024.1",
  "onMount": [
    {
      "type": "ImportPackage",
      "name": "levelone",
      "version": "1.0",
      "source": "levelonesource"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "mainContainer",
      "items": [
        {
          "type": "TouchWrapper",
          "width": "100%",
          "onPress": [
            {
              "type": "ImportPackage",
              "name": "levelone",
              "version": "1.0",
              "source": "levelonepossiblynewsource"
            }
          ],
          "items": [
            {
              "type": "Text",
              "text": "ImportPackage test template"
            }
          ]
        }
      ]
    }
  }
}
)";

static const char *STATIC_IMPORT_DOC = R"(
{
  "type": "APL",
  "version": "2024.1",
  "import": [
    {
      "name": "levelone",
      "version": "1.0"
    }
  ],
  "onMount": [],
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "mainContainer",
      "items": [
        {
          "type": "TouchWrapper",
          "width": "100%",
          "onPress": [
            {
              "type": "ImportPackage",
              "name": "levelone",
              "version": "1.0",
              "source": "levelonesource"
            }
          ],
          "items": [
            {
              "type": "Text",
              "text": "duplicate package import protection test"
            }
          ]
        }
      ]
    }
  }
}
)";

static const char *BASIC_PACKAGE_JSON = R"(
{
  "type": "APL",
  "version": "2024.1",
  "resources": [
    {
      "string": {
        "basic": "basic"
      }
    }
  ]
}
)";


TEST_F(PackageManagerTest, RequestPackageAlreadyStaticallyImported) {
    auto testPackageManager = std::make_shared<TestPackageManager>();
    testPackageManager->putPackage("levelone:1.0", BASIC_PACKAGE_JSON);
    config->packageManager(testPackageManager);

    createContent(STATIC_IMPORT_DOC, "{}", true);
    content->load([]{}, []{});
    inflate();
    ASSERT_TRUE(root);
    rootDocument = root->topDocument();

    // Send the ImportPackage command
    performClick(0, 0);
    loop->advanceToEnd();
}

TEST_F(PackageManagerTest, RequestPackageAlreadyDynamicallyImported) {
    auto testPackageManager = std::make_shared<TestPackageManager>();
    testPackageManager->putPackage("levelone:1.0", BASIC_PACKAGE_JSON);
    config->packageManager(testPackageManager);

    createContent(DYNAMIC_IMPORT_DOC, "{}", true);
    content->load([]{}, []{});
    inflate();
    ASSERT_TRUE(root);
    rootDocument = root->topDocument();

    // Send the ImportPackage command
    performClick(0, 0);
    loop->advanceToEnd();

    // Send the ImportPackage command
    performClick(0, 0);
    loop->advanceToEnd();
}

static const char *CONSECUTIVE_IMPORTS_DOC = R"(
{
  "type": "APL",
  "version": "2024.1",
  "onMount": [],
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "mainContainer",
      "items": [
        {
          "type": "TouchWrapper",
          "width": "100%",
          "onPress": [
            {
              "type": "ImportPackage",
              "name": "firstPackage",
              "version": "1.0",
              "source": "sourceone"
            },
            {
              "type": "ImportPackage",
              "name": "secondPackage",
              "version": "1.0",
              "source": "sourcetwo"
            },
            {
              "type": "ImportPackage",
              "name": "thirdPackage",
              "version": "1.0",
              "source": "sourcethree"
            }
          ],
          "items": [
            {
              "type": "Text",
              "text": "consecutive package import test"
            }
          ]
        },
        {
          "type": "TouchWrapper",
          "width": "100%",
          "onPress": [
            {
              "type": "InsertItem",
              "componentId": "mainContainer",
              "item": {
                "type": "Text",
                "text": "${@first}"
              }
            },
            {
              "type": "InsertItem",
              "componentId": "mainContainer",
              "item": {
                "type": "Text",
                "text": "@second"
              }
            },
            {
              "type": "InsertItem",
              "componentId": "mainContainer",
              "item": {
                "type": "Text",
                "text": "@third"
              }
            }
          ],
          "items": [
            {
              "type": "Text",
              "text": "InsertItem touchwrapper"
            }
          ]
        }
      ]
    }
  }
}
)";


static const char *FIRST_PACKAGE_JSON = R"(
{
  "type": "APL",
  "version": "2024.1",
  "resources": [ { "string": { "first": "first" } } ]
}
)";

static const char *SECOND_PACKAGE_JSON = R"(
{
  "type": "APL",
  "version": "2024.1",
  "resources": [ { "string": { "second": "second" } } ]
}
)";

static const char *THIRD_PACKAGE_JSON = R"(
{
  "type": "APL",
  "version": "2024.1",
  "resources": [ { "string": { "third": "third" } } ]
}
)";

TEST_F(PackageManagerTest, ConsecutiveDynamicImports) {
    auto testPackageManager = std::make_shared<TestPackageManager>();
    testPackageManager->putPackage("firstPackage:1.0", FIRST_PACKAGE_JSON);
    testPackageManager->putPackage("secondPackage:1.0", SECOND_PACKAGE_JSON);
    testPackageManager->putPackage("thirdPackage:1.0", THIRD_PACKAGE_JSON);

    config->packageManager(testPackageManager);

    createContent(CONSECUTIVE_IMPORTS_DOC, "{}", true);
    content->load([]{}, []{});
    inflate();
    ASSERT_TRUE(root);
    rootDocument = root->topDocument();

    // Trigger ImportPackages
    performClick(0, 0);
    loop->advanceToEnd();

    // Trigger InsertItems
    performClick(0, 10);
    loop->advanceToEnd();

    auto top = root->topComponent();
    ASSERT_EQ("first", top->getChildAt(2)->getCalculated(apl::kPropertyText).asString());
    ASSERT_EQ("second", top->getChildAt(3)->getCalculated(apl::kPropertyText).asString());
    ASSERT_EQ("third", top->getChildAt(4)->getCalculated(apl::kPropertyText).asString());
}