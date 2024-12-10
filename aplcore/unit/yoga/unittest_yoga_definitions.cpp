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

#include <yoga/YGNode.h>

#include "apl/yoga/yoganode.h"

using namespace apl;

class YogaDefinitionsTest : public ::testing::Test {
public:
    YogaDefinitionsTest() : node(config) {}

    YGNodeRef getYogaRef() const { return (YGNodeRef)node.get(); }

    YogaConfig config;
    YogaNode node;
};

// JUSTIFY_LOOKUP relies on int values being equal
TEST_F(YogaDefinitionsTest, JustifyLookup)
{
    node.setJustifyContent(kFlexboxJustifyContentStart);
    ASSERT_EQ(YGJustifyFlexStart, YGNodeStyleGetJustifyContent(getYogaRef()));

    node.setJustifyContent(kFlexboxJustifyContentEnd);
    ASSERT_EQ(YGJustifyFlexEnd, YGNodeStyleGetJustifyContent(getYogaRef()));

    node.setJustifyContent(kFlexboxJustifyContentCenter);
    ASSERT_EQ(YGJustifyCenter, YGNodeStyleGetJustifyContent(getYogaRef()));

    node.setJustifyContent(kFlexboxJustifyContentSpaceBetween);
    ASSERT_EQ(YGJustifySpaceBetween, YGNodeStyleGetJustifyContent(getYogaRef()));

    node.setJustifyContent(kFlexboxJustifyContentSpaceAround);
    ASSERT_EQ(YGJustifySpaceAround, YGNodeStyleGetJustifyContent(getYogaRef()));
}

// WRAP_LOOKUP relies on int values being equal
TEST_F(YogaDefinitionsTest, WrapLookup)
{
    node.setWrap(kFlexboxWrapNoWrap);
    ASSERT_EQ(YGWrapNoWrap, YGNodeStyleGetFlexWrap(getYogaRef()));

    node.setWrap(kFlexboxWrapWrap);
    ASSERT_EQ(YGWrapWrap, YGNodeStyleGetFlexWrap(getYogaRef()));

    node.setWrap(kFlexboxWrapWrapReverse);
    ASSERT_EQ(YGWrapWrapReverse, YGNodeStyleGetFlexWrap(getYogaRef()));
}

// ALIGN_LOOKUP relies on int values being equal
TEST_F(YogaDefinitionsTest, AlignLookup)
{
    node.setAlignItems(kFlexboxAlignStretch);
    ASSERT_EQ(YGAlignStretch, YGNodeStyleGetAlignItems(getYogaRef()));

    node.setAlignItems(kFlexboxAlignCenter);
    ASSERT_EQ(YGAlignCenter, YGNodeStyleGetAlignItems(getYogaRef()));

    node.setAlignItems(kFlexboxAlignStart);
    ASSERT_EQ(YGAlignFlexStart, YGNodeStyleGetAlignItems(getYogaRef()));

    node.setAlignItems(kFlexboxAlignEnd);
    ASSERT_EQ(YGAlignFlexEnd, YGNodeStyleGetAlignItems(getYogaRef()));

    node.setAlignItems(kFlexboxAlignBaseline);
    ASSERT_EQ(YGAlignBaseline, YGNodeStyleGetAlignItems(getYogaRef()));

    node.setAlignItems(kFlexboxAlignAuto);
    ASSERT_EQ(YGAlignAuto, YGNodeStyleGetAlignItems(getYogaRef()));
}

// EDGE_LOOKUP relies on int values being equal
TEST_F(YogaDefinitionsTest, EdgeLookup)
{
    node.setPadding(Edge::Left, 1);
    ASSERT_EQ(1, YGNodeStyleGetPadding(getYogaRef(), YGEdgeLeft).value);

    node.setPadding(Edge::Top, 2);
    ASSERT_EQ(2, YGNodeStyleGetPadding(getYogaRef(), YGEdgeTop).value);

    node.setPadding(Edge::Right, 3);
    ASSERT_EQ(3, YGNodeStyleGetPadding(getYogaRef(), YGEdgeRight).value);

    node.setPadding(Edge::Bottom, 4);
    ASSERT_EQ(4, YGNodeStyleGetPadding(getYogaRef(), YGEdgeBottom).value);

    node.setPadding(Edge::Start, 5);
    ASSERT_EQ(5, YGNodeStyleGetPadding(getYogaRef(), YGEdgeStart).value);

    node.setPadding(Edge::End, 6);
    ASSERT_EQ(6, YGNodeStyleGetPadding(getYogaRef(), YGEdgeEnd).value);

    node.setPadding(Edge::Horizontal, 7);
    ASSERT_EQ(7, YGNodeStyleGetPadding(getYogaRef(), YGEdgeHorizontal).value);

    node.setPadding(Edge::Vertical, 8);
    ASSERT_EQ(8, YGNodeStyleGetPadding(getYogaRef(), YGEdgeVertical).value);

    node.setPadding(Edge::All, 9);
    ASSERT_EQ(9, YGNodeStyleGetPadding(getYogaRef(), YGEdgeAll).value);
}
