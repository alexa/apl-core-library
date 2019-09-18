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

#ifndef _APL_TELEMETRY_H
#define _APL_TELEMETRY_H

#ifdef WITH_TELEMETRY
#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"

namespace apl {

class Telemetry {
public:
    /**
     * Create default root telemetry object.
     *
     * @return Shared pointer to created Telemetry object.
     */
    static std::shared_ptr<Telemetry> create();

    /**
     * Create child telemetry object.
     *
     * @return Shared pointer to created Telemetry object.
     */
    std::shared_ptr<Telemetry> create(const std::string &name);

    /**
     * Constructor.
     * @param name Telemetry object name.
     */
    Telemetry(const std::string &name) : mName{name} {};

    /**
     * Delete default constructor.
     */
    Telemetry() = delete;

    /**
     * Retrieve current telemetry state.
     *
     * @return JSON formatted string.
     */
    std::string retrieve();

    /**
     * Reset current telemetry state. Current object can be reused but all child objects will be removed.
     */
    void release();

    /**
     * Add time metric. Output name will have ".Time" suffix. Subsequent calls with the same name will add time
     * up to existing metric.
     *
     * @param name Metric name.
     * @param ms Time in milliseconds.
     */
    void addTime(const std::string &name, uint32_t ms);

    /**
     * Start time metric. Should be followed up with @see Telemetry::endTime(name). Subsequent calls of start/stop
     * pair with the same name will add time up to existing metric.
     *
     * @param name Metric name.
     */
    void startTime(const std::string &name);

    /**
     * Finalize and record time metric.
     *
     * @param name Metric name. Should be same as used for @see Telemetry::startTime(name)
     */
    void endTime(const std::string &name);

    /**
     * Add counter. Subsequent calls with the same name will add counter up to existing metric.
     *
     * @param name Metric name
     * @param count Counter to be added.
     */
    void addCounter(const std::string &name, uint32_t count);

    /**
     * Add arbitrary data to the telemetry object. Subsequent calls with the same name will overwrite existing metric.
     *
     * @param name Field name.
     * @param data Arbitrary data as string.
     */
    void addData(const std::string &name, const std::string &data);

private:
    const std::string mName;
    std::map<std::string, uint32_t> mCounters;
    std::map<std::string, std::chrono::time_point<std::chrono::system_clock>> mTimers;
    std::map<std::string, uint32_t> mCounts;
    std::map<std::string, std::string> mMetadata;

    std::vector<std::shared_ptr<Telemetry>> mChildren;

    void collect(rapidjson::Writer<rapidjson::StringBuffer>* writer);
};

} // namespace apl

#define TELEMETRY(code) code

#else // WITH_TELEMETRY

#define TELEMETRY(code)

#endif // WITH_TELEMETRY

#endif // _APL_TELEMETRY_H
