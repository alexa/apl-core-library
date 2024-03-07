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

#include "alexaext/threadsafeextensionproxy.h"

namespace alexaext {

ThreadSafeExtensionProxy::ThreadSafeExtensionProxy(const ExtensionPtr &extension,
                                                  const ExecutorPtr &executor) : mExtension(extension), mExecutor(executor) {}

bool
ThreadSafeExtensionProxy::getRegistration(const ActivityDescriptor& activity,
                                         const rapidjson::Value& registrationRequest,
                                         RegistrationSuccessActivityCallback&& success,
                                         RegistrationFailureActivityCallback&& error)
{
    ensureActivityContext(activity);
    auto request = std::make_shared<rapidjson::Document>();
    request->CopyFrom(registrationRequest, request->GetAllocator());
    enqueueTaskOnExtension([activity, request, success, error](const ExtensionPtr& extension) {
        auto registration = extension->createRegistration(activity, *request);
        success(activity, registration);
    });

    return true;
}

bool
ThreadSafeExtensionProxy::initializeExtension(const std::string& uri)
{
    std::lock_guard<std::mutex> lock(mInitializationMutex);
    if (mInitialized) return true;
    mInitialized = true;

    ThreadSafeExtensionProxyWPtr weakThis = shared_from_this();
    mExtension->registerLiveDataUpdateCallback([weakThis](const ActivityDescriptor& activity, const rapidjson::Value& liveDataUpdate) {
        auto self = weakThis.lock();
        if (!self) return;

        LiveDataCallbacks callbacks = self->ensureActivityContext(activity)->liveDataCallbacks;
        for (const auto& callback : callbacks) {
            callback(activity, liveDataUpdate);
        }
    });

    mExtension->registerEventCallback([weakThis](const ActivityDescriptor& activity, const rapidjson::Value& event) {
        auto self = weakThis.lock();
        if (!self) return;

        EventCallbacks callbacks = self->ensureActivityContext(activity)->eventCallbacks;
        for (const auto& callback : callbacks) {
            callback(activity, event);
        }
    });

    return true;
}

bool
ThreadSafeExtensionProxy::isInitialized(const std::string& uri) const
{
    std::lock_guard<std::mutex> lock(mInitializationMutex);
    return mInitialized;
}

bool
ThreadSafeExtensionProxy::invokeCommand(const ActivityDescriptor& activity,
                                       const rapidjson::Value& command,
                                       CommandSuccessActivityCallback&& success,
                                       CommandFailureActivityCallback&& error)
{
    auto cmd = std::make_shared<rapidjson::Document>();
    cmd->CopyFrom(command, cmd->GetAllocator());
    auto task = [activity, cmd, success, error](const ExtensionPtr& extension) {
        auto commandId = (int) Command::ID().Get(*cmd)->GetDouble();
        auto result = extension->invokeCommand(activity, *cmd);
        if (!result) {
            rapidjson::Document fail = CommandFailure("1.0")
                                           .uri(activity.getURI())
                                           .id(commandId)
                                           .errorCode(kErrorFailedCommand)
                                           .errorMessage(sErrorMessage[kErrorFailedCommand] + std::to_string(commandId));
            error(activity, fail);
            return;
        }

        rapidjson::Document win = CommandSuccess("1.0")
                                     .uri(activity.getURI())
                                     .id(commandId);
        success(activity, win);
    };
    enqueueTaskOnExtension(task);

    return true;
}

void
ThreadSafeExtensionProxy::registerEventCallback(const ActivityDescriptor& activity, Extension::EventActivityCallback&& callback)
{
    auto activityContext = ensureActivityContext(activity);
    activityContext->eventCallbacks.emplace_back(callback);
}

void
ThreadSafeExtensionProxy::registerLiveDataUpdateCallback(const ActivityDescriptor& activity, Extension::LiveDataUpdateActivityCallback&& callback)
{
    auto activityContext = ensureActivityContext(activity);
    activityContext->liveDataCallbacks.emplace_back(callback);
}

void
ThreadSafeExtensionProxy::onRegistered(const ActivityDescriptor& activity)
{
    enqueueTaskOnExtension([activity](const ExtensionPtr& extension) {
        extension->onActivityRegistered(activity);
    });
}

void
ThreadSafeExtensionProxy::onUnregistered(const ActivityDescriptor& activity)
{
    std::lock_guard<std::mutex> lock(mActivitiesMutex);
    mActivities.erase(activity);

    enqueueTaskOnExtension([activity](const ExtensionPtr& extension) {
        extension->onActivityUnregistered(activity);
    });
}

bool
ThreadSafeExtensionProxy::sendComponentMessage(const ActivityDescriptor &activity, const rapidjson::Value &message)
{
    auto msg = std::make_shared<rapidjson::Document>();
    msg->CopyFrom(message, msg->GetAllocator());
    enqueueTaskOnExtension([activity, msg](const ExtensionPtr& extension) {
        extension->updateComponent(activity, *msg);
    });
    return true;
}

void
ThreadSafeExtensionProxy::onResourceReady(const ActivityDescriptor& activity, const ResourceHolderPtr& resourceHolder)
{
    enqueueTaskOnExtension([activity, resourceHolder](const ExtensionPtr& extension) {
        extension->onResourceReady(activity, resourceHolder);
    });
}

void
ThreadSafeExtensionProxy::onSessionStarted(const SessionDescriptor& session)
{
    enqueueTaskOnExtension([session](const ExtensionPtr& extension) {
        extension->onSessionStarted(session);
    });
}

void
ThreadSafeExtensionProxy::onSessionEnded(const SessionDescriptor& session)
{
    enqueueTaskOnExtension([session](const ExtensionPtr& extension) {
        extension->onSessionEnded(session);
    });
}

void
ThreadSafeExtensionProxy::onForeground(const ActivityDescriptor& activity)
{
    enqueueTaskOnExtension([activity](const ExtensionPtr& extension) {
        extension->onForeground(activity);
    });
}

void
ThreadSafeExtensionProxy::onBackground(const ActivityDescriptor& activity)
{
    enqueueTaskOnExtension([activity](const ExtensionPtr& extension) {
        extension->onBackground(activity);
    });
}

void
ThreadSafeExtensionProxy::onHidden(const ActivityDescriptor& activity)
{
    enqueueTaskOnExtension([activity](const ExtensionPtr& extension) {
        extension->onHidden(activity);
    });
}

void 
ThreadSafeExtensionProxy::enqueueTaskOnExtension(std::function<void(const ExtensionPtr&)> task)
{
    ThreadSafeExtensionProxyWPtr weakThis = shared_from_this();
    mExecutor->enqueueTask([weakThis, task]() {
        auto self = weakThis.lock();
        if (!self) return;
        
        task(self->mExtension);
    });
}

ThreadSafeExtensionProxy::ActivityContextPtr
ThreadSafeExtensionProxy::ensureActivityContext(const alexaext::ActivityDescriptor& activity)
{
    std::lock_guard<std::mutex> lock(mActivitiesMutex);
    auto it = mActivities.find(activity);
    if (it == mActivities.end()) {
        mActivities.emplace(activity, std::make_shared<ActivityContext>());
    }

    return mActivities.find(activity)->second;
}

} // namespace alexaext