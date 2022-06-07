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

#ifndef _ALEXAEXT_SESSION_DESCRIPTOR_H
#define _ALEXAEXT_SESSION_DESCRIPTOR_H

#include <memory>

#include "types.h"

namespace alexaext {

/**
 * Represents an extension session, i.e. a group of related activities.
 *
 * Session descriptors are immutable and hashable, so they are
 * suitable to use as keys in unordered maps or other hashing data structures.
 *
 * @see SessionId
 */
class SessionDescriptor final {
public:
    /**
     * Use create
     */
    SessionDescriptor();

    /**
     * Use create
     */
    explicit SessionDescriptor(const SessionId& sessionId);
    ~SessionDescriptor() = default;

    /**
     * Creates a session descriptor with a randomly generated ID.
     *
     * @return The session descriptor
     */
    static std::shared_ptr<SessionDescriptor> create() { return std::make_shared<SessionDescriptor>(); }

    /**
     * Creates a session descriptor with the specified ID. This is only intended to be used for
     * situations where a session needs to be serialized/deserialized. Prefer using the no-arg
     * variant to create a new original session descriptor.
     *
     * @return The session descriptor
     */
    static std::shared_ptr<SessionDescriptor> create(const SessionId& sessionId) { return std::make_shared<SessionDescriptor>(sessionId); }

    /**
     * Returns the globally unique identifier for the current session.
     *
     * @return The globally unique identifier for this session.
     */
    const SessionId& getId() const { return mSessionId; }

    bool operator==(const SessionDescriptor& other) const {
        return mSessionId == other.mSessionId;
    }

    bool operator!=(const SessionDescriptor& rhs) const { return !(rhs == *this); }

    /**
     * Provides hashing for session descriptors so they can easily be used as unordered map keys.
     */
    class Hash final {
    public:
        std::size_t operator()(const SessionDescriptor& descriptor) const {
            static std::hash<std::string> stringHash;
            // Variant of Apache Commons HashCodeBuilder hashing algorithm
            std::size_t hash = 17;
            hash = hash * 37 + stringHash(descriptor.getId());
            return hash;
        }
    };

    /**
     * Provides comparison for session descriptors so they can easily be used as ordered map keys.
     */
    class Compare final {
    public:
        bool operator()(const SessionDescriptor& first, const SessionDescriptor& second) const {
            return first.getId() < second.getId();
        }
    };

private:
    SessionId mSessionId;
};

using SessionDescriptorPtr = std::shared_ptr<SessionDescriptor>;

} // namespace alexaext

#endif // _ALEXAEXT_SESSION_DESCRIPTOR_H
