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

#include "apl/focus/focusmanager.h"
#include "apl/graphic/graphic.h"
#include "apl/primitives/object.h"

using namespace apl;

class GraphicComponentTest : public DocumentWrapper {};

static const char * SIMPLE_TEST = R"(
{
  "type": "APL",
  "version": "1.0",
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 100,
      "lang": "en-US",
      "items": {
        "type": "path",
        "pathData": "M0,0 h100 v100 h-100 z",
        "fill": "red"
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "source": "box"
    }
  }
})";

TEST_F(GraphicComponentTest, SimpleTest)
{
    loadDocument(SIMPLE_TEST);

    // We expect the vector graphic to just wrap the defined graphic (of size 100x100)
    ASSERT_EQ( kComponentTypeVectorGraphic, component->getType());
    ASSERT_EQ( Rect(0, 0, 100, 100), component->getGlobalBounds());

    ASSERT_EQ( kVectorGraphicAlignCenter, component->getCalculated(kPropertyAlign).getInteger());
    ASSERT_EQ( kVectorGraphicScaleNone, component->getCalculated(kPropertyScale).getInteger());
    ASSERT_EQ( Object("box"), component->getCalculated(kPropertySource));
    ASSERT_TRUE( component->getCalculated(kPropertyGraphic).isGraphic());

    // Check to see if the graphic will be drawn where we thought it should be
    ASSERT_EQ(Object(Rect(0, 0, 100, 100)), component->getCalculated(kPropertyMediaBounds));

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_TRUE(graphic);

    ASSERT_EQ(100, graphic->getIntrinsicWidth());
    ASSERT_EQ(100, graphic->getIntrinsicHeight());
    ASSERT_EQ(100, graphic->getViewportHeight());
    ASSERT_EQ(100, graphic->getViewportWidth());
}

TEST_F(GraphicComponentTest, SimpleTestInfo)
{
    loadDocument(SIMPLE_TEST);

    auto count = root->info().count(Info::kInfoTypeGraphic);
    ASSERT_EQ(1, count);

    auto p = root->info().at(Info::kInfoTypeGraphic, 0);
    ASSERT_STREQ("box", p.first.c_str());
    ASSERT_STREQ("_main/graphics/box", p.second.c_str());
}

static const char * NO_SCALE = R"(
{
  "type": "APL",
  "version": "1.0",
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 100,
      "items": {
        "type": "path",
        "pathData": "M0,0 h100 v100 h-100 z",
        "fill": "red"
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "source": "box",
      "width": "100%",
      "height": "100%"
    }
  }
})";

TEST_F(GraphicComponentTest, BasicNoScale)
{
    loadDocument(NO_SCALE);

    // The vector graphic component expands to fill the entire screen.
    ASSERT_EQ( kComponentTypeVectorGraphic, component->getType());
    ASSERT_EQ( Rect(0, 0, metrics.getWidth(), metrics.getHeight()), component->getGlobalBounds());

    ASSERT_EQ( kVectorGraphicAlignCenter, component->getCalculated(kPropertyAlign).getInteger());
    ASSERT_EQ( kVectorGraphicScaleNone, component->getCalculated(kPropertyScale).getInteger());
    ASSERT_EQ( Object("box"), component->getCalculated(kPropertySource));
    ASSERT_TRUE( component->getCalculated(kPropertyGraphic).isGraphic());

    // Check to see if the graphic will be drawn where we thought it should be
    ASSERT_EQ(Object(Rect((metrics.getWidth() - 100)/2, (metrics.getHeight() - 100)/2, 100, 100)),
              component->getCalculated(kPropertyMediaBounds));

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_TRUE(graphic);

    // The graphic element is not scaled, so it should be the original 100x100 size and centered
    ASSERT_EQ(100, graphic->getIntrinsicWidth());
    ASSERT_EQ(100, graphic->getIntrinsicHeight());
    ASSERT_EQ(100, graphic->getViewportHeight());
    ASSERT_EQ(100, graphic->getViewportWidth());
}

static const char * BEST_FIT = R"(
{
  "type": "APL",
  "version": "1.0",
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 100,
      "items": {
        "type": "path",
        "pathData": "M0,0 h100 v100 h-100 z",
        "fill": "red"
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "source": "box",
      "width": "100%",
      "height": "100%",
      "scale": "best-fit"
    }
  }
})";

TEST_F(GraphicComponentTest, BasicBestFit)
{
    loadDocument(BEST_FIT);

    ASSERT_EQ( kComponentTypeVectorGraphic, component->getType());
    ASSERT_EQ( Rect(0, 0, metrics.getWidth(), metrics.getHeight()), component->getGlobalBounds());

    ASSERT_EQ( kVectorGraphicAlignCenter, component->getCalculated(kPropertyAlign).getInteger());
    ASSERT_EQ( kVectorGraphicScaleBestFit, component->getCalculated(kPropertyScale).getInteger());
    ASSERT_EQ( Object("box"), component->getCalculated(kPropertySource));
    ASSERT_TRUE( component->getCalculated(kPropertyGraphic).isGraphic());

    // Check to see if the graphic will be drawn where we thought it should be
    double minSize = std::min(metrics.getWidth(), metrics.getHeight());
    ASSERT_EQ(Object(Rect((metrics.getWidth() - minSize)/2, (metrics.getHeight() - minSize) / 2, minSize, minSize)),
              component->getCalculated(kPropertyMediaBounds));

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_TRUE(graphic);

    ASSERT_EQ(100, graphic->getIntrinsicWidth());
    ASSERT_EQ(100, graphic->getIntrinsicHeight());
    ASSERT_EQ(100, graphic->getViewportHeight());
    ASSERT_EQ(100, graphic->getViewportWidth());
}

static const char * BASE_FIT_TEST_CASE = R"(
{
  "type": "APL",
  "version": "1.0",
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 100,
      "items": {
        "type": "path",
        "pathData": "M0,0 h100 v100 h-100 z",
        "fill": "red"
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "source": "box",
      "width": "100%",
      "height": "100%"
    }
  }
})";

struct FitTestCase {
    VectorGraphicAlign align;
    VectorGraphicScale scale;
    Rect bounds;
};

// For all of these test cases, the VectorGraphicComponent will have a size of 1024 x 800
std::vector<FitTestCase> sFitTestCases = {
    { kVectorGraphicAlignTopLeft, kVectorGraphicScaleNone, Rect(0, 0, 100, 100)},
    { kVectorGraphicAlignTop, kVectorGraphicScaleNone, Rect(462, 0, 100, 100)},
    { kVectorGraphicAlignTopRight, kVectorGraphicScaleNone, Rect(924, 0, 100, 100)},
    { kVectorGraphicAlignLeft, kVectorGraphicScaleNone, Rect(0, 350, 100, 100)},
    { kVectorGraphicAlignCenter, kVectorGraphicScaleNone, Rect(462, 350, 100, 100)},
    { kVectorGraphicAlignRight, kVectorGraphicScaleNone, Rect(924, 350, 100, 100)},
    { kVectorGraphicAlignBottomLeft, kVectorGraphicScaleNone, Rect(0, 700, 100, 100)},
    { kVectorGraphicAlignBottom, kVectorGraphicScaleNone, Rect(462, 700, 100, 100)},
    { kVectorGraphicAlignBottomRight, kVectorGraphicScaleNone, Rect(924, 700, 100, 100)},

    { kVectorGraphicAlignTopLeft, kVectorGraphicScaleFill, Rect(0, 0, 1024, 800)},
    { kVectorGraphicAlignTop, kVectorGraphicScaleFill, Rect(0, 0, 1024, 800)},
    { kVectorGraphicAlignTopRight, kVectorGraphicScaleFill, Rect(0, 0, 1024, 800)},
    { kVectorGraphicAlignLeft, kVectorGraphicScaleFill, Rect(0, 0, 1024, 800)},
    { kVectorGraphicAlignCenter, kVectorGraphicScaleFill, Rect(0, 0, 1024, 800)},
    { kVectorGraphicAlignRight, kVectorGraphicScaleFill, Rect(0, 0, 1024, 800)},
    { kVectorGraphicAlignBottomLeft, kVectorGraphicScaleFill, Rect(0, 0, 1024, 800)},
    { kVectorGraphicAlignBottom, kVectorGraphicScaleFill, Rect(0, 0, 1024, 800)},
    { kVectorGraphicAlignBottomRight, kVectorGraphicScaleFill, Rect(0, 0, 1024, 800)},

    { kVectorGraphicAlignTopLeft, kVectorGraphicScaleBestFit, Rect(0, 0, 800, 800)},
    { kVectorGraphicAlignTop, kVectorGraphicScaleBestFit, Rect(112, 0, 800, 800)},
    { kVectorGraphicAlignTopRight, kVectorGraphicScaleBestFit, Rect(224, 0, 800, 800)},
    { kVectorGraphicAlignLeft, kVectorGraphicScaleBestFit, Rect(0, 0, 800, 800)},
    { kVectorGraphicAlignCenter, kVectorGraphicScaleBestFit, Rect(112, 0, 800, 800)},
    { kVectorGraphicAlignRight, kVectorGraphicScaleBestFit, Rect(224, 0, 800, 800)},
    { kVectorGraphicAlignBottomLeft, kVectorGraphicScaleBestFit, Rect(0, 0, 800, 800)},
    { kVectorGraphicAlignBottom, kVectorGraphicScaleBestFit, Rect(112, 0, 800, 800)},
    { kVectorGraphicAlignBottomRight, kVectorGraphicScaleBestFit, Rect(224, 0, 800, 800)},

    { kVectorGraphicAlignTopLeft, kVectorGraphicScaleBestFill, Rect(0, 0, 1024, 1024)},
    { kVectorGraphicAlignTop, kVectorGraphicScaleBestFill, Rect(0, 0, 1024, 1024)},
    { kVectorGraphicAlignTopRight, kVectorGraphicScaleBestFill, Rect(0, 0, 1024, 1024)},
    { kVectorGraphicAlignLeft, kVectorGraphicScaleBestFill, Rect(0, -112, 1024, 1024)},
    { kVectorGraphicAlignCenter, kVectorGraphicScaleBestFill, Rect(0, -112, 1024, 1024)},
    { kVectorGraphicAlignRight, kVectorGraphicScaleBestFill, Rect(0, -112, 1024, 1024)},
    { kVectorGraphicAlignBottomLeft, kVectorGraphicScaleBestFill, Rect(0, -224, 1024, 1024)},
    { kVectorGraphicAlignBottom, kVectorGraphicScaleBestFill, Rect(0, -224, 1024, 1024)},
    { kVectorGraphicAlignBottomRight, kVectorGraphicScaleBestFill, Rect(0, -224, 1024, 1024)},
};

TEST_F(GraphicComponentTest, FitAndScale)
{
    int index = 0;
    for (auto& ftc : sFitTestCases) {
        rapidjson::Document doc;
        doc.Parse(BASE_FIT_TEST_CASE);
        ASSERT_TRUE(doc.IsObject());
        auto& items = doc.FindMember("mainTemplate")->value.FindMember("items")->value;

        index += 1;
        rapidjson::Value scale(sVectorGraphicScaleMap.at(ftc.scale).c_str(), doc.GetAllocator());
        rapidjson::Value align(sVectorGraphicAlignMap.at(ftc.align).c_str(), doc.GetAllocator());

        items.AddMember("scale", scale, doc.GetAllocator());
        items.AddMember("align", align, doc.GetAllocator());

        auto content = Content::create(doc, makeDefaultSession());
        ASSERT_TRUE(content && content->isReady()) << "Test case #" << index;

        root = RootContext::create(Metrics().size(1024, 800), content);
        ASSERT_TRUE(root) << "test case " << index;
        component = std::dynamic_pointer_cast<CoreComponent>(root->topComponent());
        ASSERT_TRUE(component) << "test case " << index;

        // Verify that the scale and align were set correctly
        ASSERT_EQ(Object(ftc.scale), component->getCalculated(kPropertyScale)) << "test case " << index;
        ASSERT_EQ(Object(ftc.align), component->getCalculated(kPropertyAlign)) << "test case " << index;

        // Check that the media bounds have been set
        ASSERT_EQ(component->getCalculated(kPropertyMediaBounds).getRect(), ftc.bounds) << "test case " << index;
    }
}

static const char * BASE_STRETCH_TEST_CASE = R"(
{
  "type": "APL",
  "version": "1.0",
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 100,
      "items": {
        "type": "path",
        "pathData": "M0,0 h100 v100 h-100 z",
        "fill": "red"
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "source": "box",
      "width": "100%",
      "height": "100%",
      "scale": "fill"
    }
  }
})";

struct ViewportStretchCase {
    GraphicScale xScale;
    GraphicScale yScale;
    double viewportWidth;
    double viewportHeight;
};

// For all of these test cases, the VectorGraphicComponent will have a size of 1024 x 800
std::vector<ViewportStretchCase> sViewportStretch = {
    {kGraphicScaleNone,   kGraphicScaleNone,    100, 100},
    {kGraphicScaleNone,   kGraphicScaleShrink,  100, 100},
    {kGraphicScaleNone,   kGraphicScaleGrow,    100, 800},
    {kGraphicScaleNone,   kGraphicScaleStretch, 100, 800},

    {kGraphicScaleShrink, kGraphicScaleNone,    100, 100},
    {kGraphicScaleShrink, kGraphicScaleShrink,  100, 100},
    {kGraphicScaleShrink, kGraphicScaleGrow,    100, 800},
    {kGraphicScaleShrink, kGraphicScaleStretch, 100, 800},

    {kGraphicScaleGrow, kGraphicScaleNone,    1024, 100},
    {kGraphicScaleGrow, kGraphicScaleShrink,  1024, 100},
    {kGraphicScaleGrow, kGraphicScaleGrow,    1024, 800},
    {kGraphicScaleGrow, kGraphicScaleStretch, 1024, 800},

    {kGraphicScaleStretch, kGraphicScaleNone,    1024, 100},
    {kGraphicScaleStretch, kGraphicScaleShrink,  1024, 100},
    {kGraphicScaleStretch, kGraphicScaleGrow,    1024, 800},
    {kGraphicScaleStretch, kGraphicScaleStretch, 1024, 800},
};

TEST_F(GraphicComponentTest, StretchAndGrow)
{
    int index = 0;
    for (auto& ftc : sViewportStretch) {
        rapidjson::Document doc;
        doc.Parse(BASE_STRETCH_TEST_CASE);
        ASSERT_TRUE(doc.IsObject());
        auto& box = doc.FindMember("graphics")->value.FindMember("box")->value;

        index += 1;
        rapidjson::Value scaleTypeWidth(sGraphicScaleBimap.at(ftc.xScale).c_str(), doc.GetAllocator());
        rapidjson::Value scaleTypeHeight(sGraphicScaleBimap.at(ftc.yScale).c_str(), doc.GetAllocator());

        box.AddMember("scaleTypeWidth", scaleTypeWidth, doc.GetAllocator());
        box.AddMember("scaleTypeHeight", scaleTypeHeight, doc.GetAllocator());

        auto content = Content::create(doc, session);
        ASSERT_TRUE(content && content->isReady()) << "Test case #" << index;

        root = RootContext::create(Metrics().size(1024, 800), content);
        ASSERT_TRUE(root) << "test case " << index;
        component = std::dynamic_pointer_cast<CoreComponent>(root->topComponent());
        ASSERT_TRUE(component) << "test case " << index;

        ASSERT_TRUE(component->getCalculated(kPropertyGraphic).isGraphic()) << "test case " << index;
        auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
        ASSERT_TRUE(graphic) << "test case " << index;
        auto top = graphic->getRoot();

        // Verify that the scaleTypeWidth and scaleTypeHeight were set correctly
        ASSERT_EQ(Object(ftc.xScale), top->getValue(kGraphicPropertyScaleTypeWidth)) << "test case " << index;
        ASSERT_EQ(Object(ftc.yScale), top->getValue(kGraphicPropertyScaleTypeHeight)) << "test case " << index;

        // Check that the viewport width and height are correct
        ASSERT_EQ(Object(ftc.viewportWidth), graphic->getViewportWidth()) << "test case " << index;
        ASSERT_EQ(Object(ftc.viewportHeight), graphic->getViewportHeight()) << "test case " << index;
    }
}


static const char * GRAPHIC_STYLE = R"(
{
  "type": "APL",
  "version": "1.0",
  "styles": {
    "myGraphic": {
      "values": [
        {
          "color": "blue"
        },
        {
          "when": "${state.pressed}",
          "color": "red"
        }
      ]
    }
  },
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 100,
      "parameters": [
        "color"
      ],
      "items": {
        "type": "path",
        "pathData": "M0,0 h100 v100 h-100 z",
        "fill": "${color}"
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "source": "box",
      "width": "100%",
      "height": "100%",
      "style": "myGraphic"
    }
  }
})";

TEST_F(GraphicComponentTest, StyleTest)
{
    loadDocument(GRAPHIC_STYLE);

    ASSERT_EQ( kComponentTypeVectorGraphic, component->getType());
    ASSERT_EQ( Rect(0, 0, metrics.getWidth(), metrics.getHeight()), component->getGlobalBounds());

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_TRUE(graphic);

    auto box = graphic->getRoot();
    ASSERT_TRUE(box);
    ASSERT_EQ(kGraphicElementTypeContainer, box->getType());

    auto path = box->getChildAt(0);

    ASSERT_TRUE(IsEqual(Color(session, "blue"), path->getValue(kGraphicPropertyFill)));

    ASSERT_EQ(0, path->getDirtyProperties().size());
    ASSERT_EQ(0, graphic->getDirty().size());
    component->setState(kStatePressed, true);

    ASSERT_TRUE(IsEqual(Color(session, "red"), path->getValue(kGraphicPropertyFill)));
    ASSERT_TRUE(CheckDirty(path, kGraphicPropertyFill));
    ASSERT_TRUE(CheckDirty(graphic, path));
}


static const char *GRAPHIC_STYLE_WITH_ALIGNMENT = R"(
{
  "type": "APL",
  "version": "1.0",
  "styles": {
    "myGraphic": {
      "values": [
        {
          "align": "left"
        },
        {
          "when": "${state.pressed}",
          "align": "right"
        }
      ]
    }
  },
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 100,
      "parameters": [
        "color"
      ],
      "items": {
        "type": "path",
        "pathData": "M0,0 h100 v100 h-100 z",
        "fill": "${color}"
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "source": "box",
      "width": "100%",
      "height": "100%",
      "style": "myGraphic"
    }
  }
})";


TEST_F(GraphicComponentTest, StyleTestWithAlignment)
{
    loadDocument(GRAPHIC_STYLE_WITH_ALIGNMENT);

    ASSERT_EQ( kComponentTypeVectorGraphic, component->getType());
    ASSERT_EQ( Rect(0, 0, metrics.getWidth(), metrics.getHeight()), component->getGlobalBounds());

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_TRUE(graphic);

    ASSERT_EQ(Rect(0, 350, 100, 100), component->getCalculated(kPropertyMediaBounds).getRect());

    auto box = graphic->getRoot();
    ASSERT_TRUE(box);
    ASSERT_EQ(kGraphicElementTypeContainer, box->getType());

    auto path = box->getChildAt(0);

    ASSERT_EQ(0, path->getDirtyProperties().size());
    ASSERT_EQ(0, graphic->getDirty().size());
    component->setState(kStatePressed, true);

    ASSERT_EQ(Rect(924, 350, 100, 100), component->getCalculated(kPropertyMediaBounds).getRect());
    ASSERT_TRUE(CheckDirty(component, kPropertyAlign, kPropertyMediaBounds));
    ASSERT_TRUE(CheckDirty(path));
}

static const char *GRAPHIC_STYLE_WITH_STRETCH = R"(
{
  "type": "APL",
  "version": "1.0",
  "styles": {
    "myGraphic": {
      "values": [
        {
          "scale": "fill"
        },
        {
          "when": "${state.pressed}",
          "scale": "none",
          "align": "right"
        }
      ]
    }
  },
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.0",
      "height": 50,
      "width": 256,
      "viewportHeight": 100,
      "viewportWidth": 100,
      "scaleTypeHeight": "stretch",
      "scaleTypeWidth": "stretch",
      "items": {
        "type": "path",
        "pathData": "M${width},${height} L0,0"
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "source": "box",
      "width": "100%",
      "height": "100%",
      "style": "myGraphic"
    }
  }
})";


TEST_F(GraphicComponentTest, StyleTestWithStretch)
{
    loadDocument(GRAPHIC_STYLE_WITH_STRETCH);

    ASSERT_EQ(kComponentTypeVectorGraphic, component->getType());
    ASSERT_EQ(Rect(0, 0, metrics.getWidth(), metrics.getHeight()), component->getGlobalBounds());
    ASSERT_EQ(Rect(0, 0, 1024, 800), component->getCalculated(kPropertyMediaBounds).getRect());

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_TRUE(graphic);
    ASSERT_EQ(400, graphic->getViewportWidth());   // Factor of 4 = 1024 / 256
    ASSERT_EQ(1600, graphic->getViewportHeight()); // Factor of 16 = 800 / 50
    ASSERT_TRUE(CheckDirty(graphic));

    // The top-level container has no properties
    auto container = graphic->getRoot();
    ASSERT_TRUE(container);
    ASSERT_EQ(kGraphicElementTypeContainer, container->getType());
    ASSERT_TRUE(CheckDirty(container));

    // The path should be set to the correct path data based on viewport
    auto path = container->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());
    ASSERT_TRUE(IsEqual(Object("M400,1600 L0,0"), path->getValue(kGraphicPropertyPathData)));
    ASSERT_TRUE(CheckDirty(path));

    // Change the state to pressed
    component->setState(kStatePressed, true);


    // The graphic itself should have a new viewport height and width
    ASSERT_EQ(100, graphic->getViewportWidth());
    ASSERT_EQ(100, graphic->getViewportHeight());

    // The container should have four updated values
    ASSERT_EQ(Object(Dimension(50)), container->getValue(kGraphicPropertyHeightActual));
    ASSERT_EQ(Object(Dimension(256)), container->getValue(kGraphicPropertyWidthActual));
    ASSERT_EQ(Object(100), container->getValue(kGraphicPropertyViewportHeightActual));
    ASSERT_EQ(Object(100), container->getValue(kGraphicPropertyViewportWidthActual));
    ASSERT_TRUE(CheckDirty(container, kGraphicPropertyHeightActual, kGraphicPropertyWidthActual,
        kGraphicPropertyViewportHeightActual, kGraphicPropertyViewportWidthActual));

    // The path should have an updated path data
    ASSERT_EQ(Object("M100,100 L0,0"), path->getValue(kGraphicPropertyPathData));
    ASSERT_TRUE(CheckDirty(path, kGraphicPropertyPathData));

    // Internal to the graphic the container and the path should be updated
    ASSERT_TRUE(CheckDirty(graphic, container, path));

    // The vector graphic component should have a new scale, alignment, and media bounds
    ASSERT_EQ(Rect(768, 375, 256, 50), component->getCalculated(kPropertyMediaBounds).getRect());  // Right-aligned
    ASSERT_TRUE(CheckDirty(component, kPropertyScale, kPropertyAlign, kPropertyMediaBounds, kPropertyGraphic));

    ASSERT_TRUE(CheckDirty(root, component));
}

static const char *RELAYOUT_TEST = R"(
{
  "type": "APL",
  "version": "1.0",
  "styles": {
    "frameStyle": {
      "values": [
        {
          "borderWidth": 0
        },
        {
          "when": "${state.pressed}",
          "borderWidth": 100
        }
      ]
    }
  },
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 100,
      "items": {
        "type": "path",
        "pathData": "M${width},${height} L0,0"
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Frame",
      "style": "frameStyle",
      "width": "100%",
      "height": "100%",
      "item": {
        "type": "VectorGraphic",
        "source": "box",
        "width": "100%",
        "height": "100%",
        "scale": "fill"
      }
    }
  }
})";


TEST_F(GraphicComponentTest, RelayoutTest)
{
    loadDocument(RELAYOUT_TEST);

    // The top component is a Frame
    ASSERT_EQ(kComponentTypeFrame, component->getType());
    ASSERT_EQ(Rect(0, 0, metrics.getWidth(), metrics.getHeight()), component->getGlobalBounds());
    ASSERT_EQ(Rect(0, 0, 1024, 800), component->getCalculated(kPropertyInnerBounds).getRect());

    auto vg = component->getChildAt(0);
    ASSERT_EQ(kComponentTypeVectorGraphic, vg->getType());
    ASSERT_EQ(Rect(0, 0, 1024, 800), vg->getCalculated(kPropertyMediaBounds).getRect());

    auto graphic = vg->getCalculated(kPropertyGraphic).getGraphic();
    ASSERT_TRUE(graphic);
    ASSERT_EQ(100, graphic->getViewportWidth());
    ASSERT_EQ(100, graphic->getViewportHeight());
    ASSERT_EQ(0, graphic->getDirty().size());

    // The top-level container has no properties
    auto container = graphic->getRoot();
    ASSERT_TRUE(container);
    ASSERT_EQ(kGraphicElementTypeContainer, container->getType());
    ASSERT_EQ(0, container->getDirtyProperties().size());

    // Change the state to pressed
    component->setState(kStatePressed, true);
    root->clearPending();  // Ensure that the layout has been updated

    // The vector graphic component has new, smaller media bounds
    ASSERT_EQ(Rect(0, 0, 824, 600), vg->getCalculated(kPropertyMediaBounds).getRect());
    ASSERT_EQ(Rect(100, 100, 824, 600), vg->getCalculated(kPropertyBounds).getRect());  // Bounds in parent
    // The kPropertyGraphic is marked as dirty.  That's not right - it's merely resized
    ASSERT_EQ(Rect(0, 0, 824, 600), vg->getCalculated(kPropertyInnerBounds).getRect());

    // The container should have four updated values
    ASSERT_EQ(Object(Dimension(600)), container->getValue(kGraphicPropertyHeightActual));
    ASSERT_EQ(Object(Dimension(824)), container->getValue(kGraphicPropertyWidthActual));
    ASSERT_EQ(Object(100), container->getValue(kGraphicPropertyViewportHeightActual));
    ASSERT_EQ(Object(100), container->getValue(kGraphicPropertyViewportWidthActual));
    ASSERT_TRUE(CheckDirty(container, kGraphicPropertyHeightActual, kGraphicPropertyWidthActual));

    // The border width has changed on the frame.
    ASSERT_EQ(Object(Dimension(100)), component->getCalculated(kPropertyBorderWidth));
    ASSERT_EQ(Rect(100, 100, 824, 600), component->getCalculated(kPropertyInnerBounds).getRect());
    ASSERT_TRUE(CheckDirty(component, kPropertyInnerBounds, kPropertyBorderWidth, kPropertyNotifyChildrenChanged));

    // The graphic itself should have a new viewport height and width
    ASSERT_EQ(100, graphic->getViewportWidth());
    ASSERT_EQ(100, graphic->getViewportHeight());
    ASSERT_TRUE(CheckDirty(graphic, container));

    // The root should be showing dirty for both the vector graphic component and the frame
    ASSERT_TRUE(CheckDirty(vg, kPropertyGraphic, kPropertyMediaBounds, kPropertyBounds, kPropertyInnerBounds));
    ASSERT_TRUE(CheckDirty(root, component, vg));
}

// Assign a vector graphic to a component

const static char * EMPTY_GRAPHIC = R"(
{
  "type": "APL",
  "version": "1.0",
  "styles": {
    "graphicStyle": {
      "values": [
        {
          "myColor": "blue"
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "style": "graphicStyle",
      "width": "100%",
      "height": "100%",
      "scale": "fill",
      "myLineWidth": 10
    }
  }
})";

const static char *STANDALONE_GRAPHIC = R"(
{
  "type": "AVG",
  "version": "1.0",
  "height": 100,
  "width": 100,
  "parameters": [
    "myColor",
    "myLineWidth"
  ],
  "items": {
    "type": "path",
    "pathData": "M0,0 h100 v100 h-100 z",
    "fill": "${myColor}",
    "strokeWidth": "${myLineWidth}"
  }
})";


TEST_F(GraphicComponentTest, AssignGraphicLater) {
    loadDocument(EMPTY_GRAPHIC);

    // The top component is the graphic, but there is no content
    ASSERT_EQ(kComponentTypeVectorGraphic, component->getType());
    ASSERT_EQ(Rect(0, 0, metrics.getWidth(), metrics.getHeight()), component->getGlobalBounds());
    ASSERT_EQ(Rect(0, 0, 1024, 800), component->getCalculated(kPropertyInnerBounds).getRect());
    ASSERT_EQ(Object::NULL_OBJECT(), component->getCalculated(kPropertyGraphic));
    ASSERT_EQ(Object(kVectorGraphicAlignCenter), component->getCalculated(kPropertyAlign));
    ASSERT_EQ(Object(kVectorGraphicScaleFill), component->getCalculated(kPropertyScale));

    ASSERT_TRUE(CheckDirty(component));

    auto json = GraphicContent::create(session, STANDALONE_GRAPHIC);
    ASSERT_TRUE(json);
    component->updateGraphic(json);
    root->clearPending();

    ASSERT_TRUE(CheckDirty(component, kPropertyGraphic, kPropertyMediaBounds));
    ASSERT_TRUE(CheckDirty(root, component));

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    auto top = graphic->getRoot();
    auto path = top->getChildAt(0);

    ASSERT_TRUE(CheckDirty(graphic));

    ASSERT_TRUE(CheckDirty(top));
    ASSERT_EQ(Object(100), top->getValue(kGraphicPropertyViewportWidthActual));
    ASSERT_EQ(Object(100), top->getValue(kGraphicPropertyViewportHeightActual));
    ASSERT_EQ(Object(Dimension(1024)), top->getValue(kGraphicPropertyWidthActual));
    ASSERT_EQ(Object(Dimension(800)), top->getValue(kGraphicPropertyHeightActual));

    ASSERT_TRUE(path);
    ASSERT_TRUE(IsEqual(Color(session, "blue"), path->getValue(kGraphicPropertyFill)));
    ASSERT_TRUE(IsEqual(10, path->getValue(kGraphicPropertyStrokeWidth)));

}

const char* PARAMETERS_DOC = R"(
{
    "type": "APL",
    "version": "1.0",
    "graphics": {
        "myPillShape": {
            "type": "AVG",
            "version": "1.0",
            "height": 100,
            "width": 100,
            "parameters": [
                "myScaleType"
            ],
            "scaleTypeHeight": "${myScaleType}",
            "items": [
                {
                    "type": "path",
                    "pathData": "M25,50 a25,25 0 1 1 50,0 l0 ${height-100} a25,25 0 1 1 -50,0 z",
                    "stroke": "black",
                    "strokeWidth": 20
                }
            ]
        }
    },
    "mainTemplate": {
        "item": {
            "type": "Container",
            "direction": "row",
            "items": {
                "type": "VectorGraphic",
                "source": "myPillShape",
                "width": 100,
                "height": 200,
                "scale": "fill",
                "myScaleType": "${data}"
            },
            "data": [
                "none",
                "stretch"
            ]
        }
    }
})";


TEST_F(GraphicComponentTest, GraphicParameter) {
    loadDocument(PARAMETERS_DOC);

    // The top component is the graphic, but there is no content
    ASSERT_EQ(kComponentTypeContainer, component->getType());
    ASSERT_EQ(2, component->getChildCount());
    auto none = component->getChildAt(0);
    auto stretch = component->getChildAt(1);

    auto obj = none->getCalculated(kPropertyGraphic);
    ASSERT_EQ(obj.getType(), Object::kGraphicType);
    auto graphic = obj.getGraphic();
    ASSERT_TRUE(graphic->getRoot() != nullptr);
    ASSERT_EQ(graphic->getRoot()->getChildCount(), 1);
    auto path = graphic->getRoot()->getChildAt(0);
    auto pathData = path->getValue(kGraphicPropertyPathData);
    ASSERT_EQ("M25,50 a25,25 0 1 1 50,0 l0 0 a25,25 0 1 1 -50,0 z", pathData.asString());

    obj = stretch->getCalculated(kPropertyGraphic);
    ASSERT_EQ(obj.getType(), Object::kGraphicType);
    graphic = obj.getGraphic();
    ASSERT_TRUE(graphic->getRoot() != nullptr);
    ASSERT_EQ(graphic->getRoot()->getChildCount(), 1);
    path = graphic->getRoot()->getChildAt(0);
    pathData = path->getValue(kGraphicPropertyPathData);
    ASSERT_EQ("M25,50 a25,25 0 1 1 50,0 l0 100 a25,25 0 1 1 -50,0 z", pathData.asString());

}

const char* FOCUS_AND_HOVER_STYLE = R"(
{
  "type": "APL",
  "version": "1.2",
  "theme": "dark",
  "styles": {
    "styleHoverable": {
      "values": [
        {
          "circleColor": "white"
        },
        {
          "when": "${state.hover}",
          "circleColor": "red"
        }
      ]
    }
  },
  "graphics": {
    "parameterizedCircle": {
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 100,
      "parameters": [
        {
          "name": "circleColor",
          "type": "color",
          "default": "black"
        },
        {
          "name": "circleBorderWidth",
          "type": "number",
          "default": 2
        }
      ],
      "items": [
        {
          "type": "path",
          "pathData": "M25,50 a25,25 0 1 1 50,0 a25,25 0 1 1 -50,0",
          "stroke": "${circleColor}",
          "strokeWidth": "${circleBorderWidth}"
        }
      ]
    }
  },
  "mainTemplate": {
    "item": {
      "type": "Container",
      "items": [
        {
          "type": "VectorGraphic",
          "positioning": "absolute",
          "top": 50,
          "left": 50,
          "source": "parameterizedCircle",
          "width": 100,
          "height": 100,
          "style": "styleHoverable",
          "circleBorderWidth": "5"
        }
      ]
    }
  }
})";

TEST_F(GraphicComponentTest, GraphicFocusAndHover) {
    loadDocument(FOCUS_AND_HOVER_STYLE);

    auto gc = component->getCoreChildAt(0);

    // The top component is the graphic, but there is no content
    ASSERT_EQ(kComponentTypeVectorGraphic, gc->getType());

    auto obj = gc->getCalculated(kPropertyGraphic);
    ASSERT_EQ(obj.getType(), Object::kGraphicType);
    auto graphic = obj.getGraphic();
    ASSERT_TRUE(graphic->getRoot() != nullptr);
    ASSERT_EQ(graphic->getRoot()->getChildCount(), 1);
    auto path = graphic->getRoot()->getChildAt(0);
    auto pathData = path->getValue(kGraphicPropertyPathData);
    ASSERT_EQ("M25,50 a25,25 0 1 1 50,0 a25,25 0 1 1 -50,0", pathData.asString());
    auto stroke = path->getValue(kGraphicPropertyStroke).asColor();
    ASSERT_EQ(Color(0xffffffff), stroke);

    // Hover on
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(75, 75)));
    root->clearPending();
    ASSERT_TRUE(CheckDirty(path, kGraphicPropertyStroke));
    ASSERT_TRUE(CheckDirty(gc, kPropertyGraphic));
    ASSERT_TRUE(CheckDirty(root, gc));
    stroke = path->getValue(kGraphicPropertyStroke).asColor();
    ASSERT_EQ(Color(0xff0000ff), stroke);

    // Hover off
    root->handlePointerEvent(PointerEvent(PointerEventType::kPointerMove, Point(200, 200)));
    root->clearPending();
    ASSERT_TRUE(CheckDirty(path, kGraphicPropertyStroke));
    ASSERT_TRUE(CheckDirty(gc, kPropertyGraphic));
    ASSERT_TRUE(CheckDirty(root, gc));
    stroke = path->getValue(kGraphicPropertyStroke).asColor();
    ASSERT_EQ(Color(0xffffffff), stroke);
}

static const char * SLIDER =
    R"(
{
  "type": "APL",
  "version": "1.4",
  "graphics": {
    "ToggleButton": {
      "type": "AVG",
      "version": "1.0",
      "parameters": [
        "ButtonPosition",
        "ShowButton"
      ],
      "width": 256,
      "height": 90,
      "scaleTypeWidth": "stretch",
      "items": [
        {
          "type": "path",
          "description": "Slider Background",
          "pathData": "M45,55 a10,10,0,0,1,0,-20 l${width-90},0 a10,10,0,0,1,0,20 Z",
          "stroke": "#979797",
          "fill": "#d8d8d8",
          "strokeWidth": 2,
          "opacity": 0.4
        },
        {
          "type": "path",
          "description": "Slider Fill",
          "pathData": "M45,55 a10,10,0,0,1,0,-20 l${ButtonPosition *(width-90)},0 a10,10,0,0,1,0,20 Z",
          "stroke": "#979797",
          "fill": "#88e",
          "strokeWidth": 2
        },
        {
          "type": "group",
          "description": "Button",
          "translateX": "${ButtonPosition * (width - 90)}",
          "opacity": "${ShowButton ? 1 : 0}",
          "items": {
            "type": "path",
            "pathData": "M45,82 a36,36,0,0,1,0,-76 a36,36,0,1,1,0,76 Z",
            "fill": "#88e",
            "stroke": "white",
            "strokeWidth": 6
          }
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Container",
      "items": [
        {
          "type": "VectorGraphic",
          "source": "ToggleButton",
          "id": "MySlider",
          "scale": "fill",
          "width": "590",
          "bind": [
            {
              "name": "Position",
              "value": 0.50
            },
            {
              "name": "OldPosition",
              "value": 0.50
            },
            {
              "name": "ShowButton",
              "value": false
            }
          ],
          "ButtonPosition": "${Position}",
          "ShowButton": "${ShowButton}",
          "onDown": [
            {
              "type": "SetValue",
              "property": "ShowButton",
              "value": true
            },
            {
              "type": "SetValue",
              "property": "OldPosition",
              "value": "${Position}"
            },
            {
              "type": "SetValue",
              "property": "Position",
              "value": "${Math.clamp(0, (event.viewport.x - 45) / (event.viewport.width - 90), 1)}"
            }
          ],
          "onUp": [
            {
              "type": "SetValue",
              "property": "ShowButton",
              "value": false
            },
            {
              "type": "SetValue",
              "description": "Reset the position if we release the pointer at some far location",
              "when": "${!event.inBounds}",
              "property": "Position",
              "value": "${OldPosition}"
            }
          ],
          "onMove": {
            "type": "SetValue",
            "property": "Position",
            "value": "${Math.clamp(0, (event.viewport.x - 45) / (event.viewport.width - 90), 1)}"
          }
        },
        {
          "type": "TouchWrapper",
          "id": "MyButton",
          "height": 20,
          "width": 30,
          "onDown": {
            "type": "SetValue",
            "componentId": "textComp",
            "property": "text",
            "value": "Down"
          },
          "onUp": {
            "type": "SetValue",
            "componentId": "textComp",
            "property": "text",
            "value": "Up"
          },
          "items": {
            "type": "Text",
            "id": "textComp",
            "text": "Nothing"
          }
        }
      ]
    }
  }
}
)";

TEST_F(GraphicComponentTest, TAP_TO_SLIDE) {
    loadDocument(SLIDER);
    auto slider = context->findComponentById("MySlider");
    auto text = context->findComponentById("textComp");

    // We deliberately set the width of the component to .  This places the slider
    // 590 dp in from the left with an effective slider width of 500.
    // Tapping on the left side should set the slider to 0.0
    performTap(0,0);
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    double position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0, position);

    // Tapping on the left side of the slider
    performTap(45,0);
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0, position);

    // Tapping on the right side of the slider
    performTap(545,0);
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(1, position);

    // Tapping on the far right side of the screen
    performTap(590,0);
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(1, position);

    // Tapping in the midpoint
    performTap(45 + 250,0);
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0.5, position);

    // Tapping 25% of the way in
    performTap(45 + 125,0);
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0.25, position);
}

TEST_F(GraphicComponentTest, MOVE_TO_SLIDE) {
    loadDocument(SLIDER);
    auto slider = context->findComponentById("MySlider");
    auto text = context->findComponentById("textComp");

    root->handlePointerEvent(PointerEvent(kPointerDown, Point(45, 0), 0, kTouchPointer));
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    double position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0, position);

    root->handlePointerEvent(PointerEvent(kPointerMove, Point(295, 0), 0, kTouchPointer));
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0.5, position);

    root->handlePointerEvent(PointerEvent(kPointerMove, Point(170, 0), 0, kTouchPointer));
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0.25, position);

    // out of bounds on up resets
    root->handlePointerEvent(PointerEvent(kPointerUp, Point(384, 380), 0, kTouchPointer));
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0.5, position);
}

TEST_F(GraphicComponentTest, NEVER_DOUBLE_DOWN) {
    loadDocument(SLIDER);
    auto slider = context->findComponentById("MySlider");
    auto text = context->findComponentById("textComp");

    root->handlePointerEvent(PointerEvent(kPointerDown, Point(45, 0), 0, kTouchPointer));
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    double position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0, position);

    root->handlePointerEvent(PointerEvent(kPointerDown, Point(295, 0), 0, kTouchPointer));
    ASSERT_FALSE(root->isDirty());
    root->clearDirty();
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0, position);
}

TEST_F(GraphicComponentTest, NEVER_TWO_DOWN_ON_SAME_TARGET) {
    loadDocument(SLIDER);
    auto slider = context->findComponentById("MySlider");
    auto text = context->findComponentById("textComp");

    root->handlePointerEvent(PointerEvent(kPointerDown, Point(45, 0), 0, kTouchPointer));
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    double position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0, position);

    root->handlePointerEvent(PointerEvent(kPointerDown, Point(295, 0), 1, kTouchPointer));
    ASSERT_FALSE(root->isDirty());
    root->clearDirty();
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0, position);
}

TEST_F(GraphicComponentTest, TWO_DOWN_ON_DIFFERENT_TARGET) {
    loadDocument(SLIDER);
    auto slider = context->findComponentById("MySlider");
    auto text = context->findComponentById("textComp");

    ASSERT_EQ("Nothing", text->getCalculated(kPropertyText).asString());

    root->handlePointerEvent(PointerEvent(kPointerDown, Point(45, 0), 0, kTouchPointer));
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    double position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0, position);

    root->handlePointerEvent(PointerEvent(kPointerDown, Point(0, 100), 1, kTouchPointer));
    ASSERT_FALSE(root->isDirty());
    root->clearDirty();
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ("Nothing", text->getCalculated(kPropertyText).asString());
    ASSERT_EQ(0, position);

    // Move the other pointer over the slider, verify the slider does not move
    root->handlePointerEvent(PointerEvent(kPointerMove, Point(250, 0), 1, kTouchPointer));
    ASSERT_FALSE(root->isDirty());
    root->clearDirty();
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ("Nothing", text->getCalculated(kPropertyText).asString());
    ASSERT_EQ(0, position);

    // Release the other pointer over the slider, verify the slider does not move
    root->handlePointerEvent(PointerEvent(kPointerUp, Point(250, 0), 1, kTouchPointer));
    ASSERT_FALSE(root->isDirty());
    root->clearDirty();
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ("Nothing", text->getCalculated(kPropertyText).asString());
    ASSERT_EQ(0, position);

    // Move the first pointer, verify the slider moves
    root->handlePointerEvent(PointerEvent(kPointerMove, Point(295, 0), 0, kTouchPointer));
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ("Nothing", text->getCalculated(kPropertyText).asString());
    ASSERT_EQ(0.5, position);

    // Move the first pointer over the button, verify the slider moves
    root->handlePointerEvent(PointerEvent(kPointerMove, Point(0, 100), 0, kTouchPointer));
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ("Nothing", text->getCalculated(kPropertyText).asString());
    ASSERT_EQ(0, position);

    // Release the first pointer, verify the slider pops back
    root->handlePointerEvent(PointerEvent(kPointerUp, Point(0, 100), 0, kTouchPointer));
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ("Nothing", text->getCalculated(kPropertyText).asString());
    ASSERT_EQ(0.5, position);
}

TEST_F(GraphicComponentTest, CANCELED_POINTERS_DONT_MOVE) {
    loadDocument(SLIDER);
    auto slider = context->findComponentById("MySlider");
    auto text = context->findComponentById("textComp");

    root->handlePointerEvent(PointerEvent(kPointerDown, Point(45, 0), 0, kTouchPointer));
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    double position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0, position);

    root->handlePointerEvent(PointerEvent(kPointerCancel, Point(295, 0), 0, kTouchPointer));
    ASSERT_FALSE(root->isDirty());
    root->clearDirty();
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0, position);

    root->handlePointerEvent(PointerEvent(kPointerMove, Point(295, 0), 0, kTouchPointer));
    ASSERT_FALSE(root->isDirty());
    root->clearDirty();
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0, position);
}

TEST_F(GraphicComponentTest, CANCELED_POINTERS_COME_BACK) {
    loadDocument(SLIDER);
    auto slider = context->findComponentById("MySlider");
    auto text = context->findComponentById("textComp");

    root->handlePointerEvent(PointerEvent(kPointerDown, Point(45, 0), 0, kTouchPointer));
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    double position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0, position);

    root->handlePointerEvent(PointerEvent(kPointerCancel, Point(295, 0), 0, kTouchPointer));
    ASSERT_FALSE(root->isDirty());
    root->clearDirty();
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0, position);

    root->handlePointerEvent(PointerEvent(kPointerDown, Point(170, 0), 0, kTouchPointer));
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0.25, position);

    root->handlePointerEvent(PointerEvent(kPointerMove, Point(295, 0), 0, kTouchPointer));
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0.5, position);
}

static const char *EXTERNAL_EXPANDED_STYLING_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "height": 100,
      "width": 100,
      "source": "box"
    }
  },
  "styles": {
    "base": {
      "values": [
        {
          "opacity": 0.7
        },
        {
          "opacity": 0.5,
          "when": "${state.disabled}"
        }
      ]
    }
  },
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 100,
      "styles": {
        "expanded": {
          "extends": "base",
          "values": [
            {
              "fill": "red"
            },
            {
              "fill": "blue",
              "when": "${state.disabled}"
            }
          ]
        }
      },
      "items": {
        "type": "group",
        "style": "expanded",
        "items": [
          {
            "type": "path",
            "style": "expanded",
            "stroke": "blue",
            "strokeWidth": 4,
            "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z"
          },
          {
            "type": "text",
            "style": "expanded",
            "fontFamily": "amazon-ember, sans-serif",
            "fontSize": 40,
            "text": "Diamond",
            "x": 25,
            "y": 25,
            "textAnchor": "middle"
          }
        ]
      }
    }
  }
})";

TEST_F(GraphicComponentTest, ExternalExpandedStyling)
{
    loadDocument(EXTERNAL_EXPANDED_STYLING_DOC);

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();

    auto group = graphic->getRoot()->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeGroup, group->getType());
    ASSERT_EQ(0.7, group->getValue(kGraphicPropertyOpacity).asNumber());

    auto path = group->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());
    ASSERT_EQ(Object(Color(Color::RED)), path->getValue(kGraphicPropertyFill));

    auto text = group->getChildAt(1);
    ASSERT_EQ(kGraphicElementTypeText, text->getType());
    ASSERT_EQ(Object(Color(Color::RED)), text->getValue(kGraphicPropertyFill));

    component->setState(StateProperty::kStateDisabled, true);

    ASSERT_EQ(0.5, group->getValue(kGraphicPropertyOpacity).asNumber());
    ASSERT_EQ(Object(Color(Color::BLUE)), path->getValue(kGraphicPropertyFill));
    ASSERT_EQ(Object(Color(Color::BLUE)), text->getValue(kGraphicPropertyFill));
}

static const char *STYLE_EVERYTHING_DOC = R"({
  "type": "APL",
  "version": "1.4",
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "height": 100,
      "width": 100,
      "source": "box"
    }
  },
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.1",
      "height": 100,
      "width": 100,
      "resources": [
        {
          "gradients": {
            "strokeGradient1": {
              "type": "linear",
              "colorRange": [ "blue", "white" ],
              "inputRange": [0, 1],
              "x1": 0.1,
              "y1": 0.2,
              "x2": 0.3,
              "y2": 0.4
            },
            "strokeGradient2": {
              "type": "linear",
              "colorRange": [ "green", "white" ],
              "inputRange": [0, 1],
              "spreadMethod": "repeat"
            }
          },
          "patterns": {
            "fillPattern1": {
              "height": 18,
              "width": 18,
              "item": {
                "type": "path",
                "pathData": "M0,9 a9,9 0 1 1 18,0 a9,9 0 1 1 -18,0",
                "fill": "red"
              }
            },
            "fillPattern2": {
              "height": 9,
              "width": 9,
              "item": {
                "type": "path",
                "pathData": "M0,9 a9,9 0 1 1 18,0 a9,9 0 1 1 -18,0",
                "fill": "blue"
              }
            }
          }
        }
      ],
      "styles": {
        "expanded": {
          "values": [
            {
              "clipPath": "M 50 0 L 100 50 L 50 100 L 0 50 z",
              "opacity": 0.7,
              "fill": "@fillPattern1",
              "fillOpacity": 0.9,
              "pathData": "M 50 0 L 100 50 L 50 100 L 0 50 z",
              "pathLength": 50,
              "stroke": "@strokeGradient1",
              "strokeDashArray": [1, 2, 3, 4],
              "strokeDashOffset": 2,
              "strokeLineCap": "round",
              "strokeLineJoin": "bevel",
              "strokeMiterLimit": 3,
              "strokeOpacity": 1.0,
              "strokeWidth": 4,
              "fontFamily": "sans-serif",
              "fontSize": "40",
              "fontStyle": "italic",
              "fontWeight": "bold",
              "letterSpacing": 1,
              "text": "Texty text",
              "textAnchor": "start",
              "x": 2,
              "y": 3,
              "rotation": 5,
              "fillTransform": "translate(-36 45.5) ",
              "strokeTransform": "skewY(5) "
            },
            {
              "clipPath": "M 25 0 L 50 25 L 25 50 L 0 25 z",
              "opacity": 0.5,
              "fill": "@fillPattern2",
              "fillOpacity": 0.8,
              "pathData": "M 25 0 L 50 25 L 25 50 L 0 25 z",
              "pathLength": 40,
              "stroke": "@strokeGradient2",
              "strokeDashArray": [2, 1, 4, 3],
              "strokeDashOffset": 1,
              "strokeLineCap": "square",
              "strokeLineJoin": "miter",
              "strokeMiterLimit": 2,
              "strokeOpacity": 0.9,
              "strokeWidth": 2,
              "fontFamily": "funky",
              "fontSize": "35",
              "fontStyle": "normal",
              "fontWeight": "normal",
              "letterSpacing": 2,
              "text": "Less texty text",
              "textAnchor": "middle",
              "x": 5,
              "y": 7,
              "transform": "rotate(-10 50 75) ",
              "fillTransform": "translate(-36 45.5) skewX(40) ",
              "strokeTransform": "skewY(5) scale(0.7 0.5) ",
              "when": "${state.disabled}"
            }
          ]
        }
      },
      "items": {
        "type": "group",
        "style": "expanded",
        "items": [
          {
            "type": "path",
            "style": "expanded"
          },
          {
            "type": "text",
            "style": "expanded"
          }
        ]
      }
    }
  }
})";

TEST_F(GraphicComponentTest, StyleEverything)
{
    loadDocument(STYLE_EVERYTHING_DOC);

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();

    auto group = graphic->getRoot()->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypeGroup, group->getType());
    ASSERT_EQ(0.7, group->getValue(kGraphicPropertyOpacity).asNumber());
    ASSERT_EQ("M 50 0 L 100 50 L 50 100 L 0 50 z", group->getValue(kGraphicPropertyClipPath).asString());
    ASSERT_EQ(Transform2D::rotate(5), group->getValue(kGraphicPropertyTransform).getTransform2D());


    auto fillTransform = Transform2D::translate(-36, 45.5);
    auto strokeTransform = Transform2D::skewY(5);
    auto path = group->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());

    auto fill = path->getValue(kGraphicPropertyFill);
    ASSERT_TRUE(fill.isGraphicPattern());
    auto fillPattern = fill.getGraphicPattern();
    auto fillPatternPath = fillPattern->getItems().at(0);
    ASSERT_EQ(kGraphicElementTypePath, fillPatternPath->getType());
    ASSERT_EQ(Color(Color::RED), fillPatternPath->getValue(kGraphicPropertyFill).asColor());

    ASSERT_EQ(0.9, path->getValue(kGraphicPropertyFillOpacity).asNumber());
    ASSERT_EQ("M 50 0 L 100 50 L 50 100 L 0 50 z", path->getValue(kGraphicPropertyPathData).asString());
    ASSERT_EQ(50, path->getValue(kGraphicPropertyPathLength).asNumber());

    ASSERT_TRUE(path->getValue(kGraphicPropertyStroke).isGradient());
    auto stroke = path->getValue(kGraphicPropertyStroke).getGradient();
    ASSERT_EQ(Gradient::LINEAR, stroke.getProperty(kGradientPropertyType).asInt());
    auto colorRange = stroke.getProperty(kGradientPropertyColorRange);
    ASSERT_EQ(2, colorRange.size());
    ASSERT_EQ(Color(Color::BLUE), colorRange.at(0).asColor());
    ASSERT_EQ(Color(Color::WHITE), colorRange.at(1).asColor());

    auto inputRange = stroke.getProperty(kGradientPropertyInputRange);
    ASSERT_EQ(2, inputRange.size());
    ASSERT_EQ(0, inputRange.at(0).asNumber());
    ASSERT_EQ(1, inputRange.at(1).asNumber());

    auto spreadMethod = stroke.getProperty(kGradientPropertySpreadMethod);
    ASSERT_EQ(Gradient::PAD, static_cast<Gradient::GradientSpreadMethod>(spreadMethod.asInt()));

    ASSERT_EQ(0.1, stroke.getProperty(kGradientPropertyX1).asNumber());
    ASSERT_EQ(0.2, stroke.getProperty(kGradientPropertyY1).asNumber());
    ASSERT_EQ(0.3, stroke.getProperty(kGradientPropertyX2).asNumber());
    ASSERT_EQ(0.4, stroke.getProperty(kGradientPropertyY2).asNumber());

    ASSERT_EQ(std::vector<Object>({1,2,3,4}), path->getValue(kGraphicPropertyStrokeDashArray).getArray());
    ASSERT_EQ(2, path->getValue(kGraphicPropertyStrokeDashOffset).asNumber());
    ASSERT_EQ(kGraphicLineCapRound, path->getValue(kGraphicPropertyStrokeLineCap).asInt());
    ASSERT_EQ(kGraphicLineJoinBevel, path->getValue(kGraphicPropertyStrokeLineJoin).asInt());
    ASSERT_EQ(3, path->getValue(kGraphicPropertyStrokeMiterLimit).asNumber());
    ASSERT_EQ(1.0, path->getValue(kGraphicPropertyStrokeOpacity).asNumber());
    ASSERT_EQ(4, path->getValue(kGraphicPropertyStrokeWidth).asNumber());
    ASSERT_EQ(fillTransform, path->getValue(kGraphicPropertyFillTransform).getTransform2D());
    ASSERT_EQ(strokeTransform, path->getValue(kGraphicPropertyStrokeTransform).getTransform2D());


    auto text = group->getChildAt(1);
    ASSERT_EQ(kGraphicElementTypeText, text->getType());

    fill = text->getValue(kGraphicPropertyFill);
    ASSERT_TRUE(fill.isGraphicPattern());
    fillPattern = fill.getGraphicPattern();
    fillPatternPath = fillPattern->getItems().at(0);
    ASSERT_EQ(kGraphicElementTypePath, fillPatternPath->getType());
    ASSERT_EQ(Color(Color::RED), fillPatternPath->getValue(kGraphicPropertyFill).asColor());

    ASSERT_EQ(0.9, text->getValue(kGraphicPropertyFillOpacity).asNumber());
    ASSERT_EQ("sans-serif", text->getValue(kGraphicPropertyFontFamily).asString());
    ASSERT_EQ(40, text->getValue(kGraphicPropertyFontSize).asNumber());
    ASSERT_EQ(kFontStyleItalic, text->getValue(kGraphicPropertyFontStyle).asInt());
    ASSERT_EQ(700, text->getValue(kGraphicPropertyFontWeight).asNumber());
    ASSERT_EQ(1, text->getValue(kGraphicPropertyLetterSpacing).asNumber());
    ASSERT_EQ("Texty text", text->getValue(kGraphicPropertyText).asString());
    ASSERT_TRUE(text->getValue(kGraphicPropertyStroke).isGradient());
    ASSERT_EQ(1.0, text->getValue(kGraphicPropertyStrokeOpacity).asNumber());
    ASSERT_EQ(4, text->getValue(kGraphicPropertyStrokeWidth).asNumber());
    ASSERT_EQ(kGraphicTextAnchorStart, text->getValue(kGraphicPropertyTextAnchor).asInt());
    ASSERT_EQ(2, text->getValue(kGraphicPropertyCoordinateX).asNumber());
    ASSERT_EQ(3, text->getValue(kGraphicPropertyCoordinateY).asNumber());
    ASSERT_EQ(fillTransform, text->getValue(kGraphicPropertyFillTransform).getTransform2D());
    ASSERT_EQ(strokeTransform, text->getValue(kGraphicPropertyStrokeTransform).getTransform2D());

    component->setState(StateProperty::kStateDisabled, true);

    ASSERT_TRUE(CheckDirty(group, kGraphicPropertyOpacity, kGraphicPropertyClipPath, kGraphicPropertyTransform));
    ASSERT_TRUE(CheckDirty(path, kGraphicPropertyFill, kGraphicPropertyFillOpacity, kGraphicPropertyPathData,
            kGraphicPropertyPathLength, kGraphicPropertyStroke, kGraphicPropertyStrokeDashArray,
            kGraphicPropertyStrokeDashOffset, kGraphicPropertyStrokeLineCap, kGraphicPropertyStrokeLineJoin,
            kGraphicPropertyStrokeMiterLimit, kGraphicPropertyStrokeOpacity, kGraphicPropertyStrokeWidth,
            kGraphicPropertyFillTransform, kGraphicPropertyStrokeTransform));
    ASSERT_TRUE(CheckDirty(text, kGraphicPropertyFill, kGraphicPropertyFillOpacity, kGraphicPropertyFontFamily,
            kGraphicPropertyFontSize, kGraphicPropertyFontStyle, kGraphicPropertyFontWeight,
            kGraphicPropertyLetterSpacing, kGraphicPropertyText, kGraphicPropertyStroke, kGraphicPropertyStrokeOpacity,
            kGraphicPropertyStrokeWidth, kGraphicPropertyTextAnchor, kGraphicPropertyCoordinateX,
            kGraphicPropertyCoordinateY, kGraphicPropertyFillTransform, kGraphicPropertyStrokeTransform));

    ASSERT_EQ(0.5, group->getValue(kGraphicPropertyOpacity).asNumber());
    ASSERT_EQ("M 25 0 L 50 25 L 25 50 L 0 25 z", group->getValue(kGraphicPropertyClipPath).asString());
    Transform2D transform;
    transform *= Transform2D::translate(50, 75);
    transform *= Transform2D::rotate(-10);
    transform *= Transform2D::translate(-50, -75);

    ASSERT_EQ(transform, group->getValue(kGraphicPropertyTransform).getTransform2D());


    fill = path->getValue(kGraphicPropertyFill);
    ASSERT_TRUE(fill.isGraphicPattern());
    fillPattern = fill.getGraphicPattern();
    fillPatternPath = fillPattern->getItems().at(0);
    ASSERT_EQ(kGraphicElementTypePath, fillPatternPath->getType());
    ASSERT_EQ(Color(Color::BLUE), fillPatternPath->getValue(kGraphicPropertyFill).asColor());

    ASSERT_EQ(0.8, path->getValue(kGraphicPropertyFillOpacity).asNumber());
    ASSERT_EQ("M 25 0 L 50 25 L 25 50 L 0 25 z", path->getValue(kGraphicPropertyPathData).asString());
    ASSERT_EQ(40, path->getValue(kGraphicPropertyPathLength).asNumber());

    ASSERT_TRUE(path->getValue(kGraphicPropertyStroke).isGradient());
    stroke = path->getValue(kGraphicPropertyStroke).getGradient();
    ASSERT_EQ(Gradient::LINEAR, stroke.getProperty(kGradientPropertyType).asInt());
    colorRange = stroke.getProperty(kGradientPropertyColorRange);
    ASSERT_EQ(2, colorRange.size());
    ASSERT_EQ(Color(Color::GREEN), colorRange.at(0).asColor());
    ASSERT_EQ(Color(Color::WHITE), colorRange.at(1).asColor());

    inputRange = stroke.getProperty(kGradientPropertyInputRange);
    ASSERT_EQ(2, inputRange.size());
    ASSERT_EQ(0, inputRange.at(0).asNumber());
    ASSERT_EQ(1, inputRange.at(1).asNumber());

    spreadMethod = stroke.getProperty(kGradientPropertySpreadMethod);
    ASSERT_EQ(Gradient::REPEAT, static_cast<Gradient::GradientSpreadMethod>(spreadMethod.asInt()));

    ASSERT_EQ(0, stroke.getProperty(kGradientPropertyX1).asNumber());
    ASSERT_EQ(0, stroke.getProperty(kGradientPropertyY1).asNumber());
    ASSERT_EQ(1, stroke.getProperty(kGradientPropertyX2).asNumber());
    ASSERT_EQ(1, stroke.getProperty(kGradientPropertyY2).asNumber());

    ASSERT_EQ(std::vector<Object>({2,1,4,3}), path->getValue(kGraphicPropertyStrokeDashArray).getArray());
    ASSERT_EQ(1, path->getValue(kGraphicPropertyStrokeDashOffset).asNumber());
    ASSERT_EQ(kGraphicLineCapSquare, path->getValue(kGraphicPropertyStrokeLineCap).asInt());
    ASSERT_EQ(kGraphicLineJoinMiter, path->getValue(kGraphicPropertyStrokeLineJoin).asInt());
    ASSERT_EQ(2, path->getValue(kGraphicPropertyStrokeMiterLimit).asNumber());
    ASSERT_EQ(0.9, path->getValue(kGraphicPropertyStrokeOpacity).asNumber());
    ASSERT_EQ(2, path->getValue(kGraphicPropertyStrokeWidth).asNumber());
    fillTransform *= Transform2D::skewX(40);
    strokeTransform *= Transform2D::scale(0.7, 0.5);
    ASSERT_EQ(fillTransform, path->getValue(kGraphicPropertyFillTransform).getTransform2D());
    ASSERT_EQ(strokeTransform, path->getValue(kGraphicPropertyStrokeTransform).getTransform2D());


    fill = text->getValue(kGraphicPropertyFill);
    ASSERT_TRUE(fill.isGraphicPattern());
    fillPattern = fill.getGraphicPattern();
    fillPatternPath = fillPattern->getItems().at(0);
    ASSERT_EQ(kGraphicElementTypePath, fillPatternPath->getType());
    ASSERT_EQ(Color(Color::BLUE), fillPatternPath->getValue(kGraphicPropertyFill).asColor());

    ASSERT_EQ(0.8, text->getValue(kGraphicPropertyFillOpacity).asNumber());
    ASSERT_EQ("funky", text->getValue(kGraphicPropertyFontFamily).asString());
    ASSERT_EQ(35, text->getValue(kGraphicPropertyFontSize).asNumber());
    ASSERT_EQ(kFontStyleNormal, text->getValue(kGraphicPropertyFontStyle).asInt());
    ASSERT_EQ(400, text->getValue(kGraphicPropertyFontWeight).asNumber());
    ASSERT_EQ(2, text->getValue(kGraphicPropertyLetterSpacing).asNumber());
    ASSERT_EQ("Less texty text", text->getValue(kGraphicPropertyText).asString());
    ASSERT_TRUE(text->getValue(kGraphicPropertyStroke).isGradient());
    ASSERT_EQ(0.9, text->getValue(kGraphicPropertyStrokeOpacity).asNumber());
    ASSERT_EQ(2, text->getValue(kGraphicPropertyStrokeWidth).asNumber());
    ASSERT_EQ(kGraphicTextAnchorMiddle, text->getValue(kGraphicPropertyTextAnchor).asInt());
    ASSERT_EQ(5, text->getValue(kGraphicPropertyCoordinateX).asNumber());
    ASSERT_EQ(7, text->getValue(kGraphicPropertyCoordinateY).asNumber());
    ASSERT_EQ(fillTransform, text->getValue(kGraphicPropertyFillTransform).getTransform2D());
    ASSERT_EQ(strokeTransform, text->getValue(kGraphicPropertyStrokeTransform).getTransform2D());

}

static const char *TRANSFORM_IN_PATTERN = R"({
  "type": "APL",
  "version": "1.4",
  "graphics": {
    "hasPatternWithGroupOfElements": {
      "type": "AVG",
      "version": "1.1",
      "height": "150",
      "width": "150",
      "resources": [
        {
          "patterns": {
            "GraphicElementComboPattern": {
              "width": 50,
              "height": 50,
              "items": [
                {
                  "type": "group",
                  "transform": "rotate(90) ",
                  "items": [
                    {
                      "type": "path",
                      "stroke": "yellow",
                      "pathData": "M5,5, h20",
                      "strokeLineCap": "round",
                      "strokeWidth": 10
                    },
                    {
                      "type": "text",
                      "fill": "red",
                      "text": "hello AVG",
                      "y": 20,
                      "fontSize": 40,
                      "strokeWidth": 0
                    }
                  ]
                },
                {
                  "type": "path",
                  "stroke": "orange",
                  "strokeTransform": "rotate(7) ",
                  "fillTransform": "rotate(8) ",
                  "pathData": "M5,5, h20",
                  "strokeLineCap": "round",
                  "strokeWidth": 3
                }
              ]
            }
          }
        }
      ],
      "items": [
        {
          "type": "path",
          "pathData": "M5,5 h${width-10} v${height-10} h${-width+10}z",
          "strokeWidth": 10,
          "fill": "@GraphicElementComboPattern"
        }
      ]
    }
  },
  "mainTemplate": {
    "item": {
      "type": "VectorGraphic",
      "paddingTop": "10",
      "paddingBottom": "10",
      "source": "hasPatternWithGroupOfElements"
    }
  }
})";



TEST_F(GraphicComponentTest, TransformInPattern)
{
    loadDocument(TRANSFORM_IN_PATTERN);

    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();

    auto path = graphic->getRoot()->getChildAt(0);
    ASSERT_EQ(kGraphicElementTypePath, path->getType());

    auto strokePattern =  path->getValue(kGraphicPropertyFill);
    ASSERT_TRUE(strokePattern.isGraphicPattern());

    auto strokePatternGroup = strokePattern.getGraphicPattern()->getItems().at(0);
    ASSERT_EQ(kGraphicElementTypeGroup, strokePatternGroup->getType());
    ASSERT_EQ(Object(Transform2D::rotate(90)), strokePatternGroup->getValue(kGraphicPropertyTransform));

    auto strokePatternPath = strokePattern.getGraphicPattern()->getItems().at(1);
    ASSERT_EQ(kGraphicElementTypePath, strokePatternPath->getType());
    ASSERT_EQ(Object(Transform2D::rotate(7)), strokePatternPath->getValue(kGraphicPropertyStrokeTransform));
    ASSERT_EQ(Object(Transform2D::rotate(8)), strokePatternPath->getValue(kGraphicPropertyFillTransform));
}

static const char * SIMPLE_PRESS = R"(
{
  "type": "APL",
  "version": "1.0",
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 100,
      "parameters": [
        "graphicText"
      ],
      "items": {
        "type": "text",
        "text": "${graphicText}"
      }
    }
  },
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "source": "box",
      "bind": {
        "name": "boxText",
        "value": "init"
      },
      "graphicText": "${boxText}",
      "onPress": {
        "type": "SetValue",
        "property": "boxText",
        "value": "${boxText}Press"
      },
      "onUp": {
        "type": "SetValue",
        "property": "boxText",
        "value": "${boxText}Up"
      },
      "onDown": {
        "type": "SetValue",
        "property": "boxText",
        "value": "${boxText}Down"
      }
    }
  }
})";

TEST_F(GraphicComponentTest, KeyboardPress) {
    loadDocument(SIMPLE_PRESS);
    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    auto container = graphic->getRoot();
    ASSERT_TRUE(container);
    ASSERT_EQ(1, container->getChildCount());
    auto textGraphic = container->getChildAt(0);
    ASSERT_FALSE(component->getState().get(kStateFocused));

    auto& fm = root->context().focusManager();
    ASSERT_FALSE(fm.getFocus());

    fm.setFocus(component, true);
    ASSERT_TRUE(component->getState().get(kStateFocused));
    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();

    root->handleKeyboard(kKeyDown, Keyboard::ENTER_KEY());
    ASSERT_FALSE(root->isDirty());
    ASSERT_FALSE(root->hasEvent());
    root->clearDirty();
    auto text = component->getContext()->find("boxText").object().value().getString();

    ASSERT_EQ(kGraphicElementTypeText, textGraphic->getType());

    ASSERT_EQ("init", textGraphic->getValue(kGraphicPropertyText).asString());
    ASSERT_EQ("init", text);

    root->handleKeyboard(kKeyUp, Keyboard::ENTER_KEY());
    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(root->isDirty());
    root->clearDirty();
    text = component->getContext()->find("boxText").object().value().getString();
    ASSERT_EQ("initPress", textGraphic->getValue(kGraphicPropertyText).asString());
    ASSERT_EQ("initPress", text);
}

TEST_F(GraphicComponentTest, KeyboardPressNoFocus) {
    loadDocument(SIMPLE_PRESS);
    auto graphic = component->getCalculated(kPropertyGraphic).getGraphic();
    auto container = graphic->getRoot();
    ASSERT_TRUE(container);
    ASSERT_EQ(1, container->getChildCount());
    auto textGraphic = container->getChildAt(0);
    ASSERT_FALSE(component->getState().get(kStateFocused));

    auto& fm = root->context().focusManager();
    ASSERT_FALSE(fm.getFocus());

    ASSERT_FALSE(component->getState().get(kStateFocused));

    root->handleKeyboard(kKeyDown, Keyboard::ENTER_KEY());
    ASSERT_FALSE(root->isDirty());
    ASSERT_FALSE(root->hasEvent());;
    auto text = component->getContext()->find("boxText").object().value().getString();

    ASSERT_EQ(kGraphicElementTypeText, textGraphic->getType());

    ASSERT_EQ("init", textGraphic->getValue(kGraphicPropertyText).asString());
    ASSERT_EQ("init", text);

    root->handleKeyboard(kKeyUp, Keyboard::ENTER_KEY());
    ASSERT_FALSE(root->hasEvent());
    ASSERT_FALSE(root->isDirty());
    text = component->getContext()->find("boxText").object().value().getString();
    ASSERT_EQ("init", textGraphic->getValue(kGraphicPropertyText).asString());
    ASSERT_EQ("init", text);
}

static const char * TOUCH_COORDINATES = R"(
{
  "type": "APL",
  "version": "1.4",
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 100
    }
  },
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "align": "top-left",
      "paddingLeft": 10,
      "paddingRight": 10,
      "paddingTop": 10,
      "paddingBottom": 10,
      "source": "box",
      "width": 220,
      "height": 70,
      "onDown": {
        "type": "SendEvent",
        "sequencer": "foo",
        "arguments": [
          "${event.viewport.x}",
          "${event.viewport.y}",
          "${event.viewport.width}",
          "${event.viewport.height}",
          "${event.viewport.inBounds}",
          "${event.component.x}",
          "${event.component.y}",
          "${event.component.width}",
          "${event.component.height}"
        ]
      }
    }
  }
})";

/**
 * Check touch event coordinates in a vector graphic.  In this test we verify
 * that an unscaled graphic placed inside a VectorGraphic component correctly
 * receives touch events offset by the padding of the VectorGraphic component.
 */
TEST_F(GraphicComponentTest, TouchCoordinates) {
    loadDocument(TOUCH_COORDINATES);

    // Click on the top-left corner
    root->handlePointerEvent(PointerEvent(kPointerDown, Point(10,10)));
    ASSERT_TRUE(CheckSendEvent(root,
                               0, 0, 100, 100, true,
                               10, 10, 220, 70));

    // We can't reach the bottom-right corner, so move over a few points and see if we scale properly
    root->handlePointerEvent(PointerEvent(kPointerUp, Point(10,10)));
    root->handlePointerEvent(PointerEvent(kPointerDown, Point(50,50)));
    ASSERT_TRUE(CheckSendEvent(root,
                               40, 40, 100, 100, true,
                               50, 50, 220, 70));

    // This click should fall OUTSIDE of the viewport
    root->handlePointerEvent(PointerEvent(kPointerUp, Point(50,50)));
    root->handlePointerEvent(PointerEvent(kPointerDown, Point(200,50)));
    ASSERT_TRUE(CheckSendEvent(root,
                               190, 40, 100, 100, false,
                               200, 50, 220, 70));
}


static const char * TOUCH_COORDINATES_FIT = R"(
{
  "type": "APL",
  "version": "1.4",
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 100
    }
  },
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "scale": "best-fit",
      "paddingLeft": 10,
      "paddingRight": 10,
      "paddingTop": 10,
      "paddingBottom": 10,
      "source": "box",
      "width": 220,
      "height": 70,
      "onDown": {
        "type": "SendEvent",
        "sequencer": "foo",
        "arguments": [
          "${event.viewport.x}",
          "${event.viewport.y}",
          "${event.viewport.width}",
          "${event.viewport.height}",
          "${event.viewport.inBounds}",
          "${event.component.x}",
          "${event.component.y}",
          "${event.component.width}",
          "${event.component.height}"
        ]
      }
    }
  }
})";

/**
 * The graphic will be scaled and offset in the vector graphic
 *
 * The graphic is 50 by 50 dp and centered, putting the
 * top-left at (85,10) with a scale factor of 50%
 */
TEST_F(GraphicComponentTest, TouchCoordinatesFit) {
    loadDocument(TOUCH_COORDINATES_FIT);

    // Click on the top-left corner
    root->handlePointerEvent(PointerEvent(kPointerDown, Point(85,10)));
    ASSERT_TRUE(CheckSendEvent(root,
                               0, 0, 100, 100, true,
                               85, 10, 220, 70));

    root->handlePointerEvent(PointerEvent(kPointerUp, Point(85,10)));

    // Click on the bottom-right corner
    root->handlePointerEvent(PointerEvent(kPointerDown, Point(85+50,10+50)));
    ASSERT_TRUE(CheckSendEvent(root,
                               100, 100, 100, 100, true,
                               135, 60, 220, 70));
}



static const char * TOUCH_COORDINATES_FILL_ALIGN = R"(
{
  "type": "APL",
  "version": "1.4",
  "graphics": {
    "box": {
      "type": "AVG",
      "version": "1.0",
      "height": 100,
      "width": 100
    }
  },
  "mainTemplate": {
    "items": {
      "type": "VectorGraphic",
      "scale": "best-fill",
      "align": "bottom-right",
      "paddingLeft": 10,
      "paddingRight": 10,
      "paddingTop": 10,
      "paddingBottom": 10,
      "source": "box",
      "width": 220,
      "height": 70,
      "onDown": {
        "type": "SendEvent",
        "sequencer": "foo",
        "arguments": [
          "${event.viewport.x}",
          "${event.viewport.y}",
          "${event.viewport.width}",
          "${event.viewport.height}",
          "${event.viewport.inBounds}",
          "${event.component.x}",
          "${event.component.y}",
          "${event.component.width}",
          "${event.component.height}"
        ]
      }
    }
  }
})";

/**
 * The graphic will be scaled and offset in the vector graphic
 *
 * The internals of the VectorGraphic 200 x 50 dp (10dp padding all sides).  The graphic is square, so
 * it will be scaled to 200 x 200 dp so that it fills the VectorGraphic and scales normally.  The
 * alignment is bottom-right, which puts the bottom-right corner at 210,60 and the top-left
 * corner at 10,-140 with a scaling factor of x2.
 */
TEST_F(GraphicComponentTest, TouchCoordinatesFillAlign) {
    loadDocument(TOUCH_COORDINATES_FILL_ALIGN);

    // The top-left corner is not visible, so click at 10,0 which maps to 0,70
    root->handlePointerEvent(PointerEvent(kPointerDown, Point(10, 0)));
    ASSERT_TRUE(CheckSendEvent(root,
                               0, 70, 100, 100, true,
                               10, 0, 220, 70));

    root->handlePointerEvent(PointerEvent(kPointerUp, Point(85,10)));

    // Click on the bottom-right corner
    root->handlePointerEvent(PointerEvent(kPointerDown, Point(210,60)));
    ASSERT_TRUE(CheckSendEvent(root,
                               100, 100, 100, 100, true,
                               210, 60, 220, 70));
}

static const char * INHERITED_TOUCH = R"(
{
  "type": "APL",
  "version": "1.4",
  "graphics": {
    "ToggleButton": {
      "type": "AVG",
      "version": "1.0",
      "parameters": [
        "On"
      ],
      "width": 150,
      "height": 90,
      "items": [
        {
          "type": "path",
          "description": "Background shape",
          "pathData": "M45,88 A43,43,0,0,1,45,2 L105,2 A43,43,0,0,1,105,88 Z",
          "stroke": "#979797",
          "fill": "${On ? 'green' : '#d8d8d8' }",
          "strokeWidth": 2
        },
        {
          "type": "group",
          "description": "Button",
          "translateX": "${On ? 60: 0}",
          "items": {
            "type": "path",
            "pathData": "M45,82 A36,36,0,0,1,45,8 A36,36,0,1,1,45,82 Z",
            "fill": "white",
            "stroke": "#979797",
            "strokeWidth": 2
          }
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "TouchWrapper",
      "bind": {
        "name": "IsOn",
        "value": false
      },
      "onPress": [
        {
          "type": "SetValue",
          "property": "IsOn",
          "value": "${!IsOn}"
        },
        {
          "type": "SendEvent"
        }
      ],
      "items": [
        {
          "type": "VectorGraphic",
          "source": "ToggleButton",
          "On": "${IsOn}",
          "inheritParentState": true
        }
      ]
    }
  }
})";

TEST_F(GraphicComponentTest, InheritedTouchBubbles) {
    loadDocument(INHERITED_TOUCH);

    ASSERT_TRUE(component->getCalculated(kPropertyFocusable).getBoolean());
    auto vg = component->getCoreChildAt(0);
    ASSERT_FALSE(vg->getCalculated(kPropertyFocusable).getBoolean());

    root->handlePointerEvent(PointerEvent(kPointerDown, Point(75, 45)));
    root->handlePointerEvent(PointerEvent(kPointerUp, Point(75, 45)));

    ASSERT_TRUE(root->hasEvent());
    auto event = root->popEvent();
    ASSERT_EQ(kEventTypeSendEvent, event.getType());
}

static const char * INHERITED_NOT_BUBBLED_TOUCH = R"(
{
  "type": "APL",
  "version": "1.6",
  "graphics": {
    "ToggleButton": {
      "type": "AVG",
      "version": "1.0",
      "parameters": [
        "On"
      ],
      "width": 150,
      "height": 90,
      "items": [
        {
          "type": "path",
          "description": "Background shape",
          "pathData": "M45,88 A43,43,0,0,1,45,2 L105,2 A43,43,0,0,1,105,88 Z",
          "stroke": "#979797",
          "fill": "${On ? 'green' : '#d8d8d8' }",
          "strokeWidth": 2
        },
        {
          "type": "group",
          "description": "Button",
          "translateX": "${On ? 60: 0}",
          "items": {
            "type": "path",
            "pathData": "M45,82 A36,36,0,0,1,45,8 A36,36,0,1,1,45,82 Z",
            "fill": "white",
            "stroke": "#979797",
            "strokeWidth": 2
          }
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "TouchWrapper",
      "bind": {
        "name": "IsOn",
        "value": false
      },
      "onPress": [
        {
          "type": "SetValue",
          "property": "IsOn",
          "value": "${!IsOn}"
        },
        {
          "type": "SendEvent"
        }
      ],
      "items": [
        {
          "type": "VectorGraphic",
          "source": "ToggleButton",
          "On": "${IsOn}",
          "inheritParentState": true,
          "onPress": [
            {
              "type": "SetValue",
              "property": "On",
              "value": "${!On}"
            }
          ]
        }
      ]
    }
  }
})";

TEST_F(GraphicComponentTest, InheritedTouchNotBubbles) {
    loadDocument(INHERITED_NOT_BUBBLED_TOUCH);

    ASSERT_TRUE(component->getCalculated(kPropertyFocusable).getBoolean());
    auto vg = component->getCoreChildAt(0);
    ASSERT_TRUE(vg->getCalculated(kPropertyFocusable).getBoolean());

    root->handlePointerEvent(PointerEvent(kPointerDown, Point(75, 45)));
    root->handlePointerEvent(PointerEvent(kPointerUp, Point(75, 45)));

    ASSERT_FALSE(root->hasEvent());
    ASSERT_TRUE(ConsoleMessage());
}

static const char * SLIDER_DISABLED = R"(
{
  "type": "APL",
  "version": "1.4",
  "graphics": {
    "ToggleButton": {
      "type": "AVG",
      "version": "1.0",
      "parameters": [
        "ButtonPosition",
        "ShowButton"
      ],
      "width": 256,
      "height": 90,
      "scaleTypeWidth": "stretch",
      "items": [
        {
          "type": "path",
          "description": "Slider Background",
          "pathData": "M45,55 a10,10,0,0,1,0,-20 l${width-90},0 a10,10,0,0,1,0,20 Z",
          "stroke": "#979797",
          "fill": "#d8d8d8",
          "strokeWidth": 2,
          "opacity": 0.4
        },
        {
          "type": "path",
          "description": "Slider Fill",
          "pathData": "M45,55 a10,10,0,0,1,0,-20 l${ButtonPosition *(width-90)},0 a10,10,0,0,1,0,20 Z",
          "stroke": "#979797",
          "fill": "#88e",
          "strokeWidth": 2
        },
        {
          "type": "group",
          "description": "Button",
          "translateX": "${ButtonPosition * (width - 90)}",
          "opacity": "${ShowButton ? 1 : 0}",
          "items": {
            "type": "path",
            "pathData": "M45,82 a36,36,0,0,1,0,-76 a36,36,0,1,1,0,76 Z",
            "fill": "#88e",
            "stroke": "white",
            "strokeWidth": 6
          }
        }
      ]
    }
  },
  "mainTemplate": {
    "items": {
      "type": "Container",
      "items": [
        {
          "type": "VectorGraphic",
          "source": "ToggleButton",
          "id": "MySlider",
          "disabled": true,
          "scale": "fill",
          "width": "590",
          "bind": [
            {
              "name": "Position",
              "value": 0.50
            },
            {
              "name": "OldPosition",
              "value": 0.50
            },
            {
              "name": "ShowButton",
              "value": false
            }
          ],
          "ButtonPosition": "${Position}",
          "ShowButton": "${ShowButton}",
          "onDown": [
            {
              "type": "SetValue",
              "property": "ShowButton",
              "value": true
            },
            {
              "type": "SetValue",
              "property": "OldPosition",
              "value": "${Position}"
            },
            {
              "type": "SetValue",
              "property": "Position",
              "value": "${Math.clamp(0, (event.viewport.x - 45) / (event.viewport.width - 90), 1)}"
            }
          ],
          "onUp": [
            {
              "type": "SetValue",
              "property": "ShowButton",
              "value": false
            },
            {
              "type": "SetValue",
              "description": "Reset the position if we release the pointer at some far location",
              "when": "${!event.inBounds}",
              "property": "Position",
              "value": "${OldPosition}"
            }
          ],
          "onMove": {
            "type": "SetValue",
            "property": "Position",
            "value": "${Math.clamp(0, (event.viewport.x - 45) / (event.viewport.width - 90), 1)}"
          }
        }
      ]
    }
  }
}
)";

TEST_F(GraphicComponentTest, DisabledMoveToSlide) {
    loadDocument(SLIDER_DISABLED);
    auto slider = context->findComponentById("MySlider");

    // initial slider position
    double position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0.5, position);

    // move disabled slider and check position
    root->handlePointerEvent(PointerEvent(kPointerDown, Point(45, 0), 0, kTouchPointer));
    ASSERT_FALSE(root->isDirty());
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0.5, position);

    root->handlePointerEvent(PointerEvent(kPointerMove, Point(170, 0), 0, kTouchPointer));
    ASSERT_FALSE(root->isDirty());
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0.5, position);

    root->handlePointerEvent(PointerEvent(kPointerUp, Point(384, 380), 0, kTouchPointer));
    ASSERT_FALSE(root->isDirty());
    position = slider->getContext()->find("Position").object().value().asNumber();
    ASSERT_EQ(0.5, position);
}
