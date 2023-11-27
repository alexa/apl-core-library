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
#include "alexaext/APLAudioNormalizationExtension/AplAudioNormalizationExtension.h"

#include <algorithm>

namespace alexaext {
namespace audionormalization {

constexpr auto COMMAND_ENABLE = "Enable";
constexpr auto COMMAND_DISABLE = "Disable";

std::shared_ptr<AplAudioNormalizationExtension>
AplAudioNormalizationExtension::getInstance()
{
    static std::shared_ptr<AplAudioNormalizationExtension> ptr(new AplAudioNormalizationExtension());
    return ptr;
}

rapidjson::Document
AplAudioNormalizationExtension::createRegistration(const ActivityDescriptor &activity,
                                                const rapidjson::Value &registrationRequest)
{
    return RegistrationSuccess(DEFAULT_SCHEMA_VERSION)
       .uri(URI)
       .token("<AUTO_TOKEN>")
       .schema(DEFAULT_SCHEMA_VERSION, [=](ExtensionSchema& schema) {
           schema.uri(URI);
           schema.command(COMMAND_ENABLE, [](CommandSchema &command) {
               command.allowFastMode(true);
           });
           schema.command(COMMAND_DISABLE, [](CommandSchema &command) {
               command.allowFastMode(true);
           });
       });
}

bool
AplAudioNormalizationExtension::invokeCommand(const ActivityDescriptor &activity,
                                           const rapidjson::Value &command)
{
   const std::string &name = GetWithDefault(Command::NAME(), command, "");
   if (name == COMMAND_ENABLE) {
       notifyListeners([&](Listener& listener) {
           listener.onAudioNormalizationEnabled(activity);
       });
       return true;
   } else if (name == COMMAND_DISABLE) {
       notifyListeners([&](Listener& listener) {
           listener.onAudioNormalizationDisabled(activity);
       });
       return true;
   }

   return false;
}

void
AplAudioNormalizationExtension::notifyListeners(const std::function<void(Listener&)>& func)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    for (const auto& weakListener : mListeners) {
        if (auto listener = weakListener.lock()) {
            func(*listener);
        }
    }

    cleanUp();
}

void
AplAudioNormalizationExtension::onSessionEnded(const alexaext::SessionDescriptor& session)
{
    cleanUp();
}

void
AplAudioNormalizationExtension::registerListener(const std::shared_ptr<Listener> &listener)
{
   std::lock_guard<std::recursive_mutex> lock(mMutex);
   if (listener != nullptr) {
       mListeners.push_back(listener);
   }
}

void
AplAudioNormalizationExtension::unregisterListener(const std::shared_ptr<Listener> &listener)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    // Remove listeners that are expired or the listener to be removed
    mListeners.erase(std::remove_if(mListeners.begin(), mListeners.end(),
        [=](const std::weak_ptr<Listener>& it) {
            return it.expired() || it.lock() == listener;
        }), mListeners.end());
}

void
AplAudioNormalizationExtension::cleanUp()
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    // Clean up any expired listeners
    mListeners.erase(std::remove_if(mListeners.begin(), mListeners.end(),
                                    [](const std::weak_ptr<Listener>& it) {
                                        return it.expired();
                                    }), mListeners.end());
}

} // namespace audionormalization
} // namespace alexaext