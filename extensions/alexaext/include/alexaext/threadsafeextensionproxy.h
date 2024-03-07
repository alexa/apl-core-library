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

#ifndef APL_THREADSAFEEXTENSIONPROXY_H
#define APL_THREADSAFEEXTENSIONPROXY_H

#include <map>
#include <memory>
#include <mutex>

#include "executor.h"
#include "extensionproxy.h"

namespace alexaext {

// forward declare
class ThreadSafeExtensionProxy;
using ThreadSafeExtensionProxyPtr = std::shared_ptr<ThreadSafeExtensionProxy>;
using ThreadSafeExtensionProxyWPtr = std::weak_ptr<ThreadSafeExtensionProxy>;

/**
 * A thread safe implementation of ExtensionProxy. This class can be invoked on multiple threads and forwards events to
 * the extension through an executor. The executor should run tasks serially on a background thread to avoid blocking the
 * core processing thread.
 *
 * Note: this implementation only invokes Activity-based apis for Extensions.
 */
class ThreadSafeExtensionProxy : public ExtensionProxy,
                                 public std::enable_shared_from_this<ThreadSafeExtensionProxy> {
public:
    /**
     * Create a shared ptr to a ThreadSafeExtensionProxy.
     *
     * @param extension the extension to delegate calls to.
     * @param executor  the executor to run extension functions on. Defaults to synchronous execution.
     * @return
     */
    static ThreadSafeExtensionProxyPtr create(const ExtensionPtr& extension, const ExecutorPtr& executor = Executor::getSynchronousExecutor()) { return std::make_shared<ThreadSafeExtensionProxy>(extension, executor); }

    /**
     * Constructor. Use @code create as this object inherits from std::enable_shared_from_this.
     *
     * @param extension the extension to delegate calls to.
     * @param executor  the executor to run extension functions on.
     */
    explicit ThreadSafeExtensionProxy(const ExtensionPtr& extension, const ExecutorPtr& executor);

    std::set<std::string> getURIs() const final { return mExtension->getURIs(); };
    bool getRegistration(const ActivityDescriptor& activity,
                         const rapidjson::Value& registrationRequest,
                         RegistrationSuccessActivityCallback&& success,
                         RegistrationFailureActivityCallback&& error) final;
    bool initializeExtension(const std::string& uri) final;
    bool isInitialized(const std::string& uri) const final;
    bool invokeCommand(const ActivityDescriptor& activity,
                       const rapidjson::Value& command,
                       CommandSuccessActivityCallback&& success,
                       CommandFailureActivityCallback&& error) final;
    void registerEventCallback(const ActivityDescriptor& activity, Extension::EventActivityCallback&& callback) final;
    void registerLiveDataUpdateCallback(const ActivityDescriptor& activity, Extension::LiveDataUpdateActivityCallback&& callback) final;
    void onRegistered(const ActivityDescriptor& activity) final;
    void onUnregistered(const ActivityDescriptor& activity) final;
    bool sendComponentMessage(const ActivityDescriptor &activity, const rapidjson::Value &message) final;
    void onResourceReady(const ActivityDescriptor& activity, const ResourceHolderPtr& resourceHolder) final;
    void onSessionStarted(const SessionDescriptor& session) final;
    void onSessionEnded(const SessionDescriptor& session) final;
    void onForeground(const ActivityDescriptor& activity) final;
    void onBackground(const ActivityDescriptor& activity) final;
    void onHidden(const ActivityDescriptor& activity) final;

private:
    using EventCallbacks = std::vector<Extension::EventActivityCallback>;
    using LiveDataCallbacks = std::vector<Extension::LiveDataUpdateActivityCallback>;
    struct ActivityContext {
        EventCallbacks eventCallbacks;
        LiveDataCallbacks liveDataCallbacks;
    };
    using ActivityContextPtr = std::shared_ptr<ActivityContext>;

    void enqueueTaskOnExtension(std::function<void(const ExtensionPtr& extension)> task);
    ActivityContextPtr ensureActivityContext(const ActivityDescriptor& activity);

private:
    const ExtensionPtr mExtension;
    const ExecutorPtr mExecutor;
    mutable std::mutex mInitializationMutex;
    bool mInitialized = false;
    std::mutex mActivitiesMutex;
    std::map<ActivityDescriptor, std::shared_ptr<ActivityContext>, ActivityDescriptor::Compare> mActivities;
};

} // namespace alexaext

#endif // APL_THREADSAFEEXTENSIONPROXY_H
