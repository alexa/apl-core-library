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
#include "../debugtools.h"

using namespace apl;

class DeferredEvaluationTest : public DocumentWrapper {};

// The document must be APL version 2023.2 or higher to use deferred evaluation.
static const char *VERSION_TOO_OLD_RESOURCE = R"(
{
  "type": "APL",
  "version": "1.9",
  "resources": [
    {
      "strings": {
        "A": "#{2+3}",
        "B": "${eval(2+3)}"
      }
    }
  ],
  "mainTemplate": {
    "items": {
      "type": "Text",
      "text": "A=${@A} A2=${eval(@A)} B=${@B}"
    }
  }
})";

TEST_F(DeferredEvaluationTest, VersionTooOldResource)
{
    loadDocument(VERSION_TOO_OLD_RESOURCE);
    ASSERT_EQ(component->getCalculated(kPropertyText).asString(), "A=#{2+3} A2= B=");
    ASSERT_TRUE(ConsoleMessage());  // An "Invalid function" message will be logged the first time
}

static const char *VERSION_TOO_OLD_BINDING = R"(
{
  "type": "APL",
  "version": "1.9",
  "mainTemplate": {
    "items": {
      "type": "Text",
      "bind": [
         {
           "name": "A",
           "value": "#{2+3}"
         },
         {
           "name": "B",
           "value": "${eval(2+3)}"
         }
      ],
      "text": "A=${A} A2=${eval(A)} B=${B}"
    }
  }
})";

TEST_F(DeferredEvaluationTest, VersionTooOldBinding)
{
    loadDocument(VERSION_TOO_OLD_BINDING);
    ASSERT_TRUE(component);
    ASSERT_EQ(component->getCalculated(kPropertyText).asString(), "A=#{2+3} A2= B=");
    ASSERT_TRUE(ConsoleMessage());  // An "Invalid function" message will be logged the first time
}


static const char *VERSION_NOT_TOO_OLD_RESOURCE = R"(
{
  "type": "APL",
  "version": "2023.2",
  "resources": [
    {
      "strings": {
        "A": "#{2+3}",
        "B": "${eval(2+3)}"
      }
    }
  ],
  "mainTemplate": {
    "items": {
      "type": "Text",
      "text": "A=${@A} A2=${eval(@A)} B=${@B}"
    }
  }
})";

TEST_F(DeferredEvaluationTest, VersionNotTooOldResource)
{
    loadDocument(VERSION_NOT_TOO_OLD_RESOURCE);
    ASSERT_EQ(component->getCalculated(kPropertyText).asString(), "A=${2+3} A2=5 B=5");
}

static const char *VERSION_NOT_TOO_OLD_BINDING = R"(
{
  "type": "APL",
  "version": "2023.2",
  "resources": [
    {
      "strings": {
        "A": "#{2+3}",
        "B": "${eval(2+3)}"
      }
    }
  ],
  "mainTemplate": {
    "items": {
      "type": "Text",
      "bind": [
        {
          "name": "A",
          "value": "#{2+3}"
        },
        {
          "name": "B",
          "value": "${eval(2+3)}"
        }
      ],
      "text": "A=${@A} A2=${eval(@A)} B=${@B}"
    }
  }
})";

TEST_F(DeferredEvaluationTest, VersionNotTooOldBinding)
{
    loadDocument(VERSION_NOT_TOO_OLD_BINDING);
    ASSERT_EQ(component->getCalculated(kPropertyText).asString(), "A=${2+3} A2=5 B=5");
}


static const char *PASSING_LOCAL_ARGUMENT = R"(
{
  "type": "APL",
  "version": "2023.2",
  "resources": [
    {
      "strings": {
        "A": "The temperature is #{TEMP}"
      }
    }
  ],
  "mainTemplate": {
    "items": {
      "type": "Text",
      "bind": {
        "name": "TEMP",
        "value": 23
      },
      "text": "${eval(@A)}"
    }
  }
})";

TEST_F(DeferredEvaluationTest, PassingLocalArgument)
{
    loadDocument(PASSING_LOCAL_ARGUMENT);
    ASSERT_EQ(component->getCalculated(kPropertyText).asString(), "The temperature is 23");
}


static const char *SHOWING_LOCALIZATION = R"(
{
  "type": "APL",
  "version": "2023.2",
  "resources": [
    {
      "strings": {
        "CELSIUS": "#{TEMP} °C",
        "FAREN": "#{TEMP * 9 / 5 + 32} °F"
      }
    },
    {
      "strings": {
        "TEMPERATURE_FORMAT": "The temperature is ${@CELSIUS}"
      }
    },
    {
      "when": "${environment.lang == 'en_US'}",
      "strings": {
        "TEMPERATURE_FORMAT": "The temperature is ${@FAREN}"
      }
    },
    {
      "when": "${environment.lang == 'fr_CA'}",
      "strings": {
        "TEMPERATURE_FORMAT": "La température est ${@CELSIUS}"
      }
    }
  ],
  "mainTemplate": {
    "items": {
      "type": "Text",
      "bind": {
        "name": "TEMP",
        "value": 25.0
      },
      "text": "${eval(@TEMPERATURE_FORMAT)}"
    }
  }
})";

static const std::vector<std::pair<std::string, std::string>> EXPECTED = {
    { "en_US", u8"The temperature is 77 °F" },
    { "en_GB", u8"The temperature is 25 °C" },
    { "fr_CA", u8"La température est 25 °C" },
};

TEST_F(DeferredEvaluationTest, ShowingLocalization)
{
    for (const auto& m : EXPECTED) {
        config->set(kLang, m.first);
        loadDocument(SHOWING_LOCALIZATION);
        ASSERT_TRUE(component);
        ASSERT_EQ(component->getCalculated(kPropertyText).asString(), m.second) << m.first;
    }
}


static const char * DEFERRED_BINDINGS = R"(
{
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "bind": [
      {
        "name": "A",
        "value": "Duck"
      },
      {
        "name": "B",
        "value": "Test value #{A}"
      }
    ],
    "item": {
      "type": "Text",
      "text": "${eval(B)}"
    }
  }
}
)";

TEST_F(DeferredEvaluationTest, Bindings)
{
    loadDocument(DEFERRED_BINDINGS);
    ASSERT_TRUE(component);
    auto B = component->getContext()->opt("B");
    ASSERT_TRUE(IsEqual(B, "Test value ${A}"));
    ASSERT_EQ(component->getCalculated(kPropertyText).asString(), "Test value Duck");
}

static const char * DEFERRED_BINDINGS_LOCAL_VALUE = R"(
{
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "bind": [
      {
        "name": "B",
        "value": "Test value #{A}"
      }
    ],
    "item": {
      "type": "Text",
      "bind": {
        "name": "A",
        "value": "Duck"
      },
      "text": "${eval(B)}"
    }
  }
}
)";

TEST_F(DeferredEvaluationTest, BindingsLocalValue)
{
    loadDocument(DEFERRED_BINDINGS_LOCAL_VALUE);
    ASSERT_TRUE(component);
    ASSERT_EQ(component->getCalculated(kPropertyText).asString(), "Test value Duck");
}

static const char * DEFERRED_BINDINGS_TWISTED = R"(
{
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "bind": [
      {
        "name": "B",
        "value": "Test value #{eval(C)}"
      }
    ],
    "item": {
      "type": "Text",
      "bind": [
        {
          "name": "A",
          "value": "Duck"
        },
        {
          "name": "C",
          "value": "#{'This is a ${A}'}"
        }
      ],
      "text": "${eval(B)}"
    }
  }
}
)";

TEST_F(DeferredEvaluationTest, Twisted)
{
    loadDocument(DEFERRED_BINDINGS_TWISTED);
    ASSERT_TRUE(component);
    ASSERT_EQ(component->getCalculated(kPropertyText).asString(), "Test value This is a Duck");
}


static const char *SYMBOL_RESOLUTION = R"(
{
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "items": {
      "type": "Text",
      "id": "TEST",
      "bind": [
        {
          "name": "A",
          "value": "#{B?C:D}"
        },
        {
          "name": "B",
          "value": true
        },
        {
          "name": "C",
          "value": "Foo"
        },
        {
          "name": "D",
          "value": "Bar"
        }
      ],
      "text": "${eval(A)}"
    }
  }
}
)";

TEST_F(DeferredEvaluationTest, SymbolResolution) {
    loadDocument(SYMBOL_RESOLUTION);
    ASSERT_TRUE(component);
    ASSERT_EQ(component->getCalculated(kPropertyText).asString(), "Foo");

    // Changing the value of C should result in the re-evaluation of the text
    executeCommand("SetValue", {{"componentId", "TEST"}, {"property", "C"}, {"value", "Baz"}},
                   true);
    ASSERT_EQ(component->getCalculated(kPropertyText).asString(), "Baz");

    // Changing the value of D should not change things
    executeCommand("SetValue", {{"componentId", "TEST"}, {"property", "D"}, {"value", "Turtle"}},
                   true);
    ASSERT_EQ(component->getCalculated(kPropertyText).asString(), "Baz");

    // Changing B will change the output text
    executeCommand("SetValue", {{"componentId", "TEST"}, {"property", "B"}, {"value", false}},
                   true);
    ASSERT_EQ(component->getCalculated(kPropertyText).asString(), "Turtle");
}

// Test nested evaluations
static const char *NESTED_EVALUATION = R"(
{
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "items": {
      "type": "Text",
      "id": "TEST",
      "bind": [
        {
          "name": "A",
          "value": "#{B}"
        },
        {
          "name": "B",
          "value": "#{C}"
        },
        {
          "name": "C",
          "value": "FOO"
        },
        {
          "name": "D",
          "value": "TURTLE"
        }
      ],
      "text": "A=${A} eval(A)=${eval(A)} eval(eval(A))=${eval(eval(A))}"
    }
  }
}
)";

TEST_F(DeferredEvaluationTest, NestedEvaluation)
{
    loadDocument(NESTED_EVALUATION);
    ASSERT_TRUE(component);
    ASSERT_EQ(component->getCalculated(kPropertyText).asString(), "A=${B} eval(A)=${C} eval(eval(A))=FOO");

    // Change the value of "C"
    executeCommand("SetValue", {{"componentId", "TEST"}, {"property", "C"}, {"value", "BAR"}}, true);
    ASSERT_EQ(component->getCalculated(kPropertyText).asString(), "A=${B} eval(A)=${C} eval(eval(A))=BAR");

    // Change the value of "B"
    executeCommand("SetValue", {{"componentId", "TEST"}, {"property", "B"}, {"value", "#{D}"}}, true);
    ASSERT_EQ(component->getCalculated(kPropertyText).asString(), "A=${B} eval(A)=${D} eval(eval(A))=TURTLE");

    // Change the value of "D"
    executeCommand("SetValue", {{"componentId", "TEST"}, {"property", "D"}, {"value", "WOMBAT"}}, true);
    ASSERT_EQ(component->getCalculated(kPropertyText).asString(), "A=${B} eval(A)=${D} eval(eval(A))=WOMBAT");

    // Change the value of "A". Note that evaluating a non-data-bound string just gives you the string
    executeCommand("SetValue", {{"componentId", "TEST"}, {"property", "A"}, {"value", "THUD"}}, true);
    ASSERT_EQ(component->getCalculated(kPropertyText).asString(), "A=THUD eval(A)=THUD eval(eval(A))=THUD");
}


// Test for infinite evaluation loops
static const char *INFINITE_LOOP_SINGLE = R"(
{
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "items": {
      "type": "Text",
      "id": "TEST",
      "bind": {
        "name": "A",
        "value": "-#{eval(A)}-"
      },
      "text": "A=${A} eval(A)=${eval(A)}"
    }
  }
})";

TEST_F(DeferredEvaluationTest, InfiniteLoopSingle)
{
    loadDocument(INFINITE_LOOP_SINGLE);
    ASSERT_TRUE(component);
    std::string s(kEvaluationDepthLimit, '-');  // The evaluation limit is compile-time defined
    ASSERT_EQ(component->getCalculated(kPropertyText).asString(),
              "A=-${eval(A)}- eval(A)="+s+"-${eval(A)}-"+s);
    ASSERT_TRUE(ConsoleMessage());  // Expect a warning message
}

static const char *INFINITE_LOOP_PAIR = R"(
{
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "items": {
      "type": "Text",
      "id": "TEST",
      "bind": [
        {
          "name": "A",
          "value": "X#{eval(B)}"
        },
        {
          "name": "B",
          "value": "Y#{eval(A)}"
        }
      ],
      "text": "A=${A} B=${B} eval(A)=${eval(A)}"
    }
  }
})";

TEST_F(DeferredEvaluationTest, InfiniteLoopPair)
{
    loadDocument(INFINITE_LOOP_PAIR);
    ASSERT_TRUE(component);
    // Generate the string based on the evaluation depth limit
    std::string s;
    for (int i = 0 ; i <= kEvaluationDepthLimit ; i++)
        s += (i % 2) ? 'Y' : 'X';
    std::string s2(kEvaluationDepthLimit % 2 ? "A" : "B");

    ASSERT_EQ(component->getCalculated(kPropertyText).asString(),
              "A=X${eval(B)} B=Y${eval(A)} eval(A)="+s+"${eval("+s2+")}");
    ASSERT_TRUE(ConsoleMessage());  // Expect a warning message
}


static const char *ARRAY_EVALUATION = R"(
{
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "bind": [
        {
          "name": "A",
          "value": [
            1,
            "${2}",
            "#{B}"
          ]
        },
        {
          "name": "B",
          "value": 3
        },
        {
          "name": "C",
          "value": "${eval(A)}"
        }
      ],
      "text": "C0=${C[0]} C1=${C[1]} C2=${C[2]}"
    }
  }
})";

TEST_F(DeferredEvaluationTest, ArrayEvaluation)
{
    loadDocument(ARRAY_EVALUATION);
    ASSERT_TRUE(component);
    ASSERT_EQ(component->getCalculated(kPropertyText).asString(), "C0=1 C1=2 C2=3");
}


static const char *OBJECT_EVALUATION = R"(
{
  "type": "APL",
  "version": "2023.2",
  "mainTemplate": {
    "item": {
      "type": "Text",
      "bind": [
        {
          "name": "A",
          "value": {
            "X": 1,
            "Y": "${2}",
            "Z": "#{B}"
          }
        },
        {
          "name": "B",
          "value": 3
        },
        {
          "name": "C",
          "value": "${eval(A)}"
        }
      ],
      "text": "CX=${C.X} CY=${C.Y} CZ=${C.Z}"
    }
  }
})";

TEST_F(DeferredEvaluationTest, ObjectEvaluation)
{
    loadDocument(OBJECT_EVALUATION);
    ASSERT_TRUE(component);
    ASSERT_EQ(component->getCalculated(kPropertyText).asString(), "CX=1 CY=2 CZ=3");
}