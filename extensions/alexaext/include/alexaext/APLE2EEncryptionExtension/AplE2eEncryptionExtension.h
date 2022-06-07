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
#ifndef APL_APLE2EENCRYPTIONEXTENSION_H
#define APL_APLE2EENCRYPTIONEXTENSION_H

#include <alexaext/alexaext.h>
#include <alexaext/extensionuuid.h>

#include <rapidjson/document.h>

#include "AplE2eEncryptionExtensionObserverInterface.h"

namespace E2EEncryption {

static const std::string URI = "aplext:e2eencryption:10";
static const std::string ENVIRONMENT_VERSION = "APLE2EEncryptionExtension-1.0";

class AplE2eEncryptionExtension
    : public alexaext::ExtensionBase, public std::enable_shared_from_this<AplE2eEncryptionExtension> {

public:

    AplE2eEncryptionExtension(
        AplE2eEncryptionExtensionObserverInterfacePtr observer,
        alexaext::ExecutorPtr executor,
        alexaext::uuid::UUIDFunction uuidGenerator = alexaext::uuid::generateUUIDV4);

    virtual ~AplE2eEncryptionExtension() = default;

    /// @name alexaext::Extension Functions
    /// @{

    rapidjson::Document createRegistration(const std::string &uri,
                                           const rapidjson::Value &registrationRequest) override;

    bool invokeCommand(const std::string &uri, const rapidjson::Value &command) override;

    /// @}

private:

    AplE2eEncryptionExtensionObserverInterfacePtr mObserver;
    std::weak_ptr<alexaext::Executor> mExecutor;
    alexaext::uuid::UUIDFunction mUuidGenerator;
};

using AplE2eEncryptionExtensionPtr = std::shared_ptr<AplE2eEncryptionExtension>;

} // namespace E2EEncryption

#endif // APL_APLE2EENCRYPTIONEXTENSION_H
