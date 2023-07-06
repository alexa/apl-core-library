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

#include <algorithm>
#include <list>
#include <string>
#include <utility>

#include <rapidjson/document.h>

#include "alexaext/APLWebflowExtension/AplWebflowExtension.h"

using namespace alexaext::webflow;

// Data types
static const char* PAYLOAD_START_FLOW = "StartFlowPayload";

// Commands
static const char* COMMAND_START_FLOW = "StartFlow";

// Events
static const char* EVENT_ON_FLOW_END = "OnFlowEnd";

// Properties
static const char* PROPERTY_FLOW_ID = "flowId";
static const char* PROPERTY_URL = "url";
static const char* PROPERTY_TOKEN = "token";

// Constants
static const char* SCHEMA_VERSION = "1.0";

AplWebflowExtension::AplWebflowExtension(
    std::function<std::string(void)> tokenGenerator,
    std::shared_ptr<AplWebflowExtensionObserverInterface> observer,
    const std::shared_ptr<alexaext::Executor>& executor)
    : alexaext::ExtensionBase(URI),
      mObserver(std::move(observer)),
      mTokenGenerator(std::move(tokenGenerator)),
      mExecutor(executor) {}

rapidjson::Document
AplWebflowExtension::createRegistration(const ActivityDescriptor &activity,
                                        const rapidjson::Value& registrationRequest)
{
    if (activity.getURI() != URI) {
        return RegistrationFailure::forUnknownURI(activity.getURI());
    }

    // return success with the schema and environment
    return RegistrationSuccess(SCHEMA_VERSION)
        .uri(URI)
        .token(mTokenGenerator())
        .schema(SCHEMA_VERSION, [&](ExtensionSchema& schema) {
            schema
                .uri(URI)
                .dataType(PAYLOAD_START_FLOW,
                          [](TypeSchema& typeSchema) {
                              typeSchema
                                  .property(PROPERTY_FLOW_ID,
                                            [](TypePropertySchema& propertySchema) {
                                                propertySchema.type("string").required(false);
                                            })
                                  .property(PROPERTY_URL, [](TypePropertySchema& propertySchema) {
                                      propertySchema.type("string").required(true);
                                  });
                          })
                .command(COMMAND_START_FLOW,
                         [](CommandSchema& commandSchema) {
                             commandSchema.allowFastMode(true).dataType(PAYLOAD_START_FLOW);
                         })
                .event(EVENT_ON_FLOW_END);
        });
}

bool
AplWebflowExtension::invokeCommand(const ActivityDescriptor& activity, const rapidjson::Value& command)
{
    // unknown URI
    if (activity.getURI() != URI)
        return false;

    if (!mObserver)
        return false;

    auto executor = mExecutor.lock();
    if (!executor)
        return false;

    const std::string &name = GetWithDefault(Command::NAME(), command, "");

    if (COMMAND_START_FLOW == name) {
        const rapidjson::Value *params = Command::PAYLOAD().Get(command);
        if (!params || !params->HasMember(PROPERTY_URL))
            return false;

        std::string url = GetWithDefault<const char *>(PROPERTY_URL, *params, "");
        if (url.empty())
            return false;

        std::string token = GetWithDefault<const char *>(PROPERTY_TOKEN, *params, "");
        std::string flowId = GetWithDefault<const char *>(PROPERTY_FLOW_ID, *params, "");

        if (flowId.empty()) {
            executor->enqueueTask([&]() { mObserver->onStartFlow(activity, token, url, flowId); });
        }
        else {
            std::weak_ptr<AplWebflowExtension> thisWeak = shared_from_this();
            auto onFlowEnd = [thisWeak](const std::string& token, const std::string& flowId){
                auto lockPtr = thisWeak.lock();
                if (lockPtr) {
                    auto event = Event(SCHEMA_VERSION)
                        .uri(URI)
                        .target(URI)
                        .name(EVENT_ON_FLOW_END)
                        .property(PROPERTY_TOKEN, token)
                        .property(PROPERTY_FLOW_ID, flowId);
                    lockPtr->invokeExtensionEventHandler(URI, event);
                }
            };
            executor->enqueueTask([&]() { mObserver->onStartFlow(activity, token, url, flowId, onFlowEnd); });
        }
        return true;
    }

    return false;
}