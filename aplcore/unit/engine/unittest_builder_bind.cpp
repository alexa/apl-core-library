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

static const char *ON_CHANGE = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "TARGET",
      "bind": {
        "name": "NAME",
        "value": 1,
        "onChange": {
          "type": "SendEvent",
          "arguments": [
            "${event.source.handler}",
            "${event.current}",
            "${event.previous}",
            "${event.source.bind.NAME}"
          ],
          "sequencer": "FOO"
        }
      }
    }
  }
}
)apl";

/**
 * Simple test for onChange.  The handler name, current value, previous value, and event.source binding are checked.
 */
TEST_F(BuilderBindTest, OnChange)
{
    loadDocument(ON_CHANGE);
    ASSERT_TRUE(component);
    auto context = component->getContext();
    ASSERT_TRUE(context->hasLocal("NAME"));
    ASSERT_FALSE(ConsoleMessage());

    executeCommand("SetValue", {{"property", "NAME"}, {"componentId", "TARGET"}, {"value", 2}}, true);
    root->clearPending();
    ASSERT_TRUE(CheckSendEvent(root, "Change", 2, 1, 2));

    executeCommand("SetValue", {{"property", "NAME"}, {"componentId", "TARGET"}, {"value", 12}}, true);
    root->clearPending();
    ASSERT_TRUE(CheckSendEvent(root, "Change", 12, 2, 12));
}

static const char *ON_CHANGE_ARRAY = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "TARGET",
      "bind": {
        "name": "NAME",
        "value": [1,2,3],
        "onChange": {
          "type": "SendEvent",
          "arguments": [
            "${event.current[0]}",
            "${event.previous[0]}",
            "${event.current[1]}",
            "${event.previous[1]}",
            "${event.current.length}"
          ],
          "sequencer": "FOO"
        }
      }
    }
  }
}
)apl";

/**
 * Start with a bound array and assign new values to it.  The "onChange" handler
 * should be called unless the two arrays are equal.
 */
TEST_F(BuilderBindTest, OnChangeArray)
{
    loadDocument(ON_CHANGE_ARRAY);
    ASSERT_TRUE(component);
    auto context = component->getContext();
    ASSERT_TRUE(context->hasLocal("NAME"));
    ASSERT_FALSE(ConsoleMessage());

    // Assign a new array
    executeCommand("SetValue", {{"property",    "NAME"},
                                {"componentId", "TARGET"},
                                {"value",       ObjectArray{10, 2, 3, 4}}}, true);
    root->clearPending();
    ASSERT_TRUE(CheckSendEvent(root, 10, 1, 2, 2, 4));

    // Assign an array with the same values
    executeCommand("SetValue", {{"property",    "NAME"},
                                {"componentId", "TARGET"},
                                {"value",       ObjectArray{10, 2, 3, 4}}}, true);
    root->clearPending();
    ASSERT_FALSE(root->hasEvent());

    // Change to something that is not an array
    executeCommand("SetValue", {{"property",    "NAME"},
                                {"componentId", "TARGET"},
                                {"value",       "fred"}}, true);
    root->clearPending();
    ASSERT_TRUE(CheckSendEvent(root, Object::NULL_OBJECT(), 10, Object::NULL_OBJECT(), 2, Object::NULL_OBJECT()));
}


static const char *ON_CHANGE_OBJECT = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "id": "TARGET",
      "bind": {
        "name": "NAME",
        "value": {"A": 1, "B": 2},
        "onChange": {
          "type": "SendEvent",
          "arguments": [
            "${event.current['A']}",
            "${event.previous['A']}",
            "${event.current['B']}",
            "${event.previous['B']}"
          ],
          "sequencer": "FOO"
        }
      }
    }
  }
}
)apl";

/**
 * Start with a bound Map and assign new values to it.  The "onChange" handler
 * should be invoked if the two maps are not equal.
 */
TEST_F(BuilderBindTest, OnChangeObject)
{
    loadDocument(ON_CHANGE_OBJECT);
    ASSERT_TRUE(component);
    auto context = component->getContext();
    ASSERT_TRUE(context->hasLocal("NAME"));
    ASSERT_FALSE(ConsoleMessage());

    // Assign a new object with the same keys
    executeCommand("SetValue", {{"property",    "NAME"},
                                {"componentId", "TARGET"},
                                {"value",       std::make_shared<ObjectMap>(ObjectMap{{"A", 10}, {"B", 20}})}}, true);
    root->clearPending();
    ASSERT_TRUE(CheckSendEvent(root, 10, 1, 20, 2));

    // Assign the object with the same values
    executeCommand("SetValue", {{"property",    "NAME"},
                                {"componentId", "TARGET"},
                                {"value",       std::make_shared<ObjectMap>(ObjectMap{{"A", 10}, {"B", 20}})}}, true);
    root->clearPending();
    ASSERT_FALSE(root->hasEvent());

    // Change to something that is not an object
    executeCommand("SetValue", {{"property",    "NAME"},
                                {"componentId", "TARGET"},
                                {"value",       2}}, true);
    root->clearPending();
    ASSERT_TRUE(CheckSendEvent(root, Object::NULL_OBJECT(), 10, Object::NULL_OBJECT(), 20));
}


static const char *ON_CHANGE_RECURSIVE = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "TARGET",
      "text": "${NAME}",
      "bind": {
        "name": "NAME",
        "value": 10,
        "onChange": [
          {
            "type": "SetValue",
            "property": "NAME",
            "value": "${NAME + 1}"
          }
        ]
      }
    }
  }
}
)apl";

/**
 * Test a recursive call - that is, changing the value of a bound property causes it
 * to change itself again.  We avoid an infinite loop by preventing the "onChange" handler
 * from being called recursively.
 */
TEST_F(BuilderBindTest, OnChangeRecursive)
{
    loadDocument(ON_CHANGE_RECURSIVE);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(component->getCalculated(kPropertyText).asString(), "10"));

    // Set the value of NAME. This should
    //   1. Change the value of NAME from 10 to 1.
    //   2. Call NAME's "onChange" handler
    //   3. Set the value of NAME to NAME+1 (2).
    //   4. Call NAME's "onChange" handler, which refuses to run because the call in step #2 hasn't returned yet.
    executeCommand("SetValue", {{"property", "NAME"}, {"componentId", "TARGET"}, {"value", 1}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(component->getCalculated(kPropertyText).asString(), "2"));
}

static const char *ON_CHANGE_RECURSIVE_TWO = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "id": "TARGET",
      "bind": [
        {
          "name": "A",
          "value": 1,
          "onChange": {
            "type": "SetValue",
            "property": "B",
            "value": "${A + 1}"
          }
        },
        {
          "name": "B",
          "value": 2,
          "onChange": {
            "type": "SetValue",
            "property": "A",
            "value": "${B + 1}"
          }
        }
      ],
      "text": "A=${A} B=${B}"
    }
  }
}
)apl";

/**
 * Test the recursion block with two variables.
 */
TEST_F(BuilderBindTest, OnChangeRecursiveTwo)
{
    loadDocument(ON_CHANGE_RECURSIVE_TWO);
    ASSERT_TRUE(component);
    ASSERT_TRUE(IsEqual(component->getCalculated(kPropertyText).asString(), "A=1 B=2"));

    // Set the value of A.  This should:
    //   1. Change the value of A to 10
    //   2. Call A's "onChange" handler
    //   3. Set the value of B to 11
    //   4. Call B's "onChange" handler
    //   5. Set the value of A to 12
    //   6. Call A's "onChange" handler which refuses to run because step #2 hasn't finished
    executeCommand("SetValue", {{"property", "A"}, {"componentId", "TARGET"}, {"value", 10}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(component->getCalculated(kPropertyText).asString(), "A=12 B=11"));

    // Set the value of B.  This should:
    //   1. Change the value of B to 20
    //   2. Call B's "onChange" handler
    //   3. Set the value of A to 21
    //   4. Call A's "onChange" handler
    //   5. Set the value of B to 22
    //   6. Call B's "onChange" handler which refuses to run because step #2 hasn't finished
    executeCommand("SetValue", {{"property", "B"}, {"componentId", "TARGET"}, {"value", 20}}, true);
    root->clearPending();
    ASSERT_TRUE(IsEqual(component->getCalculated(kPropertyText).asString(), "A=21 B=22"));
}

static const char *ON_CHANGE_LIVE_ARRAY = R"apl(
{
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Container",
      "item": {
        "type": "Text",
        "bind": [
          {
            "name": "COUNTER",
            "value": 0
          },
          {
            "name": "DATA",
            "value": "${data}",
            "onChange": {
              "type": "SetValue",
              "property": "COUNTER",
              "value": "${COUNTER + 1}"
            }
          }
        ],
        "text": "${data} ${COUNTER}"
      },
      "data": "${TestArray}"
    }
  }
}
)apl";

/**
 * Hook up a live array.
 */
TEST_F(BuilderBindTest, OnChangeLiveArray)
{
    auto myArray = LiveArray::create({"A", "B", "C"});
    config->liveData("TestArray", myArray);

    loadDocument(ON_CHANGE_LIVE_ARRAY);
    ASSERT_TRUE(component);
    ASSERT_EQ(3, component->getChildCount());

    auto c1 = component->getChildAt(0);
    auto c2 = component->getChildAt(1);
    auto c3 = component->getChildAt(2);

    auto checker = [&](std::string s1, std::string s2, std::string s3) -> bool {
        return IsEqual(c1->getCalculated(kPropertyText).asString(), s1) &&
               IsEqual(c2->getCalculated(kPropertyText).asString(), s2) &&
               IsEqual(c3->getCalculated(kPropertyText).asString(), s3);
    };

    ASSERT_TRUE(checker("A 0", "B 0", "C 0"));

    myArray->update(0, "D");
    root->clearPending();
    ASSERT_TRUE(checker("D 1", "B 0", "C 0"));

    myArray->update(0, "E");
    myArray->update(1, "F");
    myArray->update(2, "G");
    root->clearPending();
    ASSERT_TRUE(checker("E 2", "F 1", "G 1"));

    // Modify the array, but don't actually change the values
    myArray->update(0, "E");
    myArray->update(1, "F");
    myArray->update(2, "G");
    root->clearPending();
    ASSERT_TRUE(checker("E 2", "F 1", "G 1"));
}


static const char *ON_CHANGE_LAYOUT = R"apl(
{
  "type": "APL",
  "version": "2023.2",
  "layouts": {
    "Wrapper": {
      "parameters": [
        "NAME",
        "VALUE"
      ],
      "bind": [
        {
          "name": "COUNTER",
          "value": 0
        },
        {
          "name": "WATCHER",
          "value": "${VALUE}",
          "onChange": {
            "type": "SetValue",
            "property": "COUNTER",
            "value": "${COUNTER + 1}"
          }
        }
      ],
      "items": {
        "type": "Text",
        "text": "${NAME} ${VALUE} ${COUNTER}"
      }
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "items": {
        "type": "Wrapper",
        "NAME": "${data}",
        "VALUE": "${MyObject[data]}"
      },
      "data": "${Map.keys(MyObject)}"
    }
  }
}
)apl";

/**
 * Bindings in a layout are eventually hooked up to the underlying component.
 *
 * We build a series of Text components based on a LiveMap and display the
 * name, value, and number of times the object has changed.
 */
TEST_F(BuilderBindTest, OnChangeLayout)
{
    auto myMap = LiveMap::create(ObjectMap{{"A", "Hello"},{"B", "Goodbye"}});
    config->liveData("MyObject", myMap);

    loadDocument(ON_CHANGE_LAYOUT);
    ASSERT_TRUE(component);
    ASSERT_EQ(2, component->getChildCount());

    auto c1 = component->getChildAt(0);
    auto c2 = component->getChildAt(1);

    auto checker = [&](std::string s1, std::string s2) -> bool {
        return IsEqual(c1->getCalculated(kPropertyText).asString(), s1) &&
               IsEqual(c2->getCalculated(kPropertyText).asString(), s2);
    };

    ASSERT_TRUE(checker("A Hello 0", "B Goodbye 0"));

    myMap->set("A", "Salut");
    root->clearPending();
    ASSERT_TRUE(checker("A Salut 1", "B Goodbye 0"));

    myMap->set("B", "Adios");
    root->clearPending();
    ASSERT_TRUE(checker("A Salut 1", "B Adios 1"));

    myMap->set("A", "Bonjour");
    myMap->set("B", "Au revoir");
    root->clearPending();
    ASSERT_TRUE(checker("A Bonjour 2", "B Au revoir 2"));

    // Update without changing anything
    myMap->set("B", "Au revoir");
    root->clearPending();
    ASSERT_TRUE(checker("A Bonjour 2", "B Au revoir 2"));
}

