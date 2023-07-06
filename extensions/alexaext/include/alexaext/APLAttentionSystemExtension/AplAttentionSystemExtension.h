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
#ifndef APL_APLATTENTIONSYSTEMEXTENSION_H
#define APL_APLATTENTIONSYSTEMEXTENSION_H

#include <mutex>

#include <alexaext/alexaext.h>

#include <rapidjson/document.h>

namespace alexaext {
namespace attention {

static const std::string URI = "aplext:attentionsystem:10";
static const std::string ENVIRONMENT_VERSION = "APLAttentionSystemExtension-1.0";

enum AttentionState {
    IDLE,
    LISTENING,
    THINKING,
    SPEAKING
};

class AplAttentionSystemExtension
    : public alexaext::ExtensionBase,
      public std::enable_shared_from_this<AplAttentionSystemExtension> {

public:
    AplAttentionSystemExtension(
        alexaext::ExecutorPtr executor,
        alexaext::uuid::UUIDFunction uuidGenerator = alexaext::uuid::generateUUIDV4);

    ~AplAttentionSystemExtension() override = default;

    /// @name alexaext::Extension Functions
    /// @{

    rapidjson::Document createRegistration(const ActivityDescriptor& activity,
                                           const rapidjson::Value &registrationRequest) override;

    void onActivityUnregistered(const ActivityDescriptor& activity) override;
    /// @}

    // Updates the livedata map and sends the event. Should be called on every state change.
    virtual void updateAttentionState(const AttentionState& newState);

    void applySettings(const ActivityDescriptor& activity, const rapidjson::Value &settings);

    // Publishes a LiveDataUpdate
    void publishLiveData(const ActivityDescriptor& activity);

private:
    static std::string attentionStateToString(const AttentionState& state) {
        switch (state) {
            case AttentionState::LISTENING:
                return "LISTENING";
            case AttentionState::THINKING:
                return "THINKING";
            case AttentionState::SPEAKING:
                return "SPEAKING";
            case AttentionState::IDLE:
            default:
                return "IDLE";
        }
    };

    std::weak_ptr<alexaext::Executor> mExecutor;
    alexaext::uuid::UUIDFunction mUuidGenerator;
    AttentionState mAttentionState = AttentionState::IDLE;
    std::recursive_mutex mAttentionStateNameMutex;
    std::map<ActivityDescriptor, std::string, ActivityDescriptor::Compare> mAttentionStateNameMap;
};

using AplAttentionSystemExtensionPtr = std::shared_ptr<AplAttentionSystemExtension>;

} // namespace attention
} // namespace alexaext

#endif // APL_APLATTENTIONSYSTEMEXTENSION_H
