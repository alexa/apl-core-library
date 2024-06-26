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
 */

#ifndef APL_METRICTRACKER_H
#define APL_METRICTRACKER_H

#include "MetricData.h"
#include <string>

namespace alexaext {
namespace metricsExtensionV2 {

using Dimensions = std::map<std::string, std::string>;
using Timestamp = std::chrono::steady_clock::time_point;

/**
 * Base class composing Metric structure
 */
class MetricTracker {
public:
    /**
     * Constructor
     */
    MetricTracker(std::string&& metricName, Dimensions&& dimensions, int initialValue) {
        mMetric.name = std::move(metricName);
        mMetric.dimensions = std::move(dimensions);
        mMetric.value = initialValue;
    }
    /**
     * Object to hold metric data
     */
    Metric mMetric;
};

/**
 * CounterMetricTracker for tracking counter metrics.
 */
class CounterMetricTracker : public MetricTracker {
public:
    /**
     * Constructor
     */
    CounterMetricTracker(std::string&& metricName, Dimensions&& dimensions, int initialValue = 0)
        : MetricTracker(std::move(metricName), std::move(dimensions), initialValue) {}

    /**
     * Increments the value with the given amount.
     * @param amount is the amount by which the value should be increments. If @param amount is
     * negative, value will be decremented.
     */
    void incrementCounter(int amount) { mMetric.value += amount; }

    Metric& getMetric() { return mMetric; }
};

/**
 * TimerMetricTracker for tracking start time for timer metrics.
 */
class TimerMetricTracker : public MetricTracker {
public:
    /**
     * Constructor
     */
    TimerMetricTracker(std::string&& metricName, Dimensions&& dimensions, Timestamp&& startTime)
        : MetricTracker(std::move(metricName), std::move(dimensions), 0),
          mStarted(true),
          mStartTime(startTime) {}

    /**
     * Stop the timer.
     *
     * @param metric is out param which contains the metric data once stopped
     * @param stop captured by the caller. This will be used to calculate the timer metric.
     * @return true if stop succeed, false otherwise.
     */
    bool stop(Metric& metric, const Timestamp& stopTime) {
        if (!mStarted) {
            return false;
        }

        mStarted = false;
        mMetric.value =
            std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - mStartTime).count();

        metric = mMetric;
        return true;
    }

private:
    bool mStarted;
    Timestamp mStartTime;
};
} // namespace metricsExtensionV2
} // namespace alexaext

#endif //  APL_METRICTRACKER_H