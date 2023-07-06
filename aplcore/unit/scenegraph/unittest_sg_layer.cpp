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

using namespace apl;

class SGLayerTest : public ::testing::Test {
};

TEST_F(SGLayerTest, Basic)
{
    auto layer = sg::layer("Test", Rect(10,20,200,300), 0.5f, Transform2D::scale(2.0f));

    ASSERT_EQ("Test", layer->getName());

    ASSERT_FALSE(layer->anyFlagSet());
    ASSERT_EQ(layer->debugFlagString(), "");
    ASSERT_EQ(layer->debugInteractionString(), "");

    ASSERT_EQ(0, layer->children().size());
    ASSERT_FALSE(layer->content());

    ASSERT_EQ(Rect(10,20,200,300), layer->getBounds());
    ASSERT_FALSE(layer->getOutline());
    ASSERT_FALSE(layer->getChildClip());
    ASSERT_EQ(0.5f, layer->getOpacity());
    ASSERT_EQ(Transform2D::scale(2.0f), layer->getTransform());
    ASSERT_EQ(Point(0,0), layer->getChildOffset());
    ASSERT_FALSE(layer->getShadow());
    ASSERT_FALSE(layer->getAccessibility());
    ASSERT_FALSE(layer->visible());
    ASSERT_EQ(layer->toDebugString(), "Layer Test");

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(layer->serialize(doc.GetAllocator()),
                        StringToMapObject(R"apl(
        {
            "name": "Test",
            "opacity": 0.5,
            "bounds": [ 10, 20, 200, 300 ],
            "transform": [ 2, 0, 0, 2, 0, 0],
            "childOffset": [ 0, 0 ],
            "contentOffset": [ 0, 0 ],
            "interaction": 0,
            "characteristics": 0
        }
    )apl")));
}

TEST_F(SGLayerTest, Rich)
{
    auto layer = sg::layer("Test", Rect(10, 20, 200, 300), 0.5f, Transform2D::scale(2.0f));
    layer->setContent(
        sg::draw(sg::path(Rect(10, 10, 10, 10)), sg::fill(sg::paint(Color(Color::GREEN)))));

    ASSERT_TRUE(layer->visible());
    ASSERT_EQ(layer->debugFlagString(), "CONTENT");

    ASSERT_TRUE(layer->setOpacity(1.0f));
    ASSERT_EQ(1.0f, layer->getOpacity());
    ASSERT_EQ(layer->debugFlagString(), "OPACITY CONTENT");

    ASSERT_TRUE(layer->setBounds(Rect(10,20,100,100)));
    ASSERT_EQ(layer->getBounds(), Rect(10,20,100,100));
    ASSERT_EQ(layer->debugFlagString(), "OPACITY SIZE CONTENT");

    layer->clearFlags();
    ASSERT_EQ(0, layer->getAndClearFlags());

    ASSERT_TRUE(layer->setBounds(Rect(0,0,100,100)));
    ASSERT_EQ(layer->getBounds(), Rect(0,0,100,100));
    ASSERT_EQ(layer->debugFlagString(), "POSITION");

    // Set a new transform same as the old transform
    ASSERT_FALSE(layer->setTransform(Transform2D::scale(2.0f)));
    ASSERT_EQ(layer->debugFlagString(), "POSITION");

    // Actually change the transform
    ASSERT_TRUE(layer->setTransform(Transform2D()));
    ASSERT_EQ(Transform2D(), layer->getTransform());
    ASSERT_EQ(layer->debugFlagString(), "POSITION TRANSFORM");

    layer->setChildOffset(Point(20,20));
    ASSERT_EQ(Point(20,20), layer->getChildOffset());
    ASSERT_EQ(layer->debugFlagString(), "POSITION TRANSFORM CHILD_OFFSET");

    layer->setOutline(sg::path(Rect(0,0,100,100), 20));
    ASSERT_TRUE(layer->getOutline());  // Just check if it has been set
    ASSERT_EQ(layer->debugFlagString(), "POSITION TRANSFORM CHILD_OFFSET OUTLINE");

    layer->setShadow(sg::shadow(Color(Color::BLACK), Point(4,4), 10.0f));
    ASSERT_TRUE(layer->getShadow());  // Just check if it has been set
    ASSERT_EQ(layer->debugFlagString(), "POSITION TRANSFORM CHILD_OFFSET OUTLINE SHADOW");

    ASSERT_EQ(layer->getAndClearFlags(),
              sg::Layer::kFlagPositionChanged |
              sg::Layer::kFlagTransformChanged |
              sg::Layer::kFlagChildOffsetChanged |
              sg::Layer::kFlagOutlineChanged |
              sg::Layer::kFlagRedrawShadow);

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(layer->serialize(doc.GetAllocator()),
                        StringToMapObject(R"apl(
        {
            "name": "Test",
            "opacity": 1.0,
            "bounds": [ 0, 0, 100, 100 ],
            "transform": [ 1, 0, 0, 1, 0, 0],
            "childOffset": [ 20, 20 ],
            "contentOffset": [ 0, 0 ],
            "outline": {
                "type": "roundedRectPath",
                "rect": [ 0, 0, 100, 100 ],
                "radii": [ 20, 20, 20, 20 ]
            },
            "shadow": {
                "color": "#000000ff",
                "offset": [ 4, 4 ],
                "radius": 10
            },
            "content": [
                {
                    "type": "draw",
                    "path": {
                        "type": "rectPath",
                        "rect": [ 10, 10, 10, 10 ]
                    },
                    "op": [
                        {
                            "type": "fill",
                            "fillType": "even-odd",
                            "paint": {
                                "type": "colorPaint",
                                "color": "#008000ff",
                                "opacity": 1.0
                            }
                        }
                    ]
                }
            ],
            "interaction": 0,
            "characteristics": 0
        }
    )apl")));
}

TEST_F(SGLayerTest, Interaction)
{
    auto layer = sg::layer("Test", Rect(10, 20, 200, 300), 0.0f, Transform2D());

    // Setting the interaction is used during startup and doesn't set any changed flags
    layer->setInteraction(sg::Layer::kInteractionDisabled | sg::Layer::kInteractionChecked);

    ASSERT_EQ(layer->getInteraction(), sg::Layer::kInteractionDisabled | sg::Layer::kInteractionChecked);
    ASSERT_EQ("disabled checked", layer->debugInteractionString());
    ASSERT_FALSE(layer->getAndClearFlags());

    // Now update the interaction to add a few options
    layer->updateInteraction(sg::Layer::kInteractionPressable, true);
    ASSERT_EQ("disabled checked pressable", layer->debugInteractionString());
    ASSERT_EQ(sg::Layer::kFlagInteractionChanged, layer->getAndClearFlags());

    layer->updateInteraction(sg::Layer::kInteractionDisabled, false);
    layer->updateInteraction(sg::Layer::kInteractionScrollHorizontal | sg::Layer::kInteractionScrollVertical, true);
    ASSERT_EQ("checked pressable scrollHorizontal scrollVertical", layer->debugInteractionString());
    ASSERT_EQ(sg::Layer::kFlagInteractionChanged, layer->getAndClearFlags());

    // Verify that the last step cleared the flags
    ASSERT_FALSE(layer->getAndClearFlags());
}

TEST_F(SGLayerTest, Children)
{
    auto layer = sg::layer("Test", Rect(0, 0, 100, 100), 1.0f, Transform2D());
    auto child1 = sg::layer("Child1", Rect(20, 20, 60, 10), 1.0f, Transform2D());
    auto child2 = sg::layer("Child2", Rect(20, 50, 60, 10), 1.0f, Transform2D());

    ASSERT_EQ("Test", layer->getName());

    ASSERT_FALSE(layer->anyFlagSet());
    ASSERT_FALSE(layer->visible());
    ASSERT_EQ(layer->debugFlagString(), "");
    ASSERT_EQ(layer->debugInteractionString(), "");
    ASSERT_EQ(0, layer->children().size());

    rapidjson::Document doc;
    ASSERT_TRUE(IsEqual(layer->serialize(doc.GetAllocator()),
                        StringToMapObject(R"apl(
        {
            "name": "Test",
            "opacity": 1.0,
            "bounds": [ 0, 0, 100, 100 ],
            "transform": [ 1, 0, 0, 1, 0, 0],
            "childOffset": [ 0, 0 ],
            "contentOffset": [ 0, 0 ],
            "interaction": 0,
            "characteristics": 0
        }
    )apl")));

    // Add one child
    layer->appendChild(child1);
    ASSERT_EQ(sg::Layer::kFlagChildrenChanged, layer->getAndClearFlags());
    ASSERT_EQ(1, layer->children().size());
    ASSERT_FALSE(layer->visible());   // Nothing is visible yet

    // Add another child
    layer->appendChild(child2);
    ASSERT_EQ(sg::Layer::kFlagChildrenChanged, layer->getAndClearFlags());
    ASSERT_EQ(2, layer->children().size());
    ASSERT_FALSE(layer->visible());   // Nothing is visible yet

    // Make this child visible
    child2->setShadow(sg::shadow(Color(Color::BLACK), Point(2, 2), 5.0f));
    ASSERT_EQ(0, layer->getAndClearFlags());
    ASSERT_EQ(sg::Layer::kFlagRedrawShadow, child2->getAndClearFlags());
    ASSERT_TRUE(child2->visible());
    ASSERT_TRUE(layer->visible());

    ASSERT_TRUE(IsEqual(layer->serialize(doc.GetAllocator()),
                        StringToMapObject(R"apl(
        {
            "name": "Test",
            "opacity": 1.0,
            "bounds": [ 0, 0, 100, 100 ],
            "transform": [ 1, 0, 0, 1, 0, 0],
            "childOffset": [ 0, 0 ],
            "contentOffset": [ 0, 0 ],
            "interaction": 0,
            "characteristics": 0,
            "children": [
                {
                    "name": "Child1",
                    "opacity": 1.0,
                    "bounds": [ 20, 20, 60, 10 ],
                    "transform": [ 1, 0, 0, 1, 0, 0],
                    "childOffset": [ 0, 0 ],
                    "contentOffset": [ 0, 0 ],
                    "interaction": 0,
                    "characteristics": 0
                },
                {
                    "name": "Child2",
                    "opacity": 1.0,
                    "bounds": [ 20, 50, 60, 10 ],
                    "transform": [ 1, 0, 0, 1, 0, 0],
                    "childOffset": [ 0, 0 ],
                    "contentOffset": [ 0, 0 ],
                    "interaction": 0,
                    "characteristics": 0,
                    "shadow": {
                        "color": "#000000ff",
                        "offset": [2, 2],
                        "radius": 5
                    }
                }
            ]
        }
    )apl")));

}

TEST_F(SGLayerTest, Characteristics)
{
    auto layer = sg::layer("Test", Rect(0, 0, 100, 100), 1.0f, Transform2D());
    ASSERT_EQ(0, layer->getCharacteristic());
    ASSERT_EQ(layer->debugCharacteristicString().find("DO_NOT_CLIP_CHILDREN"), std::string::npos);
    ASSERT_EQ(layer->debugCharacteristicString().find("RENDER_ONLY"), std::string::npos);
    ASSERT_FALSE(layer->isCharacteristicSet(sg::Layer::kCharacteristicRenderOnly));
    ASSERT_FALSE(layer->isCharacteristicSet(sg::Layer::kCharacteristicDoNotClipChildren));

    layer->setCharacteristic(sg::Layer::kCharacteristicRenderOnly);
    ASSERT_EQ(sg::Layer::kCharacteristicRenderOnly, layer->getCharacteristic());
    ASSERT_EQ(layer->debugCharacteristicString().find("DO_NOT_CLIP_CHILDREN"), std::string::npos);
    ASSERT_NE(layer->debugCharacteristicString().find("RENDER_ONLY"), std::string::npos);
    ASSERT_TRUE(layer->isCharacteristicSet(sg::Layer::kCharacteristicRenderOnly));
    ASSERT_FALSE(layer->isCharacteristicSet(sg::Layer::kCharacteristicDoNotClipChildren));

    layer->setCharacteristic(sg::Layer::kCharacteristicDoNotClipChildren);
    ASSERT_EQ(sg::Layer::kCharacteristicRenderOnly | sg::Layer::kCharacteristicDoNotClipChildren, layer->getCharacteristic());
    ASSERT_NE(layer->debugCharacteristicString().find("DO_NOT_CLIP_CHILDREN"), std::string::npos);
    ASSERT_NE(layer->debugCharacteristicString().find("RENDER_ONLY"), std::string::npos);
    ASSERT_TRUE(layer->isCharacteristicSet(sg::Layer::kCharacteristicRenderOnly));
    ASSERT_TRUE(layer->isCharacteristicSet(sg::Layer::kCharacteristicDoNotClipChildren));
}