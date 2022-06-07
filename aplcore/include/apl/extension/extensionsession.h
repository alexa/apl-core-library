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
#ifndef _APL_EXTENSION_SESSION_H
#define _APL_EXTENSION_SESSION_H

#include <memory>

#include <alexaext/alexaext.h>

#include "apl/utils/noncopyable.h"

namespace apl {

class ExtensionMediator;
class ExtensionSessionState;

/**
 * Represents an extension session, as exposed to an APL viewhost. This class primarily exists in
 * order to associate state with a session descriptor.
 */
class ExtensionSession final : public NonCopyable {
public:
    using SessionEndedCallback = std::function<void(ExtensionSession&)>;

    /**
     * Use create
     */
    ExtensionSession(const alexaext::SessionDescriptorPtr& sessionDescriptor)
        : mSessionDescriptor(sessionDescriptor)
    {}

    /**
     * @return A new extension session with a unique descriptor
     */
    static std::shared_ptr<ExtensionSession> create();

    /**
     * Creates a new extension session from the specificied descriptor. This returns @c nullptr
     * if the provided descriptor is null.
     *
     * @return A new extension session with the specified descriptor
     */
    static std::shared_ptr<ExtensionSession> create(const alexaext::SessionDescriptorPtr& sessionDescriptor);

    /**
     * @return The ID of the underlying session descriptor, for convenience.
     */
    const alexaext::SessionId& getId() const { return mSessionDescriptor->getId(); }

    /**
     * @return the session descriptor associated with this instance.
     */
    alexaext::SessionDescriptorPtr getSessionDescriptor() const { return mSessionDescriptor; }

    /**
     * @return @true if the session has been marked as ended, @c false otherwise
     */
    bool hasEnded() const { return mEnded; }

    /**
     * Marks the session as ended. If a callback has been registered, it will be invoked before
     * this call returns.
     */
    void end();

    /**
     * Registers a callback to be invoked when the session has ended. If the session has already
     * ended, the callback is invoked immediately, before this method returns.
     *
     * @param callback The callback to be invoked.
     */
    void onSessionEnded(SessionEndedCallback&& callback);

private:
    friend class ExtensionMediator;

    void setSessionState(const std::shared_ptr<ExtensionSessionState>& state) { mState = state; }
    std::shared_ptr<ExtensionSessionState> getSessionState() const { return mState; }

private:
    alexaext::SessionDescriptorPtr mSessionDescriptor;
    bool mEnded = false;
    SessionEndedCallback mSessionEndedCallback = nullptr;
    std::shared_ptr<ExtensionSessionState> mState;
};

using ExtensionSessionPtr = std::shared_ptr<ExtensionSession>;

} // namespace apl

#endif //_APL_EXTENSION_SESSION_H
#endif //ALEXAEXTENSIONS
