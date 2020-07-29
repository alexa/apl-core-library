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

#include "gtest/gtest.h"

#include "apl/apl.h"
#include <vector>
#include <apl/scaling/scalingcalculator.h>

using namespace apl;

/**
 *    height   Vw/w = Vh/h
 *      |         /
 *      |        /
 *      |  2    /
 *      |      /
 *    Vh|-----|g
 *      |  3 /|        1
 *      |   / |
 *      |  /  |
 *      | /4  |
 *      |/____|____________________ width
 *            Vw
 * Vw:             Viewport width
 * Vh:             Viewport height
 * Vw/w = Vh/h:    Scaling factor line
 * g:              Global minimum
 *
 * The cost function can be split into 6 general sections. The higher the k value, the more
 * distinct these section will be and the closer the minima will follow the lines of Vw and Vh.
 * These unit tests test a set of width/height ranges that fall into and cross over each section.
 * Each section has its own unique properties and should be tested in isolation
 * and in combination. Values are sanity checked in MATLAB using the
 * following function to visualize ranges
 * ```
 * function plot = plotCost(minw, maxw, minh, maxh)
 *  W = 800;
 *  H = 600;
 *  k = 10;
 *  [w, h] = meshgrid(minw:10:maxw,minh:10:maxh);
 *  s = min(W./w, H./h);
 *  z = 2 - s.*(w./W + h./H) + k.*(log(s).^2);
 *  plot = surf(w, h, z); xlabel('width'); ylabel('height'); zlabel('cost')
 * end
 * ```
 */

class SizeRange {
public:
    SizeRange() : wmin(0), wmax(0), hmin(0), hmax(0) {}
    SizeRange(double width, double height) : wmin(width), wmax(width), hmin(height), hmax(height) {}
    SizeRange(double wmin, double wmax, double hmin, double hmax)
            : wmin(wmin), wmax(wmax), hmin(hmin), hmax(hmax) {}

    SizeRange& width(double w) {
        wmin = w;
        wmax = w;
        return *this;
    }

    SizeRange& height(double h) {
        hmin = h;
        hmax = h;
        return *this;
    }

    double wmin;
    double wmax;
    double hmin;
    double hmax;
};

class ScalingTest : public ::testing::Test {
public:
    ScalingTest() : ::testing::Test(), k(10.0f), Vw(800), Vh(600), r(std::min(Vw, Vh) * 0.5f) {}
    double k;
    int Vw;
    int Vh;
    double r;

    void testRanges(const std::vector<SizeRange>& ranges, int expectedWidth,
                      int expectedHeight, double expectedScale) {
        testRanges(ranges, expectedWidth, expectedHeight, expectedScale, kViewportModeHub, false,
                          RECTANGLE);
    }

    void testRanges(const std::vector<SizeRange>& ranges, int expectedWidth,
                      int expectedHeight, double expectedScale, bool isRound) {
        testRanges(ranges, expectedWidth, expectedHeight, expectedScale, kViewportModeHub, isRound,
                          RECTANGLE);
    }

    void testRanges(const std::vector<SizeRange>& ranges, int expectedWidth,
                      int expectedHeight, double expectedScale, bool isRound, ScreenShape shape) {
        testRanges(ranges, expectedWidth, expectedHeight, expectedScale, kViewportModeHub, isRound, shape);
    }

    void testRanges(const std::vector<SizeRange>& ranges, int expectedWidth,
                      int expectedHeight, double expectedScale, ViewportMode mode, bool isRound, ScreenShape shape) {
        // density shouldm't effect which viewport is chosen, so test a number of denisties here.
        std::vector<int> dpis = {80, 160, 320, 400, 500, 600, 1000};
        for(auto dpi : dpis) {
            int pixelWidth = Vw * dpi / 160;
            int pixelHeight = Vh * dpi / 160;
            auto metrics = Metrics().size(pixelWidth, pixelHeight).shape(shape).dpi(dpi);
            std::vector<ViewportSpecification> specifications(ranges.size());
            for (auto& range : ranges) {
                specifications.emplace_back(range.wmin, range.wmax, range.hmin, range.hmax, mode, isRound);
            }
            ScalingOptions options;
            options.specifications(specifications).biasConstant(k);
            auto transform = MetricsTransform(metrics, options);
            auto m = transform.getMetrics();
            ASSERT_NEAR(expectedScale, transform.getScaleToViewhost(), 0.1);
            ASSERT_NEAR(expectedWidth, m.getWidth(), 2);
            ASSERT_NEAR(expectedHeight, m.getHeight(), 2);
            ASSERT_NEAR(m.getWidth() * transform.getScaleToViewhost() * dpi / MetricsTransform::CORE_DPI, transform.getViewhostWidth(), 2);
            ASSERT_NEAR(m.getHeight() * transform.getScaleToViewhost() * dpi / MetricsTransform::CORE_DPI, transform.getViewhostHeight(), 2);
        }
    }
};

TEST_F(ScalingTest, ContainsGlobalMinimum) {
    testRanges({{300, 600, 500, 700}, {700, 900, 500, 700}}, Vw, Vh, 1.0);
}

TEST_F(ScalingTest, ExatlyGlobalMinimum) {
    testRanges({{800, 600}}, Vw, Vh, 1.0);
}

TEST_F(ScalingTest, HitsGlobalMinimum) {
    testRanges({{800, 1000, 600, 1000}}, Vw, Vh, 1.0);
}

TEST_F(ScalingTest, HitsLine) {
    testRanges({{200, 400, 200, 300}}, 400, 300, 2.0);
}

TEST_F(ScalingTest, Sections1) {
    testRanges({{900, 1000, 200, 500}, {10000, 5000, 56600, 87700}}, 900, 500, 0.888);
}

TEST_F(ScalingTest, Sections1prime) {
    testRanges({{1000, 1200, 605, 720}, {10000, 5000, 56600, 87700}}, 1000, 720, 0.8);
}

TEST_F(ScalingTest, Sections2) {
    testRanges({{850, 1000, 800, 900}, {10000, 5000, 56600, 87700}}, 1000, 800, 0.75);
}

TEST_F(ScalingTest, Sections2prime) {
    testRanges({{500, 700, 650, 900}, {10000, 5000, 56600, 87700}}, 700, 650, 1.0);
}

TEST_F(ScalingTest, Sections3) {
    testRanges({{200, 400, 400, 550}, {10000, 5000, 56600, 87700}}, 400, 550, 1.0);
}

TEST_F(ScalingTest, Sections4) {
    testRanges({{600, 750, 250, 350}, {10000, 5000, 56600, 87700}}, 750, 350, 1.0);
}

TEST_F(ScalingTest, Sections12IntersectLeftTop) {
    testRanges({{1000, 1200, 650, 800}, {10000, 5000, 56600, 87700}}, 1000, 750, 0.80);
}

TEST_F(ScalingTest, Sections12IntersectLeftRight) {
    testRanges({{1000, 1200, 650, 1000}, {10000, 5000, 56600, 87700}}, 1000, 750, 0.80);
}

TEST_F(ScalingTest, Sections12IntersectBottomTop) {
    testRanges({{1000, 1500, 850, 1000}, {10000, 5000, 56600, 87700}}, 1133, 850, 0.7);
}

TEST_F(ScalingTest, Sections12IntersectBottomLeft) {
    testRanges({{1000, 1250, 850, 1000}, {10000, 5000, 56600, 87700}}, 1133, 850, 0.7);
}

TEST_F(ScalingTest, Sections23) {
    testRanges({{200, 656, 550, 650}, {10000, 5000, 56600, 87700}}, 656, 575, 1.0);
}

TEST_F(ScalingTest, Sections34) {
    testRanges({{300, 550, 300, 500}, {10000, 5000, 56600, 87700}}, 550, 500, 1.2);
}

TEST_F(ScalingTest, Sections41) {
    testRanges({{600, 1000, 200, 350}, {10000, 5000, 56600, 87700}}, 776, 350, 1.0);
}

TEST_F(ScalingTest, Sections234) {
    testRanges({{300, 750, 400, 900}, {10000, 5000, 56600, 87700}}, 750, 571, 1.0);
}

TEST_F(ScalingTest, Sections341) {
    testRanges({{300, 650, 400, 900}, {10000, 5000, 56600, 87700}}, 650, 575, 1.0);
}

TEST_F(ScalingTest, LargeHeightRange) {
    testRanges({{300, 650, 400, std::numeric_limits<int>::max()}, {10000, 5000, 56600, 87700}}, 650,
               575, 1.0);
}

TEST_F(ScalingTest, RoundScreenExactFit) {
    double size =  2 * std::sqrt(r * r / 2.0);
    // correct
    testRanges({{size - 100, size + 100, size - 100, size + 100}}, size, size, 1.0, false, ROUND);
}

TEST_F(ScalingTest, RoundScreenSquareFit) {
    double size = 4 * std::sqrt(r * r / 2.0);
    testRanges({{size, size + 100, size, size + 100}}, size, size, 0.5, false, ROUND);
}

TEST_F(ScalingTest, RoundScreenRectExactFitWidth) {
    // start at double
    double size = 2 * std::sqrt(r * r / 2.0);
    testRanges({{size + 50, size + 100, 100, std::numeric_limits<int>::max()}},
                              size + 50.0, 365, 1.0, false, ROUND);
}


TEST_F(ScalingTest, RoundAndRectSpecShapeMatters) {
    auto metrics = Metrics().size(Vw, Vh).shape(RECTANGLE);
    double size = 600;
    std::vector<ViewportSpecification> specifications = {
            {8000, 6000, kViewportModeHub, false},
            {size, size, kViewportModeHub, true}, //matches the viewport exactly
    };
    ScalingOptions options;
    options.specifications(specifications).biasConstant(k);
    auto result = scaling::calculate(metrics, options);
    ASSERT_EQ(8000, std::get<1>(result).getWidth());
    ASSERT_EQ(6000, std::get<1>(result).getHeight());
    ASSERT_NEAR(std::get<0>(result), 0.1, 0.001);
    ASSERT_EQ(std::get<2>(result), specifications[0]);
}

TEST_F(ScalingTest, RoundAndRectSpecShapeMattersReverse) {
    auto metrics = Metrics().size(Vw, Vh).shape(RECTANGLE);
    double size = 600;
    std::vector<ViewportSpecification> specifications = {
            {size, size, kViewportModeHub, true}, //matches the viewport exactly
            {8000, 6000, kViewportModeHub, false},
    };
    ScalingOptions options;
    options.specifications(specifications).biasConstant(k);
    auto result = scaling::calculate(metrics, options);
    ASSERT_EQ(8000, std::get<1>(result).getWidth());
    ASSERT_EQ(6000, std::get<1>(result).getHeight());
    ASSERT_NEAR(std::get<0>(result), 0.1, 0.001);
    ASSERT_EQ(std::get<2>(result), specifications[1]);
}

TEST_F(ScalingTest, RoundAndRectSpecShapeNotMatters) {
    auto metrics = Metrics().size(Vw, Vh).shape(RECTANGLE);
    double size = 600;
    std::vector<ViewportSpecification> specifications = {
            {8000, 6000, kViewportModeHub, false},
            {size, size, kViewportModeHub, true}, //matches the viewport exactly
    };
    ScalingOptions options;
    options.specifications(specifications).biasConstant(k).shapeOverridesCost(false);
    auto result = scaling::calculate(metrics, options);
    ASSERT_NEAR(size, std::get<1>(result).getWidth(), 2);
    ASSERT_NEAR(size, std::get<1>(result).getHeight(), 2);
    ASSERT_NEAR(std::get<0>(result), 1, 0.001);
    ASSERT_EQ(std::get<2>(result), specifications[1]);
}

TEST_F(ScalingTest, RoundAndRectSpecShapeNotMattersReverse) {
    auto metrics = Metrics().size(Vw, Vh).shape(RECTANGLE);
    double size = 600;
    std::vector<ViewportSpecification> specifications = {
            {size, size, kViewportModeHub, true}, //matches the viewport exactly
            {8000, 6000, kViewportModeHub, false},
    };
    ScalingOptions options;
    options.specifications(specifications).biasConstant(k).shapeOverridesCost(false);
    auto result = scaling::calculate( metrics, options);
    ASSERT_NEAR(size, std::get<1>(result).getWidth(), 2);
    ASSERT_NEAR(size, std::get<1>(result).getHeight(), 2);
    ASSERT_NEAR(std::get<0>(result), 1, 0.001);
    ASSERT_EQ(std::get<2>(result), specifications[0]);
}

TEST_F(ScalingTest, RoundAndRectSpecShapeMattersRoundVP) {
    std::vector<int> dpis = {80, 160, 320, 400, 500, 600, 1000};
    for(auto dpi : dpis) {
        int pixelWidth = Vw * dpi / 160;
        int pixelHeight = Vh * dpi / 160;
        auto metrics = Metrics().size(pixelWidth, pixelHeight).shape(ROUND).dpi(dpi);
        double size = 2 * std::sqrt(r * r / 2.0);
        std::vector<ViewportSpecification> specifications = {
                {size * 10, size * 10, kViewportModeHub, true},
                {800, 600, kViewportModeHub, false},//matches the viewport exactly
        };
        ScalingOptions options;
        options.specifications(specifications).biasConstant(k);
        auto result = scaling::calculate(metrics, options);
        ASSERT_NE(800, std::get<1>(result).getWidth());
        ASSERT_NE(600, std::get<1>(result).getHeight());
        ASSERT_EQ(std::get<2>(result), specifications[0]);
    }
}

TEST_F(ScalingTest, RoundAndRectSpecShapeNOTMattersRoundVP) {
    std::vector<int> dpis = {80, 160, 320, 400, 500, 600, 1000};
    for(auto dpi : dpis) {
        int pixelWidth = Vw * dpi / 160;
        int pixelHeight = Vh * dpi / 160;
        auto metrics = Metrics().size(pixelWidth, pixelHeight).shape(ROUND).dpi(dpi);
        double size = 2 * std::sqrt(r * r / 2.0);
        std::vector<ViewportSpecification> specifications = {
            {size * 10, size * 10, kViewportModeHub, true},
            {800, 600, kViewportModeHub, false},//matches the viewport exactly
        };
        ScalingOptions options;
        options.specifications(specifications).biasConstant(k).shapeOverridesCost(false);
        auto result = scaling::calculate( metrics, options);
        ASSERT_EQ(800, std::get<1>(result).getWidth());
        ASSERT_EQ(600, std::get<1>(result).getHeight());
        ASSERT_EQ(std::get<2>(result), specifications[1]);
    }
}

TEST_F(ScalingTest, ReturnsCorrectChosenSpec) {
    std::vector<int> dpis = {80, 160, 320, 400, 500, 600, 1000};
    for(auto dpi : dpis) {
        int pixelWidth = Vw * dpi / 160;
        int pixelHeight = Vh * dpi / 160;
        auto metrics = Metrics().size(pixelWidth, pixelHeight).shape(RECTANGLE).dpi(dpi);
        std::vector<ViewportSpecification> specifications = {
            {1600, 1600, kViewportModeHub, true},
            {800, 800, kViewportModeHub, true},
            {1600, 800, kViewportModeHub, false},
        };
        ScalingOptions options;
        options.specifications(specifications).biasConstant(k).shapeOverridesCost(false);
        auto result = scaling::calculate(metrics, options);
        ASSERT_EQ(std::get<2>(result), specifications[1]);
    }
}

TEST_F(ScalingTest, ChoosesCorrectViewportWithDifferentDenisties) {
    //The dpi value should not change the viewport selection
    std::vector<int> dpis = {80, 160, 320, 400, 500, 600, 1000};
    for(auto dpi : dpis) {
        int pixelWidth = 960 * dpi / 160;
        int pixelHeight = 540 * dpi / 160;
        auto metrics = Metrics().size(pixelWidth, pixelHeight).shape(RECTANGLE).dpi(dpi);
        std::vector<ViewportSpecification> specifications = {
                {960, 540, kViewportModeHub, false},
                {1280, 800, kViewportModeHub, false},
                {1024, 600, kViewportModeHub, false},
        };
        ScalingOptions options;
        options.specifications(specifications).biasConstant(k).shapeOverridesCost(false);
        auto result = scaling::calculate(metrics, options);
        ASSERT_EQ(std::get<2>(result), specifications[0]);
    }
}

TEST_F(ScalingTest, ChoosesCorrectViewportWithSameMode) {
    // Only the specs that match viewport mode should be valid
    double pixelWidth = 960;
    double pixelHeight = 480;
    auto metrics = Metrics().size(pixelWidth, pixelHeight).shape(RECTANGLE).mode(kViewportModeHub);
    std::vector<ViewportSpecification> specifications = {
            {pixelWidth, pixelHeight, kViewportModeTV, false}, //matches the viewport size exactly
            {1280, 800, kViewportModeHub, false},
            {1024, 600, kViewportModeHub, false},
    };
    ScalingOptions options;
    options.specifications(specifications).biasConstant(k).shapeOverridesCost(false);
    auto result = scaling::calculate(metrics, options);
    ASSERT_EQ(std::get<2>(result), specifications[2]);
}

TEST_F(ScalingTest, NoValidSpecWithSameMode) {
    double pixelWidth = 960;
    double pixelHeight = 540;
    auto metrics = Metrics().size(pixelWidth, pixelHeight).shape(RECTANGLE).mode(kViewportModeTV);
    std::vector<ViewportSpecification> specifications = {
            {pixelWidth, pixelHeight, kViewportModeHub, false}, //matches the viewport size exactly
            {1280, 800, kViewportModeHub, false},
            {1024, 600, kViewportModeHub, false},
    };
    ScalingOptions options;
    options.specifications(specifications).biasConstant(k).shapeOverridesCost(false);
    auto result = scaling::calculate(metrics, options);
    ASSERT_EQ(std::get<2>(result).isValid(), false);
}

TEST_F(ScalingTest, IgnoreModeChosesSameMode) {
    // Only the specs that match viewport mode should be valid
    double pixelWidth = 720;
    double pixelHeight = 1280;
    auto metrics = Metrics().size(pixelWidth, pixelHeight).shape(RECTANGLE).mode(kViewportModeMobile);
    std::vector<ViewportSpecification> specifications = {
            {pixelWidth, pixelHeight, kViewportModeTV, false},
            {1280, 800, kViewportModeHub, false},
            {pixelWidth, pixelHeight, kViewportModeMobile, false},
    };
    ScalingOptions options;
    options.specifications(specifications).biasConstant(k).ignoresMode(true);
    auto result = scaling::calculate(metrics, options);
    ASSERT_EQ(std::get<2>(result), specifications[2]);
}

TEST_F(ScalingTest, IgnoreModeChosesBestOne) {
    double pixelWidth = 720;
    double pixelHeight = 1280;
    auto metrics = Metrics().size(pixelWidth, pixelHeight).shape(RECTANGLE).mode(kViewportModeMobile);
    std::vector<ViewportSpecification> specifications = {
            {720, 1200, kViewportModeHub, false},
            {1280, 800, kViewportModeHub, false},
            {pixelWidth, pixelHeight, kViewportModeHub, false},
    };
    ScalingOptions options;
    options.specifications(specifications).biasConstant(k).ignoresMode(true);
    auto result = scaling::calculate(metrics, options);
    ASSERT_EQ(std::get<2>(result), specifications[2]);
}

TEST_F(ScalingTest, AllowedModesChosesSameMode) {
    // Only the specs that match viewport mode should be valid
    double pixelWidth = 720;
    double pixelHeight = 1280;
    auto metrics = Metrics().size(pixelWidth, pixelHeight).shape(RECTANGLE).mode(kViewportModeMobile);
    std::vector<ViewportSpecification> specifications = {
            {pixelWidth, pixelHeight, kViewportModeTV, false},
            {1280, 800, kViewportModeHub, false},
            {pixelWidth, pixelHeight, kViewportModeMobile, false},
    };
    ScalingOptions options;
    options.specifications(specifications).biasConstant(k).allowedModes({kViewportModeHub});
    auto result = scaling::calculate(metrics, options);
    ASSERT_EQ(std::get<1>(result).getViewportMode(), kViewportModeMobile);
    ASSERT_EQ(std::get<2>(result), specifications[2]);
}

TEST_F(ScalingTest, AllowedModesChosesBestOne) {
    double pixelWidth = 720;
    double pixelHeight = 1280;
    auto metrics = Metrics().size(pixelWidth, pixelHeight).shape(RECTANGLE).mode(kViewportModeMobile);
    std::vector<ViewportSpecification> specifications = {
            {pixelWidth, pixelHeight, kViewportModeTV, false},
            {720, 1200, kViewportModeHub, false},
            {1280, 800, kViewportModeHub, false},
    };
    ScalingOptions options;
    options.specifications(specifications).biasConstant(k).allowedModes({kViewportModeHub});
    auto result = scaling::calculate(metrics, options);
    ASSERT_EQ(std::get<1>(result).getViewportMode(), kViewportModeHub); // viewport mode is overridden
    ASSERT_EQ(std::get<2>(result), specifications[1]);
}

TEST_F(ScalingTest, AllowedModesMultipleChosesBestOne) {
    double pixelWidth = 720;
    double pixelHeight = 1280;
    auto metrics = Metrics().size(pixelWidth, pixelHeight).shape(RECTANGLE).mode(kViewportModeMobile);
    std::vector<ViewportSpecification> specifications = {
            {pixelWidth, pixelHeight, kViewportModeTV, false},
            {720, 1200, kViewportModePC, false},
            {720, 1200, kViewportModeHub, false},
    };
    ScalingOptions options;
    options.specifications(specifications).biasConstant(k).allowedModes({kViewportModeHub, kViewportModePC});
    auto result = scaling::calculate(metrics, options);
    ASSERT_EQ(std::get<1>(result).getViewportMode(), kViewportModePC); // viewport mode is overridden
    ASSERT_EQ(std::get<2>(result), specifications[1]);
}