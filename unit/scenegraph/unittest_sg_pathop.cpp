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
#include "test_sg.h"
#include "apl/scenegraph/builder.h"

#include <string.h>

using namespace apl;

class SGPathOpTest : public ::testing::Test {};

static const std::regex STROKE("Stroke width=([0-9.]+) "
                               "miterLimit=([0-9.]+) "
                               "pathLen=([0-9.]+) "
                               "dashOffset=([0-9.]+) "
                               "lineCap=(\\w+) "
                               "lineJoin=(\\w+) "
                               "dashes=\\[(.*?)\\]");

static const std::regex DASHES("([0-9.]+)?(,([0-9.]+))*");


TEST_F(SGPathOpTest, Fill)
{
    auto op = sg::fill(sg::paint(Color(Color::BLACK)));

    ASSERT_EQ(op->toDebugString(), "Fill");

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(op->serialize(doc.GetAllocator()),
                StringToMapObject(R"apl(
        {
            "type": "fill",
            "fillType": "even-odd",
            "paint": {
                "type": "colorPaint",
                "opacity": 1,
                "color": "#000000ff"
            }
        }
    )apl")));
}

TEST_F(SGPathOpTest, Stroke)
{
    auto op = sg::stroke(sg::paint(Color(Color::BLACK))).get();

    ASSERT_EQ(op->toDebugString(), "Stroke width=1.000000 miterLimit=4.000000 "
                                   "pathLen=0.000000 dashOffset=0.000000 "
                                   "lineCap=butt lineJoin=miter dashes=[]");

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(op->serialize(doc.GetAllocator()),
                        StringToMapObject(R"apl(
        {
            "type": "stroke",
            "width": 1,
            "miterLimit": 4,
            "pathLength": 0,
            "dashOffset": 0,
            "lineCap": "butt",
            "lineJoin": "miter",
            "paint": {
                "type": "colorPaint",
                "opacity": 1,
                "color": "#000000ff"
            }
        }
    )apl")));
}


TEST_F(SGPathOpTest, Fancy)
{
    auto op = sg::stroke(sg::paint(Color(Color::BLACK)))
                  .strokeWidth(10)
                  .miterLimit(8)
                  .dashOffset(2)
                  .dashes(ObjectArray{1,3})
                  .lineCap(GraphicLineCap::kGraphicLineCapRound)
                  .lineJoin(GraphicLineJoin::kGraphicLineJoinBevel)
                  .pathLength(100)
                  .get();

    ASSERT_EQ(op->toDebugString(), "Stroke width=10.000000 miterLimit=8.000000 "
                                   "pathLen=100.000000 dashOffset=2.000000 "
                                   "lineCap=round lineJoin=bevel dashes=[1.000000,3.000000]");

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(op->serialize(doc.GetAllocator()),
                        StringToMapObject(R"apl(
        {
            "type": "stroke",
            "width": 10,
            "miterLimit": 8,
            "pathLength": 100,
            "dashOffset": 2,
            "dashes": [ 1, 3],
            "lineCap": "round",
            "lineJoin": "bevel",
            "paint": {
                "type": "colorPaint",
                "opacity": 1,
                "color": "#000000ff"
            }
        }
    )apl")));
}