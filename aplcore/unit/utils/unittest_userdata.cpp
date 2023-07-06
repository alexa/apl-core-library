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

#include "../testeventloop.h"

using namespace apl;

class TestClassA : public UserData<TestClassA> {};
class TestClassB : public UserData<TestClassB> {};

class UserDataTest : public ::testing::Test {
public:
    void TearDown() {
#ifdef USER_DATA_RELEASE_CALLBACKS
        // Clear out any release callbacks that have been set
        TestClassA::setUserDataReleaseCallback(nullptr);
        TestClassB::setUserDataReleaseCallback(nullptr);
#endif
    }
};

TEST_F(UserDataTest, Base) {
    TestClassA a;
    TestClassB b;

    a.setUserData((void *)100);
    b.setUserData((void *)200);

    ASSERT_EQ((void *)100, a.getUserData());
    ASSERT_EQ((void *)200, b.getUserData());
}

#ifdef USER_DATA_RELEASE_CALLBACKS

// Verify that the release callback executes.
TEST_F(UserDataTest, ReleaseCallback) {
    auto released = std::vector<void *>();
    TestClassA::setUserDataReleaseCallback([&](void* data) { released.emplace_back(data); });

    // Create a TestClassA object and assign user data
    auto a = std::make_shared<TestClassA>();
    a->setUserData((void*)256);

    ASSERT_EQ((void*)256, a->getUserData());
    ASSERT_TRUE(released.empty());  // No release calls yet

    // Resetting the smart pointer will release the user data
    a.reset();

    auto expected = std::vector<void *>{(void *)256};
    ASSERT_EQ(expected, released);
}

// Verify that each release callback is specific to a class
TEST_F(UserDataTest, DeleteFunctionTwoClasses) {
    auto aList = std::vector<void*>();
    auto bList = std::vector<void*>();

    TestClassA::setUserDataReleaseCallback([&](void* data) { aList.emplace_back(data); });
    TestClassB::setUserDataReleaseCallback([&](void* data) { bList.emplace_back(data); });

    // Create an "A" and throw it away
    auto a = std::make_shared<TestClassA>();
    a->setUserData((void*)512);

    ASSERT_EQ((void*)512, a->getUserData());
    ASSERT_TRUE(aList.empty());
    ASSERT_TRUE(bList.empty());

    a.reset();
    ASSERT_EQ(1, aList.size());
    ASSERT_EQ(0, bList.size());
    ASSERT_EQ((void *)512, aList.front());
    aList.clear();

    // Create several "B" and throw them away
    std::make_shared<TestClassB>()->setUserData((void *)100);
    std::make_shared<TestClassB>()->setUserData((void *)200);
    std::make_shared<TestClassB>()->setUserData((void *)300);

    auto expected = std::vector<void *>{(void *)100, (void *)200, (void *)300};

    ASSERT_TRUE(aList.empty());
    ASSERT_EQ(expected, bList);
    bList.clear();

    // Interleave a bit just to double check
    std::make_shared<TestClassA>()->setUserData((void *)100);
    std::make_shared<TestClassB>()->setUserData((void *)200);
    std::make_shared<TestClassA>()->setUserData((void *)300);
    std::make_shared<TestClassB>()->setUserData((void *)400);

    auto expectedA = std::vector<void *>{(void *)100, (void *)300};
    auto expectedB = std::vector<void *>{(void *)200, (void *)400};

    ASSERT_EQ(expectedA, aList);
    ASSERT_EQ(expectedB, bList);
}

#endif
