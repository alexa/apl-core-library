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

#include "apl/scaling/scalingcalculator.h"
#include <cmath>

namespace apl {
namespace scaling {

namespace {

/** Constant PI */
constexpr double PI = 3.14159265358979323846;

/** How far to check each angle for square viewport in round screen */
constexpr double ANGLE_DELTA = PI * 1.0f / 180;

/** Maximum allowable viewport aspect ration for square viewport in round screen */
constexpr double MAX_VIEWPORT_RATIO = 3.0f;

void
ScalingCalculator::setMetricsSize(Metrics& metrics, double width, double height) {
    int pixelWidth = static_cast<int>(std::round(metrics.dpToPx(static_cast<float>(width))));
    int pixelHeight = static_cast<int>(std::round(metrics.dpToPx(static_cast<float>(height))));
    metrics.size(pixelWidth, pixelHeight);
}

void
ScalingCalculator::setMetricsMode(Metrics& metrics, ViewportMode mode) {
    metrics.mode(mode);
}

double
ScalingCalculator::cost(double w, double h) {
    double s = scaleFactor(w, h);
    double ln = std::log(s);
    return 2 - s * ((w / mViewportWidth) + (h / mViewportHeight)) + k * ln * ln;
}

void
ScalingCalculator::minimumFixedHeight(double height, double wmin, double wmax,
                                      ScalingCalculator::Size& size, double& minCost) {
    double middle = mViewportWidth * height / mViewportHeight;
    // if there's a line just calculate the endpoints
    if (wmin < middle) {
        double costLeft = cost(wmin, height);
        if (costLeft < minCost) {
            minCost = costLeft;
            size.w = wmin;
            size.h = height;
        }
        double costRight = std::min(minCost, cost(wmin, std::min(wmax, middle)));
        if (costRight < minCost) {
            minCost = costRight;
            size.w = std::min(wmax, middle);
            size.h = height;
        }
    }
    // if there's a curve iteratively solve this for now
    // TODO -> closed form solution
    if (wmax >= middle) {
        double w = std::max(middle, wmin);
        double localCost = std::numeric_limits<double>::max();
        while (w <= wmax) {
            double curveCost = cost(w, height);
            // we've iterated past the minimum
            if (curveCost > localCost) {
                break;
            }
            localCost = curveCost;
            if (curveCost < minCost) {
                minCost = curveCost;
                size.w = w;
                size.h = height;
            }
            w += std::floor(1.0);
        }
    }
}

void
ScalingCalculator::minimumFixedWidth(double width, double hmin, double hmax,
                                     ScalingCalculator::Size& size, double& minCost) {
    double middle = mViewportHeight * width / mViewportWidth;
    // if there's a line just calculate the endpoints
    if (hmin < middle) {
        double costBottom = cost(width, hmin);
        if (costBottom < minCost) {
            minCost = costBottom;
            size.w = width;
            size.h = hmin;
        }
        double costTop = cost(width, std::min(hmax, middle));
        if (costTop < minCost) {
            minCost = costTop;
            size.w = width;
            size.h = std::min(hmax, middle);
        }
    }
    // if there's a curve iteratively solve this for now
    // TODO -> closed form solution
    if (hmax >= middle) {
        double h = std::max(middle, hmin);
        double localCost = std::numeric_limits<double>::max();
        while (h <= hmax) {
            double curveCost = cost(width, h);
            // we've iterated past the minimum
            if (curveCost > localCost) {
                break;
            }
            localCost = curveCost;
            if (curveCost < minCost) {
                minCost = curveCost;
                size.w = width;
                size.h = h;
            }
            h = std::floor(h + 1.0);
        }
    }
}
} // namespace

std::tuple<double, Metrics, ViewportSpecification>
calculate(const Metrics& metrics, const ScalingOptions& options) {
    Metrics newMetrics = metrics;
    auto& specs = options.getSpecifications();
    std::vector<ViewportSpecification> validSpecs;
    bool shapeOverridesCost = options.getShapeOverridesCost();
    double biasConstant = options.getBiasConstant();
    auto allowedModes = options.getAllowedModes();

    // Only consider specs that match original viewport mode
    for (auto& spec : specs) {
        if (spec.mode == metrics.getViewportMode()){
            validSpecs.emplace_back(spec);
        }
    }

    if (options.getIgnoresMode()) {
        // All acceptable, add all to allowed
        for (auto& mode : sViewportModeBimap) {
            allowedModes.emplace(static_cast<ViewportMode>(mode.first));
        }
    }

    if (validSpecs.empty() && !allowedModes.empty()) {
        // All acceptable, but prioritize same mode so add what we don't have yet if empty
        for (auto& spec : specs) {
            if (spec.mode != metrics.getViewportMode() && allowedModes.count(spec.mode)){
                validSpecs.emplace_back(spec);
            }
        }
    }

    // If there are no specifications that match viewport mode, there is nothing to be done
    if (validSpecs.empty()) {
        return std::make_tuple(1.0, newMetrics, ViewportSpecification());
    }

    // If shape overrides cost, then modify the array
    if (shapeOverridesCost) {
        std::vector<ViewportSpecification> shapeCostValidSpecs;
        bool hasMetricsShape = false;
        for (auto& spec : validSpecs) {
            hasMetricsShape |= spec.isRound == (newMetrics.getScreenShape() == ROUND);
        }
        for (auto& spec : validSpecs) {
            bool isSameShape = spec.isRound == (newMetrics.getScreenShape() == ROUND);
            if (isSameShape || !hasMetricsShape) {
                shapeCostValidSpecs.emplace_back(spec);
            }
        }
        validSpecs = shapeCostValidSpecs;
    }


    double cost = std::numeric_limits<double>::max();
    ScalingCalculator::Size bestSize;
    double bestScale = 0.0;
    // get the width and height in dp to compare against specification in dp
    const auto metricsWidth = static_cast<double>(metrics.getWidth());
    const auto metricsHeight = static_cast<double>(metrics.getHeight());
    for (auto& specification : validSpecs) {

        // grab the bounds for this specification
        double hmin = specification.hmin;
        double hmax = specification.hmax;
        double wmin = specification.wmin;
        double wmax = specification.wmax;

        std::vector<ScalingCalculator::Size> sizes; // viewport sizes to check cost against
        if (!specification.isRound && newMetrics.getScreenShape() == ROUND) {
            // For a rectangular specification in a round viewport, the algorithm needs to check
            // against multiple viewport sizes around the perimeter of screen circle. For each
            // viewport aspect ratio permutation, calculate its cost against all other costs.

            double r = 0.5 * std::min(metricsWidth, metricsHeight);
            // to guard against large ranges make sure that the aspect ratio
            // is always between 1:3 and 3:1
            double startAngle = std::atan2(std::min(hmax, wmin * MAX_VIEWPORT_RATIO), wmin);
            double endAngle = std::atan2(hmin, std::min(wmax, hmin * MAX_VIEWPORT_RATIO));

            // always check the square case exactly
            double squareAngle = std::atan2(1.0, 1.0);
            sizes.push_back({2.0 * r * std::cos(squareAngle), 2.0 * r * std::sin(squareAngle),
                             specification.isRound, specification});

            double angle = startAngle;
            int iterations = static_cast<int>(std::abs(startAngle - endAngle) / ANGLE_DELTA);
            for (int i = 0; i < iterations; ++i) {
                sizes.push_back({2.0 * r * std::cos(angle), 2.0 * r * std::sin(angle),
                                 specification.isRound, specification});
                angle -= ANGLE_DELTA;
            }
        }
        else {
            // just check against actual viewport size
            sizes.push_back({metricsWidth, metricsHeight, specification.isRound, specification});
        }

        for (auto& size : sizes) {

            double Vw = size.w;
            double Vh = size.h;
            ScalingCalculator scaling(Vw, Vh, biasConstant);

            // first check if cost function global minimum is within the range. If it is, then
            // set the scaling to 1.
            if (Vh >= hmin && Vh <= hmax && Vw >= wmin && Vw <= wmax) {
                // set the metrics to whatever the viewport size and mode is. For rectangular screens
                // this will be just the rectangular size, for rectangular content in round
                // screens it will be whatever the optimal dimensions are within the screen
                // circle. Metrics takes pixels, so convert dp to px here.
                ScalingCalculator::setMetricsSize(newMetrics, Vw, Vh);
                ScalingCalculator::setMetricsMode(newMetrics, specification.mode);
                return std::make_tuple(1.0, newMetrics, size.spec);
            }

            // find the lowest cost along each border and set the bestSize given the
            // current cost
            double newCost = cost;
            scaling.minimumFixedHeight(hmin, wmin, wmax, bestSize, newCost);
            scaling.minimumFixedHeight(hmax, wmin, wmax, bestSize, newCost);
            scaling.minimumFixedWidth(wmin, hmin, hmax, bestSize, newCost);
            scaling.minimumFixedWidth(wmax, hmin, hmax, bestSize, newCost);

            // If a new optimal cost was found then update our best values.
            if (newCost < cost) {
                cost = newCost;
                bestSize.spec = size.spec;
                bestScale = scaling.scaleFactor(bestSize);
            }
        }
    }

    // Update the metrics with the optimal size and mode. This is the size that core
    // will use for all internal calculations.
    ScalingCalculator::setMetricsSize(newMetrics, bestSize.w, bestSize.h);
    ScalingCalculator::setMetricsMode(newMetrics, bestSize.spec.mode);

    // Return the scale factor, which the viewhost will use to scale dimensional values
    // coming from core.
    return std::make_tuple(bestScale, newMetrics, bestSize.spec);
}
} // namespace scaling

} // namespace apl