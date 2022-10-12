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
    {"M10,10 L20,20",      "ML",    {10, 10, 20, 20}},
    {"M5,10 20,30",        "",      {}},     // Fails - no path generated
    {"M5,10 m20 30",       "",      {}},
    {"m1 2 3 4 5 6",       "",      {}},
    {"M4,8 L10,12",        "ML",    {4,  8,  10, 12}},
    {"M4,8 l22 23",        "ML",    {4,  8,  26, 31}},
    {"M4,8 l22 23 -2 -2",  "MLL",   {4,  8,  26, 31, 24, 29}},
    {"H10 h20 v10 v20 30", "LLLLL", {10, 0,  30, 0,  30, 10, 30, 30, 30, 60}},
};

TEST_F(SGPathParserTest, Basic)
{
    for (const auto& m : PATH_TESTS) {
        auto path = sg::parsePathString(m.source);
        auto test = IsGeneralPath(m.commands, m.array);
        ASSERT_TRUE(test(path)) << m.source;
    }
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
                               .child(IsGenericNode().child(
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
                                                                         10)))))))))));
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

TEST_F(SGPathParserTest, Pattern) {
    loadDocument(PATTERN);
    ASSERT_TRUE(component);

    auto sg = root->getSceneGraph();
    ASSERT_TRUE(sg);

    ASSERT_TRUE(CheckSceneGraph(
        sg,
        IsLayer(Rect{0, 0, 800, 800}, "..VectorGraphic")
            .child(
                IsLayer(Rect{0, 0, 800, 800})
                    .content(IsTransformNode()
                                 .transform(Transform2D::scale(20))
                                 .child(IsGenericNode().child(
                                     IsDrawNode()
                                         .path(IsGeneralPath("MLLLZ", {0, 0, 40, 0, 40, 40, 0, 40}))
                                         .pathOp(IsFillOp(IsPatternPaint(
                                             {8, 8}, IsGenericNode().child(
                                                         IsDrawNode()
                                                             .path(IsGeneralPath(
                                                                 "MLLLZ", {0, 4, 4, 0, 8, 4, 4, 8}))
                                                             .pathOp(IsFillOp(IsColorPaint(
                                                                 Color::RED)))))))))))));
}

// TODO: Arc tests