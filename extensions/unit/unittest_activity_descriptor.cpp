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

#include <functional>

#include <alexaext/alexaext.h>
#include <gtest/gtest.h>

namespace {

static const char* URI = "aplext:test1:10";
static const char* OTHER_URI = "aplext:test2:10";

}

TEST(ActivityDescriptorTest, GeneratesUniqueIds) {
    auto session = alexaext::SessionDescriptor::create();
    auto activity1 = alexaext::ActivityDescriptor::create(URI, session);
    auto activity2 = alexaext::ActivityDescriptor::create(URI, session);

    ASSERT_NE(activity1->getId(), activity2->getId());

    ASSERT_EQ(URI, activity1->getURI());
    ASSERT_EQ(session, activity1->getSession());

    ASSERT_EQ(URI, activity2->getURI());
    ASSERT_EQ(session, activity2->getSession());

    ASSERT_TRUE(*activity1 != *activity2);
    ASSERT_FALSE(*activity1 == *activity2);
}

TEST(ActivityDescriptorTest, AcceptsExternalId) {
    auto session = alexaext::SessionDescriptor::create();
    auto externalId = "unittest-id";
    auto activity = alexaext::ActivityDescriptor::create(URI, session, externalId);

    ASSERT_EQ(URI, activity->getURI());
    ASSERT_EQ(session, activity->getSession());
    ASSERT_EQ(externalId, activity->getId());

    auto identical = alexaext::ActivityDescriptor::create(URI, session, externalId);
    ASSERT_TRUE(*identical == *activity);
    ASSERT_FALSE(*identical != *activity);
}

TEST(ActivityDescriptorTest, IsCopyable) {
    auto session = alexaext::SessionDescriptor::create();
    auto activity = alexaext::ActivityDescriptor::create(URI, session);
    alexaext::ActivityDescriptor copy = *activity;

    ASSERT_TRUE(copy == *activity);
}

TEST(ActivityDescriptorTest, IsMovable) {
    auto session = alexaext::SessionDescriptor::create();
    auto externalId = "unittest-id";
    alexaext::ActivityDescriptor moved = alexaext::ActivityDescriptor(URI, session, externalId);

    ASSERT_EQ(externalId, moved.getId());
    ASSERT_EQ(URI, moved.getURI());
    ASSERT_EQ(session, moved.getSession());
}

TEST(ActivityDescriptorTest, ComparesSessionsById) {
    auto session = alexaext::SessionDescriptor::create();
    auto sessionCopy = alexaext::SessionDescriptor::create(session->getId());

    auto activity1 = alexaext::ActivityDescriptor::create(URI, session, "unittest-id");
    auto activity2 = alexaext::ActivityDescriptor::create(URI, sessionCopy, "unittest-id");

    ASSERT_TRUE(*activity1 == *activity2);
    ASSERT_FALSE(*activity1 != *activity2);
}

TEST(ActivityDescriptorTest, IsHashable) {
    auto session1 = alexaext::SessionDescriptor::create();
    auto session2 = alexaext::SessionDescriptor::create();
    auto externalId = "unittest-id";
    auto activity1 = alexaext::ActivityDescriptor::create(URI, session1, externalId);
    auto activity2 = alexaext::ActivityDescriptor::create(URI, session1, externalId);

    alexaext::ActivityDescriptor::Hash hash;
    ASSERT_EQ(hash(*activity1), hash(*activity2));

    // Different session should produce a different hash
    activity2 = alexaext::ActivityDescriptor::create(URI, session2, externalId);
    ASSERT_NE(hash(*activity1), hash(*activity2));

    // Different URI should produce a different hash
    activity2 = alexaext::ActivityDescriptor::create(OTHER_URI, session1, externalId);
    ASSERT_NE(hash(*activity1), hash(*activity2));

    // Different ID should produce a different hash
    activity2 = alexaext::ActivityDescriptor::create(URI, session1, "other-id");
    ASSERT_NE(hash(*activity1), hash(*activity2));

    // Handles null session
    activity2 = alexaext::ActivityDescriptor::create(URI, nullptr, externalId);
    ASSERT_NE(hash(*activity1), hash(*activity2));
}

TEST(ActivityDescriptorTest, IsComparable) {
    auto session1 = alexaext::SessionDescriptor::create("abc");
    auto session2 = alexaext::SessionDescriptor::create("def");
    auto externalId = "test-id-1";
    auto activity1 = alexaext::ActivityDescriptor::create(URI, session1, externalId);
    auto activity2 = alexaext::ActivityDescriptor::create(URI, session1, externalId);

    alexaext::ActivityDescriptor::Compare compare;
    // By contract, identical objects should compare false
    ASSERT_FALSE(compare(*activity1, *activity2));

    // Give activity2 a session that is greater than activity1's
    activity2 = alexaext::ActivityDescriptor::create(URI, session2, externalId);
    ASSERT_TRUE(compare(*activity1, *activity2));
    ASSERT_FALSE(compare(*activity2, *activity1));

    // Give activity2 a URI that is greater than activity1's
    activity2 = alexaext::ActivityDescriptor::create(OTHER_URI, session1, externalId);
    ASSERT_TRUE(compare(*activity1, *activity2));
    ASSERT_FALSE(compare(*activity2, *activity1));

    activity2 = alexaext::ActivityDescriptor::create(OTHER_URI, session1, "test-id-2");
    ASSERT_TRUE(compare(*activity1, *activity2));
    ASSERT_FALSE(compare(*activity2, *activity1));

    activity2 = alexaext::ActivityDescriptor::create(OTHER_URI, session2, "test-id-2");
    ASSERT_TRUE(compare(*activity1, *activity2));
    ASSERT_FALSE(compare(*activity2, *activity1));

    // Give activity2 an ID that is greater than activity1's
    activity2 = alexaext::ActivityDescriptor::create(URI, session1, "test-id-2");
    ASSERT_TRUE(compare(*activity1, *activity2));
    ASSERT_FALSE(compare(*activity2, *activity1));

    activity2 = alexaext::ActivityDescriptor::create(URI, session2, "test-id-2");
    ASSERT_TRUE(compare(*activity1, *activity2));
    ASSERT_FALSE(compare(*activity2, *activity1));

    // Handles null session
    activity2 = alexaext::ActivityDescriptor::create(URI, nullptr, externalId);
    ASSERT_FALSE(compare(*activity1, *activity2));
    ASSERT_TRUE(compare(*activity2, *activity1));

    // Both activities are identical with a null session
    activity1 = alexaext::ActivityDescriptor::create(URI, nullptr, externalId);
    ASSERT_FALSE(compare(*activity1, *activity2));
    ASSERT_FALSE(compare(*activity2, *activity1));

    activity1 = alexaext::ActivityDescriptor::create(URI, session2, "test-id-2");
    activity2 = alexaext::ActivityDescriptor::create(OTHER_URI, session1, "test-id-1");
    ASSERT_TRUE(compare(*activity1, *activity2));
    ASSERT_FALSE(compare(*activity2, *activity1));


    activity1 = alexaext::ActivityDescriptor::create(URI, session2, "test-id-1");
    activity2 = alexaext::ActivityDescriptor::create(URI, session1, "test-id-2");
    ASSERT_TRUE(compare(*activity1, *activity2));
    ASSERT_FALSE(compare(*activity2, *activity1));
}
