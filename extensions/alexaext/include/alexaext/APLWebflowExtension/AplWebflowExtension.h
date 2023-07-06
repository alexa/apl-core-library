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

#ifndef APL_APLWEBFLOWEXTENSION_H
#define APL_APLWEBFLOWEXTENSION_H


#include <memory>
#include <string>
#include <alexaext/alexaext.h>

#include <rapidjson/document.h>

#include "AplWebflowBase.h"
#include "AplWebflowExtensionObserverInterface.h"

namespace alexaext {
namespace webflow {

static const std::string URI = "aplext:webflow:10";
static const std::string ENVIRONMENT_VERSION = "APLWebflowExtension-1.0";

/**
 * An APL Extension designed to launch a feature restricted browser that is capable of navigating
 * to a URL. This is useful for authentication and verification flows.
 *
 * This extension follows the observer model, where a common logic delegates to an observer
 * the underlying behavior.
 *
 * Because of the flow nature of the webflow extension, flows can be runtime dependant. The current model
 * allows two level of indirection.
 *
 * Extension->Observer->Flow where Observer and Flow need to implement their interfaces.
 */
class AplWebflowExtension
    : public alexaext::ExtensionBase,
      public std::enable_shared_from_this<AplWebflowExtension> {
public:

    /**
     * Constructor
     */
    explicit AplWebflowExtension(
        std::function<std::string(void)> tokenGenerator,
        std::shared_ptr<AplWebflowExtensionObserverInterface> observer,
        const std::shared_ptr<alexaext::Executor>& executor);

    ~AplWebflowExtension() override = default;

    /// @name alexaext::Extension Functions
    /// @{

    rapidjson::Document createRegistration(const ActivityDescriptor &activity,
                                           const rapidjson::Value &registrationRequest) override;

    bool invokeCommand(const ActivityDescriptor &activity, const rapidjson::Value &command) override;

    void onForeground(const ActivityDescriptor& activity) override {
        mObserver->onForeground(activity);
    }

    void onBackground(const ActivityDescriptor& activity) override {
        mObserver->onBackground(activity);
    }

    void onHidden(const ActivityDescriptor& activity) override {
        mObserver->onHidden(activity);
    }
    /// @}

private:
    /// The @c AplWebflowExtensionObserverInterface observer
    std::shared_ptr<AplWebflowExtensionObserverInterface> mObserver;

    /// The @c uuid token generator
    std::function<std::string(void)> mTokenGenerator;

    /// The @c executor to run the observer
    std::weak_ptr<alexaext::Executor> mExecutor;
};

using AplWebflowExtensionPtr = std::shared_ptr<AplWebflowExtension>;

}  // namespace webflow
}  // namespace alexaext

#endif // APL_APLWEBFLOWEXTENSION_H
