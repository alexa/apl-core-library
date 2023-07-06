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
#ifndef APL_APLMUSICALARMEXTENSION_H
#define APL_APLMUSICALARMEXTENSION_H

#include <alexaext/alexaext.h>
#include <alexaext/extensionuuid.h>

#include <rapidjson/document.h>

#include "AplMusicAlarmExtensionObserverInterface.h"

namespace alexaext {
namespace musicalarm {

static const std::string URI = "aplext:musicalarm:10";

/**
 * The MusicAlarm extension is an optional-use feature, which allows APL developers to dismiss/snooze
 * the ringing music alarm from within the APL document.
 */
class AplMusicAlarmExtension :
    public alexaext::ExtensionBase,
    public std::enable_shared_from_this<AplMusicAlarmExtension> {

public:
    AplMusicAlarmExtension(
        AplMusicAlarmExtensionObserverInterfacePtr observer,
        alexaext::ExecutorPtr executor,
        alexaext::uuid::UUIDFunction uuidGenerator = alexaext::uuid::generateUUIDV4);

    ~AplMusicAlarmExtension() override = default;

    /// @name alexaext::Extension Functions
    /// @{

    rapidjson::Document createRegistration(const std::string &uri,
                                           const rapidjson::Value &registrationRequest) override;

    bool invokeCommand(const std::string &uri, const rapidjson::Value &command) override;

    /// @}

private:
    AplMusicAlarmExtensionObserverInterfacePtr mObserver;
    std::weak_ptr<alexaext::Executor> mExecutor;
    alexaext::uuid::UUIDFunction mUuidGenerator;
};

}  // namespace musicalarm
}  // namespace alexaext

#endif // APL_APLMUSICALARMEXTENSION_H
