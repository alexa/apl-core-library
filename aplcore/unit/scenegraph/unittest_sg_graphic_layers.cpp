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
#include "apl/graphic/graphicbuilder.h"
#include "apl/graphic/graphicelementgroup.h"
#include "apl/graphic/graphicelementpath.h"
#include "apl/graphic/graphicelementtext.h"
#include "apl/scenegraph/builder.h"
#include "apl/scenegraph/node.h"
#include "test_sg.h"

using namespace apl;

class SGGraphicTestLayers : public DocumentWrapper {
public:
    SGGraphicTestLayers() : DocumentWrapper() {
       config->measure(std::make_shared<MyTestMeasurement>());
   }

   void loadGraphic(const char *str, const StyleInstancePtr& style = nullptr) {
       gc = GraphicContent::create(session, str);
       ASSERT_TRUE(gc);
       auto jr = JsonResource(&gc->get(), Path());
       auto context = Context::createTestContext(metrics, *config);
       Properties properties;
       graphic = Graphic::create(context, jr, std::move(properties), nullptr);
       ASSERT_TRUE(graphic);
   }

   void TearDown() override {
       graphic = nullptr;
       gc = nullptr;
       updates.clear();
       DocumentWrapper::TearDown();
   }

   GraphicContentPtr gc;
   GraphicPtr graphic;
   sg::SceneGraphUpdates updates;
};


// Use a custom test for checking the nodes.  The regular test_sg.h code skips over operations
// that are not visible. But we want to verify that we have exactly the correct operations.
static ::testing::AssertionResult
checkOps(sg::PathOp *op, bool hasFill, Color fillColor, bool hasStroke, Color strokeColor) {
   if (hasFill) {
       if (!sg::FillPathOp::is_type(op))
           return ::testing::AssertionFailure() << "Expected a fill operation";
       auto *fill = sg::FillPathOp::cast(op);
       auto *paint = sg::ColorPaint::cast(fill->paint);
       if (!paint || paint->getColor() != fillColor)
           return ::testing::AssertionFailure() << "Fill color mismatch";
       op = op->nextSibling.get();
   }

   if (hasStroke) {
       if (!sg::StrokePathOp::is_type(op))
           return ::testing::AssertionFailure() << "Missing stroke operation";
       auto *stroke = sg::StrokePathOp::cast(op);
       auto *paint = sg::ColorPaint::cast(stroke->paint);
       if (!paint || paint->getColor() != strokeColor)
           return ::testing::AssertionFailure() << "Stroke color mismatch";
       op = op->nextSibling.get();
   }

   if (op)
       return ::testing::AssertionFailure() << "Extra operation";

   return ::testing::AssertionSuccess();
}

static ::testing::AssertionResult
checkDraw(const sg::NodePtr& node,
         float x,
         bool hasFill,
         Color fillColor,
         bool hasStroke,
         Color strokeColor)
{
   if (!node)
       return ::testing::AssertionFailure() << "null draw node";

   if (!sg::DrawNode::is_type(node))
       return ::testing::AssertionFailure() << "not a draw node";
   auto *draw = sg::DrawNode::cast(node);

   auto *path = sg::GeneralPath::cast(draw->getPath());
   if (!path)
       return ::testing::AssertionFailure() << "missing path node";

   // negative x used to indicate no points
   if (path->getPoints() != (x < 0 ? std::vector<float>{} : std::vector<float>{0,0,x,0,x,x}))
       return ::testing::AssertionFailure() << "mismatched points";

   return checkOps(draw->getOp().get(), hasFill, fillColor, hasStroke, strokeColor);
}

static ::testing::AssertionResult
checkText(const sg::NodePtr& node,
         const std::string& textString,
         bool hasFill,
         Color fillColor,
         bool hasStroke,
         Color strokeColor)
{
   if (!node)
       return ::testing::AssertionFailure() << "null text node";

   if (!sg::TextNode::is_type(node))
       return ::testing::AssertionFailure() << "not a text node";

   auto* text = sg::TextNode::cast(node);
   if (text->getTextLayout()->toDebugString() != textString)
       return ::testing::AssertionFailure() << "text mismatch";

   return checkOps(text->getOp().get(), hasFill, fillColor, hasStroke, strokeColor);
}

static const char *DRAW_OPTIMIZATION = R"apl(
{
 "type": "AVG",
 "version": "1.2",
 "height": 100,
 "width": 100,
 "parameters": [
   {
     "name": "X",
     "default": false
   }
 ],
 "items": [
   {
     "type": "path",
     "description": "Empty path",
     "fill": "red",
     "pathData": "M10,10 M20,20"
   },
   {
     "type": "path",
     "description": "Just fill",
     "fill": "blue",
     "pathData": "h1 v1 z"
   },
   {
     "type": "path",
     "description": "Just stroke",
     "stroke": "red",
     "pathData": "h2 v2 z"
   },
   {
     "type": "path",
     "description": "Stroke, but no width",
     "stroke": "green",
     "strokeWidth": 0,
     "pathData": "h3 v3 z"
   },
   {
     "type": "path",
     "description": "Stroke and fill",
     "stroke": "yellow",
     "fill": "black",
     "strokeWidth": 5,
     "pathData": "h4 v4 z"
   },
   {
     "type": "path",
     "description": "Stroke and fill opacity zero",
     "stroke": "pink",
     "strokeOpacity": 0,
     "fill": "blue",
     "fillOpacity": 0,
     "strokeWidth": 5,
     "pathData": "h5 v5 z"
   },
   {
     "type": "path",
     "description": "Path depends on X",
     "pathData": "${X ? 'h6 v6 z' : 'M0,0'}",
     "fill": "purple"
   },
   {
     "type": "path",
     "description": "Fill depends on X",
     "pathData": "h7 v7 z",
     "fill": "${X ? 'blue' : 'transparent'}"
   },
   {
     "type": "path",
     "description": "Stroke depends on X",
     "pathData": "h8 v8 z",
     "stroke": "${X ? 'red' : 'transparent'}"
   }
 ]
}
)apl";

TEST_F(SGGraphicTestLayers, DrawOptimization)
{
   loadGraphic(DRAW_OPTIMIZATION);
   auto node = graphic->getSceneGraph(false, updates)->node();

   // Skip the empty path - there is no path data

   // Fill blue
   ASSERT_TRUE(checkDraw(node, 1, true, Color::BLUE, false, Color::TRANSPARENT));
   node = node->next();

   // Stroke red
   ASSERT_TRUE(checkDraw(node, 2, false, Color::TRANSPARENT, true, Color::RED));
   node = node->next();

   // Skip the stroke with green because there is no stroke width

   // Stroke yellow, fill black
   ASSERT_TRUE(checkDraw(node, 4, true, Color::BLACK, true, Color::YELLOW));
   node = node->next();

   // Skip stroke pink, fill blue because the opacities hide all colors

   // Allow the fill purple even though there is no path because the path is mutable
   ASSERT_TRUE(checkDraw(node, -1, true, Color::PURPLE, false, Color::TRANSPARENT));
   node = node->next();

   // Allow the 7th case - the fill color can be changed
   ASSERT_TRUE(checkDraw(node, 7, true, Color::TRANSPARENT, false, Color::TRANSPARENT));
   node = node->next();

   // Allow the 8th case - the stroke color can be changed
   ASSERT_TRUE(checkDraw(node, 8, false, Color::TRANSPARENT, true, Color::TRANSPARENT));
   node = node->next();

   ASSERT_FALSE(node);
}

TEST_F(SGGraphicTestLayers, DrawOptimizationLayers)
{
   loadGraphic(DRAW_OPTIMIZATION);
   auto layer = graphic->getSceneGraph(true, updates)->layer();

   // Until we hit an element that is parameterized, they should all be part of the content of the layer
   auto node = layer->content();
   ASSERT_TRUE(node);

   // Skip the empty path - there is no path data

   // Fill blue
   ASSERT_TRUE(checkDraw(node, 1, true, Color::BLUE, false, Color::TRANSPARENT));
   node = node->next();

   // Stroke red
   ASSERT_TRUE(checkDraw(node, 2, false, Color::TRANSPARENT, true, Color::RED));
   node = node->next();

   // Skip the stroke with green because there is no stroke width

   // Stroke yellow, fill black
   ASSERT_TRUE(checkDraw(node, 4, true, Color::BLACK, true, Color::YELLOW));
   node = node->next();

   // Skip stroke pink, fill blue because the opacities hide all colors
   // We should have run out of content
   ASSERT_FALSE(node);

   // The three parmeterized drawing nodes should be collapsed into a single node
   ASSERT_EQ(1, layer->children().size());

   // Allow the fill purple even though there is no path because the path is mutable
   node = layer->children().at(0)->content();
   ASSERT_TRUE(checkDraw(node, -1, true, Color::PURPLE, false, Color::TRANSPARENT));
   node = node->next();

   // Allow the 7th case - the fill color can be changed
   ASSERT_TRUE(checkDraw(node, 7, true, Color::TRANSPARENT, false, Color::TRANSPARENT));
   node = node->next();

   // Allow the 8th case - the stroke color can be changed
   ASSERT_TRUE(checkDraw(node, 8, false, Color::TRANSPARENT, true, Color::TRANSPARENT));
   ASSERT_FALSE(node->next());
}


static const char *TEXT_OPTIMIZATION = R"apl(
{
 "type": "AVG",
 "version": "1.2",
 "height": 100,
 "width": 100,
 "parameters": [
   {
     "name": "X",
     "default": false
   }
 ],
 "items": [
   {
     "type": "text",
     "text": "Just fill",
     "fill": "red"
   },
   {
     "type": "text",
     "text": "Just stroke",
     "stroke": "yellow",
     "fillOpacity": 0,
     "strokeWidth": 1
   },
   {
     "type": "text",
     "text": "Stroke and fill",
     "stroke": "green",
     "strokeWidth": 2,
     "fill": "blue"
   },
   {
     "type": "text",
     "text": "Nothing to draw",
     "fillOpacity": 0
   },
   {
     "type": "text",
     "text": "",
     "fill": "purple"
   },
   {
     "type": "text",
     "text": "Default"
   },
   {
     "type": "text",
     "text": "Parameterized ${X}"
   }
 ]
}
)apl";

TEST_F(SGGraphicTestLayers, TextOptimization)
{
    loadGraphic(TEXT_OPTIMIZATION);
    auto node = graphic->getSceneGraph(false, updates)->node();

    // Fill red (well, it's hidden under a transform)
    ASSERT_TRUE(checkText(node->child(), "Just fill", true, Color::RED, false, Color::TRANSPARENT));
    node = node->next();

    // Stroke yellow
    ASSERT_TRUE(
        checkText(node->child(), "Just stroke", false, Color::TRANSPARENT, true, Color::YELLOW));
    node = node->next();

    // Stroke green, fill blue
    ASSERT_TRUE(checkText(node->child(), "Stroke and fill", true, Color::BLUE, true, Color::GREEN));
    node = node->next();

    // Skip the "Nothing to draw" - there is no fill or stroke
    // Skip fill purple - no text to draw

    // Fill with black (the default color)
    ASSERT_TRUE(checkText(node->child(), "Default", true, Color::BLACK, false, Color::TRANSPARENT));
    node = node->next();

    // Parameterized and fill with black (the default color)
    ASSERT_TRUE(checkText(node->child(), "Parameterized false", true, Color::BLACK, false,
                          Color::TRANSPARENT));
    node = node->next();

    ASSERT_FALSE(node);
}

TEST_F(SGGraphicTestLayers, TextOptimizationLayers)
{
    loadGraphic(TEXT_OPTIMIZATION);
    auto layer = graphic->getSceneGraph(true, updates)->layer();
    DumpSceneGraph(layer);

    // There should be one parameterized sublayer
    ASSERT_EQ(1, layer->children().size());

    auto node = layer->content();

    // Fill red (well, it's hidden under a transform)
    ASSERT_TRUE(checkText(node->child(), "Just fill", true, Color::RED, false, Color::TRANSPARENT));
    node = node->next();

    // Stroke yellow
    ASSERT_TRUE(
        checkText(node->child(), "Just stroke", false, Color::TRANSPARENT, true, Color::YELLOW));
    node = node->next();

    // Stroke green, fill blue
    ASSERT_TRUE(checkText(node->child(), "Stroke and fill", true, Color::BLUE, true, Color::GREEN));
    node = node->next();

    // Skip the "Nothing to draw" - there is no fill or stroke
    // Skip fill purple - no text to draw

    // Fill with black (the default color)
    ASSERT_TRUE(checkText(node->child(), "Default", true, Color::BLACK, false, Color::TRANSPARENT));
    node = node->next();

    // No more children
    ASSERT_FALSE(node);

    // The parameterized black text is in the first child layer
    auto sublayer = layer->children().at(0);
    node = sublayer->content();
    ASSERT_TRUE(checkText(node->child(), "Parameterized false", true, Color::BLACK, false,
                          Color::TRANSPARENT));
    ASSERT_FALSE(node->next());
}


static const char * MERGE_DRAW_LAYERS = R"apl(
{
  "type": "AVG",
  "version": "1.2",
  "height": 100,
  "width": 100,
  "parameters": [{"name": "COLOR","default": "red"}],
  "items": [
    {
      "type": "path",
      "pathData": "h1 v1 z",
      "stroke": "black"
    },
    {
      "type": "path",
      "pathData": "h2 v2 z",
      "stroke": "black"
    },
    {
      "type": "path",
      "pathData": "h3 v3 z",
      "stroke": "${COLOR}"
    },
    {
      "type": "path",
      "pathData": "h4 v4 z",
      "stroke": "black"
    },
    {
      "type": "path",
      "pathData": "h5 v5 z",
      "stroke": "black"
    },
    {
      "type": "path",
      "pathData": "h6 v6 z",
      "stroke": "${COLOR}"
    },
    {
      "type": "path",
      "pathData": "h7 v7 z",
      "stroke": "${COLOR}"
    }
  ]
}
)apl";


// Adjacent layers that are either static or parameterized should merge
TEST_F(SGGraphicTestLayers, MergeDrawLayers)
{
    loadGraphic(MERGE_DRAW_LAYERS);
    auto layer = graphic->getSceneGraph(true, updates)->layer();

    // First two draw nodes should be in the content
    auto node = layer->content();
    ASSERT_TRUE(checkDraw(node, 1, false, Color::TRANSPARENT, true, Color::BLACK));
    node = node->next();
    ASSERT_TRUE(checkDraw(node, 2, false, Color::TRANSPARENT, true, Color::BLACK));
    ASSERT_FALSE(node->next());

    // There should be three child layers
    ASSERT_EQ(3, layer->children().size());

    // The first child layer is a single mutable draw node
    node = layer->children().at(0)->content();
    ASSERT_TRUE(checkDraw(node, 3, false, Color::TRANSPARENT, true, Color::RED));
    ASSERT_FALSE(node->next());

    // The second child layer has the two static draw nodes
    node = layer->children().at(1)->content();
    ASSERT_TRUE(checkDraw(node, 4, false, Color::TRANSPARENT, true, Color::BLACK));
    node = node->next();
    ASSERT_TRUE(checkDraw(node, 5, false, Color::TRANSPARENT, true, Color::BLACK));
    ASSERT_FALSE(node->next());

    // The final child layer has two mutable draw nodes
    node = layer->children().at(2)->content();
    ASSERT_TRUE(checkDraw(node, 6, false, Color::TRANSPARENT, true, Color::RED));
    node = node->next();
    ASSERT_TRUE(checkDraw(node, 7, false, Color::TRANSPARENT, true, Color::RED));
    ASSERT_FALSE(node->next());
}



static const char * DEBUG_CHECK_FIXED = R"apl(
{
  "type": "AVG", "version": "1.2", "height": 100, "width": 100,
  "items": {
    "type": "path",
    "pathData": "h2 v2 z",
    "stroke": "black"
  }
}
)apl";

// This test verifies toDebugString().  It's also good for checking the node and layer construction
// logic to verify that the right layer flags get set.
TEST_F(SGGraphicTestLayers, DebugCheckFixed)
{
    // Node requested on fixed content
    loadGraphic(DEBUG_CHECK_FIXED);
    auto fragment = graphic->getSceneGraph(false, updates);
    ASSERT_FALSE(fragment->isLayer());
    ASSERT_TRUE(fragment->isNode());
    ASSERT_EQ(0, fragment->toDebugString().rfind("NodeContentFixed<"));

    // Layer requested on fixed content
    loadGraphic(DEBUG_CHECK_FIXED);
    fragment = graphic->getSceneGraph(true, updates); // Ask for a layer
    ASSERT_TRUE(fragment->isLayer());
    ASSERT_FALSE(fragment->isNode());
    ASSERT_EQ(0, fragment->toDebugString().rfind("LayerFixedContentFixed<"));
    ASSERT_TRUE(fragment->layer()->content());          // Has a drawing node
    ASSERT_TRUE(fragment->layer()->children().empty()); // But no child layers
}

static const char * DEBUG_CHECK_EMPTY = R"apl(
{
  "type": "AVG", "version": "1.2", "height": 100, "width": 100,
  "items":  {
    "type": "group"
  }
}
)apl";

TEST_F(SGGraphicTestLayers, DebugCheckEmpty)
{
    // Node requested on empty content
    loadGraphic(DEBUG_CHECK_EMPTY);
    auto fragment = graphic->getSceneGraph(false, updates);
    ASSERT_FALSE(fragment->isLayer());
    ASSERT_FALSE(fragment->isNode());
    ASSERT_TRUE(fragment->empty());
    ASSERT_EQ(0, fragment->toDebugString().rfind("NodeEmpty<"));

    // Layer requested on empty content.
    loadGraphic(DEBUG_CHECK_EMPTY);
    fragment = graphic->getSceneGraph(true, updates); // Ask for a layer on an empty node
    ASSERT_FALSE(fragment->isLayer());
    ASSERT_FALSE(fragment->isNode());
    ASSERT_TRUE(fragment->empty());
    ASSERT_EQ(0, fragment->toDebugString().rfind("NodeEmpty<"));
}

static const char * DEBUG_CHECK_MUTABLE = R"apl(
{
  "type": "AVG", "version": "1.2", "height": 100, "width": 100,
  "parameters": [{"name": "COLOR", "default": "blue"}, {"name": "OPACITY", "default": 1.0}],
  "items": {
    "type": "path",
    "pathData": "h3 v3 z",
    "stroke": "${COLOR}"
  }
}
)apl";

// This test verifies toDebugString().  It's also good for checking the node and layer construction
// logic to verify that the right layer flags get set.
TEST_F(SGGraphicTestLayers, DebugCheckMutable) {
    // Node requested on mutable content
    loadGraphic(DEBUG_CHECK_MUTABLE);
    auto fragment = graphic->getSceneGraph(false, updates);
    ASSERT_FALSE(fragment->isLayer());
    ASSERT_TRUE(fragment->isNode());
    ASSERT_EQ(0, fragment->toDebugString().rfind("NodeContentMutable<"));
}


static const char * DEBUG_CHECK_SHELL = R"apl(
{
  "type": "AVG", "version": "1.2", "height": 100, "width": 100,
  "parameters": [
    {"name": "X", "default": "blue"},
    {"name": "Y", "default": 1.0},
    {"name": "T", "default": "scale(2)"},
    {"name": "LC", "default": "butt" },
    {"name": "LJ", "default": "round" }
  ],
  "items": { "type": "group" }
}
)apl";

// It's tricky to build a GraphicFragment with LayerFixedContentMutable or LayerMutable
// because the top-level element (the GraphicContainer) is a fixed layer.  So we keep a loaded
// graphic with some parameters, but inflate elements separately from that

/**
 * These test cases are for AVG path objects that don't draw anything.  They should return a
 * nullptr fragment.
 */
static const std::vector<std::string> EMPTY_PATH = {
    R"("pathData": "h3", "stroke": null )",
    R"("pathData": "", "stroke": "${X}")",
    R"("pathData": "m20,20 30,30", "stroke": "${X}")",
    R"("pathData": "h3 v3", "stroke": "transparent")",
    R"("pathData": "h3 v3", "fill": "transparent")",
    R"("pathData": "h3 v3", "fill": "blue", "fillOpacity": 0)",
    R"("pathData": "h3 v3", "fillOpacity": "${Y}")",
    R"("pathData": "h3 v3", "fillTransform": "${T}")",
    R"("pathData": "h3 v3", "pathLength": "${Y}")",
    R"("pathData": "h3 v3", "strokeDashArray": "${X}")",
    R"("pathData": "h3 v3", "strokeDashOffset": "${X}")",
    R"("pathData": "h3 v3", "strokeLineCap": "${LC}")",
    R"("pathData": "h3 v3", "strokeLineJoin": "${LJ}")",
    R"("pathData": "h3 v3", "strokeMiterLimit": "${X}")",
    R"("pathData": "h3 v3", "strokeOpacity": "${X}")",
    R"("pathData": "h3 v3", "strokeTransform": "${T}")",
    R"("pathData": "h3 v3", "strokeWidth": "${X}")",
    R"("pathData": "h3 v3", "stroke": "green", "strokeWidth": 0)",
    R"("pathData": "h3 v3", "stroke": "transparent", "strokeWidth": "${X}")",
    R"("pathData": "h3 v3", "stroke": "green", "strokeOpacity": 0)",
    R"("pathData": "h3 v3", "stroke": "transparent", "strokeOpacity": "${X}")",
    R"("pathData": "h3 v3", "stroke": {"type": "linear", "colorRange": ["transparent", "transparent"]})",
};

TEST_F(SGGraphicTestLayers, EmptyPath)
{
    loadGraphic(DEBUG_CHECK_SHELL);
    for (const auto& m : EMPTY_PATH) {
        auto data = JsonData(R"({"type": "path", )" + m + "}");
        auto path = GraphicElementPath::create(graphic, graphic->getContext(), data.get());
        ASSERT_TRUE(path) << m;
        auto fragment = path->buildSceneGraph(true, updates);
        ASSERT_FALSE(fragment) << m;
    }
}

/**
 * These test case are for AVG path objects that have fixed properties.  They should return
 * a fragment containing an sg::NodePtr with the label "NodeContentFixed".
 */
static const std::vector<std::string> NODE_PATH = {
    R"("pathData": "h3", "stroke": "blue" )",
    R"("pathData": "h3", "fill": "blue" )",
    R"("pathData": "h3 v3", "stroke": {"type": "linear", "colorRange": ["blue", "green"]})",
    R"("pathData": "h3 v3", "fill": {"type": "linear", "colorRange": ["blue", "green"]})",
};

TEST_F(SGGraphicTestLayers, NodePath)
{
    loadGraphic(DEBUG_CHECK_SHELL);
    for (const auto& m : NODE_PATH) {
        auto data = JsonData(R"({"type": "path", )" + m + "}");
        auto path = GraphicElementPath::create(graphic, graphic->getContext(), data.get());
        ASSERT_TRUE(path) << m;
        auto fragment = path->buildSceneGraph(true, updates);
        ASSERT_TRUE(fragment) << m;
        ASSERT_FALSE(fragment->isLayer()) << m;
        ASSERT_TRUE(fragment->isNode()) << m;
        ASSERT_EQ(0, fragment->toDebugString().rfind("NodeContentFixed")) << m;
    }
}

/**
 * These test cases are for AVG path objects that do not have fixed properties.  They should
 * return a fragment containing an sg::LayerPtr with the label "LayerFixedContentMutable".
 */
static const std::vector<std::string> LAYER_PATH = {
    R"("pathData": "h3 v3", "fill": "${X}")",
    R"("pathData": "h3 v3", "fill": "blue", "fillOpacity": "${Y}")",
    R"("pathData": "h3 v3", "fill": "blue", "fillTransform": "${T}")",
    R"("pathData": "h3 v3", "stroke": "white", "pathLength": "${Y}")",
    R"("pathData": "h3 v3", "stroke": "${X}")",
    R"("pathData": "h3 v3", "stroke": "white","strokeDashArray": "${X}")",
    R"("pathData": "h3 v3", "stroke": "white","strokeDashOffset": "${X}")",
    R"("pathData": "h3 v3", "stroke": "white","strokeLineCap": "${LC}")",
    R"("pathData": "h3 v3", "stroke": "white","strokeLineJoin": "${LJ}")",
    R"("pathData": "h3 v3", "stroke": "white","strokeMiterLimit": "${X}")",
    R"("pathData": "h3 v3", "stroke": "white","strokeOpacity": "${X}")",
    R"("pathData": "h3 v3", "stroke": "white","strokeTransform": "${T}")",
    R"("pathData": "h3 v3", "stroke": "white","strokeWidth": "${X}")",
    // Layer with a style is ALWAYS mutable
    R"("pathData": "", "style": "foo" )",
};

TEST_F(SGGraphicTestLayers, LayerPath)
{
    loadGraphic(DEBUG_CHECK_SHELL);
    for (const auto& m : LAYER_PATH) {
        auto data = JsonData(R"({"type": "path", )" + m + "}");
        auto path = GraphicElementPath::create(graphic, graphic->getContext(), data.get());
        ASSERT_TRUE(path) << m;
        auto fragment = path->buildSceneGraph(true, updates);
        ASSERT_TRUE(fragment) << m;
        ASSERT_TRUE(fragment->isLayer()) << m;
        ASSERT_FALSE(fragment->isNode()) << m;
        ASSERT_EQ(0, fragment->toDebugString().rfind("LayerFixedContentMutable")) << m;
    }
}


static const char * DEBUG_CHECK_TEXT = R"apl(
{
  "type": "AVG", "version": "1.2", "height": 100, "width": 100,
  "parameters": [
    {"name": "C", "default": "blue"},
    {"name": "X", "default": 1.0},
    {"name": "T", "default": "scale(2)"},
    {"name": "FF", "default": "serif" },
    {"name": "FS", "default": 40 },
    {"name": "FT", "default": "italic" },
    {"name": "FW", "default": 700 },
    {"name": "SW", "default": 2.0 },
    {"name": "TA", "default": "end" }
  ],
  "items": { "type": "group" }
}
)apl";

/**
 * These test cases are for AVG text objects that don't draw anything.  They should return a
 * nullptr fragment.
 */
static const std::vector<std::string> EMPTY_TEXT = {
    R"("text": "" )",
    R"("text": "Hi", "fill": "transparent" )",
    R"("text": "Hi", "fill": "transparent", "fillOpacity": "${X}" )",
    R"("text": "Hi", "fillOpacity": 0 )",
    R"("text": "Hi", "file": "${C}", "fillOpacity": 0 )",
    R"("text": "Hi", "fill": {"type": "linear", "colorRange": ["#0000", "#0000"]} )",
    R"("text": "Hi", "fill": "transparent", "stroke": "blue" )",
    R"("text": "Hi", "fill": "transparent", "stroke": "blue", "strokeWidth": 1, "strokeOpacity": 0 )",
    R"("text": "Hi", "fill": "transparent", "strokeWidth": 1, "strokeOpacity": 1 )",
};

TEST_F(SGGraphicTestLayers, EmptyText)
{
    loadGraphic(DEBUG_CHECK_TEXT);

    for (const auto& m : EMPTY_TEXT) {
        auto data = JsonData(R"({"type": "text", )" + m + "}");
        auto text = GraphicElementText::create(graphic, graphic->getContext(), data.get());
        ASSERT_TRUE(text) << m;
        auto fragment = text->buildSceneGraph(true, updates);
        ASSERT_FALSE(fragment) << m;
    }
}

/**
 * These test case are for AVG text objects that have fixed properties.  They should return
 * a fragment containing an sg::NodePtr with the label "NodeContentFixed".
 */
static const std::vector<std::string> NODE_TEXT = {
    R"("text": "Hello" )",
    R"("text": "Hello", "fill": "green" )",
    R"("text": "Hello", "fillOpacity": 0.5 )",
    R"("text": "Hello", "fontFamily": "serif" )",
    R"("text": "Hello", "fontSize": 10 )",
    R"("text": "Hello", "fontStyle": "italic" )",
    R"("text": "Hello", "fontWeight": 100 )",
    R"("text": "Hello", "letterSpacing": 2.0 )",
    R"("text": "Hello", "fill": "transparent", "stroke": "blue", "strokeWidth": 1 )",
    R"("text": "Hello", "textAnchor": "end" )",
    R"("text": "Hello", "x": 23 )",
    R"("text": "Hello", "y": 100 )",
};

TEST_F(SGGraphicTestLayers, NodeText)
{
    loadGraphic(DEBUG_CHECK_TEXT);

    for (const auto& m : NODE_TEXT) {
        auto data = JsonData(R"({"type": "text", )" + m + "}");
        auto path = GraphicElementText::create(graphic, graphic->getContext(), data.get());
        ASSERT_TRUE(path) << m;
        auto fragment = path->buildSceneGraph(true, updates);
        ASSERT_TRUE(fragment) << m;
        ASSERT_FALSE(fragment->isLayer()) << m;
        ASSERT_TRUE(fragment->isNode()) << m;
        ASSERT_EQ(0, fragment->toDebugString().rfind("NodeContentFixed")) << m;
    }
}

/**
 * These test cases are for AVG text objects that do not have fixed properties.  They should
 * return a fragment containing an sg::LayerPtr with the label "LayerFixedContentMutable".
 */
static const std::vector<std::string> LAYER_TEXT = {
    R"("text": "${C}" )",
    R"("text": "Hello", "fill": "${C}" )",
    R"("text": "Hello", "fillOpacity": "${X}" )",
    R"("text": "Hello", "fontFamily": "${FF}" )",
    R"("text": "Hello", "fontSize": "${FS}" )",
    R"("text": "Hello", "fontStyle": "${FT}" )",
    R"("text": "Hello", "fontWeight": "${FW}" )",
    R"("text": "Hello", "letterSpacing": "${X}" )",
    R"("text": "Hello", "fill": "transparent", "stroke": "${C}", "strokeWidth": 1 )",
    R"("text": "Hello", "fill": "transparent", "stroke": "blue", "strokeOpacity": "${X}", "strokeWidth": 1 )",
    R"("text": "Hello", "fill": "transparent", "stroke": "blue", "strokeWidth": "${SW}" )",
    R"("text": "Hello", "textAnchor": "${TA}" )",
    R"("text": "Hello", "x": "${X}" )",
    R"("text": "Hello", "y": "${X}" )",
    R"("text": "", "style": "foo" )",
};

TEST_F(SGGraphicTestLayers, LayerText)
{
    loadGraphic(DEBUG_CHECK_TEXT);

    for (const auto& m : LAYER_TEXT) {
        auto data = JsonData(R"({"type": "text", )" + m + "}");
        auto path = GraphicElementText::create(graphic, graphic->getContext(), data.get());
        ASSERT_TRUE(path) << m;
        auto fragment = path->buildSceneGraph(true, updates);
        ASSERT_TRUE(fragment) << m;
        ASSERT_TRUE(fragment->isLayer()) << m;
        ASSERT_FALSE(fragment->isNode()) << m;
        ASSERT_EQ(0, fragment->toDebugString().rfind("LayerFixedContentMutable")) << m;
    }
}

/**
 * These test cases are for AVG group objects that return a nullptr fragment because
 * they don't have children or the children are not visible
 * children should return a nullptr fragment.
 */
TEST_F(SGGraphicTestLayers, EmptyGroups) {
    // No children
    loadGraphic(R"({
                     "type": "AVG",
                     "version": "1.2",
                     "height": 100,
                     "width": 100,
                     "items": {
                       "type": "group"
                     }
                    })");
    ASSERT_TRUE(graphic);
    auto group = graphic->getRoot()->getChildAt(0);
    ASSERT_TRUE(group);
    auto fragment = group->buildSceneGraph(true, updates);
    ASSERT_FALSE(fragment);

    // Opacity fixed at zero can never be seen
    loadGraphic(R"({
                     "type": "AVG",
                     "version": "1.2",
                     "height": 100,
                     "width": 100,
                     "items": {
                       "type": "group",
                       "opacity": 0,
                       "items": {
                         "type": "text",
                         "text": "HI"
                       }
                     }
                   })");
    ASSERT_TRUE(graphic);
    group = graphic->getRoot()->getChildAt(0);
    ASSERT_TRUE(group);
    ASSERT_EQ(1, group->getChildCount());
    fragment = group->buildSceneGraph(true, updates);
    ASSERT_FALSE(fragment);
}

TEST_F(SGGraphicTestLayers, FixedNodeGroups)
{
    // One child with fixed drawing content
    loadGraphic(R"apl({
                     "type": "AVG",
                     "version": "1.2",
                     "height": 100,
                     "width": 100,
                     "items": {
                       "type": "group",
                       "opacity": 0.5,
                       "transform": "rotate(45)",
                       "items": {
                         "type": "text",
                         "text": "dog"
                       }
                     }
                    })apl");
    ASSERT_TRUE(graphic);
    auto group = graphic->getRoot()->getChildAt(0);
    ASSERT_TRUE(group);
    auto fragment = group->buildSceneGraph(true, updates);
    ASSERT_TRUE(fragment);
    ASSERT_TRUE(fragment->isNode());
    ASSERT_EQ(0, fragment->toDebugString().rfind("NodeContentFixed"));

    // Multiple children child with fixed drawing content
    loadGraphic(R"apl({
                     "type": "AVG",
                     "version": "1.2",
                     "height": 100,
                     "width": 100,
                     "items": {
                       "type": "group",
                       "opacity": 0.5,
                       "transform": "rotate(45)",
                       "items": {
                         "type": "text",
                         "text": "dog"
                       },
                       "data": "${Array.range(4)}"
                     }
                    })apl");
    ASSERT_TRUE(graphic);
    group = graphic->getRoot()->getChildAt(0);
    ASSERT_TRUE(group);
    fragment = group->buildSceneGraph(true, updates);
    ASSERT_TRUE(fragment);
    ASSERT_TRUE(fragment->isNode());
    ASSERT_EQ(0, fragment->toDebugString().rfind("NodeContentFixed"));
}


TEST_F(SGGraphicTestLayers, MutableNodeGroups) {
    // One child with mutable drawing content
    loadGraphic(R"apl({
                     "type": "AVG",
                     "version": "1.2",
                     "height": 100,
                     "width": 100,
                     "parameters": "TEXT",
                     "items": {
                       "type": "group",
                       "opacity": 0.5,
                       "transform": "rotate(45)",
                       "items": {
                         "type": "text",
                         "text": "${TEXT}"
                       }
                     }
                    })apl");
    ASSERT_TRUE(graphic);
    auto group = graphic->getRoot()->getChildAt(0);
    ASSERT_TRUE(group);
    auto fragment = group->buildSceneGraph(false, updates);  // Note: force a node, or this will be a layer
    ASSERT_TRUE(fragment);
    ASSERT_TRUE(fragment->isNode());
    ASSERT_EQ(0, fragment->toDebugString().rfind("NodeContentMutable"));

    // Same as above, but this time put the mutation in the group
    loadGraphic(R"apl({
                     "type": "AVG",
                     "version": "1.2",
                     "height": 100,
                     "width": 100,
                     "parameters": {"name": "OPACITY", "default": 1 },
                     "items": {
                       "type": "group",
                       "opacity": "${OPACITY}",
                       "transform": "rotate(45)",
                       "items": {
                         "type": "text",
                         "text": "hi"
                       }
                     }
                    })apl");
    ASSERT_TRUE(graphic);
    group = graphic->getRoot()->getChildAt(0);
    ASSERT_TRUE(group);
    fragment = group->buildSceneGraph(false, updates);  // Note: force a node, or this will be a layer
    ASSERT_TRUE(fragment);
    ASSERT_TRUE(fragment->isNode());
    ASSERT_EQ(0, fragment->toDebugString().rfind("NodeContentMutable"));

    // Assign a style to the group
    loadGraphic(R"apl({
                     "type": "AVG",
                     "version": "1.2",
                     "height": 100,
                     "width": 100,
                     "items": {
                       "type": "group",
                       "style": "happy_feet",
                       "transform": "rotate(45)",
                       "items": {
                         "type": "text",
                         "text": "hi"
                       }
                     }
                    })apl");
    ASSERT_TRUE(graphic);
    group = graphic->getRoot()->getChildAt(0);
    ASSERT_TRUE(group);
    fragment = group->buildSceneGraph(false, updates);  // Note: force a node, or this will be a layer
    ASSERT_TRUE(fragment);
    ASSERT_TRUE(fragment->isNode());
    ASSERT_EQ(0, fragment->toDebugString().rfind("NodeContentMutable"));
}

TEST_F(SGGraphicTestLayers, LayerContent) {
    // Enforce a layer with a mutable child item
    loadGraphic(R"apl({
                     "type": "AVG",
                     "version": "1.2",
                     "height": 100,
                     "width": 100,
                     "parameters": "TEXT",
                     "items": {
                       "type": "group",
                       "opacity": 0.5,
                       "transform": "rotate(45)",
                       "items": {
                         "type": "text",
                         "text": "${TEXT}"
                       }
                     }
                    })apl");
    ASSERT_TRUE(graphic);
    auto group = graphic->getRoot()->getChildAt(0);
    ASSERT_TRUE(group);
    auto fragment = group->buildSceneGraph(true, updates);
    ASSERT_TRUE(fragment);
    ASSERT_TRUE(fragment->isLayer());
    ASSERT_EQ(0, fragment->toDebugString().rfind("LayerFixedContentFixed"));

    // Same as above, but this time put the mutation in the group
    loadGraphic(R"apl({
                     "type": "AVG",
                     "version": "1.2",
                     "height": 100,
                     "width": 100,
                     "parameters": {"name": "OPACITY", "default": 1 },
                     "items": {
                       "type": "group",
                       "opacity": "${OPACITY}",
                       "transform": "rotate(45)",
                       "items": {
                         "type": "text",
                         "text": "hi"
                       }
                     }
                    })apl");
    ASSERT_TRUE(graphic);
    group = graphic->getRoot()->getChildAt(0);
    ASSERT_TRUE(group);
    fragment = group->buildSceneGraph(true, updates);
    ASSERT_TRUE(fragment);
    ASSERT_TRUE(fragment->isLayer());
    ASSERT_EQ(0, fragment->toDebugString().rfind("LayerMutable")) << fragment->toDebugString();

    // Assign a style
    loadGraphic(R"apl({
                     "type": "AVG",
                     "version": "1.2",
                     "height": 100,
                     "width": 100,
                     "items": {
                       "type": "group",
                       "style": "happy_feet",
                       "transform": "rotate(45)",
                       "items": {
                         "type": "text",
                         "text": "hi"
                       }
                     }
                    })apl");
    ASSERT_TRUE(graphic);
    group = graphic->getRoot()->getChildAt(0);
    ASSERT_TRUE(group);
    fragment = group->buildSceneGraph(true, updates);
    ASSERT_TRUE(fragment);
    ASSERT_TRUE(fragment->isLayer());
    ASSERT_EQ(0, fragment->toDebugString().rfind("LayerMutable")) << fragment->toDebugString();
}

TEST_F(SGGraphicTestLayers, MergeGroups) {
    // Enforce a layer with a mutable child item
    loadGraphic(R"apl({
                     "type": "AVG",
                     "version": "1.2",
                     "height": 100,
                     "width": 100,
                     "parameters": "TEXT",
                     "items": {
                       "type": "group",
                       "opacity": 0.5,
                       "transform": "rotate(45)",
                       "items": [
                         {
                           "type": "group"
                         },
                         {
                           "type": "group",
                           "item": {
                             "type": "text",
                             "text": "Um..."
                           }
                         },
                         {
                           "type": "text",
                           "text": "${TEXT}"
                         }
                       ]
                     }
                    })apl");
    ASSERT_TRUE(graphic);
    auto group = graphic->getRoot()->getChildAt(0);
    ASSERT_TRUE(group);
    auto fragment = group->buildSceneGraph(true, updates);
    ASSERT_TRUE(fragment);
    ASSERT_TRUE(fragment->isLayer());
    ASSERT_EQ(0, fragment->toDebugString().rfind("LayerFixedContentFixed"));
}

// TODO: Check style changes - they should update properties as appropriate