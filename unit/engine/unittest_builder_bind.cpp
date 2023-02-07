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

class BuilderBindTest : public DocumentWrapper {};

static const char *DOCUMENT_BIND = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "bind": {
          "name": "TEST",
          "value": 23
        },
        "items": {
          "type": "Text",
          "text": "${TEST}"
        }
      }
    }
)apl";

TEST_F(BuilderBindTest, DocumentBind)
{
    loadDocument(DOCUMENT_BIND);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("23", component->getCalculated(kPropertyText).asString()));
}

static const char *LAYOUT_BIND = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "layouts": {
        "MyText": {
          "parameters": "NAME",
          "bind": {
            "name": "COUNT",
            "value": 1
          },
          "items": {
            "type": "Text",
            "text": "${NAME}-${COUNT}"
          }
        }
      },
      "mainTemplate": {
        "items": {
          "type": "MyText",
          "id": "TEXTER",
          "NAME": "Spot"
        }
      }
    }
)apl";

TEST_F(BuilderBindTest, LayoutBind)
{
    loadDocument(LAYOUT_BIND);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("Spot-1", component->getCalculated(kPropertyText).asString()));

    executeCommand("SetValue", {{"componentId", "TEXTER"}, {"property", "COUNT"}, {"value", 23}}, false);
    ASSERT_TRUE(IsEqual("Spot-23", component->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(component, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component));
}

static const char *LAYOUT_BIND_INNER_BIND = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "layouts": {
        "MyText": {
          "parameters": "NAME",
          "bind": {
            "name": "COUNT",
            "value": 1
          },
          "items": {
            "type": "Text",
            "bind": {
              "name": "COUNT",
              "value": "${COUNT+100}"
            },
            "text": "${NAME}-${COUNT}"
          }
        }
      },
      "mainTemplate": {
        "items": {
          "type": "MyText",
          "id": "TEXTER",
          "NAME": "Spot"
        }
      }
    }
)apl";

TEST_F(BuilderBindTest, LayoutBindInnerBind)
{
    loadDocument(LAYOUT_BIND_INNER_BIND);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("Spot-101", component->getCalculated(kPropertyText).asString()));

    // The inner binding is attached to the component, so it gets hit
    executeCommand("SetValue", {{"componentId", "TEXTER"}, {"property", "COUNT"}, {"value", 23}}, false);
    ASSERT_TRUE(IsEqual("Spot-23", component->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(component, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component));
}

static const char *MANY_BINDS = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "layouts": {
        "MyText": {
          "parameters": "NAME",
          "bind": {
            "name": "COUNT",
            "value": "${COUNT+100}"
          },
          "items": {
            "type": "Text",
            "bind": {
              "name": "COUNT",
              "value": "${COUNT+100}"
            },
            "text": "${NAME}-${COUNT}"
          }
        }
      },
      "mainTemplate": {
        "bind": {
          "name": "COUNT",
          "value": 12
        },
        "items": {
          "type": "MyText",
          "id": "TEXTER",
          "NAME": "Spot"
        }
      }
    }
)apl";

TEST_F(BuilderBindTest, ManyBinds)
{
    loadDocument(MANY_BINDS);
    ASSERT_TRUE(component);

    ASSERT_TRUE(IsEqual("Spot-212", component->getCalculated(kPropertyText).asString()));

    // The inner binding is attached to the component, so it gets hit
    executeCommand("SetValue", {{"componentId", "TEXTER"}, {"property", "COUNT"}, {"value", 23}}, false);
    ASSERT_TRUE(IsEqual("Spot-23", component->getCalculated(kPropertyText).asString()));
    ASSERT_TRUE(CheckDirty(component, kPropertyText, kPropertyVisualHash));
    ASSERT_TRUE(CheckDirty(root, component));
}


static const char *BIND_DUPLICATES_PARAMETER = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "layouts": {
        "MyText": {
          "parameters": "NAME",
          "bind": [
            {
              "name": "NAME",
              "value": "A ${NAME}"
            }
          ],
          "items": {
            "type": "Text",
            "text": "${NAME}"
          }
        }
      },
      "mainTemplate": {
        "items": {
          "type": "MyText",
          "id": "TEXTER",
          "NAME": "Spot"
        }
      }
    }
)apl";

/**
 * Attempting to bind a named value in the same context as a pre-existing parameter should fail
 */
TEST_F(BuilderBindTest, BindDuplicatesParameter)
{
    loadDocument(BIND_DUPLICATES_PARAMETER);
    ASSERT_TRUE(ConsoleMessage());  // We should get a message warning of a pre-existing property
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual("Spot", component->getCalculated(kPropertyText).asString()));  // The "bind" didn't take place
}


static const char *BIND_DUPLICATES_BIND = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "mainTemplate": {
        "bind": [
          {
            "name": "FOO",
            "value": 23
          },
          {
            "name": "FOO",
            "value": 22
          }
        ],
        "items": {
          "type": "Text",
          "text": "${FOO}"
        }
      }
    }
)apl";


/**
 * Binding the same value twice should fail
 */
TEST_F(BuilderBindTest, BindDuplicatesBind)
{
    loadDocument(BIND_DUPLICATES_BIND);
    ASSERT_TRUE(ConsoleMessage());  // We should get a message warning of a pre-existing property
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual("23", component->getCalculated(kPropertyText).asString()));  // The second bind failed
}


static const char *BIND_NAMING = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "bind": { "name": "NAME", "value": "VALUE" }
    }
  }
}
)apl";

static std::map<std::string, std::string> GOOD_NAME_TESTS = {
    {"_foo", "A"},
    {"__bar__", "B"},
    {"_234", "C"},
    {"a", "D"},
    {"a99_____", "E"},
    {"_", "F"},
};

TEST_F(BuilderBindTest, GoodNameCheck)
{
    for (const auto& it : GOOD_NAME_TESTS) {
        auto doc = std::regex_replace(BIND_NAMING, std::regex("NAME"), it.first);
        doc = std::regex_replace(doc, std::regex("VALUE"), it.second);
        loadDocument(doc.c_str());
        ASSERT_TRUE(component);
        auto context = component->getContext();
        ASSERT_TRUE(context->hasLocal(it.first));
        ASSERT_TRUE(IsEqual(it.second, evaluate(*context, "${" + it.first + "}")));
    }
}

static std::vector<std::string> BAD_NAME_TESTS = {
    "234_foo",
    "åbc",
    "abç",
    "a-b",
    "0",
    "",
};

TEST_F(BuilderBindTest, BadNameCheck)
{
    for (const auto& m : BAD_NAME_TESTS) {
        auto doc = std::regex_replace(BIND_NAMING, std::regex("NAME"), m);
        loadDocument(doc.c_str());
        ASSERT_TRUE(component);
        auto context = component->getContext();
        ASSERT_FALSE(context->hasLocal(m));
        ASSERT_TRUE(ConsoleMessage());
    }
}

static const char *MISSING_VALUE = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "bind": { "name": "NAME" }
    }
  }
}
)apl";

TEST_F(BuilderBindTest, MissingValue)
{
    loadDocument(MISSING_VALUE);
    ASSERT_TRUE(component);
    auto context = component->getContext();
    ASSERT_FALSE(context->hasLocal("NAME"));
    ASSERT_TRUE(ConsoleMessage());
}