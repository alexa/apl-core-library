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

#include <fstream>

#include "perftest_base.h"

RootContextPtr PerftestBase::loadInternal(const std::string& document, const std::string& data) {
    // Load the main document
    auto content = Content::create(document);
    assert(content);

    // The document has one import it is waiting for
    while(content->isWaiting()) {
        auto requested = content->getRequestedPackages();
        for (auto& request : requested) {
            content->addPackage(request, packages->at(request.reference().name()));
        }
    }

    content->addData(content->getParameterAt(0), data);

    // Inflate the document
    auto metrics = Metrics().size(800,800).dpi(320);
    auto measure = std::make_shared<MyTextMeasure>();
    RootConfig rootConfig = RootConfig().measure(measure);
    auto ctx = RootContext::create( metrics, content, rootConfig );

    return ctx;
}

std::string PerftestBase::loadFile(const std::string& file) {
    std::ifstream si(file);
    std::stringstream buffer;

    buffer << si.rdbuf();

    return buffer.str();
}

RootContextPtr PerftestBase::load(const std::string& document) {
    auto doc = loadFile("./" + document + ".json");
    auto data = loadFile("./" + document + "_data.json");

    telemetry->startTime(document);
    auto root = loadInternal(doc, data);
    telemetry->endTime(document);

    return root;
}

std::shared_ptr<Telemetry> PerftestBase::getTelemetry() {
    return telemetry;
}

double PerftestBase::extractCounter(const std::string& doc) {
    rapidjson::Document document;
    document.Parse(telemetry->retrieve().c_str());
    auto countersIt = document.FindMember("counters");
    auto& counters = countersIt->value;

    return counters.FindMember((doc + ".Time").c_str())->value.GetDouble()
          /counters.FindMember((doc + ".Time.Times").c_str())->value.GetDouble();
}

void PerftestBase::SetUp() {
    telemetry = Telemetry::create();
    packages = std::make_shared<Packages>();
    packages->emplace("simple_import", loadFile("./simple-import.json"));
    packages->emplace("alexa-viewport-profiles", loadFile("./alexa-viewport-profiles.json"));
    packages->emplace("alexa-styles", loadFile("./alexa-styles.json"));
    packages->emplace("alexa-layouts", loadFile("./alexa-layouts.json"));
}

void PerftestBase::TearDown() {
    telemetry->release();
}
