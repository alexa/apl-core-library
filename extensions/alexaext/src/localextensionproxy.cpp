/**
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

#include "alexaext/localextensionproxy.h"

#include <cstring>

namespace alexaext {

LocalExtensionProxy::LocalExtensionProxy(const ExtensionPtr& extension)
    : mExtension(extension), mFactory(nullptr), mURIs(extension->getURIs())
{}

LocalExtensionProxy::LocalExtensionProxy(const std::string& uri, ExtensionFactory factory)
    : mExtension(nullptr), mFactory(std::move(factory))
{
    mURIs.emplace(uri);
}

/**
* Proxy constructor for an extension using deferred creation. The extension supports
* multiple URI.
*
* @param factory Factory to create the extension.
*/
LocalExtensionProxy::LocalExtensionProxy(std::set<std::string> uri, ExtensionFactory factory)
    : mExtension(nullptr), mFactory(std::move(factory)), mURIs(std::move(uri))
{}

std::set<std::string> LocalExtensionProxy::getURIs() const
{
    return mURIs;
}

bool
LocalExtensionProxy::getRegistrationInternal(const std::string& uri,
                                             const rapidjson::Value& registrationRequest,
                                             RegistrationSuccessCallback&& success,
                                             RegistrationFailureCallback&& error,
                                             ProcessRegistrationCallback&& processRegistration)
{
    int errorCode = kErrorNone;
    std::string errorMsg;

    // check the URI is supported
    if (mExtension == nullptr || mURIs.find(uri) == mURIs.end()) {
        if (error) {
            rapidjson::Document fail = RegistrationFailure("1.0")
                    .uri(uri)
                    .errorCode(kErrorUnknownURI)
                    .errorMessage(sErrorMessage[kErrorUnknownURI] + uri);
            error(uri, fail);
        }
        return false;
    }

    // request the schema from the extension
    auto registration = processRegistration(registrationRequest);

    // failed schema creation notify failure callback
    if (registration.IsNull() || registration.HasParseError()) {
        if (errorCode == kErrorNone) {
            //  the call was attempted but the extension failed without exception or failure message
            errorCode = kErrorInvalidExtensionSchema;
            errorMsg = sErrorMessage[kErrorInvalidExtensionSchema] + uri;
        }
        if (error) {
            rapidjson::Document fail = RegistrationFailure("1.0")
                    .errorCode(errorCode)
                    .errorMessage(errorMsg)
                    .uri(uri);
            error(uri, fail);
        }
        return false; // registration message failed execution and was not handled by the extension
    }

    // non-success messages are assumed to be failure and forwarded to the error handler
    auto method = RegistrationSuccess::METHOD().Get(registration);
    if (method &&  *method != "RegisterSuccess") {
        if (error) {
            error(uri, registration);
        }
        return true; // registration message was handled handled by the extension
    }

    // notify success callback
    if (success) {
        success(uri, registration);
    }
    return true;
}

bool
LocalExtensionProxy::getRegistration(const std::string& uri,
                                     const rapidjson::Value& registrationRequest,
                                     ExtensionProxy::RegistrationSuccessCallback success,
                                     ExtensionProxy::RegistrationFailureCallback error) {
    return getRegistrationInternal(uri,
                                   registrationRequest,
                                   std::move(success),
                                   std::move(error),
                                   [&](const rapidjson::Value &registrationRequest) {
                                       return mExtension->createRegistration(uri, registrationRequest);
                                   });
}

bool
LocalExtensionProxy::getRegistration(const ActivityDescriptor& activity,
                                     const rapidjson::Value& registrationRequest,
                                     RegistrationSuccessActivityCallback&& success,
                                     RegistrationFailureActivityCallback&& error)
{
    return getRegistrationInternal(activity.getURI(),
                                   registrationRequest,
                                   [activity, success](const std::string& uri, const rapidjson::Value &registrationSuccess) {
                                       success(activity, registrationSuccess);
                                   },
                                   [activity, error](const std::string& uri, const rapidjson::Value &registrationFailure) {
                                       error(activity, registrationFailure);
                                   },
                                   [&](const rapidjson::Value &registrationRequest) {
                                        return mExtension->createRegistration(activity, registrationRequest);
                                   });
}

bool
LocalExtensionProxy::initializeExtension(const std::string& uri)
{
    if (!mExtension && mFactory) {
        // create the extension
        mExtension = mFactory(uri);
    }

    if (!mExtension || mInitialized.count(uri)) return false;

    std::weak_ptr<LocalExtensionProxy> weakSelf = shared_from_this();
    mExtension->registerEventCallback(
            [weakSelf](const alexaext::ActivityDescriptor& activity, const rapidjson::Value &event) {
                if (auto self = weakSelf.lock()) {
                    auto it = self->mEventActivityCallbacks.find(activity);
                    if (it != self->mEventActivityCallbacks.end()) {
                        for (const auto& callback : *it->second) {
                            callback(activity, event);
                        }
                    } else {
                        // Fall back to legacy callbacks, but only if we don't have activity
                        // callbacks. Otherwise, we could end up double reporting events.
                        for (const auto& callback : self->mEventCallbacks) {
                            callback(activity.getURI(), event);
                        }
                    }
                }
            });
    // For backwards compatibility
    mExtension->registerEventCallback(
        [weakSelf](const std::string& uri, const rapidjson::Value &event) {
          if (auto self = weakSelf.lock()) {
              for (const auto &callback : self->mEventCallbacks) {
                  callback(uri, event);
              }
          }
        });

    mExtension->registerLiveDataUpdateCallback(
            [weakSelf](const alexaext::ActivityDescriptor& activity, const rapidjson::Value &liveDataUpdate) {
                if (auto self = weakSelf.lock()) {
                    auto it = self->mLiveDataActivityCallbacks.find(activity);
                    if (it != self->mLiveDataActivityCallbacks.end()) {
                        for (const auto& callback : *it->second) {
                            callback(activity, liveDataUpdate);
                        }
                    } else {
                        // Fall back to legacy callbacks, but only if we don't have activity
                        // callbacks. Otherwise, we could end up double reporting events.
                        for (const auto& callback : self->mLiveDataCallbacks) {
                            callback(activity.getURI(), liveDataUpdate);
                        }
                    }
                }
            });
    mExtension->registerLiveDataUpdateCallback(
        [weakSelf](const std::string& uri, const rapidjson::Value &liveDataUpdate) {
          if (auto self = weakSelf.lock()) {
              for (const auto &callback : self->mLiveDataCallbacks) {
                  callback(uri, liveDataUpdate);
              }
          }
        });

    mInitialized.emplace(uri);

    return true;
}

bool
LocalExtensionProxy::isInitialized(const std::string& uri) const
{
    return mURIs.count(uri) && mExtension && mInitialized.count(uri);
}

bool
LocalExtensionProxy::invokeCommandInternal(const std::string& uri,
                                           const rapidjson::Value& command,
                                           CommandSuccessCallback&& success,
                                           CommandFailureCallback&& error,
                                           ProcessCommandCallback&& processCommand)
{
    // verify the command has an ID
    const rapidjson::Value* commandValue = Command::ID().Get(command);
    if (!commandValue || !commandValue->IsNumber()) {
        if (error) {
            rapidjson::Document fail = CommandFailure("1.0")
                    .uri(uri)
                    .errorCode(kErrorInvalidMessage)
                    .errorMessage(sErrorMessage[kErrorInvalidMessage]);
            error(uri, fail);
        }
        return false;
    }
    int commandID = (int) commandValue->GetDouble();

    // check the URI is supported and the command has ID
    if (mExtension == nullptr || mURIs.find(uri) == mURIs.end()) {
        if (error) {
            rapidjson::Document fail = CommandFailure("1.0")
                    .uri(uri)
                    .id(commandID)
                    .errorCode(kErrorUnknownURI)
                    .errorMessage(sErrorMessage[kErrorUnknownURI] + uri);
            error(uri, fail);
        }
        return false;
    }

    // invoke the extension command
    int errorCode = kErrorNone;
    std::string errorMsg;
    auto result = processCommand(command);

    // failed command invocation
    if (!result) {
        if (error) {
            if (errorCode == kErrorNone) {
                //  the call was attempted but the extension failed without exception
                errorCode = kErrorFailedCommand;
                errorMsg = sErrorMessage[kErrorFailedCommand] + std::to_string(commandID);
            }
            rapidjson::Document fail = CommandFailure("1.0")
                    .uri(uri)
                    .id(commandID)
                    .errorCode(errorCode)
                    .errorMessage(errorMsg);
            error(uri, fail);
        }
        return false;
    }

    // notify success callback
    if (success) {
        rapidjson::Document win = CommandSuccess("1.0")
                .uri(uri)
                .id(commandID);
        success(uri, win);
    }
    return true;
}

bool
LocalExtensionProxy::invokeCommand(const std::string& uri, const rapidjson::Value& command,
                                   ExtensionProxy::CommandSuccessCallback success,
                                   ExtensionProxy::CommandFailureCallback error) {
    return invokeCommandInternal(uri,
                                 command,
                                 std::move(success),
                                 std::move(error),
                                 [&](const rapidjson::Value& command) {
                                     return mExtension->invokeCommand(uri, command);
                                 });
}

bool
LocalExtensionProxy::invokeCommand(const ActivityDescriptor& activity,
                                   const rapidjson::Value& command,
                                   CommandSuccessActivityCallback&& success,
                                   CommandFailureActivityCallback&& error)
{
    return invokeCommandInternal(activity.getURI(),
                                 command,
                                 [activity, success](const std::string& uri, const rapidjson::Value &commandSuccess) {
                                     success(activity, commandSuccess);
                                 },
                                 [activity, error](const std::string& uri, const rapidjson::Value &commandFailure) {
                                     error(activity, commandFailure);
                                 },
                                 [&](const rapidjson::Value& command) {
                                    return mExtension->invokeCommand(activity, command);
                                 });
}

void
LocalExtensionProxy::registerEventCallback(Extension::EventCallback callback)
{
    if (callback) mEventCallbacks.emplace_back(std::move(callback));
}

void
LocalExtensionProxy::registerLiveDataUpdateCallback(Extension::LiveDataUpdateCallback callback)
{
    if (callback) mLiveDataCallbacks.emplace_back(std::move(callback));
}

void
LocalExtensionProxy::registerEventCallback(const ActivityDescriptor& activity, Extension::EventActivityCallback&& callback)
{
    if (!callback) return;

    auto it = mEventActivityCallbacks.find(activity);
    if (it != mEventActivityCallbacks.end()) {
        it->second->emplace_back(callback);
    } else {
        auto callbacks = std::make_shared<std::vector<Extension::LiveDataUpdateActivityCallback>>();
        callbacks->emplace_back(callback);
        mEventActivityCallbacks.emplace(activity, callbacks);
    }
}

void
LocalExtensionProxy::registerLiveDataUpdateCallback(const ActivityDescriptor& activity, Extension::LiveDataUpdateActivityCallback&& callback)
{
    if (!callback) return;

    auto it = mLiveDataActivityCallbacks.find(activity);
    if (it != mLiveDataActivityCallbacks.end()) {
        it->second->emplace_back(callback);
    } else {
        auto callbacks = std::make_shared<std::vector<Extension::LiveDataUpdateActivityCallback>>();
        callbacks->emplace_back(callback);
        mLiveDataActivityCallbacks.emplace(activity, callbacks);
    }
}

void
LocalExtensionProxy::onRegistered(const std::string& uri, const std::string& token)
{
    if (mExtension) mExtension->onRegistered(uri, token);
}

void
LocalExtensionProxy::onRegistered(const ActivityDescriptor& activity)
{
    if (mExtension) mExtension->onActivityRegistered(activity);
}

void
LocalExtensionProxy::onUnregistered(const std::string& uri, const std::string& token)
{
    if (mExtension) mExtension->onUnregistered(uri, token);
}

void
LocalExtensionProxy::onUnregistered(const ActivityDescriptor& activity)
{
    if (mExtension) {
        mExtension->onActivityUnregistered(activity);
    }

    mEventActivityCallbacks.erase(activity);
    mLiveDataActivityCallbacks.erase(activity);
}

void
LocalExtensionProxy::onSessionStarted(const SessionDescriptor& session) {
    if (mExtension) mExtension->onSessionStarted(session);
}

void
LocalExtensionProxy::onSessionEnded(const SessionDescriptor& session) {
    if (mExtension) mExtension->onSessionEnded(session);
}

bool
LocalExtensionProxy::sendComponentMessage(const std::string& uri, const rapidjson::Value& message) {
    if (!mExtension) return false;

    const auto* method = GetWithDefault("method", message, "");
    if (std::strcmp(method, "Component") == 0) {
        return mExtension->updateComponent(uri, message);
    }

    return false;
}

bool
LocalExtensionProxy::sendComponentMessage(const ActivityDescriptor& activity,
                                 const rapidjson::Value& message) {
    if (!mExtension) return false;

    const auto* method = GetWithDefault("method", message, "");
    if (std::strcmp(method, "Component") == 0) {
        return mExtension->updateComponent(activity, message);
    }

    return false;
}

void
LocalExtensionProxy::onResourceReady(const std::string& uri,
                                     const ResourceHolderPtr& resourceHolder) {
    if (mExtension)
        mExtension->onResourceReady(uri, resourceHolder);
}

void
LocalExtensionProxy::onResourceReady(const ActivityDescriptor& activity,
                                     const ResourceHolderPtr& resourceHolder) {
    if (mExtension)
        mExtension->onResourceReady(activity, resourceHolder);
}

void
LocalExtensionProxy::onForeground(const ActivityDescriptor& activity) {
    if (mExtension)
        mExtension->onForeground(activity);
}

void
LocalExtensionProxy::onBackground(const ActivityDescriptor& activity) {
    if (mExtension)
        mExtension->onBackground(activity);
}

void
LocalExtensionProxy::onHidden(const ActivityDescriptor& activity) {
    if (mExtension)
        mExtension->onHidden(activity);
}

} // namespace alexaext
