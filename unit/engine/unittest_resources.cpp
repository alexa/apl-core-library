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

class ResourceTest : public DocumentWrapper {};

static const char *BASIC_TEST_SINGLE = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "resources": {
    "numbers": {
      "one": 1,
      "two": 2
    },
    "colors": {
      "myRed": "red",
      "myBlue": "rgb(0,0,255) "
    },
    "dimensions": {
      "short": "20dp",
      "medium": 40,
      "long": "50vw",
      "gap": "10%"
    },
    "strings": {
      "name": "Fred"
    },
    "booleans": {
      "myTrue": true,
      "myFalse": "${false}"
    }
  }
})";

TEST_F(ResourceTest, BasicSingle)
{
    metrics.size(1024,800);
    loadDocument(BASIC_TEST_SINGLE);

    ASSERT_EQ(11, root->info().resources().size());

    ASSERT_EQ(1, context->opt("@one").asNumber());
    ASSERT_EQ(2, context->opt("@two").asNumber());

    ASSERT_EQ(0xff0000ff, context->opt("@myRed").getColor());
    ASSERT_EQ(0x0000ffff, context->opt("@myBlue").getColor());

    auto dim = context->opt("@short").asDimension(*context);
    ASSERT_TRUE(dim.isAbsolute());
    ASSERT_EQ(20, dim.getValue());

    ASSERT_TRUE(context->opt("@medium").isAbsoluteDimension());
    ASSERT_EQ(40, context->opt("@medium").getAbsoluteDimension());

    ASSERT_TRUE(context->opt("@long").isAbsoluteDimension());
    ASSERT_EQ(512, context->opt("@long").getAbsoluteDimension());

    ASSERT_TRUE(context->opt("@gap").isRelativeDimension());
    ASSERT_EQ(10, context->opt("@gap").getRelativeDimension());

    ASSERT_EQ("Fred", context->opt("@name").asString());

    ASSERT_EQ(true, context->opt("@myTrue").asBoolean());
    ASSERT_EQ(false, context->opt("@myFalse").asBoolean());
}

static const char *BASIC_TEST = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "resources": [
    {
      "numbers": {
        "one": 1,
        "two": 2
      },
      "colors": {
        "myRed": "red",
        "myBlue": "rgb(0,0,255) "
      },
      "dimensions": {
        "short": "20dp",
        "medium": 40,
        "long": "50vw",
        "gap": "10%"
      },
      "strings": {
        "name": "Fred"
      },
      "booleans": {
        "myTrue": true,
        "myFalse": "${false}"
      }
    }
  ]
})";

TEST_F(ResourceTest, Basic)
{
    metrics.size(1024,800);
    loadDocument(BASIC_TEST);

    ASSERT_EQ(11, root->info().resources().size());

    ASSERT_EQ(1, context->opt("@one").asNumber());
    ASSERT_EQ(2, context->opt("@two").asNumber());

    ASSERT_EQ(0xff0000ff, context->opt("@myRed").getColor());
    ASSERT_EQ(0x0000ffff, context->opt("@myBlue").getColor());

    auto dim = context->opt("@short").asDimension(*context);
    ASSERT_TRUE(dim.isAbsolute());
    ASSERT_EQ(20, dim.getValue());

    ASSERT_TRUE(context->opt("@medium").isAbsoluteDimension());
    ASSERT_EQ(40, context->opt("@medium").getAbsoluteDimension());

    ASSERT_TRUE(context->opt("@long").isAbsoluteDimension());
    ASSERT_EQ(512, context->opt("@long").getAbsoluteDimension());

    ASSERT_TRUE(context->opt("@gap").isRelativeDimension());
    ASSERT_EQ(10, context->opt("@gap").getRelativeDimension());

    ASSERT_EQ("Fred", context->opt("@name").asString());

    ASSERT_EQ(true, context->opt("@myTrue").asBoolean());
    ASSERT_EQ(false, context->opt("@myFalse").asBoolean());
}

TEST_F(ResourceTest, BasicProvenance)
{
    metrics.size(1024,800);
    loadDocument(BASIC_TEST);

    ASSERT_STREQ("_main/resources/0/numbers/one", context->provenance("@one").c_str());
    ASSERT_STREQ("_main/resources/0/numbers/two", context->provenance("@two").c_str());

    ASSERT_STREQ("_main/resources/0/colors/myRed", context->provenance("@myRed").c_str());
    ASSERT_STREQ("_main/resources/0/colors/myBlue", context->provenance("@myBlue").c_str());

    ASSERT_STREQ("_main/resources/0/dimensions/short", context->provenance("@short").c_str());
    ASSERT_STREQ("_main/resources/0/dimensions/medium", context->provenance("@medium").c_str());
    ASSERT_STREQ("_main/resources/0/dimensions/long", context->provenance("@long").c_str());
    ASSERT_STREQ("_main/resources/0/dimensions/gap", context->provenance("@gap").c_str());

    ASSERT_STREQ("_main/resources/0/strings/name", context->provenance("@name").c_str());

    ASSERT_STREQ("_main/resources/0/booleans/myTrue", context->provenance("@myTrue").c_str());
    ASSERT_STREQ("_main/resources/0/booleans/myFalse", context->provenance("@myFalse").c_str());

    // Sanity check that path actually matches rapidjson Pointer implementation
    ASSERT_EQ(followPath(context->provenance("@one"))->GetInt(), 1);
}

static const std::map<std::string, std::string> EXPECTED = {
    { "@one", "_main/resources/0/numbers/one" },
    { "@two", "_main/resources/0/numbers/two" },
    { "@myRed", "_main/resources/0/colors/myRed" },
    { "@myBlue", "_main/resources/0/colors/myBlue" },
    { "@short", "_main/resources/0/dimensions/short" },
    { "@medium", "_main/resources/0/dimensions/medium" },
    { "@long", "_main/resources/0/dimensions/long" },
    { "@gap", "_main/resources/0/dimensions/gap" },
    { "@name", "_main/resources/0/strings/name" },
    { "@myTrue", "_main/resources/0/booleans/myTrue" },
    { "@myFalse", "_main/resources/0/booleans/myFalse" },
};

TEST_F(ResourceTest, BasicInfo)
{
    metrics.size(1024, 800);
    loadDocument(BASIC_TEST);

    auto resources = root->info().resources();
    ASSERT_EQ(EXPECTED.size(), resources.size());

    for (const auto& m : EXPECTED) {
        auto it = resources.find(m.first);
        ASSERT_NE(it, resources.end());
        ASSERT_EQ(it->second, m.second);
    }
}

TEST_F(ResourceTest, DisabledProvenance)
{
    metrics.size(1024,800);
    config->trackProvenance(false);
    loadDocument(BASIC_TEST);

    ASSERT_STREQ("", context->provenance("@one").c_str());
    ASSERT_STREQ("", context->provenance("@two").c_str());

    ASSERT_STREQ("", context->provenance("@myRed").c_str());
    ASSERT_STREQ("", context->provenance("@myBlue").c_str());

    ASSERT_STREQ("", context->provenance("@short").c_str());
    ASSERT_STREQ("", context->provenance("@medium").c_str());
    ASSERT_STREQ("", context->provenance("@long").c_str());
    ASSERT_STREQ("", context->provenance("@gap").c_str());

    ASSERT_STREQ("", context->provenance("@name").c_str());

    ASSERT_STREQ("", context->provenance("@myTrue").c_str());
    ASSERT_STREQ("", context->provenance("@myFalse").c_str());
}

static const char *OVERRIDE_TEST = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "resources": [
    {
      "numbers": {
        "one": 1,
        "two": 2
      },
      "colors": {
        "myRed": "red",
        "myBlue": "rgb(0,0,255) "
      },
      "dimensions": {
        "short": "20dp",
        "medium": 40,
        "long": "50vw",
        "gap": "10%"
      },
      "strings": {
        "name": "Fred"
      },
      "booleans": {
        "myTrue": true,
        "myFalse": "${false}"
      }
    },
    {
      "when": "${viewport.shape == 'round'}",
      "numbers": {
        "one": "@two",
        "three": 3.0
      },
      "strings": {
        "name": "${@name + @name}"
      }
    },
    {
      "when": "${viewport.width < 800}",
      "dimensions": {
        "medium": 22
      }
    }
  ]
})";

TEST_F(ResourceTest, Override)
{
    metrics.size(1000,1000).shape(ScreenShape::ROUND);
    loadDocument(OVERRIDE_TEST);

    ASSERT_EQ(12, root->info().resources().size());

    ASSERT_EQ(2, context->opt("@one").asNumber());  // Overridden by "when" clause
    ASSERT_STREQ("_main/resources/1/numbers/one", context->provenance("@one").c_str());

    ASSERT_EQ(2, context->opt("@two").asNumber());
    ASSERT_STREQ("_main/resources/0/numbers/two", context->provenance("@two").c_str());

    ASSERT_EQ(3, context->opt("@three").asNumber());  // New value
    ASSERT_STREQ("_main/resources/1/numbers/three", context->provenance("@three").c_str());

    ASSERT_EQ(0xff0000ff, context->opt("@myRed").getColor());
    ASSERT_STREQ("_main/resources/0/colors/myRed", context->provenance("@myRed").c_str());

    ASSERT_EQ(0x0000ffff, context->opt("@myBlue").getColor());
    ASSERT_STREQ("_main/resources/0/colors/myBlue", context->provenance("@myBlue").c_str());

    auto dim = context->opt("@short").asDimension(*context);
    ASSERT_TRUE(dim.isAbsolute());
    ASSERT_EQ(20, dim.getValue());
    ASSERT_STREQ("_main/resources/0/dimensions/short", context->provenance("@short").c_str());

    ASSERT_TRUE(context->opt("@medium").isAbsoluteDimension());
    ASSERT_EQ(40, context->opt("@medium").getAbsoluteDimension());  // Was NOT overridden
    ASSERT_STREQ("_main/resources/0/dimensions/medium", context->provenance("@medium").c_str());

    ASSERT_TRUE(context->opt("@long").isAbsoluteDimension());
    ASSERT_EQ(500, context->opt("@long").getAbsoluteDimension());   // New screen width
    ASSERT_STREQ("_main/resources/0/dimensions/long", context->provenance("@long").c_str());

    ASSERT_TRUE(context->opt("@gap").isRelativeDimension());
    ASSERT_EQ(10, context->opt("@gap").getRelativeDimension());
    ASSERT_STREQ("_main/resources/0/dimensions/gap", context->provenance("@gap").c_str());

    ASSERT_EQ("FredFred", context->opt("@name").asString());   // Overridden
    ASSERT_STREQ("_main/resources/1/strings/name", context->provenance("@name").c_str());

    ASSERT_EQ(true, context->opt("@myTrue").asBoolean());
    ASSERT_STREQ("_main/resources/0/booleans/myTrue", context->provenance("@myTrue").c_str());

    ASSERT_EQ(false, context->opt("@myFalse").asBoolean());
    ASSERT_STREQ("_main/resources/0/booleans/myFalse", context->provenance("@myFalse").c_str());
}

static const char *LINEAR_GRADIENT = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "resources": [
    {
      "gradient": {
        "myLinear": {
          "description": "Sample linear",
          "colorRange": [
            "blue",
            "red"
          ]
        }
      }
    }
  ]
})";

TEST_F(ResourceTest, LinearGradient)
{
    loadDocument(LINEAR_GRADIENT);

    ASSERT_EQ(1, root->info().resources().size());

    auto object = context->opt("@myLinear");
    ASSERT_TRUE(object.isGradient());

    auto grad = object.getGradient();
    ASSERT_EQ(Gradient::LINEAR, grad.getType());
    ASSERT_EQ(0, grad.getAngle());

    auto colorRange = grad.getColorRange();
    ASSERT_EQ(2, colorRange.size());
    ASSERT_EQ(Color(Color::BLUE), colorRange.at(0));
    ASSERT_EQ(Color(Color::RED), colorRange.at(1));

    auto inputRange = grad.getInputRange();
    ASSERT_EQ(2, inputRange.size());
    ASSERT_EQ(0, inputRange.at(0));
    ASSERT_EQ(1, inputRange.at(1));
}

static const char *RADIAL_GRADIENT = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "resources": [
    {
      "gradient": {
        "myRadial": {
          "description": "Sample radial gradient",
          "type": "radial",
          "colorRange": [
            "blue",
            "red"
          ],
          "inputRange": [
            0.2,
            0.5
          ]
        }
      }
    }
  ]
})";

TEST_F(ResourceTest, RadialGradient)
{
    loadDocument(RADIAL_GRADIENT);

    ASSERT_EQ(1, root->info().resources().size());

    auto object = context->opt("@myRadial");
    ASSERT_TRUE(object.isGradient());

    auto grad = object.getGradient();
    ASSERT_EQ(Gradient::RADIAL, grad.getType());

    auto colorRange = grad.getColorRange();
    ASSERT_EQ(2, colorRange.size());
    ASSERT_EQ(Color(Color::BLUE), colorRange.at(0));
    ASSERT_EQ(Color(Color::RED), colorRange.at(1));

    auto inputRange = grad.getInputRange();
    ASSERT_EQ(2, inputRange.size());
    ASSERT_EQ(0.2, inputRange.at(0));
    ASSERT_EQ(0.5, inputRange.at(1));
}

static const char *RICH_LINEAR = R"({
  "type": "APL",
  "version": "1.0",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "resources": [
    {
      "colors": {
        "myRed": "rgb(red, 50%) ",
        "myGreen": "blue"
      },
      "numbers": {
        "myAngle": 45,
        "myFirstStop": 0.5,
        "mySecondStop": 0.8
      }
    },
    {
      "gradients": {
        "myLinear": {
          "type": "linear",
          "colorRange": [
            "@myRed",
            "@myGreen",
            "rgba(${@myGreen}, 50%) "
          ],
          "inputRange": [
            "${@myFirstStop / 2}",
            "@mySecondStop",
            1
          ],
          "angle": "@myAngle"
        }
      }
    }
  ]
})";

TEST_F(ResourceTest, RichLinearGradient)
{
    loadDocument(RICH_LINEAR);

    auto object = context->opt("@myLinear");
    ASSERT_TRUE(object.isGradient());

    auto grad = object.getGradient();
    ASSERT_EQ(Gradient::LINEAR, grad.getType());
    ASSERT_EQ(45, grad.getAngle());

    auto colorRange = grad.getColorRange();
    ASSERT_EQ(3, colorRange.size());
    ASSERT_EQ(Color(0xff00007f), colorRange.at(0));
    ASSERT_EQ(Color(0x0000ffff), colorRange.at(1));
    ASSERT_EQ(Color(0x0000ff7f), colorRange.at(2));

    auto inputRange = grad.getInputRange();
    ASSERT_EQ(3, inputRange.size());
    ASSERT_EQ(0.25, inputRange.at(0));
    ASSERT_EQ(0.8, inputRange.at(1));
    ASSERT_EQ(1, inputRange.at(2));
}

static const char *GRADIENT_ANGLE = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "resources": [
    {
      "gradients": {
        "l0": {
          "type": "linear",
          "colorRange": [ "red", "green" ],
          "inputRange": [ 0.0, 1.0 ],
          "angle": 0
        },
        "l90": {
          "type": "linear",
          "colorRange": [ "red", "green" ],
          "inputRange": [ 0.0, 1.0 ],
          "angle": 90
        },
        "l180": {
          "type": "linear",
          "colorRange": [ "red", "green" ],
          "inputRange": [ 0.0, 1.0 ],
          "angle": 180
        },
        "l270": {
          "type": "linear",
          "colorRange": [ "red", "green" ],
          "inputRange": [ 0.0, 1.0 ],
          "angle": 270
        },
        "l360": {
          "type": "linear",
          "colorRange": [ "red", "green" ],
          "inputRange": [ 0.0, 1.0 ],
          "angle": 360
        },
        "l45": {
          "type": "linear",
          "colorRange": [ "red", "green" ],
          "inputRange": [ 0.0, 1.0 ],
          "angle": 45
        },
        "l30": {
          "type": "linear",
          "colorRange": [ "red", "green" ],
          "inputRange": [ 0.0, 1.0 ],
          "angle": 30
        },
        "l120": {
          "type": "linear",
          "colorRange": [ "red", "green" ],
          "inputRange": [ 0.0, 1.0 ],
          "angle": 120
        },
        "l210": {
          "type": "linear",
          "colorRange": [ "red", "green" ],
          "inputRange": [ 0.0, 1.0 ],
          "angle": 210
        },
        "l300": {
          "type": "linear",
          "colorRange": [ "red", "green" ],
          "inputRange": [ 0.0, 1.0 ],
          "angle": 300
        },
        "l390": {
          "type": "linear",
          "colorRange": [ "red", "green" ],
          "inputRange": [ 0.0, 1.0 ],
          "angle": 390
        },
        "ln60": {
          "type": "linear",
          "colorRange": [ "red", "green" ],
          "inputRange": [ 0.0, 1.0 ],
          "angle": -60
        },
        "ln150": {
          "type": "linear",
          "colorRange": [ "red", "green" ],
          "inputRange": [ 0.0, 1.0 ],
          "angle": -150
        }
      }
    }
  ]
})";

static const std::vector<std::vector<Object>> GRADIENT_ANGLE_TESTS = {
    {"@l0",    0,    0.5, 0, 0.5, 1},
    {"@l90",   90,   0, 0.5, 1, 0.5},
    {"@l180",  180,  0.5, 1, 0.5, 0},
    {"@l270",  270,  1, 0.5, 0, 0.5},
    {"@l360",  360,  0.5, 0, 0.5, 1},
    {"@l45",   45,   0, 0, 1, 1},

    {"@l30",   30,   0.1585, -0.0915, 0.8415, 1.0915},
    {"@l120",  120,  -0.0915, 0.8415, 1.0915, 0.1584},
    {"@l210",  210,  0.8415, 1.09150, 0.1584, -0.0915},
    {"@l300",  300,  1.0915, 0.1584, -0.0915, 0.8415},
    {"@l390",  390,  0.1585, -0.0915, 0.8415, 1.0915},
    {"@ln60",  -60,  1.0915, 0.1584, -0.0915, 0.8415},
    {"@ln150", -150, 0.8415, 1.0915, 0.1584, -0.0915},
};

TEST_F(ResourceTest, GradientAngle)
{
    loadDocument(GRADIENT_ANGLE);

    for (auto& t : GRADIENT_ANGLE_TESTS) {
        LOG(LogLevel::kWarn) << t.at(0).toDebugString();
        auto object = context->opt(t.at(0).asString());
        ASSERT_TRUE(object.isGradient());

        auto& grad = object.getGradient();
        ASSERT_EQ(Gradient::LINEAR, grad.getType());

        auto colorRange = grad.getProperty(kGradientPropertyColorRange);
        ASSERT_EQ(2, colorRange.size());
        ASSERT_EQ(Color(Color::RED), colorRange.at(0).asColor());
        ASSERT_EQ(Color(Color::GREEN), colorRange.at(1).asColor());

        auto inputRange = grad.getProperty(kGradientPropertyInputRange);
        ASSERT_EQ(2, inputRange.size());
        ASSERT_EQ(0, inputRange.at(0).asNumber());
        ASSERT_EQ(1, inputRange.at(1).asNumber());

        auto angle = grad.getProperty(kGradientPropertyAngle);
        ASSERT_EQ(t.at(1).asNumber(), angle.asNumber());

        auto spreadMethod = grad.getProperty(kGradientPropertySpreadMethod);
        ASSERT_EQ(Gradient::PAD, static_cast<Gradient::GradientSpreadMethod>(spreadMethod.asInt()));

        ASSERT_NEAR(t.at(2).asNumber(), grad.getProperty(kGradientPropertyX1).asNumber(), 0.0001);
        ASSERT_NEAR(t.at(3).asNumber(), grad.getProperty(kGradientPropertyY1).asNumber(), 0.0001);
        ASSERT_NEAR(t.at(4).asNumber(), grad.getProperty(kGradientPropertyX2).asNumber(), 0.0001);
        ASSERT_NEAR(t.at(5).asNumber(), grad.getProperty(kGradientPropertyY2).asNumber(), 0.0001);
    }
}

static const char *GRADIENT_RADIAL_FULL = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "Container"
    }
  },
  "resources": [
    {
      "gradients": {
        "rad": {
          "type": "radial",
          "colorRange": [ "red", "green" ],
          "inputRange": [ 0.0, 1.0 ]
        }
      }
    }
  ]
})";

TEST_F(ResourceTest, GradientRadialFull)
{
    loadDocument(GRADIENT_RADIAL_FULL);

    auto object = context->opt("@rad");
    ASSERT_TRUE(object.isGradient());

    auto& grad = object.getGradient();
    ASSERT_EQ(Gradient::RADIAL, grad.getType());

    auto colorRange = grad.getProperty(kGradientPropertyColorRange);
    ASSERT_EQ(2, colorRange.size());
    ASSERT_EQ(Color(Color::RED), colorRange.at(0).asColor());
    ASSERT_EQ(Color(Color::GREEN), colorRange.at(1).asColor());

    auto inputRange = grad.getProperty(kGradientPropertyInputRange);
    ASSERT_EQ(2, inputRange.size());
    ASSERT_EQ(0, inputRange.at(0).asNumber());
    ASSERT_EQ(1, inputRange.at(1).asNumber());

    ASSERT_EQ(0.5, grad.getProperty(kGradientPropertyCenterX).asNumber());
    ASSERT_EQ(0.5, grad.getProperty(kGradientPropertyCenterY).asNumber());
    ASSERT_EQ(0.7071, grad.getProperty(kGradientPropertyRadius).asNumber());
}

static const char *EASING = R"(
    {
      "type": "APL",
      "version": "1.4",
      "mainTemplate": {
        "items": {
          "type": "Text",
          "text": "${@jagged(0.25)}"
        }
      },
      "resources": [
        {
          "easings": {
            "jagged": "line(0.25,0.75) end(1,1) "
          }
        }
      ]
    }
)";

TEST_F(ResourceTest, Easing)
{
    loadDocument(EASING);
    auto object = context->opt("@jagged");
    ASSERT_TRUE(object.isEasing());

    auto easing = object.getEasing();
    ASSERT_EQ(0.75, easing->calc(0.25));

    ASSERT_TRUE(IsEqual("0.75", component->getCalculated(kPropertyText).asString()));
}
