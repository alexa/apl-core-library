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

#include "alexaext/APLMetricsExtensionV2/AplMetricsExtensionV2.h"

using namespace alexaext;
using namespace alexaext::metrics;
using namespace alexaext::metricsExtensionV2;

static constexpr char METRIC_DESTINATION_PATH[] = "/destination";
static constexpr char METRIC_TYPE_PATH[] = "/type";

static constexpr char COMMAND_INCREMENTCOUNTER_NAME[] = "IncrementCounter";
static constexpr char COMMAND_STARTTIMER_NAME[] = "StartTimer";
static constexpr char COMMAND_STOPTIMER_NAME[] = "StopTimer";
static constexpr char COMMAND_CREATECOUNTER_NAME[] = "CreateCounter";
static constexpr char COMMAND_RECORDVALUE_NAME[] = "RecordValue";

static constexpr char PROPERTY_METRIC_ID[] = "metricId";
static constexpr char PROPERTY_METRIC_NAME[] = "metricName";
static constexpr char PROPERTY_INITIAL_VALUE[] = "initialValue";
static constexpr char PROPERTY_VALUE[] = "value";
static constexpr char PROPERTY_METRIC_DIMENSIONS[] = "dimensions";
static constexpr char PROPERTY_METRIC_DIMENSIONS_PATH[] = "/dimensions";
static constexpr char PROPERTY_AMOUNT[] = "amount";

static constexpr char INCREMENT_COUNTER_DATA_TYPE[] = "IncrementCounterDataType";
static constexpr char CREATE_COUNTER_DATA_TYPE[] = "CreateCounterDataType";
static constexpr char START_TIMER_DATA_TYPE[] = "StartTimerDataType";
static constexpr char STOP_TIMER_DATA_TYPE[] = "StopTimerDataType";
static constexpr char RECORD_VALUE_DATA_TYPE[] = "RecordValueDataType";

static constexpr char STRING_TYPE[] = "String";
static constexpr char INTEGER_TYPE[] = "Integer";
static constexpr char OBJECT_TYPE[] = "object";
static constexpr char DEFAULT_EMPTY_STRING[] = "";

static constexpr char SCHEMA_VERSION[] = "1.0";

AplMetricsExtensionV2::AplMetricsExtensionV2(
    const DestinationFactoryInterfacePtr& destinationFactory, const ExecutorPtr& executor)
    : ExtensionBase(URI_V2), mDestinationFactory(destinationFactory), mExecutor(executor) {}

rapidjson::Document
AplMetricsExtensionV2::createRegistration(const ActivityDescriptor& activity,
                                          const rapidjson::Value& registrationRequest) {
    if (activity.getURI() != URI_V2) {
        return RegistrationFailure::forUnknownURI(activity.getURI());
    }

    auto* settings = RegistrationRequest::SETTINGS().Get(registrationRequest);
    if (!settings || !settings->IsObject()) {
        return RegistrationFailure::forInvalidExtensionSchema(activity.getURI());
    }

    // Validate Destination
    static const rapidjson::Pointer ptr(METRIC_DESTINATION_PATH);
    auto destinationPtr = ptr.Get(*settings);
    if (!destinationPtr || !destinationPtr->IsObject()) {
        return RegistrationFailure::forInvalidExtensionSchema(activity.getURI());
    }

    static const rapidjson::Pointer metricPathPtr(METRIC_TYPE_PATH);
    auto typePtr = metricPathPtr.Get(*destinationPtr);
    if (!typePtr || !typePtr->IsString()) {
        // Destination type required string is not present.
        return RegistrationFailure::forInvalidExtensionSchema(activity.getURI());
    }

    auto destination = mDestinationFactory->createDestination(*settings);
    if (!destination) {
        return RegistrationFailure::forException(activity.getURI(),
                                                 "Unable to setup destination for given type.");
    }

    queueTask([this, activity, destination]() mutable {
        auto activityMetrics = getActivityMetrics(activity);
        if (!activityMetrics) {
            addActivity(activity, destination);
        }
    });

    return RegistrationSuccess(SCHEMA_VERSION)
        .uri(URI_V2)
        .token("<AUTO_TOKEN>")
        .environment([&](Environment& environment) { environment.version(ENVIRONMENT_VERSION_V2); })
        .schema(SCHEMA_VERSION, [&](ExtensionSchema& schema) {
            schema.uri(URI_V2)
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
                .command(COMMAND_CREATECOUNTER_NAME,
                         [](CommandSchema& commandSchema) {
                             commandSchema.dataType(CREATE_COUNTER_DATA_TYPE);
                             commandSchema.allowFastMode(true);
                         })
                .command(COMMAND_INCREMENTCOUNTER_NAME,
                         [](CommandSchema& commandSchema) {
                             commandSchema.dataType(INCREMENT_COUNTER_DATA_TYPE);
                             commandSchema.allowFastMode(true);
                         })
                .command(COMMAND_RECORDVALUE_NAME,
                         [](CommandSchema& commandSchema) {
                             commandSchema.dataType(RECORD_VALUE_DATA_TYPE);
                             commandSchema.allowFastMode(true);
                         })
                .dataType(START_TIMER_DATA_TYPE,
                          [](TypeSchema& typeSchema) {
                              typeSchema
                                  .property(PROPERTY_METRIC_ID,
                                            [](TypePropertySchema& typePropertySchema) {
                                                typePropertySchema.type(STRING_TYPE).required(true);
                                            })
                                  .property(
                                      PROPERTY_METRIC_NAME,
                                      [](TypePropertySchema& typePropertySchema) {
                                          typePropertySchema.type(STRING_TYPE).required(false);
                                      })
                                  .property(PROPERTY_METRIC_DIMENSIONS,
                                            [](TypePropertySchema& typePropertySchema) {
                                                typePropertySchema.type(OBJECT_TYPE)
                                                    .required(false)
                                                    .defaultValue("{}");
                                            });
                          })
                .dataType(STOP_TIMER_DATA_TYPE,
                          [](TypeSchema& typeSchema) {
                              typeSchema.property(
                                  PROPERTY_METRIC_ID, [](TypePropertySchema& typePropertySchema) {
                                      typePropertySchema.type(STRING_TYPE).required(true);
                                  });
                          })
                .dataType(
                    CREATE_COUNTER_DATA_TYPE,
                    [](TypeSchema& typeSchema) {
                        typeSchema
                            .property(PROPERTY_METRIC_ID,
                                      [](TypePropertySchema& typePropertySchema) {
                                          typePropertySchema.type(STRING_TYPE).required(true);
                                      })
                            .property(PROPERTY_METRIC_NAME,
                                      [](TypePropertySchema& typePropertySchema) {
                                          typePropertySchema.type(STRING_TYPE).required(false);
                                      })
                            .property(PROPERTY_INITIAL_VALUE,
                                      [](TypePropertySchema& typePropertySchema) {
                                          typePropertySchema.type(INTEGER_TYPE).defaultValue(0);
                                      })
                            .property(PROPERTY_METRIC_DIMENSIONS,
                                      [](TypePropertySchema& typePropertySchema) {
                                          typePropertySchema.type(OBJECT_TYPE)
                                              .required(false)
                                              .defaultValue("{}");
                                      });
                    })
                .dataType(INCREMENT_COUNTER_DATA_TYPE,
                          [](TypeSchema& typeSchema) {
                              typeSchema
                                  .property(PROPERTY_METRIC_ID,
                                            [](TypePropertySchema& typePropertySchema) {
                                                typePropertySchema.type(STRING_TYPE).required(true);
                                            })
                                  .property(
                                      PROPERTY_AMOUNT, [](TypePropertySchema& typePropertySchema) {
                                          typePropertySchema.type(INTEGER_TYPE).defaultValue(1);
                                      });
                          })
                .dataType(RECORD_VALUE_DATA_TYPE, [](TypeSchema& typeSchema) {
                    typeSchema
                        .property(PROPERTY_METRIC_NAME,
                                  [](TypePropertySchema& typePropertySchema) {
                                      typePropertySchema.type(STRING_TYPE).required(true);
                                  })
                        .property(PROPERTY_VALUE,
                                  [](TypePropertySchema& typePropertySchema) {
                                      typePropertySchema.type(INTEGER_TYPE).required(true);
                                  })
                        .property(PROPERTY_METRIC_DIMENSIONS, [](TypePropertySchema&
                                                                     typePropertySchema) {
                            typePropertySchema.type(OBJECT_TYPE).required(false).defaultValue("{}");
                        });
                });
        });
}

bool
AplMetricsExtensionV2::invokeCommand(const ActivityDescriptor& activity,
                                     const rapidjson::Value& command) {
    Timestamp invokeCommandTime = std::chrono::steady_clock::now();
    const std::string commandName = GetWithDefault(Command::NAME(), command, DEFAULT_EMPTY_STRING);
    if (commandName.empty()) {
        return false;
    }
    const rapidjson::Value* params = Command::PAYLOAD().Get(command);

    std::string metricId = GetWithDefault(PROPERTY_METRIC_ID, params, DEFAULT_EMPTY_STRING);
    if (metricId.empty() && COMMAND_RECORDVALUE_NAME !=
                                commandName) { // If the APL doc explicitly passes empty metricId
        return true; // Do not emmit mertic as per spec, but command succeeds.
    }

    Dimensions dim;
    if (params->HasMember(PROPERTY_METRIC_DIMENSIONS)) {
        dim = getDimensionMap(*params);
    }

    std::string metricName =
        GetWithDefault(PROPERTY_METRIC_NAME, params,
                       metricId); // If the APL doc explicitly passes empty metricName
    if (metricName.empty()) {
        metricName = metricId;
    }

    if (COMMAND_CREATECOUNTER_NAME == commandName) {
        auto amount = GetWithDefault(PROPERTY_INITIAL_VALUE, params, 0);
        return queueTask([this, activity, metricName, metricId, dim, amount]() mutable {
            createCounter(activity, std::move(metricName), std::move(metricId), std::move(dim),
                          amount);
        });
    }

    if (COMMAND_INCREMENTCOUNTER_NAME == commandName) {
        auto amount = GetWithDefault(PROPERTY_AMOUNT, params, 1);
        return queueTask([this, activity, metricId, amount]() mutable {
            incrementCounter(activity, std::move(metricId), amount);
        });
    }

    if (COMMAND_STARTTIMER_NAME == commandName) {
        return queueTask([this, activity, metricName, metricId, dim, invokeCommandTime]() mutable {
            startTimer(activity, std::move(metricName), std::move(metricId), std::move(dim),
                       std::move(invokeCommandTime));
        });
    }

    if (COMMAND_STOPTIMER_NAME == commandName) {
        return queueTask([this, activity, metricId, invokeCommandTime]() mutable {
            stopTimer(activity, metricId, invokeCommandTime);
        });
    }

    if (COMMAND_RECORDVALUE_NAME == commandName) {
        auto value = GetWithDefault(PROPERTY_VALUE, params, 0);
        return queueTask([this, activity, metricName, dim, value]() mutable {
            recordValue(activity, std::move(metricName), std::move(dim), value);
        });
    }

    return false;
}

Dimensions
AplMetricsExtensionV2::getDimensionMap(const rapidjson::Value& param) {
    Dimensions dim;
    static const rapidjson::Pointer ptr(PROPERTY_METRIC_DIMENSIONS_PATH);
    auto dimensionPtr = ptr.Get(param);
    if (!dimensionPtr || !dimensionPtr->IsObject()) {
        return dim;
    }

    for (rapidjson::Value::ConstMemberIterator itr = dimensionPtr->MemberBegin();
         itr != dimensionPtr->MemberEnd(); ++itr) {
        if (itr->value.IsString()) {
            dim[itr->name.GetString()] = itr->value.GetString();
        }
    }
    return dim;
}

bool
AplMetricsExtensionV2::recordValue(const ActivityDescriptor& activity, std::string&& metricName,
                                   Dimensions&& dimensions, int value) {
    MetricTracker metricInfo(std::move(metricName), std::move(dimensions), value);

    auto activityMetricData = getActivityMetrics(activity);
    if (!activityMetricData) {
        return false;
    }

    activityMetricData->publish(std::move(metricInfo.mMetric));
    return true;
}

bool
AplMetricsExtensionV2::createCounter(const ActivityDescriptor& activity, std::string&& metricName,
                                     std::string&& metricId, Dimensions&& dimensions, int amount) {
    auto activityMetricData = getActivityMetrics(activity);
    if (!activityMetricData) {
        return false;
    }

    activityMetricData->createCounter(std::move(metricName), std::move(metricId),
                                      std::move(dimensions), amount);
    return true;
}

bool
AplMetricsExtensionV2::incrementCounter(const ActivityDescriptor& activity, std::string&& metricId,
                                        const int amount) {
    auto activityMetricData = getActivityMetrics(activity);
    if (!activityMetricData) {
        return false;
    }

    activityMetricData->incrementCounter(std::move(metricId), amount);
    return true;
}

bool
AplMetricsExtensionV2::startTimer(const ActivityDescriptor& activity, std::string&& metricName,
                                  std::string&& metricId, Dimensions&& dimensions,
                                  Timestamp&& startTime) {
    auto activityMetricData = getActivityMetrics(activity);
    if (!activityMetricData) {
        return false;
    }

    activityMetricData->startTimer(std::move(metricName), std::move(metricId),
                                   std::move(dimensions), std::move(startTime));
    return true;
}

bool
AplMetricsExtensionV2::stopTimer(const ActivityDescriptor& activity, const std::string& metricId,
                                 const Timestamp& stopTime) {
    auto activityMetricData = getActivityMetrics(activity);
    if (!activityMetricData) {
        return false;
    }

    Metric timerMetric;
    if (!activityMetricData->stopTimer(metricId, timerMetric, stopTime)) {
        return false;
    }

    activityMetricData->publish(std::move(timerMetric));
    return true;
}

void
AplMetricsExtensionV2::onActivityUnregistered(const ActivityDescriptor& activity) {
    queueTask([this, activity]() mutable {
        auto activtyMetrics = removeActivity(activity);
        if (!activtyMetrics) {
            return;
        }
        activtyMetrics->publish();
    });
}

bool
AplMetricsExtensionV2::queueTask(Executor::Task&& task) {
    auto executor = mExecutor.lock();
    if (!executor) {
        return false;
    }

    return executor->enqueueTask(std::move(task));
}

bool
AplMetricsExtensionV2::addActivity(
    const alexaext::ActivityDescriptor& activity,
    const std::shared_ptr<DestinationInterface>& destinationInterface) {
    if (mActivityMetricKeysMap.find(activity) != mActivityMetricKeysMap.end()) {
        // Activity already registered
        return false;
    }

    mActivityMetricKeysMap[activity] = std::make_shared<MetricDataList>(destinationInterface);
    return true;
}

std::shared_ptr<AplMetricsExtensionV2::MetricDataList>
AplMetricsExtensionV2::removeActivity(const alexaext::ActivityDescriptor& activity) {
    std::shared_ptr<MetricDataList> ret;
    auto itr = mActivityMetricKeysMap.find(activity);
    if (itr != mActivityMetricKeysMap.end()) {
        ret = itr->second;
        mActivityMetricKeysMap.erase(activity);
    }

    return ret;
}

std::shared_ptr<AplMetricsExtensionV2::MetricDataList>
AplMetricsExtensionV2::getActivityMetrics(const alexaext::ActivityDescriptor& activity) {
    auto itr = mActivityMetricKeysMap.find(activity);
    if (itr != mActivityMetricKeysMap.end()) {
        return itr->second;
    }
    return nullptr;
}

//==================================
// Definitions from MetricDataList
AplMetricsExtensionV2::MetricDataList::MetricDataList(
    const std::shared_ptr<DestinationInterface>& destinationInterface)
    : mDestinationInterface(destinationInterface) {}

void
AplMetricsExtensionV2::MetricDataList::publish(Metric&& metric) {
    mDestinationInterface->publish(metric);
}

void
AplMetricsExtensionV2::MetricDataList::publish() {
    std::vector<Metric> counterMetrics;
    counterMetrics.reserve(mMetricIdCounterMetricData.size());
    for (const auto& metric : mMetricIdCounterMetricData) {
        counterMetrics.emplace(counterMetrics.end(), metric.second->getMetric());
    }

    if (!counterMetrics.empty()) {
        mDestinationInterface->publish(std::move(counterMetrics));
    }

    mMetricIdCounterMetricData.clear();
}

void
AplMetricsExtensionV2::MetricDataList::createCounter(std::string&& metricName,
                                                     std::string&& metricId,
                                                     Dimensions&& dimensions, int amount) {
    auto counterTracker = std::make_shared<CounterMetricTracker>(std::move(metricName),
                                                                 std::move(dimensions), amount);
    mMetricIdCounterMetricData[metricId] = std::move(counterTracker);
}

void
AplMetricsExtensionV2::MetricDataList::incrementCounter(std::string&& metricId, int amount) {
    auto counterMetricItr = mMetricIdCounterMetricData.find(metricId);
    if (counterMetricItr == mMetricIdCounterMetricData.end()) {
        // If counter not present, create a new counter
        // In this case metricId becomes the metricName as well
        std::string metricName = metricId;
        createCounter(std::move(metricName), std::move(metricId), {}, amount);
    }
    else {
        auto& counterMetricTracker = counterMetricItr->second;
        counterMetricTracker->incrementCounter(amount);
    }
}

void
AplMetricsExtensionV2::MetricDataList::startTimer(std::string&& metricName, std::string&& metricId,
                                                  Dimensions&& dimensions, Timestamp&& startTime) {
    auto timerTracker = std::make_shared<TimerMetricTracker>(
        std::move(metricName), std::move(dimensions), std::move(startTime));
    mMetricIdTimerData[metricId] = std::move(timerTracker);
}

bool
AplMetricsExtensionV2::MetricDataList::stopTimer(const std::string& metricId, Metric& metric,
                                                 const Timestamp& stopTime) {
    auto timerMetricItr = mMetricIdTimerData.find(metricId);
    if (timerMetricItr == mMetricIdTimerData.end()) {
        return false;
    }

    auto& timerMetric = timerMetricItr->second;
    return timerMetric->stop(metric, stopTime);
}
// Definitions from MetricDataList
//==================================
