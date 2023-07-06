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
#ifndef APL_APLMETRICSEXTENSION_H
#define APL_APLMETRICSEXTENSION_H

#include <chrono>
#include <climits>
#include <mutex>
#include <memory>
#include <string>
#include <unordered_map>

#include <alexaext/alexaext.h>

#include <rapidjson/document.h>

#include "AplMetricsExtensionObserverInterface.h"

namespace alexaext {
namespace metrics {

using Timestamp = std::chrono::steady_clock::time_point;

static const std::string URI = "aplext:metrics:10";
static const std::string ENVIRONMENT_VERSION = "APLMetricsExtension-1.0";

/**
 * The metrics extension that enables generating metrics from APL document.
 *
 * This extension follows the observer model, where a common logic delegates to an observer
 * the underlying behavior.
 */
class AplMetricsExtension
    : public alexaext::ExtensionBase,
      public std::enable_shared_from_this<AplMetricsExtension> {
public:
    /**
     * Constructor
     * @param observer AplMetricsExtensionObserverInterface instance to report report metrics
     * @param executor Extension task executor, observer API are invoked as asynchronous tasks on this.
     * @param maxMetricIdAllowed Optional max unique number of metricId allowed for an experience.
     */
    AplMetricsExtension(
        AplMetricsExtensionObserverInterfacePtr observer,
        alexaext::ExecutorPtr executor,
        int maxMetricIdAllowed = INT_MAX);

    ~AplMetricsExtension() override = default;

    /// @name alexaext::Extension Functions
    /// @{

    rapidjson::Document createRegistration(const alexaext::ActivityDescriptor& activity,
                                           const rapidjson::Value& registrationRequest) override;

    bool invokeCommand(const alexaext::ActivityDescriptor& activity, const rapidjson::Value& command) override;

    void onSessionEnded(const alexaext::SessionDescriptor& session) override;

    /// @}

private:
    /**
     * Timer for tracking start time for timer metrics.
     */
    struct Timer {

        /**
         * Mark the timer as started and note the current timestamp as start time.
         */
        void start() {
            startTime = std::chrono::steady_clock::now();
            started = true;
        }

        /**
         * Stop the timer.
         *
         * @return duration from start time to now, zero if timer is not started or already stopped.
         */
        std::chrono::milliseconds stop() {
            if (!started) return std::chrono::milliseconds(0);

            started = false;
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime);
        }

        bool started;
        Timestamp startTime;
    };

    /**
     * Utility to track metric data for an experience within an application registered.
     */
    struct MetricData {
    public:
        MetricData(const std::string& applicationId, const std::string& experienceId)
            : applicationId(applicationId), experienceId(experienceId) {}

        void incrementCounter(const std::string& metricId, int amount) {
            std::lock_guard<std::mutex> lock(mMetricDataMutex);
            auto counterMetricItr = mCounterMetricsMap.find(metricId);
            if (counterMetricItr == mCounterMetricsMap.end()) {
                mMetricIdSet.insert(metricId);
                mCounterMetricsMap.insert(std::pair<std::string, int>(metricId, amount));
            } else {
                counterMetricItr->second += amount;
            }
        }

        std::shared_ptr<Timer> getOrCreateTimer(const std::string& metricId) {
            std::lock_guard<std::mutex> lock(mMetricDataMutex);
            auto timerMetricItr = mMetricTimerMap.find(metricId);
            if (timerMetricItr == mMetricTimerMap.end()) {
                auto timer = std::make_shared<Timer>();
                mMetricIdSet.insert(metricId);
                mMetricTimerMap.insert(std::pair<std::string, std::shared_ptr<Timer>>(metricId, timer));
                return timer;
            } else {
                return timerMetricItr->second;
            }
        }

        bool isMaxLimitExceeded(const std::string& metricId, const int maxLimit) {
            std::lock_guard<std::mutex> lock(mMetricDataMutex);
            int newCount = mMetricIdSet.size();
            newCount += mMetricIdSet.find(metricId) == mMetricIdSet.end() ? 1 : 0;
            return newCount > maxLimit;
        }

        std::map<std::string, int>& getCounters() {
            return mCounterMetricsMap;
        }

    public:
        std::string applicationId;
        std::string experienceId;
    private:
        std::set<std::string> mMetricIdSet;
        std::map<std::string, int> mCounterMetricsMap;
        std::unordered_map<std::string, std::shared_ptr<Timer>> mMetricTimerMap;
        std::mutex mMetricDataMutex;
    };

    /**
     * Track metric data within a session. A session can have multiple activities and each activity
     * is associated with an application id experience id.
     *
     * Metrics within a session are tracked for a unique combination of {applicationId, experienceId}
     * and thus can span across activities within a session. For e.g. a timer metric can be started
     * in one activity and stopped in another, similarly a counter metric can be incremented by
     * multiple activities in a session, the final count is reported when session ends.
     */
    struct SessionMetricData {
    public:
        bool createMetricData(const alexaext::ActivityDescriptor& activity,
                              const std::string& applicationId,
                              const std::string& experienceId)
        {
            std::lock_guard<std::mutex> lock(mSessionMutex);
            auto metricKey = applicationId + "." + experienceId;
            if (mActivityMetricKeysMap.find(activity) != mActivityMetricKeysMap.end()) {
                // Activity already registered
                return false;
            }

            auto metricData = std::make_shared<MetricData>(applicationId, experienceId);
            mApplicationMetricMap.insert(std::make_pair(metricKey, metricData));
            mActivityMetricKeysMap.insert(std::make_pair(activity, metricKey));
            return true;

        }

        std::shared_ptr<MetricData>
        getActivityMetrics(const alexaext::ActivityDescriptor& activity)
        {
            std::lock_guard<std::mutex> lock(mSessionMutex);
            auto itr = mActivityMetricKeysMap.find(activity);
            if (itr != mActivityMetricKeysMap.end()) {
                auto applicationMetricItr = mApplicationMetricMap.find(itr->second);
                if (applicationMetricItr != mApplicationMetricMap.end())
                    return applicationMetricItr->second;
            }
            return nullptr;
        }

        std::vector<std::shared_ptr<MetricData>> getAllMetrics()
        {
            std::lock_guard<std::mutex> lock(mSessionMutex);
            std::vector<std::shared_ptr<MetricData>> sessionMetrics;
            for (auto itr = mApplicationMetricMap.begin(); itr != mApplicationMetricMap.end(); itr++) {
                sessionMetrics.emplace_back(itr->second);
            }
            return sessionMetrics;
        }
    private:
        std::unordered_map<std::string, std::shared_ptr<MetricData>> mApplicationMetricMap;
        std::unordered_map<alexaext::ActivityDescriptor,
            std::string,
            alexaext::ActivityDescriptor::Hash> mActivityMetricKeysMap;
        std::mutex mSessionMutex;
    };

private:
    std::shared_ptr<SessionMetricData> getSessionMetrics(const alexaext::SessionDescriptor& session);
    std::shared_ptr<MetricData> getActivityMetrics(const alexaext::ActivityDescriptor& activity);
    bool incrementCounter(const alexaext::ActivityDescriptor& activity,
                          const std::string metricId,
                          const int amount);
    bool startTimer(const alexaext::ActivityDescriptor& activity, const std::string metricId);
    bool stopTimer(const alexaext::ActivityDescriptor& activity, const std::string metricId);

private:
    std::unordered_map<alexaext::SessionDescriptor,
                       std::shared_ptr<SessionMetricData>,
                       alexaext::SessionDescriptor::Hash> mSessionMetricsMap;
    AplMetricsExtensionObserverInterfacePtr mObserver;
    std::weak_ptr<alexaext::Executor> mExecutor;
    std::recursive_mutex mSessionMetricsMutex;
    const int mMaxMetricIdAllowed;
};

using AplMetricsExtensionPtr = std::shared_ptr<AplMetricsExtension>;

} // namespace metrics
} // namespace alexaext

#endif // APL_APLMETRICSEXTENSION_H
