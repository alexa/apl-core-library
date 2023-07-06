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

#include <gtest/gtest.h>

#include <apl/apl.h>

TEST(ExtensionSessionTest, CreatesSessionsWithUniqueDescriptors) {
    auto session1 = apl::ExtensionSession::create();
    auto session2 = apl::ExtensionSession::create();

    ASSERT_NE(session1->getSessionDescriptor(), session2->getSessionDescriptor());
}

TEST(ExtensionSessionTest, CreatesSessionsWithSpecifiedDescriptors) {
    auto descriptor = alexaext::SessionDescriptor::create();
    auto session = apl::ExtensionSession::create(descriptor);

    ASSERT_EQ(descriptor, session->getSessionDescriptor());
}

#endif //ALEXAEXTENSIONS