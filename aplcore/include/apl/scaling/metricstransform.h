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

#ifndef _APL_METRICSTRANSFORM_H
#define _APL_METRICSTRANSFORM_H

#include "apl/content/metrics.h"
#include <set>

namespace apl {

/**
 * Cloud defined viewport specification.
 */
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

/**
 * Set of options to control scaling algorithm.
 */
class ScalingOptions {
public:
    /**
     * @deprecated Use default constructor with chained setters.
     */
    ScalingOptions(const std::vector<ViewportSpecification>& specifications, double biasConstant,
                   bool shapeOverridesCost, bool ignoresMode)
            : mSpecifications(specifications),
              mBiasConstant(biasConstant),
              mShapeOverridesCost(shapeOverridesCost),
              mIgnoresMode(ignoresMode) {}

    /**
     * @deprecated Use default constructor with chained setters.
     */
    ScalingOptions(const std::vector<ViewportSpecification>& specifications, double biasConstant)
        : ScalingOptions(specifications, biasConstant, true, false) {}

    /**
     * @deprecated Use default constructor with chained setters.
     */
    ScalingOptions(const std::vector<ViewportSpecification>& specifications, double biasConstant,
                   bool shapeOverridesCost)
        : ScalingOptions(specifications, biasConstant, shapeOverridesCost, false) {}

    ScalingOptions() : ScalingOptions({}, 1.0f, true, false) {}

    /**
     * Set configured specifications.
     * @param specifications ViewportSpecifications.
     * @return This object for chaining
     */
    ScalingOptions& specifications(const std::vector<ViewportSpecification>& specifications) {
        mSpecifications = specifications;
        return *this;
    }

    /**
     * Set bias constant. Default is 1.0f
     * @param biasConstant bias constant value.
     * @return This object for chaining
     */
    ScalingOptions& biasConstant(double biasConstant) {
        mBiasConstant = biasConstant;
        return *this;
    }

    /**
     * Set shape to override cost. Same shape viewports will have a preference. Default is true.
     * @param shapeOverridesCost true to override, false otherwise.
     * @return This object for chaining
     */
    ScalingOptions& shapeOverridesCost(bool shapeOverridesCost) {
        mShapeOverridesCost = shapeOverridesCost;
        return *this;
    }

    /**
     * Ignore same mode requirement. If true all specifications will take a part in selection. Default is false.
     * @param ignoresMode true to ignore, false otherwise.
     * @return This object for chaining
     */
    ScalingOptions& ignoresMode(bool ignoresMode) {
        mIgnoresMode = ignoresMode;
        return *this;
    }

    /**
     * Set range of allowed modes. Only specified + device own mode specifications will be considered in selection.
     * Empty by default.
     * @param allowedModes set of allowed viewport modes.
     * @return This object for chaining
     */
    ScalingOptions& allowedModes(const std::set<ViewportMode>& allowedModes) {
        mAllowedModes = allowedModes;
        return *this;
    }

    const std::vector<ViewportSpecification>& getSpecifications() const { return mSpecifications; }
    const std::set<ViewportMode>& getAllowedModes() const { return mAllowedModes; }
    double getBiasConstant() const { return mBiasConstant; }
    bool getShapeOverridesCost() const { return mShapeOverridesCost; }
    bool getIgnoresMode() const { return mIgnoresMode; }

private:
    std::vector<ViewportSpecification> mSpecifications;
    std::set<ViewportMode> mAllowedModes;
    double mBiasConstant;
    bool mShapeOverridesCost;
    bool mIgnoresMode;
};

/**
 * Viewhost may extend this class to provide transforms between core and viewhost layer. Since core
 * makes no assumptions about viewhost display units, it is up to the viewhost to provide that logic.
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
     * Base implementation scales value to viewhost units considering dpi ratio.
     * @param value Core unit
     * @return Viewhost unit
     */
    virtual float toViewhost(float value) const;

    /**
     * Converts viewhost units into core units
     * Base implementation scales value to core units considering dpi ratio.
     * @param value Viewhost unit
     * @return Core unit
     */
    virtual float toCore(float value) const;

    /**
     * Base implementation scales width to viewhost units considering dpi ratio.
     * Return the viewport width in viewhost units
     * @return Viewhost units
     */
    virtual float getViewhostWidth() const;

    /**
     * Base implementation scales height to viewhost units considering dpi ratio.
     * Return the viewport height in viewhost units
     * @return Viewhost units
     */
    virtual float getViewhostHeight() const;

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

    /**
     * Core's virtual pixel density per inch. 
     * Equates a viewhost dpi of 160.0f as a 1:1 dpi scaling ratio.
     */
    static constexpr float CORE_DPI = 160.0f;

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
