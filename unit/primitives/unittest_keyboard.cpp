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

#include <algorithm>

#include "gtest/gtest.h"
#include "apl/primitives/keyboard.h"

using namespace apl;

/**
 * Test Keyboard construction.
 */
TEST(Keyboard, Construction) {

    auto kb = Keyboard("Code", "Key");
    ASSERT_EQ("Code", kb.getCode());
    ASSERT_EQ("Key", kb.getKey());
    ASSERT_FALSE(kb.isRepeat());
    ASSERT_FALSE(kb.isAltKey());
    ASSERT_FALSE(kb.isCtrlKey());
    ASSERT_FALSE(kb.isMetaKey());
    ASSERT_FALSE(kb.isShiftKey());

    // test that values are not transposed by setting each individually
    for (int i = 0; i < 5; i++) {

        auto kboom = Keyboard("Boom", "BoomBoom").repeat(i == 0)
                .alt(i == 1).ctrl(i == 2).meta(i == 3).shift(i == 4);
        ASSERT_EQ("Boom", kboom.getCode());
        ASSERT_EQ("BoomBoom", kboom.getKey());
        ASSERT_EQ(i == 0, kboom.isRepeat());
        ASSERT_EQ(i == 1, kboom.isAltKey());
        ASSERT_EQ(i == 2, kboom.isCtrlKey());
        ASSERT_EQ(i == 3, kboom.isMetaKey());
        ASSERT_EQ(i == 4, kboom.isShiftKey());
    }
}

TEST(Keyboard, KeyEquals) {

    // test that values are not transposed by setting each individually
    for (int i = 0; i < 5; i++) {

        auto k1 = Keyboard("Any", "Any").repeat(i == 0)
                .alt(i == 1).ctrl(i == 2).meta(i == 3).shift(i == 4);
        auto k2 = Keyboard("Any", "Any").repeat(i != 0)
                .alt(i == 1).ctrl(i == 2).meta(i == 3).shift(i == 4);
        ASSERT_TRUE(k1.keyEquals(k2));
        ASSERT_TRUE(k2.keyEquals(k1));
    }

    // negative
    for (int i = 1; i < 5; i++) {

        auto k1 = Keyboard("Any", "Any");
        auto k2 = Keyboard("Any", "Any").repeat(false)
                .alt(i == 1).ctrl(i == 2).meta(i == 3).shift(i == 4);

        ASSERT_FALSE(k1.keyEquals(k2));
        ASSERT_FALSE(k2.keyEquals(k1));
    }
}

/**
  * Test reserved keys are recognized.
  */
TEST(Keyboard, Reserved) {
    ASSERT_TRUE(Keyboard::BACK_KEY().isReservedKey());
    ASSERT_TRUE(Keyboard::PAGE_UP_KEY().isReservedKey());
    ASSERT_TRUE(Keyboard::PAGE_DOWN_KEY().isReservedKey());
    ASSERT_TRUE(Keyboard::HOME_KEY().isReservedKey());
    ASSERT_TRUE(Keyboard::END_KEY().isReservedKey());

    // False for random key
    ASSERT_FALSE(Keyboard("No", "No").isReservedKey());

    // True when user creates duplicate of intrinsic
    ASSERT_TRUE(Keyboard("Enter", "Enter").isIntrinsicKey());

}

