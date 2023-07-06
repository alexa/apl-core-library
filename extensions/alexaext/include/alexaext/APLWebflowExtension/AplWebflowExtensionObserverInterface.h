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

#ifndef APL_APLWEBFLOWEXTENSIONOBSERVERINTERFACE_H
#define APL_APLWEBFLOWEXTENSIONOBSERVERINTERFACE_H

#include "AplWebflowBase.h"

namespace alexaext {
namespace webflow {

/**
 * This class allows a @c AplWebflowExtensionObserverInterface observer to be notified of changes in the
 * @c AplWebflowExtension
 */
class AplWebflowExtensionObserverInterface {
public:

    /**
     * Destructor
     */
    virtual ~AplWebflowExtensionObserverInterface() = default;

    /**
     * Used to notify the observer when the extension has issued a StartFlow command.
     *
     * @param activity Descriptor to give information about the activity.
     * @param token Meta-information about the webflow client.
     * @param url The https url to open in the webflow.
     * @param flowId An optional id that will be returned in OnFlowEnd event.
     * @param onFlowEndEvent when flowId is passed as parameter to the StartFlow command, EndEvent gets sent
     */
    virtual void onStartFlow(
        const ActivityDescriptor &activity,
        const std::string& token,
        const std::string& url,
        const std::string& flowId,
        std::function<void(const std::string&, const std::string&)> onFlowEndEvent =
            [](const std::string&, const std::string&){}) = 0;

    /**
     * Notifies observer when the document has come to the foreground.
     * @param activity gives information about the activity
     */
    virtual void onForeground(const ActivityDescriptor &activity) {}

    /**
     * Notifies observer when the document has gone to the background.
     * @param activity gives information about the activity
     */
    virtual void onBackground(const ActivityDescriptor &activity) {}

    /**
     * Notifies observer when the document has been hidden.
     * @param activity gives information about the activity
     */
    virtual void onHidden(const ActivityDescriptor &activity) {}
};

using AplWebflowExtensionObserverInterfacePtr = std::shared_ptr<AplWebflowExtensionObserverInterface>;

}  // namespace webflow
}  // namespace alexaext

#endif // APL_APLWEBFLOWEXTENSIONOBSERVERINTERFACE_H
