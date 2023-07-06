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
#ifndef APL_APLMETRICSEXTENSIONOBSERVERINTERFACE_H
#define APL_APLMETRICSEXTENSIONOBSERVERINTERFACE_H

#include <chrono>
#include <memory>
#include <string>

namespace alexaext {
namespace metrics {

using Timestamp = std::chrono::steady_clock::time_point;

/**
 * The observer interface for metrics extension. To enable metrics extension, metric publisher need
 * to implement this interface and create @c AplMetricsExtension with it.
 *
 * Metric is created with applicationId, experienceId and metricId. These metrics are to be recorded
 * with an identifier in the form @c <applicationId>.<experienceId>.<metricId>
 */
class AplMetricsExtensionObserverInterface {
public:
    explicit AplMetricsExtensionObserverInterface(int maxMetricIdAllowed) {}

    virtual ~AplMetricsExtensionObserverInterface() = default;

    /**
     * Records a specific counter metric.
     *
     * @param applicationId property identifies the skill for which metric is created.
     * @param experienceId identifies the experience within the skill for which metric is created.
     * @param metricId The timer metric identifier.
     * @param count The duration for the timer metric.
     * @return true if metric is recorded, false otherwise
     */
    virtual bool recordCounter(const std::string& applicationId,
                               const std::string& experienceId,
                               const std::string& metricId,
                               const int count) = 0;

    /**
     * Records a specific timer metric.
     *
     * @param applicationId property identifies the skill for which metric is created.
     * @param experienceId identifies the experience within the skill for which metric is created.
     * @param metricId The timer metric identifier.
     * @param duration The duration for the timer metric.
     * @return true if metric is recorded, false otherwise
     */
    virtual bool recordTimer(const std::string& applicationId,
                             const std::string& experienceId,
                             const std::string& metricId,
                             const std::chrono::milliseconds& duration) = 0;
};

using AplMetricsExtensionObserverInterfacePtr = std::shared_ptr<AplMetricsExtensionObserverInterface>;

} // namespace metrics
} // namespace alexaext

#endif // APL_APLMETRICSEXTENSIONOBSERVERINTERFACE_H
