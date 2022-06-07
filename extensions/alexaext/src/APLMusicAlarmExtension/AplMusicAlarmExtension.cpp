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

#include "alexaext/APLMusicAlarmExtension/AplMusicAlarmExtension.h"

using namespace MusicAlarm;
using namespace alexaext;

// Commands
static const std::string COMMAND_DISMISS_NAME = "DismissAlarm";
static const std::string COMMAND_SNOOZE_NAME = "SnoozeAlarm";

// version of the extension definition Schema
static const std::string SCHEMA_VERSION = "1.0";

AplMusicAlarmExtension::AplMusicAlarmExtension(AplMusicAlarmExtensionObserverInterfacePtr observer,
                                               alexaext::ExecutorPtr executor,
                                               alexaext::uuid::UUIDFunction uuidGenerator)
    : alexaext::ExtensionBase(URI),
      mObserver(std::move(observer)),
      mExecutor(executor),
      mUuidGenerator(std::move(uuidGenerator)) {}

rapidjson::Document
AplMusicAlarmExtension::createRegistration(const std::string& uri,
                                           const rapidjson::Value& registrationRequest) {
    if (uri != URI)
        return RegistrationFailure::forUnknownURI(uri);

    return RegistrationSuccess(SCHEMA_VERSION)
        .uri(URI)
        .token(mUuidGenerator())
        .schema(SCHEMA_VERSION, [&](ExtensionSchema& schema) {
            schema
                .uri(URI)
                .command(COMMAND_DISMISS_NAME,
                         [](CommandSchema& commandSchema) { commandSchema.allowFastMode(true); })
                .command(COMMAND_SNOOZE_NAME,
                         [](CommandSchema& commandSchema) { commandSchema.allowFastMode(true); });
        });
}

bool
AplMusicAlarmExtension::invokeCommand(const std::string& uri, const rapidjson::Value& command) {
    if (!mObserver)
        return false;

    auto executor = mExecutor.lock();
    if (!executor)
        return false;

    const std::string& name = GetWithDefault(Command::NAME(), command, "");
    if (COMMAND_DISMISS_NAME == name) {
        executor->enqueueTask([&]() { mObserver->dismissAlarm(); });
        return true;
    }
    if (COMMAND_SNOOZE_NAME == name) {
        executor->enqueueTask([&]() { mObserver->snoozeAlarm(); });
        return true;
    }
    return false;
}