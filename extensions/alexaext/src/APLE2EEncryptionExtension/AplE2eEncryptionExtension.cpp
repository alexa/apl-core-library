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

#include "alexaext/APLE2EEncryptionExtension/AplE2eEncryptionExtension.h"

using namespace E2EEncryption;
using namespace alexaext;

// version of the extension definition Schema
static const std::string SCHEMA_VERSION = "1.0";

// Available algorithms
static const char* E2EENCRYPTION_AES = "AES/GCM/NoPadding";
static const char* E2EENCRYPTION_RSA = "RSA/ECB/OAEPWithSHA1AndMGF1Padding";

// Available commands
static const char* BASE64_ENCRYPT_VALUE = "Base64EncryptValue";
static const char* BASE64_ENCODE_VALUE = "Base64EncodeValue";

// Command Response Types
static const char* ENCRYPTION_PAYLOAD = "EncryptionPayload";
static const char* ENCODING_PAYLOAD = "EncodingPayload";

// Types Properties and Constants
static const char* TOKEN_PROPERTY = "token";
static const char* VALUE_PROPERTY = "value";
static const char* ALGORITHM_PROPERTY = "algorithm";
static const char* KEY_PROPERTY = "key";
static const char* AAD_PROPERTY = "aad";
static const char* BASE64_ENCODED_PROPERTY = "base64Encoded";
static const char* BASE64_ENCODED_DATA_PROPERTY = "base64EncodedData";
static const char* ERROR_REASON_PROPERTY = "errorReason";
static const char* BASE64_ENCRYPTED_DATA_PROPERTY = "base64EncryptedData";
static const char* BASE64_ENCODED_IV_PROPERTY = "base64EncodedIV";
static const char* BASE64_ENCODED_KEY_PROPERTY = "base64EncodedKey";

static const char* STRING_TYPE = "string";
static const char* BOOLEAN_TYPE = "boolean";

// Events
static const char* ON_ENCRYPT_SUCCESS = "OnEncryptSuccess";
static const char* ON_ENCRYPT_FAILURE = "OnEncryptFailure";
static const char* ON_BASE64_ENCODE_SUCCESS = "OnBase64EncodeSuccess";

AplE2eEncryptionExtension::AplE2eEncryptionExtension(
    AplE2eEncryptionExtensionObserverInterfacePtr observer,
    alexaext::ExecutorPtr executor,
    alexaext::uuid::UUIDFunction uuidGenerator)
    : alexaext::ExtensionBase(URI),
      mObserver(std::move(observer)),
      mExecutor(executor),
      mUuidGenerator(std::move(uuidGenerator)) {}

rapidjson::Document
AplE2eEncryptionExtension::createRegistration(const std::string& uri,
                                              const rapidjson::Value& registrationRequest)
{
    if (uri != URI)
        return RegistrationFailure::forUnknownURI(uri);

    return RegistrationSuccess(SCHEMA_VERSION)
        .uri(URI)
        .token(mUuidGenerator())
        .environment([](Environment& environment) {
            environment.version(ENVIRONMENT_VERSION);
            environment.property(E2EENCRYPTION_AES, true);
            environment.property(E2EENCRYPTION_RSA, true);
        })
        .schema(SCHEMA_VERSION, [&](ExtensionSchema& schema) {
            schema.uri(URI)
                .command(BASE64_ENCRYPT_VALUE,
                         [](CommandSchema& commandSchema) {
                             commandSchema.dataType(ENCRYPTION_PAYLOAD);
                         })
                .command(BASE64_ENCODE_VALUE,
                         [](CommandSchema& commandSchema) {
                             commandSchema.dataType(ENCODING_PAYLOAD);
                         })
                .event(ON_ENCRYPT_SUCCESS)
                .event(ON_ENCRYPT_FAILURE)
                .event(ON_BASE64_ENCODE_SUCCESS)
                .dataType(ENCRYPTION_PAYLOAD,
                          [](TypeSchema& typeSchema) {
                              typeSchema
                                  .property(TOKEN_PROPERTY,
                                            [](TypePropertySchema& typePropertySchema) {
                                                typePropertySchema.type(STRING_TYPE);
                                                typePropertySchema.required(true);
                                            })
                                  .property(VALUE_PROPERTY, STRING_TYPE)
                                  .property(ALGORITHM_PROPERTY, STRING_TYPE)
                                  .property(KEY_PROPERTY,
                                            [](TypePropertySchema& typePropertySchema) {
                                                typePropertySchema.type(STRING_TYPE);
                                                typePropertySchema.required(false);
                                            })
                                  .property(AAD_PROPERTY,
                                            [](TypePropertySchema& typePropertySchema) {
                                                typePropertySchema.type(STRING_TYPE);
                                                typePropertySchema.required(false);
                                            })
                                  .property(BASE64_ENCODED_PROPERTY,
                                            [](TypePropertySchema& typePropertySchema) {
                                                typePropertySchema.type(BOOLEAN_TYPE);
                                                typePropertySchema.required(false);
                                            });
                          })
                .dataType(ENCODING_PAYLOAD, [](TypeSchema& typeSchema) {
                    typeSchema
                        .property(TOKEN_PROPERTY,
                                  [](TypePropertySchema& typePropertySchema) {
                                      typePropertySchema.type(STRING_TYPE);
                                      typePropertySchema.required(true);
                                  })
                        .property(VALUE_PROPERTY, STRING_TYPE);
                });
        });
}

bool
AplE2eEncryptionExtension::invokeCommand(const std::string& uri, const rapidjson::Value& command)
{
    if (uri != URI)
        return false;

    if (!mObserver)
        return false;

    auto executor = mExecutor.lock();
    if (!executor)
        return false;

    const std::string& name = GetWithDefault(Command::NAME(), command, "");
    if (BASE64_ENCRYPT_VALUE == name) {
        const rapidjson::Value* params = Command::PAYLOAD().Get(command);
        if (!params || !params->HasMember(VALUE_PROPERTY) || !params->HasMember(ALGORITHM_PROPERTY))
            return false;
        auto token = GetWithDefault(TOKEN_PROPERTY, params, "");
        auto value = GetWithDefault(VALUE_PROPERTY, params, "");
        auto key = GetWithDefault(KEY_PROPERTY, params, "");
        auto algorithm = GetWithDefault(ALGORITHM_PROPERTY, params, "");
        auto aad = GetWithDefault(AAD_PROPERTY, params, "");
        auto base64Encoded = GetWithDefault(BASE64_ENCODED_PROPERTY, params, false);
        auto thisWeakPointer = std::weak_ptr<AplE2eEncryptionExtension>(shared_from_this());
        auto successCallback = [thisWeakPointer](const std::string& token,
                                                 const std::string& base64EncryptedData,
                                                 const std::string& base64EncodedIV,
                                                 const std::string& base64EncodedKey) {
            auto ptr = thisWeakPointer.lock();
            if (!ptr)
                return;
            auto event = Event(SCHEMA_VERSION)
                             .uri(URI)
                             .target(URI)
                             .name(ON_ENCRYPT_SUCCESS)
                             .property(TOKEN_PROPERTY, token);
            if (!base64EncryptedData.empty())
                event.property(BASE64_ENCRYPTED_DATA_PROPERTY, base64EncryptedData);
            if (!base64EncodedIV.empty())
                event.property(BASE64_ENCODED_IV_PROPERTY, base64EncodedIV);
            if (!base64EncodedKey.empty())
                event.property(BASE64_ENCODED_KEY_PROPERTY, base64EncodedKey);

            ptr->invokeExtensionEventHandler(URI, event);
        };
        auto errorCallback = [thisWeakPointer](const std::string& token,
                                               const std::string& reason) {
            auto ptr = thisWeakPointer.lock();
            if (!ptr)
                return;
            auto event = Event(SCHEMA_VERSION)
                             .uri(URI)
                             .target(URI)
                             .name(ON_ENCRYPT_FAILURE)
                             .property(TOKEN_PROPERTY, token)
                             .property(ERROR_REASON_PROPERTY, reason);
            ptr->invokeExtensionEventHandler(URI, event);
        };
        executor->enqueueTask([&](){
            mObserver->onBase64EncryptValue(token, key, algorithm, aad, value, base64Encoded,
                                            successCallback, errorCallback);
        });
        return true;
    }

    if (BASE64_ENCODE_VALUE == name) {
        const rapidjson::Value* params = Command::PAYLOAD().Get(command);
        if (!params || !params->HasMember(VALUE_PROPERTY))
            return false;
        auto token = GetWithDefault(TOKEN_PROPERTY, params, "");
        auto value = GetWithDefault(VALUE_PROPERTY, params, "");
        auto thisWeakPointer = std::weak_ptr<AplE2eEncryptionExtension>(shared_from_this());
        auto successCallback = [thisWeakPointer](const std::string& token,
                                                 const std::string& base64EncodedData) {
            auto ptr = thisWeakPointer.lock();
            if (!ptr)
                return;
            auto event = Event(SCHEMA_VERSION)
                             .uri(URI)
                             .target(URI)
                             .name(ON_BASE64_ENCODE_SUCCESS)
                             .property(TOKEN_PROPERTY, token)
                             .property(BASE64_ENCODED_DATA_PROPERTY, base64EncodedData);
            ptr->invokeExtensionEventHandler(URI, event);
        };
        executor->enqueueTask([&](){
            return mObserver->onBase64EncodeValue(token, value, successCallback);
        });
        return true;
    }

    return false;
}