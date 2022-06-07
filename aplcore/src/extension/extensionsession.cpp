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

#ifdef ALEXAEXTENSIONS

#include "apl/extension/extensionsession.h"

namespace apl {

void
ExtensionSession::end() {
    if (mEnded) return;

    mEnded = true;

    if (mSessionEndedCallback)
        mSessionEndedCallback(*this);
}

void
ExtensionSession::onSessionEnded(ExtensionSession::SessionEndedCallback&& callback) {
    mSessionEndedCallback = std::move(callback);

    if (hasEnded() && mSessionEndedCallback) {
        mSessionEndedCallback(*this);
    }
}

ExtensionSessionPtr
ExtensionSession::create(const alexaext::SessionDescriptorPtr& sessionDescriptor) {
    if (!sessionDescriptor) return nullptr;

    return std::make_shared<ExtensionSession>(sessionDescriptor);
}

std::shared_ptr<ExtensionSession>
ExtensionSession::create() {
    return create(alexaext::SessionDescriptor::create());
}

} // namespace apl

#endif //ALEXAEXTENSIONS
