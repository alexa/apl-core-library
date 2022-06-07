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

TEST(SessionDescriptorTest, HasUniqueId) {
    auto session1 = alexaext::SessionDescriptor::create();
    auto session2 = alexaext::SessionDescriptor::create();

    ASSERT_NE(session1->getId(), session2->getId());
    ASSERT_TRUE(*session1 != *session2);
    ASSERT_FALSE(*session1 == *session2);
}

TEST(SessionDescriptorTest, CanBeDeserialized) {
    auto session1 = alexaext::SessionDescriptor::create();
    auto session2 = alexaext::SessionDescriptor::create(session1->getId());

    ASSERT_EQ(session1->getId(), session2->getId());
    ASSERT_TRUE(*session1 == *session2);
    ASSERT_FALSE(*session1 != *session2);
}

TEST(SessionDescriptorTest, IsCopyable) {
    auto session = alexaext::SessionDescriptor::create();
    alexaext::SessionDescriptor copy = *session;

    ASSERT_TRUE(copy == *session);
}

TEST(SessionDescriptorTest, IsMovable) {
    alexaext::SessionDescriptor moved = alexaext::SessionDescriptor("unittest-id");

    ASSERT_EQ("unittest-id", moved.getId());
}

TEST(SessionDescriptorTest, IsHashable) {
    auto session1 = alexaext::SessionDescriptor::create();
    auto session2 = alexaext::SessionDescriptor::create();

    alexaext::SessionDescriptor::Hash hash;
    ASSERT_NE(hash(*session1), hash(*session2));
}

TEST(SessionDescriptorTest, IsComparable) {
    auto session1 = alexaext::SessionDescriptor::create("abc");
    auto session2 = alexaext::SessionDescriptor::create("def");

    alexaext::SessionDescriptor::Compare compare;
    ASSERT_TRUE(compare(*session1, *session2));
    ASSERT_FALSE(compare(*session2, *session1));

    // By contract, identical objects should compare as false
    ASSERT_FALSE(compare(*session1, *session1));
}
