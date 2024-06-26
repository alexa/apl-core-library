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

#ifndef APL_APLMETRICSEXTENSIONV2_H
#define APL_APLMETRICSEXTENSIONV2_H

#include <chrono>
#include <climits>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <alexaext/alexaext.h>

#include <rapidjson/document.h>

#include "DestinationFactoryInterface.h"
#include "DestinationInterface.h"
#include "MetricData.h"
#include "MetricTracker.h"

namespace alexaext {
namespace metricsExtensionV2 {

static constexpr char URI_V2[] = "aplext:metrics:20";
static constexpr char ENVIRONMENT_VERSION_V2[] = "2.0";

/**
 * The metrics extension that enables generating metrics from APL document.
 *
 * This extension implements the metric logic and delegates the publishing of metircs to @c
 * DestinationInterface.
 */
class AplMetricsExtensionV2 : public alexaext::ExtensionBase,
                              public std::enable_shared_from_this<AplMetricsExtensionV2> {
public:
    /**
     * Constructor
     * @param destinationFactoryInterface is the factory class to get the destination to which the
     * metric is to published.
     * @param executor Extension task executor, publish API's are invoked as
     * asynchronous tasks on this.
     */
    AplMetricsExtensionV2(const DestinationFactoryInterfacePtr& destinationFactoryInterface,
                          const alexaext::ExecutorPtr& executor);

    ~AplMetricsExtensionV2() override = default;

    /// @name alexaext::Extension Functions
    /// @{

    rapidjson::Document createRegistration(const alexaext::ActivityDescriptor& activity,
                                           const rapidjson::Value& registrationRequest) override;

    bool invokeCommand(const alexaext::ActivityDescriptor& activity,
                       const rapidjson::Value& command) override;

    void onActivityUnregistered(const ActivityDescriptor& activity) override;

    /// @}

private:
    /**
     * Utility to track metric data per activity
     */
    class MetricDataList {
    public:
        MetricDataList(const std::shared_ptr<DestinationInterface>& destinationInterface);

        void publish(Metric&& metric);

        void publish();

        void createCounter(std::string&& metricName, std::string&& metricId,
                           Dimensions&& dimensions, int amount);

        void incrementCounter(std::string&& metricId, int amount);

        void startTimer(std::string&& metricName, std::string&& metricId, Dimensions&& dimensions,
                        Timestamp&& startTime);

        bool stopTimer(const std::string& metricId, Metric& metric, const Timestamp& stopTime);

    private:
        std::shared_ptr<DestinationInterface> mDestinationInterface;
        std::map<std::string, std::shared_ptr<CounterMetricTracker>> mMetricIdCounterMetricData;
        std::unordered_map<std::string, std::shared_ptr<TimerMetricTracker>> mMetricIdTimerData;
    };

    bool addActivity(const alexaext::ActivityDescriptor& activity,
                     const std::shared_ptr<DestinationInterface>& destinationInterface);

    bool queueTask(Executor::Task&& task);

    std::shared_ptr<AplMetricsExtensionV2::MetricDataList>
    removeActivity(const alexaext::ActivityDescriptor& activity);

    std::shared_ptr<AplMetricsExtensionV2::MetricDataList>
    getActivityMetrics(const alexaext::ActivityDescriptor& activity);

    bool createCounter(const ActivityDescriptor& activity, std::string&& metricName,
                       std::string&& metricId, Dimensions&& dimensions, int amount);

    bool incrementCounter(const alexaext::ActivityDescriptor& activity, std::string&& metricId,
                          const int amount);

    bool startTimer(const alexaext::ActivityDescriptor& activity, std::string&& metricName,
                    std::string&& metricId, Dimensions&& dimensions, Timestamp&& startTime);

    bool stopTimer(const alexaext::ActivityDescriptor& activity, const std::string& metricId,
                   const Timestamp& stopTime);

    bool recordValue(const ActivityDescriptor& activity, std::string&& metricName,
                     Dimensions&& dimensions, int value);

    Dimensions getDimensionMap(const rapidjson::Value& param);

private:
    DestinationFactoryInterfacePtr mDestinationFactory;
    std::weak_ptr<alexaext::Executor> mExecutor;
    std::unordered_map<alexaext::ActivityDescriptor, std::shared_ptr<MetricDataList>,
                       alexaext::ActivityDescriptor::Hash>
        mActivityMetricKeysMap;
};

using AplMetricsExtension2Ptr = std::shared_ptr<AplMetricsExtensionV2>;

} // namespace metricsExtensionV2
} // namespace alexaext

#endif // APL_APLMETRICSEXTENSIONV2_H
