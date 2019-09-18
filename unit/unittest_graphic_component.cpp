/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "testeventloop.h"

#include "apl/graphic/graphic.h"
#include "apl/primitives/object.h"

using namespace apl;

class GraphicComponentTest : public DocumentWrapper {};

static const char * SIMPLE_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"graphics\": {"
    "    \"box\": {"
    "      \"type\": \"AVG\","
    "      \"version\": \"1.0\","
    "      \"height\": 100,"
    "      \"width\": 100,"
    "      \"items\": {"
    "        \"type\": \"path\","
    "        \"pathData\": \"M0,0 h100 v100 h-100 z\","
    "        \"fill\": \"red\""
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"VectorGraphic\","
    "      \"source\": \"box\""
    "    }"
    "  }"
    "}";

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

static const char * NO_SCALE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"graphics\": {"
    "    \"box\": {"
    "      \"type\": \"AVG\","
    "      \"version\": \"1.0\","
    "      \"height\": 100,"
    "      \"width\": 100,"
    "      \"items\": {"
    "        \"type\": \"path\","
    "        \"pathData\": \"M0,0 h100 v100 h-100 z\","
    "        \"fill\": \"red\""
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"VectorGraphic\","
    "      \"source\": \"box\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\""
    "    }"
    "  }"
    "}";

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

static const char * BEST_FIT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"graphics\": {"
    "    \"box\": {"
    "      \"type\": \"AVG\","
    "      \"version\": \"1.0\","
    "      \"height\": 100,"
    "      \"width\": 100,"
    "      \"items\": {"
    "        \"type\": \"path\","
    "        \"pathData\": \"M0,0 h100 v100 h-100 z\","
    "        \"fill\": \"red\""
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"VectorGraphic\","
    "      \"source\": \"box\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"scale\": \"best-fit\""
    "    }"
    "  }"
    "}";

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

static const char * BASE_FIT_TEST_CASE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"graphics\": {"
    "    \"box\": {"
    "      \"type\": \"AVG\","
    "      \"version\": \"1.0\","
    "      \"height\": 100,"
    "      \"width\": 100,"
    "      \"items\": {"
    "        \"type\": \"path\","
    "        \"pathData\": \"M0,0 h100 v100 h-100 z\","
    "        \"fill\": \"red\""
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"VectorGraphic\","
    "      \"source\": \"box\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\""
    "    }"
    "  }"
    "}";

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

static const char * BASE_STRETCH_TEST_CASE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"graphics\": {"
    "    \"box\": {"
    "      \"type\": \"AVG\","
    "      \"version\": \"1.0\","
    "      \"height\": 100,"
    "      \"width\": 100,"
    "      \"items\": {"
    "        \"type\": \"path\","
    "        \"pathData\": \"M0,0 h100 v100 h-100 z\","
    "        \"fill\": \"red\""
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"VectorGraphic\","
    "      \"source\": \"box\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"scale\": \"fill\""
    "    }"
    "  }"
    "}";

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


static const char * GRAPHIC_STYLE =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"styles\": {"
    "    \"myGraphic\": {"
    "      \"values\": ["
    "        {"
    "          \"color\": \"blue\""
    "        },"
    "        {"
    "          \"when\": \"${state.pressed}\","
    "          \"color\": \"red\""
    "        }"
    "      ]"
    "    }"
    "  },"
    "  \"graphics\": {"
    "    \"box\": {"
    "      \"type\": \"AVG\","
    "      \"version\": \"1.0\","
    "      \"height\": 100,"
    "      \"width\": 100,"
    "      \"parameters\": ["
    "        \"color\""
    "      ],"
    "      \"items\": {"
    "        \"type\": \"path\","
    "        \"pathData\": \"M0,0 h100 v100 h-100 z\","
    "        \"fill\": \"${color}\""
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"VectorGraphic\","
    "      \"source\": \"box\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"style\": \"myGraphic\""
    "    }"
    "  }"
    "}";

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


static const char *GRAPHIC_STYLE_WITH_ALIGNMENT =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"styles\": {"
    "    \"myGraphic\": {"
    "      \"values\": ["
    "        {"
    "          \"align\": \"left\""
    "        },"
    "        {"
    "          \"when\": \"${state.pressed}\","
    "          \"align\": \"right\""
    "        }"
    "      ]"
    "    }"
    "  },"
    "  \"graphics\": {"
    "    \"box\": {"
    "      \"type\": \"AVG\","
    "      \"version\": \"1.0\","
    "      \"height\": 100,"
    "      \"width\": 100,"
    "      \"parameters\": ["
    "        \"color\""
    "      ],"
    "      \"items\": {"
    "        \"type\": \"path\","
    "        \"pathData\": \"M0,0 h100 v100 h-100 z\","
    "        \"fill\": \"${color}\""
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"VectorGraphic\","
    "      \"source\": \"box\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"style\": \"myGraphic\""
    "    }"
    "  }"
    "}";


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

static const char *GRAPHIC_STYLE_WITH_STRETCH =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"styles\": {"
    "    \"myGraphic\": {"
    "      \"values\": ["
    "        {"
    "          \"scale\": \"fill\""
    "        },"
    "        {"
    "          \"when\": \"${state.pressed}\","
    "          \"scale\": \"none\","
    "          \"align\": \"right\""
    "        }"
    "      ]"
    "    }"
    "  },"
    "  \"graphics\": {"
    "    \"box\": {"
    "      \"type\": \"AVG\","
    "      \"version\": \"1.0\","
    "      \"height\": 50,"
    "      \"width\": 256,"
    "      \"viewportHeight\": 100,"
    "      \"viewportWidth\": 100,"
    "      \"scaleTypeHeight\": \"stretch\","
    "      \"scaleTypeWidth\": \"stretch\","
    "      \"items\": {"
    "        \"type\": \"path\","
    "        \"pathData\": \"M${width},${height} L0,0\""
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"VectorGraphic\","
    "      \"source\": \"box\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"style\": \"myGraphic\""
    "    }"
    "  }"
    "}";


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

    // The vector graphic component should have a new scale, alignment, and media bounds
    ASSERT_EQ(Rect(768, 375, 256, 50), component->getCalculated(kPropertyMediaBounds).getRect());  // Right-aligned
    ASSERT_TRUE(CheckDirty(component, kPropertyScale, kPropertyAlign, kPropertyMediaBounds, kPropertyGraphic));
    ASSERT_TRUE(CheckDirty(root, component));

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
}

static const char *RELAYOUT_TEST =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"styles\": {"
    "    \"frameStyle\": {"
    "      \"values\": ["
    "        {"
    "          \"borderWidth\": 0"
    "        },"
    "        {"
    "          \"when\": \"${state.pressed}\","
    "          \"borderWidth\": 100"
    "        }"
    "      ]"
    "    }"
    "  },"
    "  \"graphics\": {"
    "    \"box\": {"
    "      \"type\": \"AVG\","
    "      \"version\": \"1.0\","
    "      \"height\": 100,"
    "      \"width\": 100,"
    "      \"items\": {"
    "        \"type\": \"path\","
    "        \"pathData\": \"M${width},${height} L0,0\""
    "      }"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"Frame\","
    "      \"style\": \"frameStyle\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"item\": {"
    "        \"type\": \"VectorGraphic\","
    "        \"source\": \"box\","
    "        \"width\": \"100%\","
    "        \"height\": \"100%\","
    "        \"scale\": \"fill\""
    "      }"
    "    }"
    "  }"
    "}";


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

    // The border width has changed on the frame.
    ASSERT_EQ(Object(Dimension(100)), component->getCalculated(kPropertyBorderWidth));
    ASSERT_EQ(Rect(100, 100, 824, 600), component->getCalculated(kPropertyInnerBounds).getRect());
    ASSERT_TRUE(CheckDirty(component, kPropertyInnerBounds, kPropertyBorderWidth));

    // The vector graphic component has new, smaller media bounds
    ASSERT_EQ(Rect(0, 0, 824, 600), vg->getCalculated(kPropertyMediaBounds).getRect());
    ASSERT_EQ(Rect(100, 100, 824, 600), vg->getCalculated(kPropertyBounds).getRect());  // Bounds in parent
    // The kPropertyGraphic is marked as dirty.  That's not right - it's merely resized
    ASSERT_EQ(Rect(0, 0, 824, 600), vg->getCalculated(kPropertyInnerBounds).getRect());
    ASSERT_TRUE(CheckDirty(vg, kPropertyGraphic, kPropertyMediaBounds, kPropertyBounds, kPropertyInnerBounds));

    // The root should be showing dirty for both the vector graphic component and the frame
    ASSERT_TRUE(CheckDirty(root, component, vg));

    // The container should have four updated values
    ASSERT_EQ(Object(Dimension(600)), container->getValue(kGraphicPropertyHeightActual));
    ASSERT_EQ(Object(Dimension(824)), container->getValue(kGraphicPropertyWidthActual));
    ASSERT_EQ(Object(100), container->getValue(kGraphicPropertyViewportHeightActual));
    ASSERT_EQ(Object(100), container->getValue(kGraphicPropertyViewportWidthActual));
    ASSERT_TRUE(CheckDirty(container, kGraphicPropertyHeightActual, kGraphicPropertyWidthActual));

    // The graphic itself should have a new viewport height and width
    ASSERT_EQ(100, graphic->getViewportWidth());
    ASSERT_EQ(100, graphic->getViewportHeight());
    ASSERT_TRUE(CheckDirty(graphic, container));
}

// Assign a vector graphic to a component

const static char * EMPTY_GRAPHIC =
    "{"
    "  \"type\": \"APL\","
    "  \"version\": \"1.0\","
    "  \"styles\": {"
    "    \"graphicStyle\": {"
    "      \"values\": ["
    "        {"
    "          \"myColor\": \"blue\""
    "        }"
    "      ]"
    "    }"
    "  },"
    "  \"mainTemplate\": {"
    "    \"items\": {"
    "      \"type\": \"VectorGraphic\","
    "      \"style\": \"graphicStyle\","
    "      \"width\": \"100%\","
    "      \"height\": \"100%\","
    "      \"scale\": \"fill\","
    "      \"myLineWidth\": 10"
    "    }"
    "  }"
    "}";

const static char *STANDALONE_GRAPHIC =
    "{"
    "  \"type\": \"AVG\","
    "  \"version\": \"1.0\","
    "  \"height\": 100,"
    "  \"width\": 100,"
    "  \"parameters\": ["
    "    \"myColor\","
    "    \"myLineWidth\""
    "  ],"
    "  \"items\": {"
    "    \"type\": \"path\","
    "    \"pathData\": \"M0,0 h100 v100 h-100 z\","
    "    \"fill\": \"${myColor}\","
    "    \"strokeWidth\": \"${myLineWidth}\""
    "  }"
    "}";


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

const char* PARAMETERS_DOC = "{\n"
                             "    \"type\": \"APL\",\n"
                             "    \"version\": \"1.0\",\n"
                             "    \"graphics\": {\n"
                             "        \"myPillShape\": {\n"
                             "            \"type\": \"AVG\",\n"
                             "            \"version\": \"1.0\",\n"
                             "            \"height\": 100,\n"
                             "            \"width\": 100,\n"
                             "            \"parameters\": [\n"
                             "                \"myScaleType\"\n"
                             "            ],\n"
                             "            \"scaleTypeHeight\": \"${myScaleType}\",\n"
                             "            \"items\": [\n"
                             "                {\n"
                             "                    \"type\": \"path\",\n"
                             "                    \"pathData\": \"M25,50 a25,25 0 1 1 50,0 l0 ${height-100} a25,25 0 1 1 -50,0 z\",\n"
                             "                    \"stroke\": \"black\",\n"
                             "                    \"strokeWidth\": 20\n"
                             "                }\n"
                             "            ]\n"
                             "        }\n"
                             "    },\n"
                             "    \"mainTemplate\": {\n"
                             "        \"item\": {\n"
                             "            \"type\": \"Container\",\n"
                             "            \"direction\": \"row\",\n"
                             "            \"items\": {\n"
                             "                \"type\": \"VectorGraphic\",\n"
                             "                \"source\": \"myPillShape\",\n"
                             "                \"width\": 100,\n"
                             "                \"height\": 200,\n"
                             "                \"scale\": \"fill\",\n"
                             "                \"myScaleType\": \"${data}\"\n"
                             "            },\n"
                             "            \"data\": [\n"
                             "                \"none\",\n"
                             "                \"stretch\"\n"
                             "            ]\n"
                             "        }\n"
                             "    }\n"
                             "}";


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