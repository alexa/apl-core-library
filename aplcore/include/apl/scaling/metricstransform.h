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

#ifndef _APL_METRICSTRANSFORM_H
#define _APL_METRICSTRANSFORM_H

#include "apl/content/metrics.h"

namespace apl {

class ViewportSpecification {
public:
    ViewportSpecification()
        : ViewportSpecification(0, 0, kViewportModeHub, false) {}
    ViewportSpecification(double width, double height, ViewportMode mode, bool isRound)
        : ViewportSpecification(width, width, height, height, mode, isRound) {}
    ViewportSpecification(double wmin, double wmax, double hmin, double hmax, ViewportMode mode,
                          bool isRound)
        : wmin(wmin), wmax(wmax), hmin(hmin), hmax(hmax), mode(mode), isRound(isRound) {}
    double wmin;
    double wmax;
    double hmin;
    double hmax;
    ViewportMode mode;
    bool isRound;
    bool isValid() {
        return wmin > 0 && wmax > 0 && hmin > 0 && hmax > 0;
    }

    bool operator==(const ViewportSpecification& rhs) const {
        return wmin == rhs.wmin &&
                wmax == rhs.wmax &&
                hmin == rhs.hmin &&
                hmax == rhs.hmax &&
                mode == rhs.mode &&
                isRound == rhs.isRound;
    }
    std::string toDebugString() const;
};

class ScalingOptions {
public:
    friend class ScalingCalculator;
    ScalingOptions() : specifications({}), biasConstant(1.0f), shapeOverridesCost(true) {}
    ScalingOptions(const std::vector<ViewportSpecification>& specifications, double biasConstant,
                   bool shapeOverridesCost = true)
        : specifications(specifications),
          biasConstant(biasConstant),
          shapeOverridesCost(shapeOverridesCost) {}
    const std::vector<ViewportSpecification>& getSpecifications() const { return specifications; }
    double getBiasConstant() const { return biasConstant; }
    bool getShapeOverridesCost() const { return shapeOverridesCost; }
private:
    std::vector<ViewportSpecification> specifications;
    double biasConstant;
    bool shapeOverridesCost;
};

/**
 * Viewhost extends this class to provide transforms between core and viewhost layer. Since core
 * makes no assumptions about viewhost display units, it is up to the viewhost binding layer to
 * provide that logic.
 */
class MetricsTransform {
public:
    MetricsTransform(Metrics& metrics) : mMetrics(metrics) { init(); }
    MetricsTransform(Metrics& metrics, ScalingOptions& options)
        : mMetrics(metrics), mOptions(options) {
        init();
    }

    /**
     * Converts core units into viewhost units
     * @param value Core unit
     * @return Viewhost unit
     */
    virtual float toViewhost(float value) const = 0;

    /**
     * Converts viewhost units into core units
     * @param value Viewhost unit
     * @return Core unit
     */
    virtual float toCore(float value) const = 0;

    /**
     * Return the viewport width in viewhost units
     * @return Viewhost units
     */
    virtual float getViewhostWidth() const = 0;

    /**
     * Return the viewport height in viewhost units
     * @return Viewhost units
     */
    virtual float getViewhostHeight() const = 0;

    /**
     * @return The viewhost provided display pixels per inch
     */
    float getDpi() const { return mDpi; }

    /**
     * @return The raw scale factor: viewhost / core viewport ratio
     */
    float getScaleToViewhost() const { return mScaleToViewhost; }

    /**
     * @return The raw scale factor: core / viewhost viewport ratio
     */
    float getScaleToCore() const { return mScaleToCore; }

    /**
     * @return Core units width
     */
    float getWidth() const { return mWidth; }

    /**
     * @return Core units height
     */
    float getHeight() const { return mHeight; }

    /**
     * @return Viewport Mode
     */
    ViewportMode getViewportMode() const { return mMode; }

    /**
     * Get the metrics
     * @return
     */
    Metrics getMetrics() const { return mMetrics; }

    /**
     * @return The specification chosen by the scaling algorithm
     */
    ViewportSpecification getChosenSpec() const { return mChosenSpec; }

private:
    void init();

    Metrics& mMetrics;
    ScalingOptions mOptions;

    /** Viewhost provided dots per inch */
    float mDpi;

    /** viewhost / core viewport ratio */
    float mScaleToViewhost;

    /** core / viewhost viewport ratio */
    float mScaleToCore;

    /** Core units width */
    float mWidth;

    /** Core units height */
    float mHeight;

    /** Mode */
    ViewportMode mMode;

    /** The spec that was chosen */
    ViewportSpecification mChosenSpec;
};

} // namespace apl

#endif // APL_METRICSTRANSFORM_H
