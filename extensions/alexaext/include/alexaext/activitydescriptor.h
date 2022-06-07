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

#ifndef _ALEXAEXT_ACTIVITY_DESCRIPTOR_H
#define _ALEXAEXT_ACTIVITY_DESCRIPTOR_H

#include <memory>

#include "random.h"
#include "sessiondescriptor.h"
#include "types.h"

namespace alexaext {

/**
 * Represents an activity that requests and uses functionality defined by a given extension.
 * For example, a rendering task for an APL document is a common type of activity that requests
 * APL extensions. Each activity belongs to a single extension session.
 *
 * Activity descriptors are immutable and hashable, so they are suitable to use as keys in
 * unordered maps or other hashing data structures.
 *
 * @see ActivityId
 * @see SessionId
 */
class ActivityDescriptor final {
public:
    /**
     * Constructs a new immutable activity descriptor.
     *
     * @param uri The URI of the extension as requested by the activity.
     * @param session The session associated with this activity.
     */
    ActivityDescriptor(const std::string& uri,
                       const SessionDescriptorPtr& session)
        : mURI(uri),
          mSession(session),
          mActivityId(generateBase36Token("activity-"))
    {}

    /**
     * Constructs a new immutable activity descriptor.
     *
     * @param uri The URI of the extension as requested by the activity.
     * @param session The session associated with this activity.
     * @param uniqueId The unique activity ID
     */
    ActivityDescriptor(const std::string& uri,
                       const SessionDescriptorPtr& session,
                       const ActivityId& uniqueId)
        : mURI(uri),
          mSession(session),
          mActivityId(uniqueId)
    {}
    ~ActivityDescriptor() = default;

    /**
     * Constructs a new immutable activity descriptor with a generated unique ID.
     *
     * @param uri The URI of the extension as requested by the activity.
     * @param session The session associated with this activity.
     * @return The new activity descriptor
     */
    static std::shared_ptr<ActivityDescriptor> create(const std::string& uri,
                                                      const SessionDescriptorPtr& session) {
        return std::make_shared<ActivityDescriptor>(uri, session);
    }

    /**
     * Constructs a new immutable activity descriptor with specified ID. This ID should be globally
     * unique.
     *
     * @param uri The URI of the extension as requested by the activity.
     * @param session The session associated with this activity.
     * @param uniqueId A globally unique activity ID
     * @return The new activity descriptor
     */
    static std::shared_ptr<ActivityDescriptor> create(const std::string& uri,
                                                      const SessionDescriptorPtr& session,
                                                      const ActivityId& uniqueId) {
        return std::make_shared<ActivityDescriptor>(uri, session, uniqueId);
    }

    /**
     * @return The URI of the extension as requested by the activity.
     */
    const std::string& getURI() const { return mURI; }

    /**
     * @return The session for this activity.
     */
    SessionDescriptorPtr getSession() const { return mSession; }

    /**
     * @return The identifier for this activity.
     */
    const ActivityId& getId() const { return mActivityId; }

    bool operator==(const ActivityDescriptor& other) const {
        return mURI == other.mURI && mActivityId == other.mActivityId
            && (mSession ? other.mSession && *mSession == *other.mSession
                         : other.mSession == nullptr);
    }

    bool operator!=(const ActivityDescriptor& rhs) const { return !(rhs == *this); }

    /**
     * Provides hashing for activity descriptors so they can easily be used as unordered map keys.
     */
    class Hash final {
    public:
        std::size_t operator()(const ActivityDescriptor& descriptor) const {
            static SessionDescriptor::Hash sessionHash;
            static std::hash<std::string> stringHash;
            // Variant of Apache Commons HashCodeBuilder hashing algorithm
            std::size_t hash = 17;
            hash = hash * 37 + stringHash(descriptor.getURI());
            if (auto session = descriptor.getSession()) {
                hash = hash * 37 + sessionHash(*session);
            }
            hash = hash * 37 + stringHash(descriptor.getId());
            return hash;
        }
    };

    /**
     * Provides comparison for activity descriptors so they can easily be used as ordered map keys.
     */
    class Compare final {
    public:
        bool operator()(const ActivityDescriptor& first, const ActivityDescriptor& second) const {
            static SessionDescriptor::Compare sessionCompare;

            if (first.getURI() < second.getURI()) return true;
            if (first.getURI() > second.getURI()) return false;
            if (first.getId() < second.getId()) return true;
            if (first.getId() > second.getId()) return false;

            // URI and IDs are both the same, compare sessions
            if (!first.getSession() && !second.getSession()) return false; // nullptr == nullptr
            if (!first.getSession() && second.getSession()) return true;   // nullptr < first
            if (first.getSession() && !second.getSession()) return false;  // first > nullptr
            return sessionCompare(*first.getSession(), *second.getSession());
        }
    };

private:
    std::string mURI;
    SessionDescriptorPtr mSession;
    ActivityId mActivityId;
};

using ActivityDescriptorPtr = std::shared_ptr<ActivityDescriptor>;

} // namespace alexaext

#endif // _ALEXAEXT_ACTIVITY_DESCRIPTOR_H
