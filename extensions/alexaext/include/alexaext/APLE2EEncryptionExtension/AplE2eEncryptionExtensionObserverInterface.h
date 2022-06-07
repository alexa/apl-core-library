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
#ifndef APL_APLE2EENCRYPTIONEXTENSIONOBSERVERINTERFACE_H
#define APL_APLE2EENCRYPTIONEXTENSIONOBSERVERINTERFACE_H

#include <memory>
#include <string>

namespace E2EEncryption {

/**
 * Callback to run when the encryption of a value finishes successfully
 *
 * @param token with the token to identify the caller
 * @param base64EncryptedData result of the encoding process
 * @param base64EncodedIV The base64 encoded value of the initialization vector if used
 * by the given algorithm
 * @param base64EncodedKey The base64 encoded value of the key used for the encryption.
 */
using EncryptionCallbackSuccess =
    std::function<void(const std::string& token,
                       const std::string& base64EncryptedData,
                       const std::string& base64EncodedIV,
                       const std::string& base64EncodeKey)>;

/**
 * Callback to run when the encryption of a value finishes successfully
 *
 * @param token with the token to identify the caller
 * @param reason reason why the encryption process failed
 */
using EncryptionCallbackError =
    std::function<void(const std::string& token, const std::string& reason)>;


/**
 * Callback to run when the encryption of a value finishes successfully
 *
 * @param token with the token to identify the caller
 * @param base64EncodedData result of the encoding process
 */
using EncodeCallbackSuccess =
    std::function<void(const std::string& token, const std::string& base64EncodedData)>;


/**
 * This class allows a @c AplE2eEncryptionExtensionObserverInterface observer to be notified of
 * changes in the
 * @c AplE2eEncryptionExtension.
 */
class AplE2eEncryptionExtensionObserverInterface {
public:
    /**
     * Destructor
     */
    virtual ~AplE2eEncryptionExtensionObserverInterface() = default;

    /**
     * Used to encrypt the value property.
     *
     * @param token metadata used to identify the caller of the command. This is needed for async
     * purposes.
     * @param key key to use to encrypt value
     * @param algorithm the encryption algorithm used for encryption
     * @param aad additional authentication data used by the encryption algorithm
     * @param value text to encrypt
     * @param base64Encoded when true, the value needs to be base64decode before encryption
     * @param successCallback code to run when the encryption successfully encrypts value
     * @param errorCallback code to run when the encryption can't encrypt value
     */
    virtual void onBase64EncryptValue(const std::string& token,
                                      const std::string& key,
                                      const std::string& algorithm,
                                      const std::string& aad,
                                      const std::string& value,
                                      bool base64Encoded,
                                      EncryptionCallbackSuccess successCallback,
                                      EncryptionCallbackError errorCallback) = 0;

    /**
     * @param token metadata used to identify the caller of the command. This is needed for async
     * purposes.
     * @param value text to encode
     */
    virtual void onBase64EncodeValue(const std::string& token,
                                     const std::string& value,
                                     EncodeCallbackSuccess successCallback) = 0;
};

using AplE2eEncryptionExtensionObserverInterfacePtr =
    std::shared_ptr<AplE2eEncryptionExtensionObserverInterface>;

} // namespace E2EEncryption

#endif // APL_APLE2EENCRYPTIONEXTENSIONOBSERVERINTERFACE_H
