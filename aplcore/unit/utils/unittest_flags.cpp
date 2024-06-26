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

#include "gtest/gtest.h"

#include "apl/utils/flags.h"

using namespace apl;

class FlagsTest : public ::testing::Test {};

enum TestFlags : uint8_t {
    kTestFlag0 = 1u << 0,
    kTestFlag1 = 1u << 1,
    kTestFlag2 = 1u << 2,
    kTestFlag3 = 1u << 3,
    kTestFlag4 = 1u << 4,
};

TEST_F(FlagsTest, EmptyStart)
{
    auto testFlags = Flags<TestFlags>();

    // Nothing is set
    ASSERT_FALSE(testFlags.isSet(kTestFlag0));
    ASSERT_FALSE(testFlags.isSet(kTestFlag1));
    ASSERT_FALSE(testFlags.isSet(kTestFlag2));
    ASSERT_FALSE(testFlags.isSet(kTestFlag3));

    testFlags.set(kTestFlag1);
    testFlags.set(kTestFlag2);
    ASSERT_TRUE(testFlags.isSet(kTestFlag1));
    ASSERT_TRUE(testFlags.isSet(kTestFlag2));

    testFlags.clear(kTestFlag1);
    ASSERT_FALSE(testFlags.isSet(kTestFlag1));
}

TEST_F(FlagsTest, PrePopulate)
{
    auto testFlags = Flags<TestFlags>(kTestFlag0 | kTestFlag2 | kTestFlag4);

    // Nothing is set
    ASSERT_TRUE(testFlags.isSet(kTestFlag0));
    ASSERT_FALSE(testFlags.isSet(kTestFlag1));
    ASSERT_TRUE(testFlags.isSet(kTestFlag2));
    ASSERT_FALSE(testFlags.isSet(kTestFlag3));
    ASSERT_TRUE(testFlags.isSet(kTestFlag4));
}

TEST_F(FlagsTest, BiggerStorage)
{
    enum TestFlags16 : uint16_t {
        kTestFlag16_0 = 1u << 0,
        kTestFlag16_7 = 1u << 7,
        kTestFlag16_15 = 1u << 15
    };

    auto testFlags16 = Flags<TestFlags16>();

    testFlags16.set(kTestFlag16_0);
    testFlags16.set(kTestFlag16_7);
    testFlags16.set(kTestFlag16_15);

    // Nothing is set
    ASSERT_TRUE(testFlags16.isSet(kTestFlag16_0));
    ASSERT_TRUE(testFlags16.isSet(kTestFlag16_7));
    ASSERT_TRUE(testFlags16.isSet(kTestFlag16_15));


    enum TestFlags32 : uint32_t {
        kTestFlag32_0 = 1u << 0,
        kTestFlag32_15 = 1u << 7,
        kTestFlag32_31 = 1u << 31
    };

    auto testFlags32 = Flags<TestFlags32>();

    testFlags32.set(kTestFlag32_0);
    testFlags32.set(kTestFlag32_15);
    testFlags32.set(kTestFlag32_31);

    // Nothing is set
    ASSERT_TRUE(testFlags32.isSet(kTestFlag32_0));
    ASSERT_TRUE(testFlags32.isSet(kTestFlag32_15));
    ASSERT_TRUE(testFlags32.isSet(kTestFlag32_31));
}

TEST_F(FlagsTest, CheckAndClear)
{
    auto testFlags = Flags<TestFlags>(0xFF);

    // Everything is set
    ASSERT_TRUE(testFlags.isSet(kTestFlag2));
    ASSERT_TRUE(testFlags.checkAndClear(kTestFlag2));
    ASSERT_FALSE(testFlags.isSet(kTestFlag2));
}