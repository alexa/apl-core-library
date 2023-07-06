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

#include "apl/engine/evaluate.h"

#include "../testeventloop.h"

using namespace apl;

static const char *DATA = R"apl(
{
  "title": "Pecan Pie V"
}
)apl";

class LayoutTest : public DocumentWrapper {};

static const char *SIMPLE_LAYOUT = R"apl(
{
  "type": "APL",
  "version": "1.0",
  "layouts": {
    "SimpleLayout": {
      "parameters": [],
      "items": {
        "type": "Text",
        "text": "${payload.title}"
      }
    }
  },
  "mainTemplate": {
    "parameters": [
      "payload"
    ],
    "item": {
      "type": "SimpleLayout"
    }
  }
}
)apl";

TEST_F(LayoutTest, Simple)
{
    loadDocument(SIMPLE_LAYOUT, DATA);
    auto map = component->getCalculated();

    ASSERT_EQ(kComponentTypeText, component->getType());
    ASSERT_EQ("Pecan Pie V", map[kPropertyText].asString());
}

TEST_F(LayoutTest, SimpleInfo)
{
    loadDocument(SIMPLE_LAYOUT, DATA);

    auto count = root->info().count(Info::kInfoTypeLayout);
    ASSERT_EQ(1, count);

    auto p = root->info().at(Info::kInfoTypeLayout, 0);
    ASSERT_STREQ("SimpleLayout", p.first.c_str());
    ASSERT_STREQ("_main/layouts/SimpleLayout", p.second.c_str());
}


static const char *PARAMETERIZED = R"apl(
{
  "type": "APL",
  "version": "1.0",
  "layouts": {
    "SimpleLayout": {
      "parameters": [
        "text"
      ],
      "items": {
        "type": "Text",
        "text": "${text}"
      }
    }
  },
  "mainTemplate": {
    "parameters": [
      "payload"
    ],
    "item": {
      "type": "SimpleLayout",
      "text": "${payload.title}",
      "width": 222
    }
  }
}
)apl";


TEST_F(LayoutTest, Parameterized)
{
    loadDocument(PARAMETERIZED, DATA);
    auto map = component->getCalculated();

    ASSERT_EQ(kComponentTypeText, component->getType());
    ASSERT_EQ("Pecan Pie V", map[kPropertyText].asString());
    ASSERT_EQ(Object(Dimension(222)), map[kPropertyWidth]);
    ASSERT_EQ(Object(Dimension()), map[kPropertyHeight]);
}

static const char* PARAMETERIZED_DEFAULT_EVALUATE = R"apl(
{
  "type": "APL",
  "version": "1.1",
  "layouts": {
    "myLayout": {
      "parameters": [
        {
          "name": "content",
          "type": "string",
          "default": "${ordinal}"
        }
      ],
      "item": {
        "type": "Text",
        "text": "${content}"
      }
    }
  },
  "mainTemplate": {
    "items": [
      {
        "type": "Sequence",
        "data": [
          "One",
          "Two",
          "Three"
        ],
        "numbered": true,
        "items": [
          {
            "type": "myLayout"
          }
        ]
      }
    ]
  }
}
)apl";

TEST_F(LayoutTest, ParameterizedDefaultEvaluate)
{
    loadDocument(PARAMETERIZED_DEFAULT_EVALUATE);
    auto one = component->getChildAt(0);
    auto two = component->getChildAt(1);
    auto three = component->getChildAt(2);
    ASSERT_EQ(kComponentTypeText, one->getType());
    ASSERT_EQ(kComponentTypeText, two->getType());
    ASSERT_EQ(kComponentTypeText, three->getType());

    ASSERT_EQ("1", one->getCalculated(kPropertyText).asString());
    ASSERT_EQ("2", two->getCalculated(kPropertyText).asString());
    ASSERT_EQ("3", three->getCalculated(kPropertyText).asString());
}


static const char *DOUBLE = R"apl(
{
  "type": "APL",
  "version": "1.0",
  "layouts": {
    "A": {
      "parameters": [
        "text"
      ],
      "items": {
        "type": "Text",
        "text": "${text}"
      }
    },
    "B": {
      "parameters": [
        {
          "name": "w",
          "default": 10
        },
        {
          "name": "h",
          "default": 10
        }
      ],
      "items": {
        "type": "A",
        "height": "${h}",
        "width": "${w}"
      }
    }
  },
  "mainTemplate": {
    "parameters": [
      "payload"
    ],
    "item": {
      "type": "B",
      "h": 52,
      "text": "${payload.title}",
      "width": 222
    }
  }
}
)apl";

TEST_F(LayoutTest, Double)
{
    loadDocument(DOUBLE, DATA);
    auto map = component->getCalculated();

    ASSERT_EQ(kComponentTypeText, component->getType());
    ASSERT_EQ("Pecan Pie V", map[kPropertyText].asString());
    ASSERT_EQ(Object(Dimension(222)), map[kPropertyWidth]);
    ASSERT_EQ(Object(Dimension(52)), map[kPropertyHeight]);
}


static const char *BASIC_WHEN = R"apl(
{
  "type": "APL",
  "version": "1.0",
  "layouts": {
    "Basic": {
      "parameters": [
        "text"
      ],
      "items": {
        "type": "Text",
        "text": "${text}"
      }
    }
  },
  "mainTemplate": {
    "parameters": [
      "payload"
    ],
    "item": {
      "type": "Basic",
      "when": false,
      "text": "${payload.title}"
    }
  }
}
)apl";

TEST_F(LayoutTest, BasicWhen)
{
    loadDocumentExpectFailure(BASIC_WHEN, DATA);
}

static const char *BASIC_WHEN_NESTED = R"apl(
{
  "type": "APL",
  "version": "1.0",
  "layouts": {
    "Basic": {
      "parameters": [
        "text",
        {
          "name": "inflate",
          "default": false
        }
      ],
      "items": {
        "type": "Text",
        "when": "${inflate}",
        "text": "${text}"
      }
    }
  },
  "mainTemplate": {
    "parameters": [
      "payload"
    ],
    "item": {
      "type": "Basic",
      "text": "${payload.title}"
    }
  }
}
)apl";

TEST_F(LayoutTest, BasicWhenNested)
{
    loadDocumentExpectFailure(BASIC_WHEN_NESTED, DATA);
}


static const char *DOUBLE_NESTED = R"apl(
{
  "type": "APL",
  "version": "1.0",
  "layouts": {
    "A": {
      "parameters": [
        "text",
        {
          "name": "inflate",
          "default": true
        }
      ],
      "items": {
        "type": "Text",
        "when": "${inflate}",
        "text": "${text}"
      }
    },
    "B": {
      "parameters": [
        {
          "name": "doB",
          "default": true
        }
      ],
      "items": {
        "type": "A",
        "when": "${doB}"
      }
    }
  },
  "mainTemplate": {
    "parameters": [
      "payload"
    ],
    "item": {
      "type": "Container",
      "items": [
        {
          "type": "B",
          "doB": true,
          "inflate": false,
          "text": "doB=true inflate=false"
        },
        {
          "type": "B",
          "doB": false,
          "inflate": false,
          "text": "doB=false inflate=false"
        },
        {
          "type": "B",
          "doB": true,
          "inflate": true,
          "text": "doB=true inflate=true"
        },
        {
          "type": "B",
          "doB": true,
          "inflate": false,
          "text": "doB=true inflate=false"
        }
      ]
    }
  }
}
)apl";

TEST_F(LayoutTest, DoubleNested)
{
    loadDocument(DOUBLE_NESTED, DATA);
    ASSERT_TRUE(component);

    auto map = component->getCalculated();
    ASSERT_EQ(kComponentTypeContainer, component->getType());
    ASSERT_EQ(1, component->getChildCount());

    auto child = component->getChildAt(0);
    ASSERT_EQ("doB=true inflate=true", child->getCalculated(kPropertyText).asString());
}

static const char *EMBEDDED_CONTENT = R"apl(
{
  "type": "APL",
  "version": "1.1",
  "layouts": {
    "contentControl": {
      "parameters": [
        {
          "name": "content",
          "type": "component"
        },
        "backgroundColor"
      ],
      "item": {
        "type": "Frame",
        "backgroundColor": "${backgroundColor}",
        "width": "100%",
        "height": "100%",
        "item": "${content}"
      }
    }
  },
  "mainTemplate": {
    "items": [
      {
        "type": "contentControl",
        "width": "80vw",
        "height": "80vh",
        "backgroundColor": "red",
        "content": {
          "type": "Text",
          "text": "child"
        }
      }
    ]
  }
}
)apl";

TEST_F(LayoutTest, EmbeddedContent)
{
    loadDocument(EMBEDDED_CONTENT);
    ASSERT_TRUE(component);

    ASSERT_EQ(kComponentTypeFrame, component->getType());
    ASSERT_EQ(1, component->getChildCount());

    auto child = component->getChildAt(0);
    ASSERT_EQ(kComponentTypeText, child->getType());
    ASSERT_TRUE(IsEqual("child", child->getCalculated(kPropertyText).asString()));
}

static const char *DIMENSION_PARAMETER_DEFAULT = R"apl(
{
  "type": "APL",
  "version": "1.1",
  "layouts": {
    "MyText": {
      "parameters": [
        {
          "name": "size",
          "description": "Size (height and width) of the text. Defaults to 300dp.",
          "type": "dimension",
          "default": "300dp"
        }
      ],
      "item": {
        "type": "Text",
        "text": "${size/2}",
        "width": "${size}",
        "height": "${size}"
      }
    }
  },
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "width": "100vw",
        "height": "100vh",
        "items": [
          {
            "type": "MyText"
          }
        ]
      }
    ]
  }
}
)apl";

TEST_F(LayoutTest, TypedLayoutParameterDefault)
{
    loadDocument(DIMENSION_PARAMETER_DEFAULT);
    ASSERT_EQ(kComponentTypeContainer, component->getType());

    auto text = component->getCoreChildAt(0);
    ASSERT_TRUE(text);
    ASSERT_EQ(kComponentTypeText, text->getType());

    ASSERT_EQ("150dp", text->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Rect(0, 0, 300, 300), text->getCalculated(kPropertyBounds).get<Rect>());
}

static const char *DIMENSION_PARAMETER = R"apl(
{
  "type": "APL",
  "version": "1.1",
  "layouts": {
    "MyText": {
      "parameters": [
        {
          "name": "size",
          "description": "Size (height and width) of the text. Defaults to 300dp.",
          "type": "dimension",
          "default": "300dp"
        }
      ],
      "item": {
        "type": "Text",
        "text": "${size/2}",
        "width": "${size}",
        "height": "${size}"
      }
    }
  },
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "width": "100vw",
        "height": "100vh",
        "items": [
          {
            "type": "MyText",
            "size": "200dp"
          }
        ]
      }
    ]
  }
}
)apl";

TEST_F(LayoutTest, TypedLayoutParameter)
{
    loadDocument(DIMENSION_PARAMETER);
    ASSERT_EQ(kComponentTypeContainer, component->getType());

    auto text = component->getCoreChildAt(0);
    ASSERT_TRUE(text);
    ASSERT_EQ(kComponentTypeText, text->getType());

    ASSERT_EQ("100dp", text->getCalculated(kPropertyText).asString());
    ASSERT_EQ(Rect(0, 0, 200, 200), text->getCalculated(kPropertyBounds).get<Rect>());
}

static const char *PROBLEM_PARAMETERS = R"apl(
{
  "type": "APL",
  "version": "2022.2",
  "layouts": {
    "MyText": {
      "parameters": [
        "0_FIRST",
        "SECOND"
      ],
      "item": {
        "type": "Text",
        "text": "${0_FIRST} ${SECOND}"
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "MyText",
      "0_FIRST": "Hello",
      "SECOND": "Goodbye"
    }
  }
}
)apl";

TEST_F(LayoutTest, MapParameter)
{
    loadDocument(PROBLEM_PARAMETERS);
    ASSERT_TRUE(component);

    auto context = component->getContext();
    ASSERT_FALSE(context->has("0_FIRST"));
    ASSERT_TRUE(context->has("SECOND"));
    ASSERT_TRUE(IsEqual("Goodbye", context->opt("SECOND")));

    // Because the first parameter is invalid, the entire string fails to evaluate
    ASSERT_TRUE(IsEqual("${0_FIRST} ${SECOND}",
                        component->getCalculated(apl::kPropertyText).asString()));
    ASSERT_TRUE(ConsoleMessage());
}

static const char *LAYOUT_UNRESOLVED_PARAMETERS = R"apl({
  "type": "APL",
  "version": "2023.2",
  "theme": "light",
  "layouts": {
    "TextLayout": {
      "parameters": [
        {
          "name": "color",
          "default": "black"
        },
        {
          "name": "text",
          "default": "${7+4}"
        }
      ],
      "item": {
        "type": "Text",
        "id": "t1",
        "width": "20vw",
        "height": "10vh",
        "color": "${color}",
        "text": "${text}"
      }
    }
  },
  "mainTemplate": {
    "items": [
      {
        "type": "TextLayout",
        "color": "${data.color}",
        "text": "${data.text}"
      }
    ]
  }
})apl";

TEST_F(LayoutTest, UnresolvedParameters)
{
    loadDocument(LAYOUT_UNRESOLVED_PARAMETERS);
    ASSERT_TRUE(component);

    // We expect it to be resolved to default specified in parameter, and this parameter may be binding
    ASSERT_EQ("11", component->getCalculated(apl::kPropertyText).asString());
    ASSERT_EQ(Color(Color::BLACK), component->getCalculated(apl::kPropertyColor).asColor(session));
}

static const char *LAYOUT_MISSING_PARAMETERS = R"apl({
  "type": "APL",
  "version": "2023.2",
  "theme": "light",
  "layouts": {
    "TextLayout": {
      "parameters": [
        {
          "name": "color",
          "default": "black"
        },
        {
          "name": "text",
          "default": "${7+4}"
        }
      ],
      "item": {
        "type": "Text",
        "id": "t1",
        "width": "20vw",
        "height": "10vh",
        "color": "${color}",
        "text": "${text}"
      }
    }
  },
  "mainTemplate": {
    "items": [
      {
        "type": "TextLayout"
      }
    ]
  }
})apl";

TEST_F(LayoutTest, MissingParameters)
{
    loadDocument(LAYOUT_MISSING_PARAMETERS);
    ASSERT_TRUE(component);

    // We expect it to be resolved to default specified in parameter, and this parameter may be binding
    ASSERT_EQ("11", component->getCalculated(apl::kPropertyText).asString());
    ASSERT_EQ(Color(Color::BLACK), component->getCalculated(apl::kPropertyColor).asColor(session));
}

static const char *LAYOUT_UNRESOLVED_AND_MISSING_PARAMETERS_AS_REFS = R"apl({
  "type": "APL",
  "version": "2023.2",
  "theme": "light",
   "resources": [
      {
         "strings": {
            "longText": "BANANAS"
         }
      }
   ],
  "layouts": {
    "TextLayout": {
      "parameters": [
        {
          "name": "color",
          "default": "black"
        },
        {
          "name": "text",
          "default": "${7+4}"
        },
        {
           "name": "fontFamily",
           "default": "amazon-ember"
        },
        {
           "name":"fontSize",
           "default":"25dp"
        }
      ],
      "item": {
        "type": "Text",
        "id": "t1",
        "width": "20vw",
        "height": "10vh",
        "color": "${color}",
        "fontFamily":"${fontFamily}",
        "fontSize":"${fontSize}",
        "text": "${text}"
      }
    }
  },
  "mainTemplate": {
    "items": [
      {
        "type": "Container",
        "items": [
          {
            "type": "TextLayout",
            "color": "${data.color}",
            "text": "${data.text}",
            "fontFamily": "${data.fontFamily}",
            "fontSize": "${data.fontSize}"
          }
        ],
        "data": [ { "text": "@longText" } ]
      }
    ]
  }
})apl";

TEST_F(LayoutTest, UnresolvedAndMissingParametersAsRefs)
{
    loadDocument(LAYOUT_UNRESOLVED_AND_MISSING_PARAMETERS_AS_REFS);
    ASSERT_TRUE(component);

    auto text = root->findComponentById("t1");

    // We expect it to be resolved to default specified in parameter, and this parameter may be binding
    ASSERT_EQ("BANANAS", text->getCalculated(apl::kPropertyText).asString());
    ASSERT_EQ(Color(Color::BLACK), text->getCalculated(apl::kPropertyColor).asColor(session));
    ASSERT_EQ("amazon-ember", text->getCalculated(apl::kPropertyFontFamily).asString());
    ASSERT_EQ("25dp", text->getCalculated(apl::kPropertyFontSize).asString());
}

static const char* BAD_PARAMETER_NAME = R"apl({
    "type": "APL",
    "version": "2023.1",
    "theme": "dark",
    "layouts": {
      "Foo": {
        "parameters": [
          "invalid}"
        ],
        "item": {
          "type": "Container"
        }
      }
    },
    "mainTemplate": {
      "items": [
        {
          "type": "Foo"
        }
      ]
    }
})apl";

TEST_F(LayoutTest, BAD_PARAMETER_NAME)
{
    loadDocument(BAD_PARAMETER_NAME);
    ASSERT_TRUE(component);
    ASSERT_TRUE(ConsoleMessage());
}