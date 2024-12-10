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
#include <rapidjson/document.h>
#include <utility>

#include "alexaext/APLMetricsExtension/AplMetricsExtension.h"

using namespace alexaext;
using namespace alexaext::metrics;

static const std::string MAX_METRIC_ID_ALLOWED = "maxMetricIdAllowed";

static const char* APPLICATION_ID = "applicationId";
static const char* EXPERIENCE_ID = "experienceId";

static const std::string COMMAND_INCREMENTCOUNTER_NAME = "IncrementCounter";
static const std::string COMMAND_STARTTIMER_NAME = "StartTimer";
static const std::string COMMAND_STOPTIMER_NAME = "StopTimer";

static const char* PROPERTY_METRIC_ID = "metricId";
static const char* PROPERTY_AMOUNT = "amount";

static const std::string INCREMENT_COUNTER_DATA_TYPE = "IncrementCounterDataType";
static const std::string START_TIMER_DATA_TYPE = "StartTimerDataType";
static const std::string STOP_TIMER_DATA_TYPE = "StopTimerDataType";

static const std::string STRING_TYPE = "String";
static const std::string INTEGER_TYPE = "Integer";

static const std::string SCHEMA_VERSION = "1.0";

AplMetricsExtension::AplMetricsExtension(AplMetricsExtensionObserverInterfacePtr observer,
                                         ExecutorPtr executor,
                                         int maxMetricIdAllowed)
    : ExtensionBase(URI),
      mObserver(std::move(observer)),
      mExecutor(executor),
      mMaxMetricIdAllowed(maxMetricIdAllowed) {}

rapidjson::Document
AplMetricsExtension::createRegistration(const ActivityDescriptor& activity,
                                        const rapidjson::Value& registrationRequest)
{
    if (activity.getURI() != URI)
        return RegistrationFailure::forUnknownURI(activity.getURI());

    std::string applicationId;
    std::string experienceId;
    const auto *settings = RegistrationRequest::SETTINGS().Get(registrationRequest);
    if (!settings || !settings->IsObject())
        return RegistrationFailure::forInvalidMessage(activity.getURI());

    auto settingsObject = settings->GetObject();
    if (!settingsObject.HasMember(APPLICATION_ID) || !settingsObject[APPLICATION_ID].IsString())
        return RegistrationFailure::forInvalidMessage(activity.getURI());

    applicationId = settingsObject[APPLICATION_ID].GetString();
    if (applicationId.empty())
        return RegistrationFailure::forInvalidMessage(activity.getURI());

    if (settingsObject.HasMember(EXPERIENCE_ID) && settingsObject[EXPERIENCE_ID].IsString())
        experienceId = settingsObject[EXPERIENCE_ID].GetString();

    {
        std::lock_guard<std::recursive_mutex> lock(mSessionMetricsMutex);
        auto sessionMetrics = getSessionMetrics(*activity.getSession());
        if (!sessionMetrics) {
            sessionMetrics = std::make_shared<SessionMetricData>();
            mSessionMetricsMap.insert(
                std::make_pair(*activity.getSession(), sessionMetrics));
        }

        if (!sessionMetrics->createMetricData(activity, applicationId, experienceId))
            return RegistrationFailure::forException(activity.getURI(), "Activity already registered");

    }

    return RegistrationSuccess(SCHEMA_VERSION)
        .uri(URI)
        .token("<AUTO_TOKEN>")
        .environment([&](Environment& environment) {
          environment.version(ENVIRONMENT_VERSION);
          environment.property(MAX_METRIC_ID_ALLOWED, mMaxMetricIdAllowed);
        })
        .schema(SCHEMA_VERSION, [&](ExtensionSchema& schema) {
            schema.uri(URI)
                .command(COMMAND_INCREMENTCOUNTER_NAME,
                         [](CommandSchema& commandSchema) {
                             commandSchema.dataType(INCREMENT_COUNTER_DATA_TYPE);
                             commandSchema.allowFastMode(true);
                         })
                .command(COMMAND_STARTTIMER_NAME,
                         [](CommandSchema& commandSchema) {
                             commandSchema.dataType(START_TIMER_DATA_TYPE);
                             commandSchema.allowFastMode(true);
                         })
                .command(COMMAND_STOPTIMER_NAME,
                         [](CommandSchema& commandSchema) {
                             commandSchema.dataType(STOP_TIMER_DATA_TYPE);
                             commandSchema.allowFastMode(true);
                         })
                .dataType(INCREMENT_COUNTER_DATA_TYPE, [](TypeSchema& typeSchema) {
                              typeSchema
                                  .property(
                                      PROPERTY_METRIC_ID, [](TypePropertySchema& typePropertySchema) {
                                          typePropertySchema.type(STRING_TYPE).required(true);
                                      })
                                  .property(
                                      PROPERTY_AMOUNT, [](TypePropertySchema& typePropertySchema) {
                                          typePropertySchema.type(INTEGER_TYPE).defaultValue(1);
                                      });
                          })
                .dataType(START_TIMER_DATA_TYPE, [](TypeSchema& typeSchema) {
                              typeSchema
                                  .property(
                                      PROPERTY_METRIC_ID, [](TypePropertySchema& typePropertySchema) {
                                          typePropertySchema.type(STRING_TYPE).required(true);
                                      });
                          })
                .dataType(STOP_TIMER_DATA_TYPE, [](TypeSchema& typeSchema) {
                             typeSchema
                                  .property(
                                      PROPERTY_METRIC_ID, [](TypePropertySchema& typePropertySchema) {
                                                typePropertySchema.type(STRING_TYPE).required(true);
                                      });
                });
        });
}

bool
AplMetricsExtension::invokeCommand(const ActivityDescriptor& activity, const rapidjson::Value& command)
{
    if (!mObserver)
        return false;

    auto sessionMetrics = getSessionMetrics(*activity.getSession());
    if (!sessionMetrics)
        return false;

    const std::string commandName = GetWithDefault(Command::NAME(), command, "");
    const rapidjson::Value* params = Command::PAYLOAD().Get(command);

    if (!params || !params->HasMember(PROPERTY_METRIC_ID))
        return false;

    std::string metricId = GetWithDefault(PROPERTY_METRIC_ID, params, "");

    if (metricId.empty())
        return false;

    if (COMMAND_INCREMENTCOUNTER_NAME == commandName) {
        int amount = GetAsIntWithDefault(PROPERTY_AMOUNT, params, 1);
        return incrementCounter(activity, metricId, amount);
    }

    if (COMMAND_STARTTIMER_NAME == commandName)
        return startTimer(activity, metricId);

    if (COMMAND_STOPTIMER_NAME == commandName)
        return stopTimer(activity, metricId);

    return false;
}

bool
AplMetricsExtension::incrementCounter(const ActivityDescriptor& activity,
                                      const std::string metricId,
                                      const int amount)
{
    auto activityMetricData = getActivityMetrics(activity);
    if (!activityMetricData)
        return false;

    if (activityMetricData->isMaxLimitExceeded(metricId, mMaxMetricIdAllowed))
        return false;

    activityMetricData->incrementCounter(metricId, amount);
    return true;
}

bool
AplMetricsExtension::startTimer(const ActivityDescriptor& activity,
                                const std::string metricId)
{
    auto activityMetricData = getActivityMetrics(activity);
    if (!activityMetricData)
        return false;

    if (activityMetricData->isMaxLimitExceeded(metricId, mMaxMetricIdAllowed))
        return false;

    auto timer = activityMetricData->getOrCreateTimer(metricId);
    timer->start();
    return true;
}

bool
AplMetricsExtension::stopTimer(const ActivityDescriptor& activity,
                               const std::string metricId)
{
    auto executor = mExecutor.lock();
    if (!executor)
        return false;

    auto activityMetricData = getActivityMetrics(activity);
    if (!activityMetricData)
        return false;

    auto timer = activityMetricData->getOrCreateTimer(metricId);

    if (timer->started) {
        auto applicationId = activityMetricData->applicationId;
        auto experienceId = activityMetricData->experienceId;
        auto duration = timer->stop();
        executor->enqueueTask(
            [=]() { mObserver->recordTimer(applicationId, experienceId, metricId, duration); });
        return true;
    }
    return false;
}

std::shared_ptr<AplMetricsExtension::SessionMetricData>
AplMetricsExtension::getSessionMetrics(const SessionDescriptor& session)
{
    std::lock_guard<std::recursive_mutex> lock(mSessionMetricsMutex);
    auto sessionMetricDataItr = mSessionMetricsMap.find(session);
    if (sessionMetricDataItr == mSessionMetricsMap.end())
        return nullptr;
    return sessionMetricDataItr->second;
}

std::shared_ptr<AplMetricsExtension::MetricData>
AplMetricsExtension::getActivityMetrics(const ActivityDescriptor& activity)
{
    auto sessionMetrics = getSessionMetrics(*activity.getSession());
    if (!sessionMetrics)
        return nullptr;
    return sessionMetrics->getActivityMetrics(activity);
}

void
AplMetricsExtension::onSessionEnded(const SessionDescriptor& session)
{
    std::lock_guard<std::recursive_mutex> lock(mSessionMetricsMutex);
    auto sessionMetrics = getSessionMetrics(session);
    if (!sessionMetrics)
        return;

    auto executor = mExecutor.lock();
    if (!executor)
        return;

    for (auto metricData : sessionMetrics->getAllMetrics()) {
        auto counterMetrics = metricData->getCounters();
        for (auto counterMetric = counterMetrics.begin(); counterMetric != counterMetrics.end();
             counterMetric++) {
            auto applicationId = metricData->applicationId;
            auto experienceId = metricData->experienceId;
            auto metricId = counterMetric->first;
            auto count = counterMetric->second;
            executor->enqueueTask([=]() {
                mObserver->recordCounter(applicationId, experienceId, metricId, count);
            });
        }
    }

    mSessionMetricsMap.erase(session);
}