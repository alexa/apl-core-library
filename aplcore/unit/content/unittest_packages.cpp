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

#include "../testeventloop.h"

#include "../embed/testdocumentmanager.h"
#include "testpackagemanager.h"

using namespace apl;

class PackagesTest : public DocumentWrapper {
public:
    PackagesTest() {}

    bool process(const ContentPtr& c) {
        if (!c->isWaiting()) return false;

        auto imports = c->getRequestedPackages();
        while (!imports.empty()) {
            for (auto& req : imports) {
                auto pack = mPackageStore.find(req.reference().toString() + ":" + req.source());
                if (pack != mPackageStore.end()) {
                    c->addPackage(req, pack->second);
                } else {
                    c->addPackage(req, "");
                }
            }

            imports = c->getRequestedPackages();
        }

        return true;
    }

    void add(const std::string& name, const std::string& source, const std::string& package) {
        mPackageStore.emplace(name + ":" + source, package);
    }

    void add(const std::string& name, const std::string& package) {
        add(name, "", package);
    }

    void reset() {
        mPackageStore.clear();
    }

private:
    std::map<std::string, std::string> mPackageStore;
};

static const char *MAIN = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "name": "basic",
      "version": "1.2"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "100%",
      "height": "100%",
      "backgroundColor": "@MyRed"
    }
  }
})apl";

static const char *BASIC = R"apl({
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

TEST_F(PackagesTest, BasicOld)
{
    content = Content::create(MAIN, session);
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
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0101ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

TEST_F(PackagesTest, BasicNew)
{
    add("basic:1.2", BASIC);
    content = Content::create(MAIN, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0101ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

static const char *THEME_BASED_INCLUDE = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "name": "basic",
      "version": "1.2",
      "when": "${environment.hasMagic != 'magic'}"
    },
    {
      "name": "conditional",
      "version": "1.2",
      "when": "${environment.hasMagic == 'magic'}"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "100%",
      "height": "100%",
      "backgroundColor": "@MyRed"
    }
  }
})apl";

static const char *CONDITIONAL = R"apl({
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

TEST_F(PackagesTest, ThemeConditionalNotSpecified)
{
    add("basic:1.2", BASIC);
    content = Content::create(THEME_BASED_INCLUDE, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0101ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

TEST_F(PackagesTest, ThemeConditionalSpecified)
{
    config->setEnvironmentValue("hasMagic", "magic");
    add("conditional:1.2", CONDITIONAL);
    content = Content::create(THEME_BASED_INCLUDE, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0000ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

static const char *STYLED_FRAME = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "name": "basic",
      "version": "1.2",
      "when": "${environment.hasMagic != 'magic'}"
    },
    {
      "name": "conditional",
      "version": "1.2",
      "when": "${environment.hasMagic == 'magic'}"
    }
  ],
  "layouts": {
    "StyledFrame": {
      "item": {
        "type": "Frame",
        "id": "magicFrame",
        "width": "100%",
        "height": "100%",
        "backgroundColor": "@MyRed"
      }
    }
  }
})apl";

static const char *THEME_BASED_NESTED_INCLUDE = R"apl({
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

TEST_F(PackagesTest, ThemeNestedConditionalNotSpecified)
{
    add("StyledFrame:1.0", STYLED_FRAME);
    add("basic:1.2", BASIC);
    content = Content::create(THEME_BASED_NESTED_INCLUDE, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0101ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

TEST_F(PackagesTest, ThemeNestedConditionalSpecified)
{
    config->setEnvironmentValue("hasMagic", "magic");
    add("StyledFrame:1.0", STYLED_FRAME);
    add("basic:1.2", BASIC);
    add("conditional:1.2", CONDITIONAL);
    content = Content::create(THEME_BASED_NESTED_INCLUDE, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0000ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

static const char *STYLED_FRAME_OVERRIDE = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "name": "basic",
      "version": "1.2"
    },
    {
      "name": "conditional",
      "version": "1.2",
      "when": "${environment.hasMagic == 'magic'}"
    }
  ],
  "layouts": {
    "StyledFrame": {
      "item": {
        "type": "Frame",
        "id": "magicFrame",
        "width": "100%",
        "height": "100%",
        "backgroundColor": "@MyRed"
      }
    }
  }
})apl";

TEST_F(PackagesTest, ThemeNestedConditionalOverride)
{
    config->setEnvironmentValue("hasMagic", "magic");
    add("StyledFrame:1.0", STYLED_FRAME_OVERRIDE);
    add("basic:1.2", BASIC);
    add("conditional:1.2", CONDITIONAL);

    content = Content::create(THEME_BASED_NESTED_INCLUDE, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0000ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

static const char *STYLED_FRAME_OVERRIDE_DEPENDS = R"apl({
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

TEST_F(PackagesTest, ThemeNestedConditionalOverrideDepends)
{
    config->setEnvironmentValue("hasMagic", "magic");
    add("StyledFrame:1.0", STYLED_FRAME_OVERRIDE_DEPENDS);
    add("dbasic:1.2", BASIC);
    add("conditional:1.2", CONDITIONAL);

    content = Content::create(THEME_BASED_NESTED_INCLUDE, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0000ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

static const char *EVALUATION_EVERYWHERE = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "when": "${environment.customPackageName}",
      "name": "${environment.customPackageName}",
      "version": "${environment.customPackageVersion}",
      "source": "${environment.customPackageLocation}",
      "loadAfter": "${environment.loadAfter}"
    },
    {
      "name": "dependency-package",
      "version": "1.0"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame"
    }
  }
})apl";

TEST_F(PackagesTest, EvaluationEverywhere)
{
    // Just needs to sort before default
    config->setEnvironmentValue("customPackageName", "bigNastyPackage");
    config->setEnvironmentValue("customPackageVersion", "1.2");
    config->setEnvironmentValue("customPackageLocation", "custom-location");
    config->setEnvironmentValue("loadAfter", "dependency-package");

    add("bigNastyPackage:1.2", "custom-location", CONDITIONAL);
    add("dependency-package:1.0", BASIC);

    content = Content::create(EVALUATION_EVERYWHERE, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());
}

static const char *METRICS_AND_VERSION_AVAILABLE = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "name": "${'styles-' + viewport.mode + '-' + viewport.theme}",
      "version": "${environment.documentAPLVersion}"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame"
    }
  }
})apl";

TEST_F(PackagesTest, MetricsAndVersionAvailable)
{
    metrics.mode(apl::kViewportModeMobile).theme("light");
    add("styles-mobile-light:2023.3", CONDITIONAL);
    content = Content::create(METRICS_AND_VERSION_AVAILABLE, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());
}

static const char *CIRCULAR_DEPENDS = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "name": "A",
      "version": "A",
      "loadAfter": "B"
    },
    {
      "name": "B",
      "version": "B",
      "loadAfter": "A"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame"
    }
  }
})apl";

TEST_F(PackagesTest, CircularDepends)
{
    add("A:A", CONDITIONAL);
    add("B:B", BASIC);
    content = Content::create(CIRCULAR_DEPENDS, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_FALSE(content->isReady());
    ASSERT_TRUE(content->isError());

    // Complains about circular dep.
    ASSERT_TRUE(session->checkAndClear("Failure to order packages"));
}

static const char *DEPENDS_ON_ITSELF = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "name": "A",
      "version": "A",
      "loadAfter": "A"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame"
    }
  }
})apl";

TEST_F(PackagesTest, DependsOnItself)
{
    add("A:A", CONDITIONAL);
    content = Content::create(DEPENDS_ON_ITSELF, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_FALSE(content->isWaiting());
    ASSERT_TRUE(session->checkAndClear("Malformed package import record"));
}

static const char *MULTI_DEPENDS = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "name": "A",
      "version": "1.0",
      "loadAfter": "B"
    },
    {
      "name": "B",
      "version": "1.0",
      "loadAfter": [ "C", "D" ]
    },
    {
      "name": "C",
      "version": "1.0",
      "loadAfter": "D"
    },
    {
      "name": "D",
      "version": "1.0"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame"
    }
  }
})apl";

TEST_F(PackagesTest, MultiDepends)
{
    add("A:1.0", BASIC);
    add("B:1.0", BASIC);
    add("C:1.0", BASIC);
    add("D:1.0", BASIC);
    content = Content::create(MULTI_DEPENDS, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());
}

static const char *MULTI_DEPENDS_CYCLE = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "name": "A",
      "version": "A",
      "loadAfter": "B"
    },
    {
      "name": "B",
      "version": "B",
      "loadAfter": [ "C", "D" ]
    },
    {
      "name": "C",
      "version": "C",
      "loadAfter": "D"
    },
    {
      "name": "D",
      "version": "D",
      "loadAfter": "B"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame"
    }
  }
})apl";

TEST_F(PackagesTest, MultiDependsCycle)
{
    add("A:A", BASIC);
    add("B:B", BASIC);
    add("C:C", BASIC);
    add("D:D", BASIC);
    content = Content::create(MULTI_DEPENDS_CYCLE, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_FALSE(content->isReady());
    ASSERT_TRUE(session->checkAndClear("Circular package loadAfter dependency between D and B"));
}

static const char* HOST_DOC = R"({
  "type": "APL",
  "version": "2023.1",
  "onConfigChange": {
    "type": "Reinflate"
  },
  "mainTemplate": {
    "item": {
      "type": "Host",
      "width": "100%",
      "height": "100%",
      "source": "embeddedDocumentUrl",
      "onLoad": [
        {
          "type": "SendEvent",
          "sequencer": "SEND_EVENTER",
          "arguments": ["LOADED"]
        }
      ]
    }
  }
})";

TEST_F(PackagesTest, EmbeddedDoc)
{
    std::shared_ptr<TestDocumentManager> documentManager = std::make_shared<TestDocumentManager>();
    config->documentManager(std::static_pointer_cast<DocumentManager>(documentManager));

    add("StyledFrame:1.0", STYLED_FRAME_OVERRIDE);
    add("basic:1.2", BASIC);
    add("conditional:1.2", CONDITIONAL);

    content = Content::create(HOST_DOC, session, metrics, *config);
    ASSERT_TRUE(content->isReady());

    ASSERT_TRUE(documentManager->getUnresolvedRequests().empty());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    auto content = Content::create(THEME_BASED_NESTED_INCLUDE, session);

    ASSERT_TRUE(documentManager->getUnresolvedRequests().size());

    auto request = documentManager->get("embeddedDocumentUrl").lock();
    content->refresh(*request, nullptr);

    // Content becomes "Waiting again"
    ASSERT_TRUE(content->isWaiting());
    // Re-resolve
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true,
                                                            DocumentConfig::create(), true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    ASSERT_EQ(0xff0101ff, root->findComponentById("magicFrame")->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

TEST_F(PackagesTest, ChangeConfigAfterContentInitialization)
{
    add("StyledFrame:1.0", STYLED_FRAME_OVERRIDE_DEPENDS);
    add("dbasic:1.2", BASIC);
    add("conditional:1.2", CONDITIONAL);

    content = Content::create(THEME_BASED_NESTED_INCLUDE, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    // Config (or metrics/or both) changed when RootContext creation possible. Should still account
    // for it.
    config->setEnvironmentValue("hasMagic", "magic");
    content->refresh(metrics, *config);

    // Content becomes "Waiting again"
    ASSERT_TRUE(content->isWaiting());
    // Re-resolve
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0000ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

static const char *BLUE = R"apl({
  "type": "APL",
  "version": "2023.3",
  "resources": [
    {
      "colors": {
        "MyBlue": "#0101ffff"
      }
    }
  ]
})apl";

static const char *MAIN_RED_BLUE = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "name": "red",
      "version": "1.0"
    },
    {
      "name": "blue",
      "version": "1.0"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "100%",
      "height": "100%",
      "borderColor": "@MyBlue",
      "backgroundColor": "@MyRed"
    }
  }
})apl";

TEST_F(PackagesTest, RefreshUsesStashedPackages)
{
    add("red:1.0", BASIC);
    add("blue:1.0", BLUE);

    content = Content::create(MAIN_RED_BLUE, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    // Refresh it
    content->refresh(metrics, *config);

    ASSERT_FALSE(content->isWaiting());
    // Use of stashed packages means no re-processing needed
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0101ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
    ASSERT_EQ(0x0101ffff, component->getCalculated(apl::kPropertyBorderColor).getColor());
}

static const char *MAIN_RED_GREEN_BLUE = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "name": "red",
      "version": "1.0"
    },
    {
      "name": "blue",
      "version": "1.0",
      "when": "${!environment.bluegreen}"
    },
    {
      "name": "bluegreen",
      "version": "1.0",
      "when": "${environment.bluegreen}"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "100%",
      "height": "100%",
      "borderColor": "@MyBlue",
      "backgroundColor": "@MyRed"
    }
  }
})apl";

static const char *BLUEGREEN = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "name": "blue",
      "version": "1.0"
    }
  ],
  "resources": [
    {
      "colors": {
        "MyGreen": "#01ff01ff"
      }
    }
  ]
})apl";

TEST_F(PackagesTest, RefreshUsesStashedPackagesForNewImportDependency)
{
    add("red:1.0", BASIC);
    add("blue:1.0", BLUE);

    content = Content::create(MAIN_RED_GREEN_BLUE, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    config->setEnvironmentValue("bluegreen", true);
    content->refresh(metrics, *config);

    // Reset the package manager to ensure we rely on stashed "blue" and "red"
    reset();
    // Reprocessing is needed to add the "bluegreen" import, which depends on "blue"
    add("bluegreen:1.0", BLUEGREEN);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0101ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
    ASSERT_EQ(0x0101ffff, component->getCalculated(apl::kPropertyBorderColor).getColor());
}

static const char *MAIN_DEEP_BLUE = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "name": "red",
      "version": "1.0"
    },
    {
      "name": "blue",
      "version": "1.0"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "100%",
      "height": "100%",
      "backgroundColor": "@MyDeepBlue"
    }
  }
})apl";

static const char *CONDITITIONAL_BLUE = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "name": "deepblue",
      "version": "1.0",
      "when": "${environment.foo}"
    }
  ],
  "resources": [
    {
      "colors": {
        "MyBlue": "#0101ffff"
      }
    }
  ]
})apl";

static const char *DEEPBLUE = R"apl({
  "type": "APL",
  "version": "2023.3",
  "resources": [
    {
      "colors": {
        "MyDeepBlue": "#0000ffff"
      }
    }
  ]
})apl";

TEST_F(PackagesTest, StashedPackageCanPullInNewDependency)
{
    add("red:1.0", BASIC);
    add("blue:1.0", CONDITITIONAL_BLUE);

    content = Content::create(MAIN_DEEP_BLUE, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    config->setEnvironmentValue("foo", true);
    content->refresh(metrics, *config);

    // Reset the package manager to ensure we rely on stashed "blue" and "red"
    reset();
    // Existing stashed "blue" will suddenly need "deepblue"
    add("deepblue:1.0", DEEPBLUE);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0x0000ffff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

TEST_F(PackagesTest, ChangeConfigAfterContentInitializationReused)
{
    add("StyledFrame:1.0", STYLED_FRAME_OVERRIDE_DEPENDS);
    add("dbasic:1.2", BASIC);
    add("conditional:1.2", CONDITIONAL);

    content = Content::create(THEME_BASED_NESTED_INCLUDE, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    // Replace package manager with empty one
    reset();
    // Should reuse already loaded stuff and succeed
    content->refresh(metrics, *config);

    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0101ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

TEST_F(PackagesTest, ConditionalReinflate)
{
    add("StyledFrame:1.0", STYLED_FRAME_OVERRIDE_DEPENDS);
    add("dbasic:1.2", BASIC);
    add("conditional:1.2", CONDITIONAL);

    content = Content::create(THEME_BASED_NESTED_INCLUDE, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0101ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());

    auto configChange = ConfigurationChange().environmentValue("hasMagic", "magic");
    root->configurationChange(configChange);

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeReinflate, event.getType());

    // So we check related content and re-resolve it.
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    // Now reinflate
    root->reinflate();
    context = root->contextPtr();
    ASSERT_TRUE(context);
    ASSERT_TRUE(context->getReinflationFlag());
    component = CoreComponent::cast(root->topComponent());

    // And resolve
    if (event.getActionRef().isPending()) {
        event.getActionRef().resolve();
    }

    ASSERT_EQ(0xff0000ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

TEST_F(PackagesTest, ConditionalEmbedded)
{
    std::shared_ptr<TestDocumentManager> documentManager = std::make_shared<TestDocumentManager>();
    config->documentManager(std::static_pointer_cast<DocumentManager>(documentManager));

    add("StyledFrame:1.0", STYLED_FRAME_OVERRIDE);
    add("basic:1.2", BASIC);
    add("conditional:1.2", CONDITIONAL);

    content = Content::create(HOST_DOC, session, metrics, *config);
    ASSERT_TRUE(content->isReady());

    ASSERT_TRUE(documentManager->getUnresolvedRequests().empty());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    auto content = Content::create(THEME_BASED_NESTED_INCLUDE, session);

    ASSERT_TRUE(documentManager->getUnresolvedRequests().size());

    auto request = documentManager->get("embeddedDocumentUrl").lock();
    auto documentConfig = DocumentConfig::create();
    documentConfig->setEnvironmentValue("hasMagic", "magic");

    content->refresh(*request, documentConfig);

    // Content becomes "Waiting again"
    ASSERT_TRUE(content->isWaiting());
    // Re-resolve
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", content, true,
                                                            documentConfig, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    ASSERT_EQ(0xff0000ff, root->findComponentById("magicFrame")->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

static const char *SELECTOR = R"apl({
  "type": "APL",
  "version": "2023.3",
  "onConfigChange": {
    "type": "Reinflate"
  },
  "import": [
    {
      "type": "oneOf",
      "items": [
        {
          "name": "another-conditional",
          "version": "1.2",
          "when": "${environment.moreMagic == 'magic'}"
        },
        {
          "name": "conditional",
          "version": "1.2",
          "when": "${environment.hasMagic == 'magic'}"
        },
        {
          "name": "basic",
          "version": "1.2"
        }
      ]
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "100%",
      "height": "100%",
      "backgroundColor": "@MyRed"
    }
  }
})apl";

static const char *MORE_CONDITIONAL = R"apl({
  "type": "APL",
  "version": "2023.3",
  "resources": [
    {
      "colors": {
        "MyRed": "#ff0202ff"
      }
    }
  ]
})apl";

TEST_F(PackagesTest, ConditionalNotSpecifiedSelectOne)
{
    add("basic:1.2", BASIC);
    add("conditional:1.2", CONDITIONAL);
    add("another-conditional:1.2", MORE_CONDITIONAL);

    content = Content::create(SELECTOR, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0101ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

TEST_F(PackagesTest, ConditionalSpecifiedSelectOne)
{
    config->setEnvironmentValue("hasMagic", "magic");

    add("basic:1.2", BASIC);
    add("conditional:1.2", CONDITIONAL);
    add("another-conditional:1.2", MORE_CONDITIONAL);

    content = Content::create(SELECTOR, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0000ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

TEST_F(PackagesTest, ConditionalSpecifiedMultipleSelectOne)
{
    config->setEnvironmentValue("hasMagic", "magic");
    config->setEnvironmentValue("moreMagic", "magic");

    add("basic:1.2", BASIC);
    add("conditional:1.2", CONDITIONAL);
    add("another-conditional:1.2", MORE_CONDITIONAL);

    content = Content::create(SELECTOR, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0202ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

TEST_F(PackagesTest, ConditionalSelectOneReinflate)
{
    add("basic:1.2", BASIC);
    add("conditional:1.2", CONDITIONAL);
    add("another-conditional:1.2", MORE_CONDITIONAL);

    content = Content::create(SELECTOR, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0101ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());

    auto configChange = ConfigurationChange().environmentValue("hasMagic", "magic");
    root->configurationChange(configChange);
    root->clearPending();

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeReinflate, event.getType());

    // So we check related content and re-resolve it.
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    // Now reinflate
    root->reinflate();
    context = root->contextPtr();
    ASSERT_TRUE(context);
    ASSERT_TRUE(context->getReinflationFlag());
    component = CoreComponent::cast(root->topComponent());

    // And resolve
    if (event.getActionRef().isPending()) {
        event.getActionRef().resolve();
    }

    ASSERT_EQ(0xff0000ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

TEST_F(PackagesTest, ConditionalSelectOneReinflateAfterFailure)
{
    add("basic:1.2", BASIC);
    add("another-conditional:1.2", MORE_CONDITIONAL);
  
    content = Content::create(SELECTOR, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0101ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());

    // ConfigChange to a missing package
    auto configChange = ConfigurationChange().environmentValue("hasMagic", "magic");
    root->configurationChange(configChange);
    root->clearPending();

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeReinflate, event.getType());

    // There is no conditional package so expects the failure
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isError());
    ASSERT_TRUE(session->checkAndClear());

    // ConfigChange to an existing package
    configChange = ConfigurationChange().environmentValue("moreMagic", "magic");
    root->configurationChange(configChange);
    root->clearPending();

    ASSERT_TRUE(root->hasEvent());
    event = root->popEvent();
    ASSERT_EQ(kEventTypeReinflate, event.getType());

    // So we check related content and re-resolve it.
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    // Now reinflate
    root->reinflate();
    context = root->contextPtr();
    ASSERT_TRUE(context);
    ASSERT_TRUE(context->getReinflationFlag());
    component = CoreComponent::cast(root->topComponent());

    // And resolve
    if (event.getActionRef().isPending()) {
        event.getActionRef().resolve();
    }

    ASSERT_EQ(0xff0202ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

static const char *COMPLEX_SELECTOR = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "type": "oneOf",
      "items": [
        {
          "name": "first-block-conditional",
          "version": "1.2",
          "when": "${environment.moreMagic == 'magic'}",
          "loadAfter": "second-block-conditional"
        },
        {
          "name": "first-block-fallback",
          "version": "1.2",
          "loadAfter": "second-block-fallback"
        }
      ]
    },
    {
      "name": "non-selector-conditional",
      "when": "${environment.moreMagic == 'magic'}",
      "version": "1.2",
      "loadAfter": "first-block-conditional"
    },
    {
      "name": "non-selector-more-conditional",
      "when": "${environment.moreMagic != 'magic'}",
      "version": "1.2",
      "loadAfter": "first-block-fallback"
    },
    {
      "type": "oneOf",
      "items": [
        {
          "name": "second-block-conditional",
          "version": "1.2",
          "when": "${environment.moreMagic == 'magic'}"
        },
        {
          "name": "second-block-fallback",
          "version": "1.2"
        }
      ]
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "100%",
      "height": "100%",
      "backgroundColor": "@MyRed"
    }
  }
})apl";

TEST_F(PackagesTest, ComplexSelectorNoConditional)
{
    add("first-block-fallback:1.2", BASIC);
    add("first-block-conditional:1.2", BASIC);
    add("second-block-fallback:1.2", BASIC);
    add("second-block-conditional:1.2", BASIC);

    add("non-selector-conditional:1.2", CONDITIONAL);
    add("non-selector-more-conditional:1.2", MORE_CONDITIONAL);

    content = Content::create(COMPLEX_SELECTOR, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0202ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

TEST_F(PackagesTest, ComplexSelectorConditional)
{
    add("first-block-fallback:1.2", BASIC);
    add("first-block-conditional:1.2", BASIC);
    add("second-block-fallback:1.2", BASIC);
    add("second-block-conditional:1.2", BASIC);

    add("non-selector-conditional:1.2", CONDITIONAL);
    add("non-selector-more-conditional:1.2", MORE_CONDITIONAL);

    config->setEnvironmentValue("moreMagic", "magic");

    content = Content::create(COMPLEX_SELECTOR, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0000ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

static const char* STALE_HOST_DOC = R"({
  "type": "APL",
  "version": "2023.1",
  "mainTemplate": {
    "item": {
      "type": "Host",
      "width": "100%",
      "height": "100%",
      "source": "embeddedDocumentUrl",
      "onLoad": [
        {
          "type": "SendEvent",
          "sequencer": "SEND_EVENTER",
          "arguments": ["LOADED"]
        }
      ],
      "environment": { "hasMagic": "${environment.hasMagic}" }
    }
  }
})";

static const char *THEME_BASED_CONDITIONAL = R"apl({
  "type": "APL",
  "version": "2023.3",
  "onConfigChange": {
    "type": "Reinflate"
  },
  "import": [
    {
      "type": "oneOf",
      "items": [
        {
          "name": "conditional",
          "version": "1.2",
          "when": "${viewport.theme == 'magic'}"
        },
        {
          "name": "basic",
          "version": "1.2"
        }
      ]
    }
  ],
  "layouts": {
    "StyledFrame": {
      "item": {
        "type": "Frame",
        "id": "magicFrame",
        "width": "100%",
        "height": "100%",
        "backgroundColor": "@MyRed"
      }
    }
  },
  "mainTemplate": {
    "parameters": [
      "MyParams"
    ],
    "item": {
      "type": "StyledFrame",
      "id": "magicFrame"
    }
  }
})apl";

TEST_F(PackagesTest, ConditionalEmbeddedReinflateTheme)
{
    std::shared_ptr<TestDocumentManager> documentManager = std::make_shared<TestDocumentManager>();
    config->documentManager(std::static_pointer_cast<DocumentManager>(documentManager));

    add("basic:1.2", BASIC);
    add("conditional:1.2", CONDITIONAL);

    content = Content::create(STALE_HOST_DOC, session, metrics, *config);
    ASSERT_TRUE(content->isReady());

    ASSERT_TRUE(documentManager->getUnresolvedRequests().empty());

    root = std::static_pointer_cast<CoreRootContext>(RootContext::create(metrics, content, *config));
    ASSERT_TRUE(root);

    auto embeddedContent = Content::create(THEME_BASED_CONDITIONAL, session);
    ASSERT_TRUE(process(embeddedContent));
    ASSERT_FALSE(embeddedContent->isWaiting());

    ASSERT_TRUE(documentManager->getUnresolvedRequests().size());
    auto request = documentManager->get("embeddedDocumentUrl").lock();
    auto documentConfig = DocumentConfig::create();
    embeddedContent->refresh(*request, documentConfig);

    // Content becomes "Waiting again"
    ASSERT_TRUE(embeddedContent->isWaiting());
    ASSERT_FALSE(embeddedContent->isReady());

    // Re-resolve
    ASSERT_TRUE(process(embeddedContent));
    ASSERT_TRUE(embeddedContent->isReady());

    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", embeddedContent, true,
                                                            documentConfig, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    ASSERT_EQ(0xff0101ff, root->findComponentById("magicFrame")->getCalculated(apl::kPropertyBackgroundColor).getColor());

    // Reinflate

    auto configChange = ConfigurationChange().theme("magic");
    root->configurationChange(configChange);

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeContentRefresh, event.getType());
    auto doc = event.getDocument();
    ASSERT_EQ(embeddedDocumentContext, doc);

    ASSERT_TRUE(embeddedContent->isWaiting());
    ASSERT_TRUE(process(embeddedContent));
    ASSERT_TRUE(embeddedContent->isReady());

    event.getActionRef().resolve();

    advanceTime(100);
    ASSERT_EQ(0xff0000ff, root->findComponentById("magicFrame")->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

static const char *DEEP_SELECTOR = R"apl({
  "type": "APL",
  "version": "2023.3",
  "onConfigChange": {
    "type": "Reinflate"
  },
  "import": [
    {
      "type": "oneOf",
      "items": [
        {
          "type": "oneOf",
          "when": "${environment.hasMagic == 'magic'}",
          "items": [
            {
              "name": "another-conditional",
              "version": "1.2",
              "when": "${environment.moreMagic == 'magic'}"
            },
            {
              "type": "package",
              "name": "conditional",
              "version": "1.2"
            }
          ]
        },
        {
          "type": "package",
          "name": "basic",
          "version": "1.2"
        }
      ]
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "100%",
      "height": "100%",
      "backgroundColor": "@MyRed"
    }
  }
})apl";

TEST_F(PackagesTest, ConditionalDeepSelectorNoConditional)
{
    add("basic:1.2", BASIC);
    add("conditional:1.2", CONDITIONAL);
    add("another-conditional:1.2", MORE_CONDITIONAL);

    content = Content::create(DEEP_SELECTOR, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0101ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

TEST_F(PackagesTest, ConditionalDeepSelectorConditional)
{
    add("basic:1.2", BASIC);
    add("conditional:1.2", CONDITIONAL);
    add("another-conditional:1.2", MORE_CONDITIONAL);

    config->setEnvironmentValue("hasMagic", "magic");

    content = Content::create(DEEP_SELECTOR, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0000ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

TEST_F(PackagesTest, ConditionalDeepSelectorMoreConditional)
{
    add("basic:1.2", BASIC);
    add("conditional:1.2", CONDITIONAL);
    add("another-conditional:1.2", MORE_CONDITIONAL);

    config->setEnvironmentValue("hasMagic", "magic");
    config->setEnvironmentValue("moreMagic", "magic");

    content = Content::create(DEEP_SELECTOR, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0202ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

static const char *SAME_NAME_SELECTOR = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "type": "oneOf",
      "name": "basic",
      "version": "1.0",
      "items": [
        {
          "when": "${environment.moreMagic == 'magic'}",
          "name": "another-conditional",
          "source": "ac_url"
        },
        {
          "when": "${environment.hasMagic == 'magic'}",
          "type": "package",
          "version": "1.1",
          "source": "c_url"
        },
        {
          "source": "basic_url"
        }
      ]
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "100%",
      "height": "100%",
      "backgroundColor": "@MyRed"
    }
  }
})apl";

TEST_F(PackagesTest, SelectorExpandedNameVersionNoConditional)
{
    add("basic:1.0", "basic_url", BASIC);

    content = Content::create(SAME_NAME_SELECTOR, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0101ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

TEST_F(PackagesTest, SelectorExpandedNameVersionConditional)
{
    add("basic:1.1", "c_url", CONDITIONAL);

    config->setEnvironmentValue("hasMagic", "magic");

    content = Content::create(SAME_NAME_SELECTOR, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0000ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

TEST_F(PackagesTest, SelectorExpandedNameVersionMoreConditional)
{
    add("another-conditional:1.0", "ac_url", MORE_CONDITIONAL);

    config->setEnvironmentValue("moreMagic", "magic");

    content = Content::create(SAME_NAME_SELECTOR, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0202ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

static const char *OTHERWISE_SELECTOR = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "type": "oneOf",
      "name": "basic",
      "version": "1.0",
      "items": [
        {
          "when": "${environment.moreMagic == 'magic'}",
          "name": "another-conditional",
          "source": "ac_url"
        },
        {
          "when": "${environment.hasMagic == 'magic'}",
          "type": "package",
          "version": "1.1",
          "source": "c_url"
        }
      ],
      "otherwise": [
        {
          "source": "basic_url"
        }
      ]
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "100%",
      "height": "100%",
      "backgroundColor": "@MyRed"
    }
  }
})apl";

TEST_F(PackagesTest, SelectorExpandedNameVersionOtherwise)
{
    add("basic:1.0", "basic_url", BASIC);

    content = Content::create(OTHERWISE_SELECTOR, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0101ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

static const char *OTHERWISE_MALFORMED = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "type": "oneOf",
      "items": [
        {
          "when": "${environment.moreMagic == 'magic'}",
          "name": "another-conditional",
          "version": "1.0",
          "source": "ac_url"
        },
        {
          "when": "${environment.hasMagic == 'magic'}",
          "type": "package",
          "name": "basic",
          "version": "1.1",
          "source": "c_url"
        }
      ],
      "otherwise": [
        {
          "source": "basic_url"
        }
      ]
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "100%",
      "height": "100%",
      "backgroundColor": "@MyRed"
    }
  }
})apl";

TEST_F(PackagesTest, SelectorOtherwiseFail)
{
    content = Content::create(OTHERWISE_MALFORMED, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isError());
    ASSERT_TRUE(session->checkAndClear("Otherwise imports failed"));
}

static const char *OTHERWISE_EMPTY = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "type": "oneOf",
      "items": [
        {
          "when": "${environment.moreMagic == 'magic'}",
          "name": "another-conditional",
          "version": "1.0",
          "source": "ac_url"
        },
        {
          "when": "${environment.hasMagic == 'magic'}",
          "type": "package",
          "name": "basic",
          "version": "1.1",
          "source": "c_url"
        }
      ],
      "otherwise": []
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "100%",
      "height": "100%",
      "backgroundColor": "@MyRed"
    }
  }
})apl";

TEST_F(PackagesTest, SelectorOtherwiseEmpty)
{
    content = Content::create(OTHERWISE_EMPTY, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isReady());
}

static const char *NO_ITEMS_SELECTOR = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "type": "oneOf",
      "name": "basic",
      "version": "1.0",
      "otherwise": [
        {
          "source": "basic_url"
        }
      ]
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "100%",
      "height": "100%",
      "backgroundColor": "@MyRed"
    }
  }
})apl";

TEST_F(PackagesTest, SelectorNoItems)
{
    add("basic:1.0", "basic_url", BASIC);

    content = Content::create(NO_ITEMS_SELECTOR, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isError());
    ASSERT_TRUE(session->checkAndClear("Missing items field for the oneOf import"));
}

static const char *DEEP_NAME_SELECTOR = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "type": "oneOf",
      "name": "depending",
      "version": "1.2",
      "loadAfter": ["basic"],
      "items": [
        {
          "type": "oneOf",
          "when": "${environment.hasMagic == 'magic'}",
          "items": [
            {
              "when": "${environment.hasMagic == 'magic'}",
              "source": "DEEP_LOADED"
            },
            {
              "type": "package",
              "source": "DEEP_UNLOADED"
            }
          ]
        },
        {
          "source": "SHALLOW_LOADED"
        }
      ]
    },
    {
      "name": "basic",
      "version": "1.2"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "100%",
      "height": "100%",
      "backgroundColor": "@MyRed"
    }
  }
})apl";

TEST_F(PackagesTest, ConditionalDeepNameSelectorNoConditional)
{
    add("basic:1.2", BASIC);
    add("depending:1.2", "DEEP_LOADED", MORE_CONDITIONAL);
    add("depending:1.2", "SHALLOW_LOADED", CONDITIONAL);

    content = Content::create(DEEP_NAME_SELECTOR, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0000ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

TEST_F(PackagesTest, ConditionalDeepNameSelectorConditional)
{
    add("basic:1.2", BASIC);
    add("depending:1.2", "DEEP_LOADED", MORE_CONDITIONAL);
    add("depending:1.2", "SHALLOW_LOADED", CONDITIONAL);

    config->setEnvironmentValue("hasMagic", "magic");

    content = Content::create(DEEP_NAME_SELECTOR, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0202ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

static const char *CONTENT_THEME_CONDITIONAL = R"apl({
  "type": "APL",
  "version": "2023.3",
  "onConfigChange": {
    "type": "Reinflate"
  },
  "import": [
    {
      "type": "oneOf",
      "items": [
        {
          "name": "conditional",
          "version": "1.2",
          "when": "${viewport.theme == 'magic'}"
        },
        {
          "name": "basic",
          "version": "1.2"
        }
      ]
    }
  ],
  "layouts": {
    "StyledFrame": {
      "item": {
        "type": "Frame",
        "id": "magicFrame",
        "width": "100%",
        "height": "100%",
        "backgroundColor": "@MyRed"
      }
    }
  },
  "mainTemplate": {
    "item": {
      "type": "StyledFrame",
      "id": "magicFrame",
      "onMount": {
        "type": "SendEvent",
        "delay": 1000,
        "sequencer": "SEND_EVENT_MAYBE"
      }
    }
  }
})apl";

TEST_F(PackagesTest, EmbeddedThemeConditionalPropagation)
{
    std::shared_ptr<TestDocumentManager> documentManager = std::make_shared<TestDocumentManager>();
    config->documentManager(std::static_pointer_cast<DocumentManager>(documentManager));

    add("basic:1.2", BASIC);
    add("conditional:1.2", CONDITIONAL);

    content = Content::create(STALE_HOST_DOC, session, metrics, *config);
    ASSERT_TRUE(content->isReady());

    ASSERT_TRUE(documentManager->getUnresolvedRequests().empty());

    root = std::static_pointer_cast<CoreRootContext>(RootContext::create(metrics, content, *config));
    ASSERT_TRUE(root);

    auto embeddedContent = Content::create(CONTENT_THEME_CONDITIONAL, session);

    ASSERT_TRUE(documentManager->getUnresolvedRequests().size());

    auto request = documentManager->get("embeddedDocumentUrl").lock();
    auto documentConfig = DocumentConfig::create();
    embeddedContent->refresh(*request, documentConfig);

    // Content becomes "Waiting again"
    ASSERT_TRUE(embeddedContent->isWaiting());
    // Re-resolve
    ASSERT_TRUE(process(embeddedContent));
    ASSERT_TRUE(embeddedContent->isReady());

    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", embeddedContent, true,
                                                            documentConfig, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    ASSERT_EQ(0xff0101ff, root->findComponentById("magicFrame")->getCalculated(apl::kPropertyBackgroundColor).getColor());

    // Reinflate

    auto configChange = ConfigurationChange().theme("magic");
    root->configurationChange(configChange);

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeContentRefresh, event.getType());
    auto doc = event.getDocument();
    ASSERT_EQ(embeddedDocumentContext, doc);

    advanceTime(1000);

    ASSERT_FALSE(root->hasEvent());

    ASSERT_TRUE(embeddedContent->isWaiting());
    ASSERT_TRUE(process(embeddedContent));
    ASSERT_TRUE(embeddedContent->isReady());

    event.getActionRef().resolve();

    advanceTime(100);
    ASSERT_EQ(0xff0000ff, root->findComponentById("magicFrame")->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

static const char *ALL_OF = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "type": "oneOf",
      "items": [
        {
          "type": "allOf",
          "when": "${environment.hasMagic == 'magic'}",
          "items": [
            {
              "name": "another-conditional",
              "version": "1.2"
            }
          ]
        },
        {
          "type": "package",
          "name": "basic",
          "version": "1.2"
        }
      ]
    },
    {
      "type": "allOf",
      "when": "${environment.moreMagic == 'magic'}",
      "items": [
        {
          "type": "package",
          "name": "conditional",
          "loadAfter": [ "basic" ],
          "version": "1.2"
        }
      ]
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "100%",
      "height": "100%",
      "backgroundColor": "@MyRed"
    }
  }
})apl";

TEST_F(PackagesTest, AllOfNoConditional)
{
    add("basic:1.2", BASIC);
    add("conditional:1.2", CONDITIONAL);
    add("another-conditional:1.2", MORE_CONDITIONAL);

    content = Content::create(DEEP_SELECTOR, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0101ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

static const char *ALL_OF_NO_ITEMS = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "type": "allOf"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "100%",
      "height": "100%",
      "backgroundColor": "@MyRed"
    }
  }
})apl";

TEST_F(PackagesTest, AllOfNoItems)
{
    content = Content::create(ALL_OF_NO_ITEMS, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isError());
    ASSERT_TRUE(session->checkAndClear("Missing items field for the allOf import"));
}

TEST_F(PackagesTest, AllOfConditional)
{
    add("basic:1.2", BASIC);
    add("conditional:1.2", CONDITIONAL);
    add("another-conditional:1.2", MORE_CONDITIONAL);

    config->setEnvironmentValue("hasMagic", "magic");

    content = Content::create(ALL_OF, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0202ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

TEST_F(PackagesTest, AllOfMoreConditional)
{
    add("basic:1.2", BASIC);
    add("conditional:1.2", CONDITIONAL);
    add("another-conditional:1.2", MORE_CONDITIONAL);

    config->setEnvironmentValue("moreMagic", "magic");

    content = Content::create(ALL_OF, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ(0xff0000ff, component->getCalculated(apl::kPropertyBackgroundColor).getColor());
}

static const char *NO_LOAD_AFTER = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "type": "package",
      "name": "salad",
      "version": "1.2",
      "loadAfter": [ "potatoes" ]
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "100%",
      "height": "100%",
      "backgroundColor": "@MyRed"
    }
  }
})apl";

TEST_F(PackagesTest, NoLoadAfter)
{
    add("salad:1.2", BASIC);

    content = Content::create(NO_LOAD_AFTER, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_FALSE(content->isReady());

    ASSERT_TRUE(session->checkAndClear("Required loadAfter package not available potatoes for salad"));
}

static const char *LONG_CIRCULAR = R"apl({
  "type": "APL",
  "version": "2023.3",
  "import": [
    {
      "type": "package",
      "name": "A",
      "version": "1.2",
      "loadAfter": [ "B" ]
    },
    {
      "type": "package",
      "name": "B",
      "version": "1.2",
      "loadAfter": [ "C", "BB" ]
    },
    {
      "type": "package",
      "name": "BB",
      "version": "1.2"
    },
    {
      "type": "package",
      "name": "C",
      "version": "1.2",
      "loadAfter": [ "A" ]
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Frame",
      "width": "100%",
      "height": "100%",
      "backgroundColor": "@MyRed"
    }
  }
})apl";

TEST_F(PackagesTest, LongCircularLoadDependency)
{
    add("A:1.2", BASIC);
    add("B:1.2", BASIC);
    add("BB:1.2", BASIC);
    add("C:1.2", BASIC);

    content = Content::create(LONG_CIRCULAR, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_FALSE(content->isReady());

    ASSERT_TRUE(session->checkAndClear("Failure to order packages"));
}

const char* ACCEPT_ALL_OF = R"apl({
    "type": "APL",
    "version": "1.0",
    "import": [
      {
        "type": "allOf",
        "accept": ">1.0",
        "items": [
          {
            "name": "A",
            "version": "1.2"
          },
          {
            "name": "B",
            "version": "1.3"
          }
        ]
      }
    ],
    "mainTemplate": {
      "item": {
        "type": "Text"
      }
    }
})apl";

TEST_F(PackagesTest, CommonAccept)
{
    auto requestA = ImportRequest("A", "1.1", "", {}, SemanticVersion::create(session, "1.1"), nullptr);
    auto requestB = ImportRequest("B", "1.5", "", {}, SemanticVersion::create(session, "1.5"), nullptr);

    content = Content::create(ACCEPT_ALL_OF, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    auto requested = content->getRequestedPackages();
    for (const auto& request : requested) {
        if (requestA.isAcceptableReplacementFor(request))
            content->addPackage(request, BASIC);
        if (requestB.isAcceptableReplacementFor(request))
            content->addPackage(request, BASIC);
    }
    ASSERT_TRUE(content->isReady());
}

const char* ACCEPT_ALL_OF_DEEP = R"apl({
    "type": "APL",
    "version": "1.0",
    "import": [
      {
        "type": "allOf",
        "accept": ">1.0",
        "items": [
          {
            "name": "A",
            "version": "1.2"
          },
          {
            "type": "allOf",
            "items": [
              {
                "name": "B",
                "version": "1.3"
              }
            ]
          }
        ]
      }
    ],
    "mainTemplate": {
      "item": {
        "type": "Text"
      }
    }
})apl";

TEST_F(PackagesTest, CommonAcceptDeep)
{
    auto requestA = ImportRequest("A", "1.1", "", {}, SemanticVersion::create(session, "1.1"), nullptr);
    auto requestB = ImportRequest("B", "1.5", "", {}, SemanticVersion::create(session, "1.5"), nullptr);

    content = Content::create(ACCEPT_ALL_OF_DEEP, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    auto requested = content->getRequestedPackages();
    for (const auto& request : requested) {
        if (requestA.isAcceptableReplacementFor(request))
            content->addPackage(request, BASIC);
        if (requestB.isAcceptableReplacementFor(request))
            content->addPackage(request, BASIC);
    }
    ASSERT_TRUE(content->isReady());
}

const char* ACCEPT_ALL_OF_DEEP_DIFFERENT_ACCEPT = R"apl({
    "type": "APL",
    "version": "1.0",
    "import": [
      {
        "type": "allOf",
        "accept": ">1.0",
        "items": [
          {
            "name": "A",
            "version": "1.2"
          },
          {
            "type": "allOf",
            "accept": ">0.5",
            "items": [
              {
                "name": "B",
                "version": "0.9"
              }
            ]
          }
        ]
      }
    ],
    "mainTemplate": {
      "item": {
        "type": "Text"
      }
    }
})apl";

TEST_F(PackagesTest, CommonAcceptDeepDifferent)
{
    auto requestA = ImportRequest("A", "1.1", "", {}, SemanticVersion::create(session, "1.1"), nullptr);
    auto requestB = ImportRequest("B", "0.8", "", {}, SemanticVersion::create(session, "0.8"), nullptr);

    content = Content::create(ACCEPT_ALL_OF_DEEP_DIFFERENT_ACCEPT, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    auto requested = content->getRequestedPackages();
    for (const auto& request : requested) {
        if (requestA.isAcceptableReplacementFor(request))
            content->addPackage(request, BASIC);
        if (requestB.isAcceptableReplacementFor(request))
            content->addPackage(request, BASIC);
    }
    ASSERT_TRUE(content->isReady());
}

const char* ACCEPT_ALL_OF_OVERRIDE_ACCEPT = R"apl({
    "type": "APL",
    "version": "1.0",
    "import": [
      {
        "type": "allOf",
        "accept": ">1.0",
        "items": [
          {
            "name": "A",
            "version": "1.2"
          },
          {
            "name": "B",
            "version": "0.9",
            "accept": "<1.0"
          }
        ]
      }
    ],
    "mainTemplate": {
      "item": {
        "type": "Text"
      }
    }
})apl";

TEST_F(PackagesTest, CommonAcceptOverrideAccept)
{
    auto requestA = ImportRequest("A", "1.1", "", {}, SemanticVersion::create(session, "1.1"), nullptr);
    auto requestB = ImportRequest("B", "0.8", "", {}, SemanticVersion::create(session, "0.8"), nullptr);

    content = Content::create(ACCEPT_ALL_OF_OVERRIDE_ACCEPT, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    auto requested = content->getRequestedPackages();
    for (const auto& request : requested) {
        if (requestA.isAcceptableReplacementFor(request))
            content->addPackage(request, BASIC);
        if (requestB.isAcceptableReplacementFor(request))
            content->addPackage(request, BASIC);
    }
    ASSERT_TRUE(content->isReady());
}

const char* ACCEPT_ALREADY_REQUESTED = R"apl({
    "type": "APL",
    "version": "1.0",
    "import": [
      {
        "name": "A",
        "version": "1.2"
      },
      {
        "name": "A",
        "version": "0.9",
        "accept": ">1.0"
      }
    ],
    "mainTemplate": {
      "item": {
        "type": "Text"
      }
    }
})apl";

TEST_F(PackagesTest, AlreadyRequestedAcceptedVersion)
{
    auto requestA = ImportRequest("A", "1.2", "", {}, SemanticVersion::create(session, "1.2"), nullptr);

    content = Content::create(ACCEPT_ALREADY_REQUESTED, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    auto requested = content->getRequestedPackages();
    for (const auto& request : requested) {
        if (requestA.isAcceptableReplacementFor(request))
            content->addPackage(request, BASIC);
    }
    ASSERT_TRUE(content->isReady());
}

const char* ACCEPT_ALREADY_LOADED = R"apl({
    "type": "APL",
    "version": "1.0",
    "import": [
      {
        "name": "A",
        "version": "1.2"
      },
      {
        "name": "B",
        "version": "1.2"
      }
    ],
    "mainTemplate": {
      "item": {
        "type": "Text"
      }
    }
})apl";

const char* PACKAGE_ALREADY_LOADED = R"apl({
    "type": "APL",
    "version": "1.0",
    "import": [
      {
        "name": "B",
        "version": "1.3",
        "accept": ">1.0"
      }
    ]
})apl";

TEST_F(PackagesTest, AcceptAlreadyLoaded)
{
    auto requestA = ImportRequest("A", "1.2", "", {}, SemanticVersion::create(session, "1.2"), nullptr);
    auto requestB = ImportRequest("B", "1.2", "", {}, SemanticVersion::create(session, "1.2"), nullptr);

    content = Content::create(ACCEPT_ALREADY_LOADED, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    auto requested = content->getRequestedPackages();
    for (const auto& request : requested) {
        if (requestA.isAcceptableReplacementFor(request))
            content->addPackage(request, PACKAGE_ALREADY_LOADED);
        if (requestB.isAcceptableReplacementFor(request))
            content->addPackage(request, BASIC);
    }
    ASSERT_TRUE(content->isReady());
}

#ifdef ALEXAEXTENSIONS
// Extensions support

using namespace alexaext;

class LittleTestExtension final : public alexaext::ExtensionBase {
public:
    explicit LittleTestExtension(const std::string& uri) : ExtensionBase(std::set<std::string>({uri})) {};

    rapidjson::Document createRegistration(const std::string& uri, const rapidjson::Value& registerRequest) override {
        std::string schema = R"({
            "type":"Schema",
            "version":"1.0"
        })";

        rapidjson::Document doc;
        doc.Parse(schema.c_str());
        doc.AddMember("uri", rapidjson::Value(uri.c_str(), doc.GetAllocator()), doc.GetAllocator());
        return RegistrationSuccess("1.0").uri(uri).token("SessionToken12").schema(doc);
    }

    static std::shared_ptr<LocalExtensionProxy> createProxy(const std::string& uri) {
        return std::make_shared<LocalExtensionProxy>(std::make_shared<LittleTestExtension>(uri));
    }
};

class LittleTestExtensionProvider : public alexaext::ExtensionRegistrar {
public :
    std::function<bool(const std::string& uri)> returnNullProxyPredicate = nullptr;

    ExtensionProxyPtr getExtension(const std::string& uri) {
        return ExtensionRegistrar::getExtension(uri);
    }
};

class LittleTestResourceProvider final: public ExtensionResourceProvider {
public:
    bool requestResource(const std::string& uri, const std::string& resourceId,
                         ExtensionResourceSuccessCallback success,
                         ExtensionResourceFailureCallback error) override {

        // success callback if resource supported
        auto resource = std::make_shared<ResourceHolder>(resourceId);
        success(uri, resource);
        return true;
    };
};

static const char *SELECTOR_WITH_EXTENSIONS = R"apl({
  "type": "APL",
  "version": "2023.3",
  "onConfigChange": {
    "type": "Reinflate"
  },
  "import": [
    {
      "type": "oneOf",
      "items": [
        {
          "name": "conditional",
          "version": "1.2",
          "when": "${environment.hasMagic == 'magic'}"
        },
        {
          "name": "basic",
          "version": "1.2"
        }
      ]
    }
  ],
  "mainTemplate": {
    "item": {
      "id": "magicText",
      "type": "Text",
      "width": "100%",
      "height": "100%",
      "text": "B: ${environment.extension.Basic} C: ${environment.extension.Conditional}"
    }
  }
})apl";

static const char *BASIC_WITH_EXTENSIONS = R"apl({
  "type": "APL",
  "version": "2023.3",
  "extensions": [
    {
      "uri": "alexaext:basic:1.0",
      "name": "Basic"
    }
  ]
})apl";

static const char *CONDITIONAL_WITH_EXTENSIONS = R"apl({
  "type": "APL",
  "version": "2023.3",
  "extensions": [
    {
      "uri": "alexaext:conditional:1.0",
      "name": "Conditional"
    }
  ]
})apl";

TEST_F(PackagesTest, ReinflateWithExtensions)
{
    auto extSession = ExtensionSession::create();
    auto extensionProvider = std::make_shared<LittleTestExtensionProvider>();
    auto resourceProvider = std::make_shared<LittleTestResourceProvider>();
    auto mediator = ExtensionMediator::create(extensionProvider,
                                              resourceProvider,
                                              alexaext::Executor::getSynchronousExecutor(),
                                              extSession);

    extensionProvider->registerExtension(LittleTestExtension::createProxy("alexaext:basic:1.0"));
    extensionProvider->registerExtension(LittleTestExtension::createProxy("alexaext:conditional:1.0"));

    add("basic:1.2", BASIC_WITH_EXTENSIONS);
    add("conditional:1.2", CONDITIONAL_WITH_EXTENSIONS);

    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
        .extensionProvider(extensionProvider)
        .extensionMediator(mediator);

    content = Content::create(SELECTOR_WITH_EXTENSIONS, session, metrics, *config);
    ASSERT_TRUE(content);
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    mediator->initializeExtensions(ObjectMap{}, content);
    mediator->loadExtensions(ObjectMap{}, content, [&](bool result){});

    auto root = RootContext::create(metrics, content, *config);
    ASSERT_TRUE(root);

    rootDocument = root->topDocument();
    component = CoreComponent::cast(root->topComponent());

    ASSERT_EQ("B: true C: ", component->getCalculated(apl::kPropertyText).asString());

    auto configChange = ConfigurationChange().environmentValue("hasMagic", "magic");
    root->configurationChange(configChange);
    root->clearPending();

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeReinflate, event.getType());

    // So we check related content and re-resolve it.
    ASSERT_TRUE(content->isWaiting());
    ASSERT_TRUE(process(content));
    ASSERT_TRUE(content->isReady());

    mediator->initializeExtensions(ObjectMap{}, content);
    mediator->loadExtensions(ObjectMap{}, content, [&](bool result){});

    // Now reinflate
    root->reinflate();
    context = root->contextPtr();
    ASSERT_TRUE(context);
    ASSERT_TRUE(context->getReinflationFlag());
    component = CoreComponent::cast(root->topComponent());

    // And resolve
    if (event.getActionRef().isPending()) {
        event.getActionRef().resolve();
    }

    ASSERT_EQ("B:  C: true", component->getCalculated(apl::kPropertyText).asString());
}

TEST_F(PackagesTest, ReinflateWithExtensionsEmbedded)
{
    auto extSession = ExtensionSession::create();
    auto extensionProvider = std::make_shared<LittleTestExtensionProvider>();
    auto resourceProvider = std::make_shared<LittleTestResourceProvider>();
    auto mediator = ExtensionMediator::create(extensionProvider,
                                              resourceProvider,
                                              alexaext::Executor::getSynchronousExecutor(),
                                              extSession);

    extensionProvider->registerExtension(LittleTestExtension::createProxy("alexaext:basic:1.0"));
    extensionProvider->registerExtension(LittleTestExtension::createProxy("alexaext:conditional:1.0"));

    add("basic:1.2", BASIC_WITH_EXTENSIONS);
    add("conditional:1.2", CONDITIONAL_WITH_EXTENSIONS);

    std::shared_ptr<TestDocumentManager> documentManager = std::make_shared<TestDocumentManager>();
    config->enableExperimentalFeature(RootConfig::kExperimentalFeatureExtensionProvider)
        .extensionProvider(extensionProvider)
        .extensionMediator(mediator)
        .documentManager(std::static_pointer_cast<DocumentManager>(documentManager));

    content = Content::create(STALE_HOST_DOC, session, metrics, *config);
    ASSERT_TRUE(content->isReady());

    ASSERT_TRUE(documentManager->getUnresolvedRequests().empty());

    root = std::static_pointer_cast<CoreRootContext>(RootContext::create(metrics, content, *config));
    ASSERT_TRUE(root);

    auto embeddedContent = Content::create(SELECTOR_WITH_EXTENSIONS, session);

    ASSERT_TRUE(documentManager->getUnresolvedRequests().size());

    auto request = documentManager->get("embeddedDocumentUrl").lock();
    auto documentConfig = DocumentConfig::create();
    documentConfig->extensionMediator(mediator);
    embeddedContent->refresh(*request, documentConfig);

    // Content becomes "Waiting again"
    ASSERT_TRUE(embeddedContent->isWaiting());
    // Re-resolve
    ASSERT_TRUE(process(embeddedContent));
    ASSERT_TRUE(embeddedContent->isReady());

    mediator->initializeExtensions(ObjectMap{}, embeddedContent);
    mediator->loadExtensions(ObjectMap{}, embeddedContent, [&](bool result){});

    auto embeddedDocumentContext = documentManager->succeed("embeddedDocumentUrl", embeddedContent, true,
                                                            documentConfig, true);
    ASSERT_TRUE(embeddedDocumentContext);
    ASSERT_TRUE(CheckSendEvent(root, "LOADED"));

    ASSERT_EQ("B: true C: ", root->findComponentById("magicText")->getCalculated(apl::kPropertyText).asString());

    // Reinflate

    auto configChange = ConfigurationChange().environmentValue("hasMagic", "magic");
    root->configurationChange(configChange);

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeContentRefresh, event.getType());
    auto doc = event.getDocument();
    ASSERT_EQ(embeddedDocumentContext, doc);

    ASSERT_TRUE(embeddedContent->isWaiting());
    ASSERT_TRUE(process(embeddedContent));
    ASSERT_TRUE(embeddedContent->isReady());

    mediator->initializeExtensions(ObjectMap{}, embeddedContent);
    mediator->loadExtensions(ObjectMap{}, embeddedContent, [&](bool result){});

    event.getActionRef().resolve();

    advanceTime(100);
    ASSERT_EQ("B:  C: true", root->findComponentById("magicText")->getCalculated(apl::kPropertyText).asString());
}

#endif // ALEXAEXTENSIONS
