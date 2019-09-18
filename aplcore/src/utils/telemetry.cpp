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

#ifdef WITH_TELEMETRY
#warning "Telemetry is enabled. This could affect performance."

#include <chrono>

#include "apl/utils/telemetry.h"

namespace apl {

std::shared_ptr<Telemetry> Telemetry::create() {
    return std::make_shared<Telemetry>("Root");
};

std::shared_ptr<Telemetry> Telemetry::create(const std::string &name) {
    auto newTelemetry = std::make_shared<Telemetry>(name);
    this->mChildren.push_back(newTelemetry);
    return newTelemetry;
};

std::string Telemetry::retrieve() {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    collect(&writer);

    return buffer.GetString();
}

void Telemetry::collect(rapidjson::Writer<rapidjson::StringBuffer>* writer) {
    writer->StartObject();
    writer->Key("name");
    writer->String(mName.c_str());

    writer->Key("counters");
    writer->StartObject();
    for(auto& counter : mCounters) {
        writer->Key(counter.first.c_str());
        writer->Int(counter.second);
        writer->Key((counter.first + ".Times").c_str());
        writer->Int(mCounts[counter.first]);
    }
    writer->EndObject();

    writer->Key("metadata");
    writer->StartObject();
    for(auto& data : mMetadata) {
        writer->Key(data.first.c_str());
        writer->String(data.second.c_str());
    }
    writer->EndObject();

    writer->Key("mChildren");
    writer->StartArray();
    for (auto& child : mChildren) {
        child->collect(writer);
    }
    writer->EndArray();

    writer->EndObject();
}

void Telemetry::release() {
    for (auto& child : mChildren) {
        child->release();
    }

    mChildren.clear();
    mCounters.clear();
    mTimers.clear();
    mCounts.clear();
    mMetadata.clear();
}

void Telemetry::addTime(const std::string& name, uint32_t ms) {
    addCounter(name + ".Time", ms);
}

void Telemetry::startTime(const std::string& name) {
    auto start = std::chrono::system_clock::now();
    if(mTimers.find(name) != mTimers.end()) {
        mTimers[name] = start;
    } else {
        mTimers.emplace(name, start);
    }
}

void Telemetry::endTime(const std::string& name) {
    auto end = std::chrono::system_clock::now();
    // uint32_t is 49 days in milliseconds so unlikely to overflow
    addTime(name, std::chrono::duration_cast<std::chrono::milliseconds>(end - mTimers[name]).count());
}

void Telemetry::addCounter(const std::string& name, uint32_t count) {
    if(mCounters.count(name)) {
        mCounters[name] += count;
        mCounts[name] ++;
    } else {
        mCounters.emplace(name, count);
        mCounts.emplace(name, 1);
    }
}

void Telemetry::addData(const std::string& name, const std::string& data) {
    if(mMetadata.count(name)) {
        mMetadata[name] = data;
    } else {
        mMetadata.emplace(name, data);
    }
}

} // namespace apl

#endif // WITH_TELEMETRY