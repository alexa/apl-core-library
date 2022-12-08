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
#include "test_sg.h"
#include "apl/scenegraph/builder.h"
#include "apl/scenegraph/pathparser.h"

using namespace apl;

class SGPathParserTest : public DocumentWrapper {};

struct PathTextCase {
    std::string source;
    std::string commands;
    std::vector<float> array;
};

static const std::vector<PathTextCase> PATH_TESTS = {
    {"M10,10 L20,20", "ML", {10, 10, 20, 20}},
    {"M5,10 20,30", "", {}}, // Fails - no path generated
    {"M5,10 m20 30", "", {}},
    {"m1 2 3 4 5 6", "", {}},
    {"M4,8 L10,12", "ML", {4, 8, 10, 12}},
    {"M4,8 l22 23", "ML", {4, 8, 26, 31}},
    {"M4,8 l22 23 -2 -2", "MLL", {4, 8, 26, 31, 24, 29}},
    {"H10 h20 v10 v20 30", "MLLLLL", {0, 0, 10, 0, 30, 0, 30, 10, 30, 30, 30, 60}},
    {"V20 40", "MLL", {0, 0, 0, 20, 0, 40}},
    {"M10,10 C20,0 20,20 0,20", "MC", {10, 10, 20, 0, 20, 20, 0, 20}},
    {"M10,10 c10,-10 10,10 -10,10", "MC", {10, 10, 20, 0, 20, 20, 0, 20}},
    {"M0,100 S50,0 100,100 S150,200 200,100 250,0 300,100 350,200 400,100", "MCCCC",
        {0, 100,   // Move
         0, 100, 50, 0, 100, 100,        // First control point is fixed to the move
         150, 200, 150, 200, 200, 100,   // Carry over the control point
         250, 0, 250, 0, 300, 100,       // Carry over the control point
         350, 200, 350, 200, 400, 100}}, // Carry over the control point
    // Same as the last case, only relative smooth curve
    {"M0,100 s50,-100 100,0 s50,100 100,0 50,-100 100,0 50,100 100,0", "MCCCC",
     {0, 100,   // Move
      0, 100, 50, 0, 100, 100,        // First control point is fixed to the move
      150, 200, 150, 200, 200, 100,   // Carry over the control point
      250, 0, 250, 0, 300, 100,       // Carry over the control point
      350, 200, 350, 200, 400, 100}}, // Carry over the control point
    // Quadratic Bezier curves; all four variations
    {"M0,100 Q100,0 200,100 300,200 400,100", "MQQ", {0, 100, 100, 0, 200, 100, 300, 200, 400, 100}},
    {"M0,100 Q100,0 200,100 T400,100", "MQQ", {0, 100, 100, 0, 200, 100, 300, 200, 400, 100}},
    {"M0,100 q100,-100 200,0 100,100 200,0", "MQQ", {0, 100, 100, 0, 200, 100, 300, 200, 400, 100}},
    {"M0,100 q100,-100 200,0 t200,0", "MQQ", {0, 100, 100, 0, 200, 100, 300, 200, 400, 100}},
    {"M0,100 T200,0", "MQ", {0, 100, 0, 100, 200, 0}}, // Smooth curve without prior curve
    {"M0,100 t200,0", "MQ", {0, 100, 0, 100, 200, 100}}, // Smooth curve without prior curve
    // Elliptical arcs
    {"M 300 200 A 100 100 0 0 1 500 200", "MCC", {300, 200,
                                                  300, 144.771545, 344.771515, 100, 400, 100,
                                                  455.228485, 100, 500, 144.771500, 500, 200}},
    {"M 300 200 a 100 100 0 0 1 200 0", "MCC", {300, 200,
                                                300, 144.771545, 344.771515, 100, 400, 100,
                                                455.228485, 100, 500, 144.771500, 500, 200}},
    {"A0,0 0 0 1 10,10", "ML", {0,0,10,10}}, // No radius -> straight line
    {"A10,10 0 0 1 0,0", "", {}}, // No distance -> no arc
    {"A10,10 0 0 1 10,10", "MC", {0,0,5.522847, 0, 10, 4.477152, 10, 10}},  // One quarter circle
    {"A10,10 0 1 1 10,10", "MCCC", {0,0,0, -5.522846, 4.477152, -10, 10, -10,
                                   15.522847, -10, 20, -5.522851, 20, 0,
                                   20, 5.522845, 15.522850, 10, 10, 10}},  // The other three-quarters
    {"A10,10 0 0 0 10,10", "MC", {0,0,0, 5.522847, 4.477152, 10, 10, 10}},  // Flip it
    {"A10,10 0 1 0 10,10", "MCCC", {0,0,-5.522848, 0, -10, 4.477153, -10, 10,
                                   -10, 15.522848, -5.522848, 20, 0, 20,
                                   5.522848, 20, 10, 15.522846, 10, 10}},  // Flipped three-quarters
    {"M 300 200 a 10 10 0 0 1 200 0", "MCC", {300, 200,
                                              300, 144.771545, 344.771515, 100, 400, 100,
                                              455.228485, 100, 500, 144.771500, 500, 200}}, // Arc too small
    {"A1000000, 10000000 0 0 1 1 1", "ML", {0, 0, 1, 1}},   // Flat angle; draw straight line
    // Closure
    {"M0,0 h10 v10 z", "MLLZ", { 0, 0, 10, 0, 10, 10 }},
    {"M0,0 h10 v10 Z", "MLLZ", { 0, 0, 10, 0, 10, 10 }},
    {"M0,0 h10 v10 Z Z Z", "MLLZ", { 0, 0, 10, 0, 10, 10 }},
    // Multiple moves followed by one line.  The multiple moves collapse
    {"M20,30 10,20 5,1 m20,20 3,3 M18,1 H5", "ML", { 18, 1, 5, 1}},
    {"L10,10 M20,20 30,30", "ML", {0,0,10,10}},
};

TEST_F(SGPathParserTest, Basic)
{
    for (const auto& m : PATH_TESTS) {
        auto path = sg::parsePathString(m.source);
        auto test = IsGeneralPath(m.commands, m.array);
        ASSERT_TRUE(test(path)) << m.source;
    }
}

TEST_F(SGPathParserTest, Error)
{
    auto path = sg::parsePathString("M10,10 L100,100 f13 L0,100");
    ASSERT_TRUE(path);  // A Path object comes back...but it is empty
    ASSERT_TRUE(path->empty());
}

static const char * VECTOR = R"apl(
    {
      "type": "APL",
      "version": "1.4",
      "graphics": {
        "arcs": {
          "type": "AVG",
          "version": "1.1",
          "description": "Arc sample from SVG standard",
          "width": 400,
          "height": 400,
          "items": [
            {
              "type": "group",
              "clipPath": "M0,200 L200,0 L400,200 L200,400 z",
              "items": {
                "type": "path",
                "stroke": "blue",
                "strokeWidth": 10,
                "fill": "red",
                "pathData": "M40,40 L360,40 360,360 40,360 z"
              }
            }
          ]
        }
      },
      "background": "white",
      "mainTemplate": {
        "items": {
          "type": "VectorGraphic",
          "source": "arcs",
          "width": "1024",
          "height": "800",
          "scale": "best-fit"
        }
      }
    }
)apl";

TEST_F(SGPathParserTest, Path)
{
    loadDocument(VECTOR);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(sg);

    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 1024, 800}, "..VectorGraphic")
            .child(IsLayer(Rect{112, 0, 800, 800}, "...Graphic")
                       .content(
                           IsTransformNode()
                               .transform(Transform2D::scale(2))
                               .child(
                                   IsOpacityNode().child(IsTransformNode().child(
                                       IsClipNode()
                                           .path(IsGeneralPath(
                                               "MLLLZ", {0, 200, 200, 0, 400, 200, 200, 400}))
                                           .child(IsDrawNode()
                                                      .path(IsGeneralPath(
                                                          "MLLLZ",
                                                          {40, 40, 360, 40, 360, 360, 40, 360}))
                                                      .pathOp(IsFillOp(IsColorPaint(Color::RED)))
                                                      .pathOp(IsStrokeOp(IsColorPaint(Color::BLUE),
                                                                         10))))))))));
}

static const char *PATTERN = R"apl(
    {
      "type": "APL",
      "version": "1.6",
      "graphics": {
        "BigSquare": {
          "type": "AVG",
          "version": "1.1",
          "width": 40,
          "height": 40,
          "resources": {
            "patterns": {
              "RedCircle": {
                "width": 8,
                "height": 8,
                "items": {
                  "type": "path",
                  "pathData": "M0,4 L4,0 L8,4 L4,8 z",
                  "fill": "red"
                }
              }
            }
          },
          "item": {
            "type": "path",
            "fill": "@RedCircle",
            "pathData": "M0,0 L40,0 L40,40 L0,40 z"
          }
        }
      },
      "background": "white",
      "mainTemplate": {
        "items": {
          "type": "VectorGraphic",
          "source": "BigSquare",
          "scale": "best-fit",
          "width": "800",
          "height": "800"
        }
      }
    }
)apl";

TEST_F(SGPathParserTest, Pattern)
{
    loadDocument(PATTERN);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(sg);

    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 800, 800}, "..VectorGraphic")
            .child(
                IsLayer(Rect{0, 0, 800, 800})
                    .content(
                        IsTransformNode()
                            .transform(Transform2D::scale(20))
                            .child(
                                IsDrawNode()
                                    .path(IsGeneralPath("MLLLZ", {0, 0, 40, 0, 40, 40, 0, 40}))
                                    .pathOp(IsFillOp(IsPatternPaint(
                                        {8, 8},
                                        IsDrawNode()
                                            .path(IsGeneralPath("MLLLZ", {0, 4, 4, 0, 8, 4, 4, 8}))
                                            .pathOp(IsFillOp(IsColorPaint(Color::RED)))))))))));
}

// TODO: Arc tests