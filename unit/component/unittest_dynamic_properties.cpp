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

#include "rapidjson/document.h"

#include "../testeventloop.h"

#include "apl/component/component.h"

using namespace apl;

inline
::testing::AssertionResult
CheckProperties(const ComponentPtr& component, std::map<PropertyKey, Object> values) {
    for (const auto& m : values) {
        auto result = IsEqual(m.second, component->getCalculated(m.first));
        if (!result)
            return result << " on property " << sComponentPropertyBimap.at(m.first);
    }
    return ::testing::AssertionSuccess();
}

class DynamicPropertiesTest : public DocumentWrapper {};

static const char *HEIGHT_WIDTH_SETVALUE = R"apl(
{
    "type": "APL",
    "version": "1.6",
    "styles": {
        "base": {
            "values": [
                {
                    "height": 100,
                    "width": 100,
                    "maxHeight": 550,
                    "maxWidth": 200,
                    "minHeight": 10,
                    "minWidth": 10
                },
                {
                    "when": "${state.disabled}",
                    "height": 90,
                    "width": 90,
                    "maxHeight": 500,
                    "maxWidth": 150,
                    "minHeight": 5,
                    "minWidth": 5
                }
            ]
        }
    },
    "mainTemplate": {
        "item": {
            "type": "Container",
            "id": "c1",
            "height": 550,
            "width": 200,
            "items": [
                {
                    "type": "Frame",
                    "id": "frame1",
                    "style": "base"
                },
                {
                    "type": "Frame",
                    "id": "frame2",
                    "style": "base"
                },
                {
                    "type": "Frame",
                    "id": "frame3",
                    "style": "base"
                }
            ]
        }
    }
}
)apl";

// Test for base component height/width properties for styled
TEST_F(DynamicPropertiesTest, HeightWidthStyled)
{
    loadDocument(HEIGHT_WIDTH_SETVALUE);
    ASSERT_TRUE(component);

    auto frame1 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame1"));
    auto frame2 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame2"));
    auto frame3 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame3"));

    ASSERT_TRUE(frame1);
    ASSERT_EQ(kComponentTypeFrame, frame1->getType());
    ASSERT_TRUE(CheckProperties(frame1, {
            {kPropertyHeight, Dimension(100) },
            {kPropertyWidth, Dimension(100) },
            {kPropertyMaxHeight, Dimension(550) },
            {kPropertyMaxWidth, Dimension(200) },
            {kPropertyMinHeight, Dimension(10) },
            {kPropertyMinWidth, Dimension(10) },
            {kPropertyBounds, Rect(0, 0, 100, 100) },
    }));

    // disabling state to change style
    frame1->setState(StateProperty::kStateDisabled, true);
    ASSERT_TRUE(CheckDirty(root, component, frame1, frame2, frame3));
    ASSERT_TRUE(CheckProperties(frame1, {
            {kPropertyHeight, Dimension(90) },
            {kPropertyWidth, Dimension(90) },
            {kPropertyMaxHeight, Dimension(500) },
            {kPropertyMaxWidth, Dimension(150) },
            {kPropertyMinHeight, Dimension(5) },
            {kPropertyMinWidth, Dimension(5) },
            {kPropertyBounds, Rect(0, 0, 90, 90) },
    }));

    ASSERT_EQ(Rect(0, 90, 100, 100), frame2->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 190, 100, 100), frame3->getCalculated(kPropertyBounds).getRect());

    root->clearDirty();
}

// Test for base component height/width properties for dynamic
TEST_F(DynamicPropertiesTest, HeightWidthDynamic)
{
    loadDocument(HEIGHT_WIDTH_SETVALUE);
    ASSERT_TRUE(component);

    auto container = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("c1"));
    ASSERT_TRUE(container);
    ASSERT_EQ(kComponentTypeContainer, container->getType());
    ASSERT_TRUE(CheckProperties(container, {
        {kPropertyHeight, Dimension(550) },
        {kPropertyWidth, Dimension(200) },
        {kPropertyBounds, Rect(0, 0, 200, 550) },
    }));

    auto frame1 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame1"));
    ASSERT_TRUE(frame1);
    ASSERT_EQ(kComponentTypeFrame, frame1->getType());
    ASSERT_TRUE(CheckProperties(frame1, {
            {kPropertyHeight, Dimension(100) },
            {kPropertyWidth, Dimension(100) },
            {kPropertyBounds, Rect(0, 0, 100, 100) },
    }));

    auto frame2 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame2"));
    ASSERT_TRUE(frame2);
    ASSERT_EQ(kComponentTypeFrame, frame2->getType());
    ASSERT_TRUE(CheckProperties(frame2, {
            {kPropertyHeight, Dimension(100) },
            {kPropertyWidth, Dimension(100) },
            {kPropertyBounds, Rect(0, 100, 100, 100) },
    }));

    auto frame3 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame3"));
    ASSERT_TRUE(frame3);
    ASSERT_EQ(kComponentTypeFrame, frame3->getType());
    ASSERT_TRUE(CheckProperties(frame3, {
            {kPropertyHeight, Dimension(100) },
            {kPropertyWidth, Dimension(100) },
            {kPropertyBounds, Rect(0, 200, 100, 100) },
    }));

    // Set height property of frame1, it will impact frame2 and 3 also
    frame1->setProperty(kPropertyHeight, 400);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(frame1, kPropertyBounds, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(frame2, kPropertyBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(frame3, kPropertyBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, component, frame1, frame2, frame3));
    root->clearDirty();

    ASSERT_EQ(Rect(0, 0, 200, 550), container->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 0, 100, 400), frame1->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 400, 100, 100), frame2->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 500, 100, 100), frame3->getCalculated(kPropertyBounds).getRect());

    // Set width property of frame1, it will impact only frame1
    frame1->setProperty(kPropertyWidth, 150);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(frame1, kPropertyBounds, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, component, frame1));
    root->clearDirty();

    ASSERT_EQ(Rect(0, 0, 200, 550), container->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 0, 150, 400), frame1->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 400, 100, 100), frame2->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 500, 100, 100), frame3->getCalculated(kPropertyBounds).getRect());
}

// Test for base component min/max height/width properties for dynamic
TEST_F(DynamicPropertiesTest, MinMaxHeightWidth)
{
    loadDocument(HEIGHT_WIDTH_SETVALUE);
    ASSERT_TRUE(component);

    auto container = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("c1"));
    ASSERT_TRUE(container);
    ASSERT_EQ(kComponentTypeContainer, container->getType());
    ASSERT_EQ(Rect(0, 0, 200, 550), container->getCalculated(kPropertyBounds).getRect());

    auto frame1 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame1"));
    ASSERT_TRUE(frame1);
    ASSERT_EQ(kComponentTypeFrame, frame1->getType());
    ASSERT_TRUE(CheckProperties(frame1, {
            {kPropertyMaxHeight, Dimension(550) },
            {kPropertyMaxWidth, Dimension(200) },
            {kPropertyMinHeight, Dimension(10) },
            {kPropertyMinWidth, Dimension(10) },
            {kPropertyBounds, Rect(0, 0, 100, 100) },
    }));

    auto frame2 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame2"));
    ASSERT_TRUE(frame2);
    ASSERT_EQ(kComponentTypeFrame, frame2->getType());
    ASSERT_TRUE(CheckProperties(frame2, {
            {kPropertyMaxHeight, Dimension(550) },
            {kPropertyMaxWidth, Dimension(200) },
            {kPropertyMinHeight, Dimension(10) },
            {kPropertyMinWidth, Dimension(10) },
            {kPropertyBounds, Rect(0, 100, 100, 100) },
    }));

    auto frame3 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame3"));
    ASSERT_TRUE(frame3);
    ASSERT_EQ(kComponentTypeFrame, frame3->getType());
    ASSERT_TRUE(CheckProperties(frame3, {
            {kPropertyMaxHeight, Dimension(550) },
            {kPropertyMaxWidth, Dimension(200) },
            {kPropertyMinHeight, Dimension(10) },
            {kPropertyMinWidth, Dimension(10) },
            {kPropertyBounds, Rect(0, 200, 100, 100) },
    }));

    // Set maxHeight property of frame1, it will impact frame2 and 3 also
    frame1->setProperty(kPropertyMaxHeight, 90);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(frame1, kPropertyBounds, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(frame2, kPropertyBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(frame3, kPropertyBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, component, frame1, frame2, frame3));
    ASSERT_EQ(Object(Dimension(90)), frame1->getCalculated(kPropertyMaxHeight));
    root->clearDirty();

    ASSERT_EQ(Rect(0, 0, 200, 550), container->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 0, 100, 90), frame1->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 90, 100, 100), frame2->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 190, 100, 100), frame3->getCalculated(kPropertyBounds).getRect());

    // Set maxWidth property of frame1, it will not impact any component
    frame1->setProperty(kPropertyMaxWidth, 150);

    ASSERT_EQ(0, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(frame1)); // No property is dirty
    ASSERT_TRUE(CheckDirty(root));
    ASSERT_EQ(Object(Dimension(150)), frame1->getCalculated(kPropertyMaxWidth));
    root->clearDirty();

    ASSERT_EQ(Rect(0, 0, 200, 550), container->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 0, 100, 90), frame1->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 90, 100, 100), frame2->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 190, 100, 100), frame3->getCalculated(kPropertyBounds).getRect());

    // Set maxWidth property of frame1 to lower than width, it will impact only frame1
    frame1->setProperty(kPropertyMaxWidth, 90);

    root->clearPending();
    ASSERT_TRUE(CheckDirty(frame1, kPropertyBounds, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, component, frame1));
    ASSERT_EQ(Object(Dimension(90)), frame1->getCalculated(kPropertyMaxWidth));
    root->clearDirty();

    ASSERT_EQ(Rect(0, 0, 200, 550), container->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 0, 90, 90), frame1->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 90, 100, 100), frame2->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 190, 100, 100), frame3->getCalculated(kPropertyBounds).getRect());

    // Set minHeight property of frame2, it will impact frame3 also
    frame2->setProperty(kPropertyMinHeight, 125);

    root->clearPending();
    ASSERT_TRUE(CheckDirty(frame2, kPropertyBounds, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(frame3, kPropertyBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, component, frame2, frame3));
    ASSERT_EQ(Object(Dimension(125)), frame2->getCalculated(kPropertyMinHeight));
    root->clearDirty();

    ASSERT_EQ(Rect(0, 0, 200, 550), container->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 0, 90, 90), frame1->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 90, 100, 125), frame2->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 215, 100, 100), frame3->getCalculated(kPropertyBounds).getRect());

    // Set minWidth property of frame2, it will not impact any component
    frame2->setProperty(kPropertyMinWidth, 50);

    root->clearPending();
    ASSERT_TRUE(CheckDirty(frame2)); // No property is dirty
    ASSERT_TRUE(CheckDirty(root));
    ASSERT_EQ(Object(Dimension(50)), frame2->getCalculated(kPropertyMinWidth));
    root->clearDirty();

    ASSERT_EQ(Rect(0, 0, 200, 550), container->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 0, 90, 90), frame1->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 90, 100, 125), frame2->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 215, 100, 100), frame3->getCalculated(kPropertyBounds).getRect());

    // Set minWidth property of frame2 to higher than width, it will impact only frame2
    frame2->setProperty(kPropertyMinWidth, 125);

    root->clearPending();
    ASSERT_TRUE(CheckDirty(frame2, kPropertyBounds, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, component, frame2));
    ASSERT_EQ(Object(Dimension(125)), frame2->getCalculated(kPropertyMinWidth));
    root->clearDirty();

    ASSERT_EQ(Rect(0, 0, 200, 550), container->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 0, 90, 90), frame1->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 90, 125, 125), frame2->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 215, 100, 100), frame3->getCalculated(kPropertyBounds).getRect());
}

// Test for base component shadow* properties for dynamic
TEST_F(DynamicPropertiesTest, ShadowProperties)
{
    loadDocument(HEIGHT_WIDTH_SETVALUE);
    ASSERT_TRUE(component);

    auto frame2 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame2"));
    ASSERT_TRUE(frame2);
    ASSERT_EQ(kComponentTypeFrame, frame2->getType());

    // Set shadowColor property of frame2
    ASSERT_EQ(Object(Color(Color::TRANSPARENT)), frame2->getCalculated(kPropertyShadowColor));
    frame2->setProperty(kPropertyShadowColor, Color::BLUE);

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(frame2, kPropertyShadowColor));
    ASSERT_TRUE(CheckDirty(root, frame2));
    ASSERT_EQ(Object(Color(Color::BLUE)), frame2->getCalculated(kPropertyShadowColor));
    root->clearDirty();

    // Set shadowHorizontalOffset property of frame2
    ASSERT_EQ(Object(Dimension(0)), frame2->getCalculated(kPropertyShadowHorizontalOffset));
    frame2->setProperty(kPropertyShadowHorizontalOffset, 5);

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(frame2, kPropertyShadowHorizontalOffset));
    ASSERT_TRUE(CheckDirty(root, frame2));
    ASSERT_EQ(Object(Dimension(5)), frame2->getCalculated(kPropertyShadowHorizontalOffset));
    root->clearDirty();

    // Set shadowRadius property of frame2
    ASSERT_EQ(Object(Dimension(0)), frame2->getCalculated(kPropertyShadowRadius));
    frame2->setProperty(kPropertyShadowRadius, 10);

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(frame2, kPropertyShadowRadius));
    ASSERT_TRUE(CheckDirty(root, frame2));
    ASSERT_EQ(Object(Dimension(10)), frame2->getCalculated(kPropertyShadowRadius));
    root->clearDirty();

    // Set shadowVerticalOffset property of frame2
    ASSERT_EQ(Object(Dimension(0)), frame2->getCalculated(kPropertyShadowVerticalOffset));
    frame2->setProperty(kPropertyShadowVerticalOffset, 4);

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(frame2, kPropertyShadowVerticalOffset));
    ASSERT_TRUE(CheckDirty(root, frame2));
    ASSERT_EQ(Object(Dimension(4)), frame2->getCalculated(kPropertyShadowVerticalOffset));
    root->clearDirty();
}

static const char *LAYOUTDIRCTION_SETVALUE = R"apl(
{
    "type": "APL",
    "version": "1.7",
    "styles": {
        "base1": {
            "values": [
                {
                    "layoutDirection": "LTR"
                },
                {
                    "when": "${state.disabled}",
                    "layoutDirection": "RTL"
                }
            ]
        }
    },
    "mainTemplate": {
        "item": {
            "type": "Container",
            "id": "c1",
            "height": 400,
            "width": 500,
            "style": "base1",
            "items": [
                {
                    "type": "Frame",
                    "height": 100,
                    "width": 200,
                    "id": "frame1",
                    "backgroundColor": "red"
                },
                {
                    "type": "Frame",
                    "height": 100,
                    "width": 200,
                    "id": "frame2",
                    "alignSelf": "center",
                    "backgroundColor": "red",
                    "items": [
                        {
                            "type": "Frame",
                            "height": 100,
                            "width": 100,
                            "id": "frame3",
                            "backgroundColor": "blue"
                        }
                    ]
                }
            ]
        }
    }
}
)apl";

TEST_F(DynamicPropertiesTest, LayoutDirectionPropertyStyled)
{
    loadDocument(LAYOUTDIRCTION_SETVALUE);
    ASSERT_TRUE(component);
    // Given a container with layoutDirection as LTR
    auto container = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("c1"));
    ASSERT_TRUE(container);
    ASSERT_EQ(kComponentTypeContainer, container->getType());
    ASSERT_EQ(Object(kLayoutDirectionLTR), container->getCalculated(kPropertyLayoutDirection));
    // and the frame1 displays at top-left.
    auto frame1 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame1"));
    ASSERT_TRUE(CheckProperties(frame1, {
        {kPropertyLayoutDirection, Object(kLayoutDirectionLTR)},
        {kPropertyBounds, Rect(0, 0, 200, 100) },
        {kPropertyInnerBounds, Rect(0, 0, 200, 100) },
    }));
    // and the frame2 displays at center.
    auto frame2 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame2"));
    ASSERT_TRUE(CheckProperties(frame2, {
        {kPropertyLayoutDirection, Object(kLayoutDirectionLTR)},
        {kPropertyBounds, Rect(150, 100, 200, 100) },
        {kPropertyInnerBounds, Rect(0, 0, 200, 100) },
    }));
    // and the frame3 displays at top-left of frame2.
    auto frame3 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame3"));
    ASSERT_TRUE(CheckProperties(frame3, {
        {kPropertyLayoutDirection, Object(kLayoutDirectionLTR)},
        {kPropertyBounds, Rect(0, 0, 100, 100) },
        {kPropertyInnerBounds, Rect(0, 0, 100, 100) },
    }));

    // When update the container style, the layoutDirection is also updated to RTL.
    executeCommand("SetValue", {{"componentId", container->getUniqueId()},
                                {"property", "disabled"},
                                {"value", true}}, true);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(container, kPropertyDisabled, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(frame1, kPropertyBounds, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(frame2, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(frame3, kPropertyBounds, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged));

    // Then calculated layoutDirection is RTL.
    ASSERT_EQ(Object(kLayoutDirectionRTL), container->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(Object(kLayoutDirectionRTL), frame1->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(Object(kLayoutDirectionRTL), frame2->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(Object(kLayoutDirectionRTL), frame3->getCalculated(kPropertyLayoutDirection));
    // and the frame1 displays at top-right.
    ASSERT_TRUE(CheckProperties(frame1, {
        {kPropertyBounds, Rect(300, 0, 200, 100) },
        {kPropertyInnerBounds, Rect(0, 0, 200, 100) },
    }));
    // frame2 still displays at center
    ASSERT_TRUE(CheckProperties(frame2, {
        {kPropertyBounds, Rect(150, 100, 200, 100) },
        {kPropertyInnerBounds, Rect(0, 0, 200, 100) },
    }));
    // frame3 displays at top-right of frame 2
    ASSERT_TRUE(CheckProperties(frame3, {
        {kPropertyBounds, Rect(100, 0, 100, 100) },
        {kPropertyInnerBounds, Rect(0, 0, 100, 100) },
    }));
}

TEST_F(DynamicPropertiesTest, LayoutDirectionPropertyDynamic)
{
    loadDocument(LAYOUTDIRCTION_SETVALUE);
    ASSERT_TRUE(component);
    // Given a container with layoutDirection as LTR
    auto container = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("c1"));
    ASSERT_TRUE(container);
    ASSERT_EQ(kComponentTypeContainer, container->getType());
    ASSERT_EQ(Object(kLayoutDirectionLTR), container->getCalculated(kPropertyLayoutDirection));
    // and the frame1 displays at top-left.
    auto frame1 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame1"));
    ASSERT_TRUE(CheckProperties(frame1, {
        {kPropertyLayoutDirection, Object(kLayoutDirectionLTR)},
        {kPropertyBounds, Rect(0, 0, 200, 100) },
        {kPropertyInnerBounds, Rect(0, 0, 200, 100) },
    }));
    // and the frame2 displays at center.
    auto frame2 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame2"));
    ASSERT_TRUE(CheckProperties(frame2, {
        {kPropertyLayoutDirection, Object(kLayoutDirectionLTR)},
        {kPropertyBounds, Rect(150, 100, 200, 100) },
        {kPropertyInnerBounds, Rect(0, 0, 200, 100) },
    }));
    // and the frame3 displays at top-left of frame2.
    auto frame3 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame3"));
    ASSERT_TRUE(CheckProperties(frame3, {
        {kPropertyLayoutDirection, Object(kLayoutDirectionLTR)},
        {kPropertyBounds, Rect(0, 0, 100, 100) },
        {kPropertyInnerBounds, Rect(0, 0, 100, 100) },
    }));

    // If set layoutDirection to same value, should not set dirty
    executeCommand("SetValue", {{"componentId", container->getUniqueId()},
                                {"property", "layoutDirection"},
                                {"value", "LTR"}}, true);
    ASSERT_TRUE(CheckDirty(root));
    ASSERT_EQ(Object(kLayoutDirectionLTR), container->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(Object(kLayoutDirectionLTR), frame1->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(Object(kLayoutDirectionLTR), frame2->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(Object(kLayoutDirectionLTR), frame3->getCalculated(kPropertyLayoutDirection));

    // When test dynamic property for layoutDirection by set to RTL
    executeCommand("SetValue", {{"componentId", container->getUniqueId()},
                                {"property", "layoutDirection"},
                                {"value", "RTL"}}, true);
    root->clearPending();
    ASSERT_TRUE(CheckDirty(container, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(frame1, kPropertyBounds, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(frame2, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(frame3, kPropertyBounds, kPropertyLayoutDirection, kPropertyNotifyChildrenChanged));

    // Then calculated layoutDirection is RTL.
    ASSERT_EQ(Object(kLayoutDirectionRTL), container->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(Object(kLayoutDirectionRTL), frame1->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(Object(kLayoutDirectionRTL), frame2->getCalculated(kPropertyLayoutDirection));
    ASSERT_EQ(Object(kLayoutDirectionRTL), frame3->getCalculated(kPropertyLayoutDirection));
    // and the frame1 displays at top-right.
    ASSERT_TRUE(CheckProperties(frame1, {
        {kPropertyBounds, Rect(300, 0, 200, 100) },
        {kPropertyInnerBounds, Rect(0, 0, 200, 100) },
    }));
    // frame2 still displays at center
    ASSERT_TRUE(CheckProperties(frame2, {
        {kPropertyBounds, Rect(150, 100, 200, 100) },
        {kPropertyInnerBounds, Rect(0, 0, 200, 100) },
    }));
    // frame3 displays at top-right of frame 2
    ASSERT_TRUE(CheckProperties(frame3, {
        {kPropertyBounds, Rect(100, 0, 100, 100) },
        {kPropertyInnerBounds, Rect(0, 0, 100, 100) },
    }));
}

static const char *PADDING_SETVALUE = R"apl(
{
    "type": "APL",
    "version": "1.6",
    "styles": {
        "base1": {
            "values": [
                {
                    "height": 100,
                    "width": 200,
                    "padding": 10
                },
                {
                    "when": "${state.disabled}",
                    "padding": 5
                }
            ]
        },
        "base2": {
            "values": [
                {
                    "height": 100,
                    "width": 200,
                    "paddingBottom": 5,
                    "paddingLeft": 5,
                    "paddingRight": 5,
                    "paddingTop": 5
                },
                {
                    "when": "${state.disabled}",
                    "paddingBottom": 10,
                    "paddingLeft": 10,
                    "paddingRight": 10,
                    "paddingTop": 10
                }
            ]
        }
    },
    "mainTemplate": {
        "item": {
            "type": "Container",
            "id": "c1",
            "height": 400,
            "width": 500,
            "items": [
                {
                    "type": "Frame",
                    "id": "frame1",
                    "style": "base1"
                },
                {
                    "type": "Frame",
                    "id": "frame2",
                    "style": "base2"
                }
            ]
        }
    }
}
)apl";

// Test for base component padding* properties for styled
TEST_F(DynamicPropertiesTest, PaddingStyled)
{
    loadDocument(PADDING_SETVALUE);
    ASSERT_TRUE(component);

    auto frame1 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame1"));
    ASSERT_TRUE(frame1);
    ASSERT_EQ(kComponentTypeFrame, frame1->getType());


    auto frame2 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame2"));
    ASSERT_TRUE(CheckProperties(frame2, {
            {kPropertyPaddingBottom, Dimension(5) },
            {kPropertyPaddingLeft, Dimension(5) },
            {kPropertyPaddingRight, Dimension(5) },
            {kPropertyPaddingTop, Dimension(5) },
            {kPropertyBounds, Rect(0, 100, 200, 100) },
            {kPropertyInnerBounds, Rect(5, 5, 190, 90) },
    }));

    // disabling state of frame1 to change style
    frame1->setState(StateProperty::kStateDisabled, true);
    ASSERT_TRUE(CheckDirty(root, frame1));
    ASSERT_TRUE(CheckProperties(frame1, {
            {kPropertyPadding, ObjectArray{Dimension(5), Dimension(5), Dimension(5), Dimension(5)} },
            {kPropertyBounds, Rect(0, 0, 200, 100) },
            {kPropertyInnerBounds, Rect(5, 5, 190, 90) },
    }));

    root->clearDirty();

    // disabling state of frame2 to change style
    frame2->setState(StateProperty::kStateDisabled, true);
    ASSERT_TRUE(CheckDirty(root, frame2));
    ASSERT_TRUE(CheckProperties(frame2, {
            {kPropertyPaddingBottom, Dimension(10) },
            {kPropertyPaddingLeft, Dimension(10) },
            {kPropertyPaddingRight, Dimension(10) },
            {kPropertyPaddingTop, Dimension(10) },
            {kPropertyBounds, Rect(0, 100, 200, 100) },
            {kPropertyInnerBounds, Rect(10, 10, 180, 80) },
    }));

    root->clearDirty();
}

// Test for base component padding* properties for dynamic
TEST_F(DynamicPropertiesTest, PaddingDynamic) {
    loadDocument(PADDING_SETVALUE);
    ASSERT_TRUE(component);

    auto container = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("c1"));
    ASSERT_TRUE(container);
    ASSERT_EQ(kComponentTypeContainer, container->getType());
    ASSERT_TRUE(CheckProperties(container, {
            {kPropertyPaddingBottom, Object::NULL_OBJECT() },
            {kPropertyPaddingLeft, Object::NULL_OBJECT() },
            {kPropertyPaddingRight, Object::NULL_OBJECT() },
            {kPropertyPaddingTop, Object::NULL_OBJECT() },
            {kPropertyPadding, ObjectArray{} },
            {kPropertyBounds, Rect(0, 0, 500, 400) },
    }));

    auto frame1 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame1"));
    ASSERT_TRUE(frame1);
    ASSERT_EQ(kComponentTypeFrame, frame1->getType());
    ASSERT_TRUE(CheckProperties(frame1, {
            {kPropertyPadding, ObjectArray{Dimension(10), Dimension(10), Dimension(10), Dimension(10)} },
            {kPropertyBounds, Rect(0, 0, 200, 100) },
            {kPropertyInnerBounds, Rect(10, 10, 180, 80) },
    }));

    auto frame2 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame2"));
    ASSERT_TRUE(frame2);
    ASSERT_EQ(kComponentTypeFrame, frame2->getType());
    ASSERT_TRUE(CheckProperties(frame2, {
            {kPropertyPaddingBottom, Dimension(5) },
            {kPropertyPaddingLeft, Dimension(5) },
            {kPropertyPaddingRight, Dimension(5) },
            {kPropertyPaddingTop, Dimension(5) },
            {kPropertyBounds, Rect(0, 100, 200, 100) },
            {kPropertyInnerBounds, Rect(5, 5, 190, 90) },
    }));

    // Set padding property of frame1
    frame1->setProperty(kPropertyPadding, Object(ObjectArray{15, 5}));

    root->clearPending();
    ASSERT_TRUE(CheckDirty(frame1, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, frame1));
    root->clearDirty();

    ASSERT_TRUE(CheckProperties(frame1, {
            {kPropertyBounds, Rect(0, 0, 200, 100) },
            {kPropertyInnerBounds, Rect(15, 5, 170, 90) },
    }));

    // Set paddingBottom property of frame2
    frame2->setProperty(kPropertyPaddingBottom, 10);

    root->clearPending();
    ASSERT_TRUE(CheckDirty(frame2, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, frame2));
    root->clearDirty();

    ASSERT_TRUE(CheckProperties(frame2, {
            {kPropertyBounds, Rect(0, 100, 200, 100) },
            {kPropertyInnerBounds, Rect(5, 5, 190, 85) },
    }));

    // Set paddingLeft property of frame2
    frame2->setProperty(kPropertyPaddingLeft, 10);

    root->clearPending();
    ASSERT_TRUE(CheckDirty(frame2, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, frame2));
    root->clearDirty();

    ASSERT_TRUE(CheckProperties(frame2, {
            {kPropertyBounds, Rect(0, 100, 200, 100) },
            {kPropertyInnerBounds, Rect(10, 5, 185, 85) },
    }));

    // Set paddingRight property of frame2
    frame2->setProperty(kPropertyPaddingRight, 10);

    root->clearPending();
    ASSERT_TRUE(CheckDirty(frame2, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, frame2));
    root->clearDirty();

    ASSERT_TRUE(CheckProperties(frame2, {
            {kPropertyBounds, Rect(0, 100, 200, 100) },
            {kPropertyInnerBounds, Rect(10, 5, 180, 85) },
    }));

    // Set paddingTop property of frame2
    frame2->setProperty(kPropertyPaddingTop, 10);

    root->clearPending();
    ASSERT_TRUE(CheckDirty(frame2, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, frame2));
    root->clearDirty();

    ASSERT_TRUE(CheckProperties(frame2, {
            {kPropertyBounds, Rect(0, 100, 200, 100) },
            {kPropertyInnerBounds, Rect(10, 10, 180, 80) },
    }));
}

// Test for frame component borderWidth properties for dynamic
TEST_F(DynamicPropertiesTest, BorderWidth) {
    loadDocument(HEIGHT_WIDTH_SETVALUE);
    ASSERT_TRUE(component);

    auto frame1 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame1"));
    ASSERT_TRUE(frame1);
    ASSERT_EQ(kComponentTypeFrame, frame1->getType());
    ASSERT_TRUE(CheckProperties(frame1, {
            {kPropertyBorderWidth, Dimension(0) },
            {kPropertyBounds, Rect(0, 0, 100, 100) },
            {kPropertyInnerBounds, Rect(0, 0, 100, 100) },
    }));

    // Set borderWidth property of frame1
    frame1->setProperty(kPropertyBorderWidth, 10);

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(frame1, kPropertyBorderWidth, kPropertyInnerBounds, kPropertyNotifyChildrenChanged));
    ASSERT_TRUE(CheckDirty(root, frame1));
    root->clearDirty();

    ASSERT_TRUE(CheckProperties(frame1, {
            {kPropertyBorderWidth, Dimension(10) },
            {kPropertyBounds, Rect(0, 0, 100, 100) },
            {kPropertyInnerBounds, Rect(10, 10, 80, 80) },
    }));
}

// Test for frame component borderRadius properties for dynamic
TEST_F(DynamicPropertiesTest, BorderRadius) {
    loadDocument(HEIGHT_WIDTH_SETVALUE);
    ASSERT_TRUE(component);

    auto frame1 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame1"));
    ASSERT_TRUE(frame1);
    ASSERT_EQ(kComponentTypeFrame, frame1->getType());
    ASSERT_TRUE(CheckProperties(frame1, {
            {kPropertyBorderRadius, Dimension(0) },
            {kPropertyBorderRadii, Radii(0) },
    }));

    // Set borderRadius property of frame1
    frame1->setProperty(kPropertyBorderRadius, 10);

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(frame1, kPropertyBorderRadii));
    ASSERT_TRUE(CheckDirty(root, frame1));
    root->clearDirty();

    ASSERT_TRUE(CheckProperties(frame1, {
            {kPropertyBorderRadius, Dimension(10) },
            {kPropertyBorderRadii, Radii(10) },
    }));
}

// Test for frame component border*Radius properties for dynamic
TEST_F(DynamicPropertiesTest, BorderAnyRadius) {
    loadDocument(HEIGHT_WIDTH_SETVALUE);
    ASSERT_TRUE(component);

    auto frame1 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("frame1"));
    ASSERT_TRUE(frame1);
    ASSERT_EQ(kComponentTypeFrame, frame1->getType());
    ASSERT_TRUE(CheckProperties(frame1, {
            {kPropertyBorderBottomLeftRadius, Object::NULL_OBJECT() },
            {kPropertyBorderBottomRightRadius, Object::NULL_OBJECT() },
            {kPropertyBorderTopLeftRadius, Object::NULL_OBJECT() },
            {kPropertyBorderTopRightRadius, Object::NULL_OBJECT() },
            {kPropertyBorderRadii, Radii(0) },
    }));

    // Set borderBottomLeftRadius property of frame1
    frame1->setProperty(kPropertyBorderBottomLeftRadius, 10);

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(frame1, kPropertyBorderRadii));
    ASSERT_TRUE(CheckDirty(root, frame1));
    root->clearDirty();

    ASSERT_TRUE(CheckProperties(frame1, {
            {kPropertyBorderBottomLeftRadius, Dimension(10) },
            {kPropertyBorderRadii, Radii(0, 0, 10, 0) },
    }));

    // Set borderBottomRightRadius property of frame1
    frame1->setProperty(kPropertyBorderBottomRightRadius, 10);

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(frame1, kPropertyBorderRadii));
    ASSERT_TRUE(CheckDirty(root, frame1));
    root->clearDirty();

    ASSERT_TRUE(CheckProperties(frame1, {
            {kPropertyBorderBottomRightRadius, Dimension(10) },
            {kPropertyBorderRadii, Radii(0, 0, 10, 10) },
    }));

    // Set borderTopLeftRadius property of frame1
    frame1->setProperty(kPropertyBorderTopLeftRadius, 10);

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(frame1, kPropertyBorderRadii));
    ASSERT_TRUE(CheckDirty(root, frame1));
    root->clearDirty();

    ASSERT_TRUE(CheckProperties(frame1, {
            {kPropertyBorderTopLeftRadius, Dimension(10) },
            {kPropertyBorderRadii, Radii(10, 0, 10, 10) },
    }));

    // Set borderTopRightRadius property of frame1
    frame1->setProperty(kPropertyBorderTopRightRadius, 10);

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(frame1, kPropertyBorderRadii));
    ASSERT_TRUE(CheckDirty(root, frame1));
    root->clearDirty();

    ASSERT_TRUE(CheckProperties(frame1, {
            {kPropertyBorderTopRightRadius, Dimension(10) },
            {kPropertyBorderRadii, Radii(10, 10, 10, 10) },
    }));
}

static const char *IMAGE_SETVALUE = R"apl(
{
    "type": "APL",
    "version": "1.6",
    "mainTemplate": {
        "item": {
            "type": "Container",
            "items": [
                {
                    "type": "Image",
                    "id": "img1",
                    "source": "https://images.amazon.com/image/foo.png",
                    "align": "center",
                    "borderRadius": 5,
                    "overlayGradient": {
                        "colorRange": [
                            "blue",
                            "red"
                        ]
                    },
                    "scale": "fill"
                },
                {
                    "type": "Image",
                    "id": "img2",
                    "source": "https://images.amazon.com/image/bar.png",
                    "overlayGradient": {
                        "colorRange": [
                            "green",
                            "gray"
                        ]
                    }
                }
            ]
        }
    }
}
)apl";

// Test for image component align/borderRadius/overlayGradient/scale properties for dynamic
TEST_F(DynamicPropertiesTest, ImageProperties) {
    loadDocument(IMAGE_SETVALUE);
    ASSERT_TRUE(component);

    auto img1 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("img1"));
    ASSERT_TRUE(img1);
    ASSERT_EQ(kComponentTypeImage, img1->getType());
    ASSERT_TRUE(CheckProperties(img1, {
            {kPropertyAlign, kImageAlignCenter },
            {kPropertyScale, kImageScaleFill },
            {kPropertyBorderRadius, Dimension(5) },
            {kPropertySource, "https://images.amazon.com/image/foo.png" },
    }));

    auto grad1 = img1->getCalculated(kPropertyOverlayGradient);
    ASSERT_TRUE(grad1.isGradient());
    ASSERT_EQ(Object(Color(Color::BLUE)), grad1.getGradient().getColorRange().at(0));
    ASSERT_EQ(Object(Color(Color::RED)), grad1.getGradient().getColorRange().at(1));

    // Set aline property of img
    img1->setProperty(kPropertyAlign, "left");

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(img1, kPropertyAlign));
    ASSERT_TRUE(CheckDirty(root, img1));
    root->clearDirty();

    ASSERT_EQ(kImageAlignLeft, img1->getCalculated(kPropertyAlign).getInteger());

    // Set borderRadius property of img
    img1->setProperty(kPropertyBorderRadius, 10);

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(img1, kPropertyBorderRadius));
    ASSERT_TRUE(CheckDirty(root, img1));
    root->clearDirty();

    ASSERT_EQ(Object(Dimension(10)), img1->getCalculated(kPropertyBorderRadius));

    // Set scale property of img
    img1->setProperty(kPropertyScale, "best-fill");

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(img1, kPropertyScale));
    ASSERT_TRUE(CheckDirty(root, img1));
    root->clearDirty();

    ASSERT_EQ(kImageScaleBestFill, img1->getCalculated(kPropertyScale).getInteger());

    // Set overlayGradient property of img
    auto img2 = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("img2"));
    auto grad2 = img2->getCalculated(kPropertyOverlayGradient);

    img1->setProperty(kPropertyOverlayGradient, Object(grad2));

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(img1, kPropertyOverlayGradient));
    ASSERT_TRUE(CheckDirty(root, img1));
    root->clearDirty();

    grad1 = img1->getCalculated(kPropertyOverlayGradient);
    ASSERT_TRUE(grad1.isGradient());
    ASSERT_EQ(Object(Color(Color::GREEN)), grad1.getGradient().getColorRange().at(0));
    ASSERT_EQ(Object(Color(Color::GRAY)), grad1.getGradient().getColorRange().at(1));
}

static const char *VECTOR_GRAPHIC_SETVALUE = R"apl(
{
    "type": "APL",
    "version": "1.6",
    "graphics": {
        "box": {
            "type": "AVG",
            "version": "1.2",
            "height": 100,
            "width": 100,
            "items": {
                "type": "text",
                "text": "Hello"
            }
        }
    },
    "mainTemplate": {
        "items": {
            "type": "VectorGraphic",
            "id": "vg",
            "source": "box",
            "align": "left",
            "scale": "fill"
        }
    }
}
)apl";

// Test for vector graphic component align/scale properties for dynamic
TEST_F(DynamicPropertiesTest, VectorGraphicProperties) {
    loadDocument(VECTOR_GRAPHIC_SETVALUE);
    ASSERT_TRUE(component);

    auto vg = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("vg"));
    ASSERT_TRUE(vg);
    ASSERT_EQ(kComponentTypeVectorGraphic, vg->getType());
    ASSERT_TRUE(CheckProperties(vg, {
            {kPropertyAlign, kVectorGraphicAlignLeft },
            {kPropertyScale, kVectorGraphicScaleFill },
    }));

    // Set aline property of vg
    vg->setProperty(kPropertyAlign, "center");

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(vg, kPropertyAlign));
    ASSERT_TRUE(CheckDirty(root, vg));
    root->clearDirty();

    ASSERT_EQ(kVectorGraphicAlignCenter, vg->getCalculated(kPropertyAlign).getInteger());

    // Set scale property of vg
    vg->setProperty(kPropertyScale, "best-fill");

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(vg, kPropertyScale));
    ASSERT_TRUE(CheckDirty(root, vg));
    root->clearDirty();

    ASSERT_EQ(kVectorGraphicScaleBestFill, vg->getCalculated(kPropertyScale).getInteger());
}

static const char *TEXT_SETVALUE = R"apl(
{
    "type": "APL",
    "version": "1.6",
    "mainTemplate": {
        "items": {
            "type": "Text",
            "id": "txt",
            "text": "Hello",
            "fontFamily": "times new roman",
            "fontSize": "50dp",
            "fontStyle": "italic",
            "fontWeight": 100,
            "lang": "en-US"
        }
    }
}
)apl";

// Test for text component font* properties for dynamic
TEST_F(DynamicPropertiesTest, TextProperties) {
    loadDocument(TEXT_SETVALUE);
    ASSERT_TRUE(component);

    auto txt = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("txt"));
    ASSERT_TRUE(txt);
    ASSERT_EQ(kComponentTypeText, txt->getType());
    ASSERT_TRUE(CheckProperties(txt, {
            {kPropertyFontFamily, "times new roman" },
            {kPropertyFontSize, Dimension(50) },
            {kPropertyFontStyle, kFontStyleItalic },
            {kPropertyFontWeight, 100 },
            {kPropertyLang, "en-US"}
    }));

    // Set fontFamily property of txt
    txt->setProperty(kPropertyFontFamily, "amazon-ember");

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(txt, kPropertyFontFamily));
    ASSERT_TRUE(CheckDirty(root, txt));
    root->clearDirty();
    ASSERT_EQ("amazon-ember", txt->getCalculated(kPropertyFontFamily).getString());

    // Set lang property of txt
    txt->setProperty(kPropertyLang, "ja-JP");

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(txt, kPropertyLang));
    ASSERT_TRUE(CheckDirty(root, txt));
    root->clearDirty();
    ASSERT_EQ("ja-JP", txt->getCalculated(kPropertyLang).getString());

    // Set fontSize property of txt
    txt->setProperty(kPropertyFontSize, "60dp");

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(txt, kPropertyFontSize));
    ASSERT_TRUE(CheckDirty(root, txt));
    root->clearDirty();
    ASSERT_EQ(Dimension(60), txt->getCalculated(kPropertyFontSize).getAbsoluteDimension());

    // Set fontStyle property of txt
    txt->setProperty(kPropertyFontStyle, "normal");

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(txt, kPropertyFontStyle));
    ASSERT_TRUE(CheckDirty(root, txt));
    root->clearDirty();
    ASSERT_EQ(kFontStyleNormal, txt->getCalculated(kPropertyFontStyle).getInteger());

    // Set fontWeight property of txt
    txt->setProperty(kPropertyFontWeight, 700);

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(txt, kPropertyFontWeight));
    ASSERT_TRUE(CheckDirty(root, txt));
    root->clearDirty();
    ASSERT_EQ(700, txt->getCalculated(kPropertyFontWeight).getInteger());
}

static const char *EDIT_TEXT_SETVALUE = R"apl(
{
    "type": "APL",
    "version": "1.6",
    "mainTemplate": {
        "items": {
            "type": "EditText",
            "id": "editText",
            "text": "Hello",
            "height": 100,
            "width": 100,
            "fontFamily": "times new roman",
            "fontSize": "50dp",
            "fontStyle": "italic",
            "fontWeight": 100,
            "lang": "en-US",
            "color": "blue",
            "borderWidth": 2,
            "highlightColor": "yellow",
            "hint": "hint text",
            "hintColor": "green",
            "hintStyle": "italic",
            "hintWeight": 100
        }
    }
}
)apl";

// Test for edit text component font* properties for dynamic
TEST_F(DynamicPropertiesTest, EditTextFontProperties) {
    loadDocument(EDIT_TEXT_SETVALUE);
    ASSERT_TRUE(component);

    auto txt = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("editText"));
    ASSERT_TRUE(txt);
    ASSERT_EQ(kComponentTypeEditText, txt->getType());
    ASSERT_TRUE(CheckProperties(txt, {
            {kPropertyFontFamily, "times new roman" },
            {kPropertyFontSize, Dimension(50) },
            {kPropertyFontStyle, kFontStyleItalic },
            {kPropertyFontWeight, 100 },
            {kPropertyLang, "en-US"}
    }));


    // Set fontFamily property of txt
    txt->setProperty(kPropertyFontFamily, "amazon-ember");

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(txt, kPropertyFontFamily));
    ASSERT_TRUE(CheckDirty(root, txt));
    root->clearDirty();
    ASSERT_EQ("amazon-ember", txt->getCalculated(kPropertyFontFamily).getString());

    // Set lang property of txt
    txt->setProperty(kPropertyLang, "ja-JP");

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(txt, kPropertyLang));
    ASSERT_TRUE(CheckDirty(root, txt));
    root->clearDirty();
    ASSERT_EQ("ja-JP", txt->getCalculated(kPropertyLang).getString());

    // Set fontSize property of txt
    txt->setProperty(kPropertyFontSize, "60dp");

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(txt, kPropertyFontSize));
    ASSERT_TRUE(CheckDirty(root, txt));
    root->clearDirty();
    ASSERT_EQ(Dimension(60), txt->getCalculated(kPropertyFontSize).getAbsoluteDimension());

    // Set fontStyle property of txt
    txt->setProperty(kPropertyFontStyle, "normal");

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(txt, kPropertyFontStyle));
    ASSERT_TRUE(CheckDirty(root, txt));
    root->clearDirty();
    ASSERT_EQ(kFontStyleNormal, txt->getCalculated(kPropertyFontStyle).getInteger());

    // Set fontWeight property of txt
    txt->setProperty(kPropertyFontWeight, 700);

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(txt, kPropertyFontWeight));
    ASSERT_TRUE(CheckDirty(root, txt));
    root->clearDirty();
    ASSERT_EQ(700, txt->getCalculated(kPropertyFontWeight).getInteger());
}

// Test for edit text component borderWidth/color/highlightColor properties for dynamic
TEST_F(DynamicPropertiesTest, EditTextProperties) {
    loadDocument(EDIT_TEXT_SETVALUE);
    ASSERT_TRUE(component);

    auto txt = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("editText"));
    ASSERT_TRUE(txt);
    ASSERT_EQ(kComponentTypeEditText, txt->getType());
    ASSERT_TRUE(CheckProperties(txt, {
            {kPropertyBorderWidth, Dimension(2) },
            {kPropertyColor, Color(Color::BLUE) },
            {kPropertyHighlightColor, Color(Color::YELLOW) },
            {kPropertyInnerBounds, Rect(2, 2, 96, 96) },
    }));

    // Set borderWidth property of txt
    txt->setProperty(kPropertyBorderWidth, 5);

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(txt, kPropertyBorderWidth, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(root, txt));
    root->clearDirty();
    ASSERT_TRUE(CheckProperties(txt, {
            {kPropertyBorderWidth, Dimension(5) },
            {kPropertyInnerBounds, Rect(5, 5, 90, 90) },
    }));

    // Set color property of txt
    txt->setProperty(kPropertyColor, "black");

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(txt, kPropertyColor));
    ASSERT_TRUE(CheckDirty(root, txt));
    root->clearDirty();
    ASSERT_EQ(Color::BLACK, txt->getCalculated(kPropertyColor).getColor());

    // Set highlightColor property of txt
    txt->setProperty(kPropertyHighlightColor, "gray");

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(txt, kPropertyHighlightColor));
    ASSERT_TRUE(CheckDirty(root, txt));
    root->clearDirty();
    ASSERT_EQ(Color::GRAY, txt->getCalculated(kPropertyHighlightColor).getColor());
}

// Test for edit text component hint* properties for dynamic
TEST_F(DynamicPropertiesTest, EditTextHintProperties) {
    loadDocument(EDIT_TEXT_SETVALUE);
    ASSERT_TRUE(component);

    auto txt = std::dynamic_pointer_cast<CoreComponent>(context->findComponentById("editText"));
    ASSERT_TRUE(txt);
    ASSERT_EQ(kComponentTypeEditText, txt->getType());
    ASSERT_TRUE(CheckProperties(txt, {
            {kPropertyHint, "hint text" },
            {kPropertyHintColor, Color(Color::GREEN) },
            {kPropertyHintStyle, kFontStyleItalic },
            {kPropertyHintWeight, 100 },
    }));

    // Set hint property of txt
    txt->setProperty(kPropertyHint, "new hint");

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(txt, kPropertyHint));
    ASSERT_TRUE(CheckDirty(root, txt));
    root->clearDirty();
    ASSERT_EQ("new hint", txt->getCalculated(kPropertyHint).getString());

    // Set hintColor property of txt
    txt->setProperty(kPropertyHintColor, "gray");

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(txt, kPropertyHintColor));
    ASSERT_TRUE(CheckDirty(root, txt));
    root->clearDirty();
    ASSERT_EQ(Color::GRAY, txt->getCalculated(kPropertyHintColor).getColor());

    // Set hintStyle property of txt
    txt->setProperty(kPropertyHintStyle, "normal");

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(txt, kPropertyHintStyle));
    ASSERT_TRUE(CheckDirty(root, txt));
    root->clearDirty();
    ASSERT_EQ(kFontStyleNormal, txt->getCalculated(kPropertyHintStyle).getInteger());

    // Set hintWeight property of txt
    txt->setProperty(kPropertyHintWeight, 700);

    ASSERT_EQ(1, root->getDirty().size());
    ASSERT_TRUE(CheckDirty(txt, kPropertyHintWeight));
    ASSERT_TRUE(CheckDirty(root, txt));
    root->clearDirty();
    ASSERT_EQ(700, txt->getCalculated(kPropertyHintWeight).getInteger());
}

static const char *SEQUENCE_SETVALUE = R"apl(
{
    "type": "APL",
    "version": "1.6",
    "styles": {
        "base": {
            "values": [
                {
                    "height": 20,
                    "width": 100,
                    "spacing": 10
                },
                {
                    "when": "${state.disabled}",
                    "spacing": 20
                }
            ]
        }
    },
    "mainTemplate": {
        "item": {
            "type": "Sequence",
            "scrollDirection": "vertical",
            "height": 100,
            "width": 100,
            "items": [
                {
                    "type": "Text",
                    "id": "c1",
                    "text": "Child One",
                    "style": "base"
                },
                {
                    "type": "Text",
                    "text": "Child Two",
                    "id": "c2",
                    "style": "base"
                },
                {
                    "type": "Text",
                    "text": "Child Three",
                    "id": "c3",
                    "style": "base"
                }
            ]
        }
    }
}
)apl";

// Test for base component padding* properties for styled
TEST_F(DynamicPropertiesTest, SequenceStyled)
{
    loadDocument(SEQUENCE_SETVALUE);
    ASSERT_TRUE(component);

    auto child0 = std::dynamic_pointer_cast<CoreComponent>(component->getChildAt(0));
    ASSERT_TRUE(CheckProperties(child0, {
            {kPropertySpacing, Dimension(10) }, // spacing will be ignored for the first child
            {kPropertyBounds, Rect(0, 0, 100, 20) },
    }));

    auto child1 = std::dynamic_pointer_cast<CoreComponent>(component->getChildAt(1));
    ASSERT_TRUE(CheckProperties(child1, {
            {kPropertySpacing, Dimension(10) },
            {kPropertyBounds, Rect(0, 30, 100, 20) },
    }));

    auto child2 = std::dynamic_pointer_cast<CoreComponent>(component->getChildAt(2));
    ASSERT_TRUE(CheckProperties(child2, {
            {kPropertySpacing, Dimension(10) },
            {kPropertyBounds, Rect(0, 60, 100, 20) },
    }));

    // disabling state of child1 to change style
    child1->setState(StateProperty::kStateDisabled, true);
    ASSERT_TRUE(CheckDirty(root, component, child1, child2));

    ASSERT_EQ(Object(Dimension(20)), child1 ->getCalculated(kPropertySpacing));
    ASSERT_EQ(Rect(0, 0, 100, 20), child0->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 40, 100, 20), child1->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 70, 100, 20), child2->getCalculated(kPropertyBounds).getRect());

    root->clearDirty();
}

// Test for sequence component child spacing properties for dynamic
TEST_F(DynamicPropertiesTest, SequenceDynamic) {
    loadDocument(SEQUENCE_SETVALUE);
    ASSERT_TRUE(component);

    ASSERT_EQ(3, component->getChildCount());

    ASSERT_TRUE(CheckChildrenLaidOut(component, Range(0, 2), true));

    auto child0 = std::dynamic_pointer_cast<CoreComponent>(component->getChildAt(0));
    ASSERT_TRUE(CheckProperties(child0, {
            {kPropertySpacing, Dimension(10) }, // spacing will be ignored for the first child
            {kPropertyBounds, Rect(0, 0, 100, 20) },
    }));

    auto child1 = std::dynamic_pointer_cast<CoreComponent>(component->getChildAt(1));
    ASSERT_TRUE(CheckProperties(child1, {
            {kPropertySpacing, Dimension(10) },
            {kPropertyBounds, Rect(0, 30, 100, 20) },
    }));

    auto child2 = std::dynamic_pointer_cast<CoreComponent>(component->getChildAt(2));
    ASSERT_TRUE(CheckProperties(child2, {
            {kPropertySpacing, Dimension(10) },
            {kPropertyBounds, Rect(0, 60, 100, 20) },
    }));

    // Set spacing property of child at index 1
    child1->setProperty(kPropertySpacing, 20);

    root->clearPending();
    ASSERT_TRUE(CheckDirty(child1, kPropertyBounds));
    ASSERT_TRUE(CheckDirty(child2, kPropertyBounds));
    ASSERT_TRUE(CheckDirty(root, component, child1, child2));
    root->clearDirty();
    ASSERT_EQ(Object(Dimension(20)), child1 ->getCalculated(kPropertySpacing));
    ASSERT_EQ(Rect(0, 0, 100, 20), child0->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 40, 100, 20), child1->getCalculated(kPropertyBounds).getRect());
    ASSERT_EQ(Rect(0, 70, 100, 20), child2->getCalculated(kPropertyBounds).getRect());
}