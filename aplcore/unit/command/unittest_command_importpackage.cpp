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
#include "../content/testpackagemanager.h"

using namespace apl;

class ImportPackageTest: public CommandTest{};

static const char *IMPORT_PACKAGE_COMMAND_DOC = R"(
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
              "name": "packageName",
              "version": "1.0",
              "source": "sourceUri"
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
              "type": "InsertItem",
              "componentId": "mainContainer",
              "item": {
                "type": "Text",
                "text": "${@testStringImport}"
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

static const char *PACKAGE_JSON = R"(
{
  "type": "APL",
  "version": "2023.3",
  "description": "test package definition",
  "resources": [
    {
      "string": {
        "testStringImport": "wow, nice string"
      }
    }
  ],
  "import": [],
  "layouts": {}
}
)";

TEST_F(ImportPackageTest, SuccessfulPackageImport) {
    auto testPackageManager = std::make_shared<TestPackageManager>();
    testPackageManager->putPackage("packageName:1.0", PACKAGE_JSON);

    config->packageManager(testPackageManager);
    createContent(IMPORT_PACKAGE_COMMAND_DOC, "{}", true);
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
    ASSERT_EQ("wow, nice string", top->getChildAt(2)->getCalculated(apl::kPropertyText).asString());
}

static const char *IMPORT_PACKAGE_WITH_ONFAIL_ONLOAD_DOC = R"(
{
  "type": "APL",
  "version": "2024.1",
  "onMount": [
    {
      "type": "ImportPackage",
      "name": "packageName",
      "version": "1.0",
      "source": "sourceUri",
      "onFail": [
        {
          "type": "Log",
          "message": "onFail handler command",
          "arguments": [
            "${event.value}",
            "${event.error}",
            "${event.errorCode}"
          ]
        }
      ],
      "onLoad": [
        {
          "type": "Log",
          "message": "onLoad handler command",
          "arguments": [
            "${event.version}"
          ]
        }
      ],
      "accept": ">0.1.10-beta.3"
    }
  ],
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "mainContainer",
      "items": []
    }
  }
}
)";

TEST_F(ImportPackageTest, onFail) {
    auto testPackageManager = std::make_shared<TestPackageManager>();
    config->packageManager(testPackageManager);
    createContent(IMPORT_PACKAGE_WITH_ONFAIL_ONLOAD_DOC, "{}", true);
    content->load([]{}, []{});
    inflate();
    ASSERT_TRUE(root);
    rootDocument = root->topDocument();

    testPackageManager->fail(testPackageManager->get("packageName:1.0"));
    loop->advanceToEnd();

    ASSERT_EQ(1, session->logCommandMessages.size());

    auto m = session->logCommandMessages[0];
    ASSERT_EQ("onFail handler command", m.text);
    ASSERT_NE("onLoad handler command", m.text);
    auto args = m.arguments.getArray();
    ASSERT_EQ("packageName:1.0:sourceUri", args[0].getString());
    ASSERT_EQ("Package not found.", args[1].getString());
    ASSERT_EQ(404, args[2].getInteger());
}

TEST_F(ImportPackageTest, onLoad) {
    auto testPackageManager = std::make_shared<TestPackageManager>();
    testPackageManager->putPackage("packageName:1.0", PACKAGE_JSON);

    config->packageManager(testPackageManager);

    createContent(IMPORT_PACKAGE_WITH_ONFAIL_ONLOAD_DOC, "{}", true);
    content->load([]{}, []{});
    inflate();
    ASSERT_TRUE(root);
    rootDocument = root->topDocument();

    loop->advanceToEnd();

    auto m = session->logCommandMessages[0];
    ASSERT_EQ("onLoad handler command", m.text);
    ASSERT_NE("onFail handler command", m.text);
    ASSERT_EQ("1.0", m.arguments.getArray()[0].getString());
}

static const char *IMPORT_PACKAGE_WITH_MULTIPLE_SAME_IMPORTS_DOC = R"(
{
  "type": "APL",
  "version": "2024.1",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "mainContainer",
      "items": [
        {
          "type": "TouchWrapper",
          "width": 100,
          "height": 100,
          "bind": [
            { "name": "I", "value": 0 }
          ],
          "onPress": [
            {
              "type": "SetValue",
              "property": "I",
              "value": "${I + 1}"
            },
            {
              "type": "ImportPackage",
              "name": "packageName",
              "version": "1.0",
              "sequencer": "DynamicLoader_${I}",
              "source": "sourceUri",
              "onFail": [
                {
                  "type": "Log",
                  "message": "onFail handler command"
                }
              ],
              "onLoad": [
                {
                  "type": "Log",
                  "message": "onLoad handler command"
                }
              ],
              "accept": ">0.1.10-beta.3"
            }
          ]
        }
      ]
    }
  }
}
)";

TEST_F(ImportPackageTest, multipleSameImports) {
    auto testPackageManager = std::make_shared<TestPackageManager>();
    config->packageManager(testPackageManager);
    testPackageManager->putPackage("packageName:1.0", PACKAGE_JSON);

    createContent(IMPORT_PACKAGE_WITH_MULTIPLE_SAME_IMPORTS_DOC, "{}", true);
    content->load([]{}, []{});
    inflate();
    ASSERT_TRUE(root);
    rootDocument = root->topDocument();
    loop->advanceToEnd();

    performTap(1, 1);
    loop->advanceToEnd();
    performTap(1, 1);
    loop->advanceToEnd();
    performTap(1, 1);
    loop->advanceToEnd();

    ASSERT_EQ(3, session->logCommandMessages.size());
    for (const auto& logCommandMessage : session->logCommandMessages) {
        ASSERT_EQ("onLoad handler command", logCommandMessage.text);
    }
}

static const char *IMPORT_PACKAGE_WITH_DIAMOND_DEPENDENCY_DOC = R"(
{
  "type": "APL",
  "version": "2024.1",
  "onMount": {
    "type": "Parallel",
    "commands": [
      {
        "type": "ImportPackage",
        "name": "A",
        "version": "1.0",
        "onFail": [
          {
            "type": "Log",
            "message": "onFail handler command A"
          }
        ],
        "onLoad": [
          {
            "type": "SetValue",
            "componentId": "A",
            "property": "text",
            "value": "${@A}"
          }
        ],
        "accept": ">0.1.10-beta.3"
      },
      {
        "type": "ImportPackage",
        "name": "B",
        "version": "1.0",
        "onFail": [
          {
            "type": "Log",
            "message": "onFail handler command B"
          }
        ],
        "onLoad": [
          {
            "type": "SetValue",
            "componentId": "B",
            "property": "text",
            "value": "${@B}"
          }
        ],
        "accept": ">0.1.10-beta.3"
      }
    ]
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "mainContainer",
      "items": [
        {
          "type": "Text",
          "id": "A"
        },
        {
          "type": "Text",
          "id": "B"
        }
      ]
    }
  }
}
)";

const char *PACKAGE_A = R"apl({
  "type": "APL",
  "version": "1.1",
  "import": [
    {
      "name": "C",
      "version": "1.0"
    }
  ],
  "resources": [
    {
      "strings": {
        "A": "This is A"
      }
    }
  ]
})apl";

const char *PACKAGE_B = R"apl({
  "type": "APL",
  "version": "1.1",
  "import": [
    {
      "name": "C",
      "version": "1.0"
    }
  ],
  "resources": [
    {
      "strings": {
        "B": "This is B"
      }
    }
  ]
})apl";

const char *PACKAGE_C = R"apl({
  "type": "APL",
  "version": "1.1",
  "resources": [
    {
      "strings": {
        "C": "This is C"
      }
    }
  ]
})apl";

TEST_F(ImportPackageTest, DiamondDependencyPackageImport) {
    auto testPackageManager = std::make_shared<TestPackageManager>();
    config->packageManager(testPackageManager);

    createContent(IMPORT_PACKAGE_WITH_DIAMOND_DEPENDENCY_DOC, "{}", true);
    content->load([]{}, []{});
    inflate();
    ASSERT_TRUE(root);
    rootDocument = root->topDocument();

    // Send the ImportPackage command
    loop->advanceToEnd();

    testPackageManager->succeed(testPackageManager->get("A:1.0"), SharedJsonData(PACKAGE_A));
    testPackageManager->succeed(testPackageManager->get("B:1.0"), SharedJsonData(PACKAGE_B));

    auto top = root->topComponent();
    // Request of C still pending, onLoad pending, no text is displayed
    ASSERT_EQ("", top->getChildAt(0)->getCalculated(apl::kPropertyText).asString());
    ASSERT_EQ("", top->getChildAt(1)->getCalculated(apl::kPropertyText).asString());

    testPackageManager->succeed(testPackageManager->get("C:1.0"), SharedJsonData(PACKAGE_C));
    testPackageManager->succeed(testPackageManager->get("C:1.0"), SharedJsonData(PACKAGE_C));
    loop->advanceToEnd();

    ASSERT_EQ("This is A", top->getChildAt(0)->getCalculated(apl::kPropertyText).asString());
    ASSERT_EQ("This is B", top->getChildAt(1)->getCalculated(apl::kPropertyText).asString());
}

TEST_F(ImportPackageTest, NoPackageManager) {
    loadDocument(IMPORT_PACKAGE_WITH_ONFAIL_ONLOAD_DOC);

    loop->advanceToEnd();

    ASSERT_EQ(1, session->logCommandMessages.size());

    auto m = session->logCommandMessages[0];
    ASSERT_EQ("onFail handler command", m.text);
    ASSERT_NE("onLoad handler command", m.text);
}

TEST_F(ImportPackageTest, MultipleSameImportsDoesntReprocess) {
    auto testPackageManager = std::make_shared<TestPackageManager>();

    config->packageManager(testPackageManager);
    createContent(IMPORT_PACKAGE_COMMAND_DOC, "{}", true);
    content->load([]{}, []{});
    inflate();
    ASSERT_TRUE(root);
    rootDocument = root->topDocument();

    // Send the ImportPackage command
    performClick(0, 0);
    loop->advanceToEnd();

    ASSERT_EQ(1, testPackageManager->getUnresolvedRequests().size());
    testPackageManager->succeed(testPackageManager->get("packageName:1.0"), SharedJsonData(PACKAGE_JSON));

    // Send it again and we shouldn't re-trigger a load since it's cached
    performClick(0, 0);
    loop->advanceToEnd();
    ASSERT_EQ(0, testPackageManager->getUnresolvedRequests().size());

    // Trigger InsertItem - using content from dynamically loaded package
    performClick(0, 10);
    loop->advanceToEnd();

    auto top = root->topComponent();
    ASSERT_EQ("wow, nice string", top->getChildAt(2)->getCalculated(apl::kPropertyText).asString());
}

static const char *IMPORT_PACKAGE_FAST_MODE = R"(
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
          "onDown": [
            {
              "type": "ImportPackage",
              "name": "packageName",
              "version": "1.0",
              "source": "sourceUri"
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
              "type": "InsertItem",
              "componentId": "mainContainer",
              "item": {
                "type": "Text",
                "text": "${@testStringImport}"
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

TEST_F(ImportPackageTest, ImportPackageIgnoredInFastMode) {
    auto testPackageManager = std::make_shared<TestPackageManager>();
    testPackageManager->putPackage("packageName:1.0", PACKAGE_JSON);

    config->packageManager(testPackageManager);
    createContent(IMPORT_PACKAGE_FAST_MODE, "{}", true);
    content->load([]{}, []{});
    inflate();
    ASSERT_TRUE(root);
    rootDocument = root->topDocument();

    // Send the ImportPackage command which is ignored
    performClick(0, 0);
    loop->advanceToEnd();

    ASSERT_EQ(0, testPackageManager->getResolvedRequestCount());
    ASSERT_EQ(0, testPackageManager->getUnresolvedRequests().size());
    ASSERT_TRUE(session->checkAndClear());

    // Trigger InsertItem
    performClick(0, 10);
    loop->advanceToEnd();

    // Verify resource didn't load
    auto top = root->topComponent();
    ASSERT_EQ("", top->getChildAt(2)->getCalculated(apl::kPropertyText).asString());
}

static const char *DOC = R"(
{
  "type": "APL",
  "version": "2024.1",
  "mainTemplate": {
    "item": {
      "type": "Container"
    }
  }
}
)";

TEST_F(ImportPackageTest, ImportPackageCommandMissingRequired) {
    auto testPackageManager = std::make_shared<TestPackageManager>();
    testPackageManager->putPackage("packageName:1.0", PACKAGE_JSON);

    config->packageManager(testPackageManager);
    createContent(DOC, "{}", true);
    content->load([]{}, []{});
    inflate();
    ASSERT_TRUE(root);
    rootDocument = root->topDocument();

    auto commands = Object(JsonData(R"(
        [{
          "type": "ImportPackage",
          "version": "1.0",
          "source": "sourceUri"
        }])").moveToObject());
    executeCommands(commands, false);
    loop->advanceToEnd();

    ASSERT_EQ(0, testPackageManager->getResolvedRequestCount());
    ASSERT_EQ(0, testPackageManager->getUnresolvedRequests().size());
    ASSERT_TRUE(session->checkAndClear());

    commands = Object(JsonData(R"(
        [{
          "type": "ImportPackage",
          "name": "packageName",
          "source": "sourceUri"
        }])").moveToObject());
    executeCommands(commands, false);
    loop->advanceToEnd();

    ASSERT_EQ(0, testPackageManager->getResolvedRequestCount());
    ASSERT_EQ(0, testPackageManager->getUnresolvedRequests().size());
    ASSERT_TRUE(session->checkAndClear());
}

static const char *BAD_PACKAGE = R"(
{
  "type": "APL",
  "version": "2024.1",
  "import": "improper imports"
}
)";

TEST_F(ImportPackageTest, BadPackageFailsCommand)
{
    auto testPackageManager = std::make_shared<TestPackageManager>();
    testPackageManager->putPackage("packageName:1.0", BAD_PACKAGE);

    config->packageManager(testPackageManager);
    createContent(DOC, "{}", true);
    content->load([]{}, []{});
    inflate();
    ASSERT_TRUE(root);
    rootDocument = root->topDocument();

    auto commands = Object(JsonData(R"(
        [{
          "type": "ImportPackage",
          "version": "1.0",
          "name": "packageName",
          "source": "sourceUri",
          "onFail": [
            {
              "type": "Log",
              "message": "onFail handler command",
              "arguments": [
                "${event.value}",
                "${event.error}",
                "${event.errorCode}"
              ]
            }
          ]
        }])").moveToObject());
    executeCommands(commands, false);
    loop->advanceToEnd();

    ASSERT_EQ(1, session->logCommandMessages.size());

    auto m = session->logCommandMessages[0];
    ASSERT_EQ("onFail handler command", m.text);
    ASSERT_EQ("Document import property should be an array", m.arguments.getArray()[1].getString());
    ASSERT_TRUE(session->checkAndClear());
}

static const char *IMPORT_BAD = R"(
{
  "type": "APL",
  "version": "2024.1",
  "import": [
    {
      "name": "bad",
      "version": "2.0"
    }
  ]
}
)";

TEST_F(ImportPackageTest, NestedBadPackageFailsCommand)
{
    auto testPackageManager = std::make_shared<TestPackageManager>();
    testPackageManager->putPackage("packageName:1.0", IMPORT_BAD);
    testPackageManager->putPackage("bad:2.0", BAD_PACKAGE);

    config->packageManager(testPackageManager);
    createContent(DOC, "{}", true);
    content->load([]{}, []{});
    inflate();
    ASSERT_TRUE(root);
    rootDocument = root->topDocument();

    auto commands = Object(JsonData(R"(
        [{
          "type": "ImportPackage",
          "version": "1.0",
          "name": "packageName",
          "source": "sourceUri",
          "onFail": [
            {
              "type": "Log",
              "message": "onFail handler command",
              "arguments": [
                "${event.value}",
                "${event.error}",
                "${event.errorCode}"
              ]
            }
          ]
        }])").moveToObject());
    executeCommands(commands, false);
    loop->advanceToEnd();

    ASSERT_EQ(1, session->logCommandMessages.size());

    auto m = session->logCommandMessages[0];
    ASSERT_EQ("onFail handler command", m.text);
    ASSERT_EQ("bad:2.0:", m.arguments.getArray()[0].getString());
    ASSERT_EQ("Document import property should be an array", m.arguments.getArray()[1].getString());
    ASSERT_TRUE(session->checkAndClear());
}