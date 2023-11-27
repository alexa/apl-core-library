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
#ifndef APL_APLAUDIONORMALIZATIONEXTENSION_H
#define APL_APLAUDIONORMALIZATIONEXTENSION_H

#include <memory>
#include <mutex>
#include <vector>

#include "alexaext/extensionbase.h"

namespace alexaext {
namespace audionormalization {

/**
 * This class listens for audio normalization changed events.
 */
class Listener {
public:
    virtual ~Listener() = default;

    /**
     * Notify audio normalization has been enabled.
     * @param activity the activity for which audio normalization has been enabled.
     */
    virtual void onAudioNormalizationEnabled(const ActivityDescriptor& activity) = 0;

    /**
     * Notify audio normalization has been disabled.
     * @param activity the activity for which audio normalization has been disabled.
     */
    virtual void onAudioNormalizationDisabled(const ActivityDescriptor& activity) = 0;
};

/**
 * Implementation of Audio Normalization extension.
 */
class AplAudioNormalizationExtension : public ExtensionBase {
public:
    static constexpr auto URI = "aplext:audionormalization:10";

    /**
     * @return the singleton instance
     */
    static std::shared_ptr<AplAudioNormalizationExtension> getInstance();

    AplAudioNormalizationExtension(AplAudioNormalizationExtension &&) = delete;
    AplAudioNormalizationExtension(AplAudioNormalizationExtension const&) = delete;
    AplAudioNormalizationExtension& operator=(AplAudioNormalizationExtension &&) = delete;
    AplAudioNormalizationExtension& operator=(AplAudioNormalizationExtension const&) = delete;

    /// ExtensionBase
    rapidjson::Document
    createRegistration(const ActivityDescriptor &activity,
                       const rapidjson::Value &registrationRequest) final;
    bool invokeCommand(const ActivityDescriptor &activity,
                       const rapidjson::Value &command) final;
    void onSessionEnded(const SessionDescriptor& session) final;

    /**
     * Registers a listener to receive audio normalization commands. Multiple listeners may be
     * registered to receive audio normalization commands.
     *
     * Listeners will be removed if they are no longer strongly referenced when sessions are ended.
     * @param listener the listener to add.
     */
    void registerListener(const std::shared_ptr<Listener>& listener);

    /**
     * Unregisters a listener from receiving audio normalization commands.
     * @param listener the listener to remove.
     */
    void unregisterListener(const std::shared_ptr<Listener>& listener);

private:
    AplAudioNormalizationExtension() : ExtensionBase(URI) {}

    void notifyListeners(const std::function<void(Listener& listener)>& func);

    /**
     * Remove any expired listeners that never unregistered. Should be run occasionally.
     */
    void cleanUp();

private:
    std::recursive_mutex mMutex;
    std::vector<std::weak_ptr<Listener>> mListeners;
};

} // namespace audionormalization
} // namespace alexaext

#endif // APL_APLAUDIONORMALIZATIONEXTENSION_H
