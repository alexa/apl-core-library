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
#include "alexaext/APLAttentionSystemExtension/AplAttentionSystemExtension.h"

#include <rapidjson/document.h>

using namespace alexaext;
using namespace alexaext::attention;

// version of the extension definition Schema
static const char *SCHEMA_VERSION = "1.0";

// Settings read on registration
static const char *SETTING_ATTENTION_SYSTEM_STATE_NAME = "attentionSystemStateName";

// Events
static const char* ON_ATTENTION_STATE_CHANGED = "OnAttentionStateChanged";

// Data Types
static const char *DATA_TYPE_ATTENTION_STATE = "AttentionSystemState";
static const char *PROPERTY_ATTENTION_STATE = "attentionState";

AplAttentionSystemExtension::AplAttentionSystemExtension(
    alexaext::ExecutorPtr executor,
    alexaext::uuid::UUIDFunction uuidGenerator)
    : alexaext::ExtensionBase(URI),
      mExecutor(executor),
      mUuidGenerator(std::move(uuidGenerator)) {}


rapidjson::Document
AplAttentionSystemExtension::createRegistration(const ActivityDescriptor& activity,
                                                const rapidjson::Value& registrationRequest)
{
    if (activity.getURI() != URI)
        return RegistrationFailure::forUnknownURI(activity.getURI());

    const auto *settingsValue = RegistrationRequest::SETTINGS().Get(registrationRequest);
    if (settingsValue) {
        applySettings(activity, *settingsValue);
    }

    return RegistrationSuccess(SCHEMA_VERSION)
        .uri(URI)
        .token(mUuidGenerator())
        .environment([](Environment& environment) {
            environment.version(ENVIRONMENT_VERSION);
        })
        .schema(SCHEMA_VERSION, [&](ExtensionSchema& schema) {
            schema.uri(URI)
                .dataType(DATA_TYPE_ATTENTION_STATE, [](TypeSchema &dataTypeSchema) {
                    dataTypeSchema
                        .property(PROPERTY_ATTENTION_STATE, "string");
                })
                .event(ON_ATTENTION_STATE_CHANGED);
            std::lock_guard<std::recursive_mutex> lock(mAttentionStateNameMutex);
            auto entry = mAttentionStateNameMap.find(activity);
            if (entry != mAttentionStateNameMap.end()) {
                schema.liveDataMap(entry->second, [] (LiveDataSchema &liveDataSchema) {
                    liveDataSchema.dataType(DATA_TYPE_ATTENTION_STATE);
                });
            }
        });
}

void
AplAttentionSystemExtension::updateAttentionState(const AttentionState& newState)
{
    mAttentionState = newState;
    std::lock_guard<std::recursive_mutex> lock(mAttentionStateNameMutex);

    for (auto entry : mAttentionStateNameMap) {
        auto event = Event(SCHEMA_VERSION).uri(URI).target(URI)
                        .name(ON_ATTENTION_STATE_CHANGED)
                        .property(PROPERTY_ATTENTION_STATE, attentionStateToString(newState));

        publishLiveData(entry.first);
        invokeExtensionEventHandler(entry.first, event);
    }
}

void
AplAttentionSystemExtension::applySettings(const ActivityDescriptor& activity, const rapidjson::Value &settings)
{
    if (!settings.IsObject())
        return;

    /// Apply document assigned settings
    auto attentionSystemStateName = settings.FindMember(SETTING_ATTENTION_SYSTEM_STATE_NAME);
    if (attentionSystemStateName != settings.MemberEnd() && attentionSystemStateName->value.IsString()) {
        mAttentionStateNameMap.emplace(activity, settings[SETTING_ATTENTION_SYSTEM_STATE_NAME].GetString());
    }
}

void
AplAttentionSystemExtension::publishLiveData(const ActivityDescriptor& activity)
{

    std::string attentionStateName;
    {
        std::lock_guard<std::recursive_mutex> lock(mAttentionStateNameMutex);
        auto entry = mAttentionStateNameMap.find(activity);

        // live data is not used if it has not been named
        if (entry == mAttentionStateNameMap.end())
            return;

        attentionStateName = entry->second;
    }

    //  Publish attention state
    auto liveDataUpdate = LiveDataUpdate(SCHEMA_VERSION)
        .uri(URI)
        .objectName(attentionStateName)
        .target(URI)
        .liveDataMapUpdate([&](LiveDataMapOperation &operation) {
            operation
                .type("Set")
                .key(PROPERTY_ATTENTION_STATE)
                .item(attentionStateToString(mAttentionState));
        });
    invokeLiveDataUpdate(activity, liveDataUpdate);
}

void
AplAttentionSystemExtension::onActivityUnregistered(const ActivityDescriptor& activity)
{
    mAttentionStateNameMap.erase(activity);
}